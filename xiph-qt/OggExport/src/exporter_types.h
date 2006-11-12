/*
 *  exporter_types.h
 *
 *    Definitions of OggExporter data structures.
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


#if !defined(__exporter_types_h__)
#define __exporter_types_h__

#if defined(__APPLE_CC__)
#include <QuickTime/QuickTime.h>
#include <Ogg/ogg.h>
#else
#include <QuickTimeComponents.h>
#include <ogg.h>

#if defined(TARGET_OS_WIN32)
#define _WINIOCTL_
#include <windows.h>
#endif

#endif
//#include "rb.h"


#include "stream_types_audio.h"
//#include "stream_types_video.h"


enum {
    kOES_init_og_size = 5120,
};

struct stream_format_handle_funcs; //forward declaration

typedef struct {
    long serialno;

    ogg_stream_state os;

    ogg_page og;
    void * og_buffer;
    UInt32 og_buffer_size;
    ogg_int64_t og_grpos;
    Boolean og_ready;

    //UInt32   og_ts_sec;
    //Float64  og_ts_subsec;

    ogg_int64_t packets_total;
    ogg_int64_t acc_packets;
    ogg_int64_t last_grpos;
    ogg_int64_t acc_duration;

    OSType stream_type;

    long trackID;
    MovieExportGetPropertyUPP getPropertyProc;
    MovieExportGetDataUPP getDataProc;
    void *refCon;
    MovieExportGetDataParams gdp;

    Boolean src_extract_complete;
    Boolean eos;

    TimeScale sourceTimeScale;

    TimeValue time;
    //long numOfFrames;

    //Ptr compressBuffer;
    //Size compressBufferSize;

    void * out_buffer;
    UInt32 out_buffer_size;

    long lastDescSeed;

    //ComponentInstance stdAudio;

    //ImageSequence decompressSequence;
    //GWorldPtr gw;
    //PixMapHandle hPixMap;

    struct stream_format_handle_funcs *sfhf;

    union {
#if defined(_HAVE__OE_AUDIO)
        StreamInfo__audio si_a;
#endif
#if defined(_HAVE__OE_VIDEO)
        StreamInfo__video si_v;
#endif
    };

} StreamInfo, *StreamInfoPtr;

typedef struct {
    ComponentInstance  self;
    ComponentInstance  quickTimeMovieExporter;

    int                streamCount;
    StreamInfo         **streamInfoHandle;

    MovieProgressUPP   progressProc;
    long               progressRefcon;

    Boolean            canceled;

    Boolean            use_hires_audio;
} OggExportGlobals, *OggExportGlobalsPtr;


typedef Boolean (*can_handle_track) (OSType trackType, TimeScale scale,
                                     MovieExportGetPropertyUPP getPropertyProc,
                                     void *refCon);
typedef ComponentResult (*validate_movie) (OggExportGlobals *globals,
                                           Movie theMovie, Track onlyThisTrack,
                                           Boolean *valid);
typedef ComponentResult (*configure_stream) (OggExportGlobals *globals,
                                             StreamInfo *si);

typedef ComponentResult (*write_i_header) (StreamInfoPtr si, DataHandler data_h,
                                           wide *offset);
typedef ComponentResult (*write_headers) (StreamInfoPtr si, DataHandler data_h,
                                          wide *offset);

typedef ComponentResult (*fill_page) (OggExportGlobalsPtr globals,
                                      StreamInfoPtr si, Float64 max_duration,
                                      UInt32 *pos_sec, Float64 *pos_subsec);

typedef ComponentResult (*initialize_stream) (StreamInfo *si);
typedef void (*clear_stream) (StreamInfo *si);

typedef struct stream_format_handle_funcs {
    can_handle_track      can_handle;
    validate_movie        validate;

    configure_stream      configure;

    write_i_header        write_i_header;
    write_headers         write_headers;

    fill_page             fill_page;

    initialize_stream     initialize;
    clear_stream          clear;
} stream_format_handle_funcs;

#define HANDLE_FUNCTIONS__NULL { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }

#endif /* __exporter_types_h__ */
