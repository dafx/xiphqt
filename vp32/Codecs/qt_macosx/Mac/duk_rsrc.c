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

/* 
	duk_rsrc.c
	
	Several Routines that access the resoure fork of a file
	using as a sort of ini (Windows) file.
	Displays dialogs and such from resources.
	Duk_MessageBox() defined here.
*/

#include "duk_rsrc.h"

#pragma global_optimizer off

char *PtoCStr(unsigned char *s)
{
    int theLen;
    int t;

    theLen = s[0];

    for(t=0;t<theLen;t++)
        s[t] = s[t+1];
       
    s[theLen] = '\0';

    return (char *) s;
}

unsigned char *CtoPStr(char *s)
{
    int theLen;
    int t;

    theLen = strlen(s);

    for(t=theLen;t>=1;t--)
        s[t] = s[t-1];
       
    s[0] = (char ) theLen;

    return (unsigned char *) s;
}

int DukAddResourceViaPtr(char *thePtr, ResType theType,char *name)
{ 
    Handle h;
    int theID;
    Handle hNamed;
    char *rc;
      
    h = (Handle ) NewHandle((strlen(thePtr)+1) * sizeof(char));
    HLock(h);
    strcpy((char *) *h,thePtr);
    rc = ( char *) CtoPStr((char *) *h);
    HUnlock(h);
      
    theID = 128 + Count1Resources(theType);
    rc = ( char *) CtoPStr(name);
      
    hNamed = Get1NamedResource(theType,(unsigned char *) name);
      
    if (!hNamed) 
    { 
    	AddResource(h,theType,theID,(unsigned char *) name);
    	UpdateResFile(CurResFile());
    }
      
    rc = PtoCStr((unsigned char *) name);
     
    return TRUE;
}




char *DukGetStrResource(char *thePtr, ResType theType,char *name)
{ 
    Handle h;
    int t;
    int theLen;

    CtoPStr(name);
    h = GetNamedResource(theType,(unsigned char *) name);
    thePtr[0] = '\0';
    PtoCStr((unsigned char *) name);

    if (h) 
    { 
        HLock(h);
    	theLen = (unsigned char ) ((*h)[0]);
    	
    	for(t=1;t<=theLen;t++)
    	   strncat(thePtr, (char *)  &((*h)[t]) , 1   );
        
        thePtr[theLen] = '\0';
    	HUnlock(h);
    	ReleaseResource(h);
    	return thePtr;
    }
    else 
        return (char *) 0L;
 
}


unsigned char *DukGetVersionResourceShortString(
	unsigned char *thePtr, 
	ResType theType,char *name)
{ 
    Handle h;
    unsigned long theLen;

    CtoPStr(name);
    h = GetNamedResource(theType,(unsigned char *) name);
    thePtr[0] = '\0';
    PtoCStr((unsigned char *) name);

    if (h) 
    { 
        HLock(h);
    	theLen = ((unsigned char *) *h)[6];
    	BlockMove(&((*h)[7]),thePtr,theLen);
    	HUnlock(h);
    	thePtr[theLen] = '\0';
    	ReleaseResource(h);
    	return thePtr;
    }
    else 
    {
        return (unsigned char *) 0L;
    }
}


unsigned char *DukGetCustomResource(
	unsigned char *thePtr, 
	ResType theType,char *name, unsigned long *sizeData)
{ 
    Handle h;
    unsigned long theLen;

    CtoPStr(name);
    h = GetNamedResource(theType,(unsigned char *) name);
    thePtr[0] = '\0';
    PtoCStr((unsigned char *) name);

    if (h) 
    { 
        HLock(h);
    	theLen = ((unsigned long *) *h)[0];
    	BlockMove(&((*h)[4]),thePtr,theLen);
    	HUnlock(h);
    	*sizeData = theLen;
    	ReleaseResource(h);
    	return thePtr;
    }
    else 
    {
        *sizeData = 0;
        return (unsigned char *) 0L;
    }
}

int DukSetStrResource(char *thePtr, ResType theType,char *name)
{ 
    Handle h;
    int theLen;

    CtoPStr(name);
    h = GetNamedResource(theType,(unsigned char *) name);

    if (!h)
    {
        short id;
        h = NewHandle(strlen(thePtr) + 2);

        HLock(h);
        (*h)[0] = strlen(thePtr);
        strcpy(&((*h)[1]),thePtr);
        HUnlock(h);

        id = UniqueID('STR ');
        AddResource(h,'STR ',id,(unsigned char *) name);
        UpdateResFile(CurResFile());
    }
    else
    { 
        short myErr;
        theLen = strlen(thePtr);
        SetHandleSize(h,theLen + 1);

        HNoPurge(h);
        HLock(h);
        (*h)[0] = theLen;
        strncpy((char *)  &((*h)[1]) , thePtr, theLen);
        HUnlock(h);

        ChangedResource(h);
        WriteResource(h);
        myErr = ResError();
        UpdateResFile(CurResFile());
        HPurge(h);
    }

    ReleaseResource(h);  
    PtoCStr((unsigned char *) name);
    return 0;
}

int DukSetCustomResource(
	unsigned char *data, unsigned long sizeData, 
	ResType theType, char *name)
{ 
	Handle h;
  
	CtoPStr(name);
	h = GetNamedResource(theType,(unsigned char *) name);
	
	if (!h)
	{
		short id;
		h = NewHandle(sizeData + 4);
		
		HLock(h);
		((unsigned long *) *h)[0] = sizeData;
		BlockMoveData(data,&((*h)[4]),sizeData);
		HUnlock(h);
		
		id = UniqueID(theType);
		AddResource(h,theType,id,(unsigned char *) name);
		UpdateResFile(CurResFile());
	}
	else
	{
		short myErr;
        SetHandleSize(h,sizeData + 4);
       
        //HNoPurge(h);
        HLock(h);
        ((unsigned long *) *h)[0] = sizeData;
        BlockMoveData(data,&((*h)[4]),sizeData);
		HUnlock(h);
		
		ChangedResource(h);
		myErr = ResError();
		WriteResource(h);
		//HPurge(h);
	}
  
  
  ReleaseResource(h);
  PtoCStr((unsigned char *) name);

  return 0;
}


int DukSetCustomResourceWithFileData(char *filename,ResType theType,char *resName)
{
FILE *fp;
#define PROFILE_BUFFER_SIZE 1024
unsigned char buffer[PROFILE_BUFFER_SIZE];
unsigned long sizeData;

fp = fopen(filename,"rb");
if (fp) {
	sizeData = fread(buffer,1,PROFILE_BUFFER_SIZE,fp);
	if (sizeData > 0)
		DukSetCustomResource(buffer,sizeData,theType,resName);
}

return 0;
}


unsigned char *DukGetPStrResource(unsigned char *thePtr, ResType theType,char *name)
{ 
  Handle h;
  int t;
  int theLen;

  
  CtoPStr(name);
  h = GetNamedResource(theType,(unsigned char *) name);
  
  PtoCStr((unsigned char *) name);
  
  if (h) { 
        HLock(h);
		theLen = (*h)[0];
		thePtr[0] = theLen;
  		thePtr[1] = '\0';
  		thePtr++;
		for(t=1;t<=theLen;t++)
		   strncat((char *) thePtr, (char *)  &((*h)[t]) , 1   );
		HUnlock(h);
		thePtr = thePtr - 1;
		ReleaseResource(h);
		return thePtr;
		}
  else return (unsigned char *) 0L;
  
 
}


#include <TextEdit.h>
#if 0
int Duk_LongMessageBox(char *msg,char *title)
{ 
  DialogPtr theDialog;
  WindowPtr theWindow;
  CGrafPtr  theG;
  Rect theRect;
  short theItem;
  Handle h;
  Handle h2;
  Rect box;
  short itemType;
  short sx,sy;
  int wheight,wwidth;
  short resID;
  unsigned char resName[256];
  ResType theResType;
  
  Duk_ShowCursor();
    
  h2 = GetNamedResource('DLOG',"\prLongMessageBox");
  GetResInfo(h2,&resID,&theResType,resName);
  
  theDialog = GetNewDialog(resID,0L,0L);
  
  ScreenDimensions(&sx,&sy);
  
  wwidth = theDialog->portRect.right - theDialog->portRect.left;
  //theWindow = GetWindowFromDialog(theDialog);
  //wwidth
  wheight = theDialog->portRect.bottom - theDialog->portRect.top;
  
  SetWTitle(theDialog,CtoPStr(title));
  MoveWindow(theDialog,(sx-wwidth)/2,(sy-wheight)/2,TRUE);
  
  GetDialogItem(theDialog,1,&itemType,&h,&box);
 
  SelectWindow(theDialog);
  ShowWindow(theDialog);
 
 
  SetPort(theDialog);

  TETextBox(msg,strlen(msg),&box,0);
  
  do 
  	ModalDialog(0L,&theItem);
  while (theItem != 2);
  
   
  PtoCStr((unsigned char *) title);
  DisposeDialog(theDialog);
	 
	 
  return TRUE;
}
#endif



#if 0
int Duk_MessageBox(char *msg,char *title)
{ 
	return Duk_LongMessageBox(msg,title);
}
#endif

#if 0
short Duk_OKCancelEditBox(char *msg,char *title, char *editText);
short Duk_OKCancelEditBox(char *msg,char *title, char *editText)
{ DialogPtr theDialog;
  short theItem;
  Handle h;
  Rect box;
  short itemType;
  short sx,sy;
  int wheight,wwidth;
  short  rc;
  short	dialogID;
  Ptr dStorage;
  
  Duk_ShowCursor();
  
  
  /*	Get the dialog's ID */
  h = GetNamedResource('DLOG', "\prOKCancelEditBox");
  GetResInfo(h, &dialogID, NULL, NULL);
  ReleaseResource(h);
	
  dStorage = NewPtrClear(sizeof(DialogRecord));
  theDialog = GetNewDialog(dialogID,0L,0L);
  
  ScreenDimensions(&sx,&sy);
  
  wwidth = theDialog->portRect.right - theDialog->portRect.left;
  wheight = theDialog->portRect.bottom - theDialog->portRect.top;
  
  CtoPStr(title);
  SetWTitle(theDialog,(unsigned char *) title);
  PtoCStr((unsigned char *) title);
  MoveWindow(theDialog,(sx-wwidth)/2,(sy-wheight)/2,TRUE);
  
  GetDialogItem(theDialog,4,&itemType,&h,&box);
  CtoPStr(msg);
  SetDialogItemText(h,(unsigned char *) msg);
  PtoCStr((unsigned char *) msg);
  
  
  SelectWindow(theDialog);
  ShowWindow(theDialog);
  
  do 
  	ModalDialog(0L,&theItem);
  while (theItem != 1 && theItem != 2);
  
  if (theItem == 1) {
  	 	GetDialogItem(theDialog,3,&itemType,&h,&box);
 		GetDialogItemText(h,(unsigned char *) editText);
  		PtoCStr((unsigned char *) editText);
  }
 
 DisposeDialog(theDialog);
 
 
 rc = (theItem != 2);
  
 return rc;
}


int Duk_OKCancelMessageBox(char *msg,char *title)
{ DialogPtr theDialog;
  short theItem;
  Handle h;
  Rect box;
  short itemType;
  char *pStr;
  short sx,sy;
  int wheight,wwidth;
  int rc;
  
  Duk_ShowCursor();
  
  pStr = (char *) calloc( (strlen(msg) + 1),sizeof(char));
  
  strcpy(pStr,msg);
  
  CtoPStr(pStr);
  
  theDialog = GetNewDialog(134,0L,0L);
  
  ScreenDimensions(&sx,&sy);
  
  wwidth = theDialog->portRect.right - theDialog->portRect.left;
  wheight = theDialog->portRect.bottom - theDialog->portRect.top;
  
  
  SetWTitle(theDialog,CtoPStr(title));
  MoveWindow(theDialog,(sx-wwidth)/2,(sy-wheight)/2,TRUE);
  
  
  GetDialogItem(theDialog,GOC_TEXT,&itemType,&h,&box);
  SetDialogItemText(h,(ConstStr255Param) pStr);
  
  SelectWindow(theDialog);
  ShowWindow(theDialog);
  
  do 
  	ModalDialog(0L,&theItem);
  while (theItem != GOC_CANCEL && theItem != GOC_OK);
  
  free((char *) pStr);
  
  DisposeDialog(theDialog);
	 
	
  rc = theItem;
  
  return rc;
}

static char  buf[256];
int Duk_OpenOKCancelMessageBox(char *title, char *msg,DialogPtr *aDialog)
{ DialogPtr theDialog;
  Handle h;
  Rect box;
  short itemType;
  short sx,sy;
  int wheight,wwidth;
  
  Duk_ShowCursor();
  
  
  strcpy(buf,msg);
  
  CtoPStr(buf);
  
  theDialog = GetNewDialog(134,0L,0L);
  
  ScreenDimensions(&sx,&sy);
  
  wwidth = theDialog->portRect.right - theDialog->portRect.left;
  wheight = theDialog->portRect.bottom - theDialog->portRect.top;
  
  SetWTitle(theDialog,CtoPStr(title));
  MoveWindow(theDialog,(sx-wwidth)/2,(sy-wheight)/2,TRUE);
  
  
  GetDialogItem(theDialog,GOC_TEXT,&itemType,&h,&box);
  SetDialogItemText(h,(ConstStr255Param) buf);
  
  *aDialog = theDialog;

  return TRUE;
}





short Duk_GetNamedResourcesID(char *name)
{
Handle h;
Str255 dummy;
ResType rType;
short rID;

CtoPStr(name);

h = GetNamedResource('DLOG',(unsigned char *) name);

GetResInfo(h,&rID,&rType,dummy);

ReleaseResource(h);

PtoCStr((unsigned char *) name);

return rID;

}


int Duk_GetRsrcItemsRect(DialogPtr theDialog, short item, Rect *theRect)
{
Handle h;
short itemType;

GetDialogItem(theDialog,item,&itemType,&h,theRect);

return TRUE;
}
#endif
#pragma global_optimizer on

