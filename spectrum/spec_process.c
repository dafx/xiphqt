/*
 *
 *  gtk2 spectrum analyzer
 *
 *      Copyright (C) 2004-2012 Monty
 *
 *  This analyzer is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  The analyzer is distributed in the hope that it will be useful,
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

#include "analyzer.h"
#include "io.h"

char *det_entries[]={
  "V<span rise=\"-1000\" size=\"small\">peak</span> min/max",
  "V<span rise=\"-1000\" size=\"small\">peak</span>"
   "<span rise=\"4000\" size=\"x-small\">2</span> sum",
  "V<span rise=\"-1000\" size=\"small\">rms</span>"
   "<span rise=\"4000\" size=\"x-small\">2</span>/Hz avg",

  "V<span rise=\"-1000\" size=\"small\">rms</span>"
   "<span rise=\"4000\" size=\"x-small\">2</span> avg",
  "V<span rise=\"-1000\" size=\"small\">rms</span> avg",
  "log V<span rise=\"-1000\" size=\"small\">rms</span> avg",
  NULL};

/* when detector is set to 'video', run total mode as an average */
char *mode_entries[]={
  "realtime",
  "time maximum",
  "time average",
  "time sum",NULL};

char *bw_entries[]=
  {"FFT native",
   "1Hz","3Hz","10Hz","30Hz","100Hz",
   "octave/24","octave/12","octave/6","octave/3",NULL};
float bw_values[]=
  {0., 0., 1, 3, 10, 30, 100, -24., -12., -6., -3.};


static float *window=NULL;
static float *freqbuffer=0;
static float *refbuffer=0;

static fftwf_plan freqplan=NULL;

static int prev_total_ch=-1;

static pthread_mutex_t feedback_mutex=PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
static pthread_mutex_t bw_mutex=PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

static float *feedback_count=NULL;

static float **mag_acc=NULL;
static float **mag_max=NULL;
static float **mag_instant=NULL;

static float **phR_acc=NULL;
static float **phI_acc=NULL;
static float **phR_max=NULL;
static float **phI_max=NULL;
static float **phR_instant=NULL;
static float **phI_instant=NULL;
static float *phR_work=NULL;
static float *phI_work=NULL;

static float **xmappingL=NULL;
static float **xmappingM=NULL;
static float **xmappingH=NULL;
static int metascale = -1;
static int metawidth = -1;
static int metareload = 0;

sig_atomic_t acc_rewind=0;
sig_atomic_t acc_loop=0;

sig_atomic_t process_active=0;
sig_atomic_t process_exit=0;

static void init_process(void){
  int i,j,ch;

  if(!window || prev_total_ch != total_ch){

    if(freqplan)fftwf_destroy_plan(freqplan);

    if(mag_acc){
      for(i=0;i<prev_total_ch;i++)
        if(mag_acc[i])free(mag_acc[i]);
      free(mag_acc);
    }
    if(mag_max){
      for(i=0;i<prev_total_ch;i++)
        if(mag_max[i])free(mag_max[i]);
      free(mag_max);
    }
    if(mag_instant){
      for(i=0;i<prev_total_ch;i++)
        if(mag_instant[i])free(mag_instant[i]);
      free(mag_instant);
    }

    if(phR_acc){
      for(i=0;i<prev_total_ch;i++)
        if(phR_acc[i])free(phR_acc[i]);
      free(phR_acc);
    }
    if(phI_acc){
      for(i=0;i<prev_total_ch;i++)
        if(phI_acc[i])free(phI_acc[i]);
      free(phI_acc);
    }
    if(phR_max){
      for(i=0;i<prev_total_ch;i++)
        if(phR_max[i])free(phR_max[i]);
      free(phR_max);
    }
    if(phI_max){
      for(i=0;i<prev_total_ch;i++)
        if(phI_max[i])free(phI_max[i]);
      free(phI_max);
    }
    if(phR_instant){
      for(i=0;i<prev_total_ch;i++)
        if(phR_instant[i])free(phR_instant[i]);
      free(phR_instant);
    }
    if(phI_instant){
      for(i=0;i<prev_total_ch;i++)
        if(phI_instant[i])free(phI_instant[i]);
      free(phI_instant);
    }

    if(feedback_count)free(feedback_count);

    if(freqbuffer)free(freqbuffer);
    if(refbuffer)free(refbuffer);
    if(phR_work)free(phR_work);
    if(phI_work)free(phI_work);
    if(window)free(window);

    feedback_count=calloc(total_ch,sizeof(*feedback_count));

    mag_acc=calloc(total_ch,sizeof(*mag_acc));
    mag_max=calloc(total_ch,sizeof(*mag_max));
    mag_instant=calloc(total_ch,sizeof(*mag_instant));

    phR_acc=calloc(total_ch,sizeof(*phR_acc));
    phI_acc=calloc(total_ch,sizeof(*phI_acc));
    phR_max=calloc(total_ch,sizeof(*phR_max));
    phI_max=calloc(total_ch,sizeof(*phI_max));
    phR_instant=calloc(total_ch,sizeof(*phR_instant));
    phI_instant=calloc(total_ch,sizeof(*phI_instant));
    phR_work=calloc(blocksize/2+2,sizeof(*phR_work));
    phI_work=calloc(blocksize/2+2,sizeof(*phI_work));

    freqbuffer=fftwf_malloc((blocksize+2)*sizeof(*freqbuffer));
    refbuffer=fftwf_malloc((blocksize+2)*sizeof(*refbuffer));

    ch=0;
    for(i=0;i<inputs;i++){
      for(j=ch;j<ch+channels[i];j++){
        /* +2, not +1, to have a single 'one-past' value that is
           accessed but not used.  it simplifies the boundary
           condiitons in the bin_display code */
        mag_acc[j]=calloc(blocksize/2+2,sizeof(**mag_acc));
        mag_max[j]=calloc(blocksize/2+2,sizeof(**mag_max));
        mag_instant[j]=calloc(blocksize/2+2,sizeof(**mag_instant));
        if(j>ch){
          phR_acc[j]=calloc(blocksize/2+2,sizeof(**phR_acc));
          phR_max[j]=calloc(blocksize/2+2,sizeof(**phR_max));
          phR_instant[j]=calloc(blocksize/2+2,sizeof(**phR_instant));
          phI_acc[j]=calloc(blocksize/2+2,sizeof(**phI_acc));
          phI_max[j]=calloc(blocksize/2+2,sizeof(**phI_max));
          phI_instant[j]=calloc(blocksize/2+2,sizeof(**phI_instant));
        }
      }
      ch+=channels[i];
    }

    prev_total_ch = total_ch;

    freqplan=fftwf_plan_dft_r2c_1d(blocksize,freqbuffer,
                               (fftwf_complex *)freqbuffer,
                               FFTW_ESTIMATE);

    /* construct proper window (sin^4 I'd think) */
    window = calloc(blocksize,sizeof(*window));

    for(i=0;i<blocksize;i++)window[i]=sin(M_PIl*i/blocksize);
    for(i=0;i<blocksize;i++)window[i]*=window[i];
    for(i=0;i<blocksize;i++)window[i]=sin(window[i]*M_PIl*.5);
    for(i=0;i<blocksize;i++)window[i]*=window[i]/(blocksize/4)*.778;

  }
}

void rundata_clear(){
  int i;
  pthread_mutex_lock(&feedback_mutex);
  for(i=0;i<total_ch;i++){
    feedback_count[i]=0;
    memset(mag_acc[i],0,(blocksize/2+1)*sizeof(**mag_acc));
    memset(mag_max[i],0,(blocksize/2+1)*sizeof(**mag_max));
    memset(mag_instant[i],0,(blocksize/2+1)*sizeof(**mag_instant));
    if(phR_acc[i])
      memset(phR_acc[i],0,(blocksize/2+1)*sizeof(**phR_acc));
    if(phI_acc[i])
      memset(phI_acc[i],0,(blocksize/2+1)*sizeof(**phI_acc));
    if(phR_max[i])
      memset(phR_max[i],0,(blocksize/2+1)*sizeof(**phR_max));
    if(phI_max[i])
      memset(phI_max[i],0,(blocksize/2+1)*sizeof(**phI_max));
    if(phR_instant[i])
      memset(phR_instant[i],0,(blocksize/2+1)*sizeof(**phR_instant));
    if(phI_instant[i])
    memset(phI_instant[i],0,(blocksize/2+1)*sizeof(**phI_instant));
  }
  pthread_mutex_unlock(&feedback_mutex);
}

static int bandwidth_choice=0;
//static float bandwidth=-1;
static int feedback_bandwidth=0;

extern GtkWidget *clearbutton;
void set_bandwidth(int bw){

  pthread_mutex_lock(&bw_mutex);
  bandwidth_choice=bw;
  //compute_bwfilter();
  //compute_bandwidth();

  pthread_mutex_lock(&feedback_mutex);
  feedback_bandwidth=bw;
  rundata_clear();
  //accumulate_feedback();

  pthread_mutex_unlock(&feedback_mutex);
  pthread_mutex_unlock(&bw_mutex);

}

static void process(void){
  /* by channel */
  int i,j,fi,ch=0;
  for(fi=0;fi<inputs;fi++){
    if(blockbuffernew[fi]){
      blockbuffernew[fi]=0;
      for(i=ch;i<ch+channels[fi];i++){

        pthread_mutex_lock(&bw_mutex);

	float *data=blockbuffer[i];
	for(j=0;j<blocksize;j++){
	  freqbuffer[j]=window[j]*data[j];
	}

	/* transform */
        fftwf_execute(freqplan);

        if(i==ch)
          memcpy(refbuffer,freqbuffer,(blocksize+2)*sizeof(*refbuffer));

        if(i>ch){
          for(j=0;j<blocksize+2;j+=2){
            float R = freqbuffer[j];
            float I = freqbuffer[j+1];
            float rR = refbuffer[j];
            float rI = refbuffer[j+1];
            freqbuffer[j>>1] = R*R+I*I;
            phR_work[j>>1] = (rR*R + rI*I);
            phI_work[j>>1] = (rR*I - rI*R);
          }
        }else{
          float acc=0.;
          for(j=0;j<blocksize+2;j+=2){
            float R = refbuffer[j];
            float I = refbuffer[j+1];
            acc+=freqbuffer[j>>1] = R*R+I*I;
          }
        }

        if(bandwidth_choice>1){
          /* alters buffers in-place */


        }

	pthread_mutex_lock(&feedback_mutex);

        /* perform desired accumulations */
        if(i>ch){

          for(j=0;j<blocksize/2+1;j++){

            mag_instant[i][j]=freqbuffer[j];
            mag_acc[i][j]+=freqbuffer[j];
	    phR_acc[i][j]+=phR_work[j];
	    phI_acc[i][j]+=phI_work[j];
            if(mag_max[i][j]<freqbuffer[j]){
              mag_max[i][j]=freqbuffer[j];
	      phR_max[i][j]=phR_work[j];
	      phI_max[i][j]=phI_work[j];
            }
          }

          /* swap instant, don't copy */
          float *temp = phR_work;
          phR_work = phR_instant[i];
          phR_instant[i] = temp;

          temp = phI_work;
          phI_work = phI_instant[i];
          phI_instant[i] = temp;

        }else{
          for(j=0;j<blocksize/2+1;j++){

            mag_instant[i][j]=freqbuffer[j];
            mag_acc[i][j]+=freqbuffer[j];
            if(mag_max[i][j]<freqbuffer[j])
              mag_max[i][j]=freqbuffer[j];
          }

	}
	feedback_count[i]++;

	pthread_mutex_unlock(&feedback_mutex);
        pthread_mutex_unlock(&bw_mutex);
      }
    }
    ch+=channels[fi];
  }
}

void *process_thread(void *dummy){
  int ret;
  pthread_mutex_lock(&feedback_mutex);
  init_process();
  pthread_mutex_unlock(&feedback_mutex);

  while(!process_exit){

    if(acc_rewind) rewind_files();
    acc_rewind=0;

    ret=input_read(acc_loop,0);
    if(ret==0) break;
    if(ret==-1){
      /* a pipe returned EOF; attempt reopen */
      pthread_mutex_lock(&feedback_mutex);
      if(pipe_reload()){
        init_process();
        rundata_clear();
        metareload=1;
        pthread_mutex_unlock(&feedback_mutex);
        write(eventpipe[1],"",1);
        continue;
      }else{
        pthread_mutex_unlock(&feedback_mutex);
        break;
      }
    }

    process();
    write(eventpipe[1],"",1);
  }

  /* eof on all inputs */
  process_active=0;
  write(eventpipe[1],"",1);
  return NULL;
}

void process_dump(int mode){
  int fi,i,j,ch;
  FILE *out;

  pthread_mutex_lock(&feedback_mutex);

  out=fopen("accumulate.m","w");
  ch = 0;
  for(fi=0;fi<inputs;fi++){
    for(i=0;i<blocksize/2+1;i++){
      fprintf(out,"%f ",(double)i*rate[fi]/blocksize);

      for(j=ch;j<ch+channels[fi];j++)
        fprintf(out,"%f ",todB(mag_acc[j][i])*.5);
      fprintf(out,"\n");
    }
    fprintf(out,"\n");
    ch+=channels[fi];
  }
  fclose(out);

  out=fopen("max.m","w");
  ch = 0;
  for(fi=0;fi<inputs;fi++){
    for(i=0;i<blocksize/2+1;i++){
      fprintf(out,"%f ",(double)i*rate[fi]/blocksize);

      for(j=ch;j<ch+channels[fi];j++)
        fprintf(out,"%f ",todB(mag_max[j][i])*.5);
      fprintf(out,"\n");
    }
    fprintf(out,"\n");
    ch+=channels[fi];
  }
  fclose(out);

  out=fopen("instant.m","w");
  ch = 0;
  for(fi=0;fi<inputs;fi++){
    for(i=0;i<blocksize/2+1;i++){
      fprintf(out,"%f ",(double)i*rate[fi]/blocksize);

      for(j=ch;j<ch+channels[fi];j++)
        fprintf(out,"%f ",todB(mag_instant[j][i])*.5);
      fprintf(out,"\n");
    }
    fprintf(out,"\n");
    ch+=channels[fi];
  }
  fclose(out);

  out=fopen("accphase.m","w");
  ch = 0;
  for(fi=0;fi<inputs;fi++){

    /* phase */
    for(i=0;i<blocksize+2;i+=2){
      fprintf(out,"%f ",(double)i*.5*rate[fi]/blocksize);
      fprintf(out,"%f ",atan2(phI_acc[ch+1][i>>1],phR_acc[ch+1][i>>1])*57.29);
      fprintf(out,"\n");
    }
    fprintf(out,"\n");
    ch+=channels[fi];
  }
  fclose(out);

  pthread_mutex_unlock(&feedback_mutex);
}

/* takes V^2, returns dBV */
static void bin_minmax(float *in, float *out, float *ymax,
                       int width, float norm,
                       float *L, float *M, float *H){
  int i,j;
  int prevbin;
  float prevy;
  float dBnorm = todB(norm);

  for(i=0;i<width;i++){
    int first=floor(L[i]);
    int last=floor(H[i]);
    float firsty,lasty,min,max;

    /* don't allow roundoff error to skip a bin entirely */
    if(i>0 && prevbin<first)first=prevbin;
    prevbin=last;

    if(first==last){
      /* interpolate between two bins */
      float m = (H[i]+L[i])*.5-first;
      float del = H[i]-L[i];
      firsty=lasty=min=max =
        (todB(in[first])*(1.-m)+todB(in[first+1])*m+dBnorm)*.5;
    }else{
      firsty=lasty=min=max=in[first+1];
      for(j=first+2;j<last;j++){
        if(in[j]<min)min=in[j];
        if(in[j]>max)max=in[j];
        lasty=in[j-1];
      }
      lasty=(todB(lasty)+dBnorm)*.5;
      firsty=(todB(firsty)+dBnorm)*.5;
      min=(todB(min)+dBnorm)*.5;
      max=(todB(max)+dBnorm)*.5;
    }

    if(max>*ymax)*ymax=max;

    /* link non-overlapping bins into contiguous lines */
    if(i>0){
      float midpoint = (prevy+firsty)*.5;

      if(midpoint<min)min=midpoint;
      if(midpoint>max)max=midpoint;

      if(midpoint<out[i*2-2])out[i*2-2]=midpoint;
      if(midpoint>out[i*2-1])out[i*2-1]=midpoint;
    }

    out[i*2]=min;
    out[i*2+1]=max;

    prevy=lasty;
  }
}

static float aadd(float a, float b){
  double c = (double)a+b+M_PI;
  double d = floor(c/(2*M_PI));
  return (c - d*2*M_PI - M_PI);
}

static void ph_minmax(float *R, float *I, float *out,
                      float *pmin, float *pmax, int width, float adj,
                      float *L, float *M, float *H){
  int i,j;
  int prevbin;
  float prevmin;
  float prevmax;

  for(i=0;i<width;i++){
    int first=floor(L[i]);
    int last=floor(H[i]);
    float min,max;

    /* don't allow roundoff error to skip a bin entirely */
    if(i>0 && prevbin<first)first=prevbin;
    prevbin=last;

    if(first==last){
      float m = (H[i]+L[i])*.5-first;
      float aP = aadd(atan2f(I[first],R[first]),adj*first);
      float bP = aadd(atan2f(I[first+1],R[first+1]),adj*(first+1));
      min=max = (aP+(bP-aP)*m)*(360/M_PI/2);
    }else{
      int min_i,max_i;
      min=max =  fast_atan_cmp(I[first+1],R[first+1]);
      min_i=max_i = first+1;
      for(j=first+2;j<last;j++){
        float P = fast_atan_cmp(I[j],R[j]);
        if(P<min){
          min=P;
          min_i=j;
        }
        if(P>max){
          max=P;
          max_i=j;
        }
      }

      min=aadd(atan2f(I[min_i],R[min_i]),adj*min_i)*(360/M_PI/2);
      if(max_i==min_i)
        max=min;
      else
        max=aadd(atan2f(I[max_i],R[max_i]),adj*max_i)*(360/M_PI/2);
    }

    if(max>*pmax)*pmax=max;
    if(min<*pmin)*pmin=min;

    /* link non-overlapping bins into contiguous lines */
    if(i>0){
      if(prevmin>max){
        float midpoint = (prevmin+max)*.5;
        out[i*2-2]=midpoint;
        max=midpoint;
      }else if(prevmax<min){
        float midpoint = (prevmax+min)*.5;
        out[i*2-1]=midpoint;
        min=midpoint;
      }
    }

    out[i*2]=min;
    out[i*2+1]=max;

    prevmin=min;
    prevmax=max;
  }
}

/* input is always V^2 */
/* detector mode 1: sum */
/* detector mode 2: rms/Hz average */
/* detector mode 3: rms average */
/* detector mode 4: mag average */
/* detector mode 5: log average */
static void bin_display(float *di, float *out, float *ymax,
                        int det, int width, float norm,
                        float *L, float *M, float *H, int relative){
  int i,j;
  int prevbin;
  float prevy;
  float dBnorm = todB(norm)*.5;
  float *in = di;

  if(det==DET_LINEAR){
    in=alloca(sizeof(*in)*(blocksize/2+1));
    for(i=0;i<blocksize/2+1;i++)
      in[i]=sqrtf(di[i]);
  }
  if(det==DET_LOG){
    in=alloca(sizeof(*in)*(blocksize/2+1));
    for(i=0;i<blocksize/2+1;i++)
      in[i]=todB(di[i])*.5;
  }
  if(det==DET_SUM && relative){
    /* in this case, the equivalent of summing the r/p would be a
       power average once we get here */
    det=DET_RMS;
  }
  if(det!=DET_SUM && !relative)
    dBnorm += -3.0103; /* adjust from Vpk to Vrms, but not in relative
                          mode (correction only meaningful with absolute
                          values) */

  for(i=0;i<width;i++){
    int first=floor(L[i]);
    int last=floor(H[i]);
    float min,max,sum=0.,sdel=0.;

    /* don't allow roundoff error to skip a bin entirely */
    if(i>0 && prevbin<first)first=prevbin;
    prevbin=last;

    if(first==last){
      float m = (H[i]+L[i])*.5-first;
      sdel = H[i]-L[i];
      if(det==DET_LOG)
        sum = (in[first]*(1.-m) + in[first+1]*m)*sdel;
      else{
        /* this is still a division of energy, but perform a visually
           pleasing interpolation */
        sum = fromdB(todB(in[first])*(1.-m) + todB(in[first+1])*m)*sdel;
      }
    }else{
      float del = first+1-L[i];
      float m = 1.-del*.5;

      sdel = del;
      sum = (in[first]*(1.-m) + in[first+1]*m)*del;

      if(first+1<last){
        sum+=in[first+1]*.5;
        sdel+=.5;

        for(j=first+2;j<last;j++){
          sum+=in[j];
          sdel+=1.;
        }

        sum+=in[j]*.5;
        sdel+=.5;
      }

      sdel += del = H[i]-last;
      m = del*.5;
      sum += (in[last]*(1.-m) + in[last+1]*m)*del;
    }

    switch(det){
    case DET_SUM:
      sum = todB(sum)*.5+dBnorm;
      break;
    case DET_DENSITY:
    case DET_RMS:
      sum = todB(sum/sdel)*.5+dBnorm;
      break;
    case DET_LINEAR:
      sum = todB(sum/sdel)+dBnorm;
      break;
    case DET_LOG:
      sum = sum/sdel+dBnorm;
      break;
    }

    min=max=sum;

    if(sum>*ymax)*ymax=sum;

    /* link non-overlapping bins into contiguous lines */
    if(i>0){
      float midpoint = (prevy+sum)*.5;
      if(midpoint<min)min=midpoint;
      if(midpoint>max)max=midpoint;

      if(midpoint<out[i*2-2])out[i*2-2]=midpoint;
      if(midpoint>out[i*2-1])out[i*2-1]=midpoint;
    }

    out[i*2]=min;
    out[i*2+1]=max;

    prevy=sum;
  }
}

static void ph_display(float *iR, float *iI, float *out,
                       float *pmin, float *pmax, int width,
                       float *L, float *M, float *H){
  int i,j;
  int prevbin;
  float prevP;

  for(i=0;i<width;i++){
    int first=floor(L[i]);
    int last=floor(H[i]);
    float min,max,R,I,P;

    /* don't allow roundoff error to skip a bin entirely */
    if(i>0 && prevbin<first)first=prevbin;
    prevbin=last;

    if(first==last){
      float m = (H[i]+L[i])*.5-first;
      R = (iR[first]*(1.-m) + iR[first+1]*m);
      I = (iI[first]*(1.-m) + iI[first+1]*m);
    }else{
      float del = first+1-L[i];
      float m = 1.-del*.5;

      R = (iR[first]*(1.-m) + iR[first+1]*m)*del;
      I = (iI[first]*(1.-m) + iI[first+1]*m)*del;

      if(first+1<last){
        R+=iR[first+1]*.5;
        I+=iI[first+1]*.5;

        for(j=first+2;j<last;j++){
          R+=iR[j];
          I+=iI[j];
        }

        R+=iR[j]*.5;
        I+=iI[j]*.5;
      }

      del = H[i]-last;
      m = del*.5;
      R += (iR[last]*(1.-m) + iR[last+1]*m)*del;
      I += (iI[last]*(1.-m) + iI[last+1]*m)*del;
    }

    min=max=P = atan2f(I,R)*(360/2/M_PI);

    if(max>*pmax)*pmax=max;
    if(min<*pmin)*pmin=min;

    /* link non-overlapping bins into contiguous lines */
    if(i>0){
      float midpoint = (prevP+P)*.5;
      if(midpoint<min)min=midpoint;
      if(midpoint>max)max=midpoint;

      if(midpoint<out[i*2-2])out[i*2-2]=midpoint;
      if(midpoint>out[i*2-1])out[i*2-1]=midpoint;
    }

    out[i*2]=min;
    out[i*2+1]=max;

    prevP=P;
  }
}

static void ph_display_delay(float *iR, float *iI, float *out,
                             float *pmin, float *pmax, int width, float adj,
                             float *L, float *M, float *H){
  int i,j;
  int prevbin;
  float prevP;

  /* perform a progressive partial rotation across the bin */
  float unitR = cos(adj);
  float unitI = sin(adj);

  for(i=0;i<width;i++){
    int first=floor(L[i]);
    int last=floor(H[i]);
    float min,max,R,I,P;

    /* don't allow roundoff error to skip a bin entirely */
    if(i>0 && prevbin<first)first=prevbin;
    prevbin=last;

    if(first==last){
      float m = (H[i]+L[i])*.5-first;
      float rotR = iR[first]*(1.-m);
      float rotI = iI[first]*(1.-m);

      R = unitR*rotR + unitI*rotI + iR[first+1]*m;
      I = unitR*rotI - unitI*rotR + iI[first+1]*m;

    }else{
      float del = first+1-L[i];
      float m = 1.-del*.5;
      float rotR = iR[first]*(1.-m);
      float rotI = iI[first]*(1.-m);

      R = (unitR*rotR + unitI*rotI + iR[first+1]*m)*del;
      I = (unitR*rotI - unitI*rotR + iI[first+1]*m)*del;

      if(first+1<last){
        R += iR[first+1]*.5;
        I += iI[first+1]*.5;

        for(j=first+2;j<last;j++){
          rotR = (unitR*R + unitI*I);
          rotI = (unitR*I - unitI*R);

          R = rotR+iR[j];
          I = rotI+iI[j];
        }

        rotR = (unitR*R + unitI*I);
        rotI = (unitR*I - unitI*R);

        R = rotR+iR[j]*.5;
        I = rotI+iI[j]*.5;
      }

      del = H[i]-last;
      m = del*.5;

      R += iR[last]*(1.-m)*del;
      I += iI[last]*(1.-m)*del;

      rotR = (unitR*R + unitI*I);
      rotI = (unitR*I - unitI*R);

      R = rotR + iR[last+1]*m*del;
      I = rotI + iI[last+1]*m*del;
    }

    min=max=P = aadd(atan2f(I,R),adj*(last+1))*(360/M_PI/2);

    if(max>*pmax)*pmax=max;
    if(min<*pmin)*pmin=min;

    /* link non-overlapping bins into contiguous lines */
    if(i>0){
      float midpoint = (prevP+P)*.5;
      if(midpoint<min)min=midpoint;
      if(midpoint>max)max=midpoint;

      if(midpoint<out[i*2-2])out[i*2-2]=midpoint;
      if(midpoint>out[i*2-1])out[i*2-1]=midpoint;
    }

    out[i*2]=min;
    out[i*2+1]=max;

    prevP=P;
  }
}

void mag_to_display(float *in, float *out, float *ymax,
                    int fi, int width, float norm, int det, int relative){
  if(det){
    if(det==DET_DENSITY && !relative) norm*=(float)blocksize/rate[fi];
    bin_display(in, out, ymax, det, width, norm,
                xmappingL[fi], xmappingM[fi], xmappingH[fi], relative);
  }else{
    bin_minmax(in, out, ymax, width, norm,
               xmappingL[fi], xmappingM[fi], xmappingH[fi]);
  }
}

void phase_to_display(float *I, float *Q, float *out,
                      float *pmin, float *pmax,
                      int fi, int width, int det,
                      float norm){

  if(det){ /* display averaging */
    if(norm != 0.){
      ph_display_delay(I, Q, out, pmin, pmax, width, norm,
                 xmappingL[fi], xmappingM[fi], xmappingH[fi]);
    }else{
      ph_display(I, Q, out, pmin, pmax, width,
                 xmappingL[fi], xmappingM[fi], xmappingH[fi]);
    }
  }else{
    ph_minmax(I, Q, out, pmin, pmax, width, norm,
              xmappingL[fi], xmappingM[fi], xmappingH[fi]);
  }
}

int ascfloat(const void *a, const void *b){
  float A = *(float *)a;
  float B = *(float *)b;
  if(A<B)return -1;
  if(A>B)return 1;
  return 0;
}

float norm_calc(float *data,int n,int rate){
  /* normalized to median value calculated from 1kHz to 1/2 nyquist */
  int i;
  int x0 = 100.*blocksize/rate;
  int x1 = blocksize*3/8;
  float sum=0.;

  for(i=x0;i<x1;i++)
    sum += data[i];
  return -todB(sum/(x1-x0))*.5;
}

float norm_ph(float *dataR, float *dataI,int n,int rate){
  int i,j;
  int x0 = 100.*blocksize/rate;
  int x1 = x0+blocksize/4;
  int delcount=0, revcount=0;
  float delsum=0.;

  float sumR=0.;
  float sumI=0.;
  for(i=x0+1;i<x1;i++){
    sumR += dataR[i-1]*dataR[i] + dataI[i-1]*dataI[i];
    sumI += dataR[i-1]*dataI[i] - dataI[i-1]*dataR[i];
  }

  return -atan2f(sumI,sumR);
}

/* how many bins to 'trim' off the edge of calculated data when we
   know we've hit a boundary of marginal measurement */
#define binspan 5

static fetchdata fetch_ret;

/* the data returned is now 2 vals per bin; a min and a max.  The spec
   plot merely draws a vertical line between. */
fetchdata *process_fetch(int scale, int mode, int link, int det,
                         int *process_in, Plot *plot){
  int ch,ci,i,j,fi;
  float **data;
  float **phR;
  float **phI;
  float *normptr;
  float maxrate=-1.;
  float nyq;
  int *process;
  int width=-1;

  pthread_mutex_lock(&feedback_mutex);
  init_process();
  process = alloca(total_ch*sizeof(*process));

  if(total_ch!=fetch_ret.total_ch){
    if(fetch_ret.data){
      for(i=0;i<fetch_ret.total_ch;i++)
        if(fetch_ret.data[i])free(fetch_ret.data[i]);
      free(fetch_ret.data);
      fetch_ret.data=NULL;
    }
    if(fetch_ret.active){
      free(fetch_ret.active);
      fetch_ret.active=NULL;
    }
  }

  if(!fetch_ret.data)
    fetch_ret.data = calloc(total_ch,sizeof(*fetch_ret.data));

  if(!fetch_ret.active)
    fetch_ret.active = calloc(total_ch,sizeof(*fetch_ret.active));

  /* the passed in process array doesn't necesarily match the
     current channel structure.  Copy group by group. */
  {
    int ch_now=0;
    int ch_in=0;
    for(i=0;i<inputs;i++){
      int ci;
      for(ci=0;ci<channels[i] && ci<fetch_ret.channels[i];ci++)
        process[ch_now+ci] = process_in[ch_in+ci];
      for(;ci<channels[i];ci++)
        process[ch_now+ci] = 0;
      ch_now+=channels[i];
      ch_in+=fetch_ret.channels[i];
    }

    memcpy(fetch_ret.active,process,total_ch*sizeof(*process));
  }

  fetch_ret.phase_active=0;
  if(link == LINK_PHASE || link == LINK_NORMPHASE){
    int cho=0;
    int gi;
    for(gi=0;gi<inputs;gi++)
      if(channels[gi]>1 && fetch_ret.active[cho+1]){
        fetch_ret.phase_active=1;
        break;
      }
  }

  fetch_ret.groups=inputs;
  fetch_ret.scale=scale;
  fetch_ret.mode=mode;
  fetch_ret.link=link;
  memset(&fetch_ret.dBnorm,0,sizeof(fetch_ret.dBnorm));
  memset(&fetch_ret.phdelay,0,sizeof(fetch_ret.phdelay));

  fetch_ret.height=plot_height(plot);
  fetch_ret.width=width=plot_width(plot,fetch_ret.phase_active);
  fetch_ret.total_ch=total_ch;

  for(fi=0;fi<inputs;fi++)
    if(rate[fi]>maxrate)maxrate=rate[fi];

  memcpy(fetch_ret.bits,bits,sizeof(fetch_ret.bits));
  memcpy(fetch_ret.channels,channels,sizeof(fetch_ret.channels));
  memcpy(fetch_ret.rate,rate,sizeof(fetch_ret.rate));

  nyq=maxrate/2.;
  fetch_ret.maxrate=maxrate;
  fetch_ret.reload=metareload;
  metareload=0;

  /* are our scale mappings up to date? */
  if(scale != metascale || width != metawidth || fetch_ret.reload){
    if(!xmappingL) xmappingL = calloc(inputs, sizeof(*xmappingL));
    if(!xmappingM) xmappingM = calloc(inputs, sizeof(*xmappingM));
    if(!xmappingH) xmappingH = calloc(inputs, sizeof(*xmappingH));

    for(fi=0;fi<inputs;fi++){

      /* if mapping preexists, resize it */
      if(xmappingL[fi]){
	xmappingL[fi] = realloc(xmappingL[fi],(width+1)*sizeof(**xmappingL));
      }else{
	xmappingL[fi] = malloc((width+1)*sizeof(**xmappingL));
      }
      if(xmappingM[fi]){
	xmappingM[fi] = realloc(xmappingM[fi],(width+1)*sizeof(**xmappingM));
      }else{
	xmappingM[fi] = malloc((width+1)*sizeof(**xmappingM));
      }
      if(xmappingH[fi]){
	xmappingH[fi] = realloc(xmappingH[fi],(width+1)*sizeof(**xmappingH));
      }else{
	xmappingH[fi] = malloc((width+1)*sizeof(**xmappingH));
      }

      metascale = scale;
      metawidth = width;

      /* generate new numbers */
      for(i=0;i<width;i++){
	float off=.5;
	float loff=1.;
	float hoff=1.;
	float lfreq,mfreq,hfreq;

        off=.5;

	switch(scale){
	case 0: /* log */
	  lfreq= pow(10.,(i-off)/(width-1)
		     * (log10(nyq)-log10(5.))
		     + log10(5.)) * loff;
	  mfreq= pow(10.,((float)i)/(width-1)
		     * (log10(nyq)-log10(5.))
		     + log10(5.));
	  hfreq= pow(10.,(i+off)/(width-1)
		     * (log10(nyq)-log10(5.))
		     + log10(5.)) * hoff;
	  break;
	case 1: /* ISO */
	  lfreq= pow(2.,(i-off)/(width-1)
		     * (log2(nyq)-log2(25.))
		     + log2(25.)) * loff;
	  mfreq= pow(2.,((float)i)/(width-1)
		     * (log2(nyq)-log2(25.))
		     + log2(25.));
	  hfreq= pow(2.,(i+off)/(width-1)
		     * (log2(nyq)-log2(25.))
		     + log2(25.)) *hoff;
	  break;
	case 2: /* screen-resolution linear */
	  lfreq=(i-off)*nyq/(width-1)*loff;
	  mfreq=((float)i)*nyq/(width-1);
	  hfreq=(i+off)*nyq/(width-1)*hoff;
	  break;
	}

	xmappingL[fi][i]=lfreq/(rate[fi]*.5)*(blocksize/2);
	xmappingM[fi][i]=mfreq/(rate[fi]*.5)*(blocksize/2);
	xmappingH[fi][i]=hfreq/(rate[fi]*.5)*(blocksize/2);

      }

      for(i=0;i<width;i++){
	if(xmappingL[fi][i]<0.)xmappingL[fi][i]=0.;
	if(xmappingL[fi][i]>blocksize/2.)xmappingL[fi][i]=blocksize/2.;
	if(xmappingM[fi][i]<0.)xmappingM[fi][i]=0.;
	if(xmappingM[fi][i]>blocksize/2.)xmappingM[fi][i]=blocksize/2.;
	if(xmappingH[fi][i]<0.)xmappingH[fi][i]=0.;
	if(xmappingH[fi][i]>blocksize/2.)xmappingH[fi][i]=blocksize/2.;
      }
    }

    for(i=0;i<total_ch;i++){
      if(fetch_ret.data[i]){
	fetch_ret.data[i] = realloc
          (fetch_ret.data[i],(width+1)*2*sizeof(**fetch_ret.data));
      }else{
	fetch_ret.data[i] = malloc((width+1)*2*sizeof(**fetch_ret.data));
      }
    }
  }

  /* mode selects the base data set */
  normptr=NULL;
  switch(mode){
  case MODE_REALTIME: /* independent / instant */
    data=mag_instant;
    phR=phR_instant;
    phI=phI_instant;
    break;
  case MODE_MAX: /* independent / max */
    data=mag_max;
    phR=phR_max;
    phI=phI_max;
    break;
  case MODE_TOTAL: /* independent / accumulate */
    data=mag_acc;
    phR=phR_acc;
    phI=phI_acc;
    break;
  case MODE_AVERAGE: /* independent / average */
    data=mag_acc;
    phR=phR_acc;
    phI=phI_acc;
    normptr=feedback_count;
    break;
  }

  ch=0;
  fetch_ret.ymax = -210.;
  fetch_ret.pmax = -180.;
  fetch_ret.pmin = 180.;

  for(fi=0;fi<inputs;fi++){
    float normalize = (normptr && normptr[fi]) ? 1./normptr[fi] : 1.;

    switch(link){
    case LINK_INDEPENDENT:

      for(ci=0;ci<channels[fi];ci++){
	if(process[ch+ci]){
          mag_to_display(data[ch+ci], fetch_ret.data[ch+ci],
                          &fetch_ret.ymax,
                         fi, width, normalize, det, 0);
	}
      }
      break;

    case LINK_SUMMED:

      /* display first channel, but only if any channels in the group
         are processed */
      /* this is a summation of the input signals, not a sum of
         manitude or power spectrum.  It operates by summing
         equivalents of the FFTs of the original inputs (equivalent to
         summing the time-domain signals).  We can't recover the
         original FFT of channel 0, but the sqrt of the power spectrum
         is the equivalent with each sinusoid rotated to purely
         positive-real.  The ph arrays store the subsequent channels
         rotated relative to channel 0 (multiplied by ch 0's complex
         conjugate), so the fact that we can't recover the original
         channel 0 phase doesn't affect the final answer */

      {
        int any=0;
        float *y = fetch_ret.data[ch];
        float work[blocksize/2+1];

        for(j=0;j<blocksize/2+1;j++)
          work[j]=sqrtf(data[ch][j]);

        for(j=0;j<blocksize/2+1;j++){
          float R=0.;
          float I=0.;
          for(i=ch;i<ch+channels[fi];i++){
            if(fetch_ret.active[i]){
              if(ch==i){
                R+=work[j];
              }else{
                R += phR[i][j] / work[j];
                I += phR[i][j] / work[j];
              }
            }
          }
          work[j]=R*R+I*I;
        }
        fetch_ret.active[ch]=1;
        for(i=ch+1;i<ch+channels[fi];i++)
          fetch_ret.active[i]=0;
        mag_to_display(work, y, &fetch_ret.ymax,
                       fi, width, normalize, det, 0);
      }
      break;

    case LINK_PHASE: /* response/phase */
    case LINK_NORMPHASE:

      for(i=ch+2;i<ch+channels[fi];i++)
        fetch_ret.active[i]=0;

      if(channels[fi]>=2){
        if(process[ch]){
          /* response */
          float work[blocksize/2+1];
          float *r = data[ch];
          float *m = data[ch+1];
          float ymax = -210.f;
          for(j=0;j<blocksize/2+1;j++)
            work[j]=m[j]/r[j];

          if(link == LINK_NORMPHASE)
            fetch_ret.dBnorm[fi] =
              norm_calc(work,blocksize/2+1,rate[fi]);

          mag_to_display(work, fetch_ret.data[ch], &ymax,
                         fi, width, 1, det, 1);

          if(link == LINK_NORMPHASE){
            for(j=0;j<width*2+2;j++)
              fetch_ret.data[ch][j]+=fetch_ret.dBnorm[fi];
            ymax+=fetch_ret.dBnorm[fi];
          }

          if(ymax>fetch_ret.ymax)fetch_ret.ymax=ymax;

        }
        if(process[ch+1]){
          /* phase */
          double norm =
            (link == LINK_NORMPHASE ?
             norm_ph(phR[ch+1], phI[ch+1], blocksize/2+1,rate[fi]):
             0.);

          phase_to_display(phR[ch+1], phI[ch+1], fetch_ret.data[ch+1],
                           &fetch_ret.pmin, &fetch_ret.pmax,
                           fi, width, det, norm);

          fetch_ret.phdelay[fi] = norm*blocksize/(2.*M_PI*rate[fi]);
        }
      }
      break;
    }
    ch+=channels[fi];
  }

  pthread_mutex_unlock(&feedback_mutex);
  return &fetch_ret;
}
