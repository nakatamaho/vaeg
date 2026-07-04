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
# ADD CPP /nologo /W3 /GX /O2 /I ".\\" /I ".\x86" /I ".\dialog" /I ".\debuguty" /I "..\\" /I "..\common" /I "..\i286x" /I "..\io" /I "..\cbus" /I "..\bios" /I "..\vram" /I "..\sound" /I "..\sound\vermouth" /I "..\sound\getsnd" /I "..\fdd" /I "..\lio" /I "..\font" /I "..\generic" /I "..\cpuxva" /I "..\cpucva" /I "../biosva" /I "../iova" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /FAcs /YX /FD /c
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
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I ".\\" /I ".\x86" /I ".\dialog" /I ".\debuguty" /I "..\\" /I "..\common" /I "..\i286x" /I "..\io" /I "..\cbus" /I "..\bios" /I "..\vram" /I "..\sound" /I "..\sound\vermouth" /I "..\sound\getsnd" /I "..\fdd" /I "..\lio" /I "..\font" /I "..\generic" /I "..\cpuxva" /I "..\cpucva" /I "../biosva" /I "../iova" /D "_DEBUG" /D "TRACE" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "CPUDEBUG" /FR /YX /FD /GZ /c
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
InputName=parts

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	c:\bin\nasm\nasmw -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 Trace"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\trc
InputPath=.\x86\parts.x86
InputName=parts

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	c:\bin\nasm\nasmw -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 WaveRec"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\wr
InputPath=.\x86\parts.x86
InputName=parts

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	c:\bin\nasm\nasmw -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 Debug"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\dbg
InputPath=.\x86\parts.x86
InputName=parts

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	c:\bin\nasm\nasmw -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

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
InputName=dmap

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	c:\bin\nasm\nasmw -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 Trace"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\trc
InputPath=..\i286x\dmap.x86
InputName=dmap

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	c:\bin\nasm\nasmw -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 WaveRec"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\wr
InputPath=..\i286x\dmap.x86
InputName=dmap

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	c:\bin\nasm\nasmw -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 Debug"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\dbg
InputPath=..\i286x\dmap.x86
InputName=dmap

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	c:\bin\nasm\nasmw -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\i286x\egcmem.x86

!IF  "$(CFG)" == "np2 - Win32 Release"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\rel
InputPath=..\i286x\egcmem.x86
InputName=egcmem

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	c:\bin\nasm\nasmw -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 Trace"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\trc
InputPath=..\i286x\egcmem.x86
InputName=egcmem

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	c:\bin\nasm\nasmw -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 WaveRec"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\wr
InputPath=..\i286x\egcmem.x86
InputName=egcmem

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	c:\bin\nasm\nasmw -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 Debug"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\dbg
InputPath=..\i286x\egcmem.x86
InputName=egcmem

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	c:\bin\nasm\nasmw -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

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
InputName=memory

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	c:\bin\nasm\nasmw -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 Trace"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\trc
InputPath=..\i286x\memory.x86
InputName=memory

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	c:\bin\nasm\nasmw -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 WaveRec"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\wr
InputPath=..\i286x\memory.x86
InputName=memory

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	c:\bin\nasm\nasmw -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 Debug"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\dbg
InputPath=..\i286x\memory.x86
InputName=memory

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	c:\bin\nasm\nasmw -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

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
InputName=opngeng

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	c:\bin\nasm\nasmw -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 Trace"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\trc
InputPath=.\x86\opngeng.x86
InputName=opngeng

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	c:\bin\nasm\nasmw -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj -l $(IntDir)\$(InputName).cod

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 WaveRec"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\wr
InputPath=.\x86\opngeng.x86
InputName=opngeng

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	c:\bin\nasm\nasmw -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj -l $(IntDir)\$(InputName).cod

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 Debug"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\dbg
InputPath=.\x86\opngeng.x86
InputName=opngeng

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	c:\bin\nasm\nasmw -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

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
InputName=cputype

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	c:\bin\nasm\nasmw -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 Trace"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\trc
InputPath=.\x86\cputype.x86
InputName=cputype

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	c:\bin\nasm\nasmw -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 WaveRec"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\wr
InputPath=.\x86\cputype.x86
InputName=cputype

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	c:\bin\nasm\nasmw -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 Debug"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\dbg
InputPath=.\x86\cputype.x86
InputName=cputype

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	c:\bin\nasm\nasmw -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

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
InputName=dclockd

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	c:\bin\nasm\nasmw -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 Trace"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\trc
InputPath=.\dclockd.x86
InputName=dclockd

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	c:\bin\nasm\nasmw -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 WaveRec"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\wr
InputPath=.\dclockd.x86
InputName=dclockd

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	c:\bin\nasm\nasmw -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 Debug"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\dbg
InputPath=.\dclockd.x86
InputName=dclockd

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	c:\bin\nasm\nasmw -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

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

SOURCE=..\io\artic.c
# End Source File
# Begin Source File

SOURCE=..\io\bmsio.c
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

SOURCE=..\io\np2vasup.c
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
# Begin Group "CBUS"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\cbus\amd98.c
# End Source File
# Begin Source File

SOURCE=.\board118.c
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

SOURCE=..\cbus\ideio.c
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
# Begin Group "vram"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\vram\dispsync.c
# End Source File
# Begin Source File

SOURCE=.\x86\makegrph.x86

!IF  "$(CFG)" == "np2 - Win32 Release"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\rel
InputPath=.\x86\makegrph.x86
InputName=makegrph

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	c:\bin\nasm\nasmw -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 Trace"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\trc
InputPath=.\x86\makegrph.x86
InputName=makegrph

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	c:\bin\nasm\nasmw -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 WaveRec"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\wr
InputPath=.\x86\makegrph.x86
InputName=makegrph

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	c:\bin\nasm\nasmw -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 Debug"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\dbg
InputPath=.\x86\makegrph.x86
InputName=makegrph

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	c:\bin\nasm\nasmw -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ENDIF 

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

SOURCE=..\vram\scrndraw.c
# End Source File
# Begin Source File

SOURCE=..\vram\scrnsave.c
# End Source File
# Begin Source File

SOURCE=..\vram\sdraw.c
# End Source File
# Begin Source File

SOURCE=..\vram\vram.c
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

SOURCE=..\vramva\makegrphva.c
# End Source File
# Begin Source File

SOURCE=..\vramva\makesprva.c
# End Source File
# Begin Source File

SOURCE=..\vramva\maketextva.c
# End Source File
# Begin Source File

SOURCE=..\vramva\palettesva.c
# End Source File
# Begin Source File

SOURCE=..\vramva\scrndrawva.c
# End Source File
# Begin Source File

SOURCE=..\vramva\sdrawva.c
# End Source File
# End Group
# Begin Group "cpuxva"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\cpuxva\memoryva.x86

!IF  "$(CFG)" == "np2 - Win32 Release"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\rel
InputPath=..\cpuxva\memoryva.x86
InputName=memoryVA

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	c:\bin\nasm\nasmw -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ELSEIF  "$(CFG)" == "np2 - Win32 Trace"

!ELSEIF  "$(CFG)" == "np2 - Win32 WaveRec"

!ELSEIF  "$(CFG)" == "np2 - Win32 Debug"

# Begin Custom Build - ｱｾﾝﾌﾞﾙ中... $(InputPath)
IntDir=.\..\obj\dbg
InputPath=..\cpuxva\memoryva.x86
InputName=memoryVA

"$(IntDir)\$(InputName).obj" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	c:\bin\nasm\nasmw -f win32 $(InputPath) -o $(IntDir)\$(InputName).obj

# End Custom Build

!ENDIF 

# End Source File
# End Group
# Begin Group "biosva"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\biosva\biosva.c
# End Source File
# End Group
# Begin Group "IOVA"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\iova\bkupmemva.c
# End Source File
# Begin Source File

SOURCE=..\iova\boardsb2.c
# End Source File
# Begin Source File

SOURCE=..\iova\cgromva.c
# End Source File
# Begin Source File

SOURCE=..\iova\fdsubsys.c
# End Source File
# Begin Source File

SOURCE=..\iova\gactrlva.c
# End Source File
# Begin Source File

SOURCE=..\iova\i8255.c
# End Source File
# Begin Source File

SOURCE=..\iova\iocoreva.c
# End Source File
# Begin Source File

SOURCE=..\iova\memctrlva.c
# End Source File
# Begin Source File

SOURCE=..\iova\mouseifva.c
# End Source File
# Begin Source File

SOURCE=..\iova\sgp.c
# End Source File
# Begin Source File

SOURCE=..\iova\subsystem.cpp
# End Source File
# Begin Source File

SOURCE=..\iova\subsystemif.c
# End Source File
# Begin Source File

SOURCE=..\iova\subsystemmx.c
# End Source File
# Begin Source File

SOURCE=..\iova\sysportva.c
# End Source File
# Begin Source File

SOURCE=..\iova\tsp.c
# End Source File
# Begin Source File

SOURCE=..\iova\upd9002.c
# End Source File
# Begin Source File

SOURCE=..\iova\va91.c
# End Source File
# Begin Source File

SOURCE=..\iova\videova.c
# End Source File
# End Group
# Begin Group "cpucva"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\cpucva\gvramva.c
# End Source File
# Begin Source File

SOURCE=..\cpucva\z80c.cpp
# End Source File
# Begin Source File

SOURCE=..\cpucva\z80diag.cpp
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

SOURCE=..\iova\fdsubsys.h
# End Source File
# Begin Source File

SOURCE=..\iova\gaccess.h
# End Source File
# Begin Source File

SOURCE=..\vramva\makesprva.h
# End Source File
# Begin Source File

SOURCE=..\io\np2vasup.h
# End Source File
# Begin Source File

SOURCE=..\vramva\palettesva.h
# End Source File
# Begin Source File

SOURCE=..\vramva\scrndrawva.h
# End Source File
# Begin Source File

SOURCE=..\vramva\sdrawva.h
# End Source File
# Begin Source File

SOURCE=..\iova\sgp.h
# End Source File
# Begin Source File

SOURCE=..\iova\subsystem.h
# End Source File
# Begin Source File

SOURCE=..\iova\tsp.h
# End Source File
# Begin Source File

SOURCE=..\iova\videova.h
# End Source File
# Begin Source File

SOURCE=.\debuguty\viewvideova.h
# End Source File
# Begin Source File

SOURCE=..\cpucva\z80if.h
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
