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

# M109 - Demo fixes and 200-line display on the 24 kHz monitor

Status: **in progress**

Series: V3 series (maintainer request, 2026-10-14, after G106). Branch
`topic/m109-demo-display-fixes` off `main` at `d9a84316`. Commit prefix
`M109:`.

## Goal

Items carried over from M106 by the maintainer:

1. ZUNDAORB runs without an I/O bank memory board (the maintainer's
   PC-88VA2 has none; M106 found that the program stops when its BMS
   probe fails).
2. ZUNDAORB HUD: a space after the `DIST:` value, and `LOOK` and `RAD`
   moved right by one column accordingly.
3. Every pseudo-sprite demo can show up to 1024 balls.
4. Other demo fixes found on the PC-88VA2 with the self-booting SGP disk.
5. With the 24 kHz monitor setting, 640x200 and 320x200 modes are shown
   without scanlines (line-doubled), and ZUNDAORB uses such a mode too.
6. The kana and Caps Lock indicators at the bottom (front panel / status
   area).
7. A hardware check of the digital 8-colour mode (maintainer, on the
   PC-88VA2).

## Gate G109 (human)

Standard V3 gate (clean static MinGW build of the pushed commit), plus:
ZUNDAORB runs on the PC-88VA2 from the rebuilt disks; the pseudo-sprite
demos reach 1024 balls; 200-line modes show no scanlines at 24 kHz in vaeg;
the kana and Caps Lock indicators follow the keyboard; the digital
8-colour result on hardware is recorded whatever it is. Demo disks are
rebuilt and their payloads match a fresh build (AGENTS.md).

## Progress

- 24.8 kHz scanlines (item 5): on a 24.8 kHz display (port 1CBh bit 3) the
  demos that use 200-line graphics set port 100h RSM = 01 (lines doubled,
  measured on the PC-88VA2 with V480PAT D in M104) after turning graphics
  on and back to RSM = 00 before restoring the screen: SGP pseudo-sprite
  16/256/65536, sgp-scan 65536, wireframe 65536, GLASS, NEON3 (200-line),
  ZUNDAORB. NEON4 (640x400, or 65536 colours already RSM = 01) needed
  nothing. In vaeg the old SGPD_7A showed a blank odd raster on all 170 lit
  rasters, the rebuilt ones none. NEON3's unused reserve shrank by 64 words
  to fit. A vaeg display option for blank rasters was tried and dropped: the
  demos now choose the doubled mode themselves.
- ZUNDAORB without bank memory (items 1, 2): the atlas is in conventional
  RAM at 40000h-5FFFFh (SGP reads main RAM at the same address); the BMS
  probe became a check that DOS gave the program memory up to 60000h. In
  vaeg it runs with `Use_BMS_=false` and 512 KiB. HUD: `DIST:+n ` is one
  cell wider, LOOK and RAD moved right by one cell (on the HUD background).
- Zundamon artwork (maintainer): the first standing pose (layers as stored)
  of the free "ずんだもん立ち絵素材2.3.psd", redistributable. The atlas
  builder reads the PSD's merged image with psd-tools when ImageMagick is
  absent (reproducible); the IDA profile tables are tracked in
  `demos/zundamon-orbit/256/ida/` and `build-from-psd.sh` rebuilds the
  distribution disk from the PSD, so zundamon is no longer an exception in
  AGENTS.md.
- 1024 balls (item 3): all pseudo-sprite demos (16-colour M7a-M7d, M7s,
  256 static/scrolling, 65536). Their SGP command lists, and the M7d
  templates, moved out of the .COM into the segments after the program
  (CS + 1000h..); the dirty-rectangle list is capped at 513 with a full-clear
  fallback; records 128+ of the 256/65536-colour demos are generated. Found
  on the way: M7d never updated its FPS/count digits (startup glyph
  addresses), now converted each frame; M7 records 33+ started in 8 columns.
  In vaeg all draw 1024 balls (start count 1024 in a test build).
- KANA and CAPS lamps (item 6): the front-panel bar shows them from the
  guest's lock state, V3 BIOS shift byte 00AB0h (bit 3 KANA, bit 2 CAPS) or,
  in V1/V2, the PC-8801 key matrix (port 08h bit 5, 0Ah bit 7), each checked
  in vaeg; `@key caps`/`@key kana` added to the headless input.
- Display tests on the self-booting disk (maintainer: TSPTEST = TSPMODE,
  plus TSPFILL and `V480PAT A`): menu items L, M, N with command tails; the
  loader gives INT 21h 48h/49h (TSPFILL) and turns graphics off on return.
  The three tools waited on keyboard BIOS 00h/01h (hang without an OS and
  under MS-DOS) and now use 0Ah/09h. `demos/disks/sgp-boot.d88.xz` is
  committed as a distributable bootable disk (all source-built, no system
  files, reproducible, round-trip checked).
- Disks rebuilt from HEAD: glass-orbit, neon3, sgp-pseudo-sprite,
  sgp-wireframe, zundamon-orbit, all-demos, all-demos-msdos4 and the new
  sgp-boot; neon4 kept (its files are unchanged, only directory dates
  differed). The MS-DOS 4.0 source image was fetched again from release
  `msdos4-va.2` (SHA-256 matches the pin). In vaeg (VA2) all 17 programs of
  the PC-Engine 1.1 bootable and the MS-DOS 4.0 all-demos disks draw and
  leave on ESC; on the self-booting disk A-N run and return to the menu.
  [f0ad4e47](https://github.com/nakatamaho/vaeg/commit/f0ad4e473d745ca48a4fffbe3be31b54ac257a63).

