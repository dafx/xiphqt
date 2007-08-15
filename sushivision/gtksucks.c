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

#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <gtk/gtk.h>
#include <pthread.h>
#include "sushivision.h"
#include "internal.h"
#include "gtksucks.h"

/* well, no, gtk doesn't actually suck, but it does have a number of
   default behaviors that get in the way of building a UI the way
   users have come to expect.  This module encapsulates a number of
   generic fixups that eliminate inconsistencies or obstructive
   behaviors. */

/**********************************************************************/
/* fixup number 1: insensitive widgets (except children of viewports,
   a special case) eat mouse events.  This doesn't seem like a big
   deal, but if you're implementing a right-click context menu, this
   means you cannot pop a menu if the user was unlucky enough to have
   right-clicked over an insensitive widget.  Wrap the widget
   sensitivity setting method to also also set/unset the
   GDK_BUTTON_PRESS_MASK on a widget; when insensitive, this will
   remove its ability to silently swallow mouse events. */

/* Note that this only works after the widget is realized, as
   realization will clobber the event mask */

void _gtk_widget_set_sensitive_fixup(GtkWidget *w, gboolean state){
  gdk_threads_enter();
  gtk_widget_set_sensitive(w,state);

  if(state)
    gtk_widget_add_events (w, GDK_ALL_EVENTS_MASK);
  else
    _gtk_widget_remove_events (w, GDK_ALL_EVENTS_MASK);
  gdk_threads_leave();
}

static void remove_events_internal (GtkWidget *widget,
				    gint       events,
				    GList     *window_list){
  GList *l;
  
  for (l = window_list; l != NULL; l = l->next){
    GdkWindow *window = l->data;
    gpointer user_data;
    
    gdk_window_get_user_data (window, &user_data);
    if (user_data == widget){
      GList *children;
      
      gdk_window_set_events (window, gdk_window_get_events(window) & (~events));
      
      children = gdk_window_get_children (window);
      remove_events_internal (widget, events, children);
      g_list_free (children);
    }
  }
}

/* gtk provides a 'gtk_widget_add_events' but not a converse to remove
   events.  'gtk_widget_set_events' only works pre-realization, so it
   can't be used instead. */
void _gtk_widget_remove_events (GtkWidget *widget,
			       gint       events){
  
  g_return_if_fail (GTK_IS_WIDGET (widget));

  GQuark quark_event_mask = g_quark_from_static_string ("gtk-event-mask");
  gint *eventp = g_object_get_qdata (G_OBJECT (widget), quark_event_mask);
  gint original_events = events;
  
  if (!eventp){
    eventp = g_slice_new (gint);
    *eventp = 0;
  }
  
  events = ~events;
  events &= *eventp;

  if(events){ 
    *eventp = events;
    g_object_set_qdata (G_OBJECT (widget), quark_event_mask, eventp);
  }else{
    g_slice_free (gint, eventp);
    g_object_set_qdata (G_OBJECT (widget), quark_event_mask, NULL);
  }

  if (GTK_WIDGET_REALIZED (widget)){
    GList *window_list;
    
    if (GTK_WIDGET_NO_WINDOW (widget))
      window_list = gdk_window_get_children (widget->window);
    else
      window_list = g_list_prepend (NULL, widget->window);
    
    remove_events_internal (widget, original_events, window_list);
    
    g_list_free (window_list);
  }
  
  g_object_notify (G_OBJECT (widget), "events");
}

/**********************************************************************/
/* fixup number 2: Buttons (and their subclasses) don't do anything
   with mousebutton3, but they swallow the events anyway preventing
   them from cascading.  The 'supported' way of implementing a
   context-sensitive right-click menu is to recursively bind a new
   handler to each and every button on a toplevel.  This is mad
   whack.  The below 'fixes' buttons at the class level by ramming a
   new button press handler into the GtkButtonClass structure (and,
   unfortunately, button subclasses as their classes have also already
   initialized and made a copy of the Button's class structure and
   handlers */ 

static gboolean _gtk_button_button_press_new (GtkWidget      *widget,
					     GdkEventButton *event){
  
  if (event->type == GDK_BUTTON_PRESS){
    GtkButton *button = GTK_BUTTON (widget);
    
    if (event->button == 3)
      return FALSE;
    
    if (button->focus_on_click && !GTK_WIDGET_HAS_FOCUS (widget))
      gtk_widget_grab_focus (widget);
    
    if (event->button == 1)
      gtk_button_pressed (button);
    return TRUE;
  }

  return FALSE;
}

static gboolean _gtk_button_button_release_new (GtkWidget      *widget,
					       GdkEventButton *event){
  if (event->button == 1) {
    GtkButton *button = GTK_BUTTON (widget);
    gtk_button_released (button);
    return TRUE;
  }

  return FALSE;
}

/* does not currently handle all button types, just the ones we use */
void _gtk_button3_fixup(){

  GtkWidget *bb = gtk_button_new();
  GtkWidgetClass *bc = GTK_WIDGET_GET_CLASS(bb);
  bc->button_press_event = _gtk_button_button_press_new;
  bc->button_release_event = _gtk_button_button_release_new;

  bb = gtk_radio_button_new(NULL);
  bc = GTK_WIDGET_GET_CLASS(bb);
  bc->button_press_event = _gtk_button_button_press_new;
  bc->button_release_event = _gtk_button_button_release_new;

  bb = gtk_toggle_button_new();
  bc = GTK_WIDGET_GET_CLASS(bb);
  bc->button_press_event = _gtk_button_button_press_new;
  bc->button_release_event = _gtk_button_button_release_new;

  bb = gtk_check_button_new();
  bc = GTK_WIDGET_GET_CLASS(bb);
  bc->button_press_event = _gtk_button_button_press_new;
  bc->button_release_event = _gtk_button_button_release_new;
 
  // just leak 'em.  they'll go away on exit.

}

/**********************************************************************/
/* fixup number 3: GDK uses whatever default mutex type offered by the
   system, and this usually means non-recursive ('fast') mutextes.
   The problem with this is that gdk_threads_enter() and
   gdk_threads_leave() cannot be used in any call originating from the
   main loop, but are required in calls from idle handlers and other
   threads. In effect we would need seperate identical versions of
   each widget method, one locked, one unlocked, depending on where
   the call originated.  Eliminate this problem by installing a
   recursive mutex. */

static pthread_mutex_t gdkm;
static pthread_mutexattr_t gdkma;
static int depth = 0;
static int firstunder = 0;

static void recursive_gdk_lock(void){
  pthread_mutex_lock(&gdkm);
  depth++;
}

static void recursive_gdk_unlock(void){
  depth--;
  if(depth<0){
    if(!firstunder){ // annoying detail of gtk locking; in apps that
      // don't normally use threads, onr does not lock before entering
      // mainloop; in apps that do thread, the mainloop must be
      // locked.  We can't tell which situation was in place before
      // setting up our own threading, so allow one refcount error
      // which we assume was the unlocked mainloop of a normally
      // unthreaded gtk app.
      firstunder++;
      depth=0;
    }else{
      fprintf(stderr,"Internal locking error; refcount < 0. Dumping core for debugging\n");
      abort();
    }
  }else
    pthread_mutex_unlock(&gdkm);
}

void _gtk_mutex_fixup(){
  pthread_mutexattr_init(&gdkma);
  pthread_mutexattr_settype(&gdkma,PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&gdkm,&gdkma);
  gdk_threads_set_lock_functions(recursive_gdk_lock,recursive_gdk_unlock);
}

pthread_mutex_t *_gtk_get_mutex(void){
  return &gdkm;
}

/**********************************************************************/
/* Not really a fixup; generate menus that declare what the keyboard
   shortcuts are */

static gint popup_callback (GtkWidget *widget, GdkEvent *event){

  GtkMenu *menu = GTK_MENU (widget);
  GdkEventButton *event_button = (GdkEventButton *) event;

  if (event_button->button == 3){
    gtk_menu_popup (menu, NULL, NULL, NULL, NULL, 
		    event_button->button, event_button->time);
    return TRUE;
  }

  return FALSE;
}

GtkWidget *_gtk_menu_new_twocol(GtkWidget *bind, 
				_sv_propmap_t **items,
				void *callback_data){
  
  _sv_propmap_t *ptr = *items++;
  GtkWidget *ret = gtk_menu_new();
   
  /* create packable boxes for labels, put left labels in */
  while(ptr){
    GtkWidget *item;
    if(!strcmp(ptr->left,"")){
      // seperator, not item
      item = gtk_separator_menu_item_new();
      gtk_menu_shell_append(GTK_MENU_SHELL(ret),item);
    }else{
      GtkWidget *box = gtk_hbox_new(0,10);
      GtkWidget *left = gtk_label_new(NULL);
      GtkWidget *right = NULL;

      gtk_label_set_markup (GTK_LABEL (left), ptr->left);

      item = gtk_menu_item_new();
      gtk_container_add(GTK_CONTAINER(item),box);
      gtk_box_pack_start(GTK_BOX(box),left,0,0,5);
      
      if(ptr->right){
	right = gtk_label_new(NULL);	
	gtk_label_set_markup (GTK_LABEL (right), ptr->right);
	gtk_box_pack_end(GTK_BOX(box),right,0,0,5);
      }

      gtk_menu_shell_append(GTK_MENU_SHELL(ret),item);
      if(ptr->callback)
	g_signal_connect_swapped (G_OBJECT (item), "activate",
				  G_CALLBACK (ptr->callback), callback_data);
      if(ptr->submenu){
	GtkWidget *submenu = _gtk_menu_new_twocol(ret,ptr->submenu,callback_data);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item),submenu);
      }
    }
    gtk_widget_show_all(item);
    
    ptr = *items++;
  }

  if(bind){
    gtk_widget_add_events(bind, GDK_BUTTON_PRESS_MASK);
    g_signal_connect_swapped (bind, "button-press-event",
			      G_CALLBACK (popup_callback), ret);
  }

  return ret;
}

GtkWidget *_gtk_menu_get_item(GtkMenu *m, int pos){
  int i=0;
  GList *l=gtk_container_get_children (GTK_CONTAINER(m));    
  
  while(l){
    if(i == pos){
      GtkWidget *ret = (GtkWidget *)l->data;
      g_list_free (l);
      return ret;
    }

    i++;
    l=l->next;
  }

  return NULL;
}

int _gtk_menu_item_position(GtkWidget *w){
  //GtkMenuItem *mi = GTK_MENU_ITEM(w);
  GtkWidget *box = gtk_widget_get_parent(w);
  GList *l = gtk_container_get_children(GTK_CONTAINER(box));
  int i = 0;

  while(l){
    if((GtkWidget *)l->data == w){
      g_list_free (l);
      return i;
    }
    i++;
    l = l->next;
  }

  return 0;
}

void _gtk_menu_alter_item_label(GtkMenu *m, int pos, char *text){
  GList *l;
  GtkWidget *box=NULL;
  GtkWidget *label=NULL;
  GtkWidget *item = _gtk_menu_get_item(m, pos);
  if(!item)return;

  l=gtk_container_get_children (GTK_CONTAINER(item));    
  box = l->data;
  g_list_free(l);

  if(!box)return;

  l=gtk_container_get_children (GTK_CONTAINER(box));    
  label = l->data;
  g_list_free(l);

  if(!label)return;

  gtk_label_set_markup(GTK_LABEL(label),text);
}

void _gtk_menu_alter_item_right(GtkMenu *m, int pos, char *text){
  GList *l;
  GtkWidget *box=NULL;
  GtkWidget *label=NULL;
  GtkWidget *item = _gtk_menu_get_item(m, pos);
  if(!item)return;

  l=gtk_container_get_children (GTK_CONTAINER(item));    
  box = l->data;
  g_list_free(l);

  if(!box)return;

  l=gtk_container_get_children (GTK_CONTAINER(box));    
  if(l && l->next)
    label = l->next->data;
  g_list_free(l);

  if(!label)return;

  gtk_label_set_markup(GTK_LABEL(label),text);
}

/**********************************************************************/
/* unlock text combo boxes to support markup as well as straight text */

GtkWidget *_gtk_combo_box_new_markup (void){
  GtkWidget *combo_box;
  GtkCellRenderer *cell;
  GtkListStore *store;

  store = gtk_list_store_new (1, G_TYPE_STRING);
  combo_box = gtk_combo_box_new_with_model (GTK_TREE_MODEL (store));
  g_object_unref (store);

  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo_box), cell, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo_box), cell,
                                  "markup", 0,
                                  NULL);

  return combo_box;
}

/**********************************************************************/
/* Gtk has an annoying habit of 'overresizing'; superfluous resize
   events within panels that cause all hell to break loose for a
   while, only to have the panel finally look exactly like it did when
   it started.  This happens especially with expander widgets.  There
   are two problems: One, it's ugly as Hell to click an expander and
   have the whole window freak out just to make a little space.
   Second, having the Plot resize will trigger a recompute, which
   could seriously screw the user.

   The below lets us freeze/unfreeze an auto-resizing box's
   child at its current size without queueing resize events. */

void _gtk_box_freeze_child (GtkBox *box,
			   GtkWidget *child){
  GList *list;
  GtkBoxChild *child_info = NULL;

  g_return_if_fail (GTK_IS_BOX (box));
  g_return_if_fail (GTK_IS_WIDGET (child));

  list = box->children;
  while (list){
    child_info = list->data;
    if (child_info->widget == child)
      break;

    list = list->next;
  }

  if (list){
    child_info->expand = FALSE;
    child_info->fill = FALSE;
  }
}

void _gtk_box_unfreeze_child (GtkBox *box,
			     GtkWidget *child){
  GList *list;
  GtkBoxChild *child_info = NULL;
  
  g_return_if_fail (GTK_IS_BOX (box));
  g_return_if_fail (GTK_IS_WIDGET (child));

  list = box->children;
  while (list){
    child_info = list->data;
    if (child_info->widget == child)
      break;

    list = list->next;
  }

  if (list){
    child_info->expand = TRUE;
    child_info->fill = TRUE;
  }
}
