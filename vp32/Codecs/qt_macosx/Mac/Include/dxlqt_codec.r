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

#define thng_RezTemplateVersion 1
#define cfrg_RezTemplateVersion 1

#include <ConditionalMacros.r>
#include <MacTypes.r>
#include <Components.r>
#include <QuickTimeComponents.r>
#include <ImageCompression.r>
#include <CodeFragments.r>
#include <ImageCodec.r>
#include "Common.h"

#define Target_PlatformType      platformPowerPCNativeEntryPoint
#define	kCodecVendor		'On2 '
#define	kCodecFormatName	"VP31"
#define	kCodecFormatNameC	"On2 VP3 Video 3.2"

#define kDecoFlags		    ( codecInfoDoes32 | codecInfoDoes16 )
#define	kVP31FormatType	    'VP31'
#define kCompFlags          ( codecInfoDoes32 | codecInfoDoesTemporal | codecInfoDoesRateConstrain)
#define kFormatFlags		( codecInfoDepth32 )

#define resid_Codec_cfrg 400
//#define resid_Deco_cfrg 401

resource 'STR ' (130) {
	"VP32 Video Player"
};

//*************************************************************************
//	Version INFO
//*************************************************************************
#define SHORTVERSIONSTRING "3.2.2.0"
#define LONGVERSIONSTRING "On2 VP3 Video Codec 3.2.2.0\nOn2 Technologies, The Duck Corporation 2001"

 resource 'vers' (1, purgeable) {
    0x03, 0x21, final, 0x03, verUS,
    SHORTVERSIONSTRING,
    LONGVERSIONSTRING
 };
 resource 'vers' (2, purgeable) {
    0x03, 0x21, final, 0x03, verUS,
    SHORTVERSIONSTRING,
    "(for QuickTime 4.0 or greater)"
 };

//*************************************************************************
//	VP32 COMPRESSOR INFO
//*************************************************************************
resource 'cdci' (resid_Comp) {
	kCodecFormatNameC,		    //	Format Name
	Codec_Version,				//	Version
	Codec_Revision,				//	Rev Level
	kCodecVendor,			    //	Vendor
	kDecoFlags,			        //	Decompress Flags
	kCompFlags,					//	Compress Flags
	kFormatFlags,			    //	Format Flags
	100,						//	Compression Accuracy
	100,						//	Decompression Accuracy
	200,						//	Compression Speed
	200,						//	Decompression Speed
	100,						//	Compression Level
	0,							//	Reserved
	2,							//	Min Height
	2,							//	Min Width
	0,							//	Decompress Pipeline Latency
	0,							//	Compress Pipeline Latency
	0							//	Private Data
};
      
resource 'thng' (resid_Comp) {
	compressorComponentType,	// Type
	kVP31FormatType,		    // Sub-Type
	kCodecVendor,			    // Vendor
	//kCompFlags,			        // Flags
	0,							// Flags not used now..
	0,							// Mask
	'cdec', resid_Comp,			// ID
	'STR ',	resid_CompName,		// Name
	'STR ',	resid_CompInfo,		// Info
	0,	0,						// Icon
	Codec_Version,
	componentHasMultiplePlatforms + 
		componentDoAutoVersion,
	0,
	{
    	kCompFlags, 
    	'cfrg', 
        0,
    	platformPowerPCNativeEntryPoint
	}
};

resource 'STR ' (resid_CompInfo) {
	"Compresses images into the On2 VP3 format."
};
resource 'STR ' (resid_CompName) {
	"On2 VP3 Video Compressor"
};


//*************************************************************************
//	VP32 DECOMPRESSOR INFO
//*************************************************************************
resource 'cdci' (resid_Deco) 
{
	kCodecFormatName,		    //	Format Name
	Codec_Version,				//	Version
	Codec_Revision,				//	Rev Level
	kCodecVendor,			    //	Vendor
	kDecoFlags,			        //	Decompress Flags
	kCompFlags,					//	Compress Flags
	kFormatFlags,			    //	Format Flags
	100,						//	Compression Accuracy
	100,						//	Decompression Accuracy
	200,						//	Compression Speed
	200,						//	Decompression Speed
	100,						//	Compression Level
	0,							//	Reserved
	2,							//	Min Height
	2,							//	Min Width
	0,							//	Decompress Pipeline Latency
	0,							//	Compress Pipeline Latency
	0							//	Private Data
};

resource 'thng' (resid_Deco) 
{
	decompressorComponentType,	// Type
	kVP31FormatType,		    // Sub-Type
	kCodecVendor,			    // Vendor
	kDecoFlags,			        // Flags
	0,							// Mask
	'cdec', resid_Deco,			// ID
	'STR ',	resid_DecoName,		// Name
	'STR ',	resid_DecoInfo,		// Info
	0,	0,						// Icon
	Codec_Version,

	componentHasMultiplePlatforms + 
		componentDoAutoVersion,
	0,
	{
    	kDecoFlags, 
    	'cfrg', 
    	0,
    	platformPowerPCNativeEntryPoint
	}
};

resource 'cfrg' (0) {
{
    extendedEntry {
 	    kPowerPCCFragArch,			// archType
        kIsCompleteCFrag,			// updateLevel
		kNoVersionNum,				// currentVersion
		kNoVersionNum,				// oldDefVersion
		kDefaultStackSize,			// appStackSize
		kNoAppSubFolder,			// appSubFolderID
		kImportLibraryCFrag,		// usage
		kDataForkCFragLocator,		// where
		kZeroOffset,				// offset
		kCFragGoesToEOF,			// length
		"On2 VP3 Codec",			// member name
		
		// start extended info
		'cpnt',						// libKind
		"\0x00\0x00",				// qualifier 1 - hex 
		                            // 0x00 (0) matches code ID in 'thng' 128
		"",				            // qualifier 2 
		"",							// qualifier 3
		"On2 VP3 Video",		// intlName, localised
		};
	};
};
