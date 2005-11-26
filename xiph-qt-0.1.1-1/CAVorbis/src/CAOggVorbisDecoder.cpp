/*
 *  CAOggVorbisDecoder.cpp
 *
 *    CAOggVorbisDecoder class implementation; translation layer handling
 *    ogg page encapsulation of Vorbis packets, using CAVorbisDecoder
 *    for the actual decoding.
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
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *
 *  Last modified: $Id$
 *
 */


#include "CAOggVorbisDecoder.h"

#include "fccs.h"
#include "data_types.h"

#include "wrap_ogg.h"

#include "debug.h"


CAOggVorbisDecoder::CAOggVorbisDecoder() :
CAVorbisDecoder(true),
mFramesBufferedList()
{
    CAStreamBasicDescription theInputFormat(kAudioStreamAnyRate, kAudioFormatXiphOggFramedVorbis,
                                            kVorbisBytesPerPacket, kVorbisFramesPerPacket,
                                            kVorbisBytesPerFrame, kVorbisChannelsPerFrame,
                                            kVorbisBitsPerChannel, kVorbisFormatFlags);
    AddInputFormat(theInputFormat);
    
    mInputFormat.mSampleRate = 44100;
    mInputFormat.mFormatID = kAudioFormatXiphOggFramedVorbis;
    mInputFormat.mFormatFlags = kVorbisFormatFlags;
    mInputFormat.mBytesPerPacket = kVorbisBytesPerPacket;
    mInputFormat.mFramesPerPacket = kVorbisFramesPerPacket;
    mInputFormat.mBytesPerFrame = kVorbisBytesPerFrame;
    mInputFormat.mChannelsPerFrame = 2;
    mInputFormat.mBitsPerChannel = kVorbisBitsPerChannel;
    
    CAStreamBasicDescription theOutputFormat1(kAudioStreamAnyRate, kAudioFormatLinearPCM, 0, 1, 0, 0, 16,
                                              kAudioFormatFlagsNativeEndian |
                                              kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked);
    AddOutputFormat(theOutputFormat1);
    CAStreamBasicDescription theOutputFormat2(kAudioStreamAnyRate, kAudioFormatLinearPCM, 0, 1, 0, 0, 32,
                                              kAudioFormatFlagsNativeFloatPacked);
    AddOutputFormat(theOutputFormat2);
    
    mOutputFormat.mSampleRate = 44100;
	mOutputFormat.mFormatID = kAudioFormatLinearPCM;
	mOutputFormat.mFormatFlags = kAudioFormatFlagsNativeFloatPacked;
	mOutputFormat.mBytesPerPacket = 8;
	mOutputFormat.mFramesPerPacket = 1;
	mOutputFormat.mBytesPerFrame = 8;
	mOutputFormat.mChannelsPerFrame = 2;
	mOutputFormat.mBitsPerChannel = 32;
}

CAOggVorbisDecoder::~CAOggVorbisDecoder()
{
    if (mCompressionInitialized)
        ogg_stream_clear(&mO_st);
}

void CAOggVorbisDecoder::SetCurrentInputFormat(const AudioStreamBasicDescription& inInputFormat)
{
	if (!mIsInitialized) {
		if (inInputFormat.mFormatID != kAudioFormatXiphOggFramedVorbis) {
			dprintf("CAOggVorbisDecoder::SetFormats: only support Xiph Vorbis (Ogg-framed) for input\n");
			CODEC_THROW(kAudioCodecUnsupportedFormatError);
		}
        XCACodec::SetCurrentInputFormat(inInputFormat);
	} else {
		CODEC_THROW(kAudioCodecStateError);
	}
}

UInt32 CAOggVorbisDecoder::ProduceOutputPackets(void* outOutputData, UInt32& ioOutputDataByteSize, UInt32& ioNumberPackets,
                                                AudioStreamPacketDescription* outPacketDescription)
{
    dprintf(" >> [%08lx] CAOggVorbisDecoder :: ProduceOutputPackets(%ld [%ld])\n", (UInt32) this, ioNumberPackets, ioOutputDataByteSize);
    UInt32 ret = kAudioCodecProduceOutputPacketSuccess;

    if (mFramesBufferedList.empty()) {
        ioOutputDataByteSize = 0;
        ioNumberPackets = 0;
        ret = kAudioCodecProduceOutputPacketNeedsMoreInputData;
        dprintf("<!E [%08lx] CAOggVorbisDecoder :: ProduceOutputPackets(%ld [%ld]) = %ld [%ld]\n", (UInt32) this,
                ioNumberPackets, ioOutputDataByteSize, ret, FramesReady());
        return ret;
    }

    UInt32 vorbis_packets = mFramesBufferedList.front();
    UInt32 ogg_packets = 0;
    UInt32 vorbis_returned_data = ioOutputDataByteSize;
    UInt32 vorbis_total_returned_data = 0;
    Byte *the_data = static_cast<Byte*> (outOutputData);

    while (true) {
        UInt32 vorbis_return = CAVorbisDecoder::ProduceOutputPackets(the_data, vorbis_returned_data, vorbis_packets, NULL);
        if (vorbis_return == kAudioCodecProduceOutputPacketSuccess || vorbis_return == kAudioCodecProduceOutputPacketSuccessHasMore) {
            if (vorbis_packets > 0)
                mFramesBufferedList.front() -= vorbis_packets;

            if (mFramesBufferedList.front() <= 0) {
                ogg_packets++;
                mFramesBufferedList.erase(mFramesBufferedList.begin());
            }
            
            vorbis_total_returned_data += vorbis_returned_data;

            if (vorbis_total_returned_data == ioOutputDataByteSize || vorbis_return == kAudioCodecProduceOutputPacketSuccess)
            {
                ioNumberPackets = ogg_packets;
                ioOutputDataByteSize = vorbis_total_returned_data;

                if (!mFramesBufferedList.empty())
                    ret = kAudioCodecProduceOutputPacketSuccessHasMore;
                else
                    ret = kAudioCodecProduceOutputPacketSuccess;

                break;
            } else {
                the_data += vorbis_returned_data;
                vorbis_returned_data = ioOutputDataByteSize - vorbis_total_returned_data;
                vorbis_packets = mFramesBufferedList.front();
            }
        } else {
            ret = kAudioCodecProduceOutputPacketFailure;
            ioOutputDataByteSize = vorbis_total_returned_data;
            ioNumberPackets = ogg_packets;
            break;
        }
    }

    dprintf("<.. [%08lx] CAOggVorbisDecoder :: ProduceOutputPackets(%ld [%ld]) = %ld [%ld]\n",
            (UInt32) this, ioNumberPackets, ioOutputDataByteSize, ret, FramesReady());
    return ret;
}


void CAOggVorbisDecoder::BDCInitialize(UInt32 inInputBufferByteSize)
{
    CAVorbisDecoder::BDCInitialize(inInputBufferByteSize);
}

void CAOggVorbisDecoder::BDCUninitialize()
{
    mFramesBufferedList.clear();
    CAVorbisDecoder::BDCUninitialize();
}

void CAOggVorbisDecoder::BDCReset()
{
    mFramesBufferedList.clear();
    if (mCompressionInitialized)
        ogg_stream_reset(&mO_st);
    CAVorbisDecoder::BDCReset();
}

void CAOggVorbisDecoder::BDCReallocate(UInt32 inInputBufferByteSize)
{
    mFramesBufferedList.clear();
    CAVorbisDecoder::BDCReallocate(inInputBufferByteSize);
}


void CAOggVorbisDecoder::InPacket(const void* inInputData, const AudioStreamPacketDescription* inPacketDescription)
{
    if (!mCompressionInitialized)
        CODEC_THROW(kAudioCodecUnspecifiedError);
    
    ogg_page op;

    if (!WrapOggPage(&op, inInputData, inPacketDescription->mDataByteSize, inPacketDescription->mStartOffset))
        CODEC_THROW(kAudioCodecUnspecifiedError);

    ogg_packet opk;
    SInt32 packet_count = 0;
    int oret;
    AudioStreamPacketDescription vorbis_packet_desc = {0, 0, 0};
    UInt32 page_packets = ogg_page_packets(&op);

    ogg_stream_pagein(&mO_st, &op);
    while ((oret = ogg_stream_packetout(&mO_st, &opk)) != 0) {
        if (oret < 0) {
            page_packets--;
            continue;
        }
        
        packet_count++;

        vorbis_packet_desc.mDataByteSize = opk.bytes;

        CAVorbisDecoder::InPacket(opk.packet, &vorbis_packet_desc);
    }

    mFramesBufferedList.push_back(packet_count);
}


void CAOggVorbisDecoder::InitializeCompressionSettings()
{
    if (mCookie != NULL) {
        if (mCompressionInitialized)
            ogg_stream_clear(&mO_st);

        OggSerialNoAtom *atom = reinterpret_cast<OggSerialNoAtom*> (mCookie);
    
        if (EndianS32_BtoN(atom->type) == kCookieTypeOggSerialNo && EndianS32_BtoN(atom->size) <= mCookieSize) {
            ogg_stream_init(&mO_st, EndianS32_BtoN(atom->serialno));
        }
    }

    ogg_stream_reset(&mO_st);

    CAVorbisDecoder::InitializeCompressionSettings();
}
