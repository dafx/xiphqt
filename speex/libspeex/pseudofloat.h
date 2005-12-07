/* Copyright (C) 2005 Jean-Marc Valin */
/**
   @file pseudofloat.h
   @brief Pseudo-floating point
 */
/*
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

#ifndef PSEUDOFLOAT_H
#define PSEUDOFLOAT_H

#include "misc.h"

typedef struct {
   spx_int16_t m;
   spx_int16_t e;
} spx_float_t;

/*#define FLOAT_MULT(a,b) ((spx_float_t) {(spx_int16_t)((spx_int32_t)(a).m*(b).m>>15), (a).e+(b).e+15})

#define FLOAT_ADD(a,b) ( (a).e > (b).e ? (spx_float_t) {((a).m>>1) + ((a).m>>((a).e-(b).e+1)),(a).e+1} : (spx_float_t) {((b).m>>1) + ((a).m>>((b).e-(a).e+1)),(b).e+1})*/

static inline spx_float_t PSEUDOFLOAT(float x)
{
   int e=0;
   if (x==0)
      return (spx_float_t) {0,0};
   while (x>32767)
   {
      x *= .5;
      e++;
   }
   while (x<16383)
   {
      x *= 2;
      e--;
   }
   return (spx_float_t) {x,e};
}

static inline spx_float_t FLOAT_ADD(spx_float_t a, spx_float_t b)
{
   spx_float_t r = (a).e > (b).e ? (spx_float_t) {((a).m>>1) + ((a).m>>((a).e-(b).e+1)),(a).e+1} : (spx_float_t) {((b).m>>1) + ((a).m>>((b).e-(a).e+1)),(b).e+1};
   if (r.m<16384)
   {
      r.m<<=1;
      r.e-=1;
   }
   return r;
}

static inline spx_float_t FLOAT_MULT(spx_float_t a, spx_float_t b)
{
   spx_float_t r = (spx_float_t) {(spx_int16_t)((spx_int32_t)(a).m*(b).m>>15), (a).e+(b).e+15};
   if (r.m<16384)
   {
      r.m<<=1;
      r.e-=1;
   }
   return r;   
}

static inline spx_float_t FLOAT_SHR(spx_float_t a, int b)
{
   return (spx_float_t) {a.m,a.e-b};
}

static inline spx_float_t FLOAT_SHL(spx_float_t a, int b)
{
   return (spx_float_t) {a.m,a.e+b};
}

static inline spx_int16_t FLOAT_EXTRACT16(spx_float_t a)
{
   if (a.e<0)
      return (a.m+(1<<(-a.e-1)))>>-a.e;
   else
      return a.m<<a.e;
}

static inline spx_int32_t FLOAT_EXTRACT32(spx_float_t a)
{
   if (a.e<0)
      return ((spx_int32_t)a.m+(1<<(-a.e-1)))>>-a.e;
   else
      return ((spx_int32_t)a.m)<<a.e;
}

static inline spx_int32_t FLOAT_MUL32(spx_float_t a, spx_word32_t b)
{
   if (a.e<-15)
      return SHR32(MULT16_32_Q15(a.m, b),-a.e-15);
   else
      return SHL32(MULT16_32_Q15(a.m, b),15+a.e);
}

static inline spx_float_t FLOAT_DIV32(spx_word32_t a, spx_word32_t b)
{
   int e=0;
   /* FIXME: Handle the sign */
   if (a==0)
      return (spx_float_t) {0,0};
   while (b>32767)
   {
      b >>= 1;
      e--;
   }
   while (a<SHL32(b,14))
   {
      a <<= 1;
      e--;
   }
   while (a>=SHL32(b-1,15))
   {
      a >>= 1;
      e++;
   }
   return (spx_float_t) {DIV32_16(a,b),e};
}

#endif
