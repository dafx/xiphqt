/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2001             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: bitrate tracking and management
 last mod: $Id: bitrate.h,v 1.1.2.1 2001/11/16 08:17:06 xiphmont Exp $

 ********************************************************************/

#ifndef _V_BITRATE_H_
#define _V_BITRATE_H_

#include "vorbis/codec.h"
#include "codec_internal.h"
#include "os.h"

/* encode side bitrate tracking */
#define BITTRACK_DIVISOR 16
typedef struct bitrate_manager_state {
  ogg_uint32_t  *queue_binned;
  ogg_uint32_t  *queue_actual;
  int            queue_size;

  int            queue_head;
  int            queue_bins;

  long          *avg_binacc;
  int            avg_center;
  int            avg_tail;
  ogg_uint32_t   avg_centeracc;
  ogg_uint32_t   avg_sampleacc;
  ogg_uint32_t   avg_sampledesired;
  ogg_uint32_t   avg_centerdesired;

  long          *minmax_binstack;
  long          *minmax_posstack;
  long          *minmax_limitstack;
  long           minmax_stackptr;

  long           minmax_acctotal;
  int            minmax_tail;
  ogg_uint32_t   minmax_sampleacc;
  ogg_uint32_t   minmax_sampledesired;

  int            next_to_flush;
  int            last_to_flush;
  
  double         avgfloat;
  double         avgnoise;
  long           noisetrigger_postpone;
  long           noisetrigger_request;

  /* unfortunately, we need to hold queued packet data somewhere */
  unsigned char *packetdoublebuffer[2];
  unsigned char doublebufferhead[2];
  unsigned char doublebuffertail[2];
  oggpack_buffer *queue_packets;

} bitrate_manager_state;

typedef struct bitrate_manager_info{
  /* detailed bitrate management setup */
  double absolute_min_short;
  double absolute_min_long;
  double absolute_max_short;
  double absolute_max_long;

  double queue_avg_time;
  double queue_minmax_time;
  double queue_hardmin;
  double queue_hardmax;
  double queue_avgmin;
  double queue_avgmax;

  double avgfloat_initial; /* set by mode */
  double avgfloat_minimum; /* set by mode */
  double avgfloat_downslew_max;
  double avgfloat_upslew_max;
  double avgfloat_downhyst;
  double avgfloat_uphyst;
  double avgfloat_noise_lowtrigger;
  double avgfloat_noise_hightrigger;
  double avgfloat_noise_minval;
  double avgfloat_noise_maxval;
} bitrate_manager_info;

extern void vorbis_bitrate_init(vorbis_info *vi,bitrate_manager_state *bs);
extern void vorbis_bitrate_clear(bitrate_manager_state *bs);
extern int vorbis_bitrate_addblock(vorbis_block *vb);
extern int vorbis_bitrate_flushpacket(vorbis_block *vb, vorbis_packet *op);

#endif
