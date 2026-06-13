# Microsoft Developer Studio Project File - Name="np2c" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** 編集しないでください **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=np2c - Win32 Debug
!MESSAGE これは有効なﾒｲｸﾌｧｲﾙではありません。 このﾌﾟﾛｼﾞｪｸﾄをﾋﾞﾙﾄﾞするためには NMAKE を使用してください。
!MESSAGE [ﾒｲｸﾌｧｲﾙのｴｸｽﾎﾟｰﾄ] ｺﾏﾝﾄﾞを使用して実行してください
!MESSAGE 
!MESSAGE NMAKE /f "np2c.mak".
!MESSAGE 
!MESSAGE NMAKE の実行時に構成を指定できます
!MESSAGE ｺﾏﾝﾄﾞ ﾗｲﾝ上でﾏｸﾛの設定を定義します。例:
!MESSAGE 
!MESSAGE NMAKE /f "np2c.mak" CFG="np2c - Win32 Debug"
!MESSAGE 
!MESSAGE 選択可能なﾋﾞﾙﾄﾞ ﾓｰﾄﾞ:
!MESSAGE 
!MESSAGE "np2c - Win32 Release" ("Win32 (x86) Application" 用)
!MESSAGE "np2c - Win32 Debug" ("Win32 (x86) Application" 用)
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "np2c - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\bin"
# PROP Intermediate_Dir "..\obj\crel"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I ".\\" /I ".\x86" /I ".\dialog" /I "..\\" /I "..\common" /I "..\i286c" /I "..\io" /I "..\cbus" /I "..\bios" /I "..\vram" /I "..\sound" /I "..\sound\vermouth" /I "..\sound\getsnd" /I "..\fdd" /I "..\lio" /I "..\font" /I "..\generic" /D "NDEBUG" /D "TRACE" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ddraw.lib dxguid.lib DSOUND.LIB winmm.lib comctl32.lib wsock32.lib /nologo /subsystem:windows /map /machine:I386

!ELSEIF  "$(CFG)" == "np2c - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\bin"
# PROP Intermediate_Dir "..\obj\cdbg"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I ".\\" /I ".\x86" /I ".\dialog" /I "..\\" /I "..\common" /I "..\i286c" /I "..\io" /I "..\cbus" /I "..\bios" /I "..\vram" /I "..\sound" /I "..\sound\vermouth" /I "..\sound\getsnd" /I "..\fdd" /I "..\lio" /I "..\font" /I "..\generic" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x411 /d "_DEBUG"
# ADD RSC /l 0x411 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ddraw.lib dxguid.lib DSOUND.LIB winmm.lib comctl32.lib wsock32.lib /nologo /subsystem:windows /debug /machine:I386 /out:"..\bin/np2cd.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "np2c - Win32 Release"
# Name "np2c - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\bmpdata.c
# End Source File
# Begin Source File

SOURCE=..\common\lstarray.c
# End Source File
# Begin Source File

SOURCE=..\common\milstr.c
# End Source File
# Begin Source File

SOURCE=..\common\parts.c
# End Source File
# Begin Source File

SOURCE=..\common\profile.c
# End Source File
# Begin Source File

SOURCE=..\common\strres.c
# End Source File
# Begin Source File

SOURCE=..\common\textfile.c
# End Source File
# End Group
# Begin Group "cpu"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\i286c\dmap.c
# End Source File
# Begin Source File

SOURCE=..\i286c\egcmem.c
# End Source File
# Begin Source File

SOURCE=..\i286c\i286c.c
# End Source File
# Begin Source File

SOURCE=..\i286c\i286c_0f.c
# End Source File
# Begin Source File

SOURCE=..\i286c\i286c_8x.c
# End Source File
# Begin Source File

SOURCE=..\i286c\i286c_ea.c
# End Source File
# Begin Source File

SOURCE=..\i286c\i286c_f6.c
# End Source File
# Begin Source File

SOURCE=..\i286c\i286c_fe.c
# End Source File
# Begin Source File

SOURCE=..\i286c\i286c_mn.c
# End Source File
# Begin Source File

SOURCE=..\i286c\i286c_rp.c
# End Source File
# Begin Source File

SOURCE=..\i286c\i286c_sf.c
# End Source File
# Begin Source File

SOURCE=..\i286c\memory.c
# End Source File
# Begin Source File

SOURCE=..\i286c\v30patch.c
# End Source File
# End Group
# Begin Group "bios"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\BIOS\bios.c
# End Source File
# Begin Source File

SOURCE=..\BIOS\bios09.c
# End Source File
# Begin Source File

SOURCE=..\BIOS\bios0c.c
# End Source File
# Begin Source File

SOURCE=..\BIOS\bios12.c
# End Source File
# Begin Source File

SOURCE=..\BIOS\bios13.c
# End Source File
# Begin Source File

SOURCE=..\BIOS\bios18.c
# End Source File
# Begin Source File

SOURCE=..\BIOS\bios19.c
# End Source File
# Begin Source File

SOURCE=..\BIOS\bios1a.c
# End Source File
# Begin Source File

SOURCE=..\BIOS\bios1b.c
# End Source File
# Begin Source File

SOURCE=..\BIOS\bios1c.c
# End Source File
# Begin Source File

SOURCE=..\BIOS\bios1f.c
# End Source File
# Begin Source File

SOURCE=..\BIOS\sxsibios.c
# End Source File
# End Group
# Begin Group "sound"

# PROP Default_Filter ""
# Begin Group "vermouth"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sound\vermouth\midimod.c
# End Source File
# Begin Source File

SOURCE=..\sound\vermouth\midinst.c
# End Source File
# Begin Source File

SOURCE=..\sound\vermouth\midiout.c
# End Source File
# Begin Source File

SOURCE=..\sound\vermouth\midtable.c
# End Source File
# Begin Source File

SOURCE=..\sound\vermouth\midvoice.c
# End Source File
# End Group
# Begin Group "getsnd"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\sound\getsnd\getsmix.c
# End Source File
# Begin Source File

SOURCE=..\sound\getsnd\getsnd.c
# End Source File
# Begin Source File

SOURCE=..\sound\getsnd\getwave.c
# End Source File
# End Group
# Begin Source File

SOURCE=..\sound\adpcmc.c
# End Source File
# Begin Source File

SOURCE=..\sound\adpcmg.c
# End Source File
# Begin Source File

SOURCE=..\sound\beepc.c
# End Source File
# Begin Source File

SOURCE=..\sound\beepg.c
# End Source File
# Begin Source File

SOURCE=..\sound\cs4231c.c
# End Source File
# Begin Source File

SOURCE=..\sound\cs4231g.c
# End Source File
# Begin Source File

SOURCE=..\sound\fmboard.c
# End Source File
# Begin Source File

SOURCE=..\sound\fmtimer.c
# End Source File
# Begin Source File

SOURCE=..\sound\opngenc.c
# End Source File
# Begin Source File

SOURCE=..\sound\opngeng.c
# End Source File
# Begin Source File

SOURCE=..\sound\pcm86c.c
# End Source File
# Begin Source File

SOURCE=..\sound\pcm86g.c
# End Source File
# Begin Source File

SOURCE=..\sound\psggenc.c
# End Source File
# Begin Source File

SOURCE=..\sound\psggeng.c
# End Source File
# Begin Source File

SOURCE=..\sound\rhythmc.c
# End Source File
# Begin Source File

SOURCE=..\sound\s98.c
# End Source File
# Begin Source File

SOURCE=..\sound\sound.c
# End Source File
# Begin Source File

SOURCE=..\sound\soundrom.c
# End Source File
# Begin Source File

SOURCE=..\sound\tms3631c.c
# End Source File
# Begin Source File

SOURCE=..\sound\tms3631g.c
# End Source File
# End Group
# Begin Group "fdd"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\fdd\diskdrv.c
# End Source File
# Begin Source File

SOURCE=..\fdd\fdd_d88.c
# End Source File
# Begin Source File

SOURCE=..\fdd\fdd_mtr.c
# End Source File
# Begin Source File

SOURCE=..\fdd\fdd_xdf.c
# End Source File
# Begin Source File

SOURCE=..\fdd\fddfile.c
# End Source File
# Begin Source File

SOURCE=..\fdd\newdisk.c
# End Source File
# Begin Source File

SOURCE=..\fdd\sxsi.c
# End Source File
# End Group
# Begin Group "lio"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\lio\gcircle.c
# End Source File
# Begin Source File

SOURCE=..\lio\gline.c
# End Source File
# Begin Source File

SOURCE=..\lio\gpset.c
# End Source File
# Begin Source File

SOURCE=..\lio\gput1.c
# End Source File
# Begin Source File

SOURCE=..\lio\gscreen.c
# End Source File
# Begin Source File

SOURCE=..\lio\lio.c
# End Source File
# End Group
# Begin Group "font"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\font\font.c
# End Source File
# Begin Source File

SOURCE=..\font\fontdata.c
# End Source File
# Begin Source File

SOURCE=..\font\fontfm7.c
# End Source File
# Begin Source File

SOURCE=..\font\fontmake.c
# End Source File
# Begin Source File

SOURCE=..\font\fontpc88.c
# End Source File
# Begin Source File

SOURCE=..\font\fontpc98.c
# End Source File
# Begin Source File

SOURCE=..\font\fontv98.c
# End Source File
# Begin Source File

SOURCE=..\font\fontx1.c
# End Source File
# Begin Source File

SOURCE=..\font\fontx68k.c
# End Source File
# End Group
# Begin Group "win9x"

# PROP Default_Filter ""
# Begin Group "dialog"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\dialog\d_about.cpp
# End Source File
# Begin Source File

SOURCE=.\dialog\d_bmp.cpp
# End Source File
# Begin Source File

SOURCE=.\dialog\d_clnd.cpp
# End Source File
# Begin Source File

SOURCE=.\dialog\d_config.cpp
# End Source File
# Begin Source File

SOURCE=.\dialog\d_disk.cpp
# End Source File
# Begin Source File

SOURCE=.\dialog\d_mpu98.cpp
# End Source File
# Begin Source File

SOURCE=.\dialog\d_screen.cpp
# End Source File
# Begin Source File

SOURCE=.\dialog\d_sound.cpp
# End Source File
# Begin Source File

SOURCE=.\dialog\dialogs.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\commng.cpp
# End Source File
# Begin Source File

SOURCE=.\dosio.cpp
# End Source File
# Begin Source File

SOURCE=.\fontmng.cpp
# End Source File
# Begin Source File

SOURCE=.\ini.cpp
# End Source File
# Begin Source File

SOURCE=.\joymng.cpp
# End Source File
# Begin Source File

SOURCE=.\menu.cpp
# End Source File
# Begin Source File

SOURCE=.\mousemng.cpp
# End Source File
# Begin Source File

SOURCE=.\np2.cpp
# End Source File
# Begin Source File

SOURCE=.\np2.rc
# End Source File
# Begin Source File

SOURCE=.\np2arg.cpp
# End Source File
# Begin Source File

SOURCE=.\scrnmng.cpp
# End Source File
# Begin Source File

SOURCE=.\soundmng.cpp
# End Source File
# Begin Source File

SOURCE=.\sysmng.cpp
# End Source File
# Begin Source File

SOURCE=.\taskmng.cpp
# End Source File
# Begin Source File

SOURCE=.\timemng.cpp
# End Source File
# Begin Source File

SOURCE=.\trace.cpp
# End Source File
# Begin Source File

SOURCE=.\winkbd.cpp
# End Source File
# End Group
# Begin Group "io"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\IO\artic.c
# End Source File
# Begin Source File

SOURCE=..\IO\cgrom.c
# End Source File
# Begin Source File

SOURCE=..\IO\cpuio.c
# End Source File
# Begin Source File

SOURCE=..\IO\crtc.c
# End Source File
# Begin Source File

SOURCE=..\IO\dipsw.c
# End Source File
# Begin Source File

SOURCE=..\IO\dmac.c
# End Source File
# Begin Source File

SOURCE=..\IO\egc.c
# End Source File
# Begin Source File

SOURCE=..\IO\emsio.c
# End Source File
# Begin Source File

SOURCE=..\IO\epsonio.c
# End Source File
# Begin Source File

SOURCE=..\IO\fdc.c
# End Source File
# Begin Source File

SOURCE=..\IO\fdd320.c
# End Source File
# Begin Source File

SOURCE=..\IO\gdc.c
# End Source File
# Begin Source File

SOURCE=..\IO\gdc_pset.c
# End Source File
# Begin Source File

SOURCE=..\IO\gdc_sub.c
# End Source File
# Begin Source File

SOURCE=..\IO\iocore.c
# End Source File
# Begin Source File

SOURCE=..\IO\mouseif.c
# End Source File
# Begin Source File

SOURCE=..\IO\necio.c
# End Source File
# Begin Source File

SOURCE=..\IO\nmiio.c
# End Source File
# Begin Source File

SOURCE=..\IO\np2sysp.c
# End Source File
# Begin Source File

SOURCE=..\IO\pic.c
# End Source File
# Begin Source File

SOURCE=..\IO\pit.c
# End Source File
# Begin Source File

SOURCE=..\IO\printif.c
# End Source File
# Begin Source File

SOURCE=..\IO\serial.c
# End Source File
# Begin Source File

SOURCE=..\IO\sysport.c
# End Source File
# Begin Source File

SOURCE=..\IO\upd4990.c
# End Source File
# End Group
# Begin Group "cbus"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\CBUS\amd98.c
# End Source File
# Begin Source File

SOURCE=..\CBUS\board118.c
# End Source File
# Begin Source File

SOURCE=..\CBUS\board14.c
# End Source File
# Begin Source File

SOURCE=..\CBUS\board26k.c
# End Source File
# Begin Source File

SOURCE=..\CBUS\board86.c
# End Source File
# Begin Source File

SOURCE=..\CBUS\boardspb.c
# End Source File
# Begin Source File

SOURCE=..\CBUS\boardx2.c
# End Source File
# Begin Source File

SOURCE=..\CBUS\cbuscore.c
# End Source File
# Begin Source File

SOURCE=..\CBUS\cs4231io.c
# End Source File
# Begin Source File

SOURCE=..\CBUS\mpu98ii.c
# End Source File
# Begin Source File

SOURCE=..\CBUS\pc9861k.c
# End Source File
# Begin Source File

SOURCE=..\CBUS\pcm86io.c
# End Source File
# End Group
# Begin Group "vram"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\VRAM\dispsync.c
# End Source File
# Begin Source File

SOURCE=..\VRAM\makegrph.c
# End Source File
# Begin Source File

SOURCE=..\VRAM\maketext.c
# End Source File
# Begin Source File

SOURCE=..\VRAM\maketgrp.c
# End Source File
# Begin Source File

SOURCE=..\VRAM\palettes.c
# End Source File
# Begin Source File

SOURCE=..\VRAM\scrnbmp.c
# End Source File
# Begin Source File

SOURCE=..\VRAM\scrndraw.c
# End Source File
# Begin Source File

SOURCE=..\VRAM\sdraw.c
# End Source File
# Begin Source File

SOURCE=..\VRAM\vram.c
# End Source File
# End Group
# Begin Group "generic"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\generic\cmjasts.c
# End Source File
# Begin Source File

SOURCE=..\generic\cmver.c
# End Source File
# Begin Source File

SOURCE=..\generic\dipswbmp.c
# End Source File
# Begin Source File

SOURCE=..\generic\hostdrv.c
# End Source File
# Begin Source File

SOURCE=..\generic\hostdrvs.c
# End Source File
# Begin Source File

SOURCE=..\generic\np2info.c
# End Source File
# End Group
# Begin Source File

SOURCE=..\calendar.c
# End Source File
# Begin Source File

SOURCE=..\debugsub.c
# End Source File
# Begin Source File

SOURCE=..\keystat.c
# End Source File
# Begin Source File

SOURCE=..\nevent.c
# End Source File
# Begin Source File

SOURCE=..\pccore.c
# End Source File
# Begin Source File

SOURCE=..\statsave.c
# End Source File
# Begin Source File

SOURCE=..\timing.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\icons\np2.ico
# End Source File
# Begin Source File

SOURCE=.\icons\np2debug.ico
# End Source File
# End Group
# End Target
# End Project
