/*
 *
 *  postfish
 *    
 *      Copyright (C) 2002-2004 Monty and Xiph.Org
 *
 *  Postfish is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  Postfish is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with Postfish; see the file COPYING.  If not, write to the
 *  Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * 
 */

#include "postfish.h"
#include "feedback.h"
#include <fftw3.h>
#include "subband.h"
#include "bessel.h"
#include "suppress.h"

/* (since this one is kinda unique) The Reverberation Suppressor....
   
   Reverberation in a measurably live environment displays
   log amplitude decay with time (linear decay when plotted on a dB
   scale).
   
   In its simplest form, the suppressor follows actual RMS amplitude
   attacks but chooses a slower-than-actual decay, then expands
   according to the dB distance between the slow and actual decay.
   
   Thus, the suppressor can be used to 'dry out' a very 'wet'
   reverberative track. */
    
extern int input_size;
extern int input_rate;
extern int input_ch;

typedef struct {
  subband_state ss;
  
  iir_filter smooth;
  iir_filter trigger;
  iir_filter release;
  
  iir_state *iirS[suppress_freqs];
  iir_state *iirT[suppress_freqs];
  iir_state *iirR[suppress_freqs];

  float prevratio[suppress_freqs];

} suppress_state;

suppress_settings suppress_channel_set;
static suppress_state channel_state;
static subband_window sw;

void suppress_reset(){
  int i,j;
  
  subband_reset(&channel_state.ss);
  
  for(i=0;i<suppress_freqs;i++){
    for(j=0;j<input_ch;j++){
      memset(&channel_state.iirS[i][j],0,sizeof(iir_state));
      memset(&channel_state.iirT[i][j],0,sizeof(iir_state));
      memset(&channel_state.iirR[i][j],0,sizeof(iir_state));
    }
  }
}

static void filter_set(subband_state *ss,
		       float msec,
		       iir_filter *filter,
		       int attackp,
		       int order){
  float alpha;
  float corner_freq= 500./msec;
  
  /* make sure the chosen frequency doesn't require a lookahead
     greater than what's available */
  if(impulse_freq2(input_size*2-ss->qblocksize*3)*1.01>corner_freq && 
     attackp)
    corner_freq=impulse_freq2(input_size*2-ss->qblocksize*3);
  
  alpha=corner_freq/input_rate;
  filter->g=mkbessel(alpha,order,filter->c);
  filter->alpha=alpha;
  filter->Hz=alpha*input_rate;
  filter->ms=msec;
}

int suppress_load(void){
  int i;
  int qblocksize=input_size/16;
  memset(&channel_state,0,sizeof(channel_state));

  suppress_channel_set.active=calloc(input_ch,sizeof(*suppress_channel_set.active));

  subband_load(&channel_state.ss,suppress_freqs,qblocksize,input_ch);
  subband_load_freqs(&channel_state.ss,&sw,suppress_freq_list,suppress_freqs);
   
  for(i=0;i<suppress_freqs;i++){
    channel_state.iirS[i]=calloc(input_ch,sizeof(iir_state));
    channel_state.iirT[i]=calloc(input_ch,sizeof(iir_state));
    channel_state.iirR[i]=calloc(input_ch,sizeof(iir_state));
  }
  return 0;
}

static void suppress_work_helper(void *vs, suppress_settings *sset){
  suppress_state *sss=(suppress_state *)vs;
  subband_state *ss=&sss->ss;
  int i,j,k,l;
  float smoothms=sset->smooth*.1;
  float triggerms=sset->trigger*.1;
  float releasems=sset->release*.1;
  iir_filter *trigger=&sss->trigger;
  iir_filter *smooth=&sss->smooth;
  iir_filter *release=&sss->release;
  int ahead;

  if(smoothms!=smooth->ms)filter_set(ss,smoothms,smooth,1,2);
  if(triggerms!=trigger->ms)filter_set(ss,triggerms,trigger,0,2);
  if(releasems!=release->ms)filter_set(ss,releasems,release,0,2);

  ahead=impulse_ahead2(smooth->alpha);
  
  for(i=0;i<suppress_freqs;i++){
    int firstlink=0;
    float fast[input_size];
    float slow[input_size];
    float multiplier = 1.-1000./sset->ratio[i];
    
    for(j=0;j<input_ch;j++){
      int active=(ss->effect_active1[j] || 
		  ss->effect_active0[j] || 
		  ss->effect_activeC[j]);
      
      if(active){
	if(sset->linkp){
	  if(!firstlink){
	    firstlink++;
	    memset(fast,0,sizeof(fast));
	    float scale=1./input_ch;
	    for(l=0;l<input_ch;l++){
	      float *x=sss->ss.lap[i][l]+ahead;
	      for(k=0;k<input_size;k++)
		fast[k]+=x[k]*x[k];
	    }
	    for(k=0;k<input_size;k++)
	      fast[k]*=scale;
	    
	  }
	  
	}else{
	  float *x=sss->ss.lap[i][j]+ahead;
	  for(k=0;k<input_size;k++)
	    fast[k]=x[k]*x[k];
	}
	
	
	if(sset->linkp==0 || firstlink==1){
	  
	  memcpy(slow,fast,sizeof(slow));

	  
	  compute_iir_fast_attack2(fast, input_size, &sss->iirT[i][j],
				smooth,trigger);
	  compute_iir_fast_attack2(slow, input_size, &sss->iirR[i][j],
				smooth,release);
	  
	  //_analysis("fast",i,fast,input_size,1,offset);
	  //_analysis("slow",i,slow,input_size,1,offset);

	  if(multiplier==sss->prevratio[i]){

	    for(k=0;k<input_size;k++)
	      fast[k]=fromdB_a((todB_a(slow+k)-todB_a(fast+k))*.5*multiplier);

	  }else{
	    float multiplier_add=(multiplier-sss->prevratio[i])/input_size;
	    multiplier=sss->prevratio[i];

	    for(k=0;k<input_size;k++){
	      fast[k]=fromdB_a((todB_a(slow+k)-todB_a(fast+k))*.5*multiplier);
	      multiplier+=multiplier_add;
	    }

	  }

	  //_analysis("adj",i,fast,input_size,1,offset);

	  if(sset->linkp && firstlink==1){

	    for(l=0;l<input_ch;l++){
	      if(l!=j){
		memcpy(&sss->iirS[i][l],&sss->iirS[i][j],sizeof(iir_state));
		memcpy(&sss->iirT[i][l],&sss->iirT[i][j],sizeof(iir_state));
		memcpy(&sss->iirR[i][l],&sss->iirR[i][j],sizeof(iir_state));
	      }
	    }
	  }

	  firstlink++;
	}
	
	
	{
	  float *x=sss->ss.lap[i][j];
	  for(k=0;k<input_size;k++)
	    if(fast[k]<1.)
	      x[k]*=fast[k];
	}
      }else{
	/* reset filters to sane inactive default */
	memset(&sss->iirS[i][j],0,sizeof(iir_state));
	memset(&sss->iirT[i][j],0,sizeof(iir_state));
	memset(&sss->iirR[i][j],0,sizeof(iir_state));
      }
    }

    sss->prevratio[i]=multiplier;

  }
}

static void suppress_work_channel(void *vs){
  suppress_work_helper(vs,&suppress_channel_set);
}

time_linkage *suppress_read_channel(time_linkage *in){
  int visible[input_ch];
  int active [input_ch];
  subband_window *w[input_ch];
  int i;
  
  for(i=0;i<input_ch;i++){
    visible[i]=0;
    active[i]=suppress_channel_set.active[i];
    w[i]=&sw;
  }
  
  return subband_read(in, &channel_state.ss, w, visible, active, 
		      suppress_work_channel, &channel_state);
}
