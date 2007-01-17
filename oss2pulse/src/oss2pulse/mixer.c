#include "oss2pulse.h"

static void context_success_cb(pa_context *c, int success, void *userdata) {
    fd_info *i = userdata;

    assert(c);
    assert(i);

    i->operation_success = success;
    pa_threaded_mainloop_signal(i->mainloop, 0);
}

static void sink_info_cb(pa_context *context, const pa_sink_info *si, int eol, void *userdata) {
    fd_info *i = userdata;

    if (!si && eol < 0) {
        i->operation_success = 0;
        pa_threaded_mainloop_signal(i->mainloop, 0);
        return;
    }

    if (eol)
        return;

    if (!pa_cvolume_equal(&i->sink_volume, &si->volume))
        i->volume_modify_count++;
    
    i->sink_volume = si->volume;
    i->sink_index = si->index;

    i->operation_success = 1;
    pa_threaded_mainloop_signal(i->mainloop, 0);
}

static void source_info_cb(pa_context *context, const pa_source_info *si, int eol, void *userdata) {
    fd_info *i = userdata;

    if (!si && eol < 0) {
        i->operation_success = 0;
        pa_threaded_mainloop_signal(i->mainloop, 0);
        return;
    }

    if (eol)
        return;

    if (!pa_cvolume_equal(&i->source_volume, &si->volume))
        i->volume_modify_count++;
    
    i->source_volume = si->volume;
    i->source_index = si->index;

    i->operation_success = 1;
    pa_threaded_mainloop_signal(i->mainloop, 0);
}

static void subscribe_cb(pa_context *context, pa_subscription_event_type_t t, uint32_t idx, void *userdata) {
    fd_info *i = userdata;
    pa_operation *o = NULL;

    if (i->sink_index != idx)
        return;

    if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) != PA_SUBSCRIPTION_EVENT_CHANGE)
        return;

    if (!(o = pa_context_get_sink_info_by_index(i->context, i->sink_index, sink_info_cb, i))) {
        debug(DEBUG_LEVEL_NORMAL, __FILE__": Failed to get sink info: %s", pa_strerror(pa_context_errno(i->context)));
        return;
    }

    pa_operation_unref(o);
}

static void *mixer_open_thread(void *arg){
  struct fusd_file_info *file = arg;
  fd_info *i;
  pa_operation *o = NULL;
  int ret = 0;
  
  debug(DEBUG_LEVEL_NORMAL, __FILE__": mixer_open()\n");
  
  if ((i = fd_info_new(FD_INFO_MIXER, &ret))){
  
    pa_threaded_mainloop_lock(i->mainloop);
    
    pa_context_set_subscribe_callback(i->context, subscribe_cb, i);
  
    if (!(o = pa_context_subscribe(i->context, PA_SUBSCRIPTION_MASK_SINK | PA_SUBSCRIPTION_MASK_SOURCE, context_success_cb, i))) {
      debug(DEBUG_LEVEL_NORMAL, __FILE__": Failed to subscribe to events: %s", pa_strerror(pa_context_errno(i->context)));
      ret = -EIO;
      goto fail;
    }
    
    i->operation_success = 0;
    while (pa_operation_get_state(o) != PA_OPERATION_DONE) {
      pa_threaded_mainloop_wait(i->mainloop);
      CONTEXT_CHECK_DEAD_GOTO(i, fail);
    }
    
    pa_operation_unref(o);
    o = NULL;
    
    if (!i->operation_success) {
      debug(DEBUG_LEVEL_NORMAL, __FILE__":Failed to subscribe to events: %s", pa_strerror(pa_context_errno(i->context)));
      ret = -EIO;
      goto fail;
    }
    
    /* Get sink info */
    
    if (!(o = pa_context_get_sink_info_by_name(i->context, NULL, sink_info_cb, i))) {
      debug(DEBUG_LEVEL_NORMAL, __FILE__": Failed to get sink info: %s", pa_strerror(pa_context_errno(i->context)));
      ret = -EIO;
      goto fail;
    }
    
    i->operation_success = 0;
    while (pa_operation_get_state(o) != PA_OPERATION_DONE) {
      pa_threaded_mainloop_wait(i->mainloop);
      CONTEXT_CHECK_DEAD_GOTO(i, fail);
    }
    
    pa_operation_unref(o);
    o = NULL;
    
    if (!i->operation_success) {
      debug(DEBUG_LEVEL_NORMAL, __FILE__": Failed to get sink info: %s", pa_strerror(pa_context_errno(i->context)));
      ret = -EIO;
      goto fail;
    }
    
    /* Get source info */
    
    if (!(o = pa_context_get_source_info_by_name(i->context, NULL, source_info_cb, i))) {
      debug(DEBUG_LEVEL_NORMAL, __FILE__": Failed to get source info: %s", pa_strerror(pa_context_errno(i->context)));
      ret = -EIO;
      goto fail;
    }
    
    i->operation_success = 0;
    while (pa_operation_get_state(o) != PA_OPERATION_DONE) {
      pa_threaded_mainloop_wait(i->mainloop);
      CONTEXT_CHECK_DEAD_GOTO(i, fail);
    }
    
    pa_operation_unref(o);
    o = NULL;
    
    if (!i->operation_success) {
      debug(DEBUG_LEVEL_NORMAL, __FILE__": Failed to get source info: %s", pa_strerror(pa_context_errno(i->context)));
      ret = -EIO;
      goto fail;
    }
    
    file->private_data = i;
    
    pa_threaded_mainloop_unlock(i->mainloop);
  }

  fusd_return(file, 0);
  return 0;

fail:
  if (o)
    pa_operation_unref(o);
  
  pa_threaded_mainloop_unlock(i->mainloop);
  
  if (i)
    fd_info_unref(i);
  
  if(ret)
    debug(DEBUG_LEVEL_NORMAL, __FILE__": mixer_open() failed\n");
  
  fusd_return(file, ret);
  return 0;
}

static int mixer_open(struct fusd_file_info* file){
  pthread_t dummy;
  debug(DEBUG_LEVEL_NORMAL, __FILE__": mixer_open()\n");
  if(pthread_create(&dummy,NULL,mixer_open_thread,file))
    mixer_open_thread(file);
  return -FUSD_NOREPLY;
}

static void *mixer_ioctl_thread(void *arg){
  struct fusd_file_info *file = arg;
  fd_info *i = file->private_data;
  int request = i->ioctl_request;
  void *argp = i->ioctl_argp;
  int ret = 0;
  
  switch (request) {
  case SOUND_MIXER_READ_DEVMASK :
    debug(DEBUG_LEVEL_NORMAL, __FILE__": SOUND_MIXER_READ_DEVMASK\n");
    
    *(int*) argp = SOUND_MASK_PCM | SOUND_MASK_IGAIN | SOUND_MASK_VOLUME;
    break;
    
  case SOUND_MIXER_READ_RECMASK :
    debug(DEBUG_LEVEL_NORMAL, __FILE__": SOUND_MIXER_READ_RECMASK\n");
    
    *(int*) argp = SOUND_MASK_IGAIN;
    break;
    
  case SOUND_MIXER_READ_STEREODEVS:
    debug(DEBUG_LEVEL_NORMAL, __FILE__": SOUND_MIXER_READ_STEREODEVS\n");
    
    pa_threaded_mainloop_lock(i->mainloop);
    *(int*) argp = 0;
    if (i->sink_volume.channels > 1)
      *(int*) argp |= SOUND_MASK_PCM;
    if (i->source_volume.channels > 1)
      *(int*) argp |= SOUND_MASK_IGAIN;
    pa_threaded_mainloop_unlock(i->mainloop);
    break;

  case SOUND_MIXER_READ_RECSRC:
    debug(DEBUG_LEVEL_NORMAL, __FILE__": SOUND_MIXER_READ_RECSRC\n");
    
    *(int*) argp = SOUND_MASK_IGAIN;
    break;
    
  case SOUND_MIXER_WRITE_RECSRC:
    debug(DEBUG_LEVEL_NORMAL, __FILE__": SOUND_MIXER_WRITE_RECSRC\n");
    break;
    
  case SOUND_MIXER_READ_CAPS:
    debug(DEBUG_LEVEL_NORMAL, __FILE__": SOUND_MIXER_READ_CAPS\n");
    
    *(int*) argp = 0;
    break;
    
  case SOUND_MIXER_READ_PCM:
  case SOUND_MIXER_READ_VOLUME:
    {
      pa_cvolume *v;
      
      debug(DEBUG_LEVEL_NORMAL, __FILE__": SOUND_MIXER_READ_PCM\n");
      pa_threaded_mainloop_lock(i->mainloop);

      v = &i->sink_volume;
      
      *(int*) argp =
	((v->values[0]*100/PA_VOLUME_NORM)) |
	((v->values[v->channels > 1 ? 1 : 0]*100/PA_VOLUME_NORM)  << 8);
      
      pa_threaded_mainloop_unlock(i->mainloop);
    }
    break;

  case SOUND_MIXER_READ_IGAIN: 
    {
      pa_cvolume *v;
      
      debug(DEBUG_LEVEL_NORMAL, __FILE__": SOUND_MIXER_READ_IGAIN\n");
      pa_threaded_mainloop_lock(i->mainloop);
      
      v = &i->source_volume;
      
      *(int*) argp =
	((v->values[0]*100/PA_VOLUME_NORM)) |
	((v->values[v->channels > 1 ? 1 : 0]*100/PA_VOLUME_NORM)  << 8);
      
      pa_threaded_mainloop_unlock(i->mainloop);
    }
    break;

  case SOUND_MIXER_WRITE_PCM:
  case SOUND_MIXER_WRITE_VOLUME:
    {
      pa_cvolume v, *pv;
      debug(DEBUG_LEVEL_NORMAL, __FILE__": SOUND_MIXER_WRITE_PCM\n");
      pa_threaded_mainloop_lock(i->mainloop);
      
      v = i->sink_volume;
      pv = &i->sink_volume;
      
      pv->values[0] = ((*(int*) argp & 0xFF)*PA_VOLUME_NORM)/100;
      pv->values[1] = ((*(int*) argp >> 8)*PA_VOLUME_NORM)/100;
      
      if (!pa_cvolume_equal(pv, &v)) {
	pa_operation *o;
	
	o = pa_context_set_sink_volume_by_index(i->context, i->sink_index, pv, NULL, NULL);
	
	if (!o)
	  debug(DEBUG_LEVEL_NORMAL, __FILE__":Failed set volume: %s", pa_strerror(pa_context_errno(i->context)));
	else {
	  
	  i->operation_success = 0;
	  while (pa_operation_get_state(o) != PA_OPERATION_DONE) {
	    CONTEXT_CHECK_DEAD_GOTO(i, exit_loop);
	    
	    pa_threaded_mainloop_wait(i->mainloop);
	  }
	exit_loop:
	  
	  if (!i->operation_success)
	    debug(DEBUG_LEVEL_NORMAL, __FILE__": Failed to set volume: %s\n", pa_strerror(pa_context_errno(i->context)));
	  
	  pa_operation_unref(o);
	}
	
	/* We don't wait for completion here */
	i->volume_modify_count++;
      }
      
      pa_threaded_mainloop_unlock(i->mainloop);
    }   
    break;
    
  case SOUND_MIXER_WRITE_IGAIN: 
    {
      pa_cvolume v, *pv;
      
      debug(DEBUG_LEVEL_NORMAL, __FILE__": SOUND_MIXER_WRITE_IGAIN\n");
      
      pa_threaded_mainloop_lock(i->mainloop);
      
      v = i->source_volume;
      pv = &i->source_volume;
      pv->values[0] = ((*(int*) argp & 0xFF)*PA_VOLUME_NORM)/100;
      pv->values[1] = ((*(int*) argp >> 8)*PA_VOLUME_NORM)/100;
      
      if (!pa_cvolume_equal(pv, &v)) {
	pa_operation *o;
	
	o = pa_context_set_source_volume_by_index(i->context, i->source_index, pv, NULL, NULL);
	
	if (!o)
	  debug(DEBUG_LEVEL_NORMAL, __FILE__":Failed set volume: %s", pa_strerror(pa_context_errno(i->context)));
	else {
	  
	  i->operation_success = 0;
	  while (pa_operation_get_state(o) != PA_OPERATION_DONE) {
	    CONTEXT_CHECK_DEAD_GOTO(i, exit_loop2);
	    
	    pa_threaded_mainloop_wait(i->mainloop);
	  }
	exit_loop2:
	  
	  if (!i->operation_success)
	    debug(DEBUG_LEVEL_NORMAL, __FILE__": Failed to set volume: %s\n", pa_strerror(pa_context_errno(i->context)));
	  
	  pa_operation_unref(o);
	}
	
	/* We don't wait for completion here */
	i->volume_modify_count++;
      }
      
      pa_threaded_mainloop_unlock(i->mainloop);
    }
    break;

  case SOUND_MIXER_INFO: 
    {
      mixer_info *mi = argp;
      
      debug(DEBUG_LEVEL_NORMAL, __FILE__": SOUND_MIXER_INFO\n");
      
      memset(mi, 0, sizeof(mixer_info));
      strncpy(mi->id, "PULSEAUDIO", sizeof(mi->id));
      strncpy(mi->name, "PulseAudio Virtual OSS", sizeof(mi->name));
      pa_threaded_mainloop_lock(i->mainloop);
      mi->modify_counter = i->volume_modify_count;
      pa_threaded_mainloop_unlock(i->mainloop);
    }
    break;

  default:
    debug(DEBUG_LEVEL_NORMAL, __FILE__": unknown ioctl 0x%08lx\n", request);
    
    ret = -EINVAL;
    break;
  }

  fusd_return(file, ret);
  return 0;
}

static int mixer_ioctl(struct fusd_file_info *file, int request, void *argp){
  struct fd_info* i = file->private_data;
  pthread_t dummy;

  if(i == NULL) return -EBADFD;
  if(i->unusable) return -EBADFD;

  i->ioctl_request = request;
  i->ioctl_argp = argp;
  
  if(pthread_create(&dummy,NULL,mixer_ioctl_thread,file))
    mixer_ioctl_thread(file);

  return -FUSD_NOREPLY;
}

int mixer_close(struct fusd_file_info* file){
  fd_info *i = file->private_data;
  
  debug(DEBUG_LEVEL_NORMAL, __FILE__": close()\n");
  
  fd_info_unref(i);
  
  return 0;
}

struct fusd_file_operations mixer_file_ops = {
  open: mixer_open,
  ioctl: mixer_ioctl,
  close: mixer_close
};

