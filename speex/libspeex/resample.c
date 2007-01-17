/* Copyright (C) 2007 Jean-Marc Valin
      
   File: resample.c
   Resample code

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
#include <math.h>
#include <stdio.h>
            
typedef struct {
   int in_rate;
   int out_rate;
   int num_rate;
   int den_rate;
   int last_sample;
   int samp_frac_num;
   int filt_len;
   float *mem;
} SpeexResamplerState;


SpeexResamplerState *speex_resampler_init(int in_rate, int out_rate, int in_rate_den, int out_rate_den)
{
   SpeexResamplerState *st = (SpeexResamplerState *)speex_alloc(sizeof(SpeexResamplerState));
   int fact, i;
   st->in_rate = in_rate;
   st->out_rate = out_rate;
   st->num_rate = in_rate;
   st->den_rate = out_rate;
   /* FIXME: This is terribly inefficient, but who cares (at least for now)? */
   for (fact=2;fact<=sqrt(MAX32(in_rate, out_rate));fact++)
   {
      while ((st->num_rate % fact == 0) && (st->den_rate % fact == 0))
      {
         st->num_rate /= fact;
         st->den_rate /= fact;
      }
   }
   st->last_sample = 0;
   st->filt_len = 64;
   st->mem = speex_alloc((st->filt_len-1) * sizeof(float));
   for (i=0;i<st->filt_len-1;i++)
      st->mem[i] = 0;
   return st;
}

void speex_resampler_destroy(SpeexResamplerState *st)
{
   speex_free(st->mem);
   speex_free(st);
}

static float sinc(float x, int N)
{
   if (fabs(x)<1e-6)
      return 1;
   else if (fabs(x) > .5f*N)
      return 0;
   return sin(M_PI*x)/(M_PI*x) * (.5-.5*cos(2*x*M_PI/N));
}

int speex_resample_float(SpeexResamplerState *st, const float *in, int len, float *out)
{
   int j=0;
   int N = st->filt_len;
   int out_sample = 0;
   while (1)
   {
      int j;
      float sum=0;
      for (j=0;j<N;j++)
      {
         if (st->last_sample-N+1+j < 0)
            sum += st->mem[st->last_sample+j]*sinc((j-N/2)-((float)st->samp_frac_num)/st->den_rate, N);
         else
            sum += in[st->last_sample-N+1+j]*sinc((j-N/2)-((float)st->samp_frac_num)/st->den_rate, N);
      }
      out[out_sample++] = sum;
      
      st->last_sample += st->num_rate/st->den_rate;
      st->samp_frac_num += st->num_rate%st->den_rate;
      if (st->samp_frac_num >= st->den_rate)
      {
         st->samp_frac_num -= st->den_rate;
         st->last_sample++;
      }
      //fprintf (stderr, "%d %d %d %d\n", st->last_sample, st->samp_frac_num, st->num_rate, st->den_rate);
      if (st->last_sample >= len)
      {
         st->last_sample -= len;
         break;
      }      
   }
   for (j=0;j<st->filt_len-1;j++)
      st->mem[j] = in[j+len-N+1];
   return out_sample;
}

#define NN 256

int main(int argc, char **argv)
{
   int i;
   SpeexResamplerState *st = speex_resampler_init(8000, 16000, 1, 1);
   short *in;
   short *out;
   float *fin, *fout;
   in = speex_alloc(NN*sizeof(short));
   out = speex_alloc(2*NN*sizeof(short));
   fin = speex_alloc(NN*sizeof(float));
   fout = speex_alloc(2*NN*sizeof(float));
   while (1)
   {
      int out_num;
      fread(in, sizeof(short), NN, stdin);
      if (feof(stdin))
         break;
      for (i=0;i<NN;i++)
         fin[i]=in[i];
      out_num = speex_resample_float(st, fin, NN, fout);
      //fprintf (stderr, "%d\n", out_num);
      for (i=0;i<2*NN;i++)
         out[i]=fout[i];
      fwrite(out, sizeof(short), 2*NN, stdout);
   }
   free(in);
   free(out);
   free(fin);
   free(fout);
   return 0;
}

