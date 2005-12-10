/*
 *  data_types.h
 *
 *    Definitions of common data structures shared between different
 *    components.
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


#if !defined(__data_types_h__)
#define __data_types_h__


enum {
    kCookieTypeOggSerialNo = 'oCtN'
};

// format specific cookie types

enum {
    kCookieTypeVorbisHeader = 'vCtH',
    kCookieTypeVorbisComments = 'vCt#',
    kCookieTypeVorbisCodebooks = 'vCtC'
};

enum {
    kCookieTypeSpeexHeader = 'sCtH',
    kCookieTypeSpeexComments = 'sCt#',
    kCookieTypeSpeexExtraHeader	= 'sCtX'
};


struct OggSerialNoAtom {
    long size;			// = sizeof(OggSerialNoAtom)
    long type;			// = kOggCookieSerialNoType
    long serialno;
};
typedef struct OggSerialNoAtom OggSerialNoAtom;

struct CookieAtomHeader {
    long           size;
    long           type;
    unsigned char  data[1];
};
typedef struct CookieAtomHeader CookieAtomHeader;


#endif /* __data_types_h__ */
