/* Copyright (C) Jean-Marc Valin

   File: speex_echo.c


   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:

   1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
   IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
   OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
   INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
   STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
   ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
*/

#include "misc.h"
#include "speex_echo.h"
#include "smallft.h"
#include <math.h>

/** Creates a new echo canceller state */
SpeexEchoState *speex_echo_state_init(int frame_size, int filter_length)
{
   int N,M;
   SpeexEchoState *st = (SpeexEchoState *)speex_alloc(sizeof(SpeexEchoState));

   st->frame_size = frame_size;
   st->window_size = 2*frame_size;
   N = st->window_size;
   M = st->M = (filter_length+N-1)/frame_size;

   drft_init(&st->fft_lookup, N);
   
   st->x = (float*)speex_alloc(N*sizeof(float));
   st->y = (float*)speex_alloc(N*sizeof(float));

   st->X = (float*)speex_alloc(M*N*sizeof(float));
   st->Y = (float*)speex_alloc(N*sizeof(float));
   st->E = (float*)speex_alloc(N*sizeof(float));
   st->W = (float*)speex_alloc(M*N*sizeof(float));
   st->PHI = (float*)speex_alloc(N*sizeof(float));

   return st;
}

/** Destroys an echo canceller state */
void speex_echo_state_destroy(SpeexEchoState *st)
{
}

/** Performs echo cancellation a frame */
void speex_echo_cancel(SpeexEchoState *st, float *ref, float *echo, float *out, float *Yout)
{
   int i,j;
   int N,M;
   float scale;

   N = st->window_size;
   M = st->M;
   scale = 1.0/N;

   for (i=0;i<st->frame_size;i++)
   {
      st->x[i] = st->x[i+st->frame_size];
      st->x[i+st->frame_size] = echo[i];
   }

   /* Shift memory: this could be optimized eventually*/
   for (i=0;i<N*(M-1);i++)
      st->X[i]=st->X[i+N];

   for (i=0;i<N;i++)
      st->X[(M-1)*N+i]=st->x[i];

   drft_forward(&st->fft_lookup, &st->X[(M-1)*N]);

   for (i=1;i<N-1;i+=2)
   {
      st->Y[i] = st->Y[i+1] = 0;
      for (j=0;j<M;j++)
      {
         st->Y[i] += st->X[j*N+i]*st->W[j*N+i] - st->X[j*N+i+1]*st->W[j*N+i+1];
         st->Y[i+1] += st->X[j*N+i+1]*st->W[j*N+i] + st->X[j*N+i]*st->W[j*N+i+1];
      }
   }
   st->Y[0] = st->Y[N-1] = 0;

   if (Yout)
      for (i=0;i<N;i++)
         Yout[i] = st->Y[i];

   for (i=0;i<N;i++)
      st->y[i] = st->Y[i];
   
#if 0
   for (i=0;i<N;i++)
      st->y[i] = st->X[(M-1)*N+i];
#endif

   drft_backward(&st->fft_lookup, st->y);
   for (i=0;i<N;i++)
      st->y[i] *= scale;

   for (i=0;i<st->frame_size;i++)
   {
      out[i] = ref[i] - st->y[i+st->frame_size];
      st->E[i] = 0;
      st->E[i+st->frame_size] = out[i];
   }

   drft_forward(&st->fft_lookup, st->E);

   for (j=0;j<M;j++)
   {
      for (i=1;i<N-1;i+=2)
      {
         st->PHI[i] = st->X[j*N+i]*st->E[i] + st->X[j*N+i+1]*st->E[i+1];
         st->PHI[i+1] = -st->X[j*N+i+1]*st->E[i] + st->X[j*N+i]*st->E[i+1];
      }
      st->PHI[0] = st->PHI[N-1] = 0;

      /* Optionally perform some transform (normalization?) on PHI */
      
      for (i=0;i<N;i++)
         st->W[j*N+i] += .002*st->PHI[i];
   }
}

