# Microsoft Developer Studio Project File - Name="theorabase_static" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=theorabase_static - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "theorabase_static.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "theorabase_static.mak" CFG="theorabase_static - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "theorabase_static - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "theorabase_static - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "theorabase_static - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_theorabase_static"
# PROP Intermediate_Dir "Release_theorabase_static"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /Ob2 /I "../../../../../trunk/ogg/include" /I "../../include" /D "NDEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /U "OC_DUMP_IMAGES" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "theorabase_static - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug_theorabase_static"
# PROP Intermediate_Dir "Debug_theorabase_static"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W4 /Gm /GX /ZI /Od /I "../../../../../trunk/ogg/include" /I "../../include" /D "_DEBUG" /D "WIN32" /D "_MBCS" /D "_LIB" /U "OC_DUMP_IMAGES" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"Debug_theorabase_static\theorabase_static_d.lib"

!ENDIF 

# Begin Target

# Name "theorabase_static - Win32 Release"
# Name "theorabase_static - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\lib\fragment.c
# End Source File
# Begin Source File

SOURCE=..\..\lib\idct.c
# End Source File
# Begin Source File

SOURCE=..\..\lib\info.c
# End Source File
# Begin Source File

SOURCE=..\..\lib\internal.c
# End Source File
# Begin Source File

SOURCE=..\..\lib\quant.c
# End Source File
# Begin Source File

SOURCE=..\..\lib\state.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\include\theora\codec.h
# End Source File
# Begin Source File

SOURCE=..\..\lib\dct.h
# End Source File
# Begin Source File

SOURCE=..\..\lib\huffman.h
# End Source File
# Begin Source File

SOURCE=..\..\lib\idct.h
# End Source File
# Begin Source File

SOURCE=..\..\lib\internal.h
# End Source File
# Begin Source File

SOURCE=..\..\lib\ocintrin.h
# End Source File
# Begin Source File

SOURCE=..\..\lib\quant.h
# End Source File
# Begin Source File

SOURCE=..\..\include\theora\theora.h
# End Source File
# End Group
# End Target
# End Project
