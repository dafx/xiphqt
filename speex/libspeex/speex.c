/* Copyright (C) 2002 Jean-Marc Valin 
   File: speex.c
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

#ifndef M_PI
#define M_PI           3.14159265358979323846  /* pi */
#endif

#define sqr(x) ((x)*(x))

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
   st->gamma=.9;

   st->inBuf = malloc(st->bufSize*sizeof(float));
   st->frame = st->inBuf + st->bufSize - st->windowSize;
   st->wBuf = malloc(st->bufSize*sizeof(float));
   st->wframe = st->wBuf + st->bufSize - st->windowSize;
   for (i=0;i<st->bufSize;i++)
      st->inBuf[i]=0;
   for (i=0;i<st->bufSize;i++)
      st->wBuf[i]=0;

   st->window = malloc(st->windowSize*sizeof(float));
   /* Hanning window */
   for (i=0;i<st->windowSize;i++)
      st->window[i]=.5*(1-cos(2*M_PI*i/st->windowSize));

   st->buf2 = malloc(st->windowSize*sizeof(float));
   st->lpc = malloc((st->lpcSize+1)*sizeof(float));
   st->interp_lpc = malloc((st->lpcSize+1)*sizeof(float));
   st->bw_lpc = malloc((st->lpcSize+1)*sizeof(float));
   st->autocorr = malloc((st->lpcSize+1)*sizeof(float));

   /* Create the window for autocorrelation (lag-windowing) */
   st->lagWindow = malloc((st->lpcSize+1)*sizeof(float));
   for (i=0;i<st->lpcSize+1;i++)
      st->lagWindow[i]=exp(-.5*sqr(2*M_PI*.01*i));
   st->lsp = malloc(st->lpcSize*sizeof(float));
   st->old_lsp = malloc(st->lpcSize*sizeof(float));
   st->interp_lsp = malloc(st->lpcSize*sizeof(float));
   st->rc = malloc(st->lpcSize*sizeof(float));
   st->first = 1;
}

void encoder_destroy(EncState *st)
{
   /* Free all allocated memory */
   free(st->inBuf);
   free(st->wBuf);
   free(st->window);
   free(st->buf2);
   free(st->lpc);
   free(st->interp_lpc);
   free(st->bw_lpc);
   free(st->autocorr);
   free(st->lagWindow);
   free(st->lsp);
   free(st->old_lsp);
   free(st->interp_lsp);
   free(st->rc);
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
   /*printf ("prediction error = %f, R[0] = %f, gain = %f\n", error, st->autocorr[0], st->autocorr[0]/error);*/

   /*for (i=0;i<st->lpcSize+1;i++)
      printf("%f ", st->lpc[i]);
      printf ("aa\n");*/

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

   /*for (i=0;i<roots;i++)
      printf("%f ", st->lsp[i]);
      printf ("\n");*/
   
   /* LSP Quantization */
   {
      unsigned int id;
      id=lsp_quant_nb(st->lsp,10 );
      lsp_unquant_nb(st->lsp,10,id);
   }

   /* Loop on all sub-frames */
   for (sub=0;sub<st->nbSubframes;sub++)
   {
      float tmp, tmp1,tmp2,gain[3];
      int pitch, offset;

      /* Offset relative to start of frame */
      offset = st->subframeSize*sub;

      /* LSP interpolation */
      tmp = (.5 + sub)/st->nbSubframes;
      for (i=0;i<st->lpcSize;i++)
         st->interp_lsp[i] = (1-tmp)*st->old_lsp[i] + tmp*st->lsp[i];

      /* Compute interpolated LPCs */
      for (i=0;i<st->lpcSize;i++)
         st->interp_lsp[i] = cos(st->interp_lsp[i]);
      lsp_to_lpc(st->interp_lsp, st->interp_lpc, st->lpcSize);

      /*for (i=0;i<st->lpcSize+1;i++)
         printf("%f ", st->interp_lpc[i]);
      printf ("\n");
      */

      /* Compute bandwidth-expanded LPCs for perceptual weighting*/
      tmp=1;
      for (i=0;i<st->lpcSize+1;i++)
      {
         st->bw_lpc[i] = tmp * st->interp_lpc[i];
         tmp *= st->gamma;
      }
      /*for (i=0;i<st->lpcSize+1;i++)
         printf("%f ", st->bw_lpc[i]);
         printf ("\n");*/

      /* Compute perceptualy weighted residue (FIR) */      
      for (i=0;i<st->subframeSize;i++)
      {
         st->wframe[offset+i]=st->frame[offset+i];
         for (j=1;j<st->lpcSize+1;j++)
           st->wframe[offset+i] += st->frame[offset+i-j]*st->bw_lpc[j];
      }

      /* Find pitch gain and delay, gains are already quantized*/
      pitch = ltp_closed_loop(st->wframe+offset, st->subframeSize, 20, 120, gain);
      /*pitch = three_tap_ltp(st->wframe+offset, st->subframeSize, 20, 120, gain);*/

      printf ("pitch = %d, gains = %f %f %f\n",pitch,gain[0], gain[1], gain[2]);
      /*printf ("%f %f %f ",gain[0], gain[1], gain[2]);*/
      
      tmp1=0;
      for (i=0;i<st->subframeSize;i++)
         tmp1+=st->wframe[offset+i]*st->wframe[offset+i];
      predictor_three_tap(st->wframe+offset, st->subframeSize, pitch, gain);
      
      tmp2=0;
      for (i=0;i<st->subframeSize;i++)
         tmp2+=st->wframe[offset+i]*st->wframe[offset+i];
      printf ("pitch prediction gain: %f\n", tmp1/(tmp2+.001));
      
      /*Analysis by synthesis and excitation quantization here*/

      /* Reverse the 3-tab pitch predictor (IIR)*/
      inverse_three_tap(st->wframe+offset, st->subframeSize, pitch, gain);

      /*Inverse short-term predictor (1/W(z/gamma))*/
      for (i=0;i<st->subframeSize;i++)
      {
         st->frame[offset+i]=st->wframe[offset+i];
         for (j=1;j<st->lpcSize+1;j++)
            st->frame[offset+i] -= st->frame[offset+i-j]*st->bw_lpc[j];
      }

   }

   printf ("\n");
   /* Store the LSPs for interpolation in the next frame */
   for (i=0;i<st->lpcSize;i++)
      st->old_lsp[i] = st->lsp[i];
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
