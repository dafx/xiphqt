#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <pthread.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <curses.h>


#define int16 short
static char *tempdir="/tmp/beaverphonic/";
static char *installdir="/usr/local/beaverphonic/";
#define VERSION "20020128.0"

enum menutype {MENU_MAIN,MENU_KEYPRESS,MENU_ADD,MENU_EDIT,MENU_OUTPUT,MENU_QUIT};

pthread_mutex_t master_mutex=PTHREAD_MUTEX_INITIALIZER;
static int main_master_volume=50;
char *program;
static int playback_buffer_minfill=-1;
static int running=1;
static enum menutype menu=MENU_MAIN;
static int cue_list_position=0;
static int cue_list_number=0;
static int firstsave=0;
static int unsaved=0;

void *m_realloc(void *in,int bytes){
  if(!in)
    return(malloc(bytes));
  return(realloc(in,bytes));
}

void addnlstr(char *s,int n,char c){
  int len=strlen(s),i;
  addnstr(s,n);
  n-=len;
  for(i=0;i<n;i++)addch(c);
}


/******** channel mappings.  All hardwired for now... ***********/
// only OSS stereo builin for now
#define MAX_CHANNELS 2
#define CHANNEL_LABEL_LENGTH 50

typedef struct {
  char label[CHANNEL_LABEL_LENGTH];
  int  peak;
  /* real stuff not here yet */
} outchannel;
  
static outchannel channel_list[MAX_CHANNELS]={
  {"offstage left",0},
  {"offstage right",0},
};
static int channel_count=MAX_CHANNELS;

/******** label abstraction code; use this for all alloced strings
          that need to be saved to config file (shared or not) */

typedef struct {
  char *text;
  int   refcount;
} label;

typedef int label_number;

static label      *label_list;
static int         label_count;

int new_label_number(){
  int i;
  for(i=0;i<label_count;i++)
    if(!label_list[i].refcount)break;
  return(i);
}

static void _alloc_label_if_needed(int number){
  if(number>=label_count){
    int prev=label_count;
    label_count=number+1;
    label_list=m_realloc(label_list,sizeof(*label_list)*label_count);
    memset(label_list+prev,0,sizeof(*label_list)*(label_count-prev));
  }
}

void edit_label(int number,char *t){
  label *label;
  _alloc_label_if_needed(number);

  label=label_list+number;
  label->text=m_realloc(label->text,strlen(t)+1);
  
  strcpy(label->text,t);
}

void aquire_label(int number){
  _alloc_label_if_needed(number);
  label_list[number].refcount++;
}

void release_label(int number){
  label *label;
  if(number>=label_count)return;
  label=label_list+number;
  label->refcount--;
  if(label->refcount<0)
    label->refcount=0;
}

int save_label(FILE *f,int number){
  int count;
  if(number<=label_count){
    if(label_list[number].refcount){
      fprintf(f,"LBL:%d %d:%s",number,
	      strlen(label_list[number].text),
	      label_list[number].text);
      
      count=fprintf(f,"\n");
      if(count<1)return(-1);
    }
  }
  return(0);
}

int save_labels(FILE *f){
  int i;
  for(i=label_count-1;i>=0;i--)
    if(save_label(f,i))return(-1);
  return(0);
}

int load_label(FILE *f){
  int len,count,number;
  char *temp;
  count=fscanf(f,":%d %d:",&number,&len);
  if(count<2){
    addnlstr("LOAD ERROR (LBL): too few fields.",80,' ');
    return(-1);
  }
  temp=alloca(len+1);
  count=fread(temp,1,len,f);
  temp[len]='\0';
  if(count<len){
    addnlstr("LOAD ERROR (LBL): EOF reading string.",80,' ');    
    return(-1);
  }
  edit_label(number,temp);
  return(0);
}

/************* tag abstraction **********************/

typedef int tag_number;

typedef struct {
  int          refcount;

  label_number sample_path;
  int    loop_p;
  long   loop_start;
  long   loop_lapping;
  long   fade_out;

  void  *basemap;
  int16 *channel_vec[2];
  int    channels;
  long   samplelength;

  /* state */
  int    activep;
  
  long   sample_position;
  long   sample_lapping;

  double master_vol_current;
  double master_vol_target;
  double master_vol_slew;

  double outvol_current[2][MAX_CHANNELS];
  double outvol_target[2][MAX_CHANNELS];
  double outvol_slew[2][MAX_CHANNELS];

} tag;

struct sample_header{
  long sync;
  long channels;
  long samples;
};

static tag        *tag_list;
static int         tag_count;

static tag       **active_list;
static int         active_count;

int new_tag_number(){
  int i;
  for(i=0;i<tag_count;i++)
    if(!tag_list[i].refcount)break;
  return(i);
}

static void _alloc_tag_if_needed(int number){
  if(number>=tag_count){
    int prev=tag_count;
    tag_count=number+1;
    tag_list=m_realloc(tag_list,sizeof(*tag_list)*tag_count);
    active_list=m_realloc(active_list,sizeof(*active_list)*tag_count);
    
    memset(tag_list+prev,0,sizeof(*tag_list)*(tag_count-prev));
  }
}

/* UI convention: this has too many useful things to report if sample
   opening goes awry, and I don't want to define a huge message
   reporting system through ncurses in C, so we go back to tty-like
   for reporting here, waiting for a keypress if nonzero return */
int edit_tag(int number,tag t){
  int offset,ret;
  tag *tag;
  _alloc_tag_if_needed(number);
  
  t.basemap=NULL;
  t.channels=0;
  t.samplelength=0;

  tag_list[number]=t;
  tag=tag_list+number;

  /* allow the edit to be accepted if the file cannot be opened, but
     alert the operator */
  {
    /* parse the file; use preexisting tools via external Perl glue */
    /* select a temp file for the glue to convert file into */
    /* valid use of mktemp; only one Beaverphonic is running a time,
       and we need a *name*, not a *file*. */

    char *template=alloca(strlen(tempdir)+20);
    char *tmp;

    sprintf(template,"%sconversionXXXXXX",tempdir);
    tmp=mktemp(template);
    if(!tmp)return(-1);

    {
      char *orig=label_list[t.sample_path].text;
      char *buffer=alloca(strlen(installdir)+strlen(tmp)+strlen(orig)+20);
      
      sprintf(buffer,"%sconvert.pl %s %s",installdir,orig,tmp);
      ret=system(buffer);
      if(ret)return(ret);
    }
	
    /* our temp file is a host-ordered 44.1kHz 16 bit PCM file, n
       channels uninterleaved.  first word is channels */
    {
      FILE *cheat=fopen(tmp,"rb");
      struct sample_header head;
      int count,i;
      int fd;
      if(!cheat){
	fprintf(stderr,"Failed to open converted file %s:\n\t%d (%s)\n",
		tmp,ferror(cheat),strerror(ferror(cheat)));
	return(-1);
      }
      count=fread(&head,sizeof(head),1,cheat);
      fclose(cheat);
      if(count<(int)sizeof(head)){
	fprintf(stderr,"Conversion file %s openable, but truncated\n",tmp);
	return(-1);
      }
      if(head.sync!=55){
	fprintf(stderr,"Conversion file created by incorrect "
		"version of convert.pl\n\t%s unreadable\n",tmp);
	return(-1);
      }

      tag->channels=head.channels;
      tag->samplelength=head.samples;
      offset=sizeof(head);

      /* mmap the sample file */
      fd=open(tmp,O_RDONLY);
      if(fd<0){
	fprintf(stderr,"Unable to open %s fd for mmap:\n\t%d (%s)\n",
		tmp,errno,strerror(errno));
	return(-1);
      }
      tag->basemap=
	mmap(NULL,
	     tag->samplelength*sizeof(int16)*tag->channels+sizeof(head),
	     PROT_READ,MAP_PRIVATE,fd,0);
      close(fd);
      if(!tag->basemap){
	fprintf(stderr,"Unable to mmap fd %d (%s):\n\t%d (%s)\n",
		fd,tmp,errno,strerror(errno));
	return(-1);
      }
      
      for(i=0;i<tag->channels && i<2 ;i++) /* only up to stereo channels */
	tag->channel_vec[i]=(int16 *)(tag->basemap)+
	  sizeof(head)+head.samples*i;
      
    } 
  }

  /* state is zeroed when we go to production mode.  Done  */
  return(0);

}

void aquire_tag(int number){
  if(number<0)return;
  _alloc_tag_if_needed(number);
  tag_list[number].refcount++;
}

void release_tag(int number){
  if(number<0)return;
  if(number>=tag_count)return;
  tag_list[number].refcount--;
  if(tag_list[number].refcount<0)
    tag_list[number].refcount=0;
}

int save_tag(FILE *f,int number){
  tag *t;
  int count;

  if(number>=tag_count)return(0);
  t=tag_list+number;
  if(t->refcount){
    fprintf(f,"TAG:%d %d %ld %ld %ld",
	    number,t->sample_path,t->loop_p,t->loop_start,t->loop_lapping,
	    t->fade_out);
    count=fprintf(f,"\n");
    if(count<1)return(-1);
  }
  return(0);
}

int save_tags(FILE *f){
  int i;
  for(i=tag_count-1;i>=0;i--){
    if(save_tag(f,i))return(-1);
  }
  return(0);
}

int load_tag(FILE *f){
  tag t;
  int len,count,number;
  memset(&t,0,sizeof(t));
  
  count=fscanf(f,":%d %d %ld %ld %ld",
	       &number,&t.sample_path,&t.loop_p,&t.loop_start,
	       &t.loop_lapping,&t.fade_out);
  if(count<5){
    addnlstr("LOAD ERROR (TAG): too few fields.",80,' ');
    return(-1);
  }
  aquire_label(t.sample_path);
  edit_tag(number,t);
  return(0);
}

/********************* cue abstraction ********************/

typedef struct {
  int  vol_master;
  long vol_ms;
  
  int  outvol[2][MAX_CHANNELS];
} mix;

typedef struct {
  label_number label;
  label_number cue_text;
  label_number cue_desc;

  tag_number   tag;
  int          tag_create_p; 
  mix          mix;
} cue;

static cue        *cue_list;
int                cue_count;
int                cue_storage;

void add_cue(int n,cue c){
  int i;

  if(cue_count==cue_storage){
    cue_storage++;
    cue_storage*=2;
    cue_list=m_realloc(cue_list,cue_storage*sizeof(*cue_list));
  }

  /* copy insert */
  if(cue_count-n)
    memmove(cue_list+n+1,cue_list+n,sizeof(*cue_list)*(cue_count-n));
  cue_count++;

  cue_list[n]=c;
}

/* inefficient.  delete one, shift-copy list, delete one, shift-copy
   list, delete one, gaaaah.  Not worth optimizing */
static void _delete_cue_helper(int n){
  if(n>=0 && n<cue_count){
    release_label(cue_list[n].label);
    release_label(cue_list[n].cue_text);
    release_label(cue_list[n].cue_desc);
    release_tag(cue_list[n].tag);
    cue_count--;
    if(n<cue_count)
      memmove(cue_list+n,cue_list+n+1,sizeof(*cue_list)*(cue_count-n));
  }
}

/* slightly more complicated that just removing the cue from memory;
   we need to remove any tag mixer modification cues that follow if
   this cue creates a sample tag */

void delete_cue_single(int n){
  int i;
  if(n>=0 && n<cue_count){
    cue *c=cue_list+n;
    tag_number   tag=c->tag;

    if(c->tag_create_p){
      /* tag creation cue; have to delete following cues matching this
         tag */
      for(i=cue_count-1;i>n;i--)
	if(cue_list[i].tag==tag)
	  _delete_cue_helper(i);
    }
    _delete_cue_helper(n);
  }
}

/* this deletes all cues of a cue bank, and also chases the tags of
   sample creation cues */
void delete_cue_bank(int n){
  /* find first cue number */
  int first=n,last=n;

  while(first>0 && cue_list[first].label==cue_list[first-1].label)first--;
  while(last+1<cue_count && 
	cue_list[last].label==cue_list[last+1].label)last++;

  for(;last>=first;last--)
    delete_cue_single(last);
}
	    
int save_cue(FILE *f,int n){
  if(n<cue_count){
    int count,i;
    cue *c=cue_list+n;
    fprintf(f,"CUE:%d %d %d %d %d %d %ld :%d:%d:",
	    c->label,c->cue_text,c->cue_desc,
	    c->tag,c->tag_create_p,
	    c->mix.vol_master,
	    c->mix.vol_ms,
	    MAX_CHANNELS,
	    2);
    
    for(i=0;i<MAX_CHANNELS;i++)
      fprintf(f,"<%d %d>",c->mix.outvol[0][i],c->mix.outvol[1][i]);
    count=fprintf(f,"\n");
    if(count<1)return(-1);
  }
  return(0);
}

int save_cues(FILE *f){
  int i;
  for(i=0;i<cue_count;i++)
    if(save_cue(f,i))return(-1);
  return(0);
}

int load_cue(FILE *f){
  cue c;
  int len,count,i,j;
  int maxchannels,maxwavch;
  memset(&c,0,sizeof(c));
  
  count=fscanf(f,":%d %d %d %d %d %d %ld :%d:%d:",
	       &c.label,&c.cue_text,&c.cue_desc,
	       &c.tag,&c.tag_create_p,
	       &c.mix.vol_master,
	       &c.mix.vol_ms,
	       &maxchannels,&maxwavch);
  if(count<9){
    addnlstr("LOAD ERROR (CUE): too few fields.",80,' ');
    return(-1);
  }

  for(i=0;i<maxchannels;i++){
    int v;
    long lv;
    char ch=' ';
    count=fscanf(f,"%c",&ch);
    if(count<0 || ch!='<'){
      addnlstr("LOAD ERROR (CUE): parse error looking for '<'.",80,' ');
      return(-1);
    }
    for(j=0;j<maxwavch;j++){
      count=fscanf(f,"%d",&v);
      if(count<1){
	addnlstr("LOAD ERROR (CUE): parse error looking for value.",80,' ');
	return(-1);
      }
      if(j<2)
	c.mix.outvol[j][i]=v;
    }
    count=fscanf(f,"%c",&ch);
    if(count<0 || ch!='>'){
      addnlstr("LOAD ERROR (CUE): parse error looking for '>'.",80,' ');
      return(-1);
    }
  }
  
  aquire_label(c.label);
  aquire_label(c.cue_text);
  aquire_label(c.cue_desc);

  aquire_tag(c.tag);
  add_cue(cue_count,c);

  return(0);
}

int load_val(FILE *f, int *val){
  int count;
  int t;  
  count=fscanf(f,": %d",&t);
  if(count<1){
    addnlstr("LOAD ERROR (VAL): missing field.",80,' ');
    return(-1);
  }
  *val=t;
  return(0);
}

int save_val(FILE *f, char *p, int val){  
  int count;
  fprintf(f,"%s: %d",p,val);
  count=fprintf(f,"\n");
  if(count<1)return(-1);
  return(0);
}

int load_program(FILE *f){
  char buf[3];
  int ret=0,count;

  move(0,0);
  attron(A_BOLD);

  while(1){
    if(fread(buf,1,3,f)<3){
      addnlstr("LOAD ERROR: truncated program.",80,' ');
      ret=-1;
      break;
    }
    if(!strncmp(buf,"TAG",3)){
      ret|=load_tag(f);
    }else if(!strncmp(buf,"CUE",3)){
      ret|=load_cue(f);
    }else if(!strncmp(buf,"LBL",3)){
      ret|=load_label(f);
    }else if(!strncmp(buf,"MAS",3)){
      ret|=load_val(f,&main_master_volume);
    }
    
    while(1){
      count=fgetc(f);
      if(count=='\n' || count==EOF)break;
    }
    if(count==EOF)break;
    while(1){
      count=fgetc(f);
      if(count!='\n' || count==EOF)break;
    }
    if(count!=EOF)
      ungetc(count,f);
    if(count==EOF)break;
  }

  attroff(A_BOLD);
  if(ret)getch();
  return(ret);
}
   
int save_program(FILE *f){
  int ret=0;
  ret|=save_labels(f);
  ret|=save_tags(f);
  ret|=save_cues(f);
  ret|=save_val(f,"MAS",main_master_volume);
  return ret;
}

/***************** simple form entry fields *******************/

void switch_to_stderr(){
  def_prog_mode();           /* save current tty modes */
  endwin();                  /* restore original tty modes */
}

void switch_to_ncurses(){
  refresh();                 /* restore save modes, repaint screen */
}

enum field_type { FORM_YESNO, FORM_PERCENTAGE, FORM_NUMBER, FORM_GENERIC,
                  FORM_BUTTON } ;
typedef struct {
  enum field_type type;
  int x;
  int y;
  int width;
  void *var;

  int active;
  int cursor;
} formfield;

typedef struct {
  formfield *fields;
  int count;
  int storage;
  int edit;

  int cursor;
} form;

void form_init(form *f,int maxstorage){
  memset(f,0,sizeof(*f));
  f->fields=calloc(maxstorage,sizeof(formfield));
  f->storage=maxstorage;
}

void form_clear(form *f){
  if(f->fields)free(f->fields);
  memset(f,0,sizeof(*f));
}

void draw_field(formfield *f,int edit,int focus){
  int y,x;
  int i;
  getyx(stdscr,y,x);
  move(f->y,f->x);

  if(f->type==FORM_BUTTON)edit=0;

  if(edit && f->active)
    attron(A_REVERSE);
  
  addch('[');

  if(f->active){

    if(edit){
      attrset(0);
    }else{
      if(focus){
	attron(A_REVERSE);
      }else{
	attron(A_BOLD);
      }
    }
    
    switch(f->type){
    case FORM_YESNO:
      {
	int *var=(int *)(f->var);
	char *s="No ";
	if(*var)
	  s="Yes";
	for(i=0;i<f->width-5;i++)
	  addch(' ');
	addstr(s);
      }
      break;
    case FORM_PERCENTAGE:
      {
	int var=*(int *)(f->var);
	char buf[80];
	if(var<0)var=0;
	if(var>100)var=100;
	snprintf(buf,80,"%*d",f->width-2,var);
	addstr(buf);
      }
      break;
    case FORM_NUMBER:
      {
	int var=*(int *)(f->var);
	char buf[80];
	snprintf(buf,80,"%*d",f->width-2,var);
	addstr(buf);
      }
      break;
    case FORM_GENERIC:case FORM_BUTTON:
      {
	char *var=(char *)(f->var);
	addnlstr(var,f->width-2,' ');
      }
      break;
    }
    
    attrset(0);
  }else{
    attrset(0);
    addnlstr("",f->width-2,'-');
  }

  if(edit &&
     f->active)
    attron(A_REVERSE);
  
  addch(']');

  attrset(0);

  /* cursor? */
  move(y,x);
  if(focus && edit && f->type==FORM_GENERIC){
    curs_set(1);
    move(f->y,f->x+1+f->cursor);
  }else{
    curs_set(0);
  } 
}

void form_redraw(form *f){
  int i;
  for(i=0;i<f->count;i++)
    draw_field(f->fields+i,f->edit,i==f->cursor);
}

int field_add(form *f,enum field_type type,int x,int y,int width,void *var){
  int n=f->count;
  if(f->storage==n)return(-1);
  /* add the struct, then draw contents */
  f->fields[n].type=type;
  f->fields[n].x=x;
  f->fields[n].y=y;
  f->fields[n].width=width;
  f->fields[n].var=var;
  f->fields[n].active=1;
  f->count++;

  draw_field(f->fields+n,f->edit,n==f->cursor);
  return(n);
}

void field_state(form *f,int n,int activep){
  if(n<f->count){
    f->fields[n].active=activep;
    draw_field(f->fields+n,f->edit,n==f->cursor);
  }
}    

void form_next_field(form *f){
  formfield *ff=f->fields+f->cursor;
  draw_field(f->fields+f->cursor,0,0);
  
  while(1){
    f->cursor++;
    if(f->cursor>=f->count)f->cursor=0;
    ff=f->fields+f->cursor;
    if(ff->active)break;
  }

  draw_field(f->fields+f->cursor,f->edit,1);
}

void form_prev_field(form *f){
  formfield *ff=f->fields+f->cursor;
  draw_field(f->fields+f->cursor,0,0);
  
  while(1){
    f->cursor--;
    if(f->cursor<0)f->cursor=f->count-1;
    ff=f->fields+f->cursor;
    if(ff->active)break;
  }
  
  draw_field(f->fields+f->cursor,f->edit,1);
}

/* returns >=0 if it does not handle the character */
int form_handle_char(form *f,int c){
  formfield *ff=f->fields+f->cursor;
  int ret=-1;

  switch(c){
  case KEY_ENTER:
  case '\n':
  case '\r':
    if(ff->type==FORM_BUTTON){
      f->edit=0;
      ret=KEY_ENTER;
    }else{
      if(f->edit){
	f->edit=0;
	//draw_field(f->fields+f->cursor,f->edit,1);    
	//form_next_field(f);
      }else{
	f->edit=1;
      }
    }
    break;
  case KEY_UP:
    form_prev_field(f);
    break;
  case KEY_DOWN:case '\t':
    form_next_field(f);
    break;
  default:
    if(f->edit){
      switch(ff->type){
      case FORM_YESNO:
	{
	  int *val=(int *)ff->var;
	  switch(c){
	  case 'y':case 'Y':
	    *val=1;
	    break;
	  case 'n':case 'N':
	    *val=0;
	    break;
	  case ' ':
	    if(*val)
	    *val=0;
	  else
	    *val=1;
	    break;
	  default:
	    ret=c;
	    break;
	  }
	}
	break;
      case FORM_PERCENTAGE:
	{
	  int *val=(int *)ff->var;
	  switch(c){
	  case '+':case '=':case KEY_LEFT:
	    (*val)++;
	    if(*val>100)*val=100;
	    break;
	  case '-':case '_':case KEY_RIGHT:
	    (*val)--;
	    if(*val<0)*val=0;
	    break;
	  default:
	    ret=c;
	    break;
	  }
	}
	break;
      case FORM_NUMBER:
	{
	  int *val=(int *)ff->var;
	  switch(c){
	  case '0':case '1':case '2':case '3':case '4':
	  case '5':case '6':case '7':case '8':case '9':
	    if(*val<(int)rint(pow(10,ff->width-3))){
	      (*val)*=10;
	      (*val)+=c-48;
	    }
	    break;
	  case KEY_BACKSPACE:case '\b':
	    (*val)/=10;
	    break;
	  case KEY_RIGHT:case '+':case '=':
	    if(*val<(int)rint(pow(10,ff->width-2)-1))
	      (*val)++;
	    break;
	  case KEY_LEFT:case '-':case '_':
	    if(*val>0)
	      (*val)--;
	    break;
	  default:
	    ret=c;
	    break;
	  }
	}
	break;
	
	/* we assume the string for the GENERIC case is alloced to width */
      case FORM_GENERIC:
	{
	  char *val=(char *)ff->var;
	  const char *ctrl=unctrl(c);
	  switch(c){
	  case KEY_LEFT:
	    ff->cursor--;
	    if(ff->cursor<0)ff->cursor=0;
	    break;
	  case KEY_RIGHT:
	    ff->cursor++;
	    if(ff->cursor>(int)strlen(val))ff->cursor=strlen(val);
	    if(ff->cursor>ff->width-3)ff->cursor=ff->width-3;
	    break;
	  case KEY_BACKSPACE:case '\b':
	    if(ff->cursor>0){
	      memmove(val+ff->cursor-1,val+ff->cursor,strlen(val)-ff->cursor+1);
	      ff->cursor--;
	    }
	    break;
	  default:
	    if(isprint(c)){
	      if((int)strlen(val)<ff->width-3){
		memmove(val+ff->cursor+1,val+ff->cursor,strlen(val)-ff->cursor+1);
		val[ff->cursor]=c;
		ff->cursor++;
	      }
	    }else{
	      if(ctrl[0]=='^'){
		switch(ctrl[1]){
		case 'A':case 'a':
		  ff->cursor=0;
		  break;
		case 'E':case 'e':
		  ff->cursor=strlen(val);
		  break;
		case 'K':case 'k':
		  val[ff->cursor]='\0';
		  break;
		default:
		  ret=c;
		  break;
		}
	      }else{
		ret=c;
	      }
	    }
	    break;
	  }
	}
	break;
      default:
	ret=c;
	break;
      }
    }else{
      ret=c;
    }
  }
  draw_field(f->fields+f->cursor,f->edit,1);    
  return(ret);
}

/********************** main run screen ***********************/
void main_update_master(int n,int y){
  if(menu==MENU_MAIN){
    char buf[4];
    if(n>100)n=100;
    if(n<0)n=0;

    pthread_mutex_lock(&master_mutex);
    main_master_volume=n;
    pthread_mutex_unlock(&master_mutex);
    
    move(y,8);
    addstr("master: ");
    sprintf(buf,"%3d%%",main_master_volume);
    addstr(buf);
  }
}

void main_update_playbuffer(int y){
  if(menu==MENU_MAIN){
    char buf[20];
    int  n;
    static int starve=0;
    
    pthread_mutex_lock(&master_mutex);
    n=playback_buffer_minfill;
    pthread_mutex_unlock(&master_mutex);
    
    if(n==0)starve=1;
    if(n<0){
      starve=0; /* reset */
      n=0;
    }
    
    move(y,4);
    addstr("playbuffer: ");
    sprintf(buf,"%3d%% %s",n,starve?"***STARVE***":"            ");
    addstr(buf);
  }
}

#define todB_nn(x)   ((x)==0.f?-400.f:log((x))*8.6858896f)

void main_update_outchannel_levels(int y){
  int i,j;
  if(menu==MENU_MAIN){
    for(i=0;i<MAX_CHANNELS;i++){
      int val;
      char buf[11];
      pthread_mutex_lock(&master_mutex);
      val=channel_list[i].peak;
      pthread_mutex_unlock(&master_mutex);

      move(y+i+1,24);
      if(val>=32767){
	attron(A_BOLD);
	addstr("CLIP");
      }else{
	addstr("+0dB");
      }
      attron(A_BOLD);

      move(y+i+1,13);
      val=rint(10.+(todB_nn(val/32768.)/10));
      for(j=0;j<val;j++)buf[j]='=';
      buf[j]='|';
      for(;j<10;j++)buf[j]=' ';
      buf[j]='\0';
      addstr(buf);
      attroff(A_BOLD);
    }
  }
}

void main_update_outchannel_labels(int y){
  int i;
  char buf[80];
  if(menu==MENU_MAIN){
    for(i=0;i<MAX_CHANNELS;i++){
      move(y+i+1,4);
      sprintf(buf,"%2d: -Inf[          ]+0dB ",i);
      addstr(buf);
      pthread_mutex_lock(&master_mutex);
      addstr(channel_list[i].label);
      pthread_mutex_unlock(&master_mutex);
    }
  }

  main_update_outchannel_levels(y);
}

void main_update_cues(int y){
  if(menu==MENU_MAIN){
    int cn=cue_list_number-1,i;

    for(i=-1;i<2;i++){
      char buf[80];
      move(y+i*2+2,0);
      if(i==0){
	addstr("NEXT => ");
	attron(A_REVERSE);
      }
      if(cn<0){
	move(y+i*2+2,8);
	addnlstr("",71,' ');
	move(y+i*2+3,8);
	addnlstr("**** BEGIN",71,' ');
      }else if(cn>=cue_count){
	move(y+i*2+2,8);
	addnlstr("****** END",71,' ');
	attroff(A_REVERSE);
	move(y+i*2+3,8);
	addnlstr("",71,' ');
	if(i==0){
	  move(y+i*2+4,8);
	  addnlstr("",71,' ');
	  move(y+i*2+5,8);
	  addnlstr("",71,' ');
	}
	break;
      }else{
	cue *c;
	c=cue_list+cn;

	snprintf(buf,13,"%10s) ",label_list[c->label].text);
	mvaddstr(y+i*2+2,8,buf);
	addnlstr(label_list[c->cue_text].text,59,' ');

	mvaddstr(y+i*2+3,8,"            ");
	addnlstr(label_list[c->cue_desc].text,59,' ');
      }
      attroff(A_BOLD);
      while(++cn<cue_count)
	if(cue_list[cn].tag==-1)break;

      attroff(A_REVERSE);
    }
  }
}

/* assumes the existing tags/labels are not changing out from
   underneath playback.  editing a tag *must* kill playback for
   stability */
void main_update_tags(int y){
  if(menu==MENU_MAIN){
    int i;

    move(y,0);
    if(active_count){
      int len;
      addstr("playing tags:");

      for(i=0;i<active_count;i++){
	int loop;
	int ms;
	char buf[8];
	label_number path;
	
	move(y+i,14);
	/* normally called by playback so no locking */
	loop=active_list[i]->loop_p;
	ms=active_list[i]->samplelength-active_list[i]->sample_position;
	path=active_list[i]->sample_path;
	
	if(loop)
	  sprintf(buf,"[loop] ");
	else
	  sprintf(buf,"[%3ds] ",(ms+500)/1000);
	addstr(buf);
	addnlstr(label_list[path].text,55,' ');
      }
      move(y+i,14);
      addnlstr("",55,' ');
    }else{
      addstr("             ");
      move(y,14);
      addnlstr("",55,' ');
    }
  }
}

static int editable=1;
void update_editable(){
  if(menu==MENU_MAIN){
    move(0,67);
    attron(A_BOLD);
    if(!editable)
      addstr(" EDIT LOCKED ");
    else
      addstr("             ");
    attroff(A_BOLD);
  }
}

void move_next_cue(){
  if(cue_list_number<cue_count){
    while(++cue_list_number<cue_count)
      if(cue_list[cue_list_number].tag==-1)break;
    cue_list_position++;
  }
  main_update_cues(10+MAX_CHANNELS);
}

void move_prev_cue(){
  if(cue_list_number>0){
    while(--cue_list_number>0)
      if(cue_list[cue_list_number].tag==-1)break;
    cue_list_position--;
  }
  main_update_cues(10+MAX_CHANNELS);
}

int save_top_level(char *fn){
  FILE *f;
  if(!firstsave){
    char *buf=alloca(strlen(fn)*2+20);
    sprintf(buf,"cp %s %s~",fn,fn);
    /* create backup file */
    system(buf);
    firstsave=1;
  }
  move(0,0);
  attron(A_BOLD);
  
  f=fopen(fn,"w");
  if(f==NULL){
    char buf[80];
    sprintf(buf,"SAVE FAILED: %s",strerror(ferror(f)));
    addnlstr(buf,80,' ');
    attroff(A_BOLD);
    return(1);
  }

  if(save_program(f)){
    char buf[80];
    sprintf(buf,"SAVE FAILED: %s",strerror(ferror(f)));
    addnlstr(buf,80,' ');
    attroff(A_BOLD);    
    fclose(f);
    return(1);
  }
  fclose(f);
  addnlstr("PROGRAM SAVED (any key to continue)",80,' ');
  attroff(A_BOLD);    
  unsaved=0;
  return(0);
}

int main_menu(){
  clear();
  move(0,0);
  addstr("MTG Beaverphonic build "VERSION": ");
  attron(A_BOLD);
  addstr(program);
  attroff(A_BOLD);
  update_editable();

  mvvline(3,2,0,MAX_CHANNELS+5);
  mvvline(3,77,0,MAX_CHANNELS+5);
  mvhline(2,2,0,76);
  mvhline(7,2,0,76);
  mvhline(8+MAX_CHANNELS,2,0,76);
  mvaddch(2,2,ACS_ULCORNER);
  mvaddch(2,77,ACS_URCORNER);
  mvaddch(8+MAX_CHANNELS,2,ACS_LLCORNER);
  mvaddch(8+MAX_CHANNELS,77,ACS_LRCORNER);

  mvaddch(7,2,ACS_LTEE);
  mvaddch(7,77,ACS_RTEE);

  move(2,7);
  addstr(" output ");


  mvhline(9+MAX_CHANNELS,0,0,80);
  mvhline(16+MAX_CHANNELS,0,0,80);

  main_update_master(main_master_volume,5);
  main_update_playbuffer(4);
  main_update_outchannel_labels(7);

  main_update_cues(10+MAX_CHANNELS);
  main_update_tags(17+MAX_CHANNELS);
  curs_set(0);

  refresh();

  while(1){
    int ch=getch();
    switch(ch){
    case '?':
      return(MENU_KEYPRESS);
    case 'q':
      move(0,0);
      attron(A_BOLD);
      addnlstr("Really quit? [y/N] ",80,' ');
      attroff(A_BOLD);
      refresh();
      ch=getch();
      if(ch=='y'){
	if(unsaved){
	  move(0,0);
	  attron(A_BOLD);
	  addnlstr("Save changes first? [Y/n] ",80,' ');
	  attroff(A_BOLD);
	  ch=getch();
	  if(ch!='n' && ch!='N')save_top_level(program);
	}
	return(MENU_QUIT);
      }
      move(0,0);
      addstr("MTG Beaverphonic build "VERSION": ");
      attron(A_BOLD);
      addstr(program);
      attroff(A_BOLD);
      break;
    case 'e':
      if(editable && cue_list_number<cue_count)return(MENU_EDIT);
      break;
    case 'a':
      if(editable)
	return(MENU_ADD);
      break;
    case 'd':
      if(editable){
	//halt_playback();
	move(0,0);
	attron(A_BOLD);
	addnlstr("Really delete cue? [y/N] ",80,' ');
	attroff(A_BOLD);
	refresh();
	ch=getch();
	if(ch=='y'){
	  unsaved=1;
	  delete_cue_bank(cue_list_number);
	  main_update_cues(10+MAX_CHANNELS);
	}
	move(0,0);
	addstr("MTG Beaverphonic build "VERSION": ");
	attron(A_BOLD);
	addstr(program);
	attroff(A_BOLD);
      }

      break;
    case 'o':
      if(editable)
	return(MENU_OUTPUT);
      break;
    case 's':
      save_top_level(program);
      getch();
      return(MENU_MAIN);
    case '-':case '_':
      unsaved=1;
      main_update_master(main_master_volume-1,5);
      break;
    case '+':case '=':
      unsaved=1;
      main_update_master(main_master_volume+1,5);
      break;
    case ' ':
      //run_next_cue();
      move_next_cue();
      break;
    case KEY_UP:case '\b':case KEY_BACKSPACE:
      move_prev_cue();
      break;
    case KEY_DOWN:
      move_next_cue();
      break;
    case 'H':
      //halt_playback();
      break;
    case 'R':
      //halt_playback();
      cue_list_position=0;
      cue_list_number=0;
      return(MENU_MAIN);
    case 'l':
      if(editable)
	editable=0;
      else
	editable=1;
      update_editable();
    }
  }
}

int main_keypress_menu(){
  clear();
  box(stdscr,0,0);
  mvaddstr(0,2," Keypresses for main menu ");
  attron(A_BOLD);
  mvaddstr(2,2, "        ?");
  mvaddstr(4,2, "    space");
  mvaddstr(5,2, "  up/down");
  mvaddstr(6,2, "backspace");
  mvaddstr(8,2, "        a");
  mvaddstr(9,2, "        e");
  mvaddstr(10,2,"        d");
  mvaddstr(11,2,"        o");
  mvaddstr(12,2,"        l");
  mvaddstr(14,2,"        s");
  mvaddstr(15,2,"      +/-");
  mvaddstr(16,2,"        H");
  mvaddstr(17,2,"        R");

  attroff(A_BOLD);
  mvaddstr(2,12,"keypress menu (you're there now)");
  mvaddstr(4,12,"play next cue");
  mvaddstr(5,12,"move cursor to prev/next cue");
  mvaddstr(6,12,"move cursor to previous cue");
  mvaddstr(8,12,"add new cue at current cursor position");
  mvaddstr(9,12,"edit cue at current cursor position");
  mvaddstr(10,12,"delete cue at current cursor position");
  mvaddstr(11,12,"output channel configuration");
  mvaddstr(12,12,"lock non-modifiable mode (production)");

  mvaddstr(14,12,"save program");
  mvaddstr(15,12,"master volume up/down");
  mvaddstr(16,12,"halt playback");
  mvaddstr(17,12,"reset board to beginning");

  mvaddstr(19,12,"any key to return to main menu");
  refresh();
  getch();
  return(MENU_MAIN);
}

int add_cue_menu(){
  int tnum=new_tag_number();
  form f;

  char label[12]="";
  char text[61]="";
  char desc[61]="";

  form_init(&f,4);
  clear();
  box(stdscr,0,0);
  mvaddstr(0,2," Add new cue ");

  mvaddstr(2,2,"      cue label");
  mvaddstr(3,2,"       cue text");
  mvaddstr(4,2,"cue description");

  field_add(&f,FORM_GENERIC,18,2,12,label);
  field_add(&f,FORM_GENERIC,18,3,59,text);
  field_add(&f,FORM_GENERIC,18,4,59,desc);

  field_add(&f,FORM_BUTTON,68,6,9,"ADD CUE");
  
  refresh();
  
  while(1){
    int ch=form_handle_char(&f,getch());
    switch(ch){
    case KEY_ENTER:case 'x':case 'X':
      goto enter;
    case 27: /* esc */
      goto out;
    }
  }

 enter:
  {
    /* determine where in list cue is being added */
    cue c;
        
    unsaved=1;
    memset(&c,0,sizeof(c));
    c.tag=-1; /* placeholder cue for this cue bank */
	/* aquire label locks, populate */
    c.label=new_label_number();
    edit_label(c.label,label);
    aquire_label(c.label);
    c.cue_text=new_label_number();
    edit_label(c.cue_text,text);
    aquire_label(c.cue_text);
    c.cue_desc=new_label_number();
    edit_label(c.cue_desc,desc);
    aquire_label(c.cue_desc);
    
    add_cue(cue_list_number,c);
    move_next_cue();
  }
 out:
  form_clear(&f);
  return(MENU_MAIN);
}

int add_sample_cue_menu(){
  int tnum=new_tag_number();
  int i;
  char buf[50];
  form f;
  form_init(&f,9+MAX_CHANNELS*3);
  clear();
  box(stdscr,0,0);
  mvaddstr(0,2," Add new sample cue ");

  mvaddstr(2,2,"cue number");
  mvaddstr(3,2,"cue decimal");
  mvaddstr(4,2,"sample tag");
  sprintf(buf,"%4d",tnum);
  attron(A_BOLD);
  mvaddstr(4,18,buf);
  attroff(A_BOLD);

  mvaddstr(5,2,"cue text");
  mvaddstr(7,2,"sample file");
  mvaddstr(9,2,"volume master       % fade       /      ms");
  mvaddstr(10,2,"loop                     crosslap       ms");

  mvaddstr(12,2,"channels out");

  for(i=0;i<MAX_CHANNELS;i++){
    sprintf(buf,"L     %%     R -> %d ",i);
    mvaddstr(12+i,17,buf);
    addnlstr(channel_list[i].label,40,' ');
  }
    

  {
    int number=0;
    int decimal=0;
    char text[60]="\0";
    char path[60]="\0";
    int master=100;
    int masterin=15;
    int masterout=15;
    int outvol[2][MAX_CHANNELS]={{100,0},{0,100}};
    int loop=0;
    int cross=0;
    int loopfield,crossfield;

    field_add(&f,FORM_NUMBER,17,2,6,&number);
    field_add(&f,FORM_NUMBER,17,3,6,&decimal);
    field_add(&f,FORM_GENERIC,17,5,60,text);
    field_add(&f,FORM_GENERIC,17,7,60,path);

    field_add(&f,FORM_PERCENTAGE,17,9,5,&master);
    field_add(&f,FORM_NUMBER,29,9,6,&masterin);
    field_add(&f,FORM_NUMBER,36,9,6,&masterout);

    loopfield=field_add(&f,FORM_YESNO,17,10,5,&loop);
    crossfield=field_add(&f,FORM_NUMBER,36,10,6,&cross);
    field_state(&f,crossfield,0);
		
    for(i=0;i<MAX_CHANNELS;i++){
      field_add(&f,FORM_PERCENTAGE,18,12+i,5,&(outvol[0][i]));
      field_add(&f,FORM_PERCENTAGE,24,12+i,5,&(outvol[1][i]));
    }
    
    refresh();

    while(1){
      int ch=form_handle_char(&f,getch());

      switch(ch){
      case 27: /* esc */
	goto out;
      case KEY_ENTER: case '\n':case '\r':
	{

	}
	return(MENU_MAIN);
      }

      if(f.cursor==loopfield){
	if(loop)
	  field_state(&f,crossfield,1);
	else
	  field_state(&f,crossfield,0);
      }
    }
  }
  
 out:
  form_clear(&f);
  return(MENU_MAIN);
}

void edit_keypress_menu(){
  clear();
  box(stdscr,0,0);
  mvaddstr(0,2," Keypresses for cue edit menu ");
  attron(A_BOLD);
  mvaddstr(2,2, "        ?");
  mvaddstr(3,2, "  up/down");
  mvaddstr(4,2, "      tab");
  mvaddstr(5,2, "        x");

  mvaddstr(7,2, "        a");
  mvaddstr(8,2, "        m");
  mvaddstr(9,2, "        d");
  mvaddstr(10,2,"    enter");
  mvaddstr(11,2,"        l");

  attroff(A_BOLD);
  mvaddstr(2,12,"keypress menu (you're there now)");
  mvaddstr(3,12,"move cursor to prev/next field");
  mvaddstr(4,12,"move cursor to next field");
  mvaddstr(5,12,"return to main menu");
  mvaddstr(7,12,"add new sample to cue");
  mvaddstr(8,12,"add new mixer change to cue");
  mvaddstr(9,12,"delete highlighted action");
  mvaddstr(10,12,"edit highlighted action");
  mvaddstr(11,12,"list all sample and sample tags");

  mvaddstr(13,12,"any key to return to cue edit menu");
  refresh();
  getch();
}

int edit_cue_menu(){
  form f;
  char label[12]="";
  char text[61]="";
  char desc[61]="";

  /* determine first and last cue in bank */
  int base=cue_list_number;
  int first=base+1;
  int last=first;
  int actions,i;

  while(cue_list[last].tag!=-1)last++;
  actions=last-first;

  form_init(&f,4+actions);
  strcpy(label,label_list[cue_list[base].label].text);
  strcpy(text,label_list[cue_list[base].cue_text].text);
  strcpy(desc,label_list[cue_list[base].cue_desc].text);

  field_add(&f,FORM_GENERIC,18,2,12,label);
  field_add(&f,FORM_GENERIC,18,3,59,text);
  field_add(&f,FORM_GENERIC,18,4,59,desc);
  
  for(i=0;i<actions;i++){
    char *buf=alloca(81);
    int tag=cue_list[first+i].tag;
    snprintf(buf,80,"%s sample %d (%s)",
	     cue_list[first+i].tag_create_p?"ADD":"MIX",tag,
	     label_list[tag_list[tag].sample_path].text);
    field_add(&f,FORM_BUTTON,11,6+i,64,buf);
  }
  if(actions==0){
    mvaddstr(6,11,"--None--");
    i++;
  }
  field_add(&f,FORM_BUTTON,66,7+i,11,"MAIN MENU");

  refresh();

  while(1){
    int loop=1;
    clear();
    box(stdscr,0,0);
    mvaddstr(0,2," Cue edit ");

    mvaddstr(2,2,"      cue label");
    mvaddstr(3,2,"       cue text");
    mvaddstr(4,2,"cue description");
    mvaddstr(6,2,"actions:");
    form_redraw(&f);
    
    while(loop){
      int ch=form_handle_char(&f,getch());
      
      switch(ch){
      case '?':case '/':
	edit_keypress_menu(); 
	loop=0;
	break;
      case 27:
	goto out;
      case 'x':case 'X':
	unsaved=1;
	edit_label(cue_list[base].label,label);
	edit_label(cue_list[base].cue_text,text);
	edit_label(cue_list[base].cue_desc,desc);
	goto out;

      case KEY_ENTER:
	if(f.cursor==3+actions){
	  unsaved=1;
	  edit_label(cue_list[base].label,label);
	  edit_label(cue_list[base].cue_text,text);
	  edit_label(cue_list[base].cue_desc,desc);
	  goto out;
	}
	/* ... else we're an action edit */

	{

	}
	break;
      }
    }
  }
  
 out:
  form_clear(&f);
  return(MENU_MAIN);
}

int menu_output(){
  return(MENU_MAIN);
}

int main_loop(){
  
  while(running){
    switch(menu){
    case MENU_MAIN:
      menu=main_menu();
      break;
    case MENU_KEYPRESS:
      menu=main_keypress_menu();
      break;
    case MENU_QUIT:
      running=0;
      break;
    case MENU_ADD:
      menu=add_cue_menu();
      break;
    case MENU_OUTPUT:
      menu=menu_output();
      break;
    case MENU_EDIT:
      //halt_playback();
      menu=edit_cue_menu();
      break;
    }
  }
}
  
int main(int gratuitously,char *different[]){
  if(gratuitously<2){
    fprintf(stderr,"Usage: beaverphonic <settingfile>\n");
    exit(1);
  }
  
  initscr(); cbreak(); noecho();
  nonl();
  intrflush(stdscr, FALSE);
  keypad(stdscr, TRUE);
  use_default_colors();
  signal(SIGINT,SIG_IGN);

  /* load the sound config if the file exists, else create it */
  program=strdup(different[1]);
  {
    FILE *f=fopen(program,"rb");
    if(f){
      load_program(f);
      fclose(f);
    }
  }
  
  main_loop();
  endwin();                  /* restore original tty modes */  
}  


#if 0 

 Add new cue -----------------------------------------------------------------
  cue label       [          ]
  cue text        [                                                          ]
  cue description [                                                          ]

 Edit cue --------------------------------------------------------------------
  cue label       [          ]
  cue text        [                                                          ]
  cue description [                                                          ]

  actions: [MIX sample 1 (                    )                              ]
           [ADD sample 3 (                    )                              ]

  ?
  a  add sample
  m  mix sample
  l  list samples
  e/enter  edit action 
  Esc/Enter off main menu

 Add new sample to cue -------------------------------------------------------

  cue label       [          ]
  cue text        [                                                          ]
  cue description [                                                          ]

  sample path     [                                                          ]
  sample tag      0 

  loop            [No ]     crosslap [----]ms                           
  volume master   [100]% fade [  15]/[  15]ms
                            
  --- L -- R -----------------------------------------------------------------
 |  [100][  0]% -> 0 offstage left                                            |
 |  [  0][100]% -> 1 offstage right                                           |
 |  [  0][  0]% -> 2                                                          |
 |  [  0][  0]% -> 3                                                          |
 |  [  0][  0]% -> 4                                                          |
 |  [  0][  0]% -> 5                                                          |
 |  [  0][  0]% -> 6                                                          |
 |  [  0][  0]% -> 7                                                          |
  ----------------------------------------------------------------------------

 Add mix change to cue -------------------------------------------------------

  cue label       [          ]
  cue text        [                                                          ]
  cue description [                                                          ]

  modify tag      [   0]
 
  volume master   [100]% fade [  15]
                            
  --- L -- R -----------------------------------------------------------------
 |  [100][  0]% -> 0 offstage left                                            |
 |  [  0][100]% -> 1 offstage right                                           |
 |  [  0][  0]% -> 2                                                          |
 |  [  0][  0]% -> 3                                                          |
 |  [  0][  0]% -> 4                                                          |
 |  [  0][  0]% -> 5                                                          |
 |  [  0][  0]% -> 6                                                          |
 |  [  0][  0]% -> 7                                                          |
  ----------------------------------------------------------------------------

 (l: list tags)

OUTPUT CHANNELS

0: [                         ] built in OSS left 
1: [                         ] built in OSS right
2: [                         ] Quattro 0


#endif
