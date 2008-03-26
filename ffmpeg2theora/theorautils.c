/* -*- tab-width:4;c-file-style:"cc-mode"; -*- */
/*
 * theorautils.c - Ogg Theora/Ogg Vorbis Abstraction and Muxing
 * Copyright (C) 2003-2005 <j@v2v.cc>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include "theora/theora.h"
#include "vorbis/codec.h"
#include "vorbis/vorbisenc.h"
#ifdef HAVE_OGGKATE
#include "kate/oggkate.h"
#endif

#include "theorautils.h"



static double rint(double x)
{
  if (x < 0.0)
    return (double)(int)(x - 0.5);
  else
    return (double)(int)(x + 0.5);
}

void init_info(oggmux_info *info) {
    info->with_skeleton = 1; /* skeleton is enabled by default    */
    info->frontend = 0; /*frontend mode*/
    info->videotime =  0;
    info->audiotime = 0;
    info->audio_bytesout = 0;
    info->video_bytesout = 0;
    info->kate_bytesout = 0;

    info->videopage_valid = 0;
    info->audiopage_valid = 0;
    info->audiopage_buffer_length = 0;
    info->videopage_buffer_length = 0;
    info->audiopage = NULL;
    info->videopage = NULL;
    info->start_time = time(NULL);
    info->duration = -1;
    info->speed_level = -1;

    info->v_pkg=0;
    info->a_pkg=0;
    info->k_pkg=0;
#ifdef OGGMUX_DEBUG
    info->a_page=0;
    info->v_page=0;
    info->k_page=0;
#endif

    info->with_kate = 0;
    info->n_kate_streams = 0;
}

void oggmux_setup_kate_streams(oggmux_info *info, int n_kate_streams)
{
    int n;

    info->n_kate_streams = n_kate_streams;
    if (n_kate_streams == 0) return;
    info->kate_streams = (oggmux_kate_stream*)malloc(n_kate_streams*sizeof(oggmux_kate_stream));
    for (n=0; n<n_kate_streams; ++n) {
        oggmux_kate_stream *ks=info->kate_streams+n;
        ks->katepage_valid = 0;
        ks->katepage_buffer_length = 0;
        ks->katepage = NULL;
        ks->katetime = 0;
    }
}

static void write16le(unsigned char *ptr,ogg_uint16_t v)
{
  ptr[0]=v&0xff;
  ptr[1]=(v>>8)&0xff;
}

static void write32le(unsigned char *ptr,ogg_uint32_t v)
{
  ptr[0]=v&0xff;
  ptr[1]=(v>>8)&0xff;
  ptr[2]=(v>>16)&0xff;
  ptr[3]=(v>>24)&0xff;
}

static void write64le(unsigned char *ptr,ogg_int64_t v)
{
  ogg_uint32_t hi=v>>32;
  ptr[0]=v&0xff;
  ptr[1]=(v>>8)&0xff;
  ptr[2]=(v>>16)&0xff;
  ptr[3]=(v>>24)&0xff;
  ptr[4]=hi&0xff;
  ptr[5]=(hi>>8)&0xff;
  ptr[6]=(hi>>16)&0xff;
  ptr[7]=(hi>>24)&0xff;
}

void add_fishead_packet (oggmux_info *info) {
    ogg_packet op;

    memset (&op, 0, sizeof (op));

    op.packet = _ogg_calloc (64, sizeof(unsigned char));
    if (op.packet == NULL) return;

    memset (op.packet, 0, 64);
    memcpy (op.packet, FISHEAD_IDENTIFIER, 8); /* identifier */
    write16le(op.packet+8, SKELETON_VERSION_MAJOR); /* version major */
    write16le(op.packet+10, SKELETON_VERSION_MINOR); /* version minor */
    write64le(op.packet+12, (ogg_int64_t)0); /* presentationtime numerator */
    write64le(op.packet+20, (ogg_int64_t)1000); /* presentationtime denominator */
    write64le(op.packet+28, (ogg_int64_t)0); /* basetime numerator */
    write64le(op.packet+36, (ogg_int64_t)1000); /* basetime denominator */
    /* both the numerator are zero hence handled by the memset */
    write32le(op.packet+44, 0); /* UTC time, set to zero for now */

    op.b_o_s = 1; /* its the first packet of the stream */
    op.e_o_s = 0; /* its not the last packet of the stream */
    op.bytes = 64; /* length of the packet in bytes */

    ogg_stream_packetin (&info->so, &op); /* adding the packet to the skeleton stream */
    _ogg_free (op.packet);
}

/*
 * Adds the fishead packets in the skeleton output stream along with the e_o_s packet
 */
void add_fisbone_packet (oggmux_info *info) {
    ogg_packet op;
    int n;

    if (!info->audio_only) {
        memset (&op, 0, sizeof (op));
        op.packet = _ogg_calloc (82, sizeof(unsigned char));
        if (op.packet == NULL) return;

        memset (op.packet, 0, 82);
        /* it will be the fisbone packet for the theora video */
        memcpy (op.packet, FISBONE_IDENTIFIER, 8); /* identifier */
        write32le(op.packet+8, FISBONE_MESSAGE_HEADER_OFFSET); /* offset of the message header fields */
        write32le(op.packet+12, info->to.serialno); /* serialno of the theora stream */
        write32le(op.packet+16, 3); /* number of header packets */
        /* granulerate, temporal resolution of the bitstream in samples/microsecond */
        write64le(op.packet+20, info->ti.fps_numerator); /* granulrate numerator */
        write64le(op.packet+28, info->ti.fps_denominator); /* granulrate denominator */
        write64le(op.packet+36, 0); /* start granule */
        write32le(op.packet+44, 0); /* preroll, for theora its 0 */
        *(op.packet+48) = theora_granule_shift (&info->ti); /* granule shift */
        memcpy(op.packet+FISBONE_SIZE, "Content-Type: video/theora\r\n", 30); /* message header field, Content-Type */

        op.b_o_s = 0;
        op.e_o_s = 0;
        op.bytes = 82; /* size of the packet in bytes */

        ogg_stream_packetin (&info->so, &op);
        _ogg_free (op.packet);
    }

    if (!info->video_only) {
        memset (&op, 0, sizeof (op));
        op.packet = _ogg_calloc (82, sizeof(unsigned char));
        if (op.packet == NULL) return;

        memset (op.packet, 0, 82);
        /* it will be the fisbone packet for the vorbis audio */
        memcpy (op.packet, FISBONE_IDENTIFIER, 8); /* identifier */
        write32le(op.packet+8, FISBONE_MESSAGE_HEADER_OFFSET); /* offset of the message header fields */
        write32le(op.packet+12, info->vo.serialno); /* serialno of the vorbis stream */
        write32le(op.packet+16, 3); /* number of header packet */
        /* granulerate, temporal resolution of the bitstream in Hz */
        write64le(op.packet+20, info->sample_rate); /* granulerate numerator */
        write64le(op.packet+28, (ogg_int64_t)1); /* granulerate denominator */
        write64le(op.packet+36, 0); /* start granule */
        write32le(op.packet+44, 2); /* preroll, for vorbis its 2 */
        *(op.packet+48) = 0; /* granule shift, always 0 for vorbis */
        memcpy (op.packet+FISBONE_SIZE, "Content-Type: audio/vorbis\r\n", 30);
        /* Important: Check the case of Content-Type for correctness */

        op.b_o_s = 0;
        op.e_o_s = 0;
        op.bytes = 82;

        ogg_stream_packetin (&info->so, &op);
        _ogg_free (op.packet);
    }

#ifdef HAVE_KATE
    if (info->with_kate) {
        for (n=0; n<info->n_kate_streams; ++n) {
            oggmux_kate_stream *ks=info->kate_streams+n;
	    memset (&op, 0, sizeof (op));
	    op.packet = _ogg_calloc (86, sizeof(unsigned char));
	    memset (op.packet, 0, 86);
            /* it will be the fisbone packet for the kate stream */
	    memcpy (op.packet, FISBONE_IDENTIFIER, 8); /* identifier */
            write32le(op.packet+8, FISBONE_MESSAGE_HEADER_OFFSET); /* offset of the message header fields */
	    write32le(op.packet+12, ks->ko.serialno); /* serialno of the vorbis stream */
            write32le(op.packet+16, ks->ki.num_headers); /* number of header packet */
	    /* granulerate, temporal resolution of the bitstream in Hz */
	    write64le(op.packet+20, ks->ki.gps_numerator); /* granulerate numerator */
            write64le(op.packet+28, ks->ki.gps_denominator); /* granulerate denominator */
	    write64le(op.packet+36, 0); /* start granule */
            write32le(op.packet+44, 0); /* preroll, for kate it's 0 */
	    *(op.packet+48) = ks->ki.granule_shift; /* granule shift */
            memcpy (op.packet+FISBONE_SIZE, "Content-Type: application/x-kate\r\n", 34);
	    /* Important: Check the case of Content-Type for correctness */
	
	    op.b_o_s = 0;
	    op.e_o_s = 0;
	    op.bytes = 86;
	
            ogg_stream_packetin (&info->so, &op);
	    _ogg_free (op.packet);
        }
    }
#endif
}

void oggmux_init (oggmux_info *info){
    ogg_page og;
    ogg_packet op;

    /* yayness.  Set up Ogg output stream */
    srand (time (NULL));
    ogg_stream_init (&info->vo, rand ());

    if(!info->audio_only){
        ogg_stream_init (&info->to, rand ());    /* oops, add one ot the above */
        theora_encode_init (&info->td, &info->ti);

        if(info->speed_level >= 0) {
          int max_speed_level;
          theora_control(&info->td, TH_ENCCTL_GET_SPLEVEL_MAX, &max_speed_level, sizeof(int));
          if(info->speed_level > max_speed_level)
            info->speed_level = max_speed_level;
          theora_control(&info->td, TH_ENCCTL_SET_SPLEVEL, &info->speed_level, sizeof(int));
        }
    }
    /* init theora done */

    /* initialize Vorbis too, if we have audio. */
    if(!info->video_only){
        int ret;
        vorbis_info_init (&info->vi);
        /* Encoding using a VBR quality mode.  */
        if(info->vorbis_quality>-99)
            ret =vorbis_encode_init_vbr (&info->vi, info->channels,info->sample_rate,info->vorbis_quality);
        else
            ret=vorbis_encode_init(&info->vi,info->channels,info->sample_rate,-1,info->vorbis_bitrate,-1);

        if (ret){
            fprintf (stderr,
                 "The Vorbis encoder could not set up a mode according to\n"
                 "the requested quality or bitrate.\n\n");
            exit (1);
        }

        vorbis_comment_init (&info->vc);
        vorbis_comment_add_tag (&info->vc, "ENCODER",PACKAGE_STRING);
        /* set up the analysis state and auxiliary encoding storage */
        vorbis_analysis_init (&info->vd, &info->vi);
        vorbis_block_init (&info->vd, &info->vb);

    }
    /* audio init done */

    /* initialize kate if we have subtitles */
    if (info->with_kate) {
        int ret, n;
#ifdef HAVE_KATE
        for (n=0; n<info->n_kate_streams; ++n) {
            oggmux_kate_stream *ks=info->kate_streams+n;
            ogg_stream_init (&ks->ko, rand ());    /* oops, add one ot the above */
            ret = kate_encode_init (&ks->k, &ks->ki);
            if (ret<0) fprintf(stderr, "kate_encode_init: %d\n",ret);
            ret = kate_comment_init(&ks->kc);
            if (ret<0) fprintf(stderr, "kate_comment_init: %d\n",ret);
            kate_comment_add_tag (&ks->kc, "ENCODER",PACKAGE_STRING);
        }
#endif
    }
    /* kate init done */

    /* first packet should be skeleton fishead packet, if skeleton is used */

    if (info->with_skeleton) {
    ogg_stream_init (&info->so, rand());
    add_fishead_packet (info);
    if (ogg_stream_pageout (&info->so, &og) != 1){
            fprintf (stderr, "Internal Ogg library error.\n");
            exit (1);
        }
        fwrite (og.header, 1, og.header_len, info->outfile);
        fwrite (og.body, 1, og.body_len, info->outfile);
    }

    /* write the bitstream header packets with proper page interleave */

    /* first packet will get its own page automatically */
    if(!info->audio_only){
        theora_encode_header (&info->td, &op);
        ogg_stream_packetin (&info->to, &op);
        if (ogg_stream_pageout (&info->to, &og) != 1){
            fprintf (stderr, "Internal Ogg library error.\n");
            exit (1);
        }
        fwrite (og.header, 1, og.header_len, info->outfile);
        fwrite (og.body, 1, og.body_len, info->outfile);

        /* create the remaining theora headers */
        /* theora_comment_init (&info->tc); is called in main() prior to parsing options */
        theora_comment_add_tag (&info->tc, "ENCODER",PACKAGE_STRING);
        theora_encode_comment (&info->tc, &op);
        ogg_stream_packetin (&info->to, &op);
        theora_encode_tables (&info->td, &op);
        ogg_stream_packetin (&info->to, &op);
    }
    if(!info->video_only){
        ogg_packet header;
        ogg_packet header_comm;
        ogg_packet header_code;

        vorbis_analysis_headerout (&info->vd, &info->vc, &header,
                       &header_comm, &header_code);
        ogg_stream_packetin (&info->vo, &header);    /* automatically placed in its own
                                 * page */
        if (ogg_stream_pageout (&info->vo, &og) != 1){
            fprintf (stderr, "Internal Ogg library error.\n");
            exit (1);
        }
        fwrite (og.header, 1, og.header_len, info->outfile);
        fwrite (og.body, 1, og.body_len, info->outfile);

        /* remaining vorbis header packets */
        ogg_stream_packetin (&info->vo, &header_comm);
        ogg_stream_packetin (&info->vo, &header_code);
    }

#ifdef HAVE_KATE
    if (info->with_kate) {
        int n;
        for (n=0; n<info->n_kate_streams; ++n) {
            oggmux_kate_stream *ks=info->kate_streams+n;
            int ret;
            while (1) {
                ret=kate_ogg_encode_headers(&ks->k,&ks->kc,&op);
                if (ret==0) {
                  ogg_stream_packetin(&ks->ko,&op);
                  ogg_packet_clear(&op);
                }
                if (ret<0) fprintf(stderr, "kate_encode_headers: %d\n",ret);
                if (ret>0) break;
            }

            /* first header is on a separate page - libogg will do it automatically */
            ret=ogg_stream_pageout (&ks->ko, &og);
            if (ret!=1) {
                fprintf (stderr, "Internal Ogg library error.\n");
                exit (1);
            }
            fwrite (og.header, 1, og.header_len, info->outfile);
            fwrite (og.body, 1, og.body_len, info->outfile);
        }
    }
#endif

    /* output the appropriate fisbone packets */
    if (info->with_skeleton) {
    add_fisbone_packet (info);
    while (1) {
        int result = ogg_stream_flush (&info->so, &og);
            if (result < 0){
            /* can't get here */
            fprintf (stderr, "Internal Ogg library error.\n");
        exit (1);
            }
        if (result == 0)
            break;
            fwrite (og.header, 1, og.header_len, info->outfile);
        fwrite (og.body, 1, og.body_len, info->outfile);
    }
    }

    if (!info->audio_only) {
    theora_info_clear(&info->ti);
    }

    /* Flush the rest of our headers. This ensures
     * the actual data in each stream will start
     * on a new page, as per spec. */
    while (1 && !info->audio_only){
        int result = ogg_stream_flush (&info->to, &og);
        if (result < 0){
            /* can't get here */
            fprintf (stderr, "Internal Ogg library error.\n");
            exit (1);
        }
        if (result == 0)
            break;
        fwrite (og.header, 1, og.header_len, info->outfile);
        fwrite (og.body, 1, og.body_len, info->outfile);
    }
    while (1 && !info->video_only){
        int result = ogg_stream_flush (&info->vo, &og);
        if (result < 0){
            /* can't get here */
            fprintf (stderr, "Internal Ogg library error.\n");
            exit (1);
        }
        if (result == 0)
            break;
        fwrite (og.header, 1, og.header_len,info->outfile);
        fwrite (og.body, 1, og.body_len, info->outfile);
    }
    if (info->with_kate) {
        int n;
        for (n=0; n<info->n_kate_streams; ++n) {
            oggmux_kate_stream *ks=info->kate_streams+n;
            while (1) {
                int result = ogg_stream_flush (&ks->ko, &og);
                if (result < 0){
                    /* can't get here */
                    fprintf (stderr, "Internal Ogg library error.\n");
                    exit (1);
                }
                if (result == 0)
                    break;
                fwrite (og.header, 1, og.header_len,info->outfile);
                fwrite (og.body, 1, og.body_len, info->outfile);
            }
        }
    }

    if (info->with_skeleton) {
    int result;

    /* build and add the e_o_s packet */
    memset (&op, 0, sizeof (op));
        op.b_o_s = 0;
    op.e_o_s = 1; /* its the e_o_s packet */
        op.granulepos = 0;
    op.bytes = 0; /* e_o_s packet is an empty packet */
        ogg_stream_packetin (&info->so, &op);

    result = ogg_stream_flush (&info->so, &og);
        if (result < 0){
            /* can't get here */
            fprintf (stderr, "Internal Ogg library error.\n");
            exit (1);
        }
        fwrite (og.header, 1, og.header_len,info->outfile);
        fwrite (og.body, 1, og.body_len, info->outfile);
    }
}

/**
 * adds a video frame to the encoding sink
 * if e_o_s is 1 the end of the logical bitstream will be marked.
 * @param this ff2theora struct
 * @param info oggmux_info
 * @param yuv_buffer
 * @param e_o_s 1 indicates ond of stream
 */
void oggmux_add_video (oggmux_info *info, yuv_buffer *yuv, int e_o_s){
    ogg_packet op;
    theora_encode_YUVin (&info->td, yuv);
    while(theora_encode_packetout (&info->td, e_o_s, &op)) {
      ogg_stream_packetin (&info->to, &op);
      info->v_pkg++;
    }
}

/**
 * adds audio samples to encoding sink
 * @param buffer pointer to buffer
 * @param bytes bytes in buffer
 * @param samples samples in buffer
 * @param e_o_s 1 indicates end of stream.
 */
void oggmux_add_audio (oggmux_info *info, int16_t * buffer, int bytes, int samples, int e_o_s){
    ogg_packet op;

    int i,j, count = 0;
    float **vorbis_buffer;
    if (bytes <= 0 && samples <= 0){
        /* end of audio stream */
        if(e_o_s)
            vorbis_analysis_wrote (&info->vd, 0);
    }
    else{
        vorbis_buffer = vorbis_analysis_buffer (&info->vd, samples);
        /* uninterleave samples */
        for (i = 0; i < samples; i++){
            for(j=0;j<info->channels;j++){
                vorbis_buffer[j][i] = buffer[count++] / 32768.f;
            }
        }
        vorbis_analysis_wrote (&info->vd, samples);
    }
    while(vorbis_analysis_blockout (&info->vd, &info->vb) == 1){
        /* analysis, assume we want to use bitrate management */
        vorbis_analysis (&info->vb, NULL);
        vorbis_bitrate_addblock (&info->vb);

        /* weld packets into the bitstream */
        while (vorbis_bitrate_flushpacket (&info->vd, &op)){
            ogg_stream_packetin (&info->vo, &op);
            info->a_pkg++;
        }
    }
}

/**    
 * adds a subtitles text to the encoding sink
 * if e_o_s is 1 the end of the logical bitstream will be marked.
 * @param info oggmux_info
 * @param idx which kate stream to output to
 * @param t0 the show time of the text
 * @param t1 the hide time of the text
 * @param text the utf-8 text
 * @param len the number of bytes in the text
 * @param e_o_s 1 indicates end of stream
 */
void oggmux_add_kate_text (oggmux_info *info, int idx, double t0, double t1, const char *text, size_t len, int e_o_s){
#ifdef HAVE_KATE
    ogg_packet op;
    oggmux_kate_stream *ks=info->kate_streams+idx;
    int ret;
    ret = kate_ogg_encode_text(&ks->k, t0, t1, text, len, &op);
    if (ret>=0) {
        ogg_stream_packetin (&ks->ko, &op);
        info->k_pkg++;
    }
    else {
        fprintf(stderr, "Failed to encode kate data packet (%f --> %f, [%s]): %d",
            t0, t1, text, ret);
    }
    if(e_o_s) {
        ret = kate_ogg_encode_finish(&ks->k, -1, &op);
        if (ret>=0) {
            ogg_stream_packetin (&ks->ko, &op);
            info->k_pkg++;
        }
        else {
            fprintf(stderr, "Failed to encode kate end packet: %d", ret);
        }
    }
#endif
}
    
/**    
 * adds a kate end packet to the encoding sink
 * @param info oggmux_info
 * @param idx which kate stream to output to
 * @param t the time of the end packet
 */
void oggmux_add_kate_end_packet (oggmux_info *info, int idx, double t){
#ifdef HAVE_KATE
    ogg_packet op;
    oggmux_kate_stream *ks=info->kate_streams+idx;
    int ret;
    ret = kate_ogg_encode_finish(&ks->k, t, &op);
    if (ret>=0) {
        ogg_stream_packetin (&ks->ko, &op);
        info->k_pkg++;
    }
    else {
        fprintf(stderr, "Failed to encode kate end packet: %d", ret);
    }
#endif
}
    
static double get_remaining(oggmux_info *info, double timebase) {
  double remaining = 0;
  double to_encode, time_so_far;

  if(info->duration != -1 && timebase > 0) {
    time_so_far = time(NULL) - info->start_time;
    to_encode = info->duration - timebase;
    if(to_encode > 0) {
      remaining = (time_so_far / timebase) * to_encode;
    }
  }
  return remaining;
}

static void print_stats(oggmux_info *info, double timebase){
    int hundredths = timebase * 100 - (long) timebase * 100;
    int seconds = (long) timebase % 60;
    int minutes = ((long) timebase / 60) % 60;
    int hours = (long) timebase / 3600;
    double remaining = get_remaining(info, timebase);
    int remaining_seconds = (long) remaining % 60;
    int remaining_minutes = ((long) remaining / 60) % 60;
    int remaining_hours = (long) remaining / 3600;
    if(info->frontend) {
        fprintf (stderr,"\nf2t ;position: %.02lf;audio_kbps: %d;video_kbps: %d;remaining: %.02lf\n",
           timebase,
           info->akbps, info->vkbps,
           remaining
           );

    }
    else {
      if(!remaining) {
          remaining = time(NULL) - info->start_time;
          remaining_seconds = (long) remaining % 60;
          remaining_minutes = ((long) remaining / 60) % 60;
          remaining_hours = (long) remaining / 3600;
          fprintf (stderr,"\r      %d:%02d:%02d.%02d audio: %dkbps video: %dkbps, time elapsed: %02d:%02d:%02d      ",
           hours, minutes, seconds, hundredths,
           info->akbps, info->vkbps,
           remaining_hours, remaining_minutes, remaining_seconds
           );
      }
      else {
          fprintf (stderr,"\r      %d:%02d:%02d.%02d audio: %dkbps video: %dkbps, time remaining: %02d:%02d:%02d      ",
           hours, minutes, seconds, hundredths,
           info->akbps, info->vkbps,
           remaining_hours, remaining_minutes, remaining_seconds
           );
      }
    }
}

static void write_audio_page(oggmux_info *info)
{
  int ret;

  ret = fwrite(info->audiopage, 1, info->audiopage_len, info->outfile);
  if(ret < info->audiopage_len) {
    fprintf(stderr,"error writing audio page\n");
  }
  else {
    info->audio_bytesout += ret;
  }
  info->audiopage_valid = 0;
  info->a_pkg -=ogg_page_packets((ogg_page *)&info->audiopage);
#ifdef OGGMUX_DEBUG
  info->a_page++;
  info->v_page=0;
  fprintf(stderr,"\naudio page %d (%d pkgs) | pkg remaining %d\n",info->a_page,ogg_page_packets((ogg_page *)&info->audiopage),info->a_pkg);
#endif

  info->akbps = rint (info->audio_bytesout * 8. / info->audiotime * .001);
  if(info->akbps<0)
    info->akbps=0;
  print_stats(info, info->audiotime);
}

static void write_video_page(oggmux_info *info)
{
  int ret;

  ret = fwrite(info->videopage, 1, info->videopage_len, info->outfile);
  if(ret < info->videopage_len) {
    fprintf(stderr,"error writing video page\n");
  }
  else {
    info->video_bytesout += ret;
  }
  info->videopage_valid = 0;
  info->v_pkg -= ogg_page_packets((ogg_page *)&info->videopage);
#ifdef OGGMUX_DEBUG
  info->v_page++;
  info->a_page=0;
  fprintf(stderr,"\nvideo page %d (%d pkgs) | pkg remaining %d\n",info->v_page,ogg_page_packets((ogg_page *)&info->videopage),info->v_pkg);
#endif


  info->vkbps = rint (info->video_bytesout * 8. / info->videotime * .001);
  if(info->vkbps<0)
    info->vkbps=0;
  print_stats(info, info->videotime);
}

static void write_kate_page(oggmux_info *info, int idx)
{
  int ret;
  oggmux_kate_stream *ks=info->kate_streams+idx;

  ret = fwrite(ks->katepage, 1, ks->katepage_len, info->outfile);
  if(ret < ks->katepage_len) {
    fprintf(stderr,"error writing kate page\n");
  }
  else {
    info->kate_bytesout += ret;
  }
  ks->katepage_valid = 0;
  info->k_pkg -= ogg_page_packets((ogg_page *)&ks->katepage);
#ifdef OGGMUX_DEBUG
  ks->k_page++;
  fprintf(stderr,"\nkate page %d (%d pkgs) | pkg remaining %d\n",ks->k_page,ogg_page_packets((ogg_page *)&info->katepage),info->k_pkg);
#endif


  /*
  info->kkbps = rint (info->kate_bytesout * 8. / info->katetime * .001);
  if(info->kkbps<0)
    info->kkbps=0;
  print_stats(info, info->katetime);
  */
}

static int find_best_valid_kate_page(oggmux_info *info)
{
  int n;
  double t=0.0;
  int best=-1;
  if (info->with_kate) for (n=0; n<info->n_kate_streams;++n) {
    oggmux_kate_stream *ks=info->kate_streams+n;
    if (ks->katepage_valid) {
      if (best==-1 || ks->katetime<t) {
        t=ks->katetime;
        best=n;
      }
    }
  }
  return best;
}

void oggmux_flush (oggmux_info *info, int e_o_s)
{
    int n,len;
    ogg_page og;
    int best;

    /* flush out the ogg pages to info->outfile */
    while(1) {
      /* Get pages for both streams, if not already present, and if available.*/
      if(!info->audio_only && !info->videopage_valid) {
        // this way seeking is much better,
        // not sure if 23 packets  is a good value. it works though
        int v_next=0;
        if(info->v_pkg>22 && ogg_stream_flush(&info->to, &og) > 0) {
          v_next=1;
        }
        else if(ogg_stream_pageout(&info->to, &og) > 0) {
          v_next=1;
        }
        if(v_next) {
          len = og.header_len + og.body_len;
          if(info->videopage_buffer_length < len) {
            info->videopage = realloc(info->videopage, len);
            info->videopage_buffer_length = len;
          }
          info->videopage_len = len;
          memcpy(info->videopage, og.header, og.header_len);
          memcpy(info->videopage+og.header_len , og.body, og.body_len);

          info->videopage_valid = 1;
          if(ogg_page_granulepos(&og)>0) {
            info->videotime = theora_granule_time (&info->td,
                  ogg_page_granulepos(&og));
          }
        }
      }
      if(!info->video_only && !info->audiopage_valid) {
        // this way seeking is much better,
        // not sure if 23 packets  is a good value. it works though
        int a_next=0;
        if(info->a_pkg>22 && ogg_stream_flush(&info->vo, &og) > 0) {
          a_next=1;
        }
        else if(ogg_stream_pageout(&info->vo, &og) > 0) {
          a_next=1;
        }
        if(a_next) {
          len = og.header_len + og.body_len;
          if(info->audiopage_buffer_length < len) {
            info->audiopage = realloc(info->audiopage, len);
            info->audiopage_buffer_length = len;
          }
          info->audiopage_len = len;
          memcpy(info->audiopage, og.header, og.header_len);
          memcpy(info->audiopage+og.header_len , og.body, og.body_len);

          info->audiopage_valid = 1;
          if(ogg_page_granulepos(&og)>0) {
            info->audiotime= vorbis_granule_time (&info->vd,
                  ogg_page_granulepos(&og));
          }
        }
      }

#ifdef HAVE_KATE
      if (info->with_kate) for (n=0; n<info->n_kate_streams; ++n) {
        oggmux_kate_stream *ks=info->kate_streams+n;
        if (!ks->katepage_valid) {
          int k_next=0;
          /* always flush kate stream */
          if (ogg_stream_flush(&ks->ko, &og) > 0) {
            k_next = 1;
          }
          if (k_next) {
            len = og.header_len + og.body_len;
            if(ks->katepage_buffer_length < len) {
              ks->katepage = realloc(ks->katepage, len);
              ks->katepage_buffer_length = len;
            }
            ks->katepage_len = len;
            memcpy(ks->katepage, og.header, og.header_len);
            memcpy(ks->katepage+og.header_len , og.body, og.body_len);

            ks->katepage_valid = 1;
            if(ogg_page_granulepos(&og)>0) {
              ks->katetime= kate_granule_time (&ks->ki,
                    ogg_page_granulepos(&og));
            }
          }
        }
      }
#endif

#ifdef HAVE_KATE
#define CHECK_KATE_OUTPUT(which) \
        if (best>=0 && info->kate_streams[best].katetime/*-1.0*/<=info->which##time) { \
          write_kate_page(info, best); \
          continue; \
        }
#else
#define CHECK_KATE_OUTPUT(which) ((void)0)
#endif

      best=find_best_valid_kate_page(info);

      if(info->video_only && info->videopage_valid) {
        CHECK_KATE_OUTPUT(video);
        write_video_page(info);
      }
      else if(info->audio_only && info->audiopage_valid) {
        CHECK_KATE_OUTPUT(audio);
        write_audio_page(info);
      }
      /* We're using both. We can output only:
       *  a) If we have valid pages for both
       *  b) At EOS, for the remaining stream.
       */
      else if(info->videopage_valid && info->audiopage_valid) {
        /* Make sure they're in the right order. */
        if(info->videotime <= info->audiotime) {
          CHECK_KATE_OUTPUT(video);
          write_video_page(info);
        }
        else {
          CHECK_KATE_OUTPUT(audio);
          write_audio_page(info);
        }
      }
      else if(e_o_s && best>=0) {
          write_kate_page(info, best);
      }
      else if(e_o_s && info->videopage_valid) {
          write_video_page(info);
      }
      else if(e_o_s && info->audiopage_valid) {
          write_audio_page(info);
      }
      else {
        break; /* Nothing more writable at the moment */
      }
    }
}

void oggmux_close (oggmux_info *info){
    int n;

    ogg_stream_clear (&info->vo);
    vorbis_block_clear (&info->vb);
    vorbis_dsp_clear (&info->vd);
    vorbis_comment_clear (&info->vc);
    vorbis_info_clear (&info->vi);

    ogg_stream_clear (&info->to);
    theora_clear (&info->td);

#ifdef HAVE_KATE
    for (n=0; n<info->n_kate_streams; ++n) {
        ogg_stream_clear (&info->kate_streams[n].ko);
        kate_comment_clear (&info->kate_streams[n].kc);
        kate_info_clear (&info->kate_streams[n].ki);
        kate_clear (&info->kate_streams[n].k);
    }
#endif

    if (info->outfile && info->outfile != stdout)
        fclose (info->outfile);

    if(info->videopage)
      free(info->videopage);
    if(info->audiopage)
      free(info->audiopage);

    for (n=0; n<info->n_kate_streams; ++n) {
        if(info->kate_streams[n].katepage)
          free(info->kate_streams[n].katepage);
    }
}
