#include <stdio.h>
#include <stdlib.h>
#include "multibar.h"

static void draw(GtkWidget *widget,double *lowvals, double *highvals, int n){
  int i,j;
  Multibar *m=MULTIBAR(widget);
  double max=m->peak;

  if(!m->boxcolor){
    m->boxcolor=gdk_gc_new(m->backing);
    gdk_gc_copy(m->boxcolor,widget->style->black_gc);
  }
  
  if(m->cliptimer.tv_sec){
     struct timeval tv;
    gettimeofday(&tv,NULL);
    
    long val=(tv.tv_sec-m->cliptimer.tv_sec)*1000+(tv.tv_usec-m->cliptimer.tv_usec)/1000;
    if(val>2000)memset(&m->cliptimer,0,sizeof(m->cliptimer));
  }

  if(m->peaktimer.tv_sec){
    struct timeval tv;
    gettimeofday(&tv,NULL);
    
    long val=(tv.tv_sec-m->peaktimer.tv_sec)*1000+(tv.tv_usec-m->peaktimer.tv_usec)/1000;

    if(val>1500) max = m->peak -= (val-1500)*.01;
  }

  for(i=0;i<n;i++)
    if(highvals[i]>=0.){
      gettimeofday(&m->cliptimer,NULL);
      break;
    }
  for(i=0;i<n;i++)
    if(highvals[i]>max)max=highvals[i];

  if(max>m->peak){
    m->peak=max;
    gettimeofday(&m->peaktimer,NULL);
  }

  {
    int x=-1;
    int *pixhi=alloca(n*sizeof(*pixhi));
    int *pixlo=alloca(n*sizeof(*pixlo));
    
    for(i=0;i<n;i++){
      pixlo[i]=-1;
      for(j=0;j<=m->labels;j++)
	if(lowvals[i]>=m->levels[j]){
	  if(lowvals[i]<=m->levels[j+1]){
	    double del=(lowvals[i]-m->levels[j])/(m->levels[j+1]-m->levels[j]);
	    pixlo[i]=(j+del)/m->labels*widget->allocation.width;
	    break;
	  }
	}else
	  break;

      pixhi[i]=pixlo[i];
      for(;j<=m->labels;j++)
	if(highvals[i]>=m->levels[j]){
	  if(highvals[i]<=m->levels[j+1]){
	    double del=(highvals[i]-m->levels[j])/(m->levels[j+1]-m->levels[j]);
	    pixhi[i]=(j+del)/m->labels*widget->allocation.width;
	    break;
	  }
	}else
	  break;

    }

    /* dampen movement according to setup */
    if(n>m->bars){
      if(!m->bartrackers)
	m->bartrackers=calloc(n,sizeof(*m->bartrackers));
      else{
	m->bartrackers=realloc(m->bartrackers,
				    n*sizeof(*m->bartrackers));
	memset(m->bartrackers+m->bars,0,
	       sizeof(*m->bartrackers)*(n-m->bars));
      }
      
      for(i=m->bars;i<n;i++){
	m->bartrackers[i].pixelposlo=pixlo[i];
	m->bartrackers[i].pixelposhi=pixhi[i];
	m->bartrackers[i].pixeldeltalo=0;
	m->bartrackers[i].pixeldeltahi=0;
      }

      m->bars=n;
    }else if(n<m->bars)
      m->bars=n;

    for(i=0;i<n;i++){
      double trackhi=m->bartrackers[i].pixelposhi;
      double tracklo=m->bartrackers[i].pixelposlo;
      double delhi=m->bartrackers[i].pixeldeltahi;
      double dello=m->bartrackers[i].pixeldeltalo;

      /* hi */
      if(pixhi[i]>trackhi){
	/* hi attack */
	if(m->dampen_flags & HI_ATTACK){
	  /* damp the attack */
	  if(delhi<0.)
	    delhi=1.;
	  else
	    delhi+=2;
	  pixhi[i]=trackhi+delhi;
	}else
	  if(delhi<0.)delhi=0.;
	
      }else{
	/* hi decay */
	if(m->dampen_flags & HI_DECAY){
	  /* damp the decay */
	  if(delhi>0.)
	    delhi=-1.;
	  else
	    delhi-=2;
	  pixhi[i]=trackhi+delhi;
	}else
	  if(delhi>0.)delhi=0.;

      }
      m->bartrackers[i].pixelposhi=pixhi[i];
      m->bartrackers[i].pixeldeltahi=delhi;

      /* lo */
      if(pixlo[i]>tracklo){
	/* lo attack */
	if(m->dampen_flags & LO_ATTACK){
	  /* damp the attack */
	  if(dello<0.)
	    dello=1.;
	  else
	    dello+=2;
	  pixlo[i]=tracklo+dello;
	}else
	  if(dello<0.)dello=0.;

      }else{
	/* lo decay */
	if(m->dampen_flags & LO_DECAY){
	  /* damp the decay */
	  if(dello>0.)
	    dello=-1.;
	  else
	    dello-=2;
	  pixlo[i]=tracklo+dello;
	}else
	  if(dello>0.)dello=0.;

      }
      m->bartrackers[i].pixelposlo=pixlo[i];
      m->bartrackers[i].pixeldeltalo=dello;

    }

    /* draw the pixel positions */
    while(x<widget->allocation.width){
      int r=0xffff,g=0xffff,b=0xffff;
      GdkColor rgb={0,0,0,0};
      int next=widget->allocation.width;
      for(i=0;i<n;i++){
	if(pixlo[i]>x && pixlo[i]<next)next=pixlo[i];
	if(pixhi[i]>x && pixhi[i]<next)next=pixhi[i];
      }
      
      if(m->cliptimer.tv_sec!=0){
	g=0x6000;
	b=0x6000;
      }

      for(i=0;i<n;i++){
	if(pixlo[i]<=x && pixhi[i]>=next){
	  switch(i%8){
	  case 0:
	    r*=.55;
	    g*=.55;
	    b*=.55;
	    break;
	  case 1:
	    r*=.8;
	    g*=.5;
	    b*=.5;
	    break;
	  case 2:
	    r*=.6;
	    g*=.6;
	    b*=.9;
	    break;
	  case 3:
	    r*=.4;
	    g*=.7;
	    b*=.4;
	    break;
	  case 4:
	    r*=.6;
	    g*=.5;
	    b*=.3;
	    break;
	  case 5:
	    r*=.6;
	    g*=.4;
	    b*=.7;
	    break;
	  case 6:
	    r*=.3;
	    g*=.6;
	    b*=.6;
	    break;
	  case 7:
	    r*=.4;
	    g*=.4;
	    b*=.4;
	    break;
	  }
	}
      }
      rgb.red=r;
      rgb.green=g;
      rgb.blue=b;
      gdk_gc_set_rgb_fg_color(m->boxcolor,&rgb);
      gdk_draw_rectangle(m->backing,m->boxcolor,1,x+1,1,next-x,widget->allocation.height-2);

      x=next;
    }
  }

  gdk_draw_line (m->backing,
		 widget->style->white_gc,
		 0, 0, widget->allocation.width-1, 0);

  gdk_draw_line (m->backing,
		 widget->style->white_gc,
		 0, widget->allocation.height-1, 
		 widget->allocation.width-1, widget->allocation.height-1);

  /* peak follower */
  {
    int x=-10;
    for(j=0;j<=m->labels;j++)
      if(m->peak>=m->levels[j]){
	if(m->peak<=m->levels[j+1]){
	  double del=(m->peak-m->levels[j])/(m->levels[j+1]-m->levels[j]);
	  x=(j+del)/m->labels*widget->allocation.width;
	  break;
	}
      }else
	break;


    gdk_draw_polygon(m->backing,widget->style->fg_gc[0],1,
		     (GdkPoint[]){{x,5},{x+5,0},{x-4,0}},3);
    gdk_draw_polygon(m->backing,widget->style->fg_gc[0],1,
		     (GdkPoint[]){{x,widget->allocation.height-6},
		       {x+5,widget->allocation.height},
			 {x-5,widget->allocation.height}},3);

    
    gdk_draw_line(m->backing,widget->style->fg_gc[1],x,0,x,widget->allocation.height-1);

  }

  for(i=0;i<m->labels;i++){
    int x=widget->allocation.width*(i+1)/m->labels;
    int y=widget->allocation.height;
    int px,py;

    gdk_draw_line (m->backing,
		   widget->style->text_gc[0],
		   x, 0, x, y);

    pango_layout_get_pixel_size(m->layout[i],&px,&py);
    x-=px+2;
    y-=py;
    y/=2;


    gdk_draw_layout (m->backing,
		     widget->style->text_gc[0],
		     x, y,
		     m->layout[i]);

  }

}

static gboolean configure(GtkWidget *widget, GdkEventConfigure *event){
  Multibar *m=MULTIBAR(widget);
  
  if (m->backing)
    gdk_drawable_unref(m->backing);
  
  m->backing = gdk_pixmap_new(widget->window,
			      widget->allocation.width,
			      widget->allocation.height,
			      -1);
  gdk_draw_rectangle(m->backing,widget->style->white_gc,1,0,0,widget->allocation.width,
		     widget->allocation.height);
  
  draw(widget,0,0,0);
  return TRUE;
}

static gboolean expose( GtkWidget *widget, GdkEventExpose *event ){
  Multibar *m=MULTIBAR(widget);
  gdk_draw_drawable(widget->window,
		    widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		    m->backing,
		    event->area.x, event->area.y,
		    event->area.x, event->area.y,
		    event->area.width, event->area.height);
  
  return FALSE;
}

static void size_request (GtkWidget *widget,GtkRequisition *requisition){
  int i,maxx=0,maxy=0,x,y;
  Multibar *m=MULTIBAR(widget);
  for(i=0;i<m->labels;i++){
    pango_layout_get_pixel_size(m->layout[i],&x,&y);

    if(x>maxx)maxx=x;
    if(y>maxy)maxy=y;
  }

  requisition->width = (maxx*2)*m->labels;
  requisition->height = maxy;
}

static GtkDrawingAreaClass *parent_class = NULL;

static void multibar_class_init (MultibarClass *class){
  GtkWidgetClass *widget_class = (GtkWidgetClass*) class;
  parent_class = g_type_class_peek_parent (class);

  widget_class->expose_event = expose;
  widget_class->configure_event = configure;
  widget_class->size_request = size_request;
  //widget_class->button_press_event = gtk_dial_button_press;
  //widget_class->button_release_event = gtk_dial_button_release;
  //widget_class->motion_notify_event = gtk_dial_motion_notify;
}

static void multibar_init (Multibar *m){

  m->layout=0;
  m->peak=-200;

  m->peaktimer=(struct timeval){0,0};
  m->cliptimer=(struct timeval){0,0};
}

GType multibar_get_type (void){
  static GType m_type = 0;
  if (!m_type){
    static const GTypeInfo m_info={
      sizeof (MultibarClass),
      NULL, /* base_init */
      NULL, /* base_finalize */
      (GClassInitFunc) multibar_class_init,
      NULL, /* class_finalize */
      NULL, /* class_data */
      sizeof (Multibar),
      0,
      (GInstanceInitFunc) multibar_init,
      0
    };
    
    m_type = g_type_register_static (GTK_TYPE_DRAWING_AREA, "Multibar", &m_info, 0);
  }

  return m_type;
}

GtkWidget* multibar_new (int n, char **labels, double *levels, int flags){
  int i;
  GtkWidget *ret= GTK_WIDGET (g_object_new (multibar_get_type (), NULL));
  Multibar *m=MULTIBAR(ret);

  /* not the *proper* way to do it, but this is a one-shot */
  m->levels=calloc((n+1),sizeof(*m->levels));
  m->labels=n;
  memcpy(m->levels,levels,(n+1)*sizeof(*levels));

  m->layout=calloc(m->labels,sizeof(*m->layout));
  for(i=0;i<m->labels;i++)
    m->layout[i]=gtk_widget_create_pango_layout(ret,labels[i]);

  m->dampen_flags=flags;
  return ret;
}

void multibar_set(Multibar *m,double *lo, double *hi, int n){
  GtkWidget *widget=GTK_WIDGET(m);
  draw(widget,lo,hi,n);
  gdk_draw_drawable(widget->window,
		    widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
		    m->backing,
		    0, 0,
		    0, 0,
		    widget->allocation.width,		  
		    widget->allocation.height);
  
}
