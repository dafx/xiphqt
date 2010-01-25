/*
 *
 *  audio_out.c
 *
 *      Original Copyright (C) Aaron Holtzman - May 1999
 *      Modifications Copyright (C) Stan Seibert - July 2000
 *
 *  This file is part of libao, a cross-platform audio output library.  See
 *  README for a history of this source code.
 *
 *  libao is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  libao is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#if defined HAVE_DLFCN_H && defined HAVE_DLOPEN
# include <dlfcn.h>
#else
static void *dlopen(const char *filename, int flag) {return 0;}
static char *dlerror(void) { return "dlopen: unsupported"; }
static void *dlsym(void *handle, const char *symbol) { return 0; }
static int dlclose(void *handle) { return 0; }
#undef DLOPEN_FLAG
#define DLOPEN_FLAG 0
#endif
#include <sys/types.h>
#include <sys/stat.h>
#ifndef _MSC_VER
# include <unistd.h>
#endif
#include <dirent.h>

#include "ao/ao.h"
#include "ao_private.h"

/* These should have been set by the Makefile */
#ifndef AO_PLUGIN_PATH
#define AO_PLUGIN_PATH "/usr/local/lib/ao"
#endif
#ifndef SHARED_LIB_EXT
#define SHARED_LIB_EXT ".so"
#endif

/* --- Other constants --- */
#define DEF_SWAP_BUF_SIZE  1024

/* --- Driver Table --- */

typedef struct driver_list {
	ao_functions *functions;
	void *handle;
	struct driver_list *next;
} driver_list;


extern ao_functions ao_null;
extern ao_functions ao_wav;
extern ao_functions ao_raw;
extern ao_functions ao_au;
#ifdef HAVE_SYS_AUDIO_H
extern ao_functions ao_aixs;
#endif
#ifdef HAVE_WMM
extern ao_functions ao_wmm;
#endif
static ao_functions *static_drivers[] = {
	&ao_null, /* Must have at least one static driver! */
	&ao_wav,
	&ao_raw,
	&ao_au,
#ifdef HAVE_SYS_AUDIO_H
	&ao_aixs,
#endif
#ifdef HAVE_WMM
	&ao_wmm,
#endif

	NULL /* End of list */
};

static driver_list *driver_head = NULL;
static ao_config config = {
	NULL /* default_driver */
};

static ao_info **info_table = NULL;
static int driver_count = 0;

/* ---------- Helper functions ---------- */

/* Clear out all of the library configuration options and set them to
   defaults.   The defaults should match the initializer above. */
static void _clear_config()
{
	free(config.default_driver);
	config.default_driver = NULL;
}


/* Load a plugin from disk and put the function table into a driver_list
   struct. */
static driver_list *_get_plugin(char *plugin_file)
{
	driver_list *dt;
	void *handle;

	handle = dlopen(plugin_file, DLOPEN_FLAG /* See ao_private.h */);

	if (handle) {
		dt = (driver_list *)malloc(sizeof(driver_list));
		if (!dt) return NULL;

		dt->handle = handle;

		dt->functions = (ao_functions *)malloc(sizeof(ao_functions));
		if (!(dt->functions)) {
			free(dt);
			return NULL;
		}

		dt->functions->test = dlsym(dt->handle, "ao_plugin_test");
		if (!(dt->functions->test)) goto failed;

		dt->functions->driver_info =
		  dlsym(dt->handle, "ao_plugin_driver_info");
		if (!(dt->functions->driver_info)) goto failed;

		dt->functions->device_init =
		  dlsym(dt->handle, "ao_plugin_device_init");
		if (!(dt->functions->device_init )) goto failed;

		dt->functions->set_option =
		  dlsym(dt->handle, "ao_plugin_set_option");
		if (!(dt->functions->set_option)) goto failed;

		dt->functions->open = dlsym(dt->handle, "ao_plugin_open");
		if (!(dt->functions->open)) goto failed;

		dt->functions->play = dlsym(dt->handle, "ao_plugin_play");
		if (!(dt->functions->play)) goto failed;

		dt->functions->close = dlsym(dt->handle, "ao_plugin_close");
		if (!(dt->functions->close)) goto failed;

		dt->functions->device_clear =
		  dlsym(dt->handle, "ao_plugin_device_clear");
		if (!(dt->functions->device_clear)) goto failed;


	} else {
		return NULL;
	}

	return dt;

 failed:
	free(dt->functions);
	free(dt);
	return NULL;
}


/* If *name is a valid driver name, return its driver number.
   Otherwise, test all of available live drivers until one works. */
static int _find_default_driver_id (const char *name)
{
	int def_id;
	int id;
	ao_info *info;
	driver_list *driver = driver_head;

	if ( name == NULL || (def_id = ao_driver_id(name)) < 0 ) {
		/* No default specified. Find one among available drivers. */
		def_id = -1;

		id = 0;
		while (driver != NULL) {

			info = driver->functions->driver_info();

			if ( info->type == AO_TYPE_LIVE &&
			     info->priority > 0 && /* Skip static drivers */
			     driver->functions->test() ) {
				def_id = id; /* Found a usable driver */
				break;
			}

			driver = driver->next;
			id++;
		}
	}

	return def_id;
}


/* Convert the static drivers table into a linked list of drivers. */
static driver_list* _load_static_drivers(driver_list **end)
{
	driver_list *head;
	driver_list *driver;
	int i;

	/* insert first driver */
	head = driver = malloc(sizeof(driver_list));
	if (driver != NULL) {
		driver->functions = static_drivers[0];
		driver->handle = NULL;
		driver->next = NULL;

		i = 1;
		while (static_drivers[i] != NULL) {
			driver->next = malloc(sizeof(driver_list));
			if (driver->next == NULL)
				break;

			driver->next->functions = static_drivers[i];
			driver->next->handle = NULL;
			driver->next->next = NULL;

			driver = driver->next;
			i++;
		}
	}

	if (end != NULL)
		*end = driver;

	return head;
}


/* Load the dynamic drivers from disk and append them to end of the
   driver list.  end points the driver_list node to append to. */
static void _append_dynamic_drivers(driver_list *end)
{
#ifdef HAVE_DLOPEN
	struct dirent *plugin_dirent;
	char *ext;
	struct stat statbuf;
	char fullpath[PATH_MAX];
	DIR *plugindir;
	driver_list *plugin;
	driver_list *driver = end;

	/* now insert any plugins we find */
	plugindir = opendir(AO_PLUGIN_PATH);
	if (plugindir != NULL) {
		while ((plugin_dirent = readdir(plugindir)) != NULL) {
			snprintf(fullpath, PATH_MAX, "%s/%s",
				 AO_PLUGIN_PATH, plugin_dirent->d_name);
			if (!stat(fullpath, &statbuf) &&
			    S_ISREG(statbuf.st_mode) &&
			 (ext = strrchr(plugin_dirent->d_name, '.')) != NULL) {
				if (strcmp(ext, SHARED_LIB_EXT) == 0) {
					plugin = _get_plugin(fullpath);
					if (plugin) {
						driver->next = plugin;
						plugin->next = NULL;
						driver = driver->next;
					}
				}
			}
		}

		closedir(plugindir);
	}
#endif
}


/* Compare two drivers based on priority
   Used as compar function for qsort() in _make_info_table() */
static int _compar_driver_priority (const driver_list **a,
				    const driver_list **b)
{
	return memcmp(&((*b)->functions->driver_info()->priority),
		      &((*a)->functions->driver_info()->priority),
		      sizeof(int));
}


/* Make a table of driver info structures for ao_driver_info_list(). */
static ao_info ** _make_info_table (driver_list **head, int *driver_count)
{
	driver_list *list;
	int i;
	ao_info **table;
	driver_list **drivers_table;

	*driver_count = 0;

	/* Count drivers */
	list = *head;
	i = 0;
	while (list != NULL) {
		i++;
		list = list->next;
	}


	/* Sort driver_list */
	drivers_table = (driver_list **) calloc(i, sizeof(driver_list *));
	if (drivers_table == NULL)
		return (ao_info **) NULL;
	list = *head;
	*driver_count = i;
	for (i = 0; i < *driver_count; i++, list = list->next)
		drivers_table[i] = list;
	qsort(drivers_table, i, sizeof(driver_list *),
			(int(*)(const void *, const void *))
			_compar_driver_priority);
	*head = drivers_table[0];
	for (i = 1; i < *driver_count; i++)
		drivers_table[i-1]->next = drivers_table[i];
	drivers_table[i-1]->next = NULL;


	/* Alloc table */
	table = (ao_info **) calloc(i, sizeof(ao_info *));
	if (table != NULL) {
		for (i = 0; i < *driver_count; i++)
			table[i] = drivers_table[i]->functions->driver_info();
	}

	free(drivers_table);

	return table;
}


/* Return the driver struct corresponding to particular driver id
   number. */
static driver_list *_get_driver(int driver_id) {
	int i = 0;
	driver_list *driver = driver_head;

	if (driver_id < 0) return NULL;

	while (driver && (i < driver_id)) {
		i++;
		driver = driver->next;
	}

	if (i == driver_id)
		return driver;

	return NULL;
}


/* Check if driver_id is a valid id number */
static int _check_driver_id(int driver_id)
{
	int i = 0;
	driver_list *driver = driver_head;

	if (driver_id < 0) return 0;

	while (driver && (i <= driver_id)) {
		driver = driver->next;
		i++;
	}

	if (i == (driver_id + 1))
		return 1;

	return 0;
}


/* helper function to convert a byte_format of AO_FMT_NATIVE to the
   actual byte format of the machine, otherwise just return
   byte_format */
static int _real_byte_format(int byte_format)
{
	if (byte_format == AO_FMT_NATIVE) {
		if (ao_is_big_endian())
			return AO_FMT_BIG;
		else
			return AO_FMT_LITTLE;
	} else
		return byte_format;
}


/* Create a new ao_device structure and populate it with data */
static ao_device* _create_device(int driver_id, driver_list *driver,
				 ao_sample_format *format, FILE *file)
{
	ao_device *device;

	device = malloc(sizeof(ao_device));

	if (device != NULL) {
		device->type = driver->functions->driver_info()->type;
		device->driver_id = driver_id;
		device->funcs = driver->functions;
		device->file = file;
		device->machine_byte_format =
		  ao_is_big_endian() ? AO_FMT_BIG : AO_FMT_LITTLE;
		device->client_byte_format =
		  _real_byte_format(format->byte_format);
		device->swap_buffer = NULL;
		device->swap_buffer_size = 0;
		device->internal = NULL;
                device->output_channels = format->channels;
                device->permute_channels = NULL;
                device->output_matrix = NULL;
	}

	return device;
}


/* Expand the swap buffer in this device if it is smaller than
   min_size. */
static int _realloc_swap_buffer(ao_device *device, int min_size)
{
	void *temp;

	if (min_size > device->swap_buffer_size) {
		temp = realloc(device->swap_buffer, min_size);
		if (temp != NULL) {
			device->swap_buffer = temp;
			device->swap_buffer_size = min_size;
			return 1; /* Success, realloc worked */
		} else
			return 0; /* Fail to realloc */
	} else
		return 1; /* Success, no need to realloc */
}


/* Swap and copy the byte order of samples from the source buffer to
   the target buffer. */
static void _swap_samples(char *target_buffer, char* source_buffer,
			  uint_32 num_bytes)
{
	uint_32 i;

	for (i = 0; i < num_bytes; i += 2) {
		target_buffer[i] = source_buffer[i+1];
		target_buffer[i+1] = source_buffer[i];
	}
}

/* the channel locations we know right now. code below assumes M is in slot 0 */
static char *mnemonics[]={
  "M","L","C","R","CL","CR","SL","SR","BL","BC","BR","LFE","U",
  "A1","A2","A3","A4","A5","A6","A7","A8","A9",NULL
};

/* Check the requested maxtrix string for syntax and mnemonics */
static char *_sanitize_matrix(char *matrix,int quiet){
  if(matrix){
    char *ret = calloc(strlen(matrix)+1,1); /* can only get smaller */
    char *p=matrix;
    int count=0;
    while(1){
      char *h,*t;
      int m=0;

      /* trim leading space */
      while(*p && isspace(*p))p++;

      /* search for seperator */
      h=p;
      while(*h && *h!=',')h++;

      /* trim trailing space */
      t=h;
      while(t>p && isspace(*(t-1)))t--;

      while(mnemonics[m]){
        if(t-p && !strncmp(mnemonics[m],p,t-p)){
          if(count)strcat(ret,",");
          strcat(ret,mnemonics[m]);
          break;
        }
        m++;
      }
      if(!mnemonics[m]){
        /* unrecognized channel mnemonic */
        if(!quiet){
          int i;
          fprintf(stderr,"\nUnrecognized channel name \"");
          for(i=0;i<t-p;i++)fputc(p[i],stderr);
          fprintf(stderr,"\" in channel matrix \"%s\"\n",matrix);
        }
        free(ret);
        return NULL;
      }
      count++;
      if(!*h)break;
      p=h+1;
    }
    return ret;
  }else
    return NULL;
}

static int _find_channel(int needle, char *haystack){
  char *p=haystack;
  int count=0;
  while(1){
    char *h;
    int m=0;

    /* search for seperator */
    h=p;
    while(*h && *h!=',')h++;

    while(mnemonics[m]){
      if(!strncmp(mnemonics[needle],p,h-p))break;
      m++;
    }
    if(mnemonics[m])
      return count;
    count++;
    if(!*h)break;
    p=h+1;
  }
  return -1;
}

/* Open a device.  If this is a live device, file == NULL. */
static ao_device* _open_device(int driver_id, ao_sample_format *format,
			       ao_option *options, FILE *file)
{
	ao_functions *funcs;
	driver_list *driver;
	ao_device *device;
	int result;
        ao_sample_format sformat=*format;

	/* Get driver id */
	if ( (driver = _get_driver(driver_id)) == NULL ) {
		errno = AO_ENODRIVER;
		return NULL; /* No driver exists */
	}

	funcs = driver->functions;

	/* Check the driver type */
	if (file == NULL &&
	    funcs->driver_info()->type != AO_TYPE_LIVE) {

		errno = AO_ENOTLIVE;
		return NULL;
	} else if (file != NULL &&
		   funcs->driver_info()->type != AO_TYPE_FILE) {

		errno = AO_ENOTFILE;
		return NULL;
	}

	/* Make a new device structure */
	if ( (device = _create_device(driver_id, driver,
				      format, file)) == NULL ) {
		errno = AO_EFAIL;
		return NULL; /* Couldn't alloc device */
	}

	/* Initialize the device memory */
	if (!funcs->device_init(device)) {
		free(device);
		errno = AO_EFAIL;
		return NULL; /* Couldn't init internal memory */
	}

	/* Load options */
	while (options != NULL) {
          /* The output matrix option is handled for the drivers here */
          if(!strcmp(options->key,"matrix")){
            /* explicitly set the output matrix to the requested
               string; devices must not override. */
            device->output_matrix = _sanitize_matrix(options->value, device->verbose==-1);
            if(!device->output_matrix){
              errno = AO_EBADOPTION;
              return NULL;
            }
          }else if(!strcmp(options->key,"verbose")){
            device->verbose=1;
          }else if(!strcmp(options->key,"quiet")){
            device->verbose=-1;
          }else{
            if (!funcs->set_option(device, options->key, options->value)) {
              /* Problem setting options */
              free(device);
              errno = AO_EOPENDEVICE;
              return NULL;
            }
          }

          options = options->next;
	}

        /* also sanitize the format input channel matrix */
        if(format->matrix){
          sformat.matrix = _sanitize_matrix(format->matrix, device->verbose==-1);
          if(!sformat.matrix && device->verbose>=0)
            fprintf(stderr,"Input channel matrix invalid; ignoring.\n");
        }

	/* Open the device */
	result = funcs->open(device, &sformat);
	if (!result) {
                if(sformat.matrix)free(sformat.matrix);
		funcs->device_clear(device);
		free(device);
		errno = AO_EOPENDEVICE;
		return NULL; /* Couldn't open device */
	}

        /* resolve channel mapping request if any */
        if(sformat.matrix){
          if(!device->output_matrix){
            if(device->verbose>=0)
              fprintf(stderr,"\nOutput driver %s does not support channel matrixing;\n"
                      "continuing without routing channels to specific locations.\n\n",
                      info_table[device->driver_id]->short_name);
          }else{

            /* walk thorugh the output matrix, match outputs to inputs */
            char *op=device->output_matrix;
            int count=0;
            device->permute_channels = calloc(device->output_channels,sizeof(int));

            if(device->verbose>0)
              fprintf(stderr,"\n");

            while(count<device->output_channels){
              int m=0,mm;
              char *h=op;

              if(op){
                /* find mnemonic offset of output channel */
                while(*h && *h!=',')h++;
                while(mnemonics[m]){
                  if(!strncmp(mnemonics[m],op,h-op))
                    break;
                  m++;
                }
                mm=m;

                /* find match in input if any */
                device->permute_channels[count] = _find_channel(m,sformat.matrix);
                if(device->permute_channels[count] == -1 && sformat.channels == 1){
                  device->permute_channels[count] = _find_channel(0,sformat.matrix);
                  mm=0;
                }
              }else
                device->permute_channels[count] = -1;

              /* display resulting mapping for now */
              if(device->verbose>0)
                if(device->permute_channels[count]>=0){
                  fprintf(stderr,"Output %d (%s)\t <- input %d (%s)\n",
                          count,mnemonics[m],device->permute_channels[count],
                          mnemonics[mm]);
                }else{
                  fprintf(stderr,"Output %d (%s)\t <- none\n",
                          count,mnemonics[m]);
                }
              count++;
              if(!h)
                op=NULL;
              else
                op=h+1;
            }
          }
        }

	/* Resolve actual driver byte format */
	device->driver_byte_format =
		_real_byte_format(device->driver_byte_format);

	/* Only create swap buffer for 16 bit samples if needed */
	if (format->bits == 16 &&
	    device->client_byte_format != device->driver_byte_format) {

          if(device->verbose>0)
            fprintf(stderr,
                    "n\n\n\n-------------------------\n"
                    "big : %d\n"
                    "device->client_byte_format:%d\n"
                    "device->driver_byte_format:%d\n"
                    "--------------------------\n",
                    ao_is_big_endian(),device->client_byte_format,device->driver_byte_format);

		result = _realloc_swap_buffer(device, DEF_SWAP_BUF_SIZE);

		if (!result) {

			if(sformat.matrix)free(sformat.matrix);
                        device->funcs->close(device);
			device->funcs->device_clear(device);
			free(device);
			errno = AO_EFAIL;
			return NULL; /* Couldn't alloc swap buffer */
		}
	}

	/* If we made it this far, everything is OK. */
        if(sformat.matrix)free(sformat.matrix);
	return device;
}


/* ---------- Public Functions ---------- */

/* -- Library Setup/Teardown -- */

void ao_initialize(void)
{
	driver_list *end;

	/* Read config files */
	read_config_files(&config);

	if (driver_head == NULL) {
		driver_head = _load_static_drivers(&end);
		_append_dynamic_drivers(end);
	}

	/* Create the table of driver info structs */
	info_table = _make_info_table(&driver_head, &driver_count);
}


void ao_shutdown(void)
{
	driver_list *driver = driver_head;
	driver_list *next_driver;

	if (!driver_head) return;

	/* unload and free all the drivers */
	while (driver) {
		if (driver->handle) {

		  dlclose(driver->handle);
		  free(driver->functions); /* DON'T FREE STATIC FUNC TABLES */
		}
		next_driver = driver->next;
		free(driver);
		driver = next_driver;
	}

        _clear_config();
	/* NULL out driver_head or ao_initialize() won't work */
	driver_head = NULL;
}


/* -- Device Setup/Playback/Teardown -- */

int ao_append_option(ao_option **options, const char *key, const char *value)
{
	ao_option *op, *list;

	op = malloc(sizeof(ao_option));
	if (op == NULL) return 0;

	op->key = strdup(key);
	op->value = strdup(value?value:"");
	op->next = NULL;

	if ((list = *options) != NULL) {
		list = *options;
		while (list->next != NULL) list = list->next;
		list->next = op;
	} else {
		*options = op;
	}


	return 1;
}


void ao_free_options(ao_option *options)
{
	ao_option *rest;

	while (options != NULL) {
		rest = options->next;
		free(options->key);
		free(options->value);
		free(options);
		options = rest;
	}
}


ao_device *ao_open_live (int driver_id, ao_sample_format *format,
			ao_option *options)
{
	return _open_device(driver_id, format, options, NULL);
}


ao_device *ao_open_file (int driver_id, const char *filename, int overwrite,
			 ao_sample_format *format, ao_option *options)
{
	FILE *file;
	ao_device *device;

	if (strcmp("-", filename) == 0)
		file = stdout;
	else {

		if (!overwrite) {
			/* Test for file existence */
			file = fopen(filename, "r");
			if (file != NULL) {
				fclose(file);
				errno = AO_EFILEEXISTS;
				return NULL;
			}
		}


		file = fopen(filename, "w");
	}


	if (file == NULL) {
		errno = AO_EOPENFILE;
		return NULL;
	}

	device = _open_device(driver_id, format, options, file);

	if (device == NULL) {
		fclose(file);
		/* errno already set by _open_device() */
		return NULL;
	}

	return device;
}


int ao_play(ao_device *device, char* output_samples, uint_32 num_bytes)
{
	char *playback_buffer;

	if (device == NULL)
	  return 0;

#if 1
	if (device->swap_buffer != NULL) {
		if (_realloc_swap_buffer(device, num_bytes)) {
			_swap_samples(device->swap_buffer,
				      output_samples, num_bytes);
			playback_buffer = device->swap_buffer;
		} else
			return 0; /* Could not expand swap buffer */
	} else
#endif
		playback_buffer = output_samples;

	return device->funcs->play(device, playback_buffer, num_bytes);
}


int ao_close(ao_device *device)
{
	int result;

	if (device == NULL)
		result = 0;
	else {
		result = device->funcs->close(device);
		device->funcs->device_clear(device);

		if (device->file) {
			fclose(device->file);
			device->file = NULL;
		}

		if (device->swap_buffer != NULL)
			free(device->swap_buffer);
                if (device->output_matrix != NULL)
                        free(device->output_matrix);
		free(device);
	}

	return result;
}


/* -- Driver Information -- */

int ao_driver_id(const char *short_name)
{
	int i;
	driver_list *driver = driver_head;

	i = 0;
	while (driver) {
		if (strcmp(short_name,
			   driver->functions->driver_info()->short_name) == 0)
			return i;
		driver = driver->next;
		i++;
	}

	return -1; /* No driver by that name */
}


int ao_default_driver_id (void)
{
	/* Find the default driver in the list of loaded drivers */

	return _find_default_driver_id(config.default_driver);
}


ao_info *ao_driver_info(int driver_id)
{
	driver_list *driver;

	if ( (driver = _get_driver(driver_id)) )
		return driver->functions->driver_info();
	else
		return NULL;
}


ao_info **ao_driver_info_list(int *count)
{
	*count = driver_count;
	return info_table;
}


/* -- Miscellaneous -- */

/* Stolen from Vorbis' lib/vorbisfile.c */
int ao_is_big_endian(void)
{
	static uint_16 pattern = 0xbabe;
	return 0[(volatile unsigned char *)&pattern] == 0xba;
}
