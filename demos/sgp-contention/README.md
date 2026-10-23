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

# SGP and CPU contention, two screens

`SGPCONT.COM` (`sgpcont.asm`), an MS-DOS program for a PC-88VA or
PC-88VA2 (and vaeg), measures the main CPU and the SGP working at the
same time, on the two-screen display an arcade-game port uses for its
sprites: G0 in front of G1, both 320 dots packed 4 bpp, G0 with palette
set 1 and colour 0 transparent, G1 with set 0. Synthetic data only. It
repeats the loops, segments and SGP lists of the test build of that port
(GG's BENCH2) which was photographed on both machines.

Each row is what was done in 600 VRTCs (10 s; more is faster):

| Row | Work |
|---|---|
| A | CPU loop, 1024 iterations of `ADD AX,BX` |
| B | main RAM (7280h) AND-OR, 1024 bytes |
| C-G | G0 (A000h): read, write, AND-OR, `REP STOSW` 1024 bytes, `REP MOVSW` from main RAM |
| H-K | G1 (C000h): read, write, AND-OR, `REP STOSW` |
| L | SGP lists of 30 16x16 sprites drawn into G0 (BITBLT 0105h, source 0 transparent, sources in main RAM at 74800h); the CPU waits |
| M, N | the same lists (M) while the CPU repeats B in chunks of 64 iterations (N, in blocks of 1024) |
| O, P | the same lists (O) while the CPU writes G1 (P, as I) |
| Q, R | 30 16x16 rectangles of G0 cleared by the CPU (Q) and by the SGP (R, BITBLT 0005h from zeros) |

At the end the sprites are drawn over a striped G1 band on the right:
the stripes must show through the sprites' holes in their own colours.
The program needs a 15 kHz screen and memory up to segment 7500h, copies
itself to 2000:0000 and does not return: restart the machine to leave.
It takes about 3 minutes.

```sh
demos/sgp-contention/build.sh [--msdos4 msdos4-pc88va-2hd.d88] [OUTDIR]
```

`gen_font.py` writes the font and palettes. With `--msdos4` (the
FreeDOS-88VA msdos4-va.2 disk) the script also writes
`sgp-contention-msdos4.d88`, a vanilla MS-DOS 4.0 copy with the program;
type `SGPCONT`.

## Results (2026-10-23)

GG's BENCH2 on the machines (same loops; its VRTC handler does a little
more work than this program's), and SGPCONT in vaeg after M130:

| Row | VA | VA2 | vaeg VA | vaeg VA2 |
|---|---:|---:|---:|---:|
| A CPU | 3466 | 3495 | 2994 | 2994 |
| B RAM AND OR | 1491 | 1471 | 1471 | 1528 |
| C G0 READ | 2074 | 2083 | 1947 | 1947 |
| D G0 WRITE | 2429 | 2454 | 2431 | 2431 |
| E G0 AND OR | 1397 | 1400 | 1392 | 1392 |
| F STOSW G0 | 23950 | 24153 | 23204 | 23204 |
| G MOVSW RAM G0 | 13751 | 15224 | 13765 | 15251 |
| H G1 READ | 1865 | 1866 | 1947 | 1947 |
| I G1 WRITE | 2428 | 2452 | 2432 | 2432 |
| J G1 AND OR | 1347 | 1371 | 1321 | 1321 |
| K STOSW G1 | 23950 | 24153 | 23204 | 23204 |
| L SGP 30 SPRITES | 562 | 908 | 597 | 944 |
| M SGP WITH RAM | 566 | 900 | 596 | 943 |
| N RAM WITH SGP | 990 | 706 | 1080 | 766 |
| O SGP WITH G1 | 568 | 895 | 592 | 934 |
| P G1 WITH SGP | 1774 | 1118 | 1702 | 1167 |
| Q CPU CLEAR 30 | 2463 | 2488 | 2583 | 2583 |
| R SGP CLEAR 30 | 616 | 989 | 661 | 1035 |

Row A depends on where the loop lies (prefetch): GG's build gives 3519 in
vaeg on both models. GG's build in vaeg after M130 gives N 1074 / 762 and
P 1690 / 1161 (VA / VA2); before the contention model, 1229 / 1111 and
1896 / 1653.

- Two screens cost the CPU nothing: G0 and G1 are as fast as the single
  screen of GG's BENCH (GV read 1865, write 2428, AND-OR 1347 on the VA).
- While the SGP draws, the CPU loses about 0.31 of its time on the VA
  (N/B 0.66, P/I 0.73) and 0.53 on the VA2 (0.48, 0.46); the SGP is not
  slowed by the CPU (M, O against L). SGPBENCH and SPRBENCH give the same
  shares on the VA2 for a compute loop and for 16- to 128-dot sprites.
- The CPU clears 16x16 rectangles about four times (VA) and three times
  (VA2) as fast as the SGP.

SGPCONT itself has not yet been run on the machines; its photographs
would replace the first two columns.
