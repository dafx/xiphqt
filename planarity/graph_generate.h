/*
 *
 *  gPlanarity: 
 *     The geeky little puzzle game with a big noodly crunch!
 *    
 *     gPlanarity copyright (C) 2005 Monty <monty@xiph.org>
 *     Original Flash game by John Tantalo <john.tantalo@case.edu>
 *     Original game concept by Mary Radcliffe
 *
 *  gPlanarity is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  gPlanarity is distributed in the hope that it will be useful,
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

extern int generate_find_number(char *id);
extern int generate_get_meta(int num, graphmeta *gm);
extern void generate_board(graph *g,int num);

extern void generate_mesh_1(graph *g, int order);
extern void generate_mesh_1M(graph *g, int order);
extern void generate_mesh_1C(graph *g, int order);
extern void generate_mesh_1S(graph *g, int order);
extern void generate_mesh_1_cloud(graph *g, int order);

extern void generate_data(graph *g, int order);
