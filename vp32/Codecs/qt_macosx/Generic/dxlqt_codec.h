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

#ifndef _dxlqt_codec_h
#define _dxlqt_codec_h 1


#if defined(VP3_COMPRESS)

//#define USE_SHARED_COMPCONFIG 1

//#include <iostream.h>
#include <stdio.h>

extern "C" 
{
	#include "vfw_comp_interface.h"
	#include "cclib.h"
}

#endif

#if !TARGET_OS_MAC
#define REGACCESS "SOFTWARE\\On2\\QTW Encoder/Decoder\\VP31"
#endif 

// shared comp config used by all instances
typedef struct	
{
    COMP_CONFIG			*CompConfig;
} sharedGlobals;

// Globals Structure passed around to many functions -> one Per Open
typedef struct	
{
	ComponentInstance	self;					// the component we are using
	ComponentInstance	delegateComponent;		// 
	ComponentInstance	target;					// the output target component
	unsigned long		instancenum;			// Which instance of Movie Player
	RegistryAccess		regAccess;				// registry/resource fork access field

#if DECO_BUILD

#if DXV_DECOMPRESS								// decompressor variables
	DXL_XIMAGE_HANDLE   xImage;					// Internally used by DXV
	DXL_VSCREEN_HANDLE  vScreen;				// Internally used by DXV
#endif

	OSType				**PixelFormatList;		// list of the pixel form.s we support 
	long				lastframe;				// last call flags-check for erase vscreen
	long				lastdrawn;				// last frame drawn
	unsigned long		startTime;				// time start playing a video segment 
	unsigned long		endTime;				// end time playing a video segment   
	long				dxvFrames;				// frames played through dxv
	int					showWhiteDots;			// Show white dots on decompress
	int					softwareStretch;		// Use software stretching, or QT default
	int					useBlackLine;			// BlackLining on or off 
	unsigned long		stretchThis;			// Stretch this frame
    int                 postProcessLevel;       // PostProcessing Level
	int					cpuFree;				// for postprocessing
#endif 

#if COMP_BUILD									// compressor variables 

#ifdef VP3_COMPRESS

	COMP_CONFIG			CompConfig;
//    COMP_CONFIG			*sharedCompConfig;

	YUV_INPUT_BUFFER_CONFIG  yuv_config;
	xCP_INST cpi;
	unsigned char *yuv_buffer;
#endif

#endif 

} dxlqt_GlobalsRecord, *dxlqt_GlobalsPtr, **dxlqt_Globals;

// Decompress Record structure passed to draw band -> once per screen
typedef struct 
{
	long		width;							// Width of area to decompress to
	long		height;							// Height of area to decompress to
	long		pitch;							// Pitch in bytes of decompress area
	long		scrnchanged;					// Whether call and cond flags have changed
	enum BITDEPTH bitdepth;						// DXV Bitdepth to decompress to
	long		fourcc;							// fourcc
	long		compressedsize;					// size of compressed data

} dxlqt_DecompressRecord;

//******************************************************************
// Some prototypes
//******************************************************************
#if COMP_BUILD									// compressor variables 
#ifdef VP3_COMPRESS
void 
getCompConfigDefaultSettings(COMP_CONFIG *CompConfig);
#endif
#endif

#if 0
//******************************************************************
// Some Macros For Logging 
//******************************************************************
#define DXLQT_LOGGING 1

#ifdef DXLQT_LOGGING

extern  char logmsg[2000];
extern   ::ofstream logfile;



#if TARGET_OS_MAC

#define DXLQT_LOG(x) if(logfile) { x; logfile<<logmsg; };

#else

#define DXLQT_LOG(x) \
	  if(logfile) { x; logfile<<logmsg;OutputDebugString(logmsg);};

#endif

#else
  #define DXLQT_LOG(x) 
#endif




#else
//******************************************************************
// Some Macros For Logging 
//******************************************************************

//#define DXLQT_LOGGING 1


#ifdef DXLQT_LOGGING
extern  char logmsg[2000];
  //::ofstream logfile;  -TF
extern   FILE* logfile;// = fopen("dxlqt.log", "a");

#if TARGET_OS_MAC

//#define DXLQT_LOG(x) if(logfile) { x; logfile<<logmsg; };
#define DXLQT_LOG(x) if(logfile) {x; fprintf(logfile, "%s\n", logmsg); };

#else

//#define DXLQT_LOG(x) \
//	  if(logfile) { x; logfile<<logmsg;OutputDebugString(logmsg);};
#define DXLQT_LOG(x) if(logfile) {x; fprintf(logfile, "%s\n", logmsg); };

#endif

#else
  #define DXLQT_LOG(x) 
#endif

#endif

#endif
