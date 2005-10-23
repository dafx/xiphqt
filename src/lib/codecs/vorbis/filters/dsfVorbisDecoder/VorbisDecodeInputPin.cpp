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

#include "VorbisDecodeInputPin.h"


VorbisDecodeInputPin::VorbisDecodeInputPin	(		AbstractTransformFilter* inFilter
												,	CCritSec* inFilterLock
												,	AbstractTransformOutputPin* inOutputPin
												,	vector<CMediaType*> inAcceptableMediaTypes
											)

	:	AbstractTransformInputPin			(		inFilter
												,	inFilterLock
												,	inOutputPin
												,	NAME("VorbisDecodeInputPin")
												,	L"Vorbis In", inAcceptableMediaTypes
											)
	,	mBegun(false)
	,	mFishSound(NULL)

	,	mNumChannels(0)
	,	mFrameSize(0)
	,	mSampleRate(0)
	,	mUptoFrame(0)
	,	mSetupState(VSS_SEEN_NOTHING)
	,	mDecodedBuffer(NULL)
	,	mDecodedByteCount(0)
		
{
	//debugLog.open("g:\\logs\\vorbislog.log", ios_base::out);
	ConstructCodec();

	mDecodedBuffer = new unsigned char[DECODED_BUFFER_SIZE];
}

VorbisDecodeInputPin::~VorbisDecodeInputPin(void)
{
	//debugLog.close();
	DestroyCodec();
	delete[] mDecodedBuffer;

}
//Is this needed ??
STDMETHODIMP VorbisDecodeInputPin::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	if (riid == IID_IMediaSeeking) {
		*ppv = (IMediaSeeking*)this;
		((IUnknown*)*ppv)->AddRef();
		return NOERROR;
	} else if (riid == IID_IOggDecoder) {
		*ppv = (IOggDecoder*)this;
		((IUnknown*)*ppv)->AddRef();
		return NOERROR;

	}

	return AbstractTransformInputPin::NonDelegatingQueryInterface(riid, ppv); 
}
bool VorbisDecodeInputPin::ConstructCodec() 
{
	mFishSound = fish_sound_new (FISH_SOUND_DECODE, &mFishInfo);			//Deleted by destroycodec from destructor.

	int i = 1;
	//FIX::: Use new API for interleave setting
	fish_sound_command(mFishSound, FISH_SOUND_SET_INTERLEAVE, &i, sizeof(int));

	fish_sound_set_decoded_callback (mFishSound, VorbisDecodeInputPin::VorbisDecoded, this);
	//FIX::: Proper return value
	return true;
}
void VorbisDecodeInputPin::DestroyCodec() 
{
	fish_sound_delete(mFishSound);
	mFishSound = NULL;
}


STDMETHODIMP VorbisDecodeInputPin::NewSegment(REFERENCE_TIME inStartTime, REFERENCE_TIME inStopTime, double inRate) 
{
	CAutoLock locLock(mStreamLock);
	//debugLog<<"New segment "<<inStartTime<<" - "<<inStopTime<<endl;
	mUptoFrame = 0;
	return AbstractTransformInputPin::NewSegment(inStartTime, inStopTime, inRate);
	
}

int __cdecl VorbisDecodeInputPin::VorbisDecoded (FishSound* inFishSound, float** inPCM, long inFrames, void* inThisPointer) 
{
	
	VorbisDecodeInputPin* locThis = reinterpret_cast<VorbisDecodeInputPin*> (inThisPointer);
	VorbisDecodeFilter* locFilter = reinterpret_cast<VorbisDecodeFilter*>(locThis->m_pFilter);

	if (locThis->CheckStreaming() == S_OK) {
		//if (! locThis->mBegun) {
		//	//locThis->debugLog<<"First Time"<<endl;
		//	//Set up fishsound		
		//	fish_sound_command (locThis->mFishSound, FISH_SOUND_GET_INFO, &(locThis->mFishInfo), sizeof (FishSoundInfo)); 
		//	locThis->mBegun = true;
		//	
		//	locThis->mNumChannels = locThis->mFishInfo.channels;
		//	locThis->mFrameSize = locThis->mNumChannels * SIZE_16_BITS;
		//	locThis->mSampleRate = locThis->mFishInfo.samplerate;

		//}



		unsigned long locActualSize = inFrames * locThis->mFrameSize;
		unsigned long locTotalFrameCount = inFrames * locThis->mNumChannels;
		unsigned long locBufferRemaining = DECODED_BUFFER_SIZE - locThis->mDecodedByteCount;
		


		//Create a pointer into the buffer		
		signed short* locShortBuffer = (signed short*)&locThis->mDecodedBuffer[locThis->mDecodedByteCount];
		
		
		signed short tempInt = 0;
		float tempFloat = 0;
		
		//FIX:::Move the clipping to the abstract function

		if (locBufferRemaining >= locActualSize) {
			//Do float to int conversion with clipping
			const float SINT_MAX_AS_FLOAT = 32767.0f;
			for (unsigned long i = 0; i < locTotalFrameCount; i++) {
				//Clipping because vorbis puts out floats out of range -1 to 1
				if (((float*)inPCM)[i] <= -1.0f) {
					tempInt = SINT_MIN;	
				} else if (((float*)inPCM)[i] >= 1.0f) {
					tempInt = SINT_MAX;
				} else {
					//FIX:::Take out the unnescessary variable.
					tempFloat = ((( (float*) inPCM )[i]) * SINT_MAX_AS_FLOAT);
					//ASSERT((tempFloat <= 32767.0f) && (tempFloat >= -32786.0f));
					tempInt = (signed short)(tempFloat);
					//tempInt = (signed short) ((( (float*) inPCM )[i]) * SINT_MAX_AS_FLOAT);
				}
				
				*locShortBuffer = tempInt;
				locShortBuffer++;
			}

			locThis->mDecodedByteCount += locActualSize;
			
			return 0;
		} else {
			throw 0;
		}
	} else {
		DbgLog((LOG_TRACE,1,TEXT("Fishsound sending stuff we aren't ready for...")));
		return -1;
	}











































		//unsigned long locActualSize = inFrames * locThis->mFrameSize;
		//unsigned long locTotalFrameCount = inFrames * locThis->mNumChannels;
		//
		////locThis->debugLog<<"m_tStart = "<<locThis->m_tStart<<endl;
		////locThis->debugLog<<"mUptoFrame = "<<locThis->mUptoFrame<<endl;
		////Make the start presentation time
		//REFERENCE_TIME locFrameStart = (((__int64)(locThis->mUptoFrame * UNITS)) / locThis->mSampleRate);

		////Increment the frame counter
		//locThis->mUptoFrame += inFrames;

		////Make the end presentation time
		//REFERENCE_TIME locFrameEnd = (((__int64)(locThis->mUptoFrame * UNITS)) / locThis->mSampleRate);

		////locThis->debugLog<<"Sample time = "<<locFrameStart<<" - "<<locFrameEnd<<endl;
		//IMediaSample* locSample;
		//HRESULT locHR = locThis->mOutputPin->GetDeliveryBuffer(&locSample, &locFrameStart, &locFrameEnd, NULL);

		//if (locHR != S_OK) {
		//	return -1;
		//}	
		//

		////Create pointers for the samples buffer to be assigned to
		//BYTE* locBuffer = NULL;
		//signed short* locShortBuffer = NULL;
		//
		//locSample->GetPointer(&locBuffer);
		//locShortBuffer = (short *) locBuffer;
		//
		//signed short tempInt = 0;
		//float tempFloat = 0;
		//
		////FIX:::Move the clipping to the abstract function

		//if (locSample->GetSize() >= locActualSize) {
		//	//Do float to int conversion with clipping
		//	const float SINT_MAX_AS_FLOAT = 32767.0f;
		//	for (unsigned long i = 0; i < locTotalFrameCount; i++) {
		//		//Clipping because vorbis puts out floats out of range -1 to 1
		//		if (((float*)inPCM)[i] <= -1.0f) {
		//			tempInt = SINT_MIN;	
		//		} else if (((float*)inPCM)[i] >= 1.0f) {
		//			tempInt = SINT_MAX;
		//		} else {
		//			//FIX:::Take out the unnescessary variable.
		//			tempFloat = ((( (float*) inPCM )[i]) * SINT_MAX_AS_FLOAT);
		//			//ASSERT((tempFloat <= 32767.0f) && (tempFloat >= -32786.0f));
		//			tempInt = (signed short)(tempFloat);
		//			//tempInt = (signed short) ((( (float*) inPCM )[i]) * SINT_MAX_AS_FLOAT);
		//		}
		//		
		//		*locShortBuffer = tempInt;
		//		locShortBuffer++;
		//	}
		//	
		//	//Set the sample parameters.
		//	locThis->SetSampleParams(locSample, locActualSize, &locFrameStart, &locFrameEnd);

		//	{
		//
		//		CAutoLock locLock(locThis->m_pLock);

		//		//TODO::: Explain why we don't addref or release.
		//		HRESULT locHR = ((VorbisDecodeOutputPin*)(locThis->mOutputPin))->mDataQueue->Receive(locSample);
		//		if (locHR != S_OK) {
		//			DbgLog((LOG_TRACE,1,TEXT("Queue rejected us...")));
		//			return -1;
		//		}
		//	}

		//	
		//	return 0;
		//} else {
		//	throw 0;
		//}


}

STDMETHODIMP VorbisDecodeInputPin::Receive(IMediaSample* inSample) 
{
	CAutoLock locLock(mStreamLock);

	HRESULT locHR = CheckStreaming();

	if (locHR == S_OK) {
		if (!mBegun) {
			//locThis->debugLog<<"First Time"<<endl;
			//Set up fishsound		
			fish_sound_command (mFishSound, FISH_SOUND_GET_INFO, &(mFishInfo), sizeof (FishSoundInfo)); 
			mBegun = true;
			
			mNumChannels = mFishInfo.channels;
			mFrameSize = mNumChannels * SIZE_16_BITS;
			mSampleRate = mFishInfo.samplerate;

		}


		BYTE* locBuff = NULL;
		locHR = inSample->GetPointer(&locBuff);

		if (locHR != S_OK) {
			//TODO::: Do a debug dump or something here with specific error info.
			return locHR;
		} else {
			REFERENCE_TIME locStart = -1;
			REFERENCE_TIME locEnd = -1;
			__int64 locSampleDuration = 0;
			inSample->GetTime(&locStart, &locEnd);

			HRESULT locResult = TransformData(locBuff, inSample->GetActualDataLength());
			if (locResult != S_OK) {
				return S_FALSE;
			}
			if (locEnd > 0) {
				//Can dump it all downstream now	
				IMediaSample* locSample;
				unsigned long locBytesCopied = 0;
				unsigned long locBytesToCopy = 0;

				locStart = convertGranuleToTime(locEnd) - (((mDecodedByteCount / mFrameSize) * UNITS) / mSampleRate);
				do {
					HRESULT locHR = mOutputPin->GetDeliveryBuffer(&locSample, NULL, NULL, NULL);
					if (locHR != S_OK) {
						return locHR;
					}

					BYTE* locBuffer = NULL;
					locHR = locSample->GetPointer(&locBuffer);
				
					if (locHR != S_OK) {
						return locHR;
					}

					locBytesToCopy = ((mDecodedByteCount - locBytesCopied) <= locSample->GetSize()) ? (mDecodedByteCount - locBytesCopied) : locSample->GetSize();

					memcpy((void*)locBuffer, (const void*)&mDecodedBuffer[locBytesCopied], locBytesToCopy);
					locBytesCopied += locBytesToCopy;

					locSampleDuration = (((locBytesToCopy/mFrameSize) * UNITS) / mSampleRate);
					locEnd = locStart + locSampleDuration;

					//Write the sample meta data
					//TODO:: Seeking offset
					locSample->SetTime(&locStart, &locEnd);
					locSample->SetMediaTime(&locStart, &locEnd);
					locSample->SetSyncPoint(TRUE);
					locSample->SetActualDataLength(locBytesToCopy);
					locHR = ((VorbisDecodeOutputPin*)(mOutputPin))->mDataQueue->Receive(locSample);
					if (locHR != S_OK) {
						return locHR;
					}
					locStart += locSampleDuration;



				
				} while(locBytesCopied < mDecodedByteCount);

				mDecodedByteCount = 0;
				




			}
			return S_OK;

		}
	} else {
		//Not streaming - Bail out.
		return S_FALSE;
	}
}

HRESULT VorbisDecodeInputPin::TransformData(BYTE* inBuf, long inNumBytes) 
{
	//TODO::: Return types !!!

	//debugLog << "Decode called... Last Gran Pos : "<<mLastSeenStartGranPos<<endl;
	DbgLog((LOG_TRACE,1,TEXT("decodeData")));
	long locErr = fish_sound_decode(mFishSound, inBuf, inNumBytes);
	//FIX::: Do something here ?
	if (locErr < 0) {
		DbgLog((LOG_TRACE,1,TEXT("decodeData : fishsound returns < 0")));
		return S_FALSE;
		//debugLog <<"** Fish Sound error **"<<endl;
	} else {
		return S_OK;
		//debugLog << "Fish Sound OK >=0 "<<endl;
	}
	//return locErr;
}


HRESULT VorbisDecodeInputPin::SetMediaType(const CMediaType* inMediaType) {
	//FIX:::Error checking
	//RESOLVED::: Bit better.

	if (CheckMediaType(inMediaType) == S_OK) {
		((VorbisDecodeFilter*)mParentFilter)->setVorbisFormat(inMediaType->pbFormat);
		
	} else {
		throw 0;
	}
	return CBaseInputPin::SetMediaType(inMediaType);
}
HRESULT VorbisDecodeInputPin::CheckMediaType(const CMediaType *inMediaType)
{
	if (AbstractTransformInputPin::CheckMediaType(inMediaType) == S_OK) {
		if (inMediaType->cbFormat == VORBIS_IDENT_HEADER_SIZE) {
			if (strncmp((char*)inMediaType->pbFormat, "\001vorbis", 7) == 0) {
				//TODO::: Possibly verify version
				return S_OK;
			}
		}
	}
	return S_FALSE;
	
}

HRESULT VorbisDecodeInputPin::GetAllocatorRequirements(ALLOCATOR_PROPERTIES *outRequestedProps)
{
	outRequestedProps->cbBuffer = VORBIS_BUFFER_SIZE;
	outRequestedProps->cBuffers = VORBIS_NUM_BUFFERS;
	outRequestedProps->cbAlign = 1;
	outRequestedProps->cbPrefix = 0;

	return S_OK;
}

LOOG_INT64 VorbisDecodeInputPin::convertGranuleToTime(LOOG_INT64 inGranule)
{
	if (mBegun) {	
		return (inGranule * UNITS) / mSampleRate;
	} else {
		return -1;
	}
}
IOggDecoder::eAcceptHeaderResult VorbisDecodeInputPin::showHeaderPacket(OggPacket* inCodecHeaderPacket)
{
	switch (mSetupState) {
		case VSS_SEEN_NOTHING:
			if (strncmp((char*)inCodecHeaderPacket->packetData(), "\001vorbis", 7) == 0) {
				//TODO::: Possibly verify version
				if (fish_sound_decode(mFishSound, inCodecHeaderPacket->packetData(), inCodecHeaderPacket->packetSize()) >= 0) {
					mSetupState = VSS_SEEN_BOS;
					return IOggDecoder::AHR_MORE_HEADERS_TO_COME;
				}
			}
			return IOggDecoder::AHR_INVALID_HEADER;
			
			
		case VSS_SEEN_BOS:
			if (strncmp((char*)inCodecHeaderPacket->packetData(), "\003vorbis", 7) == 0) {
				if (fish_sound_decode(mFishSound, inCodecHeaderPacket->packetData(), inCodecHeaderPacket->packetSize()) >= 0) {
					mSetupState = VSS_SEEN_COMMENT;
					return IOggDecoder::AHR_MORE_HEADERS_TO_COME;
				}
				
				
			}
			return IOggDecoder::AHR_INVALID_HEADER;
			
			
		case VSS_SEEN_COMMENT:
			if (strncmp((char*)inCodecHeaderPacket->packetData(), "\005vorbis", 7) == 0) {
				if (fish_sound_decode(mFishSound, inCodecHeaderPacket->packetData(), inCodecHeaderPacket->packetSize()) >= 0) {
					mSetupState = VSS_ALL_HEADERS_SEEN;
					return IOggDecoder::AHR_ALL_HEADERS_RECEIVED;
				}
				
			}
			return IOggDecoder::AHR_INVALID_HEADER;
			
		case VSS_ALL_HEADERS_SEEN:
		case VSS_ERROR:
		default:
			return IOggDecoder::AHR_UNEXPECTED;
	}
}
string VorbisDecodeInputPin::getCodecShortName()
{
	return "vorbis";
}
string VorbisDecodeInputPin::getCodecIdentString()
{
	//TODO:::
	return "vorbis";
}