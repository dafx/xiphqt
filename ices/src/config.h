/* config.h
 * - configuration, and global structures built from config
 *
 * $Id: config.h,v 1.12 2002/08/03 08:14:54 msmith Exp $
 *
 * Copyright (c) 2001 Michael Smith <msmith@labyrinth.net.au>
 *
 * This program is distributed under the terms of the GNU General
 * Public License, version 2. You may use, modify, and redistribute
 * it under the terms of this license. A copy should be included
 * with this source.
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "stream.h"
#include "inputmodule.h"

typedef struct _module_param_tag
{
	char *name;
	char *value;

	struct _module_param_tag *next;
} module_param_t;

/* FIXME: orward declaraction because my headers are a mess. */
struct buffer_queue;

typedef struct _instance_tag
{
	char *hostname;
	int port;
	char *password;
	char *mount;
	int reconnect_delay;
	int reconnect_attempts;
	int encode;
    int downmix;
    int resampleinrate;
    int resampleoutrate;
	int max_queue_length;
	char *savefilename;

	/* Parameters for re-encoding */
    int managed;
	int min_br, nom_br, max_br;
    float quality;
	int samplerate;
	int channels;
	
	/* private */
    FILE *savefile;
	int serial;
	int buffer_failures;
	int died;
	int kill;
	int skip;
    int wait_for_critical;

	struct buffer_queue *queue;

	struct _instance_tag *next;
} instance_t;

typedef struct _config_tag
{
	int background;
	char *logpath;
	char *logfile;
	int loglevel;
    int log_stderr;

	/* <stream> */

	/* <metadata> */

	char *stream_name;
	char *stream_genre;
	char *stream_description;
	
	/* <playlist> */
	
	char *playlist_module;
	module_param_t *module_params;

	/* <instance> */

	instance_t *instances;

	/* private */
	int log_id;
	int shutdown;
    char *metadata_filename;
	cond_t queue_cond;
	cond_t event_pending_cond;
	mutex_t refcount_lock;
	mutex_t flush_lock;
	input_module_t *inmod;
	struct _config_tag *next;
} config_t;

extern config_t *ices_config;

void config_initialize(void);
void config_shutdown(void);

int config_read(const char *filename);
void config_dump(void);

void config_free_instance(instance_t *instance);

#endif /* __CONFIG_H__ */





