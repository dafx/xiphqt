#include "StdAfx.h"
#include ".\cmml_parser.h"
#using <mscorlib.dll>


namespace illiminable {
namespace libCMMLParserDotNET {
CMML_Parser::CMML_Parser(void)
{
	mCMMLParser = new CMMLParser();
}

CMML_Parser::~CMML_Parser(void)
{
	delete mCMMLParser;
	mCMMLParser = NULL;
}

bool CMML_Parser::parseDocFromBuffer(String* inBuffer, CMMLDoc* outCMMLDoc, CMMLError* outCMMLError) 
{
	wchar_t* locWS = Wrappers::netStrToWStr(inBuffer);
	wstring locBuffer = locWS;

	bool retVal = mCMMLParser->parseDocFromBuffer(locBuffer, outCMMLDoc->getMe(), outCMMLError->getMe());
	
	Wrappers::releaseWStr(locWS);
	
	return retVal;
}

bool CMML_Parser::parseDocFromFile(String* inFileName, CMMLDoc* outCMMLDoc) 
{
	wchar_t* locWS = Wrappers::netStrToWStr(inFileName);
	wstring locFileName = locWS;

	bool retVal = mCMMLParser->parseDocFromFile(locFileName, outCMMLDoc->getMe());
	
	Wrappers::releaseWStr(locWS);
	
	return retVal;
}

}	//End libCMMLParserDotNNET
}	//End illiminable