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

# TSP hardware sprites over two screens

`TSPSPR.COM` (`tspspr.asm`), an MS-DOS program for a PC-88VA or PC-88VA2
(and vaeg), shows the TSP's hardware sprites over the two-screen display
an arcade-game port for the VA uses: G0 in front of G1, both 320 dots
packed 4 bpp at 15.98 kHz and 224 lines, G0 with palette set 1 and colour
0 transparent, G1 and the sprites with set 0. Synthetic images only: the
aliens and the shot are drawn in `gen_data.py`.

The TSP's colours 1-15 are the sprite class (port 0110h), all but 0
opaque (012Eh); sprite 31, the BIOS's text cursor, is not used and CURDEF
turns the cursor off. The program sends SYNC (as the port) and SPRON but
no DSPON: D sends DSPON once, should the sprites need the text display.
Sprite images are in TVRAM's character side from 1000h, the descriptor
table at 7E00h.

| Screen | What it shows | Keys |
|---|---|---|
| 1 POSITION | white sprite outlines over red G1 outlines drawn where a sprite is expected with X = 2 x screen dot and Y = line. Aligned, no red shows. The first line gives the first box's dot, line and the X and Y written. Boxes cross the left, right and bottom edges, and two cross the top by 8 lines: the right one with its image address advanced by 8 lines (the manual's rule), the other without | arrows: the sprites by one sprite dot (half a dot) or one line |
| 2 ORDER | aliens and chequered sprites over G1's blue stripes, grey G0 bars across them; order 1 G0 > sprites > G1, 2 sprites > G0 > G1, 3 G0 > G1 > sprites (port 0106h) | UP or DOWN: the order |
| 3 LINE | N sprites (31 at first) on the same 16 lines (from line 120), each with G1 marks of its colour above and below where its left edge should be, and every fifth sprite's number under it: a sprite missing shows its marks only, so one photograph of each kind at N = 31 shows how many a line takes. Sprite 0 (in front) is on the right; each shows at least its left part. Kinds, by bytes of image a line: C16 (16 colours, 16 sprite dots: a shot, 8 bytes), C32 (32 dots: a 16-dot alien, 16 bytes), C64 (two aliens, 32 bytes), M32 (one colour, 32 dots, 4 bytes), M64 (8 bytes), M128 (16 bytes). The first line gives SPROV (bit 6 sprite over, bit 5 collision, bits 4-0 the first sprite over; FF: no result) and the TSP status's SC (20) at each VRTC | LEFT, RIGHT: N (1-31); UP or DOWN: the kind; H: HSPN 31, 15, 7, 3; A: the sweep, N 1-31 of each kind in turn, 120 VRTCs a step (about 6 minutes; for a film), A again to stop |
| 4 MOVE | N sprites (24 at first) moving, the descriptors written at each VRTC | as on 3 |
| 5 MEASURE | the rows below in turn, each pass, [P] passes (8 by default: about 15 minutes); for each row its mean, minimum, maximum and standard deviation (tenths) so far, the pass in the title line. The sprites stand on lines 208-223 | |

Screen 5's rows (a value is 1024-byte blocks in 300 VRTCs, more is
faster, unless said otherwise; B a byte loop of MOV, W REP MOVSW from
main RAM; OFF: SPROFF, 31: 31 C32 aliens shown, 8: 8 C32, 31M32 and
31C64: 31 of those kinds):

| Row | Work |
|---|---|
| A | CPU loop (ADD, DEC, JNZ), the clock's yardstick |
| B | main RAM to main RAM, W |
| C, D | G1 (GVRAM, one wait), B and W |
| E-H | TVRAM B writes and reads, OFF and 31 |
| I, J | TVRAM W writes, OFF and 31 |
| K | the attribute side (A8000h-), W, 31 |
| L | TVRAM to main RAM, W, 31 |
| M | REP STOSW to TVRAM, 31 |
| N-P | W writes with 8 C32, 31 M32, 31 C64 (the TSP's own fetches) |
| Q, R | 128-byte W chunks done only while the TSP status's VB is set, TVRAM and G1 |
| S | the vertical blank's share of a frame, thousandths (status polls) |
| T | SPRON commands (4 bytes each, waiting on the status) |
| U-X | J, F, Q and S with the text display on (DSPON), after the other rows' passes |

`TSPSPR P` sets the passes (1-99). SPACE goes to the next screen, after 5
to 1 again. The arrows held repeat (after 24 VRTCs, every 4); screen 1's
first line ends with KEY, the keys read (bits: 01 left, 02 right, 04 up,
08 down, 10 SPACE, 20 D, 40 H, 80 A). Every title line ends with the machine,
from the ROM's word at F000:FFFE: VA (FFFFh), VA2/3 (FFFEh), VA+91
(FFFDh, the PC-88VA-91 board), or ? and the word. J, L, I and K work as
the arrows. The program needs a 15 kHz screen and memory up to segment
3000h, copies itself to 2000:0000 and does not return: restart the machine
to leave.

```sh
demos/tsp-sprites/build.sh [--msdos4 msdos4-pc88va-2hd.d88] [OUTDIR]
```

`gen_data.py` writes the font, the palettes and the images. With
`--msdos4` (the FreeDOS-88VA msdos4-va.2 disk) the script also writes
`tsp-sprites-msdos4.d88`, a vanilla MS-DOS 4.0 copy with the program;
type `TSPSPR`. `tsp-sprites-msdos4.d88.xz` is that disk, built from this
directory (the FreeDOS-88VA msdos4-va.2 disk, MIT, with TSPSPR.COM added).

## Questions for the machines

- Where a sprite lands against the graphics (screen 1), and how the top
  edge is clipped.
- Whether the order G0 > sprites > G1 holds (screen 2).
- How many sprites of each kind one line takes before sprites go missing,
  how the picture breaks beyond, and what SPROV reports (screens 3, 4).
  The manual limits a line by its memory accesses (descriptors, images,
  text, the CPU's, commands), about 200-odd 16-colour dots: the six kinds
  separate a cost a sprite from a cost a byte (M64 and C16, M128 and C32
  have the same bytes a line), and D (the text display on) shows what
  the text's accesses take.
- What a TVRAM access costs the CPU (screen 5): the manual gives at least
  2 waits, normally 7-13; by bytes and by words, reads and writes, with
  no, 8 or 31 sprites of each weight, with the text display on, and in
  the vertical blank only; and how much these vary from pass to pass.
- How long the vertical blank is, and what a SPRON (a table switch at a
  VRTC) costs.

vaeg (main, 2026-10-26) draws every sprite of a line (no per-line limit)
and returns no SPROV result (FF). Screen 5, 2 passes, the same on the VA
and VA2 models (SD 0.0-0.7):

| A | B | C | D | E | F | G | H | I | J | K | L |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 1768 | 8436 | 1216 | 7528 | 1246 | 1246 | 1230 | 1230 | 8133 | 8134 | 8256 | 8207 |

| M | N | O | P | Q | R | S | T | U | V | W | X |
|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 15667 | 8134 | 8134 | 8134 | 1505 | 1505 | 21 | 21034 | 8134 | 1246 | 1505 | 21 |

TVRAM has no wait (E-P as fast as main RAM, faster than G1), sprites and
the text display cost nothing, and the VB bit lasts 2.1 % of a frame (S),
where a 224-line picture leaves about 38 of about 262 lines (about 14 %).
