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

 function: toplevel residue templates for 32/44.1/48kHz
 last mod: $Id: residue_44.h,v 1.11.6.5 2002/06/11 04:44:49 xiphmont Exp $

 ********************************************************************/

#include "vorbis/codec.h"
#include "backends.h"

/***** residue backends *********************************************/

/* the books used depend on stereo-or-not, but the residue setup below
   can apply to coupled or not.  These templates are for a first pass;
   a last pass is mechanically added in vorbisenc for residue backfill
   at 1/3 and 1/9, as well as an optional middle pass for stereo
   backfill */

/*     0   1   2   4  26   1   2   4  26   +      
           0   0   0   0         

       0   1   2   3   4   5   6   7   8   9
   1                   .               .   .
   2                   .               .   .
   4       .   .   .       .   .   .       .
 
       0   4   4   4   3   4   4   4   3   7 */
static vorbis_info_residue0 _residue_44_low={
  0,-1, -1, 8,-1,
  {0},
  {-1},
  {  .5, 1.5,   4.5,  1.5,  2.5,  4.5, 26.5},
  {  .5,   .5,   .5, 999., 999., 999., 999.},
};
/* 26 doesn't cascade well; use 28 instead */
static vorbis_info_residue0 _residue_44_low_un={
  0,-1, -1, 8,-1,
  {0},
  {-1},
  {  .5,  1.5,  4.5,  1.5,  2.5,  4.5, 28.5},
  {  99,   -1,   -1,   -1,   -1,   99,   99}
};


static vorbis_info_residue0 _residue_44_mid={
  0,-1, -1, 10,-1,
  /*  0     1     2     3     4     5     6     7     8     9 */
  {0},
  {-1},
  {  .5,  1.5,  1.5,  2.5,  2.5,  4.5,  8.5,  16.5, 32.5},
  {  .5,   .5, 999.,  1.5,  999., 4.5,  8.5,  16.5, 32.5},
};

static vorbis_info_residue0 _residue_44_mid_un={
  0,-1, -1, 10,-1,
  /*  0     1     2     3     4     5     6     7     8     9 */
  {0},
  {-1},
  {  .5,  1.5,  1.5,  2.5,  2.5,  4.5,  4.5, 16.5, 42.5},
  {  .5,   .5, 999.,   .5, 999.,   .5, 999., 999., 999,},
};

/*     0   8  42   1   2   4   8  16  56   +      
           0   0   0            

       0   1   2   3   4   5   6   7   8   9
   1           .                   .   .   .
   2           .                   .   .   .
   4       .       .   .   .   .           .
 
       0   4   3   4   4   4   4   3   3   7 */
static vorbis_info_residue0 _residue_44_high={
  0,-1, -1, 10,-1,
  /*  0     1     2     3     4     5     6     7     8     9 */
  {0},
  {-1},
  {  .5,  1.5,  2.5,  4.5,  8.5,  8.5, 16.5, 42.5, 56.5},
  {  .5,  1.5,  2.5,  2.5,  4.5,  8.5,  8.5, 16.5, 22.5},
};
/* 56 doesn't cascade well; use 59 */
static vorbis_info_residue0 _residue_44_high_un={
  0,-1, -1, 10,-1,
  /*  0     1     2     3     4     5     6     7     8     9 */
  {0},
  {-1},
  {  .5,  8.5, 42.5,  1.5,  2.5,  4.5,  8.5, 16.5, 59.5},
  {  99,   -1,   -1,   99,   99,   99,   99,   99,   99}
};

#include "books/coupled/_44c0_short.vqh"
#include "books/coupled/_44c0_long.vqh"

#include "books/coupled/_44c0_s0_p1_0.vqh"
#include "books/coupled/_44c0_s0_p2_0.vqh"
#include "books/coupled/_44c0_s0_p3_0.vqh"
#include "books/coupled/_44c0_s0_p4_0.vqh"
#include "books/coupled/_44c0_s0_p4_1.vqh"
#include "books/coupled/_44c0_s0_p5_0.vqh"
#include "books/coupled/_44c0_s0_p6_0.vqh"
#include "books/coupled/_44c0_s0_p7_0.vqh"
#include "books/coupled/_44c0_s0_p7_1.vqh"
#include "books/coupled/_44c0_s0_p7_2.vqh"

#include "books/coupled/_44c1_short.vqh"
#include "books/coupled/_44c1_long.vqh"

#include "books/coupled/_44c1_s0_p1_0.vqh"
#include "books/coupled/_44c1_s0_p2_0.vqh"
#include "books/coupled/_44c1_s0_p3_0.vqh"
#include "books/coupled/_44c1_s0_p4_0.vqh"
#include "books/coupled/_44c1_s0_p4_1.vqh"
#include "books/coupled/_44c1_s0_p5_0.vqh"
#include "books/coupled/_44c1_s0_p6_0.vqh"
#include "books/coupled/_44c1_s2_p6_0.vqh"
#include "books/coupled/_44c1_s0_p7_0.vqh"
#include "books/coupled/_44c1_s0_p7_1.vqh"
#include "books/coupled/_44c1_s0_p7_2.vqh"

#include "books/coupled/_44c2_short.vqh"
#include "books/coupled/_44c2_long.vqh"

#include "books/coupled/_44c2_s0_p1_0.vqh"
#include "books/coupled/_44c2_s0_p2_0.vqh"
#include "books/coupled/_44c2_s0_p3_0.vqh"
#include "books/coupled/_44c2_s0_p4_0.vqh"
#include "books/coupled/_44c2_s0_p4_1.vqh"
#include "books/coupled/_44c2_s0_p5_0.vqh"
#include "books/coupled/_44c2_s0_p6_0.vqh"
#include "books/coupled/_44c2_s0_p7_0.vqh"
#include "books/coupled/_44c2_s0_p7_1.vqh"
#include "books/coupled/_44c2_s0_p7_2.vqh"

#include "books/coupled/_44c3_short.vqh"
#include "books/coupled/_44c3_long.vqh"

#include "books/coupled/_44c3_s0_p1_0.vqh"
#include "books/coupled/_44c3_s0_p2_0.vqh"
#include "books/coupled/_44c3_s0_p3_0.vqh"
#include "books/coupled/_44c3_s0_p4_0.vqh"
#include "books/coupled/_44c3_s0_p4_1.vqh"
#include "books/coupled/_44c3_s0_p5_0.vqh"
#include "books/coupled/_44c3_s0_p6_0.vqh"
#include "books/coupled/_44c3_s0_p7_0.vqh"
#include "books/coupled/_44c3_s0_p7_1.vqh"
#include "books/coupled/_44c3_s0_p7_2.vqh"

#include "books/coupled/_44c4_s_short.vqh"
#include "books/coupled/_44c4_s_long.vqh"

#include "books/coupled/_44c4_s_p1_0.vqh"
#include "books/coupled/_44c4_s_p2_0.vqh"
#include "books/coupled/_44c4_s_p3_0.vqh"
#include "books/coupled/_44c4_s_p4_0.vqh"
#include "books/coupled/_44c4_s_p5_0.vqh"
#include "books/coupled/_44c4_s_p6_0.vqh"
#include "books/coupled/_44c4_s_p7_0.vqh"
#include "books/coupled/_44c4_s_p7_1.vqh"
#include "books/coupled/_44c4_s_p8_0.vqh"
#include "books/coupled/_44c4_s_p8_1.vqh"
#include "books/coupled/_44c4_s_p9_0.vqh"
#include "books/coupled/_44c4_s_p9_1.vqh"
#include "books/coupled/_44c4_s_p9_2.vqh"

#include "books/coupled/_44c4_sm_short.vqh"
#include "books/coupled/_44c4_sm_long.vqh"

#include "books/coupled/_44c4_sm_p1_0.vqh"
#include "books/coupled/_44c4_sm_p2_0.vqh"
#include "books/coupled/_44c4_sm_p3_0.vqh"
#include "books/coupled/_44c4_sm_p4_0.vqh"
#include "books/coupled/_44c4_sm_p5_0.vqh"
#include "books/coupled/_44c4_sm_p6_0.vqh"
#include "books/coupled/_44c4_sm_p7_0.vqh"
#include "books/coupled/_44c4_sm_p7_1.vqh"
#include "books/coupled/_44c4_sm_p8_0.vqh"
#include "books/coupled/_44c4_sm_p8_1.vqh"
#include "books/coupled/_44c4_sm_p9_0.vqh"
#include "books/coupled/_44c4_sm_p9_1.vqh"
#include "books/coupled/_44c4_sm_p9_2.vqh"


#include "books/coupled/_44c5_short.vqh"
#include "books/coupled/_44c5_long.vqh"

#include "books/coupled/_44c5_s0_p1_0.vqh"
#include "books/coupled/_44c5_s0_p2_0.vqh"
#include "books/coupled/_44c5_s0_p3_0.vqh"
#include "books/coupled/_44c5_s0_p4_0.vqh"
#include "books/coupled/_44c5_s0_p5_0.vqh"
#include "books/coupled/_44c5_s0_p6_0.vqh"
#include "books/coupled/_44c5_s0_p7_0.vqh"
#include "books/coupled/_44c5_s0_p7_1.vqh"
#include "books/coupled/_44c5_s0_p8_0.vqh"
#include "books/coupled/_44c5_s0_p8_1.vqh"
#include "books/coupled/_44c5_s0_p9_0.vqh"
#include "books/coupled/_44c5_s0_p9_1.vqh"
#include "books/coupled/_44c5_s0_p9_2.vqh"

#include "books/coupled/_44c6_short.vqh"
#include "books/coupled/_44c6_long.vqh"

#include "books/coupled/_44c6_s0_p1_0.vqh"
#include "books/coupled/_44c6_s0_p2_0.vqh"
#include "books/coupled/_44c6_s0_p3_0.vqh"
#include "books/coupled/_44c6_s0_p4_0.vqh"
#include "books/coupled/_44c6_s0_p5_0.vqh"
#include "books/coupled/_44c6_s0_p6_0.vqh"
#include "books/coupled/_44c6_s0_p7_0.vqh"
#include "books/coupled/_44c6_s0_p7_1.vqh"
#include "books/coupled/_44c6_s0_p8_0.vqh"
#include "books/coupled/_44c6_s0_p8_1.vqh"
#include "books/coupled/_44c6_s0_p9_0.vqh"
#include "books/coupled/_44c6_s0_p9_1.vqh"
#include "books/coupled/_44c6_s0_p9_2.vqh"

#include "books/coupled/_44c7_short.vqh"
#include "books/coupled/_44c7_long.vqh"

#include "books/coupled/_44c7_s0_p1_0.vqh"
#include "books/coupled/_44c7_s0_p1_1.vqh"
#include "books/coupled/_44c7_s0_p2_0.vqh"
#include "books/coupled/_44c7_s0_p2_1.vqh"
#include "books/coupled/_44c7_s0_p3_0.vqh"
#include "books/coupled/_44c7_s0_p4_0.vqh"
#include "books/coupled/_44c7_s0_p5_0.vqh"
#include "books/coupled/_44c7_s0_p6_0.vqh"
#include "books/coupled/_44c7_s0_p6_1.vqh"
#include "books/coupled/_44c7_s0_p7_0.vqh"
#include "books/coupled/_44c7_s0_p7_1.vqh"
#include "books/coupled/_44c7_s0_p8_0.vqh"
#include "books/coupled/_44c7_s0_p8_1.vqh"
#include "books/coupled/_44c7_s0_p9_0.vqh"
#include "books/coupled/_44c7_s0_p9_1.vqh"
#include "books/coupled/_44c7_s0_p9_2.vqh"

#include "books/coupled/_44c8_short.vqh"
#include "books/coupled/_44c8_long.vqh"

#include "books/coupled/_44c8_s0_p1_0.vqh"
#include "books/coupled/_44c8_s0_p1_1.vqh"
#include "books/coupled/_44c8_s0_p2_0.vqh"
#include "books/coupled/_44c8_s0_p2_1.vqh"
#include "books/coupled/_44c8_s0_p3_0.vqh"
#include "books/coupled/_44c8_s0_p4_0.vqh"
#include "books/coupled/_44c8_s0_p5_0.vqh"
#include "books/coupled/_44c8_s0_p6_0.vqh"
#include "books/coupled/_44c8_s0_p6_1.vqh"
#include "books/coupled/_44c8_s0_p7_0.vqh"
#include "books/coupled/_44c8_s0_p7_1.vqh"
#include "books/coupled/_44c8_s0_p8_0.vqh"
#include "books/coupled/_44c8_s0_p8_1.vqh"
#include "books/coupled/_44c8_s0_p9_0.vqh"
#include "books/coupled/_44c8_s0_p9_1.vqh"
#include "books/coupled/_44c8_s0_p9_2.vqh"

#include "books/coupled/_44c9_short.vqh"
#include "books/coupled/_44c9_long.vqh"

#include "books/coupled/_44c9_s0_p1_0.vqh"
#include "books/coupled/_44c9_s0_p1_1.vqh"
#include "books/coupled/_44c9_s0_p2_0.vqh"
#include "books/coupled/_44c9_s0_p2_1.vqh"
#include "books/coupled/_44c9_s0_p3_0.vqh"
#include "books/coupled/_44c9_s0_p4_0.vqh"
#include "books/coupled/_44c9_s0_p5_0.vqh"
#include "books/coupled/_44c9_s0_p6_0.vqh"
#include "books/coupled/_44c9_s0_p6_1.vqh"
#include "books/coupled/_44c9_s0_p7_0.vqh"
#include "books/coupled/_44c9_s0_p7_1.vqh"
#include "books/coupled/_44c9_s0_p8_0.vqh"
#include "books/coupled/_44c9_s0_p8_1.vqh"
#include "books/coupled/_44c9_s0_p9_0.vqh"
#include "books/coupled/_44c9_s0_p9_1.vqh"
#include "books/coupled/_44c9_s0_p9_2.vqh"

/* residue backfill is entered in the template array as if stereo
   backfill is not in use.  It's up to vorbisenc to make the
   appropriate index adjustment */
static vorbis_residue_template _residue_template_44_stereo[10]={
  /* mode 0; 64-ish */
  {{&_residue_44_low, &_residue_44_low},  
   {&_huff_book__44c0_short,&_huff_book__44c0_long},
   {&_huff_book__44c0_short,&_huff_book__44c0_long},
   { 
     {0},{0,0,&_44c0_s0_p1_0},{0,0,&_44c0_s0_p2_0},{0,0,&_44c0_s0_p3_0},
     {&_44c0_s0_p4_0,&_44c0_s0_p4_1},{0,0,&_44c0_s0_p5_0},{0,0,&_44c0_s0_p6_0},
     {&_44c0_s0_p7_0,&_44c0_s0_p7_1,&_44c0_s0_p7_2}
   },
  },
  /* mode 1; 80-ish */
  {{&_residue_44_low, &_residue_44_low},  
   {&_huff_book__44c1_short,&_huff_book__44c1_long},
   {&_huff_book__44c1_short,&_huff_book__44c1_long},
   { 
     {0},{0,0,&_44c1_s0_p1_0},{0,0,&_44c1_s0_p2_0},{0,0,&_44c1_s0_p3_0},
     {&_44c1_s0_p4_0,&_44c1_s0_p4_1},{0,0,&_44c1_s0_p5_0},{0,0,&_44c1_s0_p6_0},
     {&_44c1_s0_p7_0,&_44c1_s0_p7_1,&_44c1_s0_p7_2}
   },
  },
  /* mode 2; 96-ish */
  {{&_residue_44_low, &_residue_44_low},  
   {&_huff_book__44c2_short,&_huff_book__44c2_long},
   {&_huff_book__44c2_short,&_huff_book__44c2_long},
   { 
     {0},{0,0,&_44c2_s0_p1_0},{0,0,&_44c2_s0_p2_0},{0,0,&_44c2_s0_p3_0},
     {&_44c2_s0_p4_0,&_44c2_s0_p4_1},{0,0,&_44c2_s0_p5_0},{0,0,&_44c2_s0_p6_0},
     {&_44c2_s0_p7_0,&_44c2_s0_p7_1,&_44c2_s0_p7_2}
   }, 
  },
  /* mode 3; 112-ish */
  {{&_residue_44_low, &_residue_44_low},  
   {&_huff_book__44c3_short,&_huff_book__44c3_long},
   {&_huff_book__44c3_short,&_huff_book__44c3_long},
   { 
     {0},{0,0,&_44c3_s0_p1_0},{0,0,&_44c3_s0_p2_0},{0,0,&_44c3_s0_p3_0},
     {&_44c3_s0_p4_0,&_44c3_s0_p4_1},{0,0,&_44c3_s0_p5_0},{0,0,&_44c3_s0_p6_0},
     {&_44c3_s0_p7_0,&_44c3_s0_p7_1,&_44c3_s0_p7_2}
   },
  },

  /* mode 4; 128-ish */
  {{&_residue_44_mid, &_residue_44_mid},  
   {&_huff_book__44c4_s_short,&_huff_book__44c4_s_long},
   {&_huff_book__44c4_sm_short,&_huff_book__44c4_sm_long},
   {
     {0},{0,0,&_44c4_s_p1_0},{0,0,&_44c4_s_p2_0},{0,0,&_44c4_s_p3_0},
     {0,0,&_44c4_s_p4_0},{0,0,&_44c4_s_p5_0},{0,0,&_44c4_s_p6_0},
     {&_44c4_s_p7_0,&_44c4_s_p7_1},{&_44c4_s_p8_0,&_44c4_s_p8_1},
     {&_44c4_s_p9_0,&_44c4_s_p9_1,&_44c4_s_p9_2}
   },
  },

  /* mode 5; 160-ish */
  {{&_residue_44_mid, &_residue_44_mid},  
   {&_huff_book__44c5_short,&_huff_book__44c5_long},
   {&_huff_book__44c5_short,&_huff_book__44c5_long},
   {
     {0},{0,0,&_44c5_s0_p1_0},{0,0,&_44c5_s0_p2_0},{0,0,&_44c5_s0_p3_0},
     {0,0,&_44c5_s0_p4_0},{0,0,&_44c5_s0_p5_0},{0,0,&_44c5_s0_p6_0},
     {&_44c5_s0_p7_0,&_44c5_s0_p7_1},{&_44c5_s0_p8_0,&_44c5_s0_p8_1},
     {&_44c5_s0_p9_0,&_44c5_s0_p9_1,&_44c5_s0_p9_2}
   },
  },

  /* mode 6; 192-ish */
  {{&_residue_44_mid, &_residue_44_mid},  
   {&_huff_book__44c6_short,&_huff_book__44c6_long},
   {&_huff_book__44c6_short,&_huff_book__44c6_long},
   { 
     {0},{0,0,&_44c6_s0_p1_0},{0,0,&_44c6_s0_p2_0},{0,0,&_44c6_s0_p3_0},
     {0,0,&_44c6_s0_p4_0},{0,0,&_44c6_s0_p5_0},{0,0,&_44c6_s0_p6_0},
     {&_44c6_s0_p7_0,&_44c6_s0_p7_1},{&_44c6_s0_p8_0,&_44c6_s0_p8_1},
     {&_44c6_s0_p9_0,&_44c6_s0_p9_1,&_44c6_s0_p9_2}
   },
  },

  /* mode 7; 224-ish */
  {{&_residue_44_high, &_residue_44_high},  
   {&_huff_book__44c7_short,&_huff_book__44c7_long},
   {&_huff_book__44c7_short,&_huff_book__44c7_long},
   { 
     {0},{&_44c7_s0_p1_0,&_44c7_s0_p1_1},
     {&_44c7_s0_p2_0,&_44c7_s0_p2_1},
     {0,0,&_44c7_s0_p3_0},{0,0,&_44c7_s0_p4_0},{0,0,&_44c7_s0_p5_0},
     {&_44c7_s0_p6_0,&_44c7_s0_p6_1},
     {&_44c7_s0_p7_0,&_44c7_s0_p7_1},
     {&_44c7_s0_p8_0,&_44c7_s0_p8_1},
     {&_44c7_s0_p9_0,&_44c7_s0_p9_1,&_44c7_s0_p9_2}
   }, 
  },

  /* mode 8; 256-ish */
  {{&_residue_44_high, &_residue_44_high},  
   {&_huff_book__44c8_short,&_huff_book__44c8_long},
   {&_huff_book__44c8_short,&_huff_book__44c8_long},
   { 
     {0},{&_44c8_s0_p1_0,&_44c8_s0_p1_1},
     {&_44c8_s0_p2_0,&_44c8_s0_p2_1},
     {0,0,&_44c8_s0_p3_0},{0,0,&_44c8_s0_p4_0},{0,0,&_44c8_s0_p5_0},
     {&_44c8_s0_p6_0,&_44c8_s0_p6_1},
     {&_44c8_s0_p7_0,&_44c8_s0_p7_1},
     {&_44c8_s0_p8_0,&_44c8_s0_p8_1},
     {&_44c8_s0_p9_0,&_44c8_s0_p9_1,&_44c8_s0_p9_2}
   }, 
  },
  /* mode 9; 320-ish */
  {{&_residue_44_high, &_residue_44_high},  
   {&_huff_book__44c9_short,&_huff_book__44c9_long},
   {&_huff_book__44c9_short,&_huff_book__44c9_long},
   { 
     {0},{&_44c9_s0_p1_0,&_44c9_s0_p1_1},
     {&_44c9_s0_p2_0,&_44c9_s0_p2_1},
     {0,0,&_44c9_s0_p3_0},{0,0,&_44c9_s0_p4_0},{0,0,&_44c9_s0_p5_0},
     {&_44c9_s0_p6_0,&_44c9_s0_p6_1},
     {&_44c9_s0_p7_0,&_44c9_s0_p7_1},
     {&_44c9_s0_p8_0,&_44c9_s0_p8_1},
     {&_44c9_s0_p9_0,&_44c9_s0_p9_1,&_44c9_s0_p9_2}
   }, 
  }
};


#include "books/uncoupled/_44u0_p1_0.vqh"
#include "books/uncoupled/_44u0_p2_0.vqh"
#include "books/uncoupled/_44u0_p3_0.vqh"
#include "books/uncoupled/_44u0_p4_0.vqh"
#include "books/uncoupled/_44u0_p4_1.vqh"
#include "books/uncoupled/_44u0_p5_0.vqh"
#include "books/uncoupled/_44u0_p6_0.vqh"
#include "books/uncoupled/_44u0_p7_0.vqh"
#include "books/uncoupled/_44u0_p7_1.vqh"
#include "books/uncoupled/_44u0_p7_2.vqh"

#include "books/uncoupled/_44u4_p1_0.vqh"
#include "books/uncoupled/_44u4_p2_0.vqh"
#include "books/uncoupled/_44u4_p3_0.vqh"
#include "books/uncoupled/_44u4_p4_0.vqh"
#include "books/uncoupled/_44u4_p5_0.vqh"
#include "books/uncoupled/_44u4_p6_0.vqh"
#include "books/uncoupled/_44u4_p7_0.vqh"
#include "books/uncoupled/_44u4_p7_1.vqh"
#include "books/uncoupled/_44u4_p8_0.vqh"
#include "books/uncoupled/_44u4_p8_1.vqh"
#include "books/uncoupled/_44u4_p9_0.vqh"
#include "books/uncoupled/_44u4_p9_1.vqh"
#include "books/uncoupled/_44u4_p9_2.vqh"

#include "books/uncoupled/_44u7_p1_0.vqh"
#include "books/uncoupled/_44u7_p2_0.vqh"
#include "books/uncoupled/_44u7_p2_1.vqh"
#include "books/uncoupled/_44u7_p3_0.vqh"
#include "books/uncoupled/_44u7_p4_0.vqh"
#include "books/uncoupled/_44u7_p5_0.vqh"
#include "books/uncoupled/_44u7_p6_0.vqh"
#include "books/uncoupled/_44u7_p7_0.vqh"
#include "books/uncoupled/_44u7_p7_1.vqh"
#include "books/uncoupled/_44u7_p8_0.vqh"
#include "books/uncoupled/_44u7_p8_1.vqh"
#include "books/uncoupled/_44u7_p9_0.vqh"
#include "books/uncoupled/_44u7_p9_1.vqh"
#include "books/uncoupled/_44u7_p9_2.vqh"

static vorbis_residue_template _residue_template_44_uncoupled[10]={
  /* mode 0; 40/c-ish */
  {{&_residue_44_low_un, &_residue_44_low_un},  
   {&_huff_book__44c0_short,&_huff_book__44c0_long},
   {&_huff_book__44c0_short,&_huff_book__44c0_long},
   { {0},
      {0,0,&_44u0_p1_0},
      {0,0,&_44u0_p2_0},
      {0,0,&_44u0_p3_0},
      {&_44u0_p4_0,&_44u0_p4_1},
      {0,0,&_44u0_p5_0},
      {0,0,&_44u0_p6_0},
      {&_44u0_p7_0,&_44u0_p7_1,&_44u0_p7_2},
   },
  },
  /* mode 1; 50-ish */
  {{&_residue_44_low_un, &_residue_44_low_un},  
   {&_huff_book__44c1_short,&_huff_book__44c1_long},
   {&_huff_book__44c1_short,&_huff_book__44c1_long},
   { {0},
      {0,0,&_44u0_p1_0},
      {0,0,&_44u0_p2_0},
      {0,0,&_44u0_p3_0},
      {&_44u0_p4_0,&_44u0_p4_1},
      {0,0,&_44u0_p5_0},
      {0,0,&_44u0_p6_0},
      {&_44u0_p7_0,&_44u0_p7_1,&_44u0_p7_2},
   },
  },
  /* mode 2; 60-ish */
  {{&_residue_44_low_un, &_residue_44_low_un},  
   {&_huff_book__44c2_short,&_huff_book__44c2_long},
   {&_huff_book__44c2_short,&_huff_book__44c2_long},
   { {0},
      {0,0,&_44u0_p1_0},
      {0,0,&_44u0_p2_0},
      {0,0,&_44u0_p3_0},
      {&_44u0_p4_0,&_44u0_p4_1},
      {0,0,&_44u0_p5_0},
      {0,0,&_44u0_p6_0},
      {&_44u0_p7_0,&_44u0_p7_1,&_44u0_p7_2},
   },
  },
  /* mode 3; 70-ish */
  {{&_residue_44_low_un, &_residue_44_low_un},  
   {&_huff_book__44c3_short,&_huff_book__44c3_long},
   {&_huff_book__44c3_short,&_huff_book__44c3_long},
   { {0},
      {0,0,&_44u0_p1_0},
      {0,0,&_44u0_p2_0},
      {0,0,&_44u0_p3_0},
      {&_44u0_p4_0,&_44u0_p4_1},
      {0,0,&_44u0_p5_0},
      {0,0,&_44u0_p6_0},
      {&_44u0_p7_0,&_44u0_p7_1,&_44u0_p7_2},
   },
  },
  /* mode 4; 80-ish */
  {{&_residue_44_mid, &_residue_44_mid},  
   {&_huff_book__44c4_s_short,&_huff_book__44c4_s_long},
   {&_huff_book__44c4_s_short,&_huff_book__44c4_s_long},
   { {0},
      {0,0,&_44u4_p1_0},
      {0,0,&_44u4_p2_0},
      {0,0,&_44u4_p3_0},
      {0,0,&_44u4_p4_0},
      {0,0,&_44u4_p5_0},
      {0,0,&_44u4_p6_0},
      {&_44u4_p7_0,&_44u4_p7_1},
      {&_44u4_p8_0,&_44u4_p8_1},
      {&_44u4_p9_0,&_44u4_p9_1,&_44u4_p9_2},
   },
  },
  /* mode 5; 90-ish */
  {{&_residue_44_mid, &_residue_44_mid},  
   {&_huff_book__44c5_short,&_huff_book__44c5_long},
   {&_huff_book__44c5_short,&_huff_book__44c5_long},
   { {0},
      {0,0,&_44u4_p1_0},
      {0,0,&_44u4_p2_0},
      {0,0,&_44u4_p3_0},
      {0,0,&_44u4_p4_0},
      {0,0,&_44u4_p5_0},
      {0,0,&_44u4_p6_0},
      {&_44u4_p7_0,&_44u4_p7_1},
      {&_44u4_p8_0,&_44u4_p8_1},
      {&_44u4_p9_0,&_44u4_p9_1,&_44u4_p9_2},
   },
  },
  /* mode 6; 100-ish */
  {{&_residue_44_mid, &_residue_44_mid},  
   {&_huff_book__44c6_short,&_huff_book__44c6_long},
   {&_huff_book__44c6_short,&_huff_book__44c6_long},
   { {0},
      {0,0,&_44u4_p1_0},
      {0,0,&_44u4_p2_0},
      {0,0,&_44u4_p3_0},
      {0,0,&_44u4_p4_0},
      {0,0,&_44u4_p5_0},
      {0,0,&_44u4_p6_0},
      {&_44u4_p7_0,&_44u4_p7_1},
      {&_44u4_p8_0,&_44u4_p8_1},
      {&_44u4_p9_0,&_44u4_p9_1,&_44u4_p9_2},
   },
  },
  /* mode 7 */
  {{&_residue_44_high_un, &_residue_44_high_un},  
   {&_huff_book__44c7_short,&_huff_book__44c7_long},
   {&_huff_book__44c7_short,&_huff_book__44c7_long},
   { {0},
      {0,0,&_44u7_p1_0},
      {&_44u7_p2_0,&_44u7_p2_1},
      {0,0,&_44u7_p3_0},
      {0,0,&_44u7_p4_0},
      {0,0,&_44u7_p5_0},
      {0,0,&_44u7_p6_0},
      {&_44u7_p7_0,&_44u7_p7_1},
      {&_44u7_p8_0,&_44u7_p8_1},
      {&_44u7_p9_0,&_44u7_p9_1,&_44u7_p9_2},
   },
  },
  /* mode 8 */
  {{&_residue_44_high_un, &_residue_44_high_un},  
   {&_huff_book__44c8_short,&_huff_book__44c8_long},
   {&_huff_book__44c8_short,&_huff_book__44c8_long},
   { {0},
      {0,0,&_44u7_p1_0},
      {&_44u7_p2_0,&_44u7_p2_1},
      {0,0,&_44u7_p3_0},
      {0,0,&_44u7_p4_0},
      {0,0,&_44u7_p5_0},
      {0,0,&_44u7_p6_0},
      {&_44u7_p7_0,&_44u7_p7_1},
      {&_44u7_p8_0,&_44u7_p8_1},
      {&_44u7_p9_0,&_44u7_p9_1,&_44u7_p9_2},
   },
  },
  /* mode 9 */
  {{&_residue_44_high_un, &_residue_44_high_un},  
   {&_huff_book__44c9_short,&_huff_book__44c9_long},
   {&_huff_book__44c9_short,&_huff_book__44c9_long},
   { {0},
      {0,0,&_44u7_p1_0},
      {&_44u7_p2_0,&_44u7_p2_1},
      {0,0,&_44u7_p3_0},
      {0,0,&_44u7_p4_0},
      {0,0,&_44u7_p5_0},
      {0,0,&_44u7_p6_0},
      {&_44u7_p7_0,&_44u7_p7_1},
      {&_44u7_p8_0,&_44u7_p8_1},
      {&_44u7_p9_0,&_44u7_p9_1,&_44u7_p9_2},
   },
  },
};





