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

# Sprite benchmarks: SGP, CPU, both, and double-buffered

Four MS-DOS programs with the same screens, characters, positions and
table: `SGPBENCH.COM` draws the sprites with the SGP, `CPUBENCH.COM`
with the CPU, `MIXBENCH.COM` with both at once, and `DBLBENCH.COM`
erases and draws them on a hidden page every VRTC (below). `common.inc`
holds what they all share, `sgp.inc` the SGP's part and `cpu.inc` the
CPU's.

## SGPBENCH

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

## CPUBENCH

`CPUBENCH.COM` (`cpubench.asm`) runs the same 48 tests with the CPU
drawing into G0 through the GVRAM window at A0000h, in two ways instead
of the two sources (the `WAY` column):

- `COMPLD`: compiled sprites, as the CPU sprites of an arcade-game port
  for the VA. Each image (and, at 16 colours, each dot parity: an odd x
  starts a nibble later) is a far routine that writes at `DS:[DI + d16]`:
  `MOV` of an immediate word where both bytes are opaque, `MOV AX` /
  `AND AX` / `OR AX` / `MOV` back where a word has transparent dots,
  nothing for transparent bytes; the byte forms for a lone byte, then
  `RETF`. At 256 and 65536 colours every dot is either opaque or
  transparent, so the routines are only `MOV`s of immediates.
- `LOOP`: a plain loop over the image's data: at 16 colours a keep mask
  and the dots a byte (`LODSW`, `AND AL,[ES:DI]`, `OR AL,AH`, `STOSB`);
  at 256 and 65536 colours the dots, each written unless 0.

`gen_data.py --cpu DIR` makes both, for each depth, in `CPU4.DAT`,
`CPU8.DAT` and `CPU16.DAT` (88960, 54992 and 109408 bytes: a header of
32 code and data paragraph pairs, then the blocks); CPUBENCH reads the
depth's file from the current directory to CS + 1000h, so run it where
the files are. Only the VRTC interrupt (IR2) is unmasked during a test:
its handler counts the VRTCs. The CPU draws the list's sprites in turn,
over and over, and counts every sprite finished in the 180 VRTCs (the SGP
program counts whole lists). It draws only: nothing is cleared between
the passes. The program needs 192 KB after itself.

The code and data come from main RAM where MS-DOS loads the program; on
the PC-88VA main RAM below 40000h has no programmed wait, so where MS-DOS
puts the program can change the results.

## MIXBENCH

`MIXBENCH.COM` (`mixbench.asm`) shares each list between the SGP and the
CPU, which draw at the same time. Instead of the sources it has three
SGP shares (the `SHARE` column): a quarter, a half and three quarters of
the list's sprites. The sprites go in blocks of four (one of each
character, in list order): block b to the SGP when b mod 4 is below the
share in quarters, so `SGP1/4` gives the SGP blocks 0, 4, 8 and so on.

A round starts the SGP on its part (`BITBLT 0105h`, sources in main RAM,
as SGPBENCH's `RAM`), then the CPU draws the rest with the compiled
routines (as CPUBENCH's `COMPLD`), then waits until the SGP is idle. Only
rounds that end inside the 180 VRTCs count, each with all the list's
sprites; the result is again sprites a VRTC, so its rows compare
directly with the other two programs' `RAM` and `COMPLD` rows. The SGP
slows the CPU's GVRAM writes while it draws (`demos/sgp-contention`:
on the machines about 0.73 times on the PC-88VA and 0.46 times on the
PC-88VA2), so the best share differs by machine and depth, and a share
can be slower than the CPU alone. Only the VRTC interrupt is unmasked
during a test. The program reads `CPU4.DAT`, `CPU8.DAT` and `CPU16.DAT`
as CPUBENCH does and needs 256 KB after itself.

vaeg's PC-88VA2 at 15 kHz, 320x200, for comparison (not measurements on
the machine):

| Colours | Way | 8x8 | 16x16 | 32x32 | 64x64 |
|---|---|---:|---:|---:|---:|
| 16 | CPUBENCH `COMPLD` | 193.1 | 82.3 | 29.2 | 9.5 |
| 16 | SGPBENCH `RAM` | 72.5 | 43.0 | 17.4 | 5.4 |
| 16 | `SGP1/4` | 200.5 | 88.5 | 31.8 | 10.3 |
| 16 | `SGP1/2` | 145.0 | 86.0 | 33.6 | 10.8 |
| 16 | `SGP3/4` | 96.7 | 57.2 | 23.2 | 7.2 |

## DBLBENCH

`DBLBENCH.COM` (`dblbench.asm`) measures what a game sees: frames on a
hidden page, shown at the next VRTC (FB0's display start address, DSA,
ports 20Eh and 210h, as `demos/sgp-pseudo-sprite` does for FB1). A frame
starts at a VRTC, erases the sprites drawn on that page two frames
earlier, draws them again, and must end before the next VRTC, where its
page is shown. The result is the most sprites at a frame every VRTC
(about 60 frames a second at 15 kHz), found by halving between 0 and
500: a trial of N sprites runs 30 frames and fails at the first frame
that ends late. The positions are the other programs' and do not move,
so each frame erases and redraws the same places; the cost is that of
moving sprites. The `WAY` column:

- `CPU`: two pages. The CPU clears the N rectangles (`REP STOSW` a line),
  then draws the N sprites with the compiled routines (as CPUBENCH's
  `COMPLD`).
- `SGP`: two pages. One SGP list clears the N rectangles (`BITBLT 0005h`,
  an opaque copy from 8 KB of zeros in main RAM, as `demos/sgp-contention`
  clears) and then draws the N sprites (`BITBLT 0105h` from main RAM).
- `MIX`: three pages, so that the two never touch the same page. While
  the CPU draws on the page the SGP cleared during the previous frame,
  the SGP clears the page shown during the previous frame; the frame ends
  when both have finished.

The pages must fit the 256 KB of GVRAM: 320x224 at 65536 colours has no
room for two pages, and `MIX` at 65536 colours none for three; those
cells are `-`. The program reads the `.DAT` files as CPUBENCH does and
needs 320 KB after itself; it takes about 5 minutes.

vaeg's values at 15 kHz, 320x200, sprites at a frame every VRTC (not
measurements on the machine):

| Machine | Colours | Way | 8x8 | 16x16 | 32x32 | 64x64 |
|---|---|---|---:|---:|---:|---:|
| PC-88VA2 | 16 | `CPU` | 101 | 43 | 15 | 4 |
| PC-88VA2 | 16 | `SGP` | 37 | 22 | 9 | 2 |
| PC-88VA2 | 16 | `MIX` | 76 | 46 | 17 | 5 |
| PC-88VA2 | 256 | `CPU` | 90 | 33 | 9 | 2 |
| PC-88VA2 | 256 | `SGP` | 32 | 16 | 5 | 1 |
| PC-88VA2 | 256 | `MIX` | 67 | 35 | 12 | 3 |
| PC-88VA | 16 | `CPU` | 101 | 43 | 15 | 4 |
| PC-88VA | 16 | `SGP` | 25 | 14 | 5 | 1 |
| PC-88VA | 16 | `MIX` | 51 | 30 | 12 | 3 |

## On the machines

[results.md](results.md) holds the four tables as a PC-88VA and a
PC-88VA2/VA3 showed them at 15 kHz, with vaeg's values for comparison.
In short: the two machines draw with the CPU at the same speed; a GVRAM
source makes the SGP faster on the PC-88VA2 and slightly slower on the
PC-88VA; and with double buffering the CPU alone draws the most sprites
on both.

## Building

```sh
demos/sprite-bench/build.sh [--msdos4 msdos4-pc88va-2hd.d88] [OUTDIR]
```

This writes `SGPBENCH.COM`, `CPUBENCH.COM`, `MIXBENCH.COM`,
`DBLBENCH.COM` and the three `.DAT` files. With `--msdos4` (the
FreeDOS-88VA msdos4-va.2 disk) the script also writes
`sprite-bench-msdos4.d88`, a vanilla MS-DOS 4.0 copy with all seven files
in its root; type `SGPBENCH`, `CPUBENCH`, `MIXBENCH` or `DBLBENCH`. `PREVIEW=chars.ppm` also writes
the characters at four times their size.
