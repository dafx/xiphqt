/* Copyright (C) 2002 Jean-Marc Valin 
   File: filters.c
   Various analysis/synthesis filters

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   
   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   
   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
   
   - Neither the name of the Xiph.org Foundation nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.
   
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "filters.h"
#include "stack_alloc.h"
#include <stdio.h>
#include <math.h>

#define min(a,b) ((a) < (b) ? (a) : (b))

#define MAX_ORD 20

void print_vec(float *vec, int len, char *name)
{
   int i;
   printf ("%s ", name);
   for (i=0;i<len;i++)
      printf (" %f", vec[i]);
   printf ("\n");
}

void bw_lpc(float gamma, float *lpc_in, float *lpc_out, int order)
{
   int i;
   float tmp=1;
   for (i=0;i<order+1;i++)
   {
      lpc_out[i] = tmp * lpc_in[i];
      tmp *= gamma;
   }
}


void filter_mem2(float *x, float *num, float *den, float *y, int N, int ord, float *mem)
{
   int i,j;
   float xi,yi;
   for (i=0;i<N;i++)
   {
      xi=x[i];
      y[i] = num[0]*xi + mem[0];
      yi=y[i];
      for (j=0;j<ord-1;j++)
      {
         mem[j] = mem[j+1] + num[j+1]*xi - den[j+1]*yi;
      }
      mem[ord-1] = num[ord]*xi - den[ord]*yi;
   }
}

void fir_mem2(float *x, float *num, float *y, int N, int ord, float *mem)
{
   int i,j;
   float xi;
   for (i=0;i<N;i++)
   {
      xi=x[i];
      y[i] = num[0]*xi + mem[0];
      for (j=0;j<ord-1;j++)
      {
         mem[j] = mem[j+1] + num[j+1]*xi;
      }
      mem[ord-1] = num[ord]*xi;
   }
}

void iir_mem2(float *x, float *den, float *y, int N, int ord, float *mem)
{
   int i,j;
   for (i=0;i<N;i++)
   {
      y[i] = x[i] + mem[0];
      for (j=0;j<ord-1;j++)
      {
         mem[j] = mem[j+1] - den[j+1]*y[i];
      }
      mem[ord-1] = - den[ord]*y[i];
   }
}



void syn_percep_zero(float *xx, float *ak, float *awk1, float *awk2, float *y, int N, int ord, float *stack)
{
   int i;
   float *mem = PUSH(stack,ord);
   for (i=0;i<ord;i++)
      mem[i]=0;
   filter_mem2(xx, awk1, ak, y, N, ord, mem);
   for (i=0;i<ord;i++)
     mem[i]=0;
   iir_mem2(y, awk2, y, N, ord, mem);
}

void residue_percep_zero(float *xx, float *ak, float *awk1, float *awk2, float *y, int N, int ord, float *stack)
{
   int i;
   float *mem = PUSH(stack,ord);
   for (i=0;i<ord;i++)
      mem[i]=0;
   filter_mem2(xx, ak, awk1, y, N, ord, mem);
   for (i=0;i<ord;i++)
     mem[i]=0;
   fir_mem2(y, awk2, y, N, ord, mem);
}

#define MAX_FILTER 100
#define MAX_SIGNAL 1000

void qmf_decomp(float *xx, float *aa, float *y1, float *y2, int N, int M, float *mem)
{
   int i,j,k,M2;
   /* FIXME: this should be dynamic */
   float a[MAX_FILTER];
   float x[MAX_SIGNAL];
   float *x2=x+M-1;
   M2=M>>1;
   for (i=0;i<M;i++)
      a[M-i-1]=aa[i];
   for (i=0;i<M-1;i++)
      x[i]=mem[M-i-2];
   for (i=0;i<N;i++)
      x[i+M-1]=xx[i];
   for (i=0,k=0;i<N;i+=2,k++)
   {
      y1[k]=0;
      y2[k]=0;
      for (j=0;j<M2;j++)
      {
         y1[k]+=a[j]*(x[i+j]+x2[i-j]);
         y2[k]-=a[j]*(x[i+j]-x2[i-j]);
         j++;
         y1[k]+=a[j]*(x[i+j]+x2[i-j]);
         y2[k]+=a[j]*(x[i+j]-x2[i-j]);
      }
   }
   for (i=0;i<M-1;i++)
     mem[i]=xx[N-i-1];
}

void fir_decim_mem(float *xx, float *aa, float *y, int N, int M, float *mem)
{
   int i,j,M2;
   float a[MAX_FILTER];
   float x[MAX_SIGNAL];
   M2=M>>1;
   for (i=0;i<M;i++)
      a[M-i-1]=aa[i];
   for (i=0;i<M-1;i++)
      x[i]=mem[M-i-2];
   for (i=0;i<N;i++)
      x[i+M-1]=xx[i];
   for (i=0;i<N;i++)
   {
      y[i]=0;
      for (j=1;j<M;j+=2)
         y[i]+=a[j]*x[i+j];
      i++;
      y[i]=0;
      for (j=0;j<M;j+=2)
         y[i]+=a[j]*x[i+j];
   }
   for (i=0;i<M-1;i++)
     mem[i]=xx[N-i-1];
}

void comb_filter(
float *exc,          /*decoded excitation*/
float *new_exc,      /*enhanced excitation*/
float *ak,           /*LPC filter coefs*/
int p,               /*LPC order*/
int nsf,             /*sub-frame size*/
int pitch,           /*pitch period*/
float *pitch_gain,   /*pitch gain (3-tap)*/
float  comb_gain     /*gain of comb filter*/
)
{
   int i;
   float exc_energy=0, new_exc_energy=0;
   float gain;

   /*Compute excitation energy prior to enhancement*/
   for (i=0;i<nsf;i++)
      exc_energy+=exc[i]*exc[i];

   /*Apply pitch comb-filter (filter out noise between pitch harmonics)*/
   for (i=0;i<nsf;i++)
   {
      new_exc[i] = exc[i] + comb_gain * (
                                         pitch_gain[0]*exc[i-pitch+1] +
                                         pitch_gain[1]*exc[i-pitch] +
                                         pitch_gain[2]*exc[i-pitch-1]
                                         );
   }
   
   /*Gain after enhancement*/
   for (i=0;i<nsf;i++)
      new_exc_energy+=new_exc[i]*new_exc[i];

   /*Compute scaling factor and normalize energy*/
   gain = sqrt(exc_energy)/sqrt(.1+new_exc_energy);
   for (i=0;i<nsf;i++)
      new_exc[i] *= gain;
}
