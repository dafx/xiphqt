/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE Ogg Vorbis SOFTWARE CODEC SOURCE CODE.  *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU PUBLIC LICENSE 2, WHICH IS INCLUDED WITH THIS SOURCE.    *
 * PLEASE READ THESE TERMS DISTRIBUTING.                            *
 *                                                                  *
 * THE OggSQUISH SOURCE CODE IS (C) COPYRIGHT 1994-2000             *
 * by Monty <monty@xiph.org> and The XIPHOPHORUS Company            *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

 function: predefined encoding modes
 last mod: $Id: modes.h,v 1.9.2.8 2000/05/04 23:08:08 xiphmont Exp $

 ********************************************************************/

#ifndef _V_MODES_H_
#define _V_MODES_H_

#include <stdio.h>
#include "vorbis/codec.h"
#include "vorbis/backends.h"

#include "vorbis/book/lsp20_0.vqh"
#include "vorbis/book/lsp32_0.vqh"
#include "vorbis/book/resaux0_short.vqh"
#include "vorbis/book/resaux0_long.vqh"

/* A farily high quality setting mix */
static vorbis_info_psy _psy_set0={
  1,/*athp*/
  1,/*decayp*/
  1,/*smoothp*/
  1,8,0.,

  -130.,

  1,/* tonemaskp*/
  {-35.,-40.,-60.,-80.,-80.}, /* remember that el 4 is an 80 dB curve, not 100 */
  {-35.,-40.,-60.,-80.,-95.},
  {-35.,-40.,-60.,-80.,-95.},
  {-35.,-40.,-60.,-80.,-95.},
  {-35.,-40.,-60.,-80.,-95.},
  {-65.,-60.,-60.,-80.,-95.},  /* remember that el 1 is a 60 dB curve, not 40 */

  1,/*noisemaskp*/
  {-100.,-100.,-100.,-200.,-200.}, /* this is the 500 Hz curve, which
                                      is too wrong to work */
  {-60.,-60.,-60.,-80.,-80.},
  {-60.,-60.,-60.,-80.,-80.},
  {-60.,-60.,-60.,-80.,-80.},
  {-60.,-60.,-60.,-80.,-80.},
  {-50.,-55.,-60.,-80.,-80.},

  110.,

  .9998, .9997  /* attack/decay control */
};

/* with GNUisms, this could be short and readable. Oh well */
static vorbis_info_time0 _time_set0={0};
static vorbis_info_floor0 _floor_set0={20, 44100,  64, 12,150, 1, {0} };
static vorbis_info_floor0 _floor_set1={32, 44100, 256, 12,150, 1, {1} };
static vorbis_info_residue0 _residue_set0={0, 128, 16,1,2,
					   {0},
					   {0},
					   {0},
					   {}};
static vorbis_info_residue0 _residue_set1={0,1024, 16,8,3,
					   {0},
					   {0},
					   {0},
					   {}};
static vorbis_info_mapping0 _mapping_set0={1, {0,0}, {0}, {0}, {0}, {0}};
static vorbis_info_mapping0 _mapping_set1={1, {0,0}, {0}, {1}, {1}, {0}};
static vorbis_info_mode _mode_set0={0,0,0,0};
static vorbis_info_mode _mode_set1={1,0,0,1};

/* CD quality stereo, no channel coupling */
vorbis_info info_A={
  /* channels, sample rate, upperkbps, nominalkbps, lowerkbps */
  0, 2, 44100, 0,0,0,
  /* smallblock, largeblock */
  {256, 2048}, 
  /* modes,maps,times,floors,residues,books,psys */
  2,          2,    1,     2,       2,    4,   1,
  /* modes */
  {&_mode_set0,&_mode_set1},
  /* maps */
  {0,0},{&_mapping_set0,&_mapping_set1},
  /* times */
  {0,0},{&_time_set0},
  /* floors */
  {0,0},{&_floor_set0,&_floor_set1},
  /* residue */
  {0,0},{&_residue_set0,&_residue_set1},
  /* books */
  {&_vq_book_lsp20_0,      /* 0 */
   &_vq_book_lsp32_0,      /* 1 */

   &_huff_book_resaux0_short,
   &_huff_book_resaux0_long,

   /*vq_book_res0_0a,
   &_vq_book_res0_1a,
   &_vq_book_res0_1a,
   &_vq_book_res0_1a,
   &_vq_book_res0_1a,
   &_vq_book_res0_1a,
   &_vq_book_res0_1a,*/
  },
  /* psy */
  {&_psy_set0},
  /* thresh sample period, preecho clamp trigger threshhold, range */
  64, 10, 2 
};

#define PREDEF_INFO_MAX 0

#endif
