/*
   Copyright (C) 2003 Commonwealth Scientific and Industrial Research
   Organisation (CSIRO) Australia

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   - Neither the name of CSIRO Australia nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
   PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE ORGANISATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/ 

#include "StdAfx.h"
#include ".\anchortag.h"


namespace CSIRO {
namespace libCMMLTagsDotNET {

	AnchorTag::AnchorTag(void)
	{
		mBaseClass = new C_AnchorTag;
	}

	AnchorTag::~AnchorTag(void)
	{
		delete mBaseClass;
	}

	AnchorTag::AnchorTag(C_AnchorTag* inTag) {
		mBaseClass = inTag;
	}

	C_AnchorTag* AnchorTag::getMe() {
		return (C_AnchorTag*)mBaseClass;
	}	


	//Accessors
	String* AnchorTag::cls() {
		return Wrappers::CStrToNetStr( getMe()->cls().c_str() );

	}
	String* AnchorTag::href() {
		return Wrappers::CStrToNetStr( getMe()->href().c_str() );
	}

	//Mutators
	void AnchorTag::setCls(String* inCls) {
		char* tc = Wrappers::netStrToCStr( inCls );
		getMe()->setCls( tc );
		Wrappers::releaseCStr( tc );
	}
	void AnchorTag::setHref(String* inHref) {
		char* tc = Wrappers::netStrToCStr( inHref );
		getMe()->setHref( tc );
		Wrappers::releaseCStr( tc );

	}

	//Other
	String* AnchorTag::toString() {
		return Wrappers::CStrToNetStr( getMe()->toString().c_str() );

	}

}
}
