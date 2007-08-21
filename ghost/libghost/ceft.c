/* Copyright (C) 2007

   Code-Excited Fourier Transform -- This is highly experimental and
   it's not clear at all it even has a remote chance of working


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
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "ceft.h"
#include "filterbank.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "fftwrap.h"

/*
0
1
2
3
4
5
6
7
8
9
10 11
12 13
14 15
16 17 18 19
20 21 22 23
24 25 26 27
28 29 30 31 32 33
34 35 36 37 38 39 49 41
42 43 44 45 46 47 48 49
50 .. 63   (14)
64 .. 83   (20)
84 .. 127  (42)
*/

#define NBANDS 23 /*or 22 if we discard the small last band*/
int qbank[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 12, 14, 16, 20, 24, 28, 36, 44, 52, 68, 84, 116, 128};

#if 0
void compute_bank(float *ps, float *bank)
{
   int i;
   for (i=0;i<NBANDS;i++)
   {
      int j;
      bank[i]=0;
      for (j=qbank[i];j<qbank[i+1];j++)
         bank[i] += ps[j];
      bank[i] = sqrt(bank[i]/(qbank[i+1]-qbank[i]));
   }
}
#else
void compute_bank(float *X, float *bank)
{
   int i;
   bank[0] = 1e-10+fabs(X[0]);
   for (i=1;i<NBANDS;i++)
   {
      int j;
      bank[i] = 1e-10;
      for (j=qbank[i];j<qbank[i+1];j++)
      {
         bank[i] += X[j*2-1]*X[j*2-1];
         bank[i] += X[j*2]*X[j*2];
      }
      bank[i] = sqrt(.5*bank[i]/(qbank[i+1]-qbank[i]));
   }
   //FIXME: Kludge
   X[255] = 1;
}
#endif

void normalise_bank(float *X, float *bank)
{
   int i;
   X[0] /= bank[0];
   for (i=1;i<NBANDS;i++)
   {
      int j;
      float x = 1.f/bank[i];
      for (j=qbank[i];j<qbank[i+1];j++)
      {
         X[j*2-1] *= x;
         X[j*2]   *= x;
      }
   }
   //FIXME: Kludge
   X[255] = 0;
}

void denormalise_bank(float *X, float *bank)
{
   int i;
   X[0] *= bank[0];
   for (i=1;i<NBANDS;i++)
   {
      int j;
      float x = bank[i];
      for (j=qbank[i];j<qbank[i+1];j++)
      {
         X[j*2-1] *= x;
         X[j*2]   *= x;
      }
   }
   //FIXME: Kludge
   X[255] = 0;
}

#define BARK_BANDS 20

struct CEFTState_ {
   FilterBank *bank;
   void *frame_fft;
   int length;
};

CEFTState *ceft_init(int len)
{
   CEFTState *st = malloc(sizeof(CEFTState));
   st->length = len;
   st->frame_fft = spx_fft_init(st->length);
   st->bank = filterbank_new(BARK_BANDS, 48000, st->length>>1, 0);
   return st;
}

void ceft_encode(CEFTState *st, float *in, float *out)
{
   float bark[BARK_BANDS];
   float Xps[st->length>>1];
   float X[st->length];
   int i;

   spx_fft_float(st->frame_fft, in, X);
   
   Xps[0] = .1+X[0]*X[0];
   for (i=1;i<st->length>>1;i++)
      Xps[i] = .1+X[2*i-1]*X[2*i-1] + X[2*i]*X[2*i];

#if 1
   float bank[NBANDS];
   compute_bank(X, bank);
   normalise_bank(X, bank);
#else
   filterbank_compute_bank(st->bank, Xps, bark);
   filterbank_compute_psd(st->bank, bark, Xps);
   
   for(i=0;i<st->length>>1;i++)
      Xps[i] = sqrt(Xps[i]);
   X[0] /= Xps[0];
   for (i=1;i<st->length>>1;i++)
   {
      X[2*i-1] /= Xps[i];
      X[2*i] /= Xps[i];
   }
   X[st->length-1] /= Xps[(st->length>>1)-1];
#endif
   
   /*for(i=0;i<st->length;i++)
      printf ("%f ", X[i]);
   printf ("\n");
*/
   
   for(i=0;i<st->length;i++)
   {
      float q = 4;
      if (i<10)
         q = 8;
      else if (i<20)
         q = 4;
      else if (i<30)
         q = 2;
      else if (i<50)
         q = 1;
      else
         q = .5;
      //q=1;
      int sq = floor(.5+q*X[i]);
      printf ("%d ", sq);
      X[i] = (1.f/q)*(sq);
   }
   printf ("\n");
#if 0
   X[0]  *= Xps[0];
   for (i=1;i<st->length>>1;i++)
   {
      X[2*i-1] *= Xps[i];
      X[2*i] *= Xps[i];
   }
   X[st->length-1] *= Xps[(st->length>>1)-1];
#else
   float bank2[NBANDS];
   compute_bank(X, bank2);
   normalise_bank(X, bank2);

   denormalise_bank(X, bank);
#endif
   
   
#if 0
   for(i=0;i<BARK_BANDS;i++)
   {
      printf("%f ", bark[i]);
   }
   printf ("\n");
#endif 
   /*
   for(i=0;i<st->length;i++)
      printf ("%f ", X[i]);
   printf ("\n");
   */

   spx_ifft_float(st->frame_fft, X, out);

}

