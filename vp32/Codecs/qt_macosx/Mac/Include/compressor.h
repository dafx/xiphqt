//============================================================================
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1999 - 2001  On2 Technologies Inc. All Rights Reserved.
//
//----------------------------------------------------------------------------

#ifndef _comp_H_
#define _comp_H_ 1

#ifdef ACCESSOR_CALLS_ARE_FUNCTIONS
#undef ACCESSOR_CALLS_ARE_FUNCTIONS
#endif

#define ACCESSOR_CALLS_ARE_FUNCTIONS 1

#ifdef OPAQUE_TOOLBOX_STRUCTS
#undef OPAQUE_TOOLBOX_STRUCTS
#endif

#define OPAQUE_TOOLBOX_STRUCTS 1

#ifdef TARGET_API_MAC_CARBON
#undef TARGET_API_MAC_CARBON
#endif

#define TARGET_API_MAC_CARBON 1

#ifdef TARGET_OS_MAC
#undef TARGET_OS_MAC
#endif

#define TARGET_OS_MAC 1
#define ON2_OSX_QTX 1
//#include <ConditionalMacros.h>

#define COMP_BUILD 1
#define DECO_BUILD 1
#define DXV_DECOMPRESS 1
#define VP3_COMPRESS 1
//#define FAKE_DECOMPRESS 1
#define	Build_resID	resid_Deco
#include "Common.h"
#include "folders.h"

#define MACPPC
#endif
