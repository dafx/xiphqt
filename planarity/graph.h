/* graph abstraction */

#define V_RADIUS_SQ (V_RADIUS*V_RADIUS)
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))

typedef struct vertex {
  int num;
  int x;
  int y;
  int orig_x;
  int orig_y;

  int active;
  int selected_volatile;
  int selected;
  int grabbed;
  int attached_to_grabbed;
  struct edge_list *edges;
  struct vertex *next;
} vertex;

typedef struct intersection {
  struct intersection *prev;
  struct intersection *next;
  struct intersection *paired;
  double x;
  double y;
} intersection;

typedef struct edge{
  vertex *A;
  vertex *B;

  int active;

  intersection i; // correct, not a pointer
  struct edge *next;
} edge;

typedef struct edge_list{
  edge *edge;
  struct edge_list *next;
} edge_list;

typedef struct graph {
  vertex *verticies;
  int     vertex_num;
  edge *edges;
  long active_intersections;

  int num_edges;
  int num_edges_active;

  int width;
  int height;
  int orig_width;
  int orig_height;

  // scoring related metadata
  long  original_intersections;
  float intersection_mult;
  int   objective;
  int   objective_lessthan;
  float objective_mult;

} graph;

typedef struct graphmeta{
  int num;
  char *id;
  char *desc;
  void (*gen)(graph *,int arg);
  int gen_arg;
} graphmeta;

#include <stdio.h>

extern vertex *new_board(graph *g, int num_v);
extern vertex *find_vertex(graph *g, int x, int y);
extern void move_vertex(graph *g, vertex *v, int x, int y);
extern void grab_vertex(graph *g, vertex *v);
extern void ungrab_vertex(graph *g,vertex *v);
extern void activate_vertex(graph *g, vertex *v);
extern void deactivate_vertex(graph *g, vertex *v);
extern void select_verticies(graph *g, int x1, int y1, int x2, int y2);
extern void deselect_verticies(graph *g);
extern void move_selected_verticies(graph *g, int dx, int dy);
extern void scale_verticies(graph *g, float amount);
extern void randomize_verticies(graph *g);
extern edge *add_edge(graph *g,vertex *A, vertex *B);
extern int exists_edge(vertex *a, vertex *b);
extern vertex *get_verticies();
extern edge *get_edges();
extern int num_selected_verticies(graph *g);
extern int get_num_intersections();
extern int get_max_intersections();
extern void check_verticies();
extern void impress_location();
extern void commit_volatile_selection();
extern vertex *get_vertex();
extern void activate_verticies();
extern int graph_write(graph *g, FILE *f);
extern int graph_read(graph *g, FILE *f);
extern void graph_release(graph *g);

extern int graphscore_get_score(graph *g);
extern int graphscore_get_bonus(graph *g);
extern char *graphscore_objective_string(graph *g);
