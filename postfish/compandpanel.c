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
#include "readout.h"
#include "multibar.h"
#include "mainpanel.h"
#include "subpanel.h"
#include "feedback.h"
#include "multicompand.h"
#include "compandpanel.h"

extern sig_atomic_t compand_active;
extern sig_atomic_t compand_visible;
extern int input_ch;
extern int input_size;
extern int input_rate;

extern banked_compand_settings bc[multicomp_banks];
extern other_compand_settings c;

typedef struct {
  GtkWidget *label;
  GtkWidget *slider;
  GtkWidget *readouto;
  GtkWidget *readoutu;
  int number;
} cbar;

typedef struct {
  Readout *r0;
  Readout *r1;
  Readout *r2;
} multireadout;

static int bank_active=2;
static cbar bars[multicomp_freqs_max+1];
static int inactive_updatep=1;

static void under_compand_change(GtkWidget *w,gpointer in){
  char buffer[80];
  Readout *r=(Readout *)in;
  float val=1./multibar_get_value(MULTIBAR(w),0);

  if(val>=10){
    sprintf(buffer,"%4.1f:1",val);
  }else if(val>=1){
    sprintf(buffer,"%4.2f:1",val);
  }else if(val>.10001){
    sprintf(buffer,"1:%4.2f",1./val);
  }else{
    sprintf(buffer,"1:%4.1f",1./val);
  }

  readout_set(r,buffer);
  
  c.under_ratio=rint(val*1000.);
}

static void over_compand_change(GtkWidget *w,gpointer in){
  char buffer[80];
  Readout *r=(Readout *)in;
  float val=1./multibar_get_value(MULTIBAR(w),0);

  if(val>=10){
    sprintf(buffer,"%4.1f:1",val);
  }else if(val>=1){
    sprintf(buffer,"%4.2f:1",val);
  }else if(val>.10001){
    sprintf(buffer,"1:%4.2f",1./val);
  }else{
    sprintf(buffer,"1:%4.1f",1./val);
  }

  readout_set(r,buffer);
  
  c.over_ratio=rint(val*1000.);
}

static void suppress_compand_change(GtkWidget *w,gpointer in){
  char buffer[80];
  Readout *r=(Readout *)in;
  float val=1./multibar_get_value(MULTIBAR(w),0);

  if(val>.10001){
    sprintf(buffer,"1:%4.2f",1./val);
  }else{
    sprintf(buffer,"1:%4.1f",1./val);
  }

  readout_set(r,buffer);
  
  c.suppress_ratio=rint(val*1000.);
}

static void under_limit_change(GtkWidget *w,gpointer in){
  char buffer[80];
  Readout *r=(Readout *)in;
  float val=140.-multibar_get_value(MULTIBAR(w),0);

  sprintf(buffer,"%3ddB",(int)rint(val));
  readout_set(r,buffer);
  
  c.under_limit=rint(val*10.);
}

static void over_limit_change(GtkWidget *w,gpointer in){
  char buffer[80];
  Readout *r=(Readout *)in;
  float val=140.-multibar_get_value(MULTIBAR(w),0);

  sprintf(buffer,"%3ddB",(int)rint(val));
  readout_set(r,buffer);
  
  c.over_limit=rint(val*10.);
}

static void under_timing_change(GtkWidget *w,gpointer in){
  char buffer[80];
  multireadout *r=(multireadout *)in;
  float attack=multibar_get_value(MULTIBAR(w),0);
  float decay=multibar_get_value(MULTIBAR(w),1);

  if(attack<10){
    sprintf(buffer,"%4.2fms",attack);
  }else if(attack<100){
    sprintf(buffer,"%4.1fms",attack);
  }else if (attack<1000){
    sprintf(buffer,"%4.0fms",attack);
  }else if (attack<10000){
    sprintf(buffer,"%4.2fs",attack/1000.);
  }else{
    sprintf(buffer,"%4.1fs",attack/1000.);
  }

  readout_set(r->r0,buffer);
  
  if(decay<10){
    sprintf(buffer,"%4.2fms",decay);
  }else if (decay<100){
    sprintf(buffer,"%4.1fms",decay);
  }else if (decay<1000){
    sprintf(buffer,"%4.0fms",decay);
  }else if (decay<10000){
    sprintf(buffer,"%4.2fs",decay/1000.);
  }else{
    sprintf(buffer,"%4.1fs",decay/1000.);
  }

  readout_set(r->r1,buffer);
  
  multicompand_under_attack_set(attack);
  multicompand_under_decay_set(decay);
  
}

static void over_timing_change(GtkWidget *w,gpointer in){
  char buffer[80];
  multireadout *r=(multireadout *)in;
  float attack=multibar_get_value(MULTIBAR(w),0);
  float decay=multibar_get_value(MULTIBAR(w),1);

  if(attack<10){
    sprintf(buffer,"%4.2fms",attack);
  }else if(attack<100){
    sprintf(buffer,"%4.1fms",attack);
  }else if (attack<1000){
    sprintf(buffer,"%4.0fms",attack);
  }else if (attack<10000){
    sprintf(buffer,"%4.2fs",attack/1000.);
  }else{
    sprintf(buffer,"%4.1fs",attack/1000.);
  }

  readout_set(r->r0,buffer);
  
  if(decay<10){
    sprintf(buffer,"%4.2fms",decay);
  }else if (decay<100){
    sprintf(buffer,"%4.1fms",decay);
  }else if (decay<1000){
    sprintf(buffer,"%4.0fms",decay);
  }else if (decay<10000){
    sprintf(buffer,"%4.2fs",decay/1000.);
  }else{
    sprintf(buffer,"%4.1fs",decay/1000.);
  }

  readout_set(r->r1,buffer);
  
  multicompand_over_attack_set(attack);
  multicompand_over_decay_set(decay);
  
}

static void suppress_timing_change(GtkWidget *w,gpointer in){
  char buffer[80];
  multireadout *r=(multireadout *)in;
  float attack=multibar_get_value(MULTIBAR(w),0);
  float decay=multibar_get_value(MULTIBAR(w),1);
  float release=multibar_get_value(MULTIBAR(w),2);

  if(attack<10){
    sprintf(buffer,"%4.2fms",attack);
  }else if(attack<100){
    sprintf(buffer,"%4.1fms",attack);
  }else if (attack<1000){
    sprintf(buffer,"%4.0fms",attack);
  }else if (attack<10000){
    sprintf(buffer,"%4.2fs",attack/1000.);
  }else{
    sprintf(buffer,"%4.1fs",attack/1000.);
  }

  readout_set(r->r0,buffer);
  
  if(decay<10){
    sprintf(buffer,"%4.2fms",decay);
  }else if (decay<100){
    sprintf(buffer,"%4.1fms",decay);
  }else if (decay<1000){
    sprintf(buffer,"%4.0fms",decay);
  }else if (decay<10000){
    sprintf(buffer,"%4.2fs",decay/1000.);
  }else{
    sprintf(buffer,"%4.1fs",decay/1000.);
  }

  readout_set(r->r1,buffer);

  if(release<10){
    sprintf(buffer,"%4.2fms",release);
  }else if (release<100){
    sprintf(buffer,"%4.1fms",release);
  }else if (release<1000){
    sprintf(buffer,"%4.0fms",release);
  }else if (release<10000){
    sprintf(buffer,"%4.2fs",release/1000.);
  }else{
    sprintf(buffer,"%4.1fs",release/1000.);
  }

  readout_set(r->r2,buffer);
  
  multicompand_suppress_attack_set(attack);
  multicompand_suppress_decay_set(decay);
  multicompand_suppress_release_set(release);
  
}

static void under_lookahead_change(GtkWidget *w,gpointer in){
  char buffer[80];
  Readout *r=(Readout *)in;
  float val=multibar_get_value(MULTIBAR(w),0);

  sprintf(buffer,"%5.1f%%",val);
  readout_set(r,buffer);
  
  c.under_lookahead=rint(val*10.);
}

static void over_lookahead_change(GtkWidget *w,gpointer in){
  char buffer[80];
  Readout *r=(Readout *)in;
  float val=multibar_get_value(MULTIBAR(w),0);

  sprintf(buffer,"%5.1f%%",val);
  readout_set(r,buffer);
  
  c.over_lookahead=rint(val*10.);
}


static int updating_av_slider=0;
static void average_change(GtkWidget *w,gpointer in){
  char buffer[80];
  cbar *b=bars+multicomp_freqs_max;
  float o,u;
  float oav=0,uav=0;
  int i;

  u=rint(multibar_get_value(MULTIBAR(b->slider),0));
  o=rint(multibar_get_value(MULTIBAR(b->slider),1));

  /* compute the current average */
  for(i=0;i<multicomp_freqs[bank_active];i++){
    oav+=rint(bc[bank_active].static_o[i]/10.);
    uav+=rint(bc[bank_active].static_u[i]/10.);
  }
  oav=rint(oav/multicomp_freqs[bank_active]);
  uav=rint(uav/multicomp_freqs[bank_active]);

  u-=uav;
  o-=oav;

  if(!updating_av_slider){
    updating_av_slider=1;
    if(o!=0.){
      /* update o sliders */
      for(i=0;i<multicomp_freqs[bank_active];i++){
	float val=multibar_get_value(MULTIBAR(bars[i].slider),1);
	multibar_thumb_set(MULTIBAR(bars[i].slider),val+o,1);
      }
      /* update u average (might have pushed it) */
      uav=0;
      for(i=0;i<multicomp_freqs[bank_active];i++)
	uav+=rint(bc[bank_active].static_u[i]/10.);
      uav=rint(uav/multicomp_freqs[bank_active]);
      multibar_thumb_set(MULTIBAR(bars[multicomp_freqs_max].slider),uav,0);
    }else{
      /* update u sliders */
      for(i=0;i<multicomp_freqs[bank_active];i++){
	float val=multibar_get_value(MULTIBAR(bars[i].slider),0);
	multibar_thumb_set(MULTIBAR(bars[i].slider),val+u,0);
      }
      /* update o average (might have pushed it) */
      oav=0;
      for(i=0;i<multicomp_freqs[bank_active];i++)
	oav+=rint(bc[bank_active].static_o[i]/10.);
      oav=rint(oav/multicomp_freqs[bank_active]);
      multibar_thumb_set(MULTIBAR(bars[multicomp_freqs_max].slider),oav,1);
    }
    updating_av_slider=0;
  }
}

static void slider_change(GtkWidget *w,gpointer in){
  char buffer[80];
  cbar *b=(cbar *)in;
  float o,u;
  int i;

  u=multibar_get_value(MULTIBAR(b->slider),0);
  sprintf(buffer,"%+4.0fdB",u);
  readout_set(READOUT(b->readoutu),buffer);
  u=rint(u*10.);
  bc[bank_active].static_u[b->number]=u;
  
  o=multibar_get_value(MULTIBAR(b->slider),1);
  sprintf(buffer,"%+4.0fdB",o);
  readout_set(READOUT(b->readouto),buffer);
  o=rint(o*10.);
  bc[bank_active].static_o[b->number]=o;

  if(inactive_updatep){
    /* keep the inactive banks also tracking settings, but only where it
       makes sense */
    
    switch(bank_active){
    case 0:
      bc[1].static_o[b->number*2]=o;
      bc[1].static_u[b->number*2]=u;
      bc[2].static_o[b->number*3+1]=o;
      bc[2].static_u[b->number*3+1]=u;
      break;
    case 1:
      if((b->number&1)==0){
	bc[0].static_o[b->number>>1]=o;
	bc[0].static_u[b->number>>1]=u;
	bc[2].static_o[b->number/2*3+1]=o;
	bc[2].static_u[b->number/2*3+1]=u;
      }else{
	if(b->number<19){
	  float val=(bc[2].static_o[b->number/2*3+2]+
		     bc[2].static_o[b->number/2*3+3])*.5;
	  bc[2].static_o[b->number/2*3+2]+=(o-val);
	  bc[2].static_o[b->number/2*3+3]+=(o-val);
	  
	  val=(bc[2].static_u[b->number/2*3+2]+
	       bc[2].static_u[b->number/2*3+3])*.5;
	  bc[2].static_u[b->number/2*3+2]+=(u-val);
	  bc[2].static_u[b->number/2*3+3]+=(u-val);
	  
	}else{
	  bc[2].static_o[b->number/2*3+2]=o;
	  bc[2].static_u[b->number/2*3+2]=u;
	}
      }
      break;
    case 2:
      if((b->number%3)==1){
	bc[0].static_o[b->number/3]=o;
	bc[0].static_u[b->number/3]=u;
	bc[1].static_o[b->number/3*2]=o;
	bc[1].static_u[b->number/3*2]=u;
      }else if(b->number>1){
	if(b->number<29){
	  bc[1].static_o[(b->number-1)/3*2+1]=
	    (bc[2].static_o[(b->number-1)/3*3+2]+
	     bc[2].static_o[(b->number-1)/3*3+3])*.5;
	  bc[1].static_u[(b->number-1)/3*2+1]=
	    (bc[2].static_u[(b->number-1)/3*3+2]+
	     bc[2].static_u[(b->number-1)/3*3+3])*.5;
	}else{
	  bc[1].static_o[(b->number-1)/3*2+1]=o;
	  bc[1].static_u[(b->number-1)/3*2+1]=u;
	}
      }
      break;
    }
  }

  /* update average slider */
  if(!updating_av_slider && bars[multicomp_freqs_max].slider){
    float oav=0,uav=0;
    updating_av_slider=1;
    
    /* compute the current average */
    for(i=0;i<multicomp_freqs[bank_active];i++){
      oav+=rint(bc[bank_active].static_o[i]/10.);
      uav+=rint(bc[bank_active].static_u[i]/10.);
    }
    oav=rint(oav/multicomp_freqs[bank_active]);
    uav=rint(uav/multicomp_freqs[bank_active]);
    
    multibar_thumb_set(MULTIBAR(bars[multicomp_freqs_max].slider),uav,0);
    multibar_thumb_set(MULTIBAR(bars[multicomp_freqs_max].slider),oav,1);
    updating_av_slider=0;
  }
}

static void static_octave(GtkWidget *w,gpointer in){
  int octave=(int)in,i;

  if(!w || gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w))){
    if(bank_active!=octave || w==NULL){
      bank_active=octave;
      
      /* map, unmap, relabel */
      for(i=0;i<multicomp_freqs_max;i++){
	if(i<multicomp_freqs[bank_active]){
	  gtk_label_set_text(GTK_LABEL(bars[i].label),
			     multicomp_freq_labels[bank_active][i]);
	  gtk_widget_show(bars[i].label);
	  gtk_widget_show(bars[i].slider);
	  gtk_widget_show(bars[i].readouto);
	  gtk_widget_show(bars[i].readoutu);
	  
	  inactive_updatep=0;
	  
	  {
	    float o=bc[bank_active].static_o[i]/10.;
	    float u=bc[bank_active].static_u[i]/10.;

	    multibar_thumb_set(MULTIBAR(bars[i].slider),u,0);
	    multibar_thumb_set(MULTIBAR(bars[i].slider),o,1);
	  }

	  inactive_updatep=1;
	  
	}else{
	  gtk_widget_hide(bars[i].label);
	  gtk_widget_hide(bars[i].slider);
	  gtk_widget_hide(bars[i].readouto);
	  gtk_widget_hide(bars[i].readoutu);
	}
      }
      multicompand_set_bank(bank_active);
      
    }
  }
}

static void link_toggled(GtkToggleButton *b,gpointer in){
  int active=gtk_toggle_button_get_active(b);
  c.link_mode=active;
}

static void over_mode(GtkButton *b,gpointer in){
  int mode=(int)in;
  c.over_mode=mode;
}

static void under_mode(GtkButton *b,gpointer in){
  int mode=(int)in;
  c.under_mode=mode;
}

static void under_knee(GtkButton *b,gpointer in){
  int mode=(int)in;
  c.under_softknee=mode;
}

static void over_knee(GtkButton *b,gpointer in){
  int mode=(int)in;
  c.over_softknee=mode;
}

static void suppress_mode(GtkButton *b,gpointer in){
  int mode=(int)in;
  c.suppress_mode=mode;
}

void compandpanel_create(postfish_mainpanel *mp,
			 GtkWidget *windowbutton,
			 GtkWidget *activebutton){
  int i,j;
  char *labels[14]={"130","120","110","100","90","80","70",
		    "60","50","40","30","20","10","0"};
  float levels[15]={-140,-130,-120,-110,-100,-90,-80,-70,-60,-50,-40,
		     -30,-20,-10,0};

  float compand_levels[9]={.1,.25,.5,.667,1,1.5,2,4,10};
  char  *compand_labels[8]={"4:1","2:1","1:1.5","1:1","1:1.5","1:2","1:4","1:10"};

  float compand_levels_low[5]={1,1.2,1.5,2,3};
  char  *compand_labels_low[4]={"1:1.2","1:1.5","1:2","1:3"};

  float timing_levels[6]={.5,1,10,100,1000,10000};
  char  *timing_labels[5]={"1ms","10ms","100ms","1s","10s"};

  float limit_levels[9]={0,40,60,80,100,110,120,130,135};
  char  *limit_labels[8]={"100dB","80dB","60dB","40dB","30dB","20dB","10dB","5dB"};

  float per_levels[9]={0,12.5,25,37.5,50,62.5,75,87.5,100};
  char  *per_labels[8]={"","25%","","50%","","75%","","100%"};


  subpanel_generic *panel=subpanel_create(mp,windowbutton,activebutton,
					  &compand_active,
					  &compand_visible,
					  "_Multiband Compand"," [m] ");
  
  GtkWidget *hbox=gtk_hbox_new(0,0);
  GtkWidget *sliderbox=gtk_vbox_new(0,0);
  GtkWidget *sliderframe=gtk_frame_new(NULL);
  GtkWidget *staticbox=gtk_vbox_new(0,0);
  GtkWidget *slidertable=gtk_table_new(multicomp_freqs_max+2,4,0);

  GtkWidget *overlabel=gtk_label_new("Over threshold compand");
  GtkWidget *overtable=gtk_table_new(6,4,0);

  GtkWidget *underlabel=gtk_label_new("Under threshold compand");
  GtkWidget *undertable=gtk_table_new(5,4,0);

  GtkWidget *suppressframe=gtk_frame_new(" Reverb suppression ");
  GtkWidget *suppresstable=gtk_table_new(4,5,0);

  GtkWidget *link_box=gtk_hbox_new(0,0);
  GtkWidget *link_check=gtk_check_button_new_with_mnemonic("_link channel envelopes");

  gtk_widget_set_name(overlabel,"framelabel");
  gtk_widget_set_name(underlabel,"framelabel");
  gtk_container_set_border_width(GTK_CONTAINER(suppresstable),4);

  
    
  {
  
    GtkWidget *octave_box=gtk_hbox_new(0,0);
    GtkWidget *octave_a=gtk_radio_button_new_with_label(NULL,"full-octave");
    GtkWidget *octave_b=
      gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(octave_a),
						  "half-octave");
    GtkWidget *octave_c=
      gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(octave_b),
						  "third-octave");
    GtkWidget *label2=gtk_label_new("under");
    GtkWidget *label3=gtk_label_new("over");

    gtk_misc_set_alignment(GTK_MISC(label2),.5,1.);
    gtk_misc_set_alignment(GTK_MISC(label3),.5,1.);
    gtk_widget_set_name(label2,"scalemarker");
    gtk_widget_set_name(label3,"scalemarker");


    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(octave_c),1);

    g_signal_connect (G_OBJECT (octave_a), "clicked",
		      G_CALLBACK (static_octave), (gpointer)0);
    g_signal_connect (G_OBJECT (octave_b), "clicked",
		      G_CALLBACK (static_octave), (gpointer)1);
    g_signal_connect (G_OBJECT (octave_c), "clicked",
		      G_CALLBACK (static_octave), (gpointer)2);
   
    gtk_table_attach(GTK_TABLE(slidertable),label2,1,2,0,1,GTK_FILL,GTK_FILL|GTK_EXPAND,2,0);
    gtk_table_attach(GTK_TABLE(slidertable),label3,3,4,0,1,GTK_FILL,GTK_FILL|GTK_EXPAND,2,0);
 
    gtk_box_pack_start(GTK_BOX(octave_box),octave_a,0,1,3);
    gtk_box_pack_start(GTK_BOX(octave_box),octave_b,0,1,3);
    gtk_box_pack_start(GTK_BOX(octave_box),octave_c,0,1,3);
    gtk_table_attach(GTK_TABLE(slidertable),octave_box,2,3,0,1,
		     GTK_EXPAND,0,0,0);

    gtk_box_pack_start(GTK_BOX(sliderbox),sliderframe,0,0,0);
    gtk_container_add(GTK_CONTAINER(sliderframe),slidertable);

    gtk_frame_set_shadow_type(GTK_FRAME(sliderframe),GTK_SHADOW_NONE);
    
    gtk_container_set_border_width(GTK_CONTAINER(sliderframe),4);

  }

  if(input_ch>1)
    gtk_box_pack_end(GTK_BOX(link_box),link_check,0,0,0);  

  gtk_box_pack_end(GTK_BOX(staticbox),link_box,0,0,0);
  gtk_container_set_border_width(GTK_CONTAINER(link_box),5);
  g_signal_connect (G_OBJECT (link_check), "toggled",
		    G_CALLBACK (link_toggled), (gpointer)0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(link_check),0);

  gtk_box_pack_start(GTK_BOX(panel->subpanel_box),hbox,0,0,0);
  gtk_box_pack_start(GTK_BOX(hbox),sliderbox,0,0,0);
  gtk_box_pack_start(GTK_BOX(hbox),staticbox,0,0,0);

  gtk_box_pack_start(GTK_BOX(staticbox),overtable,0,0,0);
  gtk_box_pack_start(GTK_BOX(staticbox),undertable,0,0,10);
 

  gtk_box_pack_end(GTK_BOX(staticbox),suppressframe,0,0,0);
  gtk_container_add(GTK_CONTAINER(suppressframe),suppresstable);

  gtk_container_set_border_width(GTK_CONTAINER(suppressframe),5);
  gtk_container_set_border_width(GTK_CONTAINER(overtable),5);
  gtk_container_set_border_width(GTK_CONTAINER(undertable),5);

  /* under compand: mode and knee */
  {
    GtkWidget *envelopebox=gtk_hbox_new(0,0);
    GtkWidget *rms_button=gtk_radio_button_new_with_label(NULL,"RMS");
    GtkWidget *peak_button=
      gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(rms_button),
						  "peak");
    GtkWidget *knee_button=gtk_check_button_new_with_label("soft knee");

    gtk_box_pack_start(GTK_BOX(envelopebox),underlabel,0,0,5);
    gtk_box_pack_end(GTK_BOX(envelopebox),peak_button,0,0,5);
    gtk_box_pack_end(GTK_BOX(envelopebox),rms_button,0,0,5);
    gtk_box_pack_end(GTK_BOX(envelopebox),knee_button,0,0,5);

    g_signal_connect (G_OBJECT (knee_button), "clicked",
		      G_CALLBACK (under_knee), (gpointer)0);
    g_signal_connect (G_OBJECT (rms_button), "clicked",
		      G_CALLBACK (under_mode), (gpointer)0);
    g_signal_connect (G_OBJECT (peak_button), "clicked",
		      G_CALLBACK (under_mode), (gpointer)1); //To Hell I Go
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rms_button),1);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(knee_button),1);
    gtk_table_attach(GTK_TABLE(undertable),envelopebox,0,4,0,1,GTK_FILL,0,0,0);
  }

  /* under compand: ratio */
  {

    GtkWidget *label=gtk_label_new("compand ratio:");
    GtkWidget *readout=readout_new("1.55:1");
    GtkWidget *slider=multibar_slider_new(8,compand_labels,compand_levels,1);
   
    multibar_callback(MULTIBAR(slider),under_compand_change,readout);
    multibar_thumb_set(MULTIBAR(slider),1.,0);
    multibar_thumb_increment(MULTIBAR(slider),.01,.1);

    gtk_misc_set_alignment(GTK_MISC(label),1.,.5);

    gtk_table_set_row_spacing(GTK_TABLE(undertable),0,4);
    gtk_table_attach(GTK_TABLE(undertable),label,0,1,1,2,GTK_FILL,0,2,0);
    gtk_table_attach(GTK_TABLE(undertable),slider,1,3,1,2,GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND,2,0);
    gtk_table_attach(GTK_TABLE(undertable),readout,3,4,1,2,GTK_FILL,0,0,0);

  }

  /* under compand: limit */
  {

    GtkWidget *label=gtk_label_new("compand limit:");
    GtkWidget *readout=readout_new("140dB");
    GtkWidget *slider=multibar_slider_new(8,limit_labels,limit_levels,1);
   
    multibar_callback(MULTIBAR(slider),under_limit_change,readout);
    multibar_thumb_set(MULTIBAR(slider),0.,0);
    multibar_thumb_increment(MULTIBAR(slider),1.,10.);

    gtk_misc_set_alignment(GTK_MISC(label),1,.5);
    
    gtk_table_set_row_spacing(GTK_TABLE(undertable),1,4);
    gtk_table_attach(GTK_TABLE(undertable),label,0,1,2,3,GTK_FILL,0,2,0);
    gtk_table_attach(GTK_TABLE(undertable),slider,1,3,2,3,GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND,2,0);
    gtk_table_attach(GTK_TABLE(undertable),readout,3,4,2,3,GTK_FILL,0,0,0);
  }

  /* under compand: timing */
  {

    GtkWidget *label=gtk_label_new("attack/decay:");
    GtkWidget *readout1=readout_new(" 100ms");
    GtkWidget *readout2=readout_new(" 100ms");
    GtkWidget *slider=multibar_slider_new(5,timing_labels,timing_levels,2);
    multireadout *r=calloc(1,sizeof(*r));

    r->r0=(Readout *)readout1;
    r->r1=(Readout *)readout2;
   
    multibar_callback(MULTIBAR(slider),under_timing_change,r);
    multibar_thumb_set(MULTIBAR(slider),1,0);
    multibar_thumb_set(MULTIBAR(slider),100,1);

    gtk_misc_set_alignment(GTK_MISC(label),1,.5);

    gtk_table_set_row_spacing(GTK_TABLE(undertable),2,4);
    gtk_table_attach(GTK_TABLE(undertable),label,0,1,4,5,GTK_FILL,0,2,0);
    gtk_table_attach(GTK_TABLE(undertable),slider,1,2,4,5,GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND,2,0);
    gtk_table_attach(GTK_TABLE(undertable),readout1,2,3,4,5,GTK_FILL,0,0,0);
    gtk_table_attach(GTK_TABLE(undertable),readout2,3,4,4,5,GTK_FILL,0,0,0);

  }

  /* under compand: lookahead */
  {

    GtkWidget *label=gtk_label_new("lookahead:");
    GtkWidget *readout=readout_new("100%");
    GtkWidget *slider=multibar_slider_new(8,per_labels,per_levels,1);
    GtkWidget *labelbox=gtk_hbox_new(0,0);
    
    multibar_callback(MULTIBAR(slider),under_lookahead_change,readout);
    multibar_thumb_set(MULTIBAR(slider),100.,0);
    multibar_thumb_increment(MULTIBAR(slider),1.,10.);

    gtk_misc_set_alignment(GTK_MISC(label),1,.5);
    
    gtk_table_set_row_spacing(GTK_TABLE(undertable),3,4);
    gtk_table_attach(GTK_TABLE(undertable),label,0,1,3,4,GTK_FILL,0,2,0);
    gtk_table_attach(GTK_TABLE(undertable),slider,1,3,3,4,GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND,2,0);
    gtk_table_attach(GTK_TABLE(undertable),readout,3,4,3,4,GTK_FILL,0,0,0);
  }

  /* over compand: mode and knee */
  {
    GtkWidget *envelopebox=gtk_hbox_new(0,0);
    GtkWidget *rms_button=gtk_radio_button_new_with_label(NULL,"RMS");
    GtkWidget *peak_button=
      gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(rms_button),
						  "peak");
    GtkWidget *knee_button=gtk_check_button_new_with_label("soft knee");

    gtk_box_pack_start(GTK_BOX(envelopebox),overlabel,0,0,5);
    gtk_box_pack_end(GTK_BOX(envelopebox),peak_button,0,0,5);
    gtk_box_pack_end(GTK_BOX(envelopebox),rms_button,0,0,5);
    gtk_box_pack_end(GTK_BOX(envelopebox),knee_button,0,0,5);

    g_signal_connect (G_OBJECT (knee_button), "clicked",
		      G_CALLBACK (over_knee), (gpointer)0);
    g_signal_connect (G_OBJECT (rms_button), "clicked",
		      G_CALLBACK (over_mode), (gpointer)0);
    g_signal_connect (G_OBJECT (peak_button), "clicked",
		      G_CALLBACK (over_mode), (gpointer)1); //To Hell I Go
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rms_button),1);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(knee_button),1);
    gtk_table_attach(GTK_TABLE(overtable),envelopebox,0,4,0,1,GTK_FILL,0,0,0);
  }

  /* over compand: ratio */
  {

    GtkWidget *label=gtk_label_new("compand ratio:");
    GtkWidget *readout=readout_new("1.55:1");
    GtkWidget *slider=multibar_slider_new(8,compand_labels,compand_levels,1);
   
    multibar_callback(MULTIBAR(slider),over_compand_change,readout);
    multibar_thumb_set(MULTIBAR(slider),1.,0);
    multibar_thumb_increment(MULTIBAR(slider),.01,.1);

    gtk_misc_set_alignment(GTK_MISC(label),1.,.5);

    gtk_table_set_row_spacing(GTK_TABLE(overtable),0,4);
    gtk_table_attach(GTK_TABLE(overtable),label,0,1,1,2,GTK_FILL,0,2,0);
    gtk_table_attach(GTK_TABLE(overtable),slider,1,3,1,2,GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND,2,0);
    gtk_table_attach(GTK_TABLE(overtable),readout,3,4,1,2,GTK_FILL,0,0,0);

  }

  /* over compand: limit */
  {

    GtkWidget *label=gtk_label_new("compand limit:");
    GtkWidget *readout=readout_new("140dB");
    GtkWidget *slider=multibar_slider_new(8,limit_labels,limit_levels,1);
   
    multibar_callback(MULTIBAR(slider),over_limit_change,readout);
    multibar_thumb_set(MULTIBAR(slider),0.,0);
    multibar_thumb_increment(MULTIBAR(slider),1.,10.);

    gtk_misc_set_alignment(GTK_MISC(label),1,.5);
    
    gtk_table_set_row_spacing(GTK_TABLE(overtable),1,4);
    gtk_table_attach(GTK_TABLE(overtable),label,0,1,2,3,GTK_FILL,0,2,0);
    gtk_table_attach(GTK_TABLE(overtable),slider,1,3,2,3,GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND,2,0);
    gtk_table_attach(GTK_TABLE(overtable),readout,3,4,2,3,GTK_FILL,0,0,0);
  }

  /* over compand: timing */
  {

    GtkWidget *label=gtk_label_new("attack/decay:");
    GtkWidget *readout1=readout_new(" 100ms");
    GtkWidget *readout2=readout_new(" 100ms");
    GtkWidget *slider=multibar_slider_new(5,timing_labels,timing_levels,2);
   
    multireadout *r=calloc(1,sizeof(*r));
    r->r0=(Readout *)readout1;
    r->r1=(Readout *)readout2;

    multibar_callback(MULTIBAR(slider),over_timing_change,r);
    multibar_thumb_set(MULTIBAR(slider),1,0);
    multibar_thumb_set(MULTIBAR(slider),100,1);

    gtk_misc_set_alignment(GTK_MISC(label),1,.5);

    gtk_table_set_row_spacing(GTK_TABLE(overtable),2,4);
    gtk_table_attach(GTK_TABLE(overtable),label,0,1,5,6,GTK_FILL,0,2,0);
    gtk_table_attach(GTK_TABLE(overtable),slider,1,2,5,6,GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND,2,0);
    gtk_table_attach(GTK_TABLE(overtable),readout1,2,3,5,6,GTK_FILL,0,0,0);
    gtk_table_attach(GTK_TABLE(overtable),readout2,3,4,5,6,GTK_FILL,0,0,0);

  }

  /* over compand: lookahead */
  {

    GtkWidget *label=gtk_label_new("lookahead:");
    GtkWidget *readout=readout_new("100%");
    GtkWidget *slider=multibar_slider_new(8,per_labels,per_levels,1);
    GtkWidget *labelbox=gtk_hbox_new(0,0);
   
    multibar_callback(MULTIBAR(slider),over_lookahead_change,readout);
    multibar_thumb_set(MULTIBAR(slider),100.,0);
    multibar_thumb_increment(MULTIBAR(slider),1.,10.);

    gtk_misc_set_alignment(GTK_MISC(label),1,.5);
    
    gtk_table_set_row_spacing(GTK_TABLE(overtable),3,4);
    gtk_table_attach(GTK_TABLE(overtable),label,0,1,3,4,GTK_FILL,0,2,0);
    gtk_table_attach(GTK_TABLE(overtable),slider,1,3,3,4,GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND,2,0);
    gtk_table_attach(GTK_TABLE(overtable),readout,3,4,3,4,GTK_FILL,0,0,0);
  }
  
  /* base compand: ratio */
  {

    GtkWidget *label=gtk_label_new("global ratio:");
    GtkWidget *readout=readout_new("1.55:1");
    GtkWidget *slider=multibar_slider_new(8,compand_labels,compand_levels,1);
   
    //multibar_callback(MULTIBAR(slider),over_compand_change,readout);
    multibar_thumb_set(MULTIBAR(slider),1.,0);
    multibar_thumb_increment(MULTIBAR(slider),.01,.1);

    gtk_misc_set_alignment(GTK_MISC(label),1.,.5);

    gtk_table_set_row_spacing(GTK_TABLE(overtable),4,4);

    gtk_table_attach(GTK_TABLE(overtable),label,0,1,4,5,GTK_FILL,0,2,0);
    gtk_table_attach(GTK_TABLE(overtable),slider,1,3,4,5,GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND,2,0);
    gtk_table_attach(GTK_TABLE(overtable),readout,3,4,4,5,GTK_FILL,0,0,0);

  }

  /* suppress: mode  */
  {
    GtkWidget *envelopebox=gtk_hbox_new(0,0);
    GtkWidget *rms_button=gtk_radio_button_new_with_label(NULL,"RMS");
    GtkWidget *peak_button=
      gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(rms_button),
						  "peak");

    gtk_box_pack_end(GTK_BOX(envelopebox),peak_button,0,0,5);
    gtk_box_pack_end(GTK_BOX(envelopebox),rms_button,0,0,5);

    g_signal_connect (G_OBJECT (rms_button), "clicked",
		      G_CALLBACK (suppress_mode), (gpointer)0);
    g_signal_connect (G_OBJECT (peak_button), "clicked",
		      G_CALLBACK (suppress_mode), (gpointer)1); //To Hell I Go
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(rms_button),1);
    gtk_table_attach(GTK_TABLE(suppresstable),envelopebox,1,5,0,1,GTK_FILL,0,0,0);
  }

  /* suppress: ratio */
  {

    GtkWidget *label=gtk_label_new("ratio:");
    GtkWidget *readout=readout_new("1.55:1");
    GtkWidget *slider=multibar_slider_new(4,compand_labels_low,compand_levels_low,1);
    
    multibar_callback(MULTIBAR(slider),suppress_compand_change,readout);
    multibar_thumb_set(MULTIBAR(slider),1.,0);
    multibar_thumb_increment(MULTIBAR(slider),.01,.1);

    gtk_misc_set_alignment(GTK_MISC(label),1.,.5);

    gtk_table_set_row_spacing(GTK_TABLE(suppresstable),0,4);
    gtk_table_attach(GTK_TABLE(suppresstable),label,0,1,1,2,GTK_FILL,0,2,0);
    gtk_table_attach(GTK_TABLE(suppresstable),slider,1,4,1,2,GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND,2,0);
    gtk_table_attach(GTK_TABLE(suppresstable),readout,4,5,1,2,GTK_FILL,0,0,0);

  }

  /* suppress: timing */
  {

    GtkWidget *label=gtk_label_new("timing:");
    GtkWidget *label2=gtk_label_new("attack / decay / release");
    GtkWidget *readout1=readout_new(" 100ms");
    GtkWidget *readout2=readout_new(" 100ms");
    GtkWidget *readout3=readout_new(" 100ms");
    GtkWidget *slider=multibar_slider_new(5,timing_labels,timing_levels,3);

    multireadout *r=calloc(1,sizeof(*r));
    r->r0=(Readout *)readout1;
    r->r1=(Readout *)readout2;
    r->r2=(Readout *)readout3;

    gtk_widget_set_name(label2,"scalemarker");
   
    multibar_callback(MULTIBAR(slider),suppress_timing_change,r);
    multibar_thumb_set(MULTIBAR(slider),1,0);
    multibar_thumb_set(MULTIBAR(slider),100,1);
    multibar_thumb_set(MULTIBAR(slider),1000,2);

    gtk_misc_set_alignment(GTK_MISC(label),1,.5);

    gtk_table_set_row_spacing(GTK_TABLE(suppresstable),1,4);
    gtk_table_attach(GTK_TABLE(suppresstable),label,0,1,2,3,GTK_FILL,0,2,0);
    gtk_table_attach(GTK_TABLE(suppresstable),slider,1,2,2,3,GTK_FILL|GTK_EXPAND,GTK_FILL|GTK_EXPAND,2,0);
    gtk_table_attach(GTK_TABLE(suppresstable),readout1,2,3,2,3,GTK_FILL,0,0,0);
    gtk_table_attach(GTK_TABLE(suppresstable),readout2,3,4,2,3,GTK_FILL,0,0,0);
    gtk_table_attach(GTK_TABLE(suppresstable),readout3,4,5,2,3,GTK_FILL,0,0,0);
    gtk_table_attach(GTK_TABLE(suppresstable),label2,1,2,3,4,GTK_FILL,0,0,0);

  }

  /* threshold controls */

  for(i=0;i<multicomp_freqs_max;i++){
    GtkWidget *label=gtk_label_new(NULL);
    gtk_widget_set_name(label,"scalemarker");
    
    bars[i].readoutu=readout_new("  +0");
    bars[i].readouto=readout_new("  +0");
    bars[i].slider=multibar_new(14,labels,levels,2,
				LO_DECAY|HI_DECAY|LO_ATTACK);
    bars[i].number=i;
    bars[i].label=label;

    multibar_callback(MULTIBAR(bars[i].slider),slider_change,bars+i);
    multibar_thumb_set(MULTIBAR(bars[i].slider),-140.,0);
    multibar_thumb_set(MULTIBAR(bars[i].slider),0.,1);
    multibar_thumb_bounds(MULTIBAR(bars[i].slider),-140,0);
    multibar_thumb_increment(MULTIBAR(bars[i].slider),1.,10.);
    
    gtk_misc_set_alignment(GTK_MISC(label),1,.5);
      
    gtk_table_attach(GTK_TABLE(slidertable),label,0,1,i+1,i+2,
		     GTK_FILL,0,10,0);
    gtk_table_attach(GTK_TABLE(slidertable),bars[i].readoutu,1,2,i+1,i+2,
		     0,0,0,0);
    gtk_table_attach(GTK_TABLE(slidertable),bars[i].slider,2,3,i+1,i+2,
		     GTK_FILL|GTK_EXPAND,GTK_EXPAND,0,0);
    gtk_table_attach(GTK_TABLE(slidertable),bars[i].readouto,3,4,i+1,i+2,
		     0,0,0,0);
  }

  /* "average" slider */
  {
    GtkWidget *label=gtk_label_new("average");
    
    bars[multicomp_freqs_max].slider=multibar_slider_new(14,labels,levels,2);

    multibar_callback(MULTIBAR(bars[multicomp_freqs_max].slider),average_change,bars+multicomp_freqs_max);

    multibar_thumb_set(MULTIBAR(bars[multicomp_freqs_max].slider),-140.,0);
    multibar_thumb_set(MULTIBAR(bars[multicomp_freqs_max].slider),0.,1);
    multibar_thumb_bounds(MULTIBAR(bars[multicomp_freqs_max].slider),-140,0);
    
    gtk_table_attach(GTK_TABLE(slidertable),label,0,2,multicomp_freqs_max+1,multicomp_freqs_max+2,
		     GTK_FILL,0,0,0);
    gtk_table_attach(GTK_TABLE(slidertable),bars[i].slider,2,3,multicomp_freqs_max+1,multicomp_freqs_max+2,
		     GTK_FILL|GTK_EXPAND,GTK_EXPAND,0,0);
    gtk_table_set_row_spacing(GTK_TABLE(slidertable),multicomp_freqs_max,10);
  }

  
  subpanel_show_all_but_toplevel(panel);

  /* Now unmap the sliders we don't want */
  static_octave(NULL,(gpointer)2);

}

static float **peakfeed=0;
static float **rmsfeed=0;

void compandpanel_feedback(int displayit){
  int i,bands;
  if(!peakfeed){
    peakfeed=malloc(sizeof(*peakfeed)*multicomp_freqs_max);
    rmsfeed=malloc(sizeof(*rmsfeed)*multicomp_freqs_max);

    for(i=0;i<multicomp_freqs_max;i++){
      peakfeed[i]=malloc(sizeof(**peakfeed)*input_ch);
      rmsfeed[i]=malloc(sizeof(**rmsfeed)*input_ch);
    }
  }

  if(pull_multicompand_feedback(peakfeed,rmsfeed,&bands)==1)
    for(i=0;i<bands;i++)
      multibar_set(MULTIBAR(bars[i].slider),rmsfeed[i],peakfeed[i],
		   input_ch,(displayit && compand_visible));
}

void compandpanel_reset(void){
  int i,j;
  for(i=0;i<multicomp_freqs_max;i++)
    multibar_reset(MULTIBAR(bars[i].slider));
}


