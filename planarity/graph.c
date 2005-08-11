#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "graph.h"
#include "gameboard.h"
#define CHUNK 64

/* mesh/board state */
static vertex *verticies=0;
static int     vertex_num=0;
static vertex *vertex_pool=0;
static edge *edges=0;
static edge *edge_pool=0;
static edge_list *edge_list_pool=0;
static intersection *intersection_pool=0;

static int active_intersections=0;
static int reported_intersections=0;

static int num_edges=0;
static int num_edges_active=0;


/************************ edge list maint operations ********************/
static void add_edge_to_list(vertex *v, edge *e){
  edge_list *ret;
  
  if(edge_list_pool==0){
    int i;
    edge_list_pool = calloc(CHUNK,sizeof(*edge_list_pool));
    for(i=0;i<CHUNK-1;i++) /* last addition's next points to nothing */
      edge_list_pool[i].next=edge_list_pool+i+1;
  }

  ret=edge_list_pool;
  edge_list_pool=ret->next;

  ret->edge=e;
  ret->next=v->edges;
  v->edges=ret;
}

static void release_edge_list(vertex *v){
  edge_list *el=v->edges;

  if(el){
    edge_list *end=el;
    while(end->next)end=end->next;
  
    end->next = edge_list_pool;
    edge_list_pool = el;
    
    v->edges=0;
  }
}

/************************ intersection maint operations ********************/

static intersection *new_intersection(){
  intersection *ret;
  
  if(intersection_pool==0){
    int i;
    intersection_pool = calloc(CHUNK,sizeof(*intersection_pool));
    for(i=0;i<CHUNK-1;i++) /* last addition's next points to nothing */
      intersection_pool[i].next=intersection_pool+i+1;
  }

  ret=intersection_pool;
  intersection_pool=ret->next;
  return ret;
}

static void add_paired_intersection(edge *a, edge *b, double x, double y){
  intersection *A=new_intersection();
  intersection *B=new_intersection();

  A->paired=B;
  B->paired=A;

  A->next=a->i.next;
  if(A->next)
    A->next->prev=A;
  A->prev=&(a->i);
  a->i.next=A;

  B->next=b->i.next;
  if(B->next)
    B->next->prev=B;
  B->prev=&(b->i);
  b->i.next=B;

  A->x=B->x=x;
  A->y=B->y=y;
}

static void release_intersection(intersection *i){
  memset(i,0,sizeof(*i));
  i->next=intersection_pool;
  intersection_pool=i;
}

static void release_intersection_list(edge *e){
  intersection *i=e->i.next;
  
  while(i!=0){
    intersection *next=i->next;
    release_intersection(i);
    i=next;
  }
  e->i.next=0;
}

static int release_paired_intersection_list(edge *e){
  intersection *i=e->i.next;
  int count=0;

  while(i){
    intersection *next=i->next;
    intersection *j=i->paired;

    j->prev->next=j->next;
    if(j->next)
      j->next->prev=j->prev;

    release_intersection(i);
    release_intersection(j);
    i=next;
    count++;
  }
  e->i.next=0;
  return count;
}

/************************ edge maint operations ******************************/
/* also adds to the edge list */
edge *add_edge(vertex *A, vertex *B){
  edge *ret;
  
  if(edge_pool==0){
    int i;
    edge_pool = calloc(CHUNK,sizeof(*edge_pool));
    for(i=0;i<CHUNK-1;i++) /* last addition's next points to nothing */
      edge_pool[i].next=edge_pool+i+1;
  }

  ret=edge_pool;
  edge_pool=ret->next;

  ret->A=A;
  ret->B=B;
  ret->active=0;
  ret->i.next=0;
  ret->next=edges;
  edges=ret;

  add_edge_to_list(A,ret);
  add_edge_to_list(B,ret);

  num_edges++;

  return ret;
}

static void release_edges(){
  edge *e = edges;

  while(e){
    edge *next=e->next;
    release_intersection_list(e);
    e->next=edge_pool;
    edge_pool=e;
    e=next;
  }
  edges=0;
  num_edges=0;
  num_edges_active=0;
}

static int intersects(vertex *L1, vertex *L2, vertex *M1, vertex *M2, double *xo, double *yo){
  /* y = ax + b */
  float La=0;
  float Lb=0;
  float Ma=0;
  float Mb=0;

  // but first, edges that share a vertex don't intersect by definition
  if(L1==M1) return 0;
  if(L1==M2) return 0;
  if(L2==M1) return 0;
  if(L2==M2) return 0;

  if(L1->x != L2->x){
    La = (float)(L2->y - L1->y) / (L2->x - L1->x);
    Lb = (L1->y - L1->x * La);
  }
  
  if(M1->x != M2->x){
    Ma = (float)(M2->y - M1->y) / (M2->x - M1->x);
    Mb = (M1->y - M1->x * Ma);
  }
  
  if(L1->x == L2->x){
    // L is vertical line

    if(M1->x == M2->x){
      // M is also a vertical line

      if(L1->x == M1->x){
	// L and M vertical on same x, overlap?
	if(M1->y > L1->y && 
	   M1->y > L2->y &&
	   M2->y > L1->y &&
	   M2->y > L2->y) return 0;
	if(M1->y < L1->y && 
	   M1->y < L2->y &&
	   M2->y < L1->y &&
	   M2->y < L2->y) return 0;

	{
	  double y1=max( min(M1->y,M2->y), min(L1->y, L2->y));
	  double y2=min( max(M1->y,M2->y), max(L1->y, L2->y));

	  *xo = M1->x;
	  *yo = (y1+y2)*.5;

	}

      }else
	// L and M vertical, different x
	return 0;
      
    }else{
      // L vertical, M not vertical
      float y = Ma*L1->x + Mb;

      if(y < L1->y && y < L2->y) return 0;
      if(y > L1->y && y > L2->y) return 0;
      if(y < M1->y && y < M2->y) return 0;
      if(y > M1->y && y > M2->y) return 0;

      *xo = L1->x;
      *yo=y;

    }
  }else{

    if(M1->x == M2->x){
      // M vertical, L not vertical
      float y = La*M1->x + Lb;

      if(y < L1->y && y < L2->y) return 0;
      if(y > L1->y && y > L2->y) return 0;
      if(y < M1->y && y < M2->y) return 0;
      if(y > M1->y && y > M2->y) return 0;

      *xo = M1->x;
      *yo=y;

    }else{

      // L and M both have non-infinite slope
      if(La == Ma){
	//L and M have the same slope
	if(Mb != Lb) return 0; 
	
	// two segments on same line...
	if(M1->x > L1->x && 
	   M1->x > L2->x &&
	   M2->x > L1->x &&
	   M2->x > L2->x) return 0;
	if(M1->x < L1->x && 
	   M1->x < L2->x &&
	   M2->x < L1->x &&
	   M2->x < L2->x) return 0;
	
	{
	  double x1=max( min(M1->x,M2->x), min(L1->x, L2->x));
	  double x2=min( max(M1->x,M2->x), max(L1->x, L2->x));
	  double y1=max( min(M1->y,M2->y), min(L1->y, L2->y));
	  double y2=min( max(M1->y,M2->y), max(L1->y, L2->y));
	  
	  *xo = (x1+x2)*.5;
	  *yo = (y1+y2)*.5;
	  
	}
	
	
      }else{
	// finally typical case: L and M have different non-infinite slopes
	float x = (Mb-Lb) / (La - Ma);
	
	if(x < L1->x && x < L2->x) return 0;
	if(x > L1->x && x > L2->x) return 0;
	if(x < M1->x && x < M2->x) return 0;
	if(x > M1->x && x > M2->x) return 0;
	
	*xo = x;
	*yo = La*x + Lb;
      }
    }
  }

  return 1;
}

static void activate_edge(edge *e){
  /* computes all intersections */
  if(!e->active && e->A->active && e->B->active){
    edge *test=edges;
    while(test){
      if(test != e && test->active){
	double x,y;
	if(intersects(e->A,e->B,test->A,test->B,&x,&y)){
	  add_paired_intersection(e,test,x,y);
	  active_intersections++;

	}
      }
      test=test->next;
    }
    e->active=1;
    num_edges_active++;
    if(num_edges_active == num_edges)
      reported_intersections = active_intersections;
  }
}

static void deactivate_edge(edge *e){
  /* releases all associated intersections */
  if(e->active){
    active_intersections -= 
      release_paired_intersection_list(e);
    num_edges_active--;
    e->active=0;
  }
}

int exists_edge(vertex *a, vertex *b){
  edge_list *el=a->edges;
  while(el){
    if(el->edge->A == b) return 1;
    if(el->edge->B == b) return 1;
    el=el->next;
  }
  return 0;
}

/*********************** vertex maint operations *************************/
vertex *get_vertex(){
  vertex *ret;
  
  if(vertex_pool==0){
    int i;
    vertex_pool = calloc(CHUNK,sizeof(*vertex_pool));
    for(i=0;i<CHUNK-1;i++) /* last addition's next points to nothing */
      vertex_pool[i].next=vertex_pool+i+1;
  }

  ret=vertex_pool;
  vertex_pool=ret->next;

  ret->x=0;
  ret->y=0;
  ret->active=0;
  ret->selected=0;
  ret->selected_volatile=0;
  ret->grabbed=0;
  ret->attached_to_grabbed=0;
  ret->edges=0;
  ret->num=vertex_num++;

  ret->next=verticies;
  verticies=ret;

  return ret;
}

static void release_vertex(vertex *v){
  release_edge_list(v);
  v->next=vertex_pool;
  vertex_pool=v;
}

static void set_num_verticies(int num){
  /* do it the simple way; release all, link anew */
  vertex *v=verticies;

  while(v){
    vertex *next=v->next;
    release_vertex(v);
    v=next;
  }
  verticies=0;
  vertex_num=0;
  release_edges();

  while(num--)
    get_vertex();
}

void activate_vertex(vertex *v){
  edge_list *el=v->edges;
  v->active=1;
  while(el){
    activate_edge(el->edge);
    el=el->next;
  }
}

void deactivate_vertex(vertex *v){
  edge_list *el=v->edges;
  while(el){
    edge_list *next=el->next;
    deactivate_edge(el->edge);
    el=next;
  }
  v->active=0;
}

void grab_vertex(vertex *v){
  edge_list *el=v->edges;
  deactivate_vertex(v);
  while(el){
    edge_list *next=el->next;
    edge *e=el->edge;
    vertex *other=(e->A==v?e->B:e->A);
    other->attached_to_grabbed=1;
    el=next;
  }
  v->grabbed=1;
}

void ungrab_vertex(vertex *v){
  edge_list *el=v->edges;
  activate_vertex(v);
  while(el){
    edge_list *next=el->next;
    edge *e=el->edge;
    vertex *other=(e->A==v?e->B:e->A);
    other->attached_to_grabbed=0;
    el=next;
  }
  v->grabbed=0;
}

vertex *find_vertex(int x, int y){
  vertex *v = verticies;
  vertex *match = 0;

  while(v){
    vertex *next=v->next;
    int xd=x-v->x;
    int yd=y-v->y;
    if(xd*xd + yd*yd <= V_RADIUS_SQ) match=v;
    v=next;
  }
  
  return match;
}

static void check_vertex_helper(vertex *v, int reactivate){
  int flag=0;

  if(v->x>=get_board_width()){
    v->x=get_board_width()-1;
    flag=1;
  }
  if(v->x<0){
    v->x=0;
    flag=1;
  }
  if(v->y>=get_board_height()){
    v->y=get_board_height()-1;
    flag=0;
  }
  if(v->y<0){
    v->y=0;
    flag=1;
  }
  if(flag){
    if(v->edges){
      deactivate_vertex(v);
      if(reactivate)activate_vertex(v);
    }
  }
}

static void check_vertex(vertex *v){
  check_vertex_helper(v,1);
}

void check_verticies(){
  vertex *v=verticies;
  while(v){
    vertex *next=v->next;
    check_vertex_helper(v,0);
    v=next;
  }

  v=verticies;
  while(v){
    vertex *next=v->next;
    activate_vertex(v);
    v=next;
  }
}

void move_vertex(vertex *v, int x, int y){
  if(!v->grabbed) deactivate_vertex(v);
  v->x=x;
  v->y=y;
  check_vertex_helper(v,0);
  if(!v->grabbed) activate_vertex(v);
}

// tenative selection; must be confirmed if next call should not clear
void select_verticies(int x1, int y1, int x2, int y2){
  vertex *v = verticies;

  if(x1>x2){
    int temp=x1;
    x1=x2;
    x2=temp;
  }

  if(y1>y2){
    int temp=y1;
    y1=y2;
    y2=temp;
  }
  x1-=V_RADIUS;
  x2+=V_RADIUS;
  y1-=V_RADIUS;
  y2+=V_RADIUS;

  while(v){
    vertex *next=v->next;
    if(v->selected_volatile)v->selected=0;

    if(!v->selected){
      if(v->x>=x1 && v->x<=x2 && v->y>=y1 && v->y<=y2){
	v->selected=1;
	v->selected_volatile=1;
      }
    }

    v=next;
  }
}

int num_selected_verticies(){
  vertex *v = verticies;
  int count=0;
  while(v){
    if(v->selected)count++;
    v=v->next;
  }
  return count;
}

void deselect_verticies(){
  vertex *v = verticies;
  
  while(v){
    v->selected=0;
    v->selected_volatile=0;
    v=v->next;
  }

}

void commit_volatile_selection(){
  vertex *v = verticies;
  
  while(v){
    v->selected_volatile=0;
    v=v->next;
  }
}

void move_selected_verticies(int dx, int dy){
  vertex *v = verticies;
  /* deactivate selected verticies */
  while(v){
    vertex *next=v->next;
    if(v->selected)
      deactivate_vertex(v);
    v=next;
  }

  /* move selected verticies and reactivate */
  v=verticies;
  while(v){
    vertex *next=v->next;
    if(v->selected){
      v->x+=dx;
      v->y+=dy;
      check_vertex(v);
      activate_vertex(v);
    }
    v=next;
  }

}

void scale_verticies(float amount){
  vertex *v=verticies;
  int x=get_board_width()/2;
  int y=get_board_height()/2;

  while(v){
    vertex *next=v->next;
    deactivate_vertex(v);
    v->x=rint((v->x-x)*amount)+x;
    v->y=rint((v->y-y)*amount)+y;
    v=next;
  }

  v=verticies;
  while(v){
    vertex *next=v->next;
    check_vertex(v);
    activate_vertex(v);
    v=next;
  }
}

static vertex *split_vertex_list(vertex *v){
  vertex *half=v;
  vertex *prevhalf=v;

  while(v){
    v=v->next;
    if(v)v=v->next;
    prevhalf=half;
    half=half->next;
  }

  prevhalf->next=0;
  return half;
}

static vertex *randomize_helper(vertex *v){
  vertex *w=split_vertex_list(v);
  if(w){
    vertex *a = randomize_helper(v);
    vertex *b = randomize_helper(w);
    v=0;
    w=0;

    while(a && b){
      if(random()&1){
	// pull off head of a
	if(w)
	  w=w->next=a;
	else
	  v=w=a;
	a=a->next;
      }else{
	// pull off head of b
	if(w)
	  w=w->next=b;
	else
	  v=w=b;
	b=b->next;
      }
    }
    if(a)
      w->next=a;
    if(b)
      w->next=b;
  }
  return v;
}

void randomize_verticies(){
  verticies=randomize_helper(verticies);
}

int get_num_intersections(){
  return reported_intersections;
}

vertex *get_verticies(){
  return verticies;
}

edge *get_edges(){
  return edges;
}

vertex *new_board(int num_v){
  set_num_verticies(num_v);
  return verticies;
}

void impress_location(){
  vertex *v=verticies;
  while(v){
    v->orig_x=v->x;
    v->orig_y=v->y;
    v=v->next;
  }
}
