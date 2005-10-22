//===========================================================================
//Copyright (C) 2003, 2004, 2005 Zentaro Kavanagh
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
#include "StdAfx.h"
#include "OggDemuxPageSourceFilter.h"


// This template lets the Object factory create us properly and work with COM infrastructure.
CFactoryTemplate g_Templates[] = 
{
    { 
		L"OggDemuxFilter",						// Name
	    &CLSID_OggDemuxPageSourceFilter,            // CLSID
	    OggDemuxPageSourceFilter::CreateInstance,	// Method to create an instance of MyComponent
        NULL,									// Initialization function
        NULL									// Set-up information (for filters)
    }
	
	//,

	//{ 
	//	L"illiminable About Page",				// Name
	//    &CLSID_PropsAbout,						// CLSID
	//    PropsAbout::CreateInstance,				// Method to create an instance of MyComponent
 //       NULL,									// Initialization function
 //       NULL									// Set-up information (for filters)
 //   }

};

// Generic way of determining the number of items in the template
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]); 

//COM Creator Function
CUnknown* WINAPI OggDemuxPageSourceFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT *pHr) 
{
	OggDemuxPageSourceFilter *pNewObject = new OggDemuxPageSourceFilter();
    if (pNewObject == NULL) {
        *pHr = E_OUTOFMEMORY;
    }
    return pNewObject;
} 
//COM Interface query function
STDMETHODIMP OggDemuxPageSourceFilter::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	if (riid == IID_IFileSourceFilter) {
		*ppv = (IFileSourceFilter*)this;
		((IUnknown*)*ppv)->AddRef();
		return NOERROR;
	/*} else if (riid == IID_IMediaSeeking) {
		*ppv = (IMediaSeeking*)this;
		((IUnknown*)*ppv)->AddRef();
		return NOERROR;*/
	}/* else if (riid == IID_ISpecifyPropertyPages) {
		*ppv = (ISpecifyPropertyPages*)this;
		((IUnknown*)*ppv)->AddRef();
		return NOERROR;
	}*/  else if (riid == IID_IAMFilterMiscFlags) {
		*ppv = (IAMFilterMiscFlags*)this;
		((IUnknown*)*ppv)->AddRef();
		return NOERROR;
	//} else if (riid == IID_IAMMediaContent) {
	//	//debugLog<<"Queries for IAMMediaContent///"<<endl;
	//	*ppv = (IAMMediaContent*)this;
	//	((IUnknown*)*ppv)->AddRef();
	//	return NOERROR;
	}

	return CBaseFilter::NonDelegatingQueryInterface(riid, ppv); 
}
OggDemuxPageSourceFilter::OggDemuxPageSourceFilter(void)
	:	CBaseFilter(NAME("OggDemuxPageSourceFilter"), NULL, m_pLock, CLSID_OggDemuxPageSourceFilter)
{
}

OggDemuxPageSourceFilter::~OggDemuxPageSourceFilter(void)
{
}

int OggDemuxPageSourceFilter::GetPinCount() 
{
	//TODO::: Implement
	return 0;//mStreamMapper->numStreams();
}
CBasePin* OggDemuxPageSourceFilter::GetPin(int inPinNo) 
{
	//TODO::: IMplement
	return NULL;
}

//IFileSource Interface
STDMETHODIMP OggDemuxPageSourceFilter::GetCurFile(LPOLESTR* outFileName, AM_MEDIA_TYPE* outMediaType) 
{
	////Return the filename and mediatype of the raw data
	//LPOLESTR x = SysAllocString(mFileName.c_str());
	//*outFileName = x;

	//TODO:::
	
	return S_OK;
}


STDMETHODIMP OggDemuxPageSourceFilter::Load(LPCOLESTR inFileName, const AM_MEDIA_TYPE* inMediaType) 
{
	////Initialise the file here and setup all the streams
	//CAutoLock locLock(m_pLock);
	//mFileName = inFileName;

	//debugLog<<"Loading : "<<StringHelper::toNarrowStr(mFileName)<<endl;

	//debugLog << "Opening source file : "<<StringHelper::toNarrowStr(mFileName)<<endl;
	//mSeekTable = new AutoOggSeekTable(StringHelper::toNarrowStr(mFileName));
	//mSeekTable->buildTable();
	//
	//return SetUpPins();

	//TODO:::
	return S_OK;
}

//IAMFilterMiscFlags Interface
ULONG OggDemuxPageSourceFilter::GetMiscFlags(void) 
{
	return AM_FILTER_MISC_FLAGS_IS_SOURCE;
}

//CAMThread Stuff
DWORD OggDemuxPageSourceFilter::ThreadProc(void) {
	
	//while(true) {
	//	DWORD locThreadCommand = GetRequest();
	//
	//	switch(locThreadCommand) {
	//		case THREAD_EXIT:
	//
	//			Reply(S_OK);
	//			return S_OK;

	//		case THREAD_RUN:
	//
	//			Reply(S_OK);
	//			DataProcessLoop();
	//			break;
	//	}
	//}
	return S_OK;
}




STDMETHODIMP OggDemuxPageSourceFilter::GetCapabilities(DWORD* inCapabilities) 
{
	//if (mSeekTable->enabled())  {
	//	//debugLog<<"GetCaps "<<mSeekingCap<<endl;
	//	*inCapabilities = mSeekingCap;
	//	return S_OK;
	//} else {
	//	//debugLog<<"Get Caps failed !!!!!!!"<<endl;
	//	*inCapabilities = 0;
	//	return S_OK;;
	//}


	//TODO:::
	return E_NOTIMPL;
}
STDMETHODIMP OggDemuxPageSourceFilter::GetDuration(LONGLONG* outDuration) 
{
	//if (mSeekTable->enabled())  {
	//	//debugLog<<"GetDuration = " << mSeekTable->fileDuration()<<" ds units"<<endl;
	//	*outDuration = mSeekTable->fileDuration();
	//	return S_OK;
	//} else {
	//	return E_NOTIMPL;
	//}

	//TODO:::
	return E_NOTIMPL;

}
	 
STDMETHODIMP OggDemuxPageSourceFilter::CheckCapabilities(DWORD *pCapabilities)
{
	//debugLog<<"CheckCaps	: Not impl"<<endl;

	//TODO:::
	return E_NOTIMPL;
}
STDMETHODIMP OggDemuxPageSourceFilter::IsFormatSupported(const GUID *pFormat)
{
	//ASSERT(pFormat != NULL);
	//if (*pFormat == TIME_FORMAT_MEDIA_TIME) {
	//	//debugLog<<"IsFormatSupported	: TRUE"<<endl;
	//	return S_OK;
	//} else {
	//	//debugLog<<"IsFormatSupported	: FALSE !!!"<<endl;
	//	return S_FALSE;
	//}

	//TODO:::
	return E_NOTIMPL;

	
}
STDMETHODIMP OggDemuxPageSourceFilter::QueryPreferredFormat(GUID *pFormat){
	//debugLog<<"QueryPrefferedTimeFormat	: MEDIA TIME"<<endl;
	*pFormat = TIME_FORMAT_MEDIA_TIME;
	return S_OK;
}
STDMETHODIMP OggDemuxPageSourceFilter::SetTimeFormat(const GUID *pFormat){
	//debugLog<<"SetTimeForamt : NOT IMPL"<<endl;
	return E_NOTIMPL;
}
STDMETHODIMP OggDemuxPageSourceFilter::GetTimeFormat( GUID *pFormat){
	*pFormat = TIME_FORMAT_MEDIA_TIME;
	return S_OK;
}
STDMETHODIMP OggDemuxPageSourceFilter::GetStopPosition(LONGLONG *pStop){
	//if (mSeekTable->enabled())  {

	//	//debugLog<<"GetStopPos = " << mSeekTable->fileDuration()<<" ds units"<<endl;
	//	*pStop = mSeekTable->fileDuration();
	//	return S_OK;
	//} else {
	//	//debugLog<<"GetStopPos NOT IMPL"<<endl;
	//	return E_NOTIMPL;
	//}

	//TODO:::
	return E_NOTIMPL;

}
STDMETHODIMP OggDemuxPageSourceFilter::GetCurrentPosition(LONGLONG *pCurrent)
{
	//TODO::: Implement this properly

	//debugLog<<"GetCurrentPos = NOT_IMPL"<<endl;
	return E_NOTIMPL;
}
STDMETHODIMP OggDemuxPageSourceFilter::ConvertTimeFormat(LONGLONG *pTarget, const GUID *pTargetFormat, LONGLONG Source, const GUID *pSourceFormat){
	//debugLog<<"ConvertTimeForamt : NOT IMPL"<<endl;
	return E_NOTIMPL;
}
STDMETHODIMP OggDemuxPageSourceFilter::SetPositions(LONGLONG *pCurrent,DWORD dwCurrentFlags,LONGLONG *pStop,DWORD dwStopFlags){


	//CAutoLock locLock(m_pLock);
	////debugLog<<"Set Positions "<<*pCurrent<<" to "<<*pStop<<" with flags "<<dwCurrentFlags<<" and "<<dwStopFlags<<endl;
	//if (mSeekTable->enabled())  {
	//	//debugLog<<"SetPos : Current = "<<*pCurrent<<" Flags = "<<dwCurrentFlags<<" Stop = "<<*pStop<<" dwStopFlags = "<<dwStopFlags<<endl;
	//	//debugLog<<"       : Delivering begin flush..."<<endl;

	//
	//	CAutoLock locSourceLock(mSourceFileLock);
	//	mSetIgnorePackets = false;
	//	DeliverBeginFlush();
	//	//debugLog<<"       : Begin flush Delviered."<<endl;

	//	//Find the byte position for this time.
	//	OggSeekTable::tSeekPair locStartPos = mSeekTable->getStartPos(*pCurrent);
	//	bool locSendExcess = false;

	//	//FIX::: This code needs to be removed, and handle start seek case.
	//	//.second is the file position.
	//	//.first is the time in DS units
	//	if (locStartPos.second == mStreamMapper->startOfData()) {
	//		locSendExcess = true;
	//		//GGFF:::
	//		//mStreamMapper->toStartOfData();
	//		mSetIgnorePackets = true;
	//	}
	//	
	//	
	//	//We have to save this here now... since time can't be reverted to granule pos in all cases
	//	// we have to use granule pos timestamps in order for downstream codecs to work.
	//	// Because of this we can't factor time bases after seeking into the sample times.
	//	*pCurrent	= mSeekTimeBase 
	//				= locStartPos.first;		//Time from seek pair.

	//	//debugLog<<"Corrected pCurrent : "<<mSeekTimeBase<<endl;
	//	for (unsigned long i = 0; i < mStreamMapper->numStreams(); i++) {
	//		mStreamMapper->getOggStream(i)->setSendExcess(locSendExcess);		//Not needed
	//		mStreamMapper->getOggStream(i)->setLastEndGranPos(*pCurrent);
	//	}
	//	{
	//		//debugLog<<"       : Delivering End Flush..."<<endl;
	//		DeliverEndFlush();
	//		//debugLog<<"       : End flush Delviered."<<endl;
	//		DeliverNewSegment(*pCurrent, mSeekTable->fileDuration(), 1.0);
	//	}

	//	//.second is the file position.
	//	mDataSource->seek(locStartPos.second);
	//
	//	//debugLog<<"       : Seek complete."<<endl;
	//} else {
	//	//debugLog<<"Seek not IMPL"<<endl;
	//	return E_NOTIMPL;
	//}

	//return S_OK;

	//TODO:::
	return E_NOTIMPL;

}
STDMETHODIMP OggDemuxPageSourceFilter::GetPositions(LONGLONG *pCurrent, LONGLONG *pStop)
{
	//debugLog<<"Getpos : Not IMPL"<<endl;
	//debugLog<<"GetPos : Current = HARDCODED 2 secs , Stop = "<<mSeekTable->fileDuration()/UNITS <<" secs."<<endl;
	return E_NOTIMPL;
}
STDMETHODIMP OggDemuxPageSourceFilter::GetAvailable(LONGLONG *pEarliest, LONGLONG *pLatest){
	//debugLog<<"****GetAvailable : NOT IMPL"<<endl;
	//if (mSeekTable->enabled())  {
	//	//debugLog<<"Get Avail ok"<<endl;
	//	*pEarliest = 0;
	//	//debugLog<<"+++++ Duration is "<<mSeekTable->fileDuration()<<endl;
	//	*pLatest = mSeekTable->fileDuration();
	//	return S_OK;
	//} else {
	//	return E_NOTIMPL;
	//}

	//TODO:::
	return E_NOTIMPL;

}
STDMETHODIMP OggDemuxPageSourceFilter::SetRate(double dRate)
{
	//debugLog<<"Set RATE : NOT IMPL"<<endl;
	return E_NOTIMPL;
}
STDMETHODIMP OggDemuxPageSourceFilter::GetRate(double *dRate)
{

	*dRate = 1.0;
	return S_OK;;
}
STDMETHODIMP OggDemuxPageSourceFilter::GetPreroll(LONGLONG *pllPreroll)
{

	*pllPreroll = 0;
	//debugLog<<"GetPreroll : HARD CODED TO 0"<<endl;
	return S_OK;
}
STDMETHODIMP OggDemuxPageSourceFilter::IsUsingTimeFormat(const GUID *pFormat){
	//if (*pFormat == TIME_FORMAT_MEDIA_TIME) {
	//	//debugLog<<"IsUsingTimeFormat : MEDIA TIME TRUE"<<endl;
	//	return S_OK;
	//} else {
	//	//debugLog<<"IsUsingTimeFormat : MEDIA TIME FALSE !!!!"<<endl;
	//	return S_FALSE;
	//}

	//TODO:::
	return E_NOTIMPL;

}

