<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->
# GUI Parity Inventory

M10 scope note: this file is the step 1 inventory only. No SDL2 ImGui
implementation exists in this commit, so no item is marked `done` yet.
The M10 gate items are marked `stub-visible`; the implementation commits
must make them real while keeping unimplemented Win32 features visible as
disabled stubs.

## Sources

- Main menu resource: `win9x/np2.rc:840`.
- Dynamic FDD, SCSI, state-save, operation-record, and system-menu items:
  `win9x/menu.cpp:211`, `win9x/menu.cpp:236`, `win9x/menu.cpp:250`,
  `win9x/menu.cpp:276`, `win9x/menu.cpp:336`.
- Command dispatch: `win9x/np2.cpp:699`.
- Dialog resources: `win9x/np2.rc:32` through `win9x/np2.rc:635`.
- Dialog implementations: `win9x/dialog/*.cpp`, `win9x/toolwin.cpp`,
  `win9x/subwind.cpp`, and `win9x/debuguty/*.cpp`.
- Current SDL2 frontend: `sdl2/README.md`, `sdl2/np2.c`,
  `sdl2/scrnmng.c`, `sdl2/sdlkbd.c`, `sdl2/soundmng.c`.

## Status Legend

| Status | Meaning |
|---|---|
| `done` | Implemented in the SDL2 ImGui frontend. None yet in step 1. |
| `stub-visible` | In M10 gate scope; show it in the UI and disable or wire it during M10 implementation. |
| `later` | Deliberately out of M10 scope. Keep visible as disabled where it appears in the Win32 menu. |

## M10 Gate Scope

| Feature | Win32 origin | SDL2/ImGui target | Status | Notes |
|---|---|---|---|---|
| FDD image mount/unmount, drive 1 and 2 | Dynamic FDD menus in `win9x/menu.cpp:211`; handlers in `win9x/np2.cpp:756` | File browser plus eject command for FDD1/FDD2 | `done` | M8 already supports CLI mount. Win32 also exposes FDD3/FDD4, left visible as later stubs. |
| Reset | `&Emulate` / `IDM_RESET`; `win9x/np2.cpp:699` | Main menu command | `done` | SDL2 has no `np2oscfg.comfirm` prompt setting yet, so the command resets immediately. |
| State save/load | Dynamic flag menu in `win9x/menu.cpp:250`; handler in `win9x/np2.cpp:1422` | Save/load slots | `stub-visible` | Slot count follows `SUPPORT_STATSAVE`. |
| Display scale and aspect | Win32 window/fullscreen plus system screen multiple controls in `win9x/menu.cpp:107` | x1/x2/x3, aspect toggle | `stub-visible` | This is an SDL2-specific projection of the Win32 screen controls. |
| Sound on/off and volume | Sound board enable menu in `win9x/np2.rc:892`; mixer dialog `IDD_SNDMIX` | Sound enable plus master/board volume UI | `stub-visible` | Full board jumper pages are `later`. |
| Key/joystick config, minimal | Keyboard menu in `win9x/np2.rc:872`; sound pad page in `IDD_SNDPAD1` | Keyboard/joystick mode and minimal mappings | `stub-visible` | Detailed F12, mechanical keys, and joystick rapid settings are `later` unless needed for G10. |
| Exit | `&Emulate` / `IDM_EXIT`; `win9x/np2.cpp:752` | Main menu command | `stub-visible` | Should reuse the same shutdown path as SDL2 quit. |

## Main Menu Inventory

| Win32 menu | Items | Status | Notes |
|---|---|---|---|
| Emulate | Reset; Configure; NewDisk; Font; Exit | Reset and Exit `stub-visible`; others `later` | Configure covers base clock, model, sampling rate, sound buffer, resize, MMX, confirm, resume. Font selection is not the ImGui Japanese font decision. |
| FDD dynamic menu | FDD1-FDD4 Open/Eject | FDD1/FDD2 `stub-visible`; FDD3/FDD4 `later` | FDD1/FDD2 are required for G10. |
| HardDisk | SASI1/SASI2 Open/Remove | `later` | HDD mounting is not in the M10 must-have set. |
| SCSI dynamic menu | SCSI0-SCSI3 Open/Remove | `later` | Added dynamically when SCSI support is compiled. |
| Screen | Window; FullScreen; rotation; display vsync; real palettes; no wait; frame skip; screen option | Display scale/aspect `stub-visible`; rest `later` | Screen option dialog has LCD/skipline, GDC/GRCG/color, wait-state and real palette pages. |
| Device / Keyboard | Keyboard/JoyKey modes; mechanical SHIFT/CTRL/GRPH; F12 mapping; Alt-right mapping | Minimal key/joy config `stub-visible`; rest `later` | Alt-right mapping is a VAEG extension. IME/text input work is out of M10 scope. |
| Device / Sound | Beep level; disable boards; VA Sound Board 2; PC-9801 boards; JAST; seek sound | Sound on/off and volume `stub-visible`; rest `later` | Board selection and detailed option pages stay visible but disabled. |
| Device / Memory | 640KB, 1.6MB, 3.6MB, 7.6MB | `later` | Win32 command handler also contains 11.6MB and 13.6MB cases. |
| Device | IO Bank Memory; Mouse; Mouse Port; Version Up Board; Serial; MIDI; MIDI Panic; Sound option | `later` | Mouse capture itself is frontend input, but the Win32 device options are not M10 gate items. |
| Other | BMP save; S98 logging; Calendar; shortcuts; clock/frame display; joy reverse/rapid; mouse rapid; SSTP; Help; About | `later` | Help uses the retired Windows help path. About can be added after core menu parity. |
| State dynamic menu | Save slots and load slots | `stub-visible` | Required by M10 as state save/load. |
| Operation record dynamic menu | Stop; Play; Multi play; Record; Repeat; record/play slots | `later` | `SUPPORT_OPRECORD` feature, not M10 gate scope. |
| System menu additions | Tool window; key display; soft keyboard; screen center; snap; background; background sound; memory dump; debugger utility; debug control; screen multiples | Screen multiple related controls `stub-visible`; rest `later` | Debugger UI is explicitly out of M10. |

## Dialog Inventory

| Dialog/resource | Implementation | Function summary | Status |
|---|---|---|---|
| `IDD_CONFIG` | `win9x/dialog/d_config.cpp` | Base clock, multiplier, model, sampling rate, sound buffer, resize, MMX, confirm, resume | `later` |
| `IDD_NEWDISK`, `IDD_NEWDISK2`, `IDD_NEWHDDDISK`, `IDD_NEWSASI` | `win9x/dialog/d_disk.cpp` | Create floppy/HDD image files | `later` |
| FDD/HDD file selectors | `win9x/dialog/d_disk.cpp` | Open FDD, SASI/IDE, SCSI images | FDD1/FDD2 `stub-visible`; HDD/SCSI `later` |
| `IDD_SCROPT1` | `win9x/dialog/d_screen.cpp` | LCD mode, skipline, skiplight | `later` |
| `IDD_SCROPT2` | `win9x/dialog/d_screen.cpp` | GDC chip, GRCG/EGC, PC-9801-24 color | `later` |
| `IDD_SCROPT3` | `win9x/dialog/d_screen.cpp` | Wait-state and real palette tuning | `later` |
| `IDD_SERIAL1`, `IDD_PC9861A/B/C` | `win9x/dialog/d_serial.cpp` | Serial and PC-9861K options | `later` |
| `IDD_MPUPC98` | `win9x/dialog/d_mpu98.cpp` | MPU-PC98 I/O, interrupt, MIDI in/out, MIMPI def | `later` |
| `IDD_SNDMIX` | `win9x/dialog/d_sound.cpp` | FM/PSG/ADPCM/PCM/rhythm volumes | Volume `stub-visible`; detailed mixer `later` |
| `IDD_SND14`, `IDD_SND26`, `IDD_SND86`, `IDD_SNDSPB`, `IDD_SNDPAD1` | `win9x/dialog/d_sound.cpp` | Board-specific volume/jumper/joystick pad settings | `later` |
| S98 and WAV file selectors | `win9x/dialog/d_sound.cpp` | Audio logging and WAV recording | `later` |
| `IDD_BMS` | `win9x/dialog/d_bms.cpp` | IO bank memory enable, port, bank count | `later` |
| `IDD_CALENDAR` | `win9x/dialog/d_clnd.cpp` | Real/virtual calendar and BCD time fields | `later` |
| `IDD_ABOUT` | `win9x/dialog/d_about.cpp` | About box | `later` |
| `IDD_VIEW_ADDRESS`, `IDR_VIEW` | `win9x/debuguty/*.cpp` | Debug/viewer address, register, dump, disassembly, VA views | `later` |
| Tool window and key display | `win9x/toolwin.cpp`, `win9x/subwind.cpp` | Skin menu, tool buttons, FM/MIDI key display | `later` |

## Explicit M10 Gaps

- Debugger UI, debug control, memory dump, and debug viewers are `later`.
- IME text input is not part of M10.
- Native platform file dialogs are not required; an ImGui file browser is
  acceptable for M10.
- The ImGui Japanese font must come from the bundled font asset, never
  from the guest font ROM.
- The first ImGui menu ordering should follow `IDR_MAIN` and the dynamic
  Win32 menus before any redesign work.
