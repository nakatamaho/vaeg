# Phase 1.4 Visual Studio Project Inventory

Generated from the checked-in Visual Studio project files after Phase 1.3 rename normalization. No project files are deleted in this phase.

## Repository State

- HEAD: `76b4e15 Fix VC2008 UTF-8 source handling`
- Branch: `main`
- Intent: preserve old project files as build/source-list ledgers until a replacement build can generate an equivalent `vaeg.exe`.
- Windows build gate before this inventory: Debug and Release Win32 passed; demo and PC-Engine were run successfully by the owner.
- Untracked instruction files intentionally excluded from this phase: `AGENTS.md`, `refactor-instructions.md`.

## Scope

Tracked Visual Studio files found:

- `Win9x/np2.dsp`
- `Win9xC/np2c.dsp`
- `accessories/bin2txt.dsp`
- `accessories/lzxpack.dsp`
- `sdl/sdlw32s.dsp`
- `Win9x/np2.dsw`
- `Win9xC/np2c.dsw`
- `accessories/bin2txt.dsw`
- `accessories/lzxpack.dsw`
- `sdl/sdlw32s.dsw`
- `Win9x/np2.vcproj`

No `.sln` files are tracked. No `M88_2008.sln` file is tracked. The only tracked `.vcproj` is `Win9x/np2.vcproj`, which is the current VC2008 build gate.

## Classification

| Project | Type | Build role | Keep/delete decision |
|---|---|---|---|
| `Win9x/np2.dsp` | VS6 Win32 application | Primary vaeg ledger. Lists VA sources, `CPUXVA`, `CPUCVA`, `BIOSVA`, `IOVA`, NASM custom builds, and `vaeg.exe`/`vaegd.exe` outputs. | Keep. Do not delete until an equivalent replacement build is verified. |
| `Win9x/np2.vcproj` | VC2008 Win32 application | Current Windows build gate derived from the Win9x project. Debug and Release are verified after Phase 1.3. | Keep. This is active verification infrastructure. |
| `Win9xC/np2c.dsp` | VS6 Win32 application | Alternate C-core Windows frontend using `i286c`; no VA source set observed. | Keep as legacy ledger. |
| `sdl/sdlw32s.dsp` | VS6 Win32 console application | SDL/Win32s-era NP2 frontend using `i286c` and `embed`; no VA source set observed. | Keep as legacy ledger. |
| `accessories/bin2txt.dsp` | VS6 Win32 console tool | Accessory tool build path. | Keep as legacy tool ledger. |
| `accessories/lzxpack.dsp` | VS6 Win32 console tool | Accessory compression tool build path. | Keep as legacy tool ledger. |

## DSW Project Links

### `Win9x/np2.dsw`

- `np2 => .\np2.dsp - Package Owner=<4>`

### `Win9xC/np2c.dsw`

- `np2c => .\np2c.dsp - Package Owner=<4>`

### `accessories/bin2txt.dsw`

- `bin2txt => .\bin2txt.dsp - Package Owner=<4>`

### `accessories/lzxpack.dsw`

- `lzxpack => .\lzxpack.dsp - Package Owner=<4>`

### `sdl/sdlw32s.dsw`

- `sdlw32s => .\sdlw32s.dsp - Package Owner=<4>`

## DSP Inventories

### `Win9x/np2.dsp`

- Project name: `np2`
- Target type: `Win32 (x86) Application`
- Configurations: `np2 - Win32 Release`, `np2 - Win32 Trace`, `np2 - Win32 WaveRec`, `np2 - Win32 Debug`
- Source entries: `261`
- Custom build entries: `9`

Include paths:

- `../BIOSVA`
- `../IOVA`
- `..\CPUCVA`
- `..\CPUXVA`
- `..\\`
- `..\bios`
- `..\cbus`
- `..\common`
- `..\fdd`
- `..\font`
- `..\generic`
- `..\i286x`
- `..\io`
- `..\lio`
- `..\sound`
- `..\sound\getsnd`
- `..\sound\vermouth`
- `..\vram`
- `.\\`
- `.\debuguty`
- `.\dialog`
- `.\x86`
Preprocessor defines:

- `CPUDEBUG`
- `MEMTRACE`
- `NDEBUG`
- `SUPPORT_WAVEREC`
- `TRACE`
- `WIN32`
- `_DEBUG`
- `_MBCS`
- `_WINDOWS`

Resource defines:

- `NDEBUG`
- `_DEBUG`

Link libraries:

- `DSOUND.LIB`
- `advapi32.lib`
- `comctl32.lib`
- `comdlg32.lib`
- `ddraw.lib`
- `dxguid.lib`
- `gdi32.lib`
- `kernel32.lib`
- `odbc32.lib`
- `odbccp32.lib`
- `ole32.lib`
- `oleaut32.lib`
- `shell32.lib`
- `user32.lib`
- `uuid.lib`
- `winmm.lib`
- `winspool.lib`
- `wsock32.lib`

Output paths:

- `..\bin/np2t.exe`
- `..\bin/np2wr.exe`
- `..\bin/vaeg.exe`
- `..\bin/vaegd.exe`

Source list:

```text
..\common\_memory.c
..\common\bmpdata.c
..\common\lstarray.c
..\common\milstr.c
..\common\mimpidef.c
.\x86\parts.x86
..\common\profile.c
..\common\strres.c
..\common\textfile.c
..\common\wavefile.c
..\i286x\dmap.x86
..\i286x\egcmem.x86
..\i286x\i286x.cpp
..\i286x\i286xadr.cpp
..\i286x\i286xcts.cpp
..\i286x\i286xrep.cpp
..\i286x\i286xs.cpp
..\i286x\memory.x86
..\i286x\v30patch.cpp
..\BIOS\bios.c
..\BIOS\bios09.c
..\BIOS\bios0c.c
..\BIOS\bios12.c
..\BIOS\bios13.c
..\BIOS\bios18.c
..\BIOS\bios19.c
..\BIOS\bios1a.c
..\BIOS\bios1b.c
..\BIOS\bios1c.c
..\BIOS\bios1f.c
..\BIOS\sxsibios.c
..\sound\vermouth\midimod.c
..\sound\vermouth\midinst.c
..\sound\vermouth\midiout.c
..\sound\vermouth\midtable.c
..\sound\vermouth\midvoice.c
..\sound\getsnd\getsmix.c
..\sound\getsnd\getsnd.c
..\sound\getsnd\getwave.c
..\sound\adpcmc.c
..\sound\adpcmg.c
..\sound\beepc.c
..\sound\beepg.c
..\sound\cs4231c.c
..\sound\cs4231g.c
..\sound\fmboard.c
..\sound\fmtimer.c
..\sound\opngenc.c
.\x86\opngeng.x86
..\sound\pcm86c.c
..\sound\pcm86g.c
..\sound\psggenc.c
..\sound\psggeng.c
..\sound\rhythmc.c
..\sound\s98.c
..\sound\sound.c
..\sound\soundrom.c
..\sound\tms3631c.c
..\sound\tms3631g.c
..\fdd\diskdrv.c
..\fdd\fdd_d88.c
..\fdd\fdd_mtr.c
..\fdd\fdd_xdf.c
..\fdd\fddfile.c
..\fdd\newdisk.c
..\fdd\sxsi.c
..\lio\gcircle.c
..\lio\gline.c
..\lio\gpset.c
..\lio\gput1.c
..\lio\gscreen.c
..\lio\lio.c
..\font\font.c
..\font\fontdata.c
..\font\fontfm7.c
..\font\fontmake.c
..\font\fontpc88.c
..\font\fontpc98.c
..\font\fontv98.c
..\font\fontx1.c
..\font\fontx68k.c
.\dialog\d_about.cpp
.\dialog\d_bmp.cpp
.\dialog\d_bms.cpp
.\dialog\d_clnd.cpp
.\dialog\d_config.cpp
.\dialog\d_disk.cpp
.\dialog\d_mpu98.cpp
.\dialog\d_oprecord.cpp
.\dialog\d_screen.cpp
.\dialog\d_serial.cpp
.\dialog\d_sound.cpp
.\dialog\dialogs.cpp
.\dialog\np2class.cpp
.\debuguty\debugctrl.cpp
.\debuguty\view1mb.cpp
.\debuguty\viewasm.cpp
.\debuguty\viewcmn.cpp
.\debuguty\viewer.cpp
.\debuguty\viewgactrlva.cpp
.\debuguty\viewmem.cpp
.\debuguty\viewmenu.cpp
.\debuguty\viewreg.cpp
.\debuguty\viewseg.cpp
.\debuguty\viewsnd.cpp
.\debuguty\viewsubasm.cpp
.\debuguty\viewsubmem.cpp
.\debuguty\viewsubreg.cpp
.\debuguty\viewvabank.cpp
.\debuguty\viewvideova.cpp
.\cmmidi.cpp
.\cmpara.cpp
.\cmserial.cpp
.\commng.cpp
.\x86\cputype.x86
.\dclock.cpp
.\dclockd.x86
.\dd2.cpp
.\dosio.cpp
.\extromio.cpp
.\fontmng.cpp
.\ini.cpp
.\joymng.cpp
.\juliet.cpp
.\menu.cpp
.\mousemng.cpp
.\np2.cpp
.\np2.rc
.\np2arg.cpp
.\scrnmng.cpp
.\soundmng.cpp
.\sstp.cpp
.\sstpmsg.cpp
.\subwind.cpp
.\sysmng.cpp
.\taskmng.cpp
.\timemng.cpp
.\toolwin.cpp
.\trace.cpp
.\winkbd.cpp
.\winloc.cpp
..\IO\artic.c
..\IO\bmsio.c
..\IO\cgrom.c
..\IO\cpuio.c
..\IO\crtc.c
..\IO\dipsw.c
..\IO\dmac.c
..\IO\egc.c
..\IO\emsio.c
..\IO\epsonio.c
..\IO\fdc.c
..\IO\fdd320.c
..\IO\gdc.c
..\IO\gdc_pset.c
..\IO\gdc_sub.c
..\IO\iocore.c
..\IO\mouseif.c
..\IO\necio.c
..\IO\nmiio.c
..\IO\np2sysp.c
..\IO\np2vasup.c
..\IO\pic.c
..\IO\pit.c
..\IO\printif.c
..\IO\serial.c
..\IO\sysport.c
..\IO\upd4990.c
..\CBUS\amd98.c
.\board118.c
..\CBUS\board14.c
..\CBUS\board26k.c
..\CBUS\board86.c
..\CBUS\boardspb.c
..\CBUS\boardx2.c
..\CBUS\cbuscore.c
..\CBUS\cs4231io.c
..\CBUS\ideio.c
..\CBUS\mpu98ii.c
..\CBUS\pc9861k.c
..\CBUS\pcm86io.c
..\CBUS\sasiio.c
..\CBUS\scsicmd.c
..\CBUS\scsiio.c
..\VRAM\dispsync.c
.\x86\makegrph.x86
..\VRAM\maketext.c
..\VRAM\maketgrp.c
..\VRAM\palettes.c
..\VRAM\scrndraw.c
..\VRAM\scrnsave.c
..\VRAM\sdraw.c
..\VRAM\vram.c
..\generic\cmjasts.c
..\generic\cmndraw.c
..\generic\dipswbmp.c
..\generic\hostdrv.c
..\generic\hostdrvs.c
..\generic\keydisp.c
..\generic\np2info.c
..\generic\softkbd.c
..\generic\unasm.c
..\VRAMVA\makegrphva.c
..\VRAMVA\makesprva.c
..\VRAMVA\maketextva.c
..\VRAMVA\palettesva.c
..\VRAMVA\scrndrawva.c
..\VRAMVA\sdrawva.c
..\CPUXVA\memoryva.x86
..\BIOSVA\biosva.c
..\IOVA\bkupmemva.c
..\IOVA\boardsb2.c
..\IOVA\cgromva.c
..\IOVA\fdsubsys.c
..\IOVA\gactrlva.c
..\IOVA\i8255.c
..\IOVA\iocoreva.c
..\IOVA\memctrlva.c
..\IOVA\mouseifva.c
..\IOVA\sgp.c
..\IOVA\subsystem.cpp
..\IOVA\subsystemif.c
..\IOVA\subsystemmx.c
..\IOVA\sysportva.c
..\IOVA\tsp.c
..\IOVA\upd9002.c
..\IOVA\va91.c
..\IOVA\videova.c
..\CPUCVA\GVRAMVA.c
..\CPUCVA\z80c.cpp
..\CPUCVA\z80diag.cpp
..\breakpoint.c
..\calendar.c
..\debugsub.c
..\keystat.c
..\nevent.c
..\oprecord.c
..\pccore.c
..\statsave.c
..\timing.c
..\breakpoint.h
..\IOVA\fdsubsys.h
..\IOVA\GACCESS.H
..\VRAMVA\makesprva.h
..\IO\np2vasup.h
..\VRAMVA\palettesva.h
..\VRAMVA\scrndrawva.h
..\VRAMVA\sdrawva.h
..\IOVA\sgp.h
..\IOVA\subsystem.h
..\IOVA\tsp.h
..\IOVA\videova.h
.\debuguty\viewvideova.h
..\CPUCVA\z80if.h
.\icons\nekop2.bmp
.\icons\np2.ico
.\icons\np2debug.ico
.\icons\np2tool.bmp
.\icons\np2tool2.bmp
.\icons\fddseek.wav
.\icons\fddseek1.wav
```

Custom build steps:

- Source: `.\x86\parts.x86`
  Command: not parsed from legacy block.

- Source: `..\i286x\dmap.x86`
  Command: not parsed from legacy block.

- Source: `..\i286x\egcmem.x86`
  Command: not parsed from legacy block.

- Source: `..\i286x\memory.x86`
  Command: not parsed from legacy block.

- Source: `.\x86\opngeng.x86`
  Command: not parsed from legacy block.

- Source: `.\x86\cputype.x86`
  Command: not parsed from legacy block.

- Source: `.\dclockd.x86`
  Command: not parsed from legacy block.

- Source: `.\x86\makegrph.x86`
  Command: not parsed from legacy block.

- Source: `..\CPUXVA\memoryva.x86`
  Command: not parsed from legacy block.

### `Win9xC/np2c.dsp`

- Project name: `np2c`
- Target type: `Win32 (x86) Application`
- Configurations: `np2c - Win32 Release`, `np2c - Win32 Debug`
- Source entries: `169`
- Custom build entries: `0`

Include paths:

- `..\\`
- `..\bios`
- `..\cbus`
- `..\common`
- `..\fdd`
- `..\font`
- `..\generic`
- `..\i286c`
- `..\io`
- `..\lio`
- `..\sound`
- `..\sound\getsnd`
- `..\sound\vermouth`
- `..\vram`
- `.\\`
- `.\dialog`
- `.\x86`

Preprocessor defines:

- `NDEBUG`
- `TRACE`
- `WIN32`
- `_DEBUG`
- `_MBCS`
- `_WINDOWS`

Resource defines:

- `NDEBUG`
- `_DEBUG`

Link libraries:

- `DSOUND.LIB`
- `advapi32.lib`
- `comctl32.lib`
- `comdlg32.lib`
- `ddraw.lib`
- `dxguid.lib`
- `gdi32.lib`
- `kernel32.lib`
- `odbc32.lib`
- `odbccp32.lib`
- `ole32.lib`
- `oleaut32.lib`
- `shell32.lib`
- `user32.lib`
- `uuid.lib`
- `winmm.lib`
- `winspool.lib`
- `wsock32.lib`

Output paths:

- `..\bin/np2cd.exe`

Source list:

```text
..\common\bmpdata.c
..\common\lstarray.c
..\common\milstr.c
..\common\parts.c
..\common\profile.c
..\common\strres.c
..\common\textfile.c
..\i286c\dmap.c
..\i286c\egcmem.c
..\i286c\i286c.c
..\i286c\i286c_0f.c
..\i286c\i286c_8x.c
..\i286c\i286c_ea.c
..\i286c\i286c_f6.c
..\i286c\i286c_fe.c
..\i286c\i286c_mn.c
..\i286c\i286c_rp.c
..\i286c\i286c_sf.c
..\i286c\memory.c
..\i286c\v30patch.c
..\BIOS\bios.c
..\BIOS\bios09.c
..\BIOS\bios0c.c
..\BIOS\bios12.c
..\BIOS\bios13.c
..\BIOS\bios18.c
..\BIOS\bios19.c
..\BIOS\bios1a.c
..\BIOS\bios1b.c
..\BIOS\bios1c.c
..\BIOS\bios1f.c
..\BIOS\sxsibios.c
..\sound\vermouth\midimod.c
..\sound\vermouth\midinst.c
..\sound\vermouth\midiout.c
..\sound\vermouth\midtable.c
..\sound\vermouth\midvoice.c
..\sound\getsnd\getsmix.c
..\sound\getsnd\getsnd.c
..\sound\getsnd\getwave.c
..\sound\adpcmc.c
..\sound\adpcmg.c
..\sound\beepc.c
..\sound\beepg.c
..\sound\cs4231c.c
..\sound\cs4231g.c
..\sound\fmboard.c
..\sound\fmtimer.c
..\sound\opngenc.c
..\sound\opngeng.c
..\sound\pcm86c.c
..\sound\pcm86g.c
..\sound\psggenc.c
..\sound\psggeng.c
..\sound\rhythmc.c
..\sound\s98.c
..\sound\sound.c
..\sound\soundrom.c
..\sound\tms3631c.c
..\sound\tms3631g.c
..\fdd\diskdrv.c
..\fdd\fdd_d88.c
..\fdd\fdd_mtr.c
..\fdd\fdd_xdf.c
..\fdd\fddfile.c
..\fdd\newdisk.c
..\fdd\sxsi.c
..\lio\gcircle.c
..\lio\gline.c
..\lio\gpset.c
..\lio\gput1.c
..\lio\gscreen.c
..\lio\lio.c
..\font\font.c
..\font\fontdata.c
..\font\fontfm7.c
..\font\fontmake.c
..\font\fontpc88.c
..\font\fontpc98.c
..\font\fontv98.c
..\font\fontx1.c
..\font\fontx68k.c
.\dialog\d_about.cpp
.\dialog\d_bmp.cpp
.\dialog\d_clnd.cpp
.\dialog\d_config.cpp
.\dialog\d_disk.cpp
.\dialog\d_mpu98.cpp
.\dialog\d_screen.cpp
.\dialog\d_sound.cpp
.\dialog\dialogs.cpp
.\commng.cpp
.\dosio.cpp
.\fontmng.cpp
.\ini.cpp
.\joymng.cpp
.\menu.cpp
.\mousemng.cpp
.\np2.cpp
.\np2.rc
.\np2arg.cpp
.\scrnmng.cpp
.\soundmng.cpp
.\sysmng.cpp
.\taskmng.cpp
.\timemng.cpp
.\trace.cpp
.\winkbd.cpp
..\IO\artic.c
..\IO\cgrom.c
..\IO\cpuio.c
..\IO\crtc.c
..\IO\dipsw.c
..\IO\dmac.c
..\IO\egc.c
..\IO\emsio.c
..\IO\epsonio.c
..\IO\fdc.c
..\IO\fdd320.c
..\IO\gdc.c
..\IO\gdc_pset.c
..\IO\gdc_sub.c
..\IO\iocore.c
..\IO\mouseif.c
..\IO\necio.c
..\IO\nmiio.c
..\IO\np2sysp.c
..\IO\pic.c
..\IO\pit.c
..\IO\printif.c
..\IO\serial.c
..\IO\sysport.c
..\IO\upd4990.c
..\CBUS\amd98.c
..\CBUS\board118.c
..\CBUS\board14.c
..\CBUS\board26k.c
..\CBUS\board86.c
..\CBUS\boardspb.c
..\CBUS\boardx2.c
..\CBUS\cbuscore.c
..\CBUS\cs4231io.c
..\CBUS\mpu98ii.c
..\CBUS\pc9861k.c
..\CBUS\pcm86io.c
..\VRAM\dispsync.c
..\VRAM\makegrph.c
..\VRAM\maketext.c
..\VRAM\maketgrp.c
..\VRAM\palettes.c
..\VRAM\scrnbmp.c
..\VRAM\scrndraw.c
..\VRAM\sdraw.c
..\VRAM\vram.c
..\generic\cmjasts.c
..\generic\cmver.c
..\generic\dipswbmp.c
..\generic\hostdrv.c
..\generic\hostdrvs.c
..\generic\np2info.c
..\calendar.c
..\debugsub.c
..\keystat.c
..\nevent.c
..\pccore.c
..\statsave.c
..\timing.c
.\icons\np2.ico
.\icons\np2debug.ico
```

### `accessories/bin2txt.dsp`

- Project name: `bin2txt`
- Target type: `Win32 (x86) Console Application`
- Configurations: `bin2txt - Win32 Release`
- Source entries: `4`
- Custom build entries: `0`

Include paths:

- `..\\`
- `..\common`
- `..\win9x`
- `.\\`

Preprocessor defines:

- `NDEBUG`
- `WIN32`
- `_CONSOLE`
- `_MBCS`

Resource defines:

- `NDEBUG`

Link libraries:

- `advapi32.lib`
- `comdlg32.lib`
- `gdi32.lib`
- `kernel32.lib`
- `odbc32.lib`
- `odbccp32.lib`
- `ole32.lib`
- `oleaut32.lib`
- `shell32.lib`
- `user32.lib`
- `uuid.lib`
- `winspool.lib`

Output paths:

- none

Source list:

```text
..\common\milstr.c
..\Win9x\dosio.cpp
.\bin2txt.c
.\textout.c
```

### `accessories/lzxpack.dsp`

- Project name: `lzxpack`
- Target type: `Win32 (x86) Console Application`
- Configurations: `lzxpack - Win32 Release`
- Source entries: `4`
- Custom build entries: `0`

Include paths:

- `..\\`
- `..\common`
- `..\win9x`
- `.\\`

Preprocessor defines:

- `NDEBUG`
- `WIN32`
- `_CONSOLE`
- `_MBCS`

Resource defines:

- `NDEBUG`

Link libraries:

- `advapi32.lib`
- `comdlg32.lib`
- `gdi32.lib`
- `kernel32.lib`
- `odbc32.lib`
- `odbccp32.lib`
- `ole32.lib`
- `oleaut32.lib`
- `shell32.lib`
- `user32.lib`
- `uuid.lib`
- `winspool.lib`

Output paths:

- none

Source list:

```text
..\common\milstr.c
..\Win9x\dosio.cpp
.\lzxpack.c
.\textout.c
```

### `sdl/sdlw32s.dsp`

- Project name: `sdlw32s`
- Target type: `Win32 (x86) Console Application`
- Configurations: `sdlw32s - Win32 Release`, `sdlw32s - Win32 Trace`, `sdlw32s - Win32 Debug`
- Source entries: `186`
- Custom build entries: `0`

Include paths:

- `..\\`
- `..\bios`
- `..\cbus`
- `..\common`
- `..\embed`
- `..\embed\menu`
- `..\embed\menubase`
- `..\fdd`
- `..\font`
- `..\generic`
- `..\i286c`
- `..\io`
- `..\lio`
- `..\sound`
- `..\sound\getsnd`
- `..\sound\vermouth`
- `..\vram`
- `.\\`
- `.\win32s`

Preprocessor defines:

- `MEMTRACE`
- `NDEBUG`
- `RESOURCE_US`
- `SIZE_VGA`
- `TRACE`
- `WIN32`
- `_CONSOLE`
- `_DEBUG`
- `_MBCS`

Resource defines:

- `NDEBUG`
- `_DEBUG`

Link libraries:

- `advapi32.lib`
- `comdlg32.lib`
- `gdi32.lib`
- `kernel32.lib`
- `odbc32.lib`
- `odbccp32.lib`
- `ole32.lib`
- `oleaut32.lib`
- `shell32.lib`
- `user32.lib`
- `uuid.lib`
- `winspool.lib`

Output paths:

- `..\bin\sdl/np2.exe`
- `..\bin\sdl/np2d.exe`
- `..\bin\sdl/np2t.exe`

Source list:

```text
..\common\_memory.c
..\common\bmpdata.c
..\common\codecnv.c
..\common\lstarray.c
..\common\milstr.c
..\common\mimpidef.c
..\common\parts.c
..\common\profile.c
..\common\rect.c
..\common\resize.c
..\common\strres.c
..\common\textfile.c
..\common\wavefile.c
..\i286c\dmap.c
..\i286c\egcmem.c
..\i286c\i286c.c
..\i286c\i286c_0f.c
..\i286c\i286c_8x.c
..\i286c\i286c_ea.c
..\i286c\i286c_f6.c
..\i286c\i286c_fe.c
..\i286c\i286c_mn.c
..\i286c\i286c_rp.c
..\i286c\i286c_sf.c
..\i286c\memory.c
..\i286c\v30patch.c
..\IO\artic.c
..\IO\cgrom.c
..\IO\cpuio.c
..\IO\crtc.c
..\IO\dipsw.c
..\IO\dmac.c
..\IO\egc.c
..\IO\emsio.c
..\IO\epsonio.c
..\IO\fdc.c
..\IO\fdd320.c
..\IO\gdc.c
..\IO\gdc_pset.c
..\IO\gdc_sub.c
..\IO\iocore.c
..\IO\mouseif.c
..\IO\necio.c
..\IO\nmiio.c
..\IO\np2sysp.c
..\IO\pic.c
..\IO\pit.c
..\IO\printif.c
..\IO\serial.c
..\IO\sysport.c
..\IO\upd4990.c
..\CBUS\amd98.c
..\CBUS\board118.c
..\CBUS\board14.c
..\CBUS\board26k.c
..\CBUS\board86.c
..\CBUS\boardspb.c
..\CBUS\boardx2.c
..\CBUS\cbuscore.c
..\CBUS\cs4231io.c
..\CBUS\mpu98ii.c
..\CBUS\pc9861k.c
..\CBUS\pcm86io.c
..\CBUS\sasiio.c
..\CBUS\scsicmd.c
..\CBUS\scsiio.c
..\BIOS\bios.c
..\BIOS\bios09.c
..\BIOS\bios0c.c
..\BIOS\bios12.c
..\BIOS\bios13.c
..\BIOS\bios18.c
..\BIOS\bios19.c
..\BIOS\bios1a.c
..\BIOS\bios1b.c
..\BIOS\bios1c.c
..\BIOS\bios1f.c
..\BIOS\sxsibios.c
..\VRAM\dispsync.c
..\VRAM\makegrph.c
..\VRAM\maketext.c
..\VRAM\maketgrp.c
..\VRAM\palettes.c
..\VRAM\scrnbmp.c
..\VRAM\scrndraw.c
..\VRAM\sdraw.c
..\VRAM\sdrawq16.c
..\VRAM\vram.c
..\sound\vermouth\midimod.c
..\sound\vermouth\midinst.c
..\sound\vermouth\midiout.c
..\sound\vermouth\midtable.c
..\sound\vermouth\midvoice.c
..\sound\getsnd\getsmix.c
..\sound\getsnd\getsnd.c
..\sound\getsnd\getwave.c
..\sound\adpcmc.c
..\sound\adpcmg.c
..\sound\beepc.c
..\sound\beepg.c
..\sound\cs4231c.c
..\sound\cs4231g.c
..\sound\fmboard.c
..\sound\fmtimer.c
..\sound\opngenc.c
..\sound\opngeng.c
..\sound\pcm86c.c
..\sound\pcm86g.c
..\sound\psggenc.c
..\sound\psggeng.c
..\sound\rhythmc.c
..\sound\s98.c
..\sound\sndcsec.c
..\sound\sound.c
..\sound\soundrom.c
..\sound\tms3631c.c
..\sound\tms3631g.c
..\fdd\diskdrv.c
..\fdd\fdd_d88.c
..\fdd\fdd_mtr.c
..\fdd\fdd_xdf.c
..\fdd\fddfile.c
..\fdd\newdisk.c
..\fdd\sxsi.c
..\lio\gcircle.c
..\lio\gline.c
..\lio\gpset.c
..\lio\gput1.c
..\lio\gscreen.c
..\lio\lio.c
..\font\font.c
..\font\fontdata.c
..\font\fontfm7.c
..\font\fontmake.c
..\font\fontpc88.c
..\font\fontpc98.c
..\font\fontv98.c
..\font\fontx1.c
..\font\fontx68k.c
.\win32s\sdl.c
.\win32s\sdl_ttf.c
.\win32s\sdlaudio.c
.\win32s\sdlevent.c
.\win32s\sdlmpw.c
.\win32s\sdlvideo.c
.\commng.c
.\dosio.c
.\fontmng.c
.\ini.c
.\inputmng.c
.\joymng.c
.\mousemng.c
.\np2.c
.\scrnmng.c
.\sdlkbd.c
.\soundmng.c
.\sysmenu.c
.\sysmng.c
.\taskmng.c
.\timemng.c
.\trace.c
..\generic\cmjasts.c
..\generic\cmver.c
..\generic\hostdrv.c
..\generic\hostdrvs.c
..\embed\menu\dlgabout.c
..\embed\menu\dlgcfg.c
..\embed\menu\dlgscr.c
..\embed\menu\filesel.c
..\embed\menu\menustr.c
..\embed\menubase\menubase.c
..\embed\menubase\menudlg.c
..\embed\menubase\menuicon.c
..\embed\menubase\menumbox.c
..\embed\menubase\menures.c
..\embed\menubase\menusys.c
..\embed\menubase\menuvram.c
..\embed\vramhdl.c
..\embed\vrammix.c
..\calendar.c
..\debugsub.c
..\keystat.c
..\nevent.c
..\pccore.c
..\statsave.c
..\timing.c
```

## VC2008 Build Gate Inventory

### `Win9x/np2.vcproj`

- Project name: `np2`
- Configurations: `Debug|Win32`, `Release|Win32`, `Trace|Win32`, `WaveRec|Win32`
- RelativePath entries: `261`

Include paths:

- `.\,.\x86,.\dialog,.\debuguty,..\,..\common,..\i286x,..\io,..\cbus,..\bios,..\vram,..\sound,..\sound\vermouth,..\sound\getsnd,..\fdd,..\lio,..\font,..\generic`
- `.\,.\x86,.\dialog,.\debuguty,..\,..\common,..\i286x,..\io,..\cbus,..\bios,..\vram,..\sound,..\sound\vermouth,..\sound\getsnd,..\fdd,..\lio,..\font,..\generic,..\CPUXVA,..\CPUCVA,../BIOSVA,../IOVA`

Preprocessor definitions:

- `CPUDEBUG`
- `MEMTRACE`
- `NDEBUG`
- `SUPPORT_WAVEREC`
- `TRACE`
- `WIN32`
- `_DEBUG`
- `_WINDOWS`

Link libraries:

- `DSOUND.LIB`
- `comctl32.lib`
- `dxguid.lib`
- `odbc32.lib`
- `odbccp32.lib`
- `winmm.lib`
- `wsock32.lib`

Output files:

- `..\bin/np2t.exe`
- `..\bin/np2wr.exe`
- `..\bin/vaeg.exe`
- `..\bin/vaegd.exe`
- `.\..\bin/np2.bsc`

Custom build descriptions:

- `Assembling... $(InputPath)`

Custom build commands:

- `nasm -f win32 -w-label-orphan $(InputPath) -o $(IntDir)\$(InputName).obj -l $(IntDir)\$(InputName).cod&#x0D;&#x0A;`
- `nasm -f win32 -w-label-orphan $(InputPath) -o $(IntDir)\$(InputName).obj&#x0D;&#x0A;`

## Phase 1.4 Result

- No `.dsp`, `.dsw`, `.vcproj`, or `.sln` file was deleted.
- `Win9x/np2.dsp` and `Win9x/np2.vcproj` remain the key Windows build ledgers.
- Deletion of legacy Visual Studio project files is deferred until a future replacement build can produce a VA-capable `vaeg.exe` and the owner approves removal.
