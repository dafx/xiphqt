/* Copyright (C) 2002 Jean-Marc Valin 
   File: speex.c

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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "speex.h"
#include "lpc.h"
#include "lsp.h"
#include "ltp.h"
#include "quant_lsp.h"
#include "cb_search.h"
#include "filters.h"

extern float stoc[];

#ifndef M_PI
#define M_PI           3.14159265358979323846  /* pi */
#endif

#define sqr(x) ((x)*(x))
#define min(a,b) ((a) < (b) ? (a) : (b))

void encoder_init(EncState *st)
{
   int i;
   /* Codec parameters, should eventually have several "modes"*/
   st->frameSize = 160;
   st->windowSize = 320;
   st->nbSubframes=4;
   st->subframeSize=40;
   st->lpcSize = 10;
   st->bufSize = 640;
   st->gamma1=.9;
   st->gamma2=.5;

   st->inBuf = malloc(st->bufSize*sizeof(float));
   st->frame = st->inBuf + st->bufSize - st->windowSize;
   st->wBuf = malloc(st->bufSize*sizeof(float));
   st->wframe = st->wBuf + st->bufSize - st->windowSize;
   st->excBuf = malloc(st->bufSize*sizeof(float));
   st->exc = st->excBuf + st->bufSize - st->windowSize;
   st->resBuf = malloc(st->bufSize*sizeof(float));
   st->res = st->resBuf + st->bufSize - st->windowSize;
   st->tBuf = malloc(st->bufSize*sizeof(float));
   st->tframe = st->tBuf + st->bufSize - st->windowSize;
   for (i=0;i<st->bufSize;i++)
      st->inBuf[i]=0;
   for (i=0;i<st->bufSize;i++)
      st->wBuf[i]=0;
   for (i=0;i<st->bufSize;i++)
      st->resBuf[i]=0;
   for (i=0;i<st->bufSize;i++)
      st->excBuf[i]=0;
   for (i=0;i<st->bufSize;i++)
      st->tBuf[i]=0;

   /* Hanning window */
   st->window = malloc(st->windowSize*sizeof(float));
   for (i=0;i<st->windowSize;i++)
      st->window[i]=.5*(1-cos(2*M_PI*i/st->windowSize));

   /* Create the window for autocorrelation (lag-windowing) */
   st->lagWindow = malloc((st->lpcSize+1)*sizeof(float));
   for (i=0;i<st->lpcSize+1;i++)
      st->lagWindow[i]=exp(-.5*sqr(2*M_PI*.01*i));

   st->autocorr = malloc((st->lpcSize+1)*sizeof(float));

   st->buf2 = malloc(st->windowSize*sizeof(float));

   st->lpc = malloc((st->lpcSize+1)*sizeof(float));
   st->interp_lpc = malloc((st->lpcSize+1)*sizeof(float));
   st->interp_qlpc = malloc((st->lpcSize+1)*sizeof(float));
   st->bw_lpc1 = malloc((st->lpcSize+1)*sizeof(float));
   st->bw_lpc2 = malloc((st->lpcSize+1)*sizeof(float));
   st->bw_az = malloc((st->lpcSize*2+1)*sizeof(float));

   st->lsp = malloc(st->lpcSize*sizeof(float));
   st->qlsp = malloc(st->lpcSize*sizeof(float));
   st->old_lsp = malloc(st->lpcSize*sizeof(float));
   st->old_qlsp = malloc(st->lpcSize*sizeof(float));
   st->interp_lsp = malloc(st->lpcSize*sizeof(float));
   st->interp_qlsp = malloc(st->lpcSize*sizeof(float));
   st->rc = malloc(st->lpcSize*sizeof(float));
   st->first = 1;
   
   st->mem1 = calloc(st->lpcSize, sizeof(float));
   st->mem2 = calloc(st->lpcSize, sizeof(float));
   st->mem3 = calloc(st->lpcSize, sizeof(float));
   st->mem4 = calloc(st->lpcSize, sizeof(float));
   st->mem5 = calloc(st->lpcSize, sizeof(float));
   st->mem6 = calloc(st->lpcSize, sizeof(float));
   st->mem7 = calloc(st->lpcSize, sizeof(float));
}

void encoder_destroy(EncState *st)
{
   /* Free all allocated memory */
   free(st->inBuf);
   free(st->wBuf);
   free(st->resBuf);
   free(st->excBuf);
   free(st->tBuf);

   free(st->window);
   free(st->buf2);
   free(st->lpc);
   free(st->interp_lpc);
   free(st->interp_qlpc);

   free(st->bw_lpc1);
   free(st->bw_lpc2);
   free(st->bw_az);
   free(st->autocorr);
   free(st->lagWindow);
   free(st->lsp);
   free(st->qlsp);
   free(st->old_lsp);
   free(st->interp_lsp);
   free(st->old_qlsp);
   free(st->interp_qlsp);
   free(st->rc);

   free(st->mem1);
   free(st->mem2);
   free(st->mem3);
   free(st->mem4);
   free(st->mem5);
   free(st->mem6);
   free(st->mem7);
}

void encode(EncState *st, float *in, int *outSize, void *bits)
{
   int i, j, sub, roots;
   float error;

   /* Copy new data in input buffer */
   memmove(st->inBuf, st->inBuf+st->frameSize, (st->bufSize-st->frameSize)*sizeof(float));
   for (i=0;i<st->frameSize;i++)
      st->inBuf[st->bufSize-st->frameSize+i] = in[i];
   memmove(st->wBuf, st->wBuf+st->frameSize, (st->bufSize-st->frameSize)*sizeof(float));
   memmove(st->resBuf, st->resBuf+st->frameSize, (st->bufSize-st->frameSize)*sizeof(float));
   memmove(st->excBuf, st->excBuf+st->frameSize, (st->bufSize-st->frameSize)*sizeof(float));
   memmove(st->tBuf, st->tBuf+st->frameSize, (st->bufSize-st->frameSize)*sizeof(float));

   /* Window for analysis */
   for (i=0;i<st->windowSize;i++)
      st->buf2[i] = st->frame[i] * st->window[i];

   /* Compute auto-correlation */
   autocorr(st->buf2, st->autocorr, st->lpcSize+1, st->windowSize);

   st->autocorr[0] += 1;        /* prevents NANs */
   st->autocorr[0] *= 1.0001;   /* 40 dB noise floor */
   /* Lag windowing: equivalent to filtering in the power-spectrum domain */
   for (i=0;i<st->lpcSize+1;i++)
      st->autocorr[i] *= st->lagWindow[i];

   /* Levinson-Durbin */
   error = wld(st->lpc+1, st->autocorr, st->rc, st->lpcSize);
   st->lpc[0]=1;

   /* LPC to LSPs (x-domain) transform */
   roots=lpc_to_lsp (st->lpc, st->lpcSize, st->lsp, 6, 0.02);
   if (roots!=st->lpcSize)
   {
      fprintf (stderr, "roots!=st->lpcSize\n");
      exit(1);
   }

   /* x-domain to angle domain*/
   for (i=0;i<st->lpcSize;i++)
      st->lsp[i] = acos(st->lsp[i]);
   
   /* LSP Quantization */
   {
      unsigned int id;
      for (i=0;i<st->lpcSize;i++)
         st->buf2[i]=st->lsp[i];
      id=lsp_quant_nb(st->buf2,10 );
      lsp_unquant_nb(st->qlsp,10,id);
   }

   /* Loop on sub-frames */
   for (sub=0;sub<st->nbSubframes;sub++)
   {
      float tmp, tmp1,tmp2,gain[3];
      int pitch, offset;
      float *sp, *sw, *res, *exc, *target;
      
      /* Offset relative to start of frame */
      offset = st->subframeSize*sub;
      sp=st->frame+offset;
      sw=st->wframe+offset;
      res=st->res+offset;
      exc=st->exc+offset;
      target=st->tframe+offset;

      /* LSP interpolation (quantized and unquantized) */
      tmp = (.5 + sub)/st->nbSubframes;
      for (i=0;i<st->lpcSize;i++)
         st->interp_lsp[i] = (1-tmp)*st->old_lsp[i] + tmp*st->lsp[i];
      for (i=0;i<st->lpcSize;i++)
         st->interp_qlsp[i] = (1-tmp)*st->old_qlsp[i] + tmp*st->qlsp[i];

      /* Compute interpolated LPCs (quantized and unquantized) */
      for (i=0;i<st->lpcSize;i++)
         st->interp_lsp[i] = cos(st->interp_lsp[i]);
      lsp_to_lpc(st->interp_lsp, st->interp_lpc, st->lpcSize);

      for (i=0;i<st->lpcSize;i++)
         st->interp_qlsp[i] = cos(st->interp_qlsp[i]);
      lsp_to_lpc(st->interp_qlsp, st->interp_qlpc, st->lpcSize);

      /* Compute bandwidth-expanded (unquantized) LPCs for perceptual weighting */
      tmp=1;
      for (i=0;i<st->lpcSize+1;i++)
      {
         st->bw_lpc1[i] = tmp * st->interp_lpc[i];
         tmp *= st->gamma1;
      }
      tmp=1;
      for (i=0;i<st->lpcSize+1;i++)
      {
         st->bw_lpc2[i] = tmp * st->interp_lpc[i];
         tmp *= st->gamma2;
      }
      /* Compute residue */
      /*residue(sp, st->interp_qlpc, res, st->subframeSize, st->lpcSize);*/
      
      /* Reset excitation */
      for (i=0;i<st->subframeSize;i++)
         exc[i]=0;

      /* Compute zero response of A(z/g1) / ( A(z/g2) * Aq(z) ) */
      residue(exc, st->bw_lpc1, exc, st->subframeSize, st->lpcSize);
      syn_filt_mem(exc, st->interp_qlpc, res, st->subframeSize, st->lpcSize, st->mem3);
      syn_filt_mem(exc, st->bw_lpc2, exc, st->subframeSize, st->lpcSize, st->mem4);


      /* Compute weighted signal */
      residue_mem(sp, st->bw_lpc1, sw, st->subframeSize, st->lpcSize, st->mem1);
      for (i=0;i<st->lpcSize;i++)
        st->mem3[i]=sw[st->subframeSize-i-1];
      syn_filt_mem(sw, st->bw_lpc2, sw, st->subframeSize, st->lpcSize, st->mem2);
      for (i=0;i<st->lpcSize;i++)
        st->mem4[i]=sw[st->subframeSize-i-1];


      /* Compute target signal */
      for (i=0;i<st->subframeSize;i++)
         target[i]=sw[i]-res[i];
#if 0
      /* Perform adaptive codebook search (pitch prediction) */
      overlap_cb_search(target, st->interp_qlpc, st->bw_lpc1, st->bw_lpc2,
                        &exc[-120], 100, &gain[0], &pitch, st->lpcSize,
                        st->subframeSize);
      printf ("gain = %f, pitch = %d\n",gain[0], 120-pitch);
      for (i=0;i<st->subframeSize;i++)
        exc[i]=gain[0]*exc[i-120+pitch];
      residue_zero(exc, st->bw_lpc1, res, st->subframeSize, st->lpcSize);
      syn_filt_zero(res, st->interp_qlpc, res, st->subframeSize, st->lpcSize);
      syn_filt_zero(res, st->bw_lpc2, res, st->subframeSize, st->lpcSize);
      for (i=0;i<st->subframeSize;i++)
        target[i]-=res[i];


      overlap_cb_search(target, st->interp_qlpc, st->bw_lpc1, st->bw_lpc2,
                        stoc, 512, &gain[0], &pitch, st->lpcSize,
                        st->subframeSize);
      printf ("gain = %f, index = %d\n",gain[0], pitch);
      for (i=0;i<st->subframeSize;i++)
         exc[i]+=gain[0]*stoc[i+pitch];
      residue_zero(exc, st->bw_lpc1, res, st->subframeSize, st->lpcSize);
      syn_filt_zero(res, st->interp_qlpc, res, st->subframeSize, st->lpcSize);
      syn_filt_zero(res, st->bw_lpc2, res, st->subframeSize, st->lpcSize);
      for (i=0;i<st->subframeSize;i++)
         target[i]-=res[i];
#endif

      /* Update target for adaptive codebook contribution */

      /* Perform stochastic codebook search */

#if 1 /* We're cheating: computing excitation directly from target */
      residue_zero(target, st->interp_qlpc, exc, st->subframeSize, st->lpcSize);
      residue_zero(target, st->bw_lpc2, exc, st->subframeSize, st->lpcSize);
      syn_filt_zero(exc, st->bw_lpc1, exc, st->subframeSize, st->lpcSize);
      
#endif

      /* Final signal synthesis from excitation */
      syn_filt_mem(exc, st->interp_qlpc, sp, st->subframeSize, st->lpcSize, st->mem5);

      /* Compute weighted signal again, from synthesized speech (not sure it the right thing) */
      /*residue_mem(sp, st->bw_lpc1, sw, st->subframeSize, st->lpcSize, st->mem6);
      for (i=0;i<st->lpcSize;i++)
        st->mem3[i]=sw[st->subframeSize-i-1];
      syn_filt_mem(sw, st->bw_lpc2, sw, st->subframeSize, st->lpcSize, st->mem6);
      for (i=0;i<st->lpcSize;i++)
      st->mem4[i]=sw[st->subframeSize-i-1];*/

   }

   /* Store the LSPs for interpolation in the next frame */
   for (i=0;i<st->lpcSize;i++)
      st->old_lsp[i] = st->lsp[i];
   for (i=0;i<st->lpcSize;i++)
      st->old_qlsp[i] = st->qlsp[i];

   /* The next frame will not by the first (Duh!) */
   st->first = 0;

   /* Replace input by synthesized speech */
   for (i=0;i<st->frameSize;i++)
     in[i]=st->frame[i];
}


void decoder_init(DecState *st)
{
}

void decoder_destroy(DecState *st)
{
}

void decode(DecState *st, float *bits, float *out)
{
}
