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

# M117 — TSP attribute modes 4-7 and blink; screen rotation

Status: **complete; G117 passed on 2026-10-16; merged to `main` at [2c95625](https://github.com/nakatamaho/vaeg/commit/2c95625ae5010704e867941ab27f690ed01bfc50)**

## Request

The maintainer reported (2026-10-16) that text which blinks on the hardware
does not blink in vaeg: the blink attribute is not read. The TSPMODE record
(M104) itself is valid.

## Cause

`vram/maketextva.c` decoded only attribute modes 0 and 1; modes 2-7 were all
read as mode 0 (bits 7:4 background, 3:0 foreground, no attributes). Modes 4
and 5 keep blink in bit 7 (Technical Manual), so their blink was lost and
bit 7 showed as a brighter background (F0h: grey instead of white).

## Work

- Modes 4 and 5: bit 7 blink, bits 6:4 background, 3:0 foreground; mode 5
  adds the horizontal line when `(foreground & 7) == 1`.
- Modes 6 and 7 (undocumented): 4-bit background and foreground, mode 6
  with bit 7 as blink, mode 7 without; matches the PC-88VA2 TSPMODE
  observations with F0h (6: grey, blinking; 7: grey, steady).
- Modes 2 and 3 are unchanged (still read as mode 0).
- Selftest `TSP attribute modes`, mutation-checked.

## Verification

- TSPMODE 4 under PC-Engine (VA2, vaeg headless): before, the block stays
  grey and identical in every sampled frame; after, it is white on the
  white main split and disappears and returns with the blink phase.
- ctest 93/93, selftest all passed.
- Media build regression at `6b9e5e23` (clean clone): the utility disk
  (4096 bytes free, boots), both SASI images (boot on their models and run
  `\BIN\V480PAT.COM`), the five demo `build-d88.sh`, `build-from-psd.sh`,
  the SGP boot disk, all-demos and all-demos-msdos4 exit 0 with every file
  identical to the committed images (SGP boot D88 and all-demos-msdos4
  `.xz` byte-identical).
- Clean-checkout MinGW build: below.

## Attribute modes 2, 3 and 6 (2026-10-16)

The maintainer ran TSPMODE from the self-booting SGP disk on a PC-88VA2
(photographs and films kept privately): mode 6 does not blink; mode 2
shows each character's left half at double width with a line under every
row; mode 3 is black; modes 8-10 show the static pattern known from M104.
The menu text carries attribute 70h and the test block F0h, so mode 2 reads
70h as DWID, underline and horizontal line (Technical Manual bits) and mode
3 sees only shape records. vaeg now decodes mode 2 (DWID left half, DWIDC
right half, underline, line), mode 3 (colour/shape records from black on
black at each split) and modes 6-7 as mode 0. vaeg's TSPMODE 2 matches the
photograph line for line; 3 is black; 6 is grey and steady. The 8-14
pattern is not modelled (its source is unidentified). Selftest extended and
mutation-checked; ctest 93/93.

## Screen rotation

Requested by the maintainer on 2026-10-16: turn the screen 90 degrees right.

- 画面 > 右に 90° 回転, config `GUI_rotate` (0 none, 1 right), command line
  `--rotate none|right` (applies to the session only, like `--effect`).
- Display only: the guest frame, 加工前 screenshots and state thumbnails
  stay unturned; rendered screenshots and `--screenshot` are turned. The SDL path draws the texture turned
  with `SDL_RenderCopyEx`; Native CRT receives the turned picture (height x
  width, 3:4 with the aspect correction). The viewport fits the turned
  picture, the window size from the 画面 menu becomes height x width, mouse
  points are turned back for the guest, and the scanline and CRT-lite
  overlays follow the guest rows and columns.
- Startup keeps the saved window size, as for the window scale; toggling
  the menu item resizes the window.
- Selftest `viewport`: turned geometry (800x600: 375x600 at x 212; 450x600
  with the aspect correction) and the turned inverse mapping (1:1 corners),
  each mutation-checked.

Verification (Linux, headless): `--rotate right` shows the PC-Engine screen
turned right with the guest top on the right (SDL renderer and the dummy
capture); with `--effect scanline` and `crt-lite` the lines run vertically;
the menu item appears and resizes the window to 400x676; `--rotate` is not
written to the configuration. The Native CRT path could not be run here.

Clean-checkout MinGW build of `5d2b03ad` (blink fix and rotation):
`vaeg.exe` SHA-256
`eb604e044aa36a637c2ec2fa6bcda2ee0ca62852d85ed6bc60168f174be692db`.

## Cursor after SYNC

Reported by the maintainer on 2026-10-16: a commercial game disk shows the text
cursor in vaeg (VA and VA2) but not on the hardware. The game's TSP
commands end with CURDEF FBh (sprite 31, CE and BE) and a SYNC with no
DSPON; no sprite has SW set. The CE cursor is now shown only while DSPON is
in effect. Verified: no cursor over eight frames of the game screen on both
models; the PC-Engine, V1/V2 BASIC and N-BASIC cursors still blink; ctest
93/93; selftest extended and mutation-checked.

Clean-checkout MinGW build of `2fe4d9a7` (with the cursor fix and
attribute modes 2, 3 and 6; built as `da3c9353` before the 2026-10-16
history rewrite, which changed only comments in its sources):
`vaeg.exe` SHA-256 `539d656872aa329e9cedcf36f2e25cf8be9284f905754f1ec51876be19eadb94`.

## Gate G117 (human)

Standard gate, plus: the software where the maintainer saw blinking on the
hardware blinks in vaeg; TSPMODE 2-7 matches the hardware (2: halves at
double width with lines, 3: black, 4-5: blinking, 6-7: grey and steady); 画面 > 右に
90° 回転 turns the picture right with the SDL renderer and with Native CRT,
the mouse follows, and turning it off restores the window; the
commercial game shows no text cursor on either model. (Rotation passed with the SDL
renderer and librashader, maintainer, 2026-10-16.)

### G117

- Passed on 2026-10-16 (maintainer) with the Windows build
  `539d656872aa...db94` (SHA-256, static MinGW) from
  [2fe4d9a7](https://github.com/nakatamaho/vaeg/commit/2fe4d9a7b42e842a0a6a3c14fdd616f8aa5a38da):
  blink, TSPMODE 2-7, the rotation (SDL renderer and librashader) and the
  game's hidden cursor. The `MODE` 8-14 pattern moves to a separate milestone.
