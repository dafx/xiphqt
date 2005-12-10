/*
 *  stream_speex.h
 *
 *    Declaration of Speex format related functions of OggImporter.
 *
 *
 *  Copyright (c) 2005  Arek Korbik
 *
 *  This file is part of XiphQT, the Xiph QuickTime Components.
 *
 *  XiphQT is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  XiphQT is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with XiphQT; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *
 *  Last modified: $Id$
 *
 */


#if !defined(__stream_speex_h__)
#define __stream_speex_h__

#include <Ogg/ogg.h>
#include <Speex/speex.h>

#include "importer_types.h"

extern int recognize_header__speex(ogg_page *op);
extern int verify_header__speex(ogg_page *op);

extern int initialize_stream__speex(StreamInfo *si);
extern void clear_stream__speex(StreamInfo *si);
extern ComponentResult create_sample_description__speex(StreamInfo *si);

extern int process_first_packet__speex(StreamInfo *si, ogg_page *op, ogg_packet *opckt);
extern ComponentResult process_stream_page__speex(OggImportGlobals *globals, StreamInfo *si, ogg_page *opg);

#define HANDLE_FUNCTIONS__SPEEX { &process_stream_page__speex, &recognize_header__speex, \
            &verify_header__speex, &process_first_packet__speex, &create_sample_description__speex, \
            &initialize_stream__speex, &clear_stream__speex }


#endif /* __stream_vorbis_h__ */
