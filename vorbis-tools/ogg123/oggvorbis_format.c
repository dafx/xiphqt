/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU PUBLIC LICENSE 2, WHICH IS INCLUDED WITH THIS SOURCE.    *
 * PLEASE READ THESE TERMS BEFORE DISTRIBUTING.                     *
 *                                                                  *
 * THE Ogg123 SOURCE CODE IS (C) COPYRIGHT 2000-2001                *
 * by Stan Seibert <volsung@xiph.org> AND OTHER CONTRIBUTORS        *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

 last mod: $Id: oggvorbis_format.c,v 1.1.2.2 2001/12/09 03:45:26 volsung Exp $

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#include "transport.h"
#include "format.h"


typedef struct ovf_private_t {
  OggVorbis_File vf;
  vorbis_comment *vc;
  vorbis_info *vi;
  int current_section;

  int bos; /* At beginning of logical bitstream */
} ovf_private_t;

/* Forward declarations */
format_t oggvorbis_format;
ov_callbacks vorbisfile_callbacks;


/* Known vorbis comment keys */
struct {
  char *key;         /* includes the '=' for programming convenience */
  char *formatstr;   /* formatted output */
} vorbis_comment_keys[] = {
  {"ARTIST=", "Artist: %s\n"},
  {"ALBUM=", "Album: %s\n"},
  {"TITLE=", "Title: %s\n"},
  {"VERSION=", "Version: %s\n"},
  {"TRACKNUMBER=", "Track number: %s\n"},
  {"ORGANIZATION=", "Organization: %s\n"},
  {"GENRE=", "Genre: %s\n"},
  {"DESCRIPTION=", "Description: %s\n"},
  {"DATE=", "Date: %s\n"},
  {"LOCATION=", "Location: %s\n"},
  {"COPYRIGHT=", "Copyright %s\n"},
  {NULL, "Comment: %s\n"}
};


/* Private functions declarations */
char *lookup_comment_formatstr (char *comment, int *offset);
void print_stream_comments (decoder_t *decoder);
void print_stream_info (decoder_t *decoder);


/* ----------------------------------------------------------- */


int ovf_can_decode (data_source_t *source)
{
  return 1;  /* The file transport is tested last, so always try it */
}


decoder_t* ovf_init (data_source_t *source, audio_format_t *audio_fmt,
		     decoder_callbacks_t *callbacks, void *callback_arg)
{
  decoder_t *decoder;
  ovf_private_t *private;
  int ret;


  /* Allocate data source structures */
  decoder = malloc(sizeof(decoder_t));
  private = malloc(sizeof(ovf_private_t));

  if (decoder != NULL && private != NULL) {
    decoder->source = source;
    decoder->actual_fmt = decoder->request_fmt = *audio_fmt;
    decoder->format = &oggvorbis_format;
    decoder->callbacks = callbacks;
    decoder->callback_arg = callback_arg;
    decoder->private = private;

    private->bos = 1;
    private->current_section = -1;
  } else {
    fprintf(stderr, "Error: Out of memory.\n");
    exit(1);
  }

  /* Initialize vorbisfile decoder */
  
  ret = ov_open_callbacks (decoder, &private->vf, NULL, 0, 
			   vorbisfile_callbacks);

  if (ret < 0) {
    free(private);
    free(source);
    return NULL;
  }

  return decoder;
}


int ovf_read (decoder_t *decoder, void *ptr, int nbytes, int *eos,
	      audio_format_t *audio_fmt)
{
  ovf_private_t *priv = decoder->private;
  decoder_callbacks_t *cb = decoder->callbacks;
  int bytes_read = 0;
  int ret;
  int old_section;

  /* Read comments and audio info at the start of a logical bitstream */
  if (priv->bos) {
    priv->vc = ov_comment(&priv->vf, -1);
    priv->vi = ov_info(&priv->vf, -1);

    decoder->actual_fmt.rate = priv->vi->rate;
    decoder->actual_fmt.channels = priv->vi->channels;

    print_stream_comments(decoder);
    print_stream_info(decoder);
    priv->bos = 0;
  }

  audio_format_copy(audio_fmt, &decoder->actual_fmt);

  /* Attempt to read as much audio as is requested */
  while (nbytes > 0) {

    old_section = priv->current_section;
    ret = ov_read(&priv->vf, ptr, nbytes, audio_fmt->big_endian,
		  audio_fmt->word_size, audio_fmt->signed_sample,
		  &priv->current_section);

    if (ret == 0) {

      /* EOF */
      *eos = 1;
      break;

    } else if (ret == OV_HOLE) {
      
      if (cb->printf_error != NULL)
	cb->printf_error(decoder->callback_arg, WARNING,
			  "--- Hole in the stream; probably harmless\n");
    
    } else if (ret < 0) {
      
      if (cb->printf_error != NULL)
	cb->printf_error(decoder->callback_arg, ERROR,
			 "=== Vorbis library reported a stream error.\n");
      
    } else {
      
      bytes_read += ret;
      ptr += ret;
      nbytes -= ret;

      /* did we enter a new logical bitstream? */
      if (old_section != priv->current_section && old_section != -1) {
	
	*eos = 1;
	priv->bos = 1; /* Read new headers next time through */
	break;
      }
    }

  }
  
  return bytes_read;
}

long ovf_instant_bitrate (decoder_t *decoder)
{
  ovf_private_t *priv = decoder->private;

  return ov_bitrate_instant(&priv->vf);
}


void ovf_cleanup (decoder_t *decoder)
{
  ovf_private_t *priv = decoder->private;

  ov_clear(&priv->vf);

  free(decoder->private);
  free(decoder);
}


format_t oggvorbis_format = {
  "oggvorbis",
  &ovf_can_decode,
  &ovf_init,
  &ovf_read,
  &ovf_instant_bitrate,
  &ovf_cleanup,
};


/* ------------------- Vorbisfile Callbacks ----------------- */

size_t vorbisfile_cb_read (void *ptr, size_t size, size_t nmemb, void *arg)
{
  decoder_t *decoder = arg;

  return decoder->source->transport->read(decoder->source, ptr, size, nmemb);
}

int vorbisfile_cb_seek (void *arg, ogg_int64_t offset, int whence)
{
  decoder_t *decoder = arg;

  return decoder->source->transport->seek(decoder->source, offset, whence);
}

int vorbisfile_cb_close (void *arg)
{
  return 1; /* Ignore close request so transport can be closed later */
}

long vorbisfile_cb_tell (void *arg)
{
  decoder_t *decoder = arg;

  return decoder->source->transport->tell(decoder->source);
}


ov_callbacks vorbisfile_callbacks = {
  &vorbisfile_cb_read,
  &vorbisfile_cb_seek,
  &vorbisfile_cb_close,
  &vorbisfile_cb_tell
};


/* ------------------- Private functions -------------------- */


char *lookup_comment_formatstr (char *comment, int *offset)
{
  int i;

  for (i = 0; vorbis_comment_keys[i].key != NULL; i++) {

    if ( !strncasecmp (vorbis_comment_keys[i].key, comment,
		       strlen(vorbis_comment_keys[i].key)) ) {

      *offset = strlen(vorbis_comment_keys[i].key);
      return vorbis_comment_keys[i].formatstr;
    }

  }

  /* Unrecognized comment, use last format string */
  *offset = 0;
  return vorbis_comment_keys[i].formatstr;
}


void print_stream_comments (decoder_t *decoder)
{
  ovf_private_t *priv = decoder->private;
  decoder_callbacks_t *cb = decoder->callbacks;
  char *comment, *comment_formatstr;
  int offset;
  int i;

  
  if (cb == NULL || cb->printf_metadata == NULL)
    return;


  for (i = 0; i < priv->vc->comments; i++) {
    comment = priv->vc->user_comments[i];
    comment_formatstr = lookup_comment_formatstr(comment, &offset);
    
    cb->printf_metadata(decoder->callback_arg, 1,
			       comment_formatstr, comment + offset);
  }
}

void print_stream_info (decoder_t *decoder)
{
  ovf_private_t *priv = decoder->private;
  decoder_callbacks_t *cb = decoder->callbacks;


  if (cb == NULL || cb->printf_metadata == NULL)
    return;
    

  cb->printf_metadata(decoder->callback_arg, 3,
		      "Version is %d", 
		      priv->vi->version);
  
  cb->printf_metadata(decoder->callback_arg, 3,
		      "Bitrate hints: upper=%ld nominal=%ld lower=%ld "
		      "window=%ld", 
		      priv->vi->bitrate_upper,
		      priv->vi->bitrate_nominal,
		      priv->vi->bitrate_lower,
		      priv->vi->bitrate_window);
  
  cb->printf_metadata(decoder->callback_arg, 2,
		      "Bitstream is %d channel, %ldHz",
		      priv->vi->channels,
		      priv->vi->rate);
  
  cb->printf_metadata(decoder->callback_arg, 2,
		      "Encoded by: %s", priv->vc->vendor);
}

