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

# SGP sprite benchmark

`SGPBENCH.COM` (`sgpbench.asm`), an MS-DOS program for a PC-88VA,
PC-88VA2 or PC-88VA3 (and vaeg), measures how many sprites the SGP draws
in each screen, colour depth, source and size a game is likely to use:

| Dimension | Values |
|---|---|
| Screen | 320x200, 320x224 (G0 only, single plane, 320 dots) |
| Colours | 16 (4 bpp, palette), 256 (8 bpp, GGGRRRBB), 65536 (16 bpp, G6R5B5) |
| Source | main RAM, GVRAM (offset 24000h, beyond the displayed lines) |
| Size | 8x8, 16x16, 32x32, 64x64 |

Each of the 48 tests draws one SGP list over and over for 180 VRTCs (about
3 s): `SET_WORK`, then for each sprite `SET_SOURCE`, `SET_DEST` and
`BITBLT 0105h` (forward copy, source colour 0 transparent), then `END`.
A list holds 128 8x8, 64 16x16, 32 32x32 or 16 64x64 sprites at fixed
scattered positions (the same at every depth and source, with the start
dot of any x), the four characters in turn. The CPU polls the SGP's busy
bit and the TSP's VRTC bit with interrupts off, starts the next list as
soon as one ends, and counts only lists that finished inside the 180
VRTCs. The result is sprites a VRTC: the sprites of the finished lists
divided by 180.

The characters are synthetic: `gen_data.py` draws four original figures
(a round robot, a squid, a box robot and a star) from circles,
rectangles and polygons at each size, in 16 colours with colour 0
transparent; the program expands them to 8 and 16 bpp with the same
colours.

During a test line 1 names it (`320X224 256 GVRAM 16X16 N64`) and, when
it ends, shows its result at the right, so a photograph of the screen
identifies the test. At the end the table is drawn on a 320x200,
16-colour screen (it fits there; at 15 kHz MS-DOS shows too few text
lines for it), headed by the machine read from F000:FFFE (FFFFh PC-88VA,
FFFEh PC-88VA2 or VA3, FFFDh PC-88VA with the PC-88VA-91 board) and the
display frequency. A key returns to the BIOS screen, and the same table is
printed through DOS:

```text
SGPBENCH  PC-88VA2/VA3  15 KHZ
SPRITES A VRTC, 180 VRTCS A TEST, BITBLT 0105H
SCREEN  COL.  SOURCE    8X8  16X16  32X32  64X64
320X200 16    RAM      72.5   43.0   17.4    5.4
...
SPRITES A LIST: 128, 64, 32, 16
```

(The row shown is vaeg's PC-88VA2, not a measurement on the machine. vaeg
does not yet distinguish the two sources.)

`SGPBENCH > RESULT.TXT` writes the table to a file instead. ESC between
tests stops; the remaining rows are then 0.

The 320x224 screen uses the 224-line SYNC of `demos/tsp-sprites` and
`demos/sgp-contention` (15.98 kHz, 267-line frame, 59.9 Hz; measured on
the PC-88VA2, see
[pc88va-video-modes.md](../../docs/modernization/pc88va-video-modes.md)
12.2). At 24.8 kHz (port 1CBh bit 3) the 224-line rows are skipped
(`-`), because the graphics stop advancing after line 200 there, and the
200-line rows are shown line-doubled (port 100h RSM = 01). The VRTC rate
differs: about 61.5 Hz at 15.98 kHz and 200 lines, 59.9 Hz at 224 lines,
56.4 Hz at 24.8 kHz; the table is per VRTC, so multiply by the rate for
sprites a second.

The program needs 128 KB after itself (the expanded sources at CS +
1000h) and returns to DOS.

```sh
demos/sgp-sprite-bench/build.sh [--msdos4 msdos4-pc88va-2hd.d88] [OUTDIR]
```

With `--msdos4` (the FreeDOS-88VA msdos4-va.2 disk) the script also writes
`sgp-sprite-bench-msdos4.d88`, a vanilla MS-DOS 4.0 copy with the
program; type `SGPBENCH`. `PREVIEW=chars.ppm` also writes the characters
at four times their size.
