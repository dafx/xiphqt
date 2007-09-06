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

#ifndef _SUSHIVISION_
#define _SUSHIVISION_

typedef struct sv_scale sv_scale_t;
typedef struct sv_panel sv_panel_t;
typedef struct sv_dim   sv_dim_t;
typedef struct sv_obj   sv_obj_t;
typedef struct sv_func  sv_func_t;
typedef struct _sv_scale_internal sv_scale_internal_t;
typedef struct _sv_dim_internal sv_dim_internal_t;
typedef union  _sv_dim_subtype sv_dim_subtype_t;
typedef struct _sv_func_internal sv_func_internal_t;
typedef union  _sv_func_subtype sv_func_subtype_t;
typedef struct _sv_obj_internal sv_obj_internal_t;
typedef union  _sv_obj_subtype sv_obj_subtype_t;
typedef struct _sv_panel_internal sv_panel_internal_t;
typedef union  _sv_panel_subtype sv_panel_subtype_t;

int sv_submain(int argc, char *argv[]);
int sv_atexit(void);
int sv_save(char *filename);
int sv_load(char *filename);

/* toplevel ******************************************************/
extern int sv_init(void);
extern int sv_go(void);
extern int sv_join(void);

/* scales ********************************************************/

struct sv_scale{
  int vals;
  double *val_list;
  char **label_list; 
  char *legend;

  sv_scale_internal_t *private;
  unsigned flags;
};

sv_scale_t        *sv_scale_new (char *name,
				 char *values);

sv_scale_t       *sv_scale_copy (sv_scale_t *s);

void              sv_scale_free (sv_scale_t *s);

/* dimensions ****************************************************/

#define SV_DIM_NO_X 0x100
#define SV_DIM_NO_Y 0x200
#define SV_DIM_ZEROINDEX 0x1
#define SV_DIM_MONOTONIC 0x2

enum sv_dim_type { SV_DIM_CONTINUOUS, 
			 SV_DIM_DISCRETE, 
			 SV_DIM_PICKLIST};

struct sv_dim{ 
  int number;
  char *name;
  char *legend;
  enum sv_dim_type type;

  double bracket[2];
  double val;

  sv_scale_t *scale;
  unsigned flags;
  
  int (*callback)(sv_dim_t *);
  sv_dim_subtype_t *subtype;
  sv_dim_internal_t *private;
};

sv_dim_t            *sv_dim_new (int number, 
				 char *name,
				 unsigned flags);

sv_dim_t                *sv_dim (char *name);

int            sv_dim_set_scale (sv_scale_t *scale);

int           sv_dim_make_scale (char *format);

int         sv_dim_set_discrete (long quant_numerator,
				 long quant_denominator);

int         sv_dim_set_picklist ();

int            sv_dim_set_value (double val);

int          sv_dim_set_bracket (double lo,
				 double hi);

int       sv_dim_callback_value (int (*callback)(sv_dim_t *, void*),
				 void *callback_data);

/* functions *****************************************************/

enum sv_func_type  { SV_FUNC_BASIC };

struct sv_func {
  int number;
  enum sv_func_type type;
  int inputs;
  int outputs;
  
  void (*callback)(double *,double *);
  unsigned flags;

  sv_func_subtype_t *subtype;
  sv_func_internal_t *private;
};

sv_func_t          *sv_func_new (int number,
				 int out_vals,
				 void (*function)(double *,double *),
				 unsigned flags);

/* objectives ****************************************************/

enum sv_obj_type { SV_OBJ_BASIC };

struct sv_obj { 
  int number;
  char *name;
  enum sv_obj_type type;

  sv_scale_t *scale;
  int outputs;
  int *function_map;
  int *output_map;
  char *output_types;
  unsigned flags;

  sv_obj_subtype_t *subtype;
  sv_obj_internal_t *private;
};

sv_obj_t            *sv_obj_new (int number,
				 char *name,
				 sv_func_t **function_map,
				 int *function_output_map,
				 char *output_type_map,
				 unsigned flags);

sv_obj_t   *sv_obj_new_defaults (int number,
				 char *name,
				 sv_func_t *function,
				 unsigned flags);

int            sv_obj_set_scale (sv_obj_t *o,
				 sv_scale_t *scale);

int           sv_obj_make_scale (sv_obj_t *o,
				 char *format);

/* panels ********************************************************/

enum sv_panel_type     { SV_PANEL_1D, 
			 SV_PANEL_2D, 
			 SV_PANEL_XY };
enum sv_background     { SV_BG_WHITE, 
			 SV_BG_BLACK, 
			 SV_BG_CHECKS };

#define SV_PANEL_FLIP 0x4 

typedef struct {
  sv_dim_t *d;
  sv_panel_t *p;
} sv_dim_list_t;

typedef struct {
  sv_obj_t *o;
  sv_panel_t *p;
} sv_obj_list_t;
 
struct sv_panel {
  int number;
  char *name;
  enum sv_panel_type type;

  int dimensions;
  sv_dim_list_t *dimension_list;
  int objectives;
  sv_obj_list_t *objective_list;

  unsigned flags;

  sv_panel_subtype_t *subtype;
  sv_panel_internal_t *private;
};

sv_panel_t     *sv_panel_new_1d (int number,
				 char *name,
				 sv_scale_t *y_scale,
				 sv_obj_t **objectives,
				 char *dimensions,	
				 unsigned flags);

sv_panel_t     *sv_panel_new_xy (int number,
				 char *name,
				 sv_scale_t *x_scale,
				 sv_scale_t *y_scale,
				 sv_obj_t **objectives,
				 char *dimensions,	
				 unsigned flags);

sv_panel_t     *sv_panel_new_2d (int number,
				 char *name, 
				 sv_obj_t **objectives,
				 char *dimensions,
				 unsigned flags);

int sv_panel_callback_recompute (sv_panel_t *p,
				 int (*callback)(sv_panel_t *p,void *data),
				 void *data);

int       sv_panel_set_resample (sv_panel_t *p,
				 int numerator,
				 int denominator);

int     sv_panel_set_background (sv_panel_t *p,
				 enum sv_background bg);

int           sv_panel_set_axis (sv_panel_t *p,
				 char axis,
				 sv_dim_t *d);

int       sv_panel_get_resample (sv_panel_t *p,
				 int *numerator,
				 int *denominator);

int     sv_panel_get_background (sv_panel_t *p,
				 enum sv_background *bg);

sv_dim_t     *sv_panel_get_axis (sv_panel_t *p, char axis);
				    

#endif
