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

#pragma once
//#include <sstream>
#include "CircularBuffer.h"
#include "OggPage.h"
#include "SerialNoRego.h"
#include "IOggCallback.h"
#include "IFIFOBuffer.h"

//Only needed for debugging
//#include <fstream>
//
using namespace std;

class LIBOOOGG_API OggDataBuffer {
public:
	//Public Constants and enums
	enum eState {
		EOS,
		AWAITING_BASE_HEADER = 32,
		AWAITING_SEG_TABLE,
		AWAITING_DATA,
		LOST_PAGE_SYNC
	};

	enum eFeedResult {
		FEED_OK,
		FEED_NULL_POINTER = 64,
		FEED_BUFFER_WRITE_ERROR
	};

	enum eDispatchResult {
		DISPATCH_OK,
		DISPATCH_NO_CALLBACK = 256,
		DISPATCH_FALSE
	};

	enum eProcessResult {
		PROCESS_OK,
		PROCESS_UNKNOWN_INTERNAL_ERROR = 512,
		PROCESS_STREAM_READ_ERROR,
		PROCESS_DISPATCH_FAILED,
		PROCESS_FAILED_TO_SET_HEADER,
		PROCESS_LOST_SYNC = 4096

	};

	static const int MAX_OGG_PAGE_SIZE =	OggPageHeader::OGG_BASE_HEADER_SIZE +											//Header
											(OggPageHeader::MAX_NUM_SEGMENTS * OggPageHeader::MAX_SEGMENT_SIZE) +		//Page Data
											(OggPageHeader::SEGMENT_WIDTH * OggPageHeader::MAX_NUM_SEGMENTS);			//Segment table

	//Debug only
	OggDataBuffer::OggDataBuffer(bool x);
	void debugWrite(string inString);
	//

	//Constructors
	OggDataBuffer(void);
	~OggDataBuffer(void);

	//Setting callbacks
	bool registerStaticCallback(fPageCallback inPageCallback);
	bool registerVirtualCallback(IOggCallback* inPageCallback);
	
	//Buffer Control
	eFeedResult feed(const unsigned char* inData, unsigned long inNumBytes);
	void clearData();
	
	//Buffer state
	unsigned long numBytesAvail();
	eState state();

protected:
	//FIFO Buffer
	IFIFOBuffer* mBuffer;
	
	//Buffer State
	unsigned long mNumBytesNeeded;
	eState mState;	
	OggPage* pendingPage;
	__int64 mPrevGranPos;

	//Callback pointers
	IOggCallback* mVirtualCallback;
	fPageCallback mStaticCallback;

	//Internal processing
	eProcessResult processBuffer();
	eProcessResult processBaseHeader();
	eProcessResult processSegTable();
	eProcessResult processDataSegment();

	//Internal dispatch
	virtual eDispatchResult dispatch(OggPage* inOggPage);

	//DEBUG
	//fstream debugLog;
	//
};
