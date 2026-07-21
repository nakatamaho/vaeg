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

M10 scope note: the gate items are implemented in the SDL2 ImGui frontend.
Unimplemented Win32 features remain visible as disabled stubs with
`(not implemented)`.

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
| `done` | Implemented in the SDL2 ImGui frontend. |
| `stub-visible` | In M10 gate scope; show it in the UI and disable or wire it during M10 implementation. |
| `later` | Deliberately out of M10 scope. Keep visible as disabled where it appears in the Win32 menu. |

## M10 Gate Scope

| Feature | Win32 origin | SDL2/ImGui target | Status | Notes |
|---|---|---|---|---|
| FDD image mount/unmount, drive 1 and 2 | Dynamic FDD menus in `win9x/menu.cpp:211`; handlers in `win9x/np2.cpp:756` | File browser plus eject command for FDD1/FDD2 | `done` | M8 already supports CLI mount. Win32 also exposes FDD3/FDD4, left visible as later stubs. |
| Reset | `&Emulate` / `IDM_RESET`; `win9x/np2.cpp:699` | Main menu command | `done` | SDL2 has no `np2oscfg.comfirm` prompt setting yet, so the command resets immediately. |
| State save/load | Dynamic flag menu in `win9x/menu.cpp:250`; handler in `win9x/np2.cpp:1422` | Save/load slots | `done` | SDL2 exposes 10 slots matching Win32 `SUPPORT_STATSAVE`. |
| Display scale and aspect | Win32 window/fullscreen plus system screen multiple controls in `win9x/menu.cpp:107` | Native/x2/x3/Custom window size; Native/Fit/Fit 8-dot/Integer/Stretch; aspect toggle | `done` | M21 adds a shared High-DPI viewport and edge-drag resize without reallocating the guest framebuffer. |
| Sound on/off, hardware, output, and volume | Sound board enable menu in `win9x/np2.rc:892`; Configure sampling/buffer controls; mixer dialog `IDD_SNDMIX` | Host audio enable, VA OPN/OPNA selection, NP2/ymfm backend, common sampling/buffer controls, ymfm fidelity, and master volume UI | `done` | M28 applies 11.025/22.05/44.1 kHz and 40-1000 ms to both backends. ymfm fidelity is separate and disabled for NP2. Full board jumper pages are `later`. |
| Key/joystick config, minimal | Keyboard menu in `win9x/np2.rc:872`; sound pad page in `IDD_SNDPAD1` | Keyboard/joystick mode and minimal mappings | `done` | M10 implements SDL key/joy mode and F12 binding. Mechanical keys and joystick rapid settings are `later`. |
| Exit | `&Emulate` / `IDM_EXIT`; `win9x/np2.cpp:752` | Main menu command | `done` | Reuses the same `taskmng_exit()` shutdown path as SDL2 quit and smoke exit. |

## Main Menu Inventory

| Win32 menu | Items | Status | Notes |
|---|---|---|---|
| Emulate | Reset; Configure; NewDisk; Font; Exit | Reset, CPU/SGP/host-pacing Configure, and Exit `done`; others `later` | M20 Configure covers CPU/SGP speed; M53 adds non-blocking host pacing. M28 places sampling rate and sound buffer under Sound. Resize, MMX, confirm, and resume remain later. Font selection is not the ImGui Japanese font decision. |
| Edit | Copy; Paste | ASCII host-to-guest Paste `done`; guest-to-host Copy `later` | M24 uses SDL clipboard UTF-8 input but emits only paced VA guest keyboard make/break events. Japanese/IME paste is later. |
| FDD dynamic menu | FDD1-FDD4 Open/Eject | FDD1/FDD2 `done`; FDD3/FDD4 `later` | FDD1/FDD2 are required for G10. |
| HardDisk | New SASI image; SASI1/SASI2 Open/Remove | SASI HDI create and SASI1/SASI2 Open/Remove `done`; SCSI/IDE `later` | M16 restores SASI through `HDD1FILE`/`HDD2FILE`; reset is the reliable apply point after changing images. |
| SCSI dynamic menu | SCSI0-SCSI3 Open/Remove | `later` | Added dynamically when SCSI support is compiled. |
| Screen | Window; FullScreen; rotation; display vsync; real palettes; no wait; frame skip; frame display; screen option | Resize, immediate Windowed/current-desktop Exclusive, effects, scale/aspect, No Wait, frame skip, and measured frame display `done`; Borderless, detailed mode selection, rotation, and advanced screen options `later` | M27 restores the original approximately two-second `drawcount` FPS measurement in the native title. The simplified M21 UI needs no separate Apply step. Procedural Unfiltered/Linear/Scanline/CRT Lite effects remain available. M20 host pacing remains unchanged. |
| Device / Keyboard | Keyboard/JoyKey modes; mechanical SHIFT/CTRL/GRPH; F12 mapping; Alt-right mapping; host-layout mapping mode | Host-layout mapping and binding table `done`; mechanical key options `later` | M14 implements JIS/US/custom SDL scancode mapping, Kana modes, and Roman-Kana helper. Mechanical SHIFT/CTRL/GRPH mode options remain later. |
| Device / Sound | Beep level; disable boards; VA Sound Board 2; PC-9801 boards; JAST; seek sound | Sound on/off, VA OPN/OPNA hardware, backend, output rate/buffer, ymfm fidelity, volume, and seek sound `done`; rest `later` | Hardware selection is model-aware. M28 output settings rebuild both backends; fidelity affects only ymfm FM synthesis. Detailed jumper pages remain later. |
| Device / Memory | 640KB, 1.6MB, 3.6MB, 7.6MB | `later` | Win32 command handler also contains 11.6MB and 13.6MB cases. |
| Device | I/O Bank Memory; Mouse; Mouse Port; Version Up Board; Serial; MIDI; MIDI Panic; Sound option | I/O Bank Memory, mouse capture, and VA joystick/mouse port `done`; rest `later` | M52 restores the BMS dialog and one-based overlay semantics. M26 uses SDL relative mode and the existing guest mouse I/O path. |
| Other | BMP save; S98 logging; Calendar; shortcuts; clock/frame display; joy reverse/rapid; mouse rapid; SSTP; Help; About | Frame display, Mouse rapid, and About `done`; rest `later` | Frame display is organized under Screen in SDL2. CPU clock display remains later. Mouse rapid is exposed under Device -> Mouse. Help uses the retired Windows help path. |
| State dynamic menu | Save slots and load slots | `done` | Required by M10 as state save/load. |
| Operation record dynamic menu | Stop; Play; Multi play; Record; Repeat; record/play slots | `later` | `SUPPORT_OPRECORD` feature, not M10 gate scope. |
| System menu additions | Tool window; key display; soft keyboard; screen center; snap; background; background sound; memory dump; debugger utility; debug control; screen multiples | Screen multiple related controls `done`; rest `later` | Debugger UI is explicitly out of M10. |

## Dialog Inventory

| Dialog/resource | Implementation | Function summary | Status |
|---|---|---|---|
| `IDD_CONFIG` | `win9x/dialog/d_config.cpp` | Base clock, multiplier, model, sampling rate, sound buffer, resize, MMX, confirm, resume | CPU/SGP speed, host pacing, and Sound-menu sampling/buffer controls `done`; remaining fields `later` |
| `IDD_NEWDISK`, `IDD_NEWDISK2`, `IDD_NEWHDDDISK`, `IDD_NEWSASI` | `win9x/dialog/d_disk.cpp` | Create floppy/HDD image files | Formatted FAT12 D88/IMG and SASI HDI creation `done`; THD/NHD and SCSI creation `later` |
| FDD/HDD file selectors | `win9x/dialog/d_disk.cpp` | Open FDD, SASI/IDE, SCSI images | FDD1/FDD2 and SASI1/SASI2 `done`; SCSI/IDE `later` |
| `IDD_SCROPT1` | `win9x/dialog/d_screen.cpp` | LCD mode, skipline, skiplight | `later` |
| `IDD_SCROPT2` | `win9x/dialog/d_screen.cpp` | GDC chip, GRCG/EGC, PC-9801-24 color | `later` |
| `IDD_SCROPT3` | `win9x/dialog/d_screen.cpp` | Wait-state and real palette tuning | `later` |
| `IDD_SERIAL1`, `IDD_PC9861A/B/C` | `win9x/dialog/d_serial.cpp` | Serial and PC-9861K options | `later` |
| `IDD_MPUPC98` | `win9x/dialog/d_mpu98.cpp` | MPU-PC98 I/O, interrupt, MIDI in/out, MIMPI def | `later` |
| `IDD_SNDMIX` | `win9x/dialog/d_sound.cpp` | FM/PSG/ADPCM/PCM/rhythm volumes | Volume `done`; detailed mixer `later` |
| `IDD_SND14`, `IDD_SND26`, `IDD_SND86`, `IDD_SNDSPB`, `IDD_SNDPAD1` | `win9x/dialog/d_sound.cpp` | Board-specific volume/jumper/joystick pad settings | `later` |
| S98 and WAV file selectors | `win9x/dialog/d_sound.cpp` | Audio logging and WAV recording | `later` |
| `IDD_BMS` | `win9x/dialog/d_bms.cpp` | I/O bank memory enable, port, bank count | `done` (M52); legacy keys persist, bank zero restores main RAM, and changes apply through guest reset |
| `IDD_CALENDAR` | `win9x/dialog/d_clnd.cpp` | Real/virtual calendar and BCD time fields | `later` |
| `IDD_ABOUT` | `win9x/dialog/d_about.cpp` | About box | `later` |
| `IDD_VIEW_ADDRESS`, `IDR_VIEW` | `win9x/debuguty/*.cpp` | Debug/viewer address, register, dump, disassembly, VA views | `later` |
| Tool window and key display | `win9x/toolwin.cpp`, `win9x/subwind.cpp` | Skin menu, tool buttons, FM/MIDI key display | `later` |

## Explicit M10 Gaps

- Debugger UI, debug control, memory dump, and debug viewers are `later`.
- Host IME text input is not part of M10/M14/M24. M14 Roman-Kana and M24
  ASCII clipboard paste emit guest key sequences, not host text or Unicode.
- Host physical keyboard layout mapping, including US-host to JIS-guest
  symbol positions, is `done` for the documented M14 inventory, with
  unresolved bindings visible in the GUI.
- Native platform file dialogs are not required; an ImGui file browser is
  acceptable for M10.
- The ImGui Japanese font must come from the bundled font asset, never
  from the guest font ROM.
- The first ImGui menu ordering should follow `IDR_MAIN` and the dynamic
  Win32 menus before any redesign work.
