/* Copyright (C) 2002 Jean-Marc Valin 
   File: modes.c

   Describes the different modes of the codec

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
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#include "modes.h"
#include "ltp.h"
#include "quant_lsp.h"

SpeexMode nb_mode = {
   160,    /*frameSize*/
   40,     /*subframeSize*/
   320,    /*windowSize*/
   10,     /*lpcSize*/
   640,    /*bufSize*/
   20,     /*pitchStart*/
   140,    /*pitchEnd*/
   0.9,    /*gamma1*/
   0.6,    /*gamma2*/
   lsp_quant_nb,
   lsp_unquant_nb,
   pitch_search_3tap,
   pitch_unquant_3tap
};
