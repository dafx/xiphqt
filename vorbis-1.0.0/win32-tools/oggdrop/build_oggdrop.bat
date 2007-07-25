@echo off
echo ---+++--- Building Oggdrop ---+++---

if .%SRCROOT%==. set SRCROOT=c:\src

set OLDPATH=%PATH%
set OLDINCLUDE=%INCLUDE%
set OLDLIB=%LIB%

call "c:\program files\microsoft visual studio\vc98\bin\vcvars32.bat"
echo Setting include/lib paths for Vorbis
set INCLUDE=%INCLUDE%;%SRCROOT%\ogg\include;%SRCROOT%\vorbis\include
set LIB=%LIB%;%SRCROOT%\ogg\win32\Static_Release;%SRCROOT%\vorbis\win32\Vorbis_Static_Release;%SRCROOT%\vorbis\win32\Vorbisenc_Static_Release
echo Compiling...
msdev oggdrop.dsp /useenv /make "oggdrop - Win32 Release" /rebuild

set PATH=%OLDPATH%
set INCLUDE=%OLDINCLUDE%
set LIB=%OLDLIB%
