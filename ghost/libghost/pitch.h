/**
   @file pitch.h
   @brief Pitch analysis
 */

/* Copyright (C) 2005

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef _PITCH_H
#define _PITCH_H

float inner_prod(float *x, float *y, int len);

void find_pitch(float *x, float *gain, float *pitch, int start, int end, int len);

void find_spectral_pitch(float *x, float *y, int lag, int len, int *pitch, float *curve);

#endif
