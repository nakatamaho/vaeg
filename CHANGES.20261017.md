<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# 88VA Eternal Grafx Rel.20261017

This release records the changes from Rel.20260908 through Rel.20261017.
The main additions are the V1/V2 compatibility modes (PC-8801 and N-BASIC
software on the VA), PC-8801 and PC-8001 machines started from
user-supplied ROMs, the 8087, the PCG-8100/8800, and a reorganized menu.

<div align="center">
<table align="center" width="100%">
<tr>
<td align="center" width="50%"><img width="100%" src="https://github.com/nakatamaho/vaeg/releases/download/rel-20261017/vaeg-20261017-155514-0000091005-000.png" alt="PC-8801mkIISR demonstration in V2 mode"><br><em>PC-8801mkIISR demonstration in V2 mode</em></td>
<td align="center" width="50%"><img width="100%" src="https://github.com/nakatamaho/vaeg/releases/download/rel-20261017/vaeg-20261017-155419-0000036240-000.png" alt="N88-BASIC V2 in V2 mode"><br><em>N88-BASIC V2 in V2 mode</em></td>
</tr>
</table>
</div>

## V1/V2 modes (PC-8801 compatibility)

- V1/V2 mode now runs: N88-BASIC V1/V2 and N88-DISK BASIC with text,
  colour and monochrome graphics, `FILES`/`SAVE`/`LOAD`, uPD3301 text with
  semigraphics and 40 columns, the port 40h buzzer and port sound.
  PC-8801 demonstration software runs.
- N mode starts N-BASIC from a user-supplied ROM.
- 模擬 > 起動 selects the machine and its mode: 自動 (the ROM decides from
  the boot disk), V1/V2 H/S (also with a V3 disk or no disk) and N.
  `BootMode` in `vaeg.cfg` replaces `V1V2_NMode`.
- Cassette tape images (`.cmt`, `.t88`) can be loaded in V1/V2 BASIC, with
  tape sound and an optional fast load (off by default). This is a vaeg
  extension; the VA has no cassette interface.
- A PC-8801-style display option for V1/V2, monitor switches (15 kHz,
  digital RGB 8 colours), and a multi-image D88 selection
  (`--fdd1-image`/`--fdd2-image`).
- FDD > new FDD image creates unformatted D88 images for 2HD, 2DD (720 KB)
  and 2D (320 KB).

## PC-8801, PC-8001 and PCG

- PC-8801 models (PC-8801 through the later V1/V2 models) start from ROMs
  placed in `extra/<model>/<role>.rom` next to the executable, or a combined
  `PC88.ROM`. They run in the VA's V1/V2 mode with their own ROMs; the VA
  ROM set is still needed. Models are named from MAME's published set
  identities. `--list-extra-roms` lists what was found, and 起動 lets you
  choose a ROM revision.
- PC-8001, PC-8001mkII and PC-8001mkIISR start in the VA's N mode with
  their own N-BASIC, on a 15 kHz display.
- PCG-8100/8800 (ported from the public-domain X88000): 256 user-defined
  8x8 characters and the 8253 three-voice tone generator in V1/V2 mode, as
  an option in the Configure dialog. A sample BASIC program is in
  `demos/pcg-sample`.
- No PC-8801 or PC-8001 ROMs are included.

## V3 display, CPU and sound

- 8087 emulation is included and enabled by default, with the VA2's 8 MHz
  clock.
- The uPD9002's Z80 emulation mode follows the real chip's flags and
  undocumented opcodes, checked with ZEX on a PC-88VA2.
- Display timings beyond the documented modes (480-line frames, 15 kHz
  240-line frames) are shown as on a PC-88VA2.
- TSP attribute modes 2-7 are decoded, including blinking text; the text
  cursor is hidden while the text display is off.
- SGP: the original VA reads the source pitch like the VA2 (no more
  striped sprites read upwards), and LINE uses the direction bits measured
  on hardware.
- Pasted text no longer loses repeated characters and is paced by guest
  frames.

## Frontend

- Reorganized Japanese menus (模擬 and nine further top menus); Pause is an
  F12 assignment; one speed control; CPU clock presets.
- State save and load with ten slots per software, each with a thumbnail,
  and a slot list window.
- Master volume and per-source volumes.
- 最近の起動 and media histories; disks extracted from archives are
  remembered as their archive, and a missing file is shown instead of
  silently booting the previous disk.
- 画面 > 右に 90° 回転 turns the screen 90 degrees right (also `--rotate`),
  with the SDL renderer and the CRT shader.
- A front-panel lamp bar (FDD access, V1/V2/V3 mode) and KANA and CAPS
  lamps.
- 15 kHz scanline fill (CRT gaps or LCD fill); 200-line modes are line
  doubled at 24 kHz.

## Disks and demos

- A self-booting SGP demo disk (`demos/disks/sgp-boot.d88.xz`) runs the SGP
  demos and display tests without an operating system.
- `demos/disks/all-demos-msdos4.d88.xz` boots MS-DOS 4.0 for the PC-88VA
  with every demo.
- The SGP demos are assembled for the 80186, as the VA's CPU requires.
- Fixed-disk boot follows the attached HDD; the utility FDD and SASI disks
  include VZ Editor and V480PAT.

## Documentation

- BIOS interrupt references under `docs/bios/`, and the I/O port map with
  TSP and SGP references under `docs/io/`.

ROM images, fonts, and private disk images are not included. Supply the
model-specific PC-88VA ROM files separately.
