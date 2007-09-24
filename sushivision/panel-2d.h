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

typedef struct {
  pthread_rwlock_t panel_m;
  pthread_mutex_t  status_m;
  
  // request payload (locked by status)
  int               recompute_pending;
  int               dims;
  int               w;
  int               h;
  int               xdim;
  int               ydim;
  sv_dim_data_t    *dim_data;

  // composite 'background' plane
  _sv_plane_bg_t *bg;

  // objective planes
  int planes;
  _sv_plane_t **plane_list;
  int next_plane; 
  pthread_mutex_t  planelock; // locks plane status, not data
  
  // UI elements
  GtkWidget *obj_table;
  GtkWidget *dim_table;

  double oldbox[4];

  GtkWidget **dim_xb; // X axis selector buttons
  GtkWidget **dim_yb; // Y axis selector buttons

  _sv_dim_widget_t *x_scale; // pointer to current X axis dimwidget
  _sv_dim_widget_t *y_scale; // pointer to current Y axis dimwidget
  int x_dnum; // panel, not global list context
  int y_dnum; // panel, not global list context

} _sv_panel2d_t;

typedef struct {
  double *fout; 
  int fout_size;


} _sv_bythread_cache_2d_t;
