/* Copyright (C) 2002 Jean-Marc Valin 
   File: speex.h

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.
   
   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#ifndef SB_CELP_H
#define SB_CELP_H

#include "modes.h"
#include "bits.h"
#include "speex.h"

/**Structure representing the full state of the encoder*/
typedef struct SBEncState {
   EncState st_low;
   int    full_frame_size;
   int    frame_size;
   int    subframeSize;
   int    nbSubframes;
   int    windowSize;
   int    lpcSize;
   int    first;
   float  lag_factor;
   float  lpc_floor;
   float  gamma1;
   float  gamma2;

   float *stack;
   float *x0, *x0d, *x1, *x1d;
   float *high;
   float *y0, *y1;
   float *h0_mem, *h1_mem, *g0_mem, *g1_mem;

   float *excBuf;
   float *exc;
   float *buf;
   float *res;
   float *sw;
   float *target;
   float *window;
   float *lagWindow;
   float *autocorr;
   float *rc;
   float *lpc;
   float *lsp;
   float *qlsp;
   float *old_lsp;
   float *old_qlsp;
   float *interp_lsp;
   float *interp_qlsp;
   float *interp_lpc;
   float *interp_qlpc;
   float *bw_lpc1;
   float *bw_lpc2;

   float *mem_sp;
   float *mem_sw;
} SBEncState;


/**Initializes encoder state*/
void sb_encoder_init(SBEncState *st, SpeexMode *mode);

/**De-allocates encoder state resources*/
void sb_encoder_destroy(SBEncState *st);

/**Encodes one frame*/
void sb_encode(SBEncState *st, float *in, FrameBits *bits);


#endif
