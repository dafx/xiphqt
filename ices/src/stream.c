/* stream.c
 * - Core streaming functions/main loop.
 *
 * $Id: stream.c,v 1.10 2002/01/23 03:40:28 jack Exp $
 *
 * Copyright (c) 2001 Michael Smith <msmith@labyrinth.net.au>
 *
 * This program is distributed under the terms of the GNU General
 * Public License, version 2. You may use, modify, and redistribute
 * it under the terms of this license. A copy should be included
 * with this source.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>

#include <shout/shout.h>

#include "config.h"
#include "input.h"
#include "im_playlist.h"
#include "net/resolver.h"
#include "signals.h"
#include "thread/thread.h"
#include "reencode.h"
#include "encode.h"
#include "inputmodule.h"
#include "stream_shared.h"
#include "stream.h"

#include <ogg/ogg.h>
#include <vorbis/codec.h>

#define MODULE "stream/"
#include "logging.h"

#define MAX_ERRORS 10

/* The main loop for each instance. Gets data passed to it from the stream
 * manager (which gets it from the input module), and streams it to the
 * specified server
 */
void *ices_instance_stream(void *arg)
{
	int ret, shouterr;
	ref_buffer *buffer;
	char *connip;
	stream_description *sdsc = arg;
	instance_t *stream = sdsc->stream;
	input_module_t *inmod = sdsc->input;
	int reencoding = (inmod->type == ICES_INPUT_VORBIS) && stream->encode;
	int encoding = (inmod->type == ICES_INPUT_PCM) && stream->encode;
	
	vorbis_comment_init(&sdsc->vc);

	sdsc->shout = shout_new();

	/* we only support the ice protocol and vorbis streams currently */
	shout_set_format(sdsc->shout, SHOUT_FORMAT_VORBIS);
	shout_set_protocol(sdsc->shout, SHOUT_PROTOCOL_ICE);

	signal(SIGPIPE, signal_hup_handler);

	connip = malloc(16);
	if(!resolver_getip(stream->hostname, connip, 16))
	{
		LOG_ERROR1("Could not resolve hostname \"%s\"", stream->hostname);
		free(connip);
		stream->died = 1;
		return NULL;
	}

	if (!(shout_set_host(sdsc->shout, connip)) == SHOUTERR_SUCCESS) {
		LOG_ERROR1("libshout error: %s\n", shout_get_error(sdsc->shout));
		free(connip);
		stream->died = 1;
		return NULL;
	}

	shout_set_port(sdsc->shout, stream->port);
	if (!(shout_set_password(sdsc->shout, stream->password)) == SHOUTERR_SUCCESS) {
		LOG_ERROR1("libshout error: %s\n", shout_get_error(sdsc->shout));
		free(connip);
		stream->died = 1;
		return NULL;
	}
	if (!(shout_set_mount(sdsc->shout, stream->mount)) == SHOUTERR_SUCCESS) {
		LOG_ERROR1("libshout error: %s\n", shout_get_error(sdsc->shout));
		free(connip);
		stream->died = 1;
		return NULL;
	}

	/* set the metadata for the stream */
	if (ices_config->stream_name)
		if (!(shout_set_name(sdsc->shout, ices_config->stream_name)) == SHOUTERR_SUCCESS) {
			LOG_ERROR1("libshout error: %s\n", shout_get_error(sdsc->shout));
			free(connip);
			stream->died = 1;
			return NULL;
		}
	if (ices_config->stream_genre)
		if (!(shout_set_genre(sdsc->shout, ices_config->stream_genre)) == SHOUTERR_SUCCESS) {
			LOG_ERROR1("libshout error: %s\n", shout_get_error(sdsc->shout));
			free(connip);
			stream->died = 1;
			return NULL;
		}
	if (ices_config->stream_description)
		if (!(shout_set_description(sdsc->shout, ices_config->stream_description)) == SHOUTERR_SUCCESS) {
			LOG_ERROR1("libshout error: %s\n", shout_get_error(sdsc->shout));
			free(connip);
			stream->died = 1;
			return NULL;
		}

	if(encoding)
	{
		if(inmod->metadata_update)
			inmod->metadata_update(inmod->internal, &sdsc->vc);
		sdsc->enc = encode_initialise(stream->channels, stream->samplerate,
				stream->bitrate, stream->serial++, &sdsc->vc);
	}
	else if(reencoding)
		sdsc->reenc = reencode_init(stream);

    if(stream->savefilename != NULL) 
    {
        stream->savefile = fopen(stream->savefilename, "wb");
        if(!stream->savefile)
            LOG_ERROR2("Failed to open stream save file %s: %s", 
                    stream->savefilename, strerror(errno));
        else
            LOG_INFO1("Saving stream to file %s", stream->savefilename);
    }

	if((shouterr = shout_open(sdsc->shout)) == SHOUTERR_SUCCESS)
	{
		LOG_INFO3("Connected to server: %s:%d%s", 
				shout_get_host(sdsc->shout), shout_get_port(sdsc->shout), shout_get_mount(sdsc->shout));

		while(1)
		{
			if(stream->buffer_failures > MAX_ERRORS)
			{
				LOG_WARN0("Too many errors, shutting down");
				break;
			}

			buffer = stream_wait_for_data(stream);

			/* buffer being NULL means that either a fatal error occured,
			 * or we've been told to shut down
			 */
			if(!buffer)
				break;

			/* If data is NULL or length is 0, we should just skip this one.
			 * Probably, we've been signalled to shut down, and that'll be
			 * caught next iteration. Add to the error count just in case,
			 * so that we eventually break out anyway 
			 */
			if(!buffer->buf || !buffer->len)
			{
				LOG_WARN0("Bad buffer dequeued!");
				stream->buffer_failures++;
				continue; 
			}

            if(stream->wait_for_critical)
            {
                LOG_INFO0("Trying restart on new substream");
                stream->wait_for_critical = 0;
            }

            ret = process_and_send_buffer(sdsc, buffer);

            /* No data produced, do nothing */
            if(ret == -1)
                ;
            /* Fatal error */
            else if(ret == -2)
            {
                LOG_ERROR0("Serious error, waiting to restart on "
                           "next substream. Stream temporarily suspended.");
                /* Set to wait until a critical buffer comes through (start of
                 * a new substream, typically), and flush existing queue.
                 */
                thread_mutex_lock(&ices_config->flush_lock);
                stream->wait_for_critical = 1;
                input_flush_queue(stream->queue, 0);
                thread_mutex_unlock(&ices_config->flush_lock);
            }
            /* Non-fatal shout error */
            else if(ret == 0)
			{
				LOG_ERROR1("Send error: %s", 
                        shout_get_error(sdsc->shout));
				if(shouterr == SHOUTERR_SOCKET)
				{
					int i=0;

					/* While we're trying to reconnect, don't receive data
					 * to this instance, or we'll overflow once reconnect
					 * succeeds
					 */
					thread_mutex_lock(&ices_config->flush_lock);
					stream->skip = 1;

					/* Also, flush the current queue */
					input_flush_queue(stream->queue, 1);
					thread_mutex_unlock(&ices_config->flush_lock);
					
					while((i < stream->reconnect_attempts ||
							stream->reconnect_attempts==-1) && 
                            !ices_config->shutdown)
					{
						i++;
						LOG_WARN0("Trying reconnect after server socket error");
						shout_close(sdsc->shout);
						if((shouterr = shout_open(sdsc->shout)) == SHOUTERR_SUCCESS)
						{
							LOG_INFO3("Connected to server: %s:%d%s", 
                                    shout_get_host(sdsc->shout), shout_get_port(sdsc->shout), 
                                    shout_get_mount(sdsc->shout));
							break;
						}
						else
						{
							LOG_ERROR3("Failed to reconnect to %s:%d (%s)",
								shout_get_host(sdsc->shout),shout_get_port(sdsc->shout),
								shout_get_error(sdsc->shout));
							if(i==stream->reconnect_attempts)
							{
								LOG_ERROR0("Reconnect failed too many times, "
										  "giving up.");
                                /* We want to die now */
								stream->buffer_failures = MAX_ERRORS+1; 
							}
							else /* Don't try again too soon */
								sleep(stream->reconnect_delay); 
						}
					}
					stream->skip = 0;
				}
				stream->buffer_failures++;
			}
			stream_release_buffer(buffer);
		}
	}
	else
	{
		LOG_ERROR3("Failed initial connect to %s:%d (%s)", 
				shout_get_host(sdsc->shout),shout_get_port(sdsc->shout), shout_get_error(sdsc->shout));
	}
	
	shout_close(sdsc->shout);

    if(stream->savefile != NULL) 
        fclose(stream->savefile);

    	shout_free(sdsc->shout);
	encode_clear(sdsc->enc);
	reencode_clear(sdsc->reenc);
	vorbis_comment_clear(&sdsc->vc);

	stream->died = 1;
	return NULL;
}

