/*
 *
 *  gtk2 spectrum analyzer
 *    
 *      Copyright (C) 2004-2012 Monty
 *
 *  This analyzer is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  The analyzer is distributed in the hope that it will be useful,
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

#ifndef _IO_H_
#define _IO_H_

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#define _ISOC99_SOURCE
#define _FILE_OFFSET_BITS 64
#define _REENTRANT 1
#define __USE_GNU 1
#endif

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <fcntl.h>

#define MAX_FILES 16
extern pthread_mutex_t blockbuffer_mutex;

extern int input_load(void);
extern int pipe_reload(void);
extern int input_read(int loop_p, int partial_p);
extern int rewind_files(void);

extern int inputs;
extern int total_ch;

extern int bits[MAX_FILES];
extern int bigendian[MAX_FILES];
extern int channels[MAX_FILES];
extern int rate[MAX_FILES];
extern int signedp[MAX_FILES];
extern int bits_force[MAX_FILES];
extern int bigendian_force[MAX_FILES];
extern int channels_force[MAX_FILES];
extern int rate_force[MAX_FILES];
extern int signed_force[MAX_FILES];

extern char *inputname[MAX_FILES];
extern int seekable[MAX_FILES];
extern int global_seekable;

extern sig_atomic_t blockslice_frac;
extern int blockbufferfill[MAX_FILES];
extern int blockbuffernew[MAX_FILES];
extern float **blockbuffer;
#endif

