/*
 *
 *     sushivision copyright (C) 2006-2007 Monty <monty@xiph.org>
 *
 *  sushivision is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  sushivision is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with sushivision; see the file COPYING.  If not, write to the
 *  Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * 
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "internal.h"

/* modules attempts to encapsulate and hide differences between the
   different types of dimensions with respect to widgets, iterators,
   shared dimensions */

/* A panel that supports discrete dimensions must deal with up to
   three different scales for the discrete axis; the first is
   pixel-oriented to the actually displayed panel, the second covers
   the same range but is oriented to integer bin numbers in the
   underlying data vector (often the same as the display), and the
   third the same as the second, but over the absolute range [0 - n)
   such that discrete dimensions will count from 0 in iteration. */
/* data_w ignored except in the continuous case, where it may be used
   to generate linked or over/undersampled data scales. */
int _sushiv_dimension_scales(sushiv_dimension_t *d,
			     double lo,
			     double hi,
			     int panel_w, int data_w,
			     int spacing,
			     char *legend,
			     scalespace *panel, 
			     scalespace *data, 
			     scalespace *iter){

  switch(d->type){
  case SUSHIV_DIM_CONTINUOUS:
    {
      double fl = ((d->flags & SUSHIV_DIM_ZEROINDEX) ? d->scale->val_list[0] : 0.);
      *panel = scalespace_linear(lo, hi, panel_w, spacing, legend);
      *data = scalespace_linear(lo, hi, data_w, spacing, legend);
      *iter = scalespace_linear(lo-fl, hi-fl, data_w, spacing, legend);
    }
    break;
  case SUSHIV_DIM_DISCRETE:
    {
      /* return a scale that when iterated will only hit values
	 quantized to the discrete base */
      /* what is the absolute base? */
      int floor_i =  rint(d->scale->val_list[0] * d->private->discrete_denominator / 
			  d->private->discrete_numerator);
      int ceil_i =  rint(d->scale->val_list[d->scale->vals-1] * d->private->discrete_denominator / 
			 d->private->discrete_numerator);
      
      int lo_i =  floor(lo * d->private->discrete_denominator / 
			d->private->discrete_numerator);
      int hi_i =  floor(hi * d->private->discrete_denominator / 
		       d->private->discrete_numerator);

      /* the panel scales may be reversed (the easiest way to do y)
	 and/or the dimension may have an inverted scale. */
      int pneg, dimneg;

      if(lo_i>hi_i){ // == must be 1 to match scale gen code when width is 0
	pneg = -1;
      }else{
	pneg = 1;
      }

      if(d->scale->val_list[0] > d->scale->val_list[d->scale->vals-1]){
	dimneg = -1;
      }else{
	dimneg = 1;
      }
      
      if(floor_i < ceil_i){
	if(lo_i < floor_i)lo_i = floor_i;
	if(hi_i > ceil_i)hi_i = ceil_i;
      }else{
	if(lo_i > floor_i)lo_i = floor_i;
	if(hi_i < ceil_i)hi_i = ceil_i;
      }

      *panel = scalespace_linear((double)(lo_i-pneg*.4) * d->private->discrete_numerator / 
				 d->private->discrete_denominator,
				 (double)(hi_i+pneg*.4) * d->private->discrete_numerator / 
				 d->private->discrete_denominator,
				 panel_w, spacing, legend);

      /* if possible, the data/iterator scales should cover the entire pane exposed
	 by the panel scale so long as there's room left to extend them without
	 overflowing the lo/hi fenceposts */
      double panel1 = scalespace_value(panel,0)*pneg;
      double panel2 = scalespace_value(panel,panel->pixels)*pneg;
      double data1 = (double)(lo_i-.49*pneg) * d->private->discrete_numerator / 
	d->private->discrete_denominator*pneg;
      double data2 = (double)(hi_i-.51*pneg) * d->private->discrete_numerator / 
	d->private->discrete_denominator*pneg;

      while(data1 > panel1 && lo_i*dimneg > floor_i*dimneg){
	lo_i -= pneg;
	data1 = (double)(lo_i-.49*pneg) * d->private->discrete_numerator / 
	  d->private->discrete_denominator*pneg;
      }

      while(data2 < panel2 && hi_i*dimneg <= ceil_i*dimneg){ // inclusive upper
	hi_i += pneg;
	data2 = (double)(hi_i-.51*pneg) * d->private->discrete_numerator / 
	  d->private->discrete_denominator*pneg;
      }

      /* cosmetic adjustment complete, generate the scales */
      data_w = abs(hi_i-lo_i);
      if(!(d->flags & SUSHIV_DIM_ZEROINDEX))
	floor_i = 0;

      *data = scalespace_linear((double)lo_i * d->private->discrete_numerator / 
				d->private->discrete_denominator,
				(double)hi_i * d->private->discrete_numerator / 
				d->private->discrete_denominator,
				data_w, spacing, legend);
      
      if(d->flags & SUSHIV_DIM_MONOTONIC)
	*iter = scalespace_linear(lo_i - floor_i, hi_i - floor_i,
				  data_w, 1, legend);
      else
	*iter = scalespace_linear((double)(lo_i - floor_i) * 
				  d->private->discrete_numerator / 
				  d->private->discrete_denominator,
				  (double)(hi_i - floor_i) * 
				  d->private->discrete_numerator / 
				  d->private->discrete_denominator,
				  data_w, 1, legend);
      break;
    }
    break;
  case SUSHIV_DIM_PICKLIST:
    fprintf(stderr,"ERROR: Cannot iterate over picklist dimension!\n"
	    "\tA picklist dimension may not be a panel axis.\n");
    data_w = 0;
    break;
  default:
    fprintf(stderr,"ERROR: Unsupporrted dimension type in dimension_datascale.\n");
    data_w = 0;
    break;
  }

  return data_w;
}

int _sushiv_dimension_scales_from_panel(sushiv_dimension_t *d,
					scalespace panel,
					int data_w,
					scalespace *data, 
					scalespace *iter){

  return _sushiv_dimension_scales(d,
				  panel.lo,
				  panel.hi,
				  panel.pixels,
				  data_w,
				  panel.spacing,
				  panel.legend,
				  &panel, // dummy
				  data, 
				  iter);
}


static double discrete_quantize_val(sushiv_dimension_t *d, double val){
  if(d->type == SUSHIV_DIM_DISCRETE){
    val *= d->private->discrete_denominator;
    val /= d->private->discrete_numerator;
    
    val = rint(val);
    
    val *= d->private->discrete_numerator;
    val /= d->private->discrete_denominator;
  }
  return val;
}

static void _sushiv_dimension_center_callback(void *data, int buttonstate){
  gdk_threads_enter();

  sushiv_dim_widget_t *dw = (sushiv_dim_widget_t *)data;

  if(!dw->center_updating){
    sushiv_dimension_t *d = dw->dl->d;
    sushiv_panel_t *p = dw->dl->p;
    double val = slider_get_value(dw->scale,1);

    val = discrete_quantize_val(d,val);
    dw->center_updating = 1;
    
    if(buttonstate == 0){
      _sushiv_panel_undo_push(p);
      _sushiv_panel_undo_suspend(p);
    }
    
    if(d->val != val){
      int i;

      d->val = val;
      
      /* dims can be shared amongst multiple widgets; all must be updated */
      for(i=0;i<d->private->widgets;i++){
	sushiv_dim_widget_t *w = d->private->widget_list[i];
	if(w->scale) // all shared widgets had better have scales, but bulletproof in case
	  slider_set_value(w->scale,1,val);
      }

      /* dims can be shared amongst multiple widgets; all must get callbacks */
      for(i=0;i<d->private->widgets;i++){
	sushiv_dim_widget_t *w = d->private->widget_list[i];
	w->center_callback(dw->dl);
      }
    }
    
    if(buttonstate == 2)
      _sushiv_panel_undo_resume(p);
    
    dw->center_updating = 0;
  }
  gdk_threads_leave();
}

static void _sushiv_dimension_bracket_callback(void *data, int buttonstate){
  gdk_threads_enter();

  sushiv_dim_widget_t *dw = (sushiv_dim_widget_t *)data;

  if(!dw->bracket_updating){
    sushiv_dimension_t *d = dw->dl->d;
    sushiv_panel_t *p = dw->dl->p;
    double lo = slider_get_value(dw->scale,0);
    double hi = slider_get_value(dw->scale,2);
 
    hi = discrete_quantize_val(d,hi);
    lo = discrete_quantize_val(d,lo);

    dw->bracket_updating = 1;
    
    if(buttonstate == 0){
      _sushiv_panel_undo_push(p);
      _sushiv_panel_undo_suspend(p);
    }
    
    if(d->bracket[0] != lo || d->bracket[1] != hi){
      int i;

      d->bracket[0] = lo;
      d->bracket[1] = hi;

      /* dims can be shared amongst multiple widgets; all must be updated */
      for(i=0;i<d->private->widgets;i++){
	sushiv_dim_widget_t *w = d->private->widget_list[i];
	if(w->scale){ // all shared widgets had better have scales, but bulletproof in case
	  slider_set_value(w->scale,0,lo);
	  slider_set_value(w->scale,2,hi);
	}
      }

      /* dims can be shared amongst multiple widgets; all must get callbacks */
      for(i=0;i<d->private->widgets;i++){
	sushiv_dim_widget_t *w = d->private->widget_list[i];
	w->bracket_callback(dw->dl);
      }
    }
    
    if(buttonstate == 2)
      _sushiv_panel_undo_resume(p);
    
    dw->bracket_updating = 0;
  }
  gdk_threads_leave();
}

static void _sushiv_dimension_dropdown_callback(GtkWidget *dummy, void *data){
  gdk_threads_enter();

  sushiv_dim_widget_t *dw = (sushiv_dim_widget_t *)data;

  if(!dw->center_updating){
    sushiv_dimension_t *d = dw->dl->d;
    sushiv_panel_t *p = dw->dl->p;
    int bin = gtk_combo_box_get_active(GTK_COMBO_BOX(dw->menu));
    double val = d->scale->val_list[bin];
 
    dw->center_updating = 1;
    
    _sushiv_panel_undo_push(p);
    _sushiv_panel_undo_suspend(p);
    
    if(d->val != val){
      int i;

      d->val = val;
      d->bracket[0] = val;
      d->bracket[1] = val;

      /* dims can be shared amongst multiple widgets; all must be updated */
      for(i=0;i<d->private->widgets;i++){
	sushiv_dim_widget_t *w = d->private->widget_list[i];
	if(w->menu) // all shared widgets had better have scales, but bulletproof in case
	  gtk_combo_box_set_active(GTK_COMBO_BOX(w->menu),bin);
      }

      /* dims can be shared amongst multiple widgets; all must get callbacks */
      for(i=0;i<d->private->widgets;i++){
	sushiv_dim_widget_t *w = d->private->widget_list[i];
	w->center_callback(dw->dl);
      }
    }
    _sushiv_panel_undo_resume(p);
    
    dw->center_updating = 0;
  }
  gdk_threads_leave();
}

/* undo/redo have to frob widgets; this is indirected here too */
void _sushiv_dimension_set_value(sushiv_dim_widget_t *dw, int thumb, double val){
  sushiv_dimension_t *d = dw->dl->d;

  switch(d->type){
  case SUSHIV_DIM_CONTINUOUS:
    slider_set_value(dw->scale,thumb,val);
    break;
  case SUSHIV_DIM_DISCRETE:
    val *= d->private->discrete_denominator;
    val /= d->private->discrete_numerator;

    val = rint(val);

    val *= d->private->discrete_numerator;
    val /= d->private->discrete_denominator;

    slider_set_value(dw->scale,thumb,val);
    break;
  case SUSHIV_DIM_PICKLIST:
    /* find the picklist val closest to matching requested val */
    if(thumb == 1){
      int best=-1;
      double besterr=0;
      int i;
      
      for(i=0;i<d->scale->vals;i++){
	double err = fabs(val - d->scale->val_list[i]);
	if( best == -1 || err < besterr){
	  best = i;
	  besterr = err;
	}
      }
      
      if(best > -1)
	gtk_combo_box_set_active(GTK_COMBO_BOX(dw->menu),best);
    }
    break;
  default:
    fprintf(stderr,"ERROR: Unsupporrted dimension type in dimension_set_value.\n");
    break;
  }
}

/* external version with externalish API */
void sushiv_dimension_set_value(sushiv_instance_t *s, int dim_number, int thumb, double val){
  sushiv_dimension_t *d;

  if(dim_number<0 || dim_number>=s->dimensions){
    fprintf(stderr,"Dimension number %d out of range.\n",dim_number);
    return;
  }

  d=s->dimension_list[dim_number];
  if(!d){
    fprintf(stderr,"Dimension %d does not exist.\n",dim_number);
    return;
  }

  if(!d->private->widgets){
    switch(thumb){
    case 0:
      d->bracket[0] = discrete_quantize_val(d,val);
      break;
    case 1:
      d->val = discrete_quantize_val(d,val);
      break;
    default:
      d->bracket[1] = discrete_quantize_val(d,val);
      break;
    }
  }else
    _sushiv_dimension_set_value(d->private->widget_list[0],thumb,val);
  
}

void _sushiv_dim_widget_set_thumb_active(sushiv_dim_widget_t *dw, int thumb, int active){
  if(dw->scale)
    slider_set_thumb_active(dw->scale,thumb,active);
}

sushiv_dim_widget_t *_sushiv_new_dimension_widget(sushiv_dimension_list_t *dl,   
						 void (*center_callback)(sushiv_dimension_list_t *),
						 void (*bracket_callback)(sushiv_dimension_list_t *)){
  
  sushiv_dim_widget_t *dw = calloc(1, sizeof(*dw));
  sushiv_dimension_t *d = dl->d;

  dw->dl = dl;
  dw->center_callback = center_callback;
  dw->bracket_callback = bracket_callback;

  switch(d->type){
  case SUSHIV_DIM_CONTINUOUS:
  case SUSHIV_DIM_DISCRETE:
    /* Continuous and discrete dimensions get sliders */
    {
      double v[3];
      GtkWidget **sl = calloc(3,sizeof(*sl));
      dw->t = GTK_TABLE(gtk_table_new(1,3,0));
      v[0]=d->bracket[0];
      v[1]=d->val;
      v[2]=d->bracket[1];

      sl[0] = slice_new(_sushiv_dimension_bracket_callback,dw);
      sl[1] = slice_new(_sushiv_dimension_center_callback,dw);
      sl[2] = slice_new(_sushiv_dimension_bracket_callback,dw);
      
      gtk_table_attach(dw->t,sl[0],0,1,0,1,
		       GTK_EXPAND|GTK_FILL,0,0,0);
      gtk_table_attach(dw->t,sl[1],1,2,0,1,
		       GTK_EXPAND|GTK_FILL,0,0,0);
      gtk_table_attach(dw->t,sl[2],2,3,0,1,
		       GTK_EXPAND|GTK_FILL,0,0,0);

      dw->scale = slider_new((Slice **)sl,3,d->scale->label_list,d->scale->val_list,
			     d->scale->vals,0);
      if(d->type == SUSHIV_DIM_DISCRETE)
	slider_set_quant(dw->scale,d->private->discrete_numerator,d->private->discrete_denominator);

      slice_thumb_set((Slice *)sl[0],v[0]);
      slice_thumb_set((Slice *)sl[1],v[1]);
      slice_thumb_set((Slice *)sl[2],v[2]);
    }
    break;
  case SUSHIV_DIM_PICKLIST:
    /* picklist dimensions get a wide dropdown */
    dw->t = GTK_TABLE(gtk_table_new(1,1,0));

    {
      int j;
      dw->menu=gtk_combo_box_new_markup();
      for(j=0;j<d->scale->vals;j++)
	gtk_combo_box_append_text (GTK_COMBO_BOX (dw->menu), d->scale->label_list[j]);

      g_signal_connect (G_OBJECT (dw->menu), "changed",
			G_CALLBACK (_sushiv_dimension_dropdown_callback), dw);
      
      gtk_table_attach(dw->t,dw->menu,0,1,0,1,
		       GTK_EXPAND|GTK_FILL,GTK_SHRINK,0,0);
      _sushiv_dimension_set_value(dw,1,d->val);
      //gtk_combo_box_set_active(GTK_COMBO_BOX(dw->menu),0);
    }
    break;
  default:
    fprintf(stderr,"ERROR: Unsupporrted dimension type in new_dimension_widget.\n");
    break;
  }

  /* add widget to dimension */
  if(!d->private->widget_list){
    d->private->widget_list = calloc (1, sizeof(*d->private->widget_list));
  }else{
    d->private->widget_list = realloc (d->private->widget_list,
				       d->private->widgets+1 * sizeof(*d->private->widget_list));
  }
  d->private->widget_list[d->private->widgets] = dw;
  d->private->widgets++;

  return dw;
};

int sushiv_new_dimension(sushiv_instance_t *s,
			 int number,
			 const char *name,
			 unsigned scalevals, 
			 double *scaleval_list,
			 int (*callback)(sushiv_dimension_t *),
			 unsigned flags){
  sushiv_dimension_t *d;
  
  if(number<0){
    fprintf(stderr,"Dimension number must be >= 0\n");
    return -EINVAL;
  }

  if(number<s->dimensions){
    if(s->dimension_list[number]!=NULL){
      fprintf(stderr,"Dimension number %d already exists\n",number);
      return -EINVAL;
    }
  }else{
    if(s->dimensions == 0){
      s->dimension_list = calloc (number+1,sizeof(*s->dimension_list));
    }else{
      s->dimension_list = realloc (s->dimension_list,(number+1) * sizeof(*s->dimension_list));
      memset(s->dimension_list + s->dimensions, 0, sizeof(*s->dimension_list)*(number + 1 - s->dimensions));
    }
    s->dimensions=number+1;
  }

  d = s->dimension_list[number] = calloc(1, sizeof(**s->dimension_list));
  d->number = number;
  d->name = strdup(name);
  d->flags = flags;
  d->sushi = s;
  d->callback = callback;
  d->scale = scale_new(scalevals, scaleval_list, name);
  d->type = SUSHIV_DIM_CONTINUOUS;
  d->private = calloc(1, sizeof(*d->private));

  d->bracket[0]=d->scale->val_list[0];
  d->val = 0;
  d->bracket[1]=d->scale->val_list[d->scale->vals-1];

  return 0;
}

int sushiv_new_dimension_discrete(sushiv_instance_t *s,
				  int number,
				  const char *name,
				  unsigned scalevals, 
				  double *scaleval_list,
				  int (*callback)(sushiv_dimension_t *),
				  long num,
				  long denom,
				  unsigned flags){
  
  /* template is a normal continuous dim */
  sushiv_dimension_t *d;
  int ret=sushiv_new_dimension(s,number,name,scalevals,scaleval_list,callback,flags);

  if(ret)return ret;

  d = s->dimension_list[number];

  d->private->discrete_numerator = num;
  d->private->discrete_denominator = denom;
  d->type = SUSHIV_DIM_DISCRETE;
  
  return 0;
}

int sushiv_new_dimension_picklist(sushiv_instance_t *s,
				  int number,
				  const char *name,
				  unsigned pickvals, 
				  double *pickval_list,
				  char **pickval_labels,
				  int (*callback)(sushiv_dimension_t *),
				  unsigned flags){

  /* template is a normal continuous dim */
  sushiv_dimension_t *d;
  int ret=sushiv_new_dimension(s,number,name,pickvals,pickval_list,callback,flags);

  if(ret)return ret;

  d = s->dimension_list[number];
  
  scale_set_scalelabels(d->scale, pickval_labels);
  d->flags |= SUSHIV_DIM_NO_X;
  d->flags |= SUSHIV_DIM_NO_Y;
  d->type = SUSHIV_DIM_PICKLIST;

  return 0;

}
