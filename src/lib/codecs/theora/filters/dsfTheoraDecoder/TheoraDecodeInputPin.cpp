//===========================================================================
//Copyright (C) 2003, 2004 Zentaro Kavanagh
//
//Redistribution and use in source and binary forms, with or without
//modification, are permitted provided that the following conditions
//are met:
//
//- Redistributions of source code must retain the above copyright
//  notice, this list of conditions and the following disclaimer.
//
//- Redistributions in binary form must reproduce the above copyright
//  notice, this list of conditions and the following disclaimer in the
//  documentation and/or other materials provided with the distribution.
//
//- Neither the name of Zentaro Kavanagh nor the names of contributors 
//  may be used to endorse or promote products derived from this software 
//  without specific prior written permission.
//
//THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
//LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
//PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE ORGANISATION OR
//CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
//EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
//PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
//PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
//LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//===========================================================================

#include "stdafx.h"
#include "theoradecodeinputpin.h"


TheoraDecodeInputPin::TheoraDecodeInputPin(CTransformFilter* inParentFilter, HRESULT* outHR) 
	:	CTransformInputPin(NAME("Theora Input Pin"), inParentFilter, outHR, L"Theora In")
	,	mSetupState(VSS_SEEN_NOTHING)
{
	//debugLog.open("G:\\logs\\theoinput.log", ios_base::out);
}
TheoraDecodeInputPin::~TheoraDecodeInputPin() {
	//debugLog.close();
}

STDMETHODIMP TheoraDecodeInputPin::NonDelegatingQueryInterface(REFIID riid, void **ppv) 
{



	if (riid == IID_IMediaSeeking) {
		//debugLog<<"Got Seeker"<<endl;
		*ppv = (IMediaSeeking*)this;
		((IUnknown*)*ppv)->AddRef();
		
		return NOERROR;
	} else if (riid == IID_IOggDecoder) {
		*ppv = (IOggDecoder*)this;
		//((IUnknown*)*ppv)->AddRef();
		return NOERROR;

	}

	return CBaseInputPin::NonDelegatingQueryInterface(riid, ppv); 
}

HRESULT TheoraDecodeInputPin::BreakConnect() {
	CAutoLock locLock(m_pLock);
	//debugLog<<"Break conenct"<<endl;
	//Need a lock ??
	ReleaseDelegate();
	return CTransformInputPin::BreakConnect();
}
HRESULT TheoraDecodeInputPin::CompleteConnect (IPin *inReceivePin) {
	CAutoLock locLock(m_pLock);
	//debugLog<<"Complete conenct"<<endl;
	IMediaSeeking* locSeeker = NULL;
	inReceivePin->QueryInterface(IID_IMediaSeeking, (void**)&locSeeker);
	if (locSeeker == NULL) {
		//debugLog<<"Seeker is null"<<endl;
	}
	SetDelegate(locSeeker);
	return CTransformInputPin::CompleteConnect(inReceivePin);
}

LOOG_INT64 TheoraDecodeInputPin::convertGranuleToTime(LOOG_INT64 inGranule)
{
	//if (mBegun) {	
	//	return (inGranule * UNITS) / mSampleRate;
	//} else {
	//	return -1;
	//}

	//XTODO:::
	return -1;
}

LOOG_INT64 TheoraDecodeInputPin::mustSeekBefore(LOOG_INT64 inGranule)
{
	//TODO::: Get adjustment from block size info... for now, it doesn't matter if no preroll
	return inGranule;
}
IOggDecoder::eAcceptHeaderResult TheoraDecodeInputPin::showHeaderPacket(OggPacket* inCodecHeaderPacket)
{
	unsigned char* locPacketData = new unsigned char[inCodecHeaderPacket->packetSize()];
	memcpy((void*)locPacketData, (const void**)inCodecHeaderPacket->packetData(), inCodecHeaderPacket->packetSize());
	StampedOggPacket* locStamped = new StampedOggPacket(locPacketData, inCodecHeaderPacket->packetSize(), false, false, 0,0, StampedOggPacket::NONE);

	TheoraDecodeFilter* locParent = (TheoraDecodeFilter*)m_pFilter;

	IOggDecoder::eAcceptHeaderResult retResult = IOggDecoder::AHR_INVALID_HEADER;
	switch (mSetupState) {
		case VSS_SEEN_NOTHING:
			if (strncmp((char*)inCodecHeaderPacket->packetData(), "\200theora", 7) == 0) {
				//TODO::: Possibly verify version
				if (locParent->mTheoraDecoder->decodeTheora(locStamped) == NULL) {
					mSetupState = VSS_SEEN_BOS;
					retResult = IOggDecoder::AHR_MORE_HEADERS_TO_COME;
				}
			}
			//return IOggDecoder::AHR_INVALID_HEADER;
			break;
			
			
		case VSS_SEEN_BOS:
			if (strncmp((char*)inCodecHeaderPacket->packetData(), "\201theora", 7) == 0) {
				if (locParent->mTheoraDecoder->decodeTheora(locStamped) == NULL) {
					mSetupState = VSS_SEEN_COMMENT;
					retResult = IOggDecoder::AHR_MORE_HEADERS_TO_COME;
				}
				
				
			}
			//return IOggDecoder::AHR_INVALID_HEADER;
			break;
			
			
		case VSS_SEEN_COMMENT:
			if (strncmp((char*)inCodecHeaderPacket->packetData(), "\202theora", 7) == 0) {
				if (locParent->mTheoraDecoder->decodeTheora(locStamped) == NULL) {
		
					//fish_sound_command (mFishSound, FISH_SOUND_GET_INFO, &(mFishInfo), sizeof (FishSoundInfo)); 
					//Is mBegun useful ?
					//mBegun = true;
			
					//mNumChannels = mFishInfo.channels;
					//mFrameSize = mNumChannels * SIZE_16_BITS;
					//mSampleRate = mFishInfo.samplerate;

		
					mSetupState = VSS_ALL_HEADERS_SEEN;
					retResult = IOggDecoder::AHR_ALL_HEADERS_RECEIVED;
				}
				
			}
			//return IOggDecoder::AHR_INVALID_HEADER;
			break;
			
		case VSS_ALL_HEADERS_SEEN:
		case VSS_ERROR:
		default:
			retResult = IOggDecoder::AHR_UNEXPECTED;
	}
	delete locStamped;
	return retResult;
}
string TheoraDecodeInputPin::getCodecShortName()
{
	return "theora";
}
string TheoraDecodeInputPin::getCodecIdentString()
{
	//TODO:::
	return "theora";
}