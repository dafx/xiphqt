//#include <Files.h>
//#include <Folders.h>
//#include <LowMem.h>
//#include <string.h>
/*
folderType -- same as FindFile
send 0 for SystemFolder

path -- return value is full path to folder
*/
#ifndef getsysfolder_h
#define getsysfolder_h

#ifdef __cplusplus
extern "C" {
#endif

short GetSystemFolderPath(OSType folderType, char *path);

#ifdef __cplusplus
}
#endif

#endif
