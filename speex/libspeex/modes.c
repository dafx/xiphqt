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
#include "cb_search.h"

extern float gain_cdbk_nb[];
extern float exc_gains_table[];
extern float exc_table[];

ltp_params ltp_params_nb = {
   gain_cdbk_nb,
   7,
   7
};

split_cb_params split_cb_nb = {
   8,               /*subvect_size*/
   5,               /*nb_subvect*/
   exc_table,       /*shape_cb*/
   7,               /*shape_bits*/
   exc_gains_table, /*gain_cb*/
   8                /*gain_bits*/
};

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
   /*LSP quantization*/
   lsp_quant_nb,
   lsp_unquant_nb,
   /*Pitch quantization*/
   pitch_search_3tap,
   pitch_unquant_3tap,
   &ltp_params_nb,
   /*Innovation quantization*/
   split_cb_search,
   split_cb_unquant,
   &split_cb_nb
};


SpeexMode wb_mode = {
   256,    /*frameSize*/
   64,     /*subframeSize*/
   512,    /*windowSize*/
   16,     /*lpcSize*/
   1024,   /*bufSize*/
   35,     /*pitchStart*/
   290,    /*pitchEnd*/
   0.9,    /*gamma1*/
   0.2,    /*gamma2*/
   /*LSP quantization*/
   lsp_quant_nb,
   lsp_unquant_nb,
   /*Pitch quantization*/
   pitch_search_3tap,
   pitch_unquant_3tap,
   &ltp_params_nb,
   /*Innovation quantization*/
   split_cb_search,
   split_cb_unquant,
   &split_cb_nb
};
