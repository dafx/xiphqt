/* reencode.c
 * - runtime reencoding of vorbis audio (usually to lower bitrates).
 *
 * $Id: reencode.c,v 1.2 2001/09/25 12:04:22 msmith Exp $
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

#include <ogg/ogg.h>
#include <vorbis/codec.h>

#include "reencode.h"
#include "config.h"
#include "stream.h"
#include "encode.h"

#define MODULE "reencode/"
#include "logging.h"

reencode_state *reencode_init(instance_t *stream)
{
	reencode_state *new = calloc(1, sizeof(reencode_state));

	new->out_bitrate = stream->bitrate;
	new->out_samplerate = stream->samplerate;
	new->out_channels = stream->channels;
	new->current_serial = -1; /* FIXME: that's a valid serial */
	new->need_headers = 0;

	return new;
}

void reencode_clear(reencode_state *s)
{
	if(s) 
	{
		ogg_stream_clear(&s->os);
		vorbis_block_clear(&s->vb);
		vorbis_dsp_clear(&s->vd);
		vorbis_comment_clear(&s->vc);
		vorbis_info_clear(&s->vi);

		free(s);
	}
}

/* Returns: -1 fatal death failure, argh!
 * 		     0 haven't produced any output yet
 * 		    >0 success
 */
int reencode_page(reencode_state *s, ref_buffer *buf,
		unsigned char **outbuf, int *outlen)
{
	ogg_page og, encog;
	ogg_packet op;
	int retbuflen=0, old;
	unsigned char *retbuf=NULL;
		

	og.header_len = buf->aux_data;
	og.body_len = buf->len - buf->aux_data;
	og.header = buf->buf;
	og.body = buf->buf + og.header_len;

	if(s->current_serial != ogg_page_serialno(&og))
	{
		s->current_serial = ogg_page_serialno(&og);

		if(s->encoder)
		{
			encode_finish(s->encoder);
			while(encode_flush(s->encoder, &encog) != 0)
			{
				old = retbuflen;
				retbuflen += encog.header_len + encog.body_len;
				retbuf = realloc(retbuf, retbuflen);
				memcpy(retbuf+old, encog.header, encog.header_len);
				memcpy(retbuf+old+encog.header_len, encog.body, 
						encog.body_len);
			}
		}
		encode_clear(s->encoder);

		ogg_stream_clear(&s->os);
		ogg_stream_init(&s->os, s->current_serial);
		ogg_stream_pagein(&s->os, &og);

		vorbis_block_clear(&s->vb);
		vorbis_dsp_clear(&s->vd);
		vorbis_comment_clear(&s->vc);
		vorbis_info_clear(&s->vi);

		vorbis_info_init(&s->vi);
		vorbis_comment_init(&s->vc);

		if(ogg_stream_packetout(&s->os, &op) != 1)
		{
			LOG_ERROR0("Invalid primary header in stream");
			return -1;
		}

		if(vorbis_synthesis_headerin(&s->vi, &s->vc, &op) < 0)
		{
			LOG_ERROR0("Input stream not vorbis, can't reencode");
			return -1;
		}

		s->need_headers = 2; /* We still need two more header packets */
		LOG_DEBUG0("Reinitialising reencoder for new logical stream");
	}
	else
	{
		ogg_stream_pagein(&s->os, &og);
		while(ogg_stream_packetout(&s->os, &op) > 0)
		{
			if(s->need_headers)
			{
				vorbis_synthesis_headerin(&s->vi, &s->vc, &op);
				/* If this was the last header, init the rest */
				if(!--s->need_headers)
				{
					vorbis_block_init(&s->vd, &s->vb);
					vorbis_synthesis_init(&s->vd, &s->vi);
					if(s->vi.channels != s->out_channels
						|| s->vi.rate != s->out_samplerate)
					{
						/* FIXME: Implement this. In the shorter term, at
						 * least free all the memory that's lying around.
						 */
						LOG_ERROR0("Converting channels or sample rate NOT "
								"currently supported.");
						return -1;
					}
					
					s->encoder = encode_initialise(s->out_channels, 
							s->out_samplerate, s->out_bitrate, 
							s->current_serial, &s->vc);
				}
			}
			else
			{
				float **pcm;
				int samples;


				if(vorbis_synthesis(&s->vb, &op)==0)
				{
					vorbis_synthesis_blockin(&s->vd, &s->vb);
				}

				while((samples = vorbis_synthesis_pcmout(&s->vd, &pcm))>0)
				{
					encode_data_float(s->encoder, pcm, samples);
					vorbis_synthesis_read(&s->vd, samples);
				}

				while(encode_dataout(s->encoder, &encog) != 0)
				{

					old = retbuflen;
					retbuflen += encog.header_len + encog.body_len;
					retbuf = realloc(retbuf, retbuflen);
					memcpy(retbuf+old, encog.header, encog.header_len);
					memcpy(retbuf+old+encog.header_len, encog.body, 
							encog.body_len);
				}
			}
		}
	}

	/* We've completed every packet from this page, so
	 * now we can return what we wanted, depending on whether
	 * we actually got data out or not
	 */
	if(retbuflen > 0)
	{
		*outbuf = retbuf;
		*outlen = retbuflen;
		return retbuflen;
	}
	else
	{
		return 0;
	}
}


