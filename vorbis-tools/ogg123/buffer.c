/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU PUBLIC LICENSE 2, WHICH IS INCLUDED WITH THIS SOURCE.    *
 * PLEASE READ THESE TERMS BEFORE DISTRIBUTING.                     *
 *                                                                  *
 * THE Ogg123 SOURCE CODE IS (C) COPYRIGHT 2000-2001                *
 * by Kenneth C. Arnold <ogg@arnoldnet.net> AND OTHER CONTRIBUTORS  *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

 last mod: $Id: buffer.c,v 1.7.2.23.2.2 2001/10/15 05:56:55 volsung Exp $

 ********************************************************************/

/* buffer.c
 *  buffering code for ogg123. This is Unix-specific. Other OSes anyone?
 *
 * Thanks to Lee McLouchlin's buffer(1) for inspiration; no code from
 * that program is used in this buffer.
 */

#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>

#include <assert.h> /* for debug purposes */

#include "ogg123.h"
#include "buffer.h"

#define MIN(x,y)     ( (x) < (y) ? (x) : (y) )
#define MIN3(x,y,z)  MIN(x,MIN(y,z))

#define DEBUG_BUFFER

#ifdef DEBUG_BUFFER
FILE *debugfile;
#define DEBUG(x, y...) { if (pthread_self() != buf->thread) fprintf (debugfile, "D: " x "\n", ## y ); else fprintf (debugfile, "P: " x "\n", ## y ); }
#else
#define DEBUG(x, y...)
#endif

#define LOCK_MUTEX(mutex) { DEBUG("Locking mutex %s.", #mutex); pthread_mutex_lock (&(mutex)); }
#define UNLOCK_MUTEX(mutex) { DEBUG("Unlocking mutex %s", #mutex); pthread_mutex_unlock(&(mutex)); }
#define COND_WAIT(cond, mutex) { DEBUG("Unlocking %s and waiting on %s", #mutex, #cond); pthread_cond_wait(&(cond), &(mutex)); }
#define COND_SIGNAL(cond) { DEBUG("Signalling %s", #cond); pthread_cond_signal(&(cond)); }

void _buffer_thread_cleanup (void *arg)
{
  buf_t *buf = (buf_t *)arg;

  DEBUG("Enter _buffer_thread_cleanup");

  /* Cleanup thread data structures */
  pthread_mutex_destroy (&buf->mutex);
  pthread_cond_destroy (&buf->playback_cond);
  pthread_cond_destroy (&buf->write_cond);
}


void _buffer_thread_init (buf_t *buf)
{
  sigset_t set;

  DEBUG("Enter _buffer_thread_init");

  /* Block signals to this thread */
  sigfillset (&set);
  sigaddset (&set, SIGINT);
  sigaddset (&set, SIGTSTP);
  sigaddset (&set, SIGCONT);
  if (pthread_sigmask (SIG_BLOCK, &set, NULL) != 0)
    DEBUG("pthread_sigmask failed");
      
  
  /* Run the initialization function, if there is one */
  if (buf->init_func) 
    {
      int ret = buf->init_func (buf->initData);
      if (!ret)
	pthread_exit ((void*)ret);
    }
	
  /* Setup pthread variables */
  pthread_mutex_init (&buf->mutex, NULL);
  pthread_cond_init (&buf->write_cond, NULL);
  pthread_cond_init (&buf->playback_cond, NULL);

  /* Initialize buffer flags */
  buf->prebuffering = buf->prebuffer_size > 0;
  buf->paused = 0;
  buf->eos = 0;
  
  buf->curfill = 0;
  buf->start = 0;
}


void *_buffer_thread_func (void *arg)
{
  buf_t *buf = (buf_t*) arg;
  size_t write_amount;
  
  DEBUG("Enter _buffer_thread_func");
  
  _buffer_thread_init(buf);

  pthread_cleanup_push (_buffer_thread_cleanup, buf);
  
  DEBUG("Start main play loop");
  
  while (1) 
    {

      DEBUG("Check for something to play");
      /* Block until we can play something */
      LOCK_MUTEX (buf->mutex);
      if (buf->prebuffering || 
	  buf->paused || 
	  (buf->curfill < buf->audio_chunk_size && !buf->eos))
	{
	  DEBUG("Waiting for more data to play.");
	  COND_WAIT(buf->playback_cond, buf->mutex);
	}

      DEBUG("Ready to play");

      /* Figure out how much we can play in the buffer */

      /* For simplicity, the number of bytes played must satisfy the following
         three requirements:
	 1. Do not play more bytes than are stored in the buffer.
	 2. Do not play more bytes than the suggested audio chunk size.
	 3. Do not run off the end of the buffer. */
      write_amount = MIN3(buf->curfill, buf->audio_chunk_size, 
			  buf->size - buf->start);
      UNLOCK_MUTEX(buf->mutex);
      
      /* No need to lock mutex here because the other thread will
         NEVER reduce the number of bytes stored in the buffer */
      DEBUG("Sending %d bytes to the audio device", write_amount);
      buf->write_func(buf->buffer + buf->start, sizeof(chunk), write_amount,
		      buf->data, buf->eos);
      
      LOCK_MUTEX(buf->mutex);
      buf->curfill -= write_amount;
      buf->start = (buf->start + write_amount) % buf->size;
      DEBUG("Updated buffer fill, curfill = %ld", buf->curfill);

      /* If we've essentially emptied the buffer and prebuffering is enabled,
	 we need to do another prebuffering session */
      if (!buf->eos && (buf->curfill < buf->audio_chunk_size))
	buf->prebuffering = buf->prebuffer_size > 0;

      /* Signal a waiting decoder thread that they can put more audio into the
	 buffer */
      DEBUG("Signal decoder thread that buffer space is available");
      COND_SIGNAL(buf->write_cond);
      UNLOCK_MUTEX(buf->mutex);
    }

  /* should never get here */
  pthread_cleanup_pop(1);
  DEBUG("exiting buffer_thread_func");
}


void _submit_data_chunk (buf_t *buf, chunk *data, size_t size)
{
  long   buf_write_pos; /* offset of first available write location */
  size_t write_size;

  DEBUG("Enter _submit_data_chunk, size %d", size);

  /* Put the data into the buffer as space is made available */
  while (size > 0)
    {

      /* Section 1: Write a chunk of data */
      DEBUG("Obtaining lock on buffer");
      LOCK_MUTEX(buf->mutex);
      if (buf->size - buf->curfill > 0)
	{
	  /* Figure how much we can write into the buffer.  Requirements:
	     1. Don't write more data than we have.
	     2. Don't write more data than we have room for.
	     3. Don't write past the end of the buffer. */
	  buf_write_pos = (buf->start + buf->curfill) % buf->size;
	  write_size = MIN3(size, buf->size - buf->curfill, 
			    buf->size - buf_write_pos);

	  memmove (buf->buffer + buf_write_pos, data, write_size);
	  buf->curfill += write_size;
	  data += write_size;
	  size -= write_size;
	  DEBUG("writing chunk into buffer, curfill = %ld", buf->curfill);
	}
      else
	{
	  /* No room for more data, wait until there is */
	  DEBUG("No room for data in buffer.  Waiting.");
	  COND_WAIT(buf->write_cond, buf->mutex);
	}
      
      /* Section 2: signal if we are not prebuffering, done
         prebuffering, or paused */
      if (buf->prebuffering && (buf->prebuffer_size <= buf->curfill))
	{
	  DEBUG("prebuffering done")
	    buf->prebuffering = 0; /* done prebuffering */
	}

      if (!buf->prebuffering && !buf->paused)
	{
	  DEBUG("Signalling playback thread that more data is available.");
	  COND_SIGNAL(buf->playback_cond);      
	}
      else
	DEBUG("Not signalling playback thread since prebuffering or paused.");
      UNLOCK_MUTEX(buf->mutex);
    }

  DEBUG("Exit submit_data_chunk");
}


/* ------------------ Begin public interface ------------------ */

/* --- Buffer allocation --- */

buf_t *buffer_create (long size, long prebuffer_size, void *data, 
		      pWriteFunc write_func, void *initData, 
		      pInitFunc init_func, int audio_chunk_size)
{
  buf_t *buf = malloc (sizeof(buf_t) + sizeof (chunk) * (size - 1));

  if (buf == NULL)
    {
      perror ("malloc");
      exit (1);
    }

#ifdef DEBUG_BUFFER
  debugfile = fopen ("/tmp/bufferdebug", "w");
  setvbuf (debugfile, NULL, _IONBF, 0);
#endif

  /* Initialize the buffer structure. */
  DEBUG("buffer_create, size = %ld", size);

  memset (buf, 0, sizeof(*buf));

  buf->data = data;
  buf->write_func = write_func;

  buf->initData = initData;
  buf->init_func = init_func;
  
  /* Correct for impossible chunk sizes */
  if (audio_chunk_size > size || audio_chunk_size == 0)
    audio_chunk_size = size / 2;

  buf->audio_chunk_size = audio_chunk_size;
  buf->prebuffer_size = prebuffer_size;
  buf->size = size;

  return buf;
}


void buffer_destroy (buf_t *buf)
{
  DEBUG("buffer_destroy");
  free(buf);
}


/* --- Buffer thread control --- */

int buffer_thread_start   (buf_t *buf)
{
  DEBUG("Starting new thread.");

  return pthread_create(&buf->thread, NULL, _buffer_thread_func, buf);
}

/* WARNING: DO NOT call buffer_submit_data after you pause the
   playback thread, or you run the risk of deadlocking.  Call
   buffer_thread_unpause first. */
void buffer_thread_pause   (buf_t *buf)
{
  DEBUG("Pausing playback thread");

  LOCK_MUTEX(buf->mutex);
  buf->paused = 1;
  UNLOCK_MUTEX(buf->mutex);
}


void buffer_thread_unpause (buf_t *buf)
{
  DEBUG("Unpausing playback thread");
  
  LOCK_MUTEX(buf->mutex);
  buf->paused = 0;
  COND_SIGNAL(buf->playback_cond);
  UNLOCK_MUTEX(buf->mutex);
}


void buffer_thread_kill    (buf_t *buf)
{
  DEBUG("Attempting to kill playback thread.");

  /* End thread */
  pthread_cancel (buf->thread);
  
  /* Signal all the playback condition to wake stuff up */
  COND_SIGNAL(buf->playback_cond);

  pthread_join (buf->thread, NULL);

  _buffer_thread_cleanup(buf);

  DEBUG("Playback thread killed.");
}


/* --- Data buffering functions --- */
void buffer_submit_data (buf_t *buf, chunk *data, size_t size, size_t nmemb)
{
  size *= nmemb;
  _submit_data_chunk (buf, data, size);
}


void buffer_mark_eos (buf_t *buf)
{
  DEBUG("buffer_mark_eos");

  LOCK_MUTEX(buf->mutex);
  buf->eos = 1;
  COND_SIGNAL(buf->playback_cond);
  UNLOCK_MUTEX(buf->mutex);
}


/* --- Buffer status functions --- */

void buffer_wait_for_empty (buf_t *buf)
{
  int empty = 0;

  DEBUG("Enter buffer_wait_for_empty");
  
  LOCK_MUTEX(buf->mutex);
  while (!empty)
    {
      if (buf->curfill > 0)
	{
	  DEBUG("Buffer curfill = %ld, going back to sleep.", buf->curfill);
	  COND_WAIT(buf->write_cond, buf->mutex);
	}
      else 
	empty = 1;
    }
  UNLOCK_MUTEX(buf->mutex);

  DEBUG("Exit buffer_wait_for_empty");
}


long buffer_full (buf_t *buf)
{
  return buf->curfill;
}
