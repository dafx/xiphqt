/*
 *
 *  postfish
 *    
 *      Copyright (C) 2002-2004 Monty
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
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "mainpanel.h"
#include "windowbutton.h"
#include "subpanel.h"

static int clippanel_hide(GtkWidget *widget,
			  GdkEvent *event,
			  gpointer in){
  subpanel_generic *p=in;
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->subpanel_windowbutton),0);
  return TRUE;
}
static int windowbutton_action(GtkWidget *widget,
			gpointer in){
  int active;
  subpanel_generic *p=in;
  if(widget==p->subpanel_windowbutton){
    active=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->subpanel_windowbutton));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->mainpanel_windowbutton),active);
  }else{
    active=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->mainpanel_windowbutton));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->subpanel_windowbutton),active);
  }
  
  if(active)
    gtk_widget_show_all(p->subpanel_toplevel);
  else
    gtk_widget_hide_all(p->subpanel_toplevel);

  return FALSE;
}

static int activebutton_action(GtkWidget *widget,
			gpointer in){
  subpanel_generic *p=in;
  int active;

  if(widget==p->subpanel_activebutton){
    active=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->subpanel_activebutton));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->mainpanel_activebutton),active);
  }else{
    active=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(p->mainpanel_activebutton));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(p->subpanel_activebutton),active);
  }
  
  *p->activevar=active;
  return FALSE;
}

static gboolean rebind_space(GtkWidget *widget,
			     GdkEventKey *event,
			     gpointer in){
  /* do not capture Alt accellerators */
  if(event->state&GDK_MOD1_MASK) return FALSE;
  if(event->state&GDK_CONTROL_MASK) return FALSE;

  if(event->keyval==GDK_space){
    subpanel_generic *p=in;
    GdkEvent copy=*(GdkEvent *)event;
    copy.any.window=p->mainpanel->toplevel->window;
    gtk_main_do_event((GdkEvent *)(&copy));
    return TRUE;
  }
  return FALSE;
}

static gboolean forward_events(GtkWidget *widget,
			       GdkEvent *event,
			       gpointer in){
  subpanel_generic *p=in;
  GdkEvent copy=*(GdkEvent *)event;
  copy.any.window=p->mainpanel->toplevel->window;
  gtk_main_do_event((GdkEvent *)(&copy));
  return TRUE;
}

subpanel_generic *subpanel_create(postfish_mainpanel *mp,
				  GtkWidget *windowbutton,
				  GtkWidget *activebutton,
				  sig_atomic_t *activevar,
				  char *prompt,char *shortcut){

  subpanel_generic *panel=calloc(1,sizeof(*panel));
  GdkWindow *root=gdk_get_default_root_window();
  GtkWidget *topframe=gtk_frame_new (NULL);

  GtkWidget *toplabelbox=gtk_event_box_new();
  GtkWidget *toplabelframe=gtk_frame_new(NULL);
  GtkWidget *toplabel=gtk_hbox_new(0,0);
  GtkWidget *toplabelwb=windowbutton_new(prompt);
  GtkWidget *toplabelab=0;
  
  if(activebutton && activevar)
    toplabelab=gtk_toggle_button_new_with_label(shortcut);
  
  panel->subpanel_windowbutton=toplabelwb;
  panel->subpanel_activebutton=toplabelab;

  panel->mainpanel_windowbutton=windowbutton;
  panel->mainpanel_activebutton=activebutton;
  panel->activevar=activevar;
  
  gtk_box_pack_start(GTK_BOX(toplabel),toplabelwb,0,0,5);
  gtk_box_pack_end(GTK_BOX(toplabel),toplabelab,0,0,5);

  gtk_widget_set_name(toplabelwb,"panelbutton");
  gtk_widget_set_name(toplabelbox,"panelbox");
  gtk_frame_set_shadow_type(GTK_FRAME(toplabelframe),GTK_SHADOW_ETCHED_IN);
  gtk_container_add(GTK_CONTAINER (toplabelbox), toplabelframe);
  gtk_container_add(GTK_CONTAINER (toplabelframe), toplabel);

  panel->subpanel_box=gtk_vbox_new (0,0);

  panel->subpanel_toplevel=gtk_window_new (GTK_WINDOW_TOPLEVEL);
  panel->mainpanel=mp;

  gtk_container_add (GTK_CONTAINER (panel->subpanel_toplevel), topframe);
  gtk_container_add (GTK_CONTAINER (topframe), panel->subpanel_box);
  gtk_container_set_border_width (GTK_CONTAINER (topframe), 3);
  gtk_container_set_border_width (GTK_CONTAINER (panel->subpanel_box), 5);
  gtk_frame_set_shadow_type(GTK_FRAME(topframe),GTK_SHADOW_NONE);
  gtk_frame_set_label_widget(GTK_FRAME(topframe),toplabelbox);

    
  /* space *always* means play/pause */
  g_signal_connect (G_OBJECT (panel->subpanel_toplevel), "key-press-event",
		    G_CALLBACK (rebind_space), 
		    panel);
  /* forward unhandled events to the main window */
  g_signal_connect_after (G_OBJECT (panel->subpanel_toplevel), "key-press-event",
			  G_CALLBACK (forward_events), 
			  panel);

  /* delete should == hide */
  g_signal_connect (G_OBJECT (panel->subpanel_toplevel), "delete-event",
		    G_CALLBACK (clippanel_hide), 
		    panel);

  /* link the mainpanel and subpanel buttons */
  g_signal_connect_after (G_OBJECT (panel->mainpanel_windowbutton), "clicked",
			  G_CALLBACK (windowbutton_action), panel);
  g_signal_connect_after (G_OBJECT (panel->subpanel_windowbutton), "clicked",
			  G_CALLBACK (windowbutton_action), panel);
  if(activebutton && activevar){
    g_signal_connect_after (G_OBJECT (panel->mainpanel_activebutton), "clicked",
			    G_CALLBACK (activebutton_action), panel);
    g_signal_connect_after (G_OBJECT (panel->subpanel_activebutton), "clicked",
			    G_CALLBACK (activebutton_action), panel);
  }

  gtk_window_set_resizable(GTK_WINDOW(panel->subpanel_toplevel),0);

  return panel;
}

