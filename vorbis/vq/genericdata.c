/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU LESSER/LIBRARY PUBLIC LICENSE, WHICH IS INCLUDED WITH    *
 * THIS SOURCE. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.        *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2000             *
 * by Monty <monty@xiph.org> and the XIPHOPHORUS Company            *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

 function: generic euclidian distance metric for VQ codebooks
 last mod: $Id: genericdata.c,v 1.6.2.1 2000/11/04 06:22:10 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include "vqgen.h"
#include "vqext.h"

char *vqext_booktype="GENERICdata";  
int vqext_aux=0;                
quant_meta q={0,0,0,0};          /* non sequence data; each scalar 
				    independent */

void vqext_quantize(vqgen *v,quant_meta *q){
  vqgen_quantize(v,q);
}

float *vqext_weight(vqgen *v,float *p){
  /*noop*/
  return(p);
}

                            /* candidate,actual */
float vqext_metric(vqgen *v,float *e, float *p){
  int i;
  float acc=0.;
  for(i=0;i<v->elements;i++){
    float val=p[i]-e[i];
    acc+=val*val;
  }
  return sqrt(acc/v->elements);
}

void vqext_addpoint_adj(vqgen *v,float *b,int start,int dim,int cols,int num){
  vqgen_addpoint(v,b+start,NULL);
}

void vqext_preprocess(vqgen *v){
  /* noop */
}






