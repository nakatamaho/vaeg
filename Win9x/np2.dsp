# Microsoft Developer Studio Project File - Name="np2" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** 編集しないでください **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=np2 - Win32 Trace
!MESSAGE これは有効なﾒｲｸﾌｧｲﾙではありません。 このﾌﾟﾛｼﾞｪｸﾄをﾋﾞﾙﾄﾞするためには NMAKE を使用してください。
!MESSAGE [ﾒｲｸﾌｧｲﾙのｴｸｽﾎﾟｰﾄ] ｺﾏﾝﾄﾞを使用して実行してください
!MESSAGE 
!MESSAGE NMAKE /f "np2.mak".
!MESSAGE 
!MESSAGE NMAKE の実行時に構成を指定できます
!MESSAGE ｺﾏﾝﾄﾞ ﾗｲﾝ上でﾏｸﾛの設定を定義します。例:
!MESSAGE 
!MESSAGE NMAKE /f "np2.mak" CFG="np2 - Win32 Trace"
!MESSAGE 
!MESSAGE 選択可能なﾋﾞﾙﾄﾞ ﾓｰﾄﾞ:
!MESSAGE 
!MESSAGE "np2 - Win32 Release" ("Win32 (x86) Application" 用)
!MESSAGE "np2 - Win32 Trace" ("Win32 (x86) Application" 用)
!MESSAGE "np2 - Win32 WaveRec" ("Win32 (x86) Application" 用)
!MESSAGE "np2 - Win32 Debug" ("Win32 (x86) Application" 用)
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "np2 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\bin"
# PROP Intermediate_Dir "..\obj\rel"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I ".\\" /I ".\x86" /I ".\dialog" /I ".\debuguty" /I "..\\" /I "..\common" /I "..\i286x" /I "..\io" /I "..\cbus" /I "..\bios" /I "..\vram" /I "..\sound" /I "..\sound\vermouth" /I "..\sound\getsnd" /I "..\fdd" /I "..\lio" /I "..\font" /I "..\generic" /I "..\CPUXVA" /I "..\CPUCVA" /I "../BIOSVA" /I "../IOVA" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /FAcs /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib ddraw.lib dxguid.lib DSOUND.LIB winmm.lib comdlg32.lib comctl32.lib wsock32.lib shell32.lib /nologo /subsystem:windows /map /machine:I386 /out:"..\bin/vaeg.exe"

!ELSEIF  "$(CFG)" == "np2 - Win32 Trace"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Trace"
# PROP BASE Intermediate_Dir "Trace"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\bin"
# PROP Intermediate_Dir "..\obj\trc"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I ".\\" /I ".\x86" /I ".\dialog" /I ".\debuguty" /I "..\\" /I "..\common" /I "..\i286x" /I "..\io" /I "..\cbus" /I "..\bios" /I "..\vram" /I "..\sound" /I "..\sound\vermouth" /I "..\sound\getsnd" /I "..\fdd" /I "..\lio" /I "..\font" /I "..\generic" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "TRACE" /D "MEMTRACE" /FAcs /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib ddraw.lib dxguid.lib DSOUND.LIB winmm.lib comdlg32.lib comctl32.lib wsock32.lib shell32.lib /nologo /subsystem:windows /map /machine:I386 /out:"..\bin/np2t.exe"

!ELSEIF  "$(CFG)" == "np2 - Win32 WaveRec"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WaveRec"
# PROP BASE Intermediate_Dir "WaveRec"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "..\bin"
# PROP Intermediate_Dir "..\obj\wr"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I ".\\" /I ".\x86" /I ".\dialog" /I ".\debuguty" /I "..\\" /I "..\common" /I "..\i286x" /I "..\io" /I "..\cbus" /I "..\bios" /I "..\vram" /I "..\sound" /I "..\sound\vermouth" /I "..\sound\getsnd" /I "..\fdd" /I "..\lio" /I "..\font" /I "..\generic" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "SUPPORT_WAVEREC" /FAcs /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib ddraw.lib dxguid.lib DSOUND.LIB winmm.lib comdlg32.lib comctl32.lib wsock32.lib shell32.lib /nologo /subsystem:windows /map /machine:I386 /out:"..\bin/np2wr.exe"

!ELSEIF  "$(CFG)" == "np2 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "..\bin"
# PROP Intermediate_Dir "..\obj\dbg"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I ".\\" /I ".\x86" /I ".\dialog" /I ".\debuguty" /I "..\\" /I "..\common" /I "..\i286x" /I "..\io" /I "..\cbus" /I "..\bios" /I "..\vram" /I "..\sound" /I "..\sound\vermouth" /I "..\sound\getsnd" /I "..\fdd" /I "..\lio" /I "..\font" /I "..\generic" /I "..\CPUXVA" /I "..\CPUCVA" /I "../BIOSVA" /I "../IOVA" /D "_DEBUG" /D "TRACE" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "CPUDEBUG" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x411 /d "_DEBUG"
# ADD RSC /l 0x411 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib ddraw.lib dxguid.lib DSOUND.LIB winmm.lib comdlg32.lib comctl32.lib wsock32.lib shell32.lib /nologo /subsystem:windows /debug /machine:I386 /out:"..\bin/vaegd.exe" /pdbtype:sept

!ENDIF 

# Begin Target

# Name "np2 - Win32 Release"
# Name "np2 - Win32 Trace"
# Name "np2 - Win32 WaveRec"
# Name "np2 - Win32 Debug"
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

SOURCE=..\common\lstarray.c
# End Source File
# Begin Source File

SOURCE=..\common\milstr.c
# End Source File
# Begin Source File

SOURCE=..\common\mimpidef.c
# End Source File
# Begin Source File

SOURCE=.\x86\parts.x86

!IF  "$(CFG)" == "np2 - Win32 Release"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\rel
InputPath=.\x86\parts.x86
InputName=PARTS

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 Trace"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\trc
InputPath=.\x86\parts.x86
InputName=PARTS

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 WaveRec"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\wr
InputPath=.\x86\parts.x86
InputName=PARTS

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 Debug"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\dbg
InputPath=.\x86\parts.x86
InputName=PARTS

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ENDIF 

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
# Begin Source File

SOURCE=..\common\wavefile.c
# End Source File
# End Group
# Begin Group "cpu"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\i286x\dmap.x86

!IF  "$(CFG)" == "np2 - Win32 Release"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\rel
InputPath=..\i286x\dmap.x86
InputName=DMAP

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 Trace"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\trc
InputPath=..\i286x\dmap.x86
InputName=DMAP

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 WaveRec"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\wr
InputPath=..\i286x\dmap.x86
InputName=DMAP

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 Debug"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\dbg
InputPath=..\i286x\dmap.x86
InputName=DMAP

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\i286x\egcmem.x86

!IF  "$(CFG)" == "np2 - Win32 Release"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\rel
InputPath=..\i286x\egcmem.x86
InputName=EGCMEM

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 Trace"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\trc
InputPath=..\i286x\egcmem.x86
InputName=EGCMEM

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 WaveRec"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\wr
InputPath=..\i286x\egcmem.x86
InputName=EGCMEM

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 Debug"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\dbg
InputPath=..\i286x\egcmem.x86
InputName=EGCMEM

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\i286x\i286x.cpp
# End Source File
# Begin Source File

SOURCE=..\i286x\i286xadr.cpp
# End Source File
# Begin Source File

SOURCE=..\i286x\i286xcts.cpp
# End Source File
# Begin Source File

SOURCE=..\i286x\i286xrep.cpp
# End Source File
# Begin Source File

SOURCE=..\i286x\i286xs.cpp
# End Source File
# Begin Source File

SOURCE=..\i286x\memory.x86

!IF  "$(CFG)" == "np2 - Win32 Release"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\rel
InputPath=..\i286x\memory.x86
InputName=MEMORY

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 Trace"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\trc
InputPath=..\i286x\memory.x86
InputName=MEMORY

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 WaveRec"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\wr
InputPath=..\i286x\memory.x86
InputName=MEMORY

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 Debug"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\dbg
InputPath=..\i286x\memory.x86
InputName=MEMORY

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\i286x\v30patch.cpp
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

SOURCE=.\x86\opngeng.x86

!IF  "$(CFG)" == "np2 - Win32 Release"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\rel
InputPath=.\x86\opngeng.x86
InputName=OPNGENG

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 Trace"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\trc
InputPath=.\x86\opngeng.x86
InputName=OPNGENG

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj -l $(IntDir)\$(InputName).cod

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 WaveRec"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\wr
InputPath=.\x86\opngeng.x86
InputName=OPNGENG

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj -l $(IntDir)\$(InputName).cod

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 Debug"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\dbg
InputPath=.\x86\opngeng.x86
InputName=OPNGENG

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ENDIF 

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
# Begin Group "Win9x"

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

SOURCE=.\dialog\d_bms.cpp
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

SOURCE=.\dialog\d_oprecord.cpp
# End Source File
# Begin Source File

SOURCE=.\dialog\d_screen.cpp
# End Source File
# Begin Source File

SOURCE=.\dialog\d_serial.cpp
# End Source File
# Begin Source File

SOURCE=.\dialog\d_sound.cpp
# End Source File
# Begin Source File

SOURCE=.\dialog\dialogs.cpp
# End Source File
# Begin Source File

SOURCE=.\dialog\np2class.cpp
# End Source File
# End Group
# Begin Group "debuguty"

# PROP Default_Filter ""
# Begin Source File

SOURCE=.\debuguty\debugctrl.cpp
# End Source File
# Begin Source File

SOURCE=.\debuguty\view1mb.cpp
# End Source File
# Begin Source File

SOURCE=.\debuguty\viewasm.cpp
# End Source File
# Begin Source File

SOURCE=.\debuguty\viewcmn.cpp
# End Source File
# Begin Source File

SOURCE=.\debuguty\viewer.cpp
# End Source File
# Begin Source File

SOURCE=.\debuguty\viewgactrlva.cpp
# End Source File
# Begin Source File

SOURCE=.\debuguty\viewmem.cpp
# End Source File
# Begin Source File

SOURCE=.\debuguty\viewmenu.cpp
# End Source File
# Begin Source File

SOURCE=.\debuguty\viewreg.cpp
# End Source File
# Begin Source File

SOURCE=.\debuguty\viewseg.cpp
# End Source File
# Begin Source File

SOURCE=.\debuguty\viewsnd.cpp
# End Source File
# Begin Source File

SOURCE=.\debuguty\viewsubasm.cpp
# End Source File
# Begin Source File

SOURCE=.\debuguty\viewsubmem.cpp
# End Source File
# Begin Source File

SOURCE=.\debuguty\viewsubreg.cpp
# End Source File
# Begin Source File

SOURCE=.\debuguty\viewvabank.cpp
# End Source File
# Begin Source File

SOURCE=.\debuguty\viewvideova.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=.\cmmidi.cpp
# End Source File
# Begin Source File

SOURCE=.\cmpara.cpp
# End Source File
# Begin Source File

SOURCE=.\cmserial.cpp
# End Source File
# Begin Source File

SOURCE=.\commng.cpp
# End Source File
# Begin Source File

SOURCE=.\x86\cputype.x86

!IF  "$(CFG)" == "np2 - Win32 Release"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\rel
InputPath=.\x86\cputype.x86
InputName=CPUTYPE

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 Trace"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\trc
InputPath=.\x86\cputype.x86
InputName=CPUTYPE

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 WaveRec"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\wr
InputPath=.\x86\cputype.x86
InputName=CPUTYPE

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 Debug"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\dbg
InputPath=.\x86\cputype.x86
InputName=CPUTYPE

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dclock.cpp
# End Source File
# Begin Source File

SOURCE=.\dclockd.x86

!IF  "$(CFG)" == "np2 - Win32 Release"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\rel
InputPath=.\dclockd.x86
InputName=DCLOCKD

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 Trace"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\trc
InputPath=.\dclockd.x86
InputName=DCLOCKD

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 WaveRec"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\wr
InputPath=.\dclockd.x86
InputName=DCLOCKD

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 Debug"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\dbg
InputPath=.\dclockd.x86
InputName=DCLOCKD

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dd2.cpp
# End Source File
# Begin Source File

SOURCE=.\dosio.cpp
# End Source File
# Begin Source File

SOURCE=.\extromio.cpp
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

SOURCE=.\juliet.cpp
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

SOURCE=.\sstp.cpp
# End Source File
# Begin Source File

SOURCE=.\sstpmsg.cpp
# End Source File
# Begin Source File

SOURCE=.\subwind.cpp
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

SOURCE=.\toolwin.cpp
# End Source File
# Begin Source File

SOURCE=.\trace.cpp
# End Source File
# Begin Source File

SOURCE=.\winkbd.cpp
# End Source File
# Begin Source File

SOURCE=.\winloc.cpp
# End Source File
# End Group
# Begin Group "IO"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\IO\artic.c
# End Source File
# Begin Source File

SOURCE=..\IO\bmsio.c
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

SOURCE=..\IO\np2vasup.c
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
# Begin Group "CBUS"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\CBUS\amd98.c
# End Source File
# Begin Source File

SOURCE=.\board118.c
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

SOURCE=..\CBUS\ideio.c
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
# Begin Source File

SOURCE=..\CBUS\sasiio.c
# End Source File
# Begin Source File

SOURCE=..\CBUS\scsicmd.c
# End Source File
# Begin Source File

SOURCE=..\CBUS\scsiio.c
# End Source File
# End Group
# Begin Group "vram"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\VRAM\dispsync.c
# End Source File
# Begin Source File

SOURCE=.\x86\makegrph.x86

!IF  "$(CFG)" == "np2 - Win32 Release"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\rel
InputPath=.\x86\makegrph.x86
InputName=MAKEGRPH

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 Trace"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\trc
InputPath=.\x86\makegrph.x86
InputName=MAKEGRPH

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 WaveRec"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\wr
InputPath=.\x86\makegrph.x86
InputName=MAKEGRPH

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 Debug"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\dbg
InputPath=.\x86\makegrph.x86
InputName=MAKEGRPH

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ENDIF 

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

SOURCE=..\VRAM\scrndraw.c
# End Source File
# Begin Source File

SOURCE=..\VRAM\scrnsave.c
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

SOURCE=..\generic\cmndraw.c
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

SOURCE=..\generic\keydisp.c
# End Source File
# Begin Source File

SOURCE=..\generic\np2info.c
# End Source File
# Begin Source File

SOURCE=..\generic\softkbd.c
# End Source File
# Begin Source File

SOURCE=..\generic\unasm.c
# End Source File
# End Group
# Begin Group "vramva"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\VRAMVA\makegrphva.c
# End Source File
# Begin Source File

SOURCE=..\VRAMVA\makesprva.c
# End Source File
# Begin Source File

SOURCE=..\VRAMVA\maketextva.c
# End Source File
# Begin Source File

SOURCE=..\VRAMVA\palettesva.c
# End Source File
# Begin Source File

SOURCE=..\VRAMVA\scrndrawva.c
# End Source File
# Begin Source File

SOURCE=..\VRAMVA\sdrawva.c
# End Source File
# End Group
# Begin Group "cpuxva"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\CPUXVA\memoryva.x86

!IF  "$(CFG)" == "np2 - Win32 Release"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\rel
InputPath=..\CPUXVA\memoryva.x86
InputName=MEMORYVA

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 Trace"

!ELSEIF  "$(CFG)" == "np2 - Win32 WaveRec"

!ELSEIF  "$(CFG)" == "np2 - Win32 Debug"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\dbg
InputPath=..\CPUXVA\memoryva.x86
InputName=MEMORYVA

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	nasm -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "biosva"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\BIOSVA\biosva.c
# End Source File
# End Group
# Begin Group "IOVA"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\IOVA\bkupmemva.c
# End Source File
# Begin Source File

SOURCE=..\IOVA\boardsb2.c
# End Source File
# Begin Source File

SOURCE=..\IOVA\cgromva.c
# End Source File
# Begin Source File

SOURCE=..\IOVA\fdsubsys.c
# End Source File
# Begin Source File

SOURCE=..\IOVA\gactrlva.c
# End Source File
# Begin Source File

SOURCE=..\IOVA\i8255.c
# End Source File
# Begin Source File

SOURCE=..\IOVA\iocoreva.c
# End Source File
# Begin Source File

SOURCE=..\IOVA\memctrlva.c
# End Source File
# Begin Source File

SOURCE=..\IOVA\mouseifva.c
# End Source File
# Begin Source File

SOURCE=..\IOVA\sgp.c
# End Source File
# Begin Source File

SOURCE=..\IOVA\subsystem.cpp
# End Source File
# Begin Source File

SOURCE=..\IOVA\subsystemif.c
# End Source File
# Begin Source File

SOURCE=..\IOVA\subsystemmx.c
# End Source File
# Begin Source File

SOURCE=..\IOVA\sysportva.c
# End Source File
# Begin Source File

SOURCE=..\IOVA\tsp.c
# End Source File
# Begin Source File

SOURCE=..\IOVA\upd9002.c
# End Source File
# Begin Source File

SOURCE=..\IOVA\va91.c
# End Source File
# Begin Source File

SOURCE=..\IOVA\videova.c
# End Source File
# End Group
# Begin Group "cpucva"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\CPUCVA\GVRAMVA.c
# End Source File
# Begin Source File

SOURCE=..\CPUCVA\z80c.cpp
# End Source File
# Begin Source File

SOURCE=..\CPUCVA\z80diag.cpp
# End Source File
# End Group
# Begin Source File

SOURCE=..\breakpoint.c
# End Source File
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

SOURCE=..\oprecord.c
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
# Begin Source File

SOURCE=..\breakpoint.h
# End Source File
# Begin Source File

SOURCE=..\IOVA\fdsubsys.h
# End Source File
# Begin Source File

SOURCE=..\IOVA\GACCESS.H
# End Source File
# Begin Source File

SOURCE=..\VRAMVA\makesprva.h
# End Source File
# Begin Source File

SOURCE=..\IO\np2vasup.h
# End Source File
# Begin Source File

SOURCE=..\VRAMVA\palettesva.h
# End Source File
# Begin Source File

SOURCE=..\VRAMVA\scrndrawva.h
# End Source File
# Begin Source File

SOURCE=..\VRAMVA\sdrawva.h
# End Source File
# Begin Source File

SOURCE=..\IOVA\sgp.h
# End Source File
# Begin Source File

SOURCE=..\IOVA\subsystem.h
# End Source File
# Begin Source File

SOURCE=..\IOVA\tsp.h
# End Source File
# Begin Source File

SOURCE=..\IOVA\videova.h
# End Source File
# Begin Source File

SOURCE=.\debuguty\viewvideova.h
# End Source File
# Begin Source File

SOURCE=..\CPUCVA\z80if.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\icons\nekop2.bmp
# End Source File
# Begin Source File

SOURCE=.\icons\np2.ico
# End Source File
# Begin Source File

SOURCE=.\icons\np2debug.ico
# End Source File
# Begin Source File

SOURCE=.\icons\np2tool.bmp
# End Source File
# Begin Source File

SOURCE=.\icons\np2tool2.bmp
# End Source File
# End Group
# Begin Source File

SOURCE=.\icons\fddseek.wav
# End Source File
# Begin Source File

SOURCE=.\icons\fddseek1.wav
# End Source File
# End Target
# End Project
