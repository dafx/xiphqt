@echo off
echo ---+++--- Making Win32 SDK ---+++---

if .%SRCROOT%==. set SRCROOT=i:\xiph

rem --- prepare directory

rd /s /q sdk > nul
md sdk
md sdk\include
md sdk\include\ogg
md sdk\include\vorbis
md sdk\lib
md sdk\bin
md sdk\doc
md sdk\doc\ogg
md sdk\doc\ogg\ogg
md sdk\doc\vorbis
md sdk\doc\vorbis\vorbisenc
md sdk\doc\vorbis\vorbisfile
md sdk\examples
md sdk\examples\vorbis

rem --- is ogg here?

echo Searching for ogg...

if exist %SRCROOT%\ogg\include\ogg\ogg.h goto OGGFOUND

echo ... ogg not found.

goto ERROR

:OGGFOUND

echo ... ogg found.

rem --- is vorbis here?

echo Searching for vorbis...

if exist %SRCROOT%\vorbis\include\vorbis\codec.h goto VORBISFOUND

echo ... vorbis not found.

goto ERROR

:VORBISFOUND

echo ... vorbis found.

rem --- copy include files into sdk

echo Copying include files...

xcopy %SRCROOT%\ogg\include\ogg\*.h %SRCROOT%\win32sdk\sdk\include\ogg > nul
xcopy %SRCROOT%\vorbis\include\vorbis\*.h %SRCROOT%\win32sdk\sdk\include\vorbis > nul

echo ... copied.

rem --- copy docs into sdk

echo Copying docs...

xcopy %SRCROOT%\ogg\doc\*.html %SRCROOT%\win32sdk\sdk\doc\ogg > nul
xcopy %SRCROOT%\ogg\doc\*.png %SRCROOT%\win32sdk\sdk\doc\ogg > nul
xcopy %SRCROOT%\ogg\doc\ogg\*.html %SRCROOT%\win32sdk\sdk\doc\ogg\ogg > nul
xcopy %SRCROOT%\ogg\doc\ogg\*.css %SRCROOT%\win32sdk\sdk\doc\ogg\ogg > nul
xcopy %SRCROOT%\vorbis\doc\*.html %SRCROOT%\win32sdk\sdk\doc\vorbis > nul
xcopy %SRCROOT%\vorbis\doc\*.txt %SRCROOT%\win32sdk\sdk\doc\vorbis > nul
xcopy %SRCROOT%\vorbis\doc\*.png %SRCROOT%\win32sdk\sdk\doc\vorbis > nul
xcopy %SRCROOT%\vorbis\doc\vorbisenc\*.html %SRCROOT%\win32sdk\sdk\doc\vorbis\vorbisenc > nul
xcopy %SRCROOT%\vorbis\doc\vorbisenc\*.css %SRCROOT%\win32sdk\sdk\doc\vorbis\vorbisenc > nul
xcopy %SRCROOT%\vorbis\doc\vorbisfile\*.html %SRCROOT%\win32sdk\sdk\doc\vorbis\vorbisfile > nul
xcopy %SRCROOT%\vorbis\doc\vorbisfile\*.css %SRCROOT%\win32sdk\sdk\doc\vorbis\vorbisfile > nul

echo ... copied.

rem --- copy examples into sdk

echo Copying examples...

xcopy %SRCROOT%\vorbis\examples\*.c %SRCROOT%\win32sdk\sdk\examples\vorbis

echo ... copied.

rem --- build and copy ogg

echo Building ogg...
cd %SRCROOT%\ogg\win32
call build_ogg_static.bat > nul
echo ... static release built ...
xcopy %SRCROOT%\ogg\win32\Static_Release\ogg_static.lib %SRCROOT%\win32sdk\sdk\lib > nul
if errorlevel 1 goto ERROR
echo ... static release copied ...
call build_ogg_static_debug.bat > nul
echo ... static debug built ...
xcopy %SRCROOT%\ogg\win32\Static_Debug\ogg_static_d.lib %SRCROOT%\win32sdk\sdk\lib > nul
if errorlevel 1 goto ERROR
echo ... static debug copied ...
call build_ogg_dynamic.bat > nul
echo ... dynamic release built ...
xcopy %SRCROOT%\ogg\win32\Dynamic_Release\ogg.lib %SRCROOT%\win32sdk\sdk\lib > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\ogg\win32\Dynamic_Release\ogg.dll %SRCROOT%\win32sdk\sdk\bin > nul
if errorlevel 1 goto ERROR
echo ... dynamic release copied ...
call build_ogg_dynamic_debug.bat > nul
echo ... dynamic debug built ...
xcopy %SRCROOT%\ogg\win32\Dynamic_Debug\ogg_d.lib %SRCROOT%\win32sdk\sdk\lib > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\ogg\win32\Dynamic_Debug\ogg_d.pdb %SRCROOT%\win32sdk\sdk\lib > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\ogg\win32\Dynamic_Debug\ogg_d.dll %SRCROOT%\win32sdk\sdk\bin > nul
if errorlevel 1 goto ERROR
echo ... dynamic debug copied ...
echo ... ogg building done.

rem --- build and copy vorbis

echo Building vorbis...
cd %SRCROOT%\vorbis\win32
call build_vorbis_static.bat > nul
echo ... static release built ...
xcopy %SRCROOT%\vorbis\win32\Vorbis_Static_Release\vorbis_static.lib %SRCROOT%\win32sdk\sdk\lib > nul
if errorlevel 1 goto ERROR
echo ... static release copied ...
call build_vorbis_static_debug.bat > nul
echo ... static debug built ...
xcopy %SRCROOT%\vorbis\win32\Vorbis_Static_Debug\vorbis_static_d.lib %SRCROOT%\win32sdk\sdk\lib > nul
if errorlevel 1 goto ERROR
echo ... static debug copied ...
call build_vorbis_dynamic.bat > nul
echo ... dynamic release built ...
xcopy %SRCROOT%\vorbis\win32\Vorbis_Dynamic_Release\vorbis.lib %SRCROOT%\win32sdk\sdk\lib > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\vorbis\win32\Vorbis_Dynamic_Release\vorbis.dll %SRCROOT%\win32sdk\sdk\bin > nul
if errorlevel 1 goto ERROR
echo ... dynamic release copied ...
call build_vorbis_dynamic_debug.bat > nul
echo ... dynamic debug built ...
xcopy %SRCROOT%\vorbis\win32\Vorbis_Dynamic_Debug\vorbis_d.lib %SRCROOT%\win32sdk\sdk\lib > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\vorbis\win32\Vorbis_Dynamic_Debug\vorbis_d.pdb %SRCROOT%\win32sdk\sdk\lib > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\vorbis\win32\Vorbis_Dynamic_Debug\vorbis_d.dll %SRCROOT%\win32sdk\sdk\bin > nul
if errorlevel 1 goto ERROR
echo ... dynamic debug copied ...
echo ... vorbis building done.

rem --- build and copy vorbisfile

echo Building vorbisfile...
cd %SRCROOT%\vorbis\win32
call build_vorbisfile_static.bat > nul
echo ... static release built ...
xcopy %SRCROOT%\vorbis\win32\VorbisFile_Static_Release\vorbisfile_static.lib %SRCROOT%\win32sdk\sdk\lib > nul
if errorlevel 1 goto ERROR
echo ... static release copied ...
call build_vorbisfile_static_debug.bat > nul
echo ... static debug built ...
xcopy %SRCROOT%\vorbis\win32\VorbisFile_Static_Debug\vorbisfile_static_d.lib %SRCROOT%\win32sdk\sdk\lib > nul
if errorlevel 1 goto ERROR
echo ... static debug copied ...
call build_vorbisfile_dynamic.bat > nul
echo ... dynamic release built ...
xcopy %SRCROOT%\vorbis\win32\VorbisFile_Dynamic_Release\vorbisfile.lib %SRCROOT%\win32sdk\sdk\lib > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\vorbis\win32\VorbisFile_Dynamic_Release\vorbisfile.dll %SRCROOT%\win32sdk\sdk\bin > nul
if errorlevel 1 goto ERROR
echo ... dynamic release copied ...
call build_vorbisfile_dynamic_debug.bat > nul
echo ... dynamic debug built ...
xcopy %SRCROOT%\vorbis\win32\VorbisFile_Dynamic_Debug\vorbisfile_d.lib %SRCROOT%\win32sdk\sdk\lib > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\vorbis\win32\VorbisFile_Dynamic_Debug\vorbisfile_d.pdb %SRCROOT%\win32sdk\sdk\lib > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\vorbis\win32\VorbisFile_Dynamic_Debug\vorbisfile_d.dll %SRCROOT%\win32sdk\sdk\bin > nul
if errorlevel 1 goto ERROR
echo ... dynamic debug copied ...
echo ... vorbisfile building done.

rem --- build and copy vorbisenc

echo Building vorbisenc...
cd %SRCROOT%\vorbis\win32
call build_vorbisenc_static.bat > nul
echo ... static release built ...
xcopy %SRCROOT%\vorbis\win32\VorbisEnc_Static_Release\vorbisenc_static.lib %SRCROOT%\win32sdk\sdk\lib > nul
if errorlevel 1 goto ERROR
echo ... static release copied ...
call build_vorbisenc_static_debug.bat > nul
echo ... static debug built ...
xcopy %SRCROOT%\vorbis\win32\VorbisEnc_Static_Debug\vorbisenc_static_d.lib %SRCROOT%\win32sdk\sdk\lib > nul
if errorlevel 1 goto ERROR
echo ... static debug copied ...
call build_vorbisenc_dynamic.bat > nul
echo ... dynamic release built ...
xcopy %SRCROOT%\vorbis\win32\VorbisEnc_Dynamic_Release\vorbisenc.lib %SRCROOT%\win32sdk\sdk\lib > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\vorbis\win32\VorbisEnc_Dynamic_Release\vorbisenc.dll %SRCROOT%\win32sdk\sdk\bin > nul
if errorlevel 1 goto ERROR
echo ... dynamic release copied ...
call build_vorbisenc_dynamic_debug.bat > nul
echo ... dynamic debug built ...
xcopy %SRCROOT%\vorbis\win32\VorbisEnc_Dynamic_Debug\vorbisenc_d.lib %SRCROOT%\win32sdk\sdk\lib > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\vorbis\win32\VorbisEnc_Dynamic_Debug\vorbisenc_d.pdb %SRCROOT%\win32sdk\sdk\lib > nul
if errorlevel 1 goto ERROR
xcopy %SRCROOT%\vorbis\win32\VorbisEnc_Dynamic_Debug\vorbisenc_d.dll %SRCROOT%\win32sdk\sdk\bin > nul
if errorlevel 1 goto ERROR
echo ... dynamic debug copied ...
echo ... vorbisenc building done.

rem -- finished

goto DONE
:ERROR

cd %SRCROOT%\win32sdk
rd /s /q sdk > nul
echo.
echo Some error(s) occurred. Fix it.
goto EXIT

:DONE
cd %SRCROOT%\win32sdk
echo All done.

:EXIT
