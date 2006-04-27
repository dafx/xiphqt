/*
 *  stream_types_theora.h
 *
 *    Definition of Theora specific data structures.
 *
 *
 *  Copyright (c) 2006  Arek Korbik
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


#if !defined(__stream_types_theora_h__)
#define __stream_types_theora_h__


#if !defined(_NO_THEORA_SUPPORT)
#include <Theora/theoradec.h>

typedef enum TheoraImportStates {
    kTStateInitial,
    kTStateReadingComments,
    kTStateReadingCodebooks,
    kTStateReadingFirstPacket,
    //kTStateSeekingLastPacket,
    kTStateReadingPackets
} TheoraImportStates;

typedef struct {
    TheoraImportStates state;

    th_info ti;
    th_comment tc;
    th_setup_info *ts;

    UInt32 granulepos_shift;
    UInt32 fps_framelen;
} StreamInfo__theora;


#define _HAVE__THEORA_SUPPORT 1
#endif

#endif /* __stream_types_theora_h__ */
