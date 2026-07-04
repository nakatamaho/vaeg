# Microsoft Developer Studio Project File - Name="sdlw32s" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** 編集しないでください **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=sdlw32s - Win32 Trace
!MESSAGE これは有効なﾒｲｸﾌｧｲﾙではありません。 このﾌﾟﾛｼﾞｪｸﾄをﾋﾞﾙﾄﾞするためには NMAKE を使用してください。
!MESSAGE [ﾒｲｸﾌｧｲﾙのｴｸｽﾎﾟｰﾄ] ｺﾏﾝﾄﾞを使用して実行してください
!MESSAGE 
!MESSAGE NMAKE /f "sdlw32s.mak".
!MESSAGE 
!MESSAGE NMAKE の実行時に構成を指定できます
!MESSAGE ｺﾏﾝﾄﾞ ﾗｲﾝ上でﾏｸﾛの設定を定義します。例:
!MESSAGE 
!MESSAGE NMAKE /f "sdlw32s.mak" CFG="sdlw32s - Win32 Trace"
!MESSAGE 
!MESSAGE 選択可能なﾋﾞﾙﾄﾞ ﾓｰﾄﾞ:
!MESSAGE 
!MESSAGE "sdlw32s - Win32 Release" ("Win32 (x86) Console Application" 用)
!MESSAGE "sdlw32s - Win32 Trace" ("Win32 (x86) Console Application" 用)
!MESSAGE "sdlw32s - Win32 Debug" ("Win32 (x86) Console Application" 用)
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "sdlw32s - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\bin\sdl"
# PROP Intermediate_Dir "..\obj\sdlw32srel"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I ".\\" /I ".\win32s" /I "..\\" /I "..\common" /I "..\i286c" /I "..\io" /I "..\cbus" /I "..\bios" /I "..\vram" /I "..\sound" /I "..\sound\vermouth" /I "..\sound\getsnd" /I "..\fdd" /I "..\lio" /I "..\font" /I "..\generic" /I "..\embed" /I "..\embed\menu" /I "..\embed\menubase" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "SIZE_VGA" /D "RESOURCE_US" /YX /FD /c
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /out:"..\bin\sdl/np2.exe"

!ELSEIF  "$(CFG)" == "sdlw32s - Win32 Trace"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Trace"
# PROP BASE Intermediate_Dir "Trace"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\bin\sdl"
# PROP Intermediate_Dir "..\obj\sdlw32strc"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /I ".\\" /I ".\win32s" /I "..\\" /I "..\common" /I "..\i286c" /I "..\io" /I "..\cbus" /I "..\bios" /I "..\vram" /I "..\sound" /I "..\sound\vermouth" /I "..\fdd" /I "..\lio" /I "..\font" /I "..\embed" /I "..\embed\qvga" /I "..\embed\menu" /I "..\embed\menubase" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I ".\\" /I ".\win32s" /I "..\\" /I "..\common" /I "..\i286c" /I "..\io" /I "..\cbus" /I "..\bios" /I "..\vram" /I "..\sound" /I "..\sound\vermouth" /I "..\sound\getsnd" /I "..\fdd" /I "..\lio" /I "..\font" /I "..\generic" /I "..\embed" /I "..\embed\menu" /I "..\embed\menubase" /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "TRACE" /D "MEMTRACE" /D "RESOURCE_US" /YX /FD /c
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /out:"..\bin\sdl/np2t.exe"

!ELSEIF  "$(CFG)" == "sdlw32s - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\bin\sdl"
# PROP Intermediate_Dir "..\obj\sdlw32sdbg"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I ".\\" /I ".\win32s" /I "..\\" /I "..\common" /I "..\i286c" /I "..\io" /I "..\cbus" /I "..\bios" /I "..\vram" /I "..\sound" /I "..\sound\vermouth" /I "..\sound\getsnd" /I "..\fdd" /I "..\lio" /I "..\font" /I "..\generic" /I "..\embed" /I "..\embed\menu" /I "..\embed\menubase" /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /YX /FD /GZ /c
# ADD BASE RSC /l 0x411 /d "_DEBUG"
# ADD RSC /l 0x411 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /out:"..\bin\sdl/np2d.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "sdlw32s - Win32 Release"
# Name "sdlw32s - Win32 Trace"
# Name "sdlw32s - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "common"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\common\_memory.c
# End Source File
# Begin Source File

SOURCE=..\common\bmpdata.c
# End Source File
# Begin Source File

SOURCE=..\common\codecnv.c
# End Source File
# Begin Source File

SOURCE=..\common\lstarray.c
# End Source File
# Begin Source File

SOURCE=..\common\milstr.c
# End Source File
# Begin Source File

SOURCE=..\common\mimpidef.c
# End Source File
# Begin Source File

SOURCE=..\common\parts.c
# End Source File
# Begin Source File

SOURCE=..\common\profile.c
# End Source File
# Begin Source File

SOURCE=..\common\rect.c
# End Source File
# Begin Source File

SOURCE=..\common\resize.c
# End Source File
# Begin Source File

SOURCE=..\common\strres.c
# End Source File
# Begin Source File

SOURCE=..\common\textfile.c
# End Source File
# Begin Source File

SOURCE=..\common\wavefile.c
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
# Begin Group "io"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\io\artic.c
# End Source File
# Begin Source File

SOURCE=..\io\cgrom.c
# End Source File
# Begin Source File

SOURCE=..\io\cpuio.c
# End Source File
# Begin Source File

SOURCE=..\io\crtc.c
# End Source File
# Begin Source File

SOURCE=..\io\dipsw.c
# End Source File
# Begin Source File

SOURCE=..\io\dmac.c
# End Source File
# Begin Source File

SOURCE=..\io\egc.c
# End Source File
# Begin Source File

SOURCE=..\io\emsio.c
# End Source File
# Begin Source File

SOURCE=..\io\epsonio.c
# End Source File
# Begin Source File

SOURCE=..\io\fdc.c
# End Source File
# Begin Source File

SOURCE=..\io\fdd320.c
# End Source File
# Begin Source File

SOURCE=..\io\gdc.c
# End Source File
# Begin Source File

SOURCE=..\io\gdc_pset.c
# End Source File
# Begin Source File

SOURCE=..\io\gdc_sub.c
# End Source File
# Begin Source File

SOURCE=..\io\iocore.c
# End Source File
# Begin Source File

SOURCE=..\io\mouseif.c
# End Source File
# Begin Source File

SOURCE=..\io\necio.c
# End Source File
# Begin Source File

SOURCE=..\io\nmiio.c
# End Source File
# Begin Source File

SOURCE=..\io\np2sysp.c
# End Source File
# Begin Source File

SOURCE=..\io\pic.c
# End Source File
# Begin Source File

SOURCE=..\io\pit.c
# End Source File
# Begin Source File

SOURCE=..\io\printif.c
# End Source File
# Begin Source File

SOURCE=..\io\serial.c
# End Source File
# Begin Source File

SOURCE=..\io\sysport.c
# End Source File
# Begin Source File

SOURCE=..\io\upd4990.c
# End Source File
# End Group
# Begin Group "cbus"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\cbus\amd98.c
# End Source File
# Begin Source File

SOURCE=..\cbus\board118.c
# End Source File
# Begin Source File

SOURCE=..\cbus\board14.c
# End Source File
# Begin Source File

SOURCE=..\cbus\board26k.c
# End Source File
# Begin Source File

SOURCE=..\cbus\board86.c
# End Source File
# Begin Source File

SOURCE=..\cbus\boardspb.c
# End Source File
# Begin Source File

SOURCE=..\cbus\boardx2.c
# End Source File
# Begin Source File

SOURCE=..\cbus\cbuscore.c
# End Source File
# Begin Source File

SOURCE=..\cbus\cs4231io.c
# End Source File
# Begin Source File

SOURCE=..\cbus\mpu98ii.c
# End Source File
# Begin Source File

SOURCE=..\cbus\pc9861k.c
# End Source File
# Begin Source File

SOURCE=..\cbus\pcm86io.c
# End Source File
# Begin Source File

SOURCE=..\cbus\sasiio.c
# End Source File
# Begin Source File

SOURCE=..\cbus\scsicmd.c
# End Source File
# Begin Source File

SOURCE=..\cbus\scsiio.c
# End Source File
# End Group
# Begin Group "bios"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\bios\bios.c
# End Source File
# Begin Source File

SOURCE=..\bios\bios09.c
# End Source File
# Begin Source File

SOURCE=..\bios\bios0c.c
# End Source File
# Begin Source File

SOURCE=..\bios\bios12.c
# End Source File
# Begin Source File

SOURCE=..\bios\bios13.c
# End Source File
# Begin Source File

SOURCE=..\bios\bios18.c
# End Source File
# Begin Source File

SOURCE=..\bios\bios19.c
# End Source File
# Begin Source File

SOURCE=..\bios\bios1a.c
# End Source File
# Begin Source File

SOURCE=..\bios\bios1b.c
# End Source File
# Begin Source File

SOURCE=..\bios\bios1c.c
# End Source File
# Begin Source File

SOURCE=..\bios\bios1f.c
# End Source File
# Begin Source File

SOURCE=..\bios\sxsibios.c
# End Source File
# End Group
# Begin Group "vram"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\vram\dispsync.c
# End Source File
# Begin Source File

SOURCE=..\vram\makegrph.c
# End Source File
# Begin Source File

SOURCE=..\vram\maketext.c
# End Source File
# Begin Source File

SOURCE=..\vram\maketgrp.c
# End Source File
# Begin Source File

SOURCE=..\vram\palettes.c
# End Source File
# Begin Source File

SOURCE=..\vram\scrnbmp.c
# End Source File
# Begin Source File

SOURCE=..\vram\scrndraw.c
# End Source File
# Begin Source File

SOURCE=..\vram\sdraw.c
# End Source File
# Begin Source File

SOURCE=..\vram\sdrawq16.c
# End Source File
# Begin Source File

SOURCE=..\vram\vram.c
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

SOURCE=..\sound\sndcsec.c
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
# Begin Group "sdl"

# PROP Default_Filter ""
# Begin Group "win32"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\win32s\sdl.c
# End Source File
# Begin Source File

SOURCE=.\win32s\sdl_ttf.c
# End Source File
# Begin Source File

SOURCE=.\win32s\sdlaudio.c
# End Source File
# Begin Source File

SOURCE=.\win32s\sdlevent.c
# End Source File
# Begin Source File

SOURCE=.\win32s\sdlmpw.c
# End Source File
# Begin Source File

SOURCE=.\win32s\sdlvideo.c
# End Source File
# End Group
# Begin Source File

SOURCE=.\commng.c
# End Source File
# Begin Source File

SOURCE=.\dosio.c
# End Source File
# Begin Source File

SOURCE=.\fontmng.c
# End Source File
# Begin Source File

SOURCE=.\ini.c
# End Source File
# Begin Source File

SOURCE=.\inputmng.c
# End Source File
# Begin Source File

SOURCE=.\joymng.c
# End Source File
# Begin Source File

SOURCE=.\mousemng.c
# End Source File
# Begin Source File

SOURCE=.\np2.c
# End Source File
# Begin Source File

SOURCE=.\scrnmng.c
# End Source File
# Begin Source File

SOURCE=.\sdlkbd.c
# End Source File
# Begin Source File

SOURCE=.\soundmng.c
# End Source File
# Begin Source File

SOURCE=.\sysmenu.c
# End Source File
# Begin Source File

SOURCE=.\sysmng.c
# End Source File
# Begin Source File

SOURCE=.\taskmng.c
# End Source File
# Begin Source File

SOURCE=.\timemng.c
# End Source File
# Begin Source File

SOURCE=.\trace.c
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

SOURCE=..\generic\hostdrv.c
# End Source File
# Begin Source File

SOURCE=..\generic\hostdrvs.c
# End Source File
# End Group
# Begin Group "embed"

# PROP Default_Filter ""
# Begin Group "menu"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\embed\menu\dlgabout.c
# End Source File
# Begin Source File

SOURCE=..\embed\menu\dlgcfg.c
# End Source File
# Begin Source File

SOURCE=..\embed\menu\dlgscr.c
# End Source File
# Begin Source File

SOURCE=..\embed\menu\filesel.c
# End Source File
# Begin Source File

SOURCE=..\embed\menu\menustr.c
# End Source File
# End Group
# Begin Group "menubase"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\embed\menubase\menubase.c
# End Source File
# Begin Source File

SOURCE=..\embed\menubase\menudlg.c
# End Source File
# Begin Source File

SOURCE=..\embed\menubase\menuicon.c
# End Source File
# Begin Source File

SOURCE=..\embed\menubase\menumbox.c
# End Source File
# Begin Source File

SOURCE=..\embed\menubase\menures.c
# End Source File
# Begin Source File

SOURCE=..\embed\menubase\menusys.c
# End Source File
# Begin Source File

SOURCE=..\embed\menubase\menuvram.c
# End Source File
# End Group
# Begin Source File

SOURCE=..\embed\vramhdl.c
# End Source File
# Begin Source File

SOURCE=..\embed\vrammix.c
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
# End Group
# End Target
# End Project
