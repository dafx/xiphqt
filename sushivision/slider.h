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

#include <sys/types.h>

struct _sv_slider {
  GtkWidget **slices;
  int num_slices;
  int realized;
  int flip; // native is horizontal

  u_int32_t *backdata;
  cairo_surface_t *background;
  cairo_surface_t *foreground;
  int w;
  int h;
  _sv_mapping_t *gradient;
  int xpad;
  int ypad;

  char **label;
  double *label_vals;
  int labels;
  int neg;
  int flags;
  
  double quant_num;
  double quant_denom;
};

#define _SV_SLIDER_FLAG_INDEPENDENT_MIDDLE 0x1
#define _SV_SLIDER_FLAG_VERTICAL 0x80

extern void _sv_slider_realize(_sv_slider_t *s);
extern void _sv_slider_draw(_sv_slider_t *s);
extern void _sv_slider_expose_slice(_sv_slider_t *s, int slicenum);
extern void _sv_slider_expose(_sv_slider_t *s);
extern void _sv_slider_size_request_slice(_sv_slider_t *s,GtkRequisition *requisition);
extern double _sv_slider_pixel_to_val(_sv_slider_t *slider,double x);
extern double _sv_slider_pixel_to_del(_sv_slider_t *slider,double x);
extern double _sv_slider_val_to_del(_sv_slider_t *slider,double v);
extern void _sv_slider_vals_bound(_sv_slider_t *slider,int slicenum);
extern int _sv_slider_lightme(_sv_slider_t *slider,int slicenum,int x,int y);
extern void _sv_slider_unlight(_sv_slider_t *slider);
extern void _sv_slider_button_press(_sv_slider_t *slider,int slicenum,int x,int y);
extern void _sv_slider_button_release(_sv_slider_t *s,int slicenum,int x,int y);
extern void _sv_slider_motion(_sv_slider_t *s,int slicenum,int x,int y);
extern gboolean _sv_slider_key_press(_sv_slider_t *slider,GdkEventKey *event,int slicenum);
extern _sv_slider_t *_sv_slider_new(_sv_slice_t **slices, int num_slices, 
			  char **labels, double *label_vals, int num_labels,
			  unsigned flags);
extern void _sv_slider_set_thumb_active(_sv_slider_t *s, int thumbnum, int activep);
extern void _sv_slider_set_gradient(_sv_slider_t *s, _sv_mapping_t *m);
extern double _sv_slider_get_value(_sv_slider_t *s, int thumbnum);
extern void _sv_slider_set_value(_sv_slider_t *s, int thumbnum, double v);
extern double _sv_slider_del_to_val(_sv_slider_t *s, double del);
extern void _sv_slider_set_quant(_sv_slider_t *s, double n, double d);

extern void _sv_slider_print(_sv_slider_t *s, cairo_t *c, int w, int h);
extern double _sv_slider_print_height(_sv_slider_t *s);
