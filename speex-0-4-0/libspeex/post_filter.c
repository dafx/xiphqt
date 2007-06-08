/* Copyright (C) 2002 Jean-Marc Valin 
   File: post_filter.c
   Post-filtering routines: processing to enhance perceptual quality

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

#include <math.h>
#include "filters.h"
#include "stack_alloc.h"
#include "post_filter.h"

/* Perceptual post-filter for narrowband */
void nb_post_filter(
float *exc, 
float *new_exc, 
float *ak, 
int p, 
int nsf,
int pitch,
float *pitch_gain,
void *par,
float *mem,
float *mem2,
float *stack)
{
   int i;
   float exc_energy=0, new_exc_energy=0;
   float *awk;
   float gain;
   pf_params *params;
   float *tmp_exc;
   float formant_num, formant_den, voiced_fact;

   params = (pf_params*) par;

   awk = PUSH(stack, p);
   tmp_exc = PUSH(stack, nsf);
  

   for (i=0;i<nsf;i++)
      exc_energy+=exc[i]*exc[i];

   for (i=0;i<nsf;i++)
   {
      new_exc[i] = exc[i] + params->pitch_enh*(
                                               pitch_gain[0]*exc[i-pitch+1] +
                                               pitch_gain[1]*exc[i-pitch] +
                                               pitch_gain[2]*exc[i-pitch-1]
                                               );
   }
   
   voiced_fact = pitch_gain[0]+pitch_gain[1]+pitch_gain[2];
   if (voiced_fact<0)
      voiced_fact=0;
   if (voiced_fact>1)
      voiced_fact=1;
   formant_num = params->formant_enh_num;
   formant_den =   voiced_fact    * params->formant_enh_den 
                 + (1-voiced_fact)* params->formant_enh_num;

   bw_lpc (formant_num, ak, awk, p);
   residue_mem(new_exc, awk, tmp_exc, nsf, p, mem);
   bw_lpc (formant_den, ak, awk, p);
   syn_filt_mem(tmp_exc, awk, new_exc, nsf, p, mem2);

   for (i=0;i<nsf;i++)
      new_exc_energy+=new_exc[i]*new_exc[i];

   gain = sqrt(exc_energy)/sqrt(.1+new_exc_energy);
   /*gain=1;*/
   for (i=0;i<nsf;i++)
      new_exc[i] *= gain;
   
   POP(stack);
   POP(stack);
}
