#ifndef _duk_rsrc_h
#define _duk_rsrc_h 1

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <Resources.h>
#include <Memory.h>
#include <Dialogs.h>
#include <Lists.h>

#ifdef __cplusplus
extern "C" {
#endif

char *PtoCStr(unsigned char *s);

unsigned char *CtoPStr(char *s);

void SetDukCurResFile(short i);

short GetDukCurResFile(void);

int DukAddResourceViaPtr(char *thePtr, ResType theType,char *name);

char *DukGetStrResource(char *thePtr, ResType theType,char *name);

unsigned char *DukGetVersionResourceShortString(unsigned char *thePtr, ResType theType,char *name);
	
unsigned char *DukGetCustomResource(unsigned char *thePtr, ResType theType,char *name, unsigned long *sizeData);

int DukSetStrResource(char *thePtr, ResType theType,char *name);

int DukSetCustomResource(unsigned char *data, unsigned long sizeData, ResType theType, char *name);

int DukSetCustomResourceWithFileData(char *filename,ResType theType,char *resName);

unsigned char *DukGetPStrResource(unsigned char *thePtr, ResType theType,char *name);

int Duk_LongMessageBox(char *msg,char *title);

int Duk_MessageBox(char *msg, char *title);

int Duk_OKCancelMessageBox(char *msg,char *title);

int Duk_OpenOKCancelMessageBox(char *title, char *msg,DialogPtr *aDialog);

short Duk_GetNamedResourcesID(char *name);

int Duk_GetRsrcItemsRect(DialogPtr theDialog, short item, Rect *theRect);

enum SAVE_GAME_DLG_ITEM { SG_TEDIT=1, SG_OK=3, SG_CANCEL=4 };

#ifdef __cplusplus
} /* extern C */
#endif


#define GOC_TEXT 1
#define GOC_OK 3
#define GOC_CANCEL 2

#define TREK_TITLE "Star Trek Klingons"


#endif
