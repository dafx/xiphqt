/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggGhost SOFTWARE CODEC SOURCE CODE.    *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggGhost SOURCE CODE IS (C) COPYRIGHT 2007-2011              *
 * by the Xiph.Org Foundation http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: research-grade chirp estimation code
 last mod: $Id$

 Provides a set of chirp estimation algorithm variants for comparison
 and characterization.  This is canned code meant for testing and
 exploration of sinusoidal estimation.  It's designed to be held still
 and used for reference and comparison against later improvement.
 Please, don't optimize this code.  Test optimizations elsewhere in
 code to be compared to this code.  Roll new technique improvements
 into this reference set only after they have been completed.

 ********************************************************************/

#define _GNU_SOURCE
#include <math.h>
#include "chirp.h"
#include "scales.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int cmp_descending_A(const void *p1, const void *p2){
  chirp *A = (chirp *)p1;
  chirp *B = (chirp *)p2;

  return (A->A<B->A) - (B->A<A->A);
}
static int cmp_ascending_W(const void *p1, const void *p2){
  chirp *A = (chirp *)p1;
  chirp *B = (chirp *)p2;

  return (B->W<A->W) - (A->W<B->W);
}

/* The stimators project a residue vecotr onto basis functions; the
   results of the accumulation map directly to the parameters we're
   interested in (A, P, W, dA, dW, ddA) and vice versa.  The mapping
   equations are broken out below. */

static float toA(float Ai, float Bi){
  return hypotf(Ai,Bi);
}
static float toP(float Ai, float Bi){
  return -atan2f(Bi, Ai);
}
static float toW(float Ai, float Bi, float Ci, float Di){
  return (Ai*Ai || Bi*Bi) ? (Ci*Bi - Di*Ai)/(Ai*Ai + Bi*Bi) : 0;
}
static float todA(float Ai, float Bi, float Ci, float Di){
  return (Ai*Ai || Bi*Bi) ? (Ci*Ai + Di*Bi)/hypotf(Ai,Bi) : 0;
}
static float todW(float Ai, float Bi, float Ei, float Fi){
  return (Ai*Ai || Bi*Bi) ? (Ei*Bi - Fi*Ai)/(Ai*Ai + Bi*Bi) : 0;
}
static float toddA(float Ai, float Bi, float Ei, float Fi){
  return (Ai*Ai || Bi*Bi) ? (Ei*Ai + Fi*Bi)/hypotf(Ai,Bi) : 0;
}

static float toAi(float A, float P){
  return A * cosf(-P);
}
static float toBi(float A, float P){
  return A * sinf(-P);
}

static float WtoCi(float Ai, float Bi, float W){
  return W*Bi;
}
static float WtoDi(float Ai, float Bi, float W){
  return -W*Ai;
}
static float dWtoEi(float Ai, float Bi, float dW){
  return dW*Bi;
}
static float dWtoFi(float Ai, float Bi, float dW){
  return -dW*Ai;
}

static float dAtoCi(float Ai, float Bi, float dA){
  return (Ai*Ai || Bi*Bi) ? dA*Ai/hypotf(Ai,Bi) : 0.;
}
static float dAtoDi(float Ai, float Bi, float dA){
  return (Ai*Ai || Bi*Bi) ? dA*Bi/hypotf(Ai,Bi) : 0.;
}
static float ddAtoEi(float Ai, float Bi, float ddA){
  return (Ai*Ai || Bi*Bi) ? ddA*Ai/hypotf(Ai,Bi) : 0.;
}
static float ddAtoFi(float Ai, float Bi, float ddA){
  return (Ai*Ai || Bi*Bi) ? ddA*Bi/hypotf(Ai,Bi) : 0.;
}

/* Nonlinear estimation iterator; functionally, it should differ from
   the linear version only in that it restarts the basis functions for
   each chirp after each new fit. */
static int nonlinear_iterate(const float *x,
                             float *y,
                             const float *window,
                             int len,
                             chirp *cc,
                             int n,

                             float fit_limit,
                             int iter_limit,
                             int fit_gs,

                             int fitW,
                             int fitdA,
                             int fitdW,
                             int fitddA,
                             int fit_recenter_dW,
                             float fit_W_alpha,
                             float fit_dW_alpha,

                             int symm_norm,
                             float E0,
                             float E1,
                             float E2,

                             int bound_edges){
  int i,j;
  int flag=1;
  float r[len];

  /* outer fit iteration */
  while(flag && iter_limit>0){
    flag=0;

    /* precompute the portion of the projection/fit estimate shared by
       the zero, first and second order fits.  Subtracts the current
       best fits from the input signal and windows the result. */
    for(j=0;j<len;j++)
      r[j]=(x[j]-y[j])*window[j];
    memset(y,0,sizeof(*y)*len);

    /* Sort chirps by descending amplitude */
    qsort(cc, n, sizeof(*cc), cmp_descending_A);

    for(i=0;i<n;i++){
      chirp *c = cc+i;
      float aP=0, bP=0;
      float cP=0, dP=0;
      float eP=0, fP=0;
      float cP2=0, dP2=0;
      float eP2=0, fP2=0;
      float eP3=0, fP3=0;
      float aE=0, bE=0;
      float cE=0, dE=0;
      float eE=0, fE=0;
      float aC = cos(c->P);
      float aS = sin(c->P);

      /* Not recentering dW allows potential simplification of the
         nonlinear solver. This code is designed to recenter dW as
         easily as not, so the following looks a bit silly.  The point
         of the flag is to emulate/study the behavior of the simplified
         algorithm */
      float cdW = fit_recenter_dW ? c->dW : 0;

      for(j=0;j<len;j++){

        /* no part of the nonlinear algorithm requires double
           precision, however the largest source of noise will be from
           a naive computation of the instantaneous basis or
           reconstruction chirp phase.  Because dW is usually very
           small compared to W (especially c->W*jj), (W + dW*jj)*jj
           quickly becomes noticably short of significant bits.

           We can either calculate everything mod 2PI, use a double
           for just this calculation (and passing the result to
           sincos) or decide we don't care.  In the production code we
           will probably not care as most of the depth in the
           estimator isn't needed.  Here we'll take the easiest route
           to have a deep reference and just use doubles. */

        double jj = j-len*.5+.5;
        float jj2 = jj*jj;
        double co,si;
        float c2,s2;
        float yy=r[j];

        sincos((c->W + cdW*jj)*jj,&si,&co);

        si*=window[j];
        co*=window[j];
        c2 = co*co*jj;
        s2 = si*si*jj;

        /* add the current estimate back to the residue vector */
        r[j] += (aC*co-aS*si) * (c->A + (c->dA + c->ddA*jj)*jj);

        /* zero order projection */
        aP += co*yy;
        bP += si*yy;
        /* first order projection */
        cP += co*yy*jj;
        dP += si*yy*jj;
        /* second order projection */
        eP += co*yy*jj2;
        fP += si*yy*jj2;

        if(fit_gs){
          /* subtract zero order estimate from first */
          cP2 += c2;
          dP2 += s2;
          /* subtract zero order estimate from second */
          eP2 += c2*jj;
          fP2 += s2*jj;
          /* subtract first order estimate from second */
          eP3 += c2*jj2;
          fP3 += s2*jj2;
        }

        if(!symm_norm){
          /* accumulate per-basis energy for normalization */
          aE += co*co;
          bE += si*si;
          cE += c2*jj;
          dE += s2*jj;
          eE += c2*jj*jj2;
          fE += s2*jj*jj2;
        }
      }

      /* normalize, complete projections */
      if(symm_norm){
        aP *= E0;
        bP *= E0;
        cP = (cP-aP*cP2)*E1;
        dP = (dP-bP*dP2)*E1;
        eP = (eP-aP*eP2-cP*eP3)*E2;
        fP = (fP-bP*fP2-dP*fP3)*E2;
      }else{
        aP = (aE?aP/aE:0.);
        bP = (bE?bP/bE:0.);
        cP = (cE?(cP-aP*cP2)/cE:0.);
        dP = (dE?(dP-bP*dP2)/dE:0.);
        eP = (eE?(eP-aP*eP2-cP*eP3)/eE:0.);
        fP = (fE?(fP-bP*fP2-dP*fP3)/fE:0.);
      }

      if(!fitW && !fitdA)
        cP=dP=0;
      if(!fitdW && !fitddA)
        eP=fP=0;

      /* base convergence on relative projection movement this
         iteration */
      {
        float move = (aP*aP + bP*bP)/(c->A*c->A + fit_limit*fit_limit) +
          (cP*cP + dP*dP)/(c->A*c->A + fit_limit*fit_limit) +
          (eP*eP + fP*fP)/(c->A*c->A + fit_limit*fit_limit);

        if(fit_limit>0 && move>fit_limit*fit_limit)flag=1;
        if(fit_limit<0 && move>1e-14)flag=1;
      }

      /* we're fitting to the remaining error; add the fit to date
         back in to relate our newest incremental results to the
         global fit so far.  Note that this does not include W or dW,
         as they're already 'subtracted' when the bases are recentered
         each iteration */
      {
        float A = toAi(c->A, c->P);
        float B = toBi(c->A, c->P);
        aP += A;
        bP += B;
        cP += dAtoCi(A,B,c->dA);
        dP += dAtoDi(A,B,c->dA);
        eP += ddAtoEi(A,B,c->ddA);
        fP += ddAtoFi(A,B,c->ddA);

        /* guard overflow; if we're this far out, assume we're never
           coming back. drop out now. */
        if((aP*aP + bP*bP)>1e10 ||
           (cP*cP + dP*dP)>1e10 ||
           (eP*eP + fP*fP)>1e10){
          iter_limit=0;
          i=n;
        }
      }

      /* save new estimate */
      c->A = toA(aP,bP);
      c->P = toP(aP,bP);
      c->W += fit_W_alpha*(fitW ? toW(aP,bP,cP,dP) : 0);
      c->dA = (fitdA ? todA(aP,bP,cP,dP) : 0);
      if(fit_recenter_dW)
        c->dW += fit_dW_alpha*(fitdW ? todW(aP,bP,eP,fP) : 0);
      else
        c->dW = (fitdW ? todW(aP,bP,eP,fP) : 0);
      c->ddA = (fitddA ? toddA(aP,bP,eP,fP) : 0);

      if(bound_edges){
        /* do not allow negative or trans-Nyquist center frequencies */
        if(c->W<0){
          c->W = 0; /* clamp frequency to 0 (DC) */
          c->dW = 0; /* if W is 0, the chirp rate must also be 0 to
                                 avoid negative frequencies */
        }
        if(c->W>M_PI){
          c->W = M_PI; /* clamp frequency to Nyquist */
          c->dW = 0; /* if W is at Nyquist, the chirp rate must
                           also be 0 to avoid trans-Nyquist
                           frequencies */
        }
        /* Just like with the center frequency, don't allow the
           chirp rate (dW) parameter to cross over to a negative
           instantaneous frequency */
        if(c->W+c->dW*len < 0)
          c->dW = -c->W/len;
        if(c->W-c->dW*len < 0)
          c->dW = c->W/len;
        /* ...or exceed Nyquist */
        if(c->W + c->dW*len > M_PI)
          c->dW = M_PI/len - c->W/len;
        if(c->W - c->dW*len > M_PI)
          c->dW = c->W/len - M_PI/len;
      }

      /* update the reconstruction/residue vectors with new fit */
      {
        float cdW = fit_recenter_dW ? c->dW : 0;
        for(j=0;j<len;j++){
          double jj = j-len*.5+.5;
          float a = c->A + c->dA*jj + c->ddA*jj*jj;
          float v = a*cos(cdW*jj*jj + c->P + c->W*jj);
          r[j] -= v*window[j];
          y[j] += v;
        }
      }
    }
    if(flag) iter_limit--;
  }
  return iter_limit;
}

/* linear estimation iterator; sets fixed basis functions for each
   chirp according to the passed in initial estimates. This code looks
   quite different from the nonlinear estimator above as I wanted to
   keep the structure very similar to the way it was as originally
   coded by JM. New techniques are bolted on rather than fully
   integrated. */

static int linear_iterate(const float *x,
                          float *y,
                          const float *window,
                          int len,
                          chirp *ch,
                          int n,

                          float fit_limit,
                          int iter_limit,
                          int fit_gs,

                          int fitW,
                          int fitdA,
                          int fitdW,
                          int fitddA,
                          int symm_norm,
                          float E0,
                          float E1,
                          float E2){

  float *cos_table[n];
  float *sin_table[n];
  float *tcos_table[n];
  float *tsin_table[n];
  float *ttcos_table[n];
  float *ttsin_table[n];
  float cosE[n], sinE[n];
  float tcosE[n], tsinE[n];
  float ttcosE[n], ttsinE[n];
  float tcosC2[n], tsinC2[n];
  float ttcosC2[n], ttsinC2[n];
  float ttcosC3[n], ttsinC3[n];

  float ai[n], bi[n];
  float ci[n], di[n];
  float ei[n], fi[n];
  int i,j;
  int flag=1;

  for (i=0;i<n;i++){
    float tmpa=0;
    float tmpb=0;
    float tmpc=0;
    float tmpd=0;
    float tmpe=0;
    float tmpf=0;

    float tmpc2=0;
    float tmpd2=0;
    float tmpe3=0;
    float tmpf3=0;

    /* Build a table for the basis functions at each frequency */
    cos_table[i]=calloc(len,sizeof(**cos_table));
    sin_table[i]=calloc(len,sizeof(**sin_table));
    tcos_table[i]=calloc(len,sizeof(**tcos_table));
    tsin_table[i]=calloc(len,sizeof(**tsin_table));
    ttcos_table[i]=calloc(len,sizeof(**ttcos_table));
    ttsin_table[i]=calloc(len,sizeof(**ttsin_table));

    for (j=0;j<len;j++){
      float jj = j-len/2.+.5;
      float c,s;
      sincosf((ch[i].W + ch[i].dW*jj)*jj,&s,&c);

      /* save basis funcs */
      cos_table[i][j] = c;
      sin_table[i][j] = s;
      tcos_table[i][j] = jj*c;
      tsin_table[i][j] = jj*s;
      ttcos_table[i][j] = jj*jj*c;
      ttsin_table[i][j] = jj*jj*s;

      /* The sinusoidal terms */
      tmpa += cos_table[i][j]*cos_table[i][j]*window[j]*window[j];
      tmpb += sin_table[i][j]*sin_table[i][j]*window[j]*window[j];

      /* The modulation terms */
      tmpc += tcos_table[i][j]*tcos_table[i][j]*window[j]*window[j];
      tmpd += tsin_table[i][j]*tsin_table[i][j]*window[j]*window[j];

      /* The second order modulations */
      tmpe += ttcos_table[i][j]*ttcos_table[i][j]*window[j]*window[j];
      tmpf += ttsin_table[i][j]*ttsin_table[i][j]*window[j]*window[j];

      /* gs fit terms */
      tmpc2 += cos_table[i][j]*tcos_table[i][j]*window[j]*window[j];
      tmpd2 += sin_table[i][j]*tsin_table[i][j]*window[j]*window[j];
      tmpe3 += tcos_table[i][j]*ttcos_table[i][j]*window[j]*window[j];
      tmpf3 += tsin_table[i][j]*ttsin_table[i][j]*window[j]*window[j];

    }

    /* Set basis normalizations */
    if(symm_norm){
      cosE[i] = sinE[i] = E0;
      tcosE[i] = tsinE[i] = E1;
      ttcosE[i] = ttsinE[i] = E2;
    }else{
      cosE[i] = (tmpa>0.f ? 1./tmpa : 0);
      sinE[i] = (tmpb>0.f ? 1./tmpb : 0);
      tcosE[i] = (tmpc>0.f ? 1./tmpc : 0);
      tsinE[i] = (tmpd>0.f ? 1./tmpd : 0);
      ttcosE[i] = (tmpe>0.f ? 1./tmpe : 0);
      ttsinE[i] = (tmpf>0.f ? 1./tmpf : 0);
    }

    /* set gs fit terms */
    tcosC2[i] = tmpc2;
    tsinC2[i] = tmpd2;
    ttcosC2[i] = tmpc;
    ttsinC2[i] = tmpd;
    ttcosC3[i] = tmpe3;
    ttsinC3[i] = tmpf3;

    /* seed the basis accumulators from our passed in estimated parameters */
    /* Don't add in W or dW; these are already included by the basis */
    {
      float A = toAi(ch[i].A, ch[i].P);
      float B = toBi(ch[i].A, ch[i].P);
      ai[i] = A;
      bi[i] = B;
      ci[i] = dAtoCi(A,B,ch[i].dA);
      di[i] = dAtoDi(A,B,ch[i].dA);
      ei[i] = ddAtoEi(A,B,ch[i].ddA);
      fi[i] = ddAtoFi(A,B,ch[i].ddA);
    }
  }

  while(flag && iter_limit){
    flag=0;

    for (i=0;i<n;i++){

      float tmpa=0, tmpb=0;
      float tmpc=0, tmpd=0;
      float tmpe=0, tmpf=0;

      /* (Sort of) project the residual on the four basis functions */
      for (j=0;j<len;j++){
        float r = (x[j]-y[j])*window[j]*window[j];
        tmpa += r*cos_table[i][j];
        tmpb += r*sin_table[i][j];
        tmpc += r*tcos_table[i][j];
        tmpd += r*tsin_table[i][j];
        tmpe += r*ttcos_table[i][j];
        tmpf += r*ttsin_table[i][j];
      }

      tmpa*=cosE[i];
      tmpb*=sinE[i];

      if(fit_gs){
        tmpc -= tmpa*tcosC2[i];
        tmpd -= tmpb*tsinC2[i];
      }

      tmpc*=tcosE[i];
      tmpd*=tsinE[i];

      if(fit_gs){
        tmpe -= tmpa*ttcosC2[i] + tmpc*ttcosC3[i];
        tmpf -= tmpb*ttsinC2[i] + tmpd*ttsinC3[i];
      }

      tmpe*=ttcosE[i];
      tmpf*=ttsinE[i];

      if(!fitW && !fitdA)
        tmpc=tmpd=0;
      if(!fitdW && !fitddA)
        tmpe=tmpf=0;

      /* Update the signal approximation for the next iteration */
      for (j=0;j<len;j++){
        y[j] +=
          tmpa*cos_table[i][j]+
          tmpb*sin_table[i][j]+
          tmpc*tcos_table[i][j]+
          tmpd*tsin_table[i][j]+
          tmpe*ttcos_table[i][j]+
          tmpf*ttsin_table[i][j];
      }

      ai[i] += tmpa;
      bi[i] += tmpb;
      ci[i] += tmpc;
      di[i] += tmpd;
      ei[i] += tmpe;
      fi[i] += tmpf;

      /* base convergence on basis projection movement this
         iteration */
      if( fit_limit<0 ||
          (tmpa*tmpa + tmpb*tmpb)/(ai[i]*ai[i]+bi[i]*bi[i]+fit_limit*fit_limit) +
          (tmpc*tmpc + tmpd*tmpd)/(ci[i]*ci[i]+di[i]*di[i]+fit_limit*fit_limit) +
          (tmpe*tmpe + tmpf*tmpf)/(ei[i]*ei[i]+fi[i]*fi[i]+fit_limit*fit_limit) > fit_limit*fit_limit) flag=1;

    }
    if(flag) iter_limit--;
  }

  for(i=0;i<n;i++){
    ch[i].A = toA(ai[i],bi[i]);
    ch[i].P = toP(ai[i],bi[i]);
    ch[i].W += (fitW ? toW(ai[i],bi[i],ci[i],di[i]) : 0);
    ch[i].dA = (fitdA ? todA(ai[i],bi[i],ci[i],di[i]) : 0);
    ch[i].dW += (fitdW ? todW(ai[i],bi[i],ei[i],fi[i]) : 0);
    ch[i].ddA = (fitddA ? toddA(ai[i],bi[i],ei[i],fi[i]) : 0);

    free(cos_table[i]);
    free(sin_table[i]);
    free(tcos_table[i]);
    free(tsin_table[i]);
    free(ttcos_table[i]);
    free(ttsin_table[i]);
  }

  return iter_limit;
}

/* Performs an iterative chirp estimation using the passed
   in chirp paramters as an initial estimate.

   x: input signal (unwindowed)
   y: output reconstruction (unwindowed)
   window: window to apply to input and bases during fitting process
   len: length of x,y,r and window vectors
   c: chirp initial estimation inputs, chirp fit outputs (sorted in frequency order)
   n: number of chirps
   iter_limit: maximum allowable number of iterations
   fit_limit: desired fit quality limit

   fitW : flag indicating that fitting should include the W parameter.
   fitdA : flag indicating that fitting should include the dA parameter.
   fitdW : flag indicating that fitting should include the dW parameter.
   fitddA : flag indicating that fitting should include the ddA parameter.
   symm_norm
   int fit_gs
   int bound_zero

   Input estimates affect convergence region and speed and fit
   quality; precise effects depend on the fit estimation algorithm
   selected.  Note that non-zero initial values for dA, dW or ddA are
   ignored when fitdA, fitdW, fitddA are unset.  An initial value for
   W is always used even when W is left unfitted.

   Fitting terminates when no fit parameter changes by more than
   fit_limit in an iteration or when the fit loop reaches the
   iteration limit.  The fit parameters as checked are scaled over the
   length of the block.

*/

int estimate_chirps(const float *x,
                    float *y,
                    const float *window,
                    int len,
                    chirp *c,
                    int n,

                    float fit_limit,
                    int iter_limit,
                    int fit_gs,

                    int fitW,
                    int fitdA,
                    int fitdW,
                    int fitddA,
                    int nonlinear,
                    float fit_W_alpha,
                    float fit_dW_alpha,
                    int symm_norm,
                    int bound_zero
                    ){

  int i,j;
  float E0=0, E1=0, E2=0;

  if(symm_norm){
    for(i=0;i<len;i++){
      float jj = i-len*.5+.5;
      float w2 = window[i]*window[i];
      float w2j2 = w2*jj*jj;
      float w2j4 = w2j2*jj*jj;
      E0 += w2;
      E1 += w2j2;
      E2 += w2j4;
    }
    E0=2/E0;
    E1=2/E1;
    E2=2/E2;
  }

  /* zero out initial estimates for params we'll not fit */
  for(i=0;i<n;i++){
    if(!fitdA) c[i].dA=0.;
    if(!fitdW) c[i].dW=0.;
    if(!fitddA) c[i].ddA=0.;
  }

  /* Build a starting reconstruction based on the passed-in initial
     chirp estimates */
  memset(y,0,sizeof(*y)*len);
  for(i=0;i<n;i++){
    for(j=0;j<len;j++){
      float jj = j-len*.5+.5;
      float a = c[i].A + (c[i].dA + c[i].ddA*jj)*jj;
      y[j] += a*cosf((c[i].W + c[i].dW*jj)*jj + c[i].P);
    }
  }

  if(!nonlinear){
    if(bound_zero) return -1;
    if(fit_W_alpha!=1.0) return -1;
    if(fit_dW_alpha!=1.0) return -1;
    iter_limit = linear_iterate(x,y,window,len,c,n,
                                fit_limit,iter_limit,fit_gs,
                                fitW,fitdA,fitdW,fitddA,
                                symm_norm,E0,E1,E2);
  }else{
    iter_limit = nonlinear_iterate(x,y,window,len,c,n,
                                   fit_limit,iter_limit,fit_gs,
                                   fitW,fitdA,fitdW,fitddA,
                                   nonlinear-1,fit_W_alpha,fit_dW_alpha,
                                   symm_norm,E0,E1,E2,
                                   bound_zero);
  }

  /* Sort by ascending frequency */
  qsort(c, n, sizeof(*c), cmp_ascending_W);

  return iter_limit;
}

/* advances/extrapolates the passed in chirps so that the params are
   centered forward in time by len samples. */

void advance_chirps(chirp *c, int n, int len){
  int i;
  for(i=0;i<n;i++){
    c[i].A += c[i].dA*len;
    c[i].P += (c[i].W+c[i].dW*len)*len;
    c[i].W += c[i].dW*len*2;
  }
}

/* dead, but leave it around; does a slow, brute force DCFT */
void slow_dcft_row(float *x, float *y, int k, int n){
  int i,j;
  for(i=0;i<n/2+1;i++){ /* frequency loop */
    float real=0.,imag=0.;
    for(j=0;j<n;j++){ /* multiply loop */
      float s,c;
      float jj = (j-n/2+.5)/n;
      sincosf(2.*M_PI*(i+k*jj)*jj,&s,&c);
      real += c*x[j];
      imag += s*x[j];
    }
    y[i] = hypot(real,imag);
  }
}

