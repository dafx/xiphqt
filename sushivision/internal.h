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

typedef struct _sv_instance_undo _sv_instance_undo_t;

#include <time.h>
#include <signal.h>
#include <gtk/gtk.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "sushivision.h"

// used to glue numeric settings to semantic labels for menus/save files
typedef struct _sv_propmap _sv_propmap_t;
struct _sv_propmap {
  char *left;
  int value;
  
  char *right;
  _sv_propmap_t **submenu;
  void (*callback)(sv_panel_t *, GtkWidget *);
};

#include "mapping.h"
#include "slice.h"
#include "spinner.h"
#include "slider.h"
#include "scale.h"
#include "plot.h"
#include "dimension.h"
#include "objective.h"
#include "panel-1d.h"
#include "panel-xy.h"
#include "panel-2d.h"
#include "xml.h"
#include "gtksucks.h"

union _sv_panel_subtype {
  _sv_panelxy_t *xy;
  _sv_panel1d_t *p1;
  _sv_panel2d_t *p2;
};

// undo history; the panel types vary slightly, but only slightly, so
// for now we use a master undo type which leaves one or two fields
// unused for a given panel.
typedef struct {
  int *mappings;
  double *scale_vals[3];
  
  int x_d;
  int y_d;

  double box[4];
  int box_active;

  int grid_mode;
  int cross_mode;
  int legend_mode;
  int bg_mode;
  int text_mode;
  int menu_cursamp;
  int oversample_n;
  int oversample_d;

} _sv_panel_undo_t;

typedef union {
  _sv_bythread_cache_1d_t p1;
  _sv_bythread_cache_xy_t xy;
  _sv_bythread_cache_2d_t p2;
} _sv_bythread_cache_t;

struct _sv_panel_internal {
  GtkWidget *toplevel;
  GtkWidget *topbox;
  GtkWidget *plotbox;
  GtkWidget *graph;
  _sv_spinner_t *spinner;
  GtkWidget *popmenu;

  enum sv_background bg_type;
  _sv_dim_widget_t **dim_scales;
  int oldbox_active;

  int realized;

  int map_active;
  int map_progress_count;
  int map_complete_count;
  int map_serialno;

  int legend_active;
  int legend_progress_count;
  int legend_complete_count;
  int legend_serialno;

  int plot_active;
  int plot_progress_count;
  int plot_complete_count;
  int plot_serialno;

  time_t last_map_throttle;

  int menu_cursamp;
  int oversample_n;
  int oversample_d;
  int def_oversample_n;
  int def_oversample_d;

  // function bundles 
  void (*realize)(sv_panel_t *p);
  int (*map_action)(sv_panel_t *p, _sv_bythread_cache_t *c);
  int (*legend_action)(sv_panel_t *p);
  int (*compute_action)(sv_panel_t *p, _sv_bythread_cache_t *c);
  void (*request_compute)(sv_panel_t *p);
  void (*crosshair_action)(sv_panel_t *p);
  void (*print_action)(sv_panel_t *p, cairo_t *c, int w, int h);
  int (*save_action)(sv_panel_t *p, xmlNodePtr pn);
  int (*load_action)(sv_panel_t *p, _sv_panel_undo_t *u, xmlNodePtr pn, int warn);

  void (*undo_log)(_sv_panel_undo_t *u, sv_panel_t *p);
  void (*undo_restore)(_sv_panel_undo_t *u, sv_panel_t *p);
};

struct _sv_instance_undo {
  _sv_panel_undo_t *panels;
  double *dim_vals[3];
};

struct _sv_instance_internal {
  int undo_level;
  int undo_suspend;
  _sv_instance_undo_t **undo_stack;
};

extern void _sv_clean_exit(int sig);
extern void _sv_wake_workers();
extern int  _sv_main_save();
extern int  _sv_main_load();
extern void _sv_first_load_warning(int *);


extern sv_panel_t *_sv_panel_new(sv_instance_t *s,
				 int number,
				 char *name, 
				 sv_obj_t **objectives,
				 sv_dim_t **dimensions,	
				 unsigned flags);
extern void _sv_panel_realize(sv_panel_t *p);
extern void _sv_panel_dirty_map(sv_panel_t *p);
extern void _sv_panel_dirty_map_throttled(sv_panel_t *p);
extern void _sv_panel_dirty_legend(sv_panel_t *p);
extern void _sv_panel_dirty_plot(sv_panel_t *p);
extern void _sv_panel_recompute(sv_panel_t *p);
extern void _sv_panel_clean_map(sv_panel_t *p);
extern void _sv_panel_clean_legend(sv_panel_t *p);
extern void _sv_panel_clean_plot(sv_panel_t *p);
extern void _sv_panel_undo_log(sv_panel_t *p, _sv_panel_undo_t *u);
extern void _sv_panel_undo_restore(sv_panel_t *p, _sv_panel_undo_t *u);
extern void _sv_panel_update_menus(sv_panel_t *p);
extern int  _sv_panel_save(sv_panel_t *p, xmlNodePtr instance);
extern int  _sv_panel_load(sv_panel_t *p, _sv_panel_undo_t *u, xmlNodePtr instance, int warn);

extern void _sv_panel1d_mark_recompute_linked(sv_panel_t *p); 
extern void _sv_panel1d_update_linked_crosshairs(sv_panel_t *p, int xflag, int yflag); 

extern void _sv_map_set_throttle_time(sv_panel_t *p);

extern void _sv_undo_log(sv_instance_t *s);
extern void _sv_undo_push(sv_instance_t *s);
extern void _sv_undo_pop(sv_instance_t *s);
extern void _sv_undo_suspend(sv_instance_t *s);
extern void _sv_undo_resume(sv_instance_t *s);
extern void _sv_undo_restore(sv_instance_t *s);
extern void _sv_undo_up(sv_instance_t *s);
extern void _sv_undo_down(sv_instance_t *s);

extern sig_atomic_t _sv_exiting;
extern char *_sv_filebase;
extern char *_sv_filename;
extern char *_sv_dirname;
extern char *_sv_cwdname;
