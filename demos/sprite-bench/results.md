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

# Sprite benchmarks on the machines

The tables of the four programs as a PC-88VA and a PC-88VA2/VA3 showed
them at 15 kHz, from MS-DOS 4.0 (msdos4-va.2) and the disk that
`build.sh --msdos4` makes. The values were read from photographs of the
result screens. MIXBENCH's and DBLBENCH's tables have no gap between
their rows, so a few of their digits (0 and 8, 2 and 3) are less certain;
a value in parentheses is vaeg's for the same test before any correction,
for comparison (PC-88VA2 or PC-88VA as the table's machine).

## SGPBENCH (sprites a VRTC)

| Screen | Col. | Source | VA 8x8 | 16x16 | 32x32 | 64x64 | VA2 8x8 | 16x16 | 32x32 | 64x64 |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 320x200 | 16 | RAM | 51.2 | 28.0 | 10.8 | 3.2 | 76.0 | 44.4 | 17.7 | 5.5 |
| 320x200 | 16 | GVRAM | 50.4 | 27.3 | 10.6 | 3.2 | 78.9 | 48.0 | 20.0 | 6.3 |
| 320x200 | 256 | RAM | 42.6 | 19.2 | 6.4 | 1.7 | 64.0 | 30.9 | 10.4 | 2.9 |
| 320x200 | 256 | GVRAM | 41.9 | 18.8 | 6.2 | 1.6 | 67.5 | 34.1 | 11.9 | 3.3 |
| 320x200 | 65536 | RAM | 31.2 | 11.7 | 3.5 | 0.8 | 48.3 | 19.2 | 5.6 | 1.5 |
| 320x200 | 65536 | GVRAM | 30.5 | 11.3 | 3.3 | 0.8 | 51.9 | 21.3 | 6.4 | 1.6 |
| 320x224 | 16 | RAM | 52.6 | 28.8 | 11.2 | 3.3 | 78.2 | 45.5 | 18.3 | 5.6 |
| 320x224 | 16 | GVRAM | 51.9 | 28.0 | 10.8 | 3.2 | 81.0 | 49.0 | 20.4 | 6.4 |
| 320x224 | 256 | RAM | 43.3 | 19.9 | 6.5 | 1.8 | 65.4 | 31.6 | 10.6 | 3.0 |
| 320x224 | 256 | GVRAM | 42.6 | 19.5 | 6.4 | 1.7 | 69.6 | 34.8 | 12.2 | 3.4 |
| 320x224 | 65536 | RAM | 32.7 | 12.0 | 3.5 | 0.8 | 49.7 | 19.5 | 5.8 | 1.5 |
| 320x224 | 65536 | GVRAM | 32.0 | 11.7 | 3.3 | 0.8 | 53.3 | 21.6 | 6.5 | 1.7 |

vaeg before correction, 320x200, both sources: PC-88VA 48.3 / 27.0 /
10.3 / 3.1 (16), 41.2 / 18.4 / 6.2 / 1.6 (256), 31.2 / 11.7 / 3.3 / 0.8
(65536); PC-88VA2 72.5 / 43.0 / 17.4 / 5.4 (16).

On the PC-88VA a GVRAM source is 1 to 6 percent slower than one in main
RAM; on the PC-88VA2 it is 4 to 15 percent faster, the more the larger
the sprite and the depth.

## CPUBENCH (sprites a VRTC)

| Screen | Col. | Way | VA 8x8 | 16x16 | 32x32 | 64x64 | VA2 8x8 | 16x16 | 32x32 | 64x64 |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 320x200 | 16 | COMPLD | 179.6 | 77.9 | 27.7 | 8.8 | 180.8 | 78.4 | 27.9 | 9.0 |
| 320x200 | 16 | LOOP | 50.1 | 15.2 | 4.1 | 1.0 | 50.5 | 15.2 | 4.1 | 1.0 |
| 320x200 | 256 | COMPLD | 197.2 | 71.9 | 20.2 | 5.5 | 198.5 | 72.4 | 20.4 | 5.5 |
| 320x200 | 256 | LOOP | 37.1 | 10.2 | 2.6 | 0.6 | 37.4 | 10.2 | 2.6 | 0.6 |
| 320x200 | 65536 | COMPLD | 143.0 | 45.4 | 12.0 | 3.0 | 143.9 | 45.7 | 12.1 | 3.1 |
| 320x200 | 65536 | LOOP | 34.8 | 9.4 | 2.4 | 0.6 | 35.1 | 9.5 | 2.4 | 0.6 |
| 320x224 | 16 | COMPLD | 184.3 | 79.9 | 28.4 | 9.0 | 185.5 | 80.4 | 28.6 | 9.2 |
| 320x224 | 16 | LOOP | 51.4 | 15.6 | 4.2 | 1.0 | 51.8 | 15.6 | 4.2 | 1.0 |
| 320x224 | 256 | COMPLD | 202.4 | 73.8 | 20.8 | 5.6 | 203.8 | 74.4 | 20.9 | 5.7 |
| 320x224 | 256 | LOOP | 38.1 | 10.4 | 2.7 | 0.6 | 38.4 | 10.5 | 2.7 | 0.6 |
| 320x224 | 65536 | COMPLD | 146.9 | 46.6 | 12.4 | 3.1 | 147.8 | 46.9 | 12.4 | 3.2 |
| 320x224 | 65536 | LOOP | 35.8 | 9.7 | 2.5 | 0.6 | 36.0 | 9.8 | 2.5 | 0.6 |

The two machines draw with the CPU at the same speed (within 1 percent).
vaeg before correction, PC-88VA, 320x200 `COMPLD`: 193.1 / 82.3 / 29.2 /
9.4 (16), 215.0 / 76.6 / 21.4 / 6.0 (256), 166.1 / 53.6 / 14.3 / 3.6
(65536): 6 to 16 percent too fast, the most at 65536 colours.

## MIXBENCH (sprites a VRTC)

| Screen | Col. | Share | VA 8x8 | 16x16 | 32x32 | 64x64 | VA2 8x8 | 16x16 | 32x32 | 64x64 |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 320x200 | 16 | SGP1/4 | 189.1 | 82.8 | 29.3 | 9.1 | 184.8 | 82.4 | 29.6 | 9.4 |
| 320x200 | 16 | SGP1/2 | 100.2 | 54.0 | 20.0 | 6.3 | 147.9 | 82.4 | 30.9 | 9.6 |
| 320x200 | 16 | SGP3/4 | 67.5 | 36.2 | 13.0 | 4.1 | 100.2 | 57.9 | 22.9 | 7.1 |
| 320x200 | 256 | SGP1/4 | 164.9 | 70.4 | 20.0 | 5.4 | 189.8 | 71.1 | 20.6 | 5.6 |
| 320x200 | 256 | SGP1/2 | 83.2 | 36.9 | 12.0 | 3.3 | 126.5 | 59.0 | 19.7 | 5.6 |
| 320x200 | 256 | SGP3/4 | 55.4 | 24.8 | 8.1 | 2.2 | 84.6 | 40.5 | 13.6 | 3.8 |
| 320x200 | 65536 | SGP1/4 | 125.1 | 43.7 | 11.7 | 2.9 | 136.5 | 44.4 | 11.9 | 3.0 |
| 320x200 | 65536 | SGP1/2 | 61.8 | 23.1 | 6.7 | 1.7 | 98.1 | 38.7 | 11.5 | 3.0 |
| 320x200 | 65536 | SGP3/4 | 41.2 | 15.2 | 4.4 | 1.1 | 64.7 | 25.6 | 7.6 | 1.9 |
| 320x224 | 16 | SGP1/4 | 194.1 | 84.9 | 30.2 | 9.4 | 189.8 | 84.6 | 30.5 | 9.6 |
| 320x224 | 16 | SGP1/2 | 103.1 | 55.4 | 21.3 | 6.4 | 152.1 | 84.6 | 31.6 | 9.9 |
| 320x224 | 16 | SGP3/4 | 68.9 | 37.3 | 14.2 | 4.3 | 103.1 | 59.3 | 23.6 | 7.2 |
| 320x224 | 256 | SGP1/4 | 169.2 | 72.5 | 20.6 | 5.6 | 194.8 | 73.2 | 21.1 | 5.7 |
| 320x224 | 256 | SGP1/2 | 85.3 | 38.0 | 12.4 | 3.4 | 129.4 | 60.8 | 20.2 | 5.6 |
| 320x224 | 256 | SGP3/4 | 56.8 | 25.6 | 8.3 | 2.3 | 87.4 | 41.6 | 14.0 | 3.9 |
| 320x224 | 65536 | SGP1/4 | 128.7 | 45.1 | 12.0 | 3.0 | 140.0 | 45.5 | 12.2 | 3.1 |
| 320x224 | 65536 | SGP1/2 | 63.2 | 23.8 | 6.9 | 1.8 | 100.9 | 39.4 | 11.7 | 3.1 |
| 320x224 | 65536 | SGP3/4 | 41.9 | 15.6 | 4.6 | 1.1 | 66.8 | 26.3 | 7.8 | 2.0 |

With the SGP's share at a half or more, vaeg agrees within a few
percent. At `SGP1/4`, where the CPU's part is the longer, vaeg is too fast:
PC-88VA2 320x200 16 colours (200.5) / (88.5) / (31.8) / (10.3), PC-88VA
(194.8) / (97.4) / (34.8) / (11.2). On both machines `SGP1/4` is at most
6 percent faster than `COMPLD` alone at 16 colours, and slower than it at
256 and 65536 colours.

## DBLBENCH (most sprites at a frame every VRTC)

| Screen | Col. | Way | VA 8x8 | 16x16 | 32x32 | 64x64 | VA2 8x8 | 16x16 | 32x32 | 64x64 |
|---|---|---|---:|---:|---:|---:|---:|---:|---:|---:|
| 320x200 | 16 | CPU | 94 | 41 | 15 | 4 | 95 | 42 | 15 | 4 |
| 320x200 | 16 | SGP | 25 | 14 | 5 | 1 | 38 | 23 | 9 | 2 |
| 320x200 | 16 | MIX | 52 | 29 | 11 | 3 | 76 | 40 | 15 | 4 |
| 320x200 | 256 | CPU | 88 | 33 | 9 | 2 | 88 | 33 | 10 | 2 |
| 320x200 | 256 | SGP | 21 | 9 | 3 | 0 | 32 | 16 | 5 | 1 |
| 320x200 | 256 | MIX | 43 | 20 | 6 | 1 | 66 | 32 | 9 | 2 |
| 320x200 | 65536 | CPU | 71 | 24 | 6 | 1 | 72 | 24 | 6 | 1 |
| 320x200 | 65536 | SGP | 15 | 6 | 1 | 0 | 25 | 10 | 3 | 0 |
| 320x200 | 65536 | MIX | - | - | - | - | - | - | - | - |
| 320x224 | 16 | CPU | 97 | 42 | 15 | 4 | 98 | 43 | 15 | 4 |
| 320x224 | 16 | SGP | 26 | 14 | 5 | 1 | 39 | 23 | 9 | 3 |
| 320x224 | 16 | MIX | 53 | 29 | 11 | 3 | 78 | 41 | 15 | 4 |
| 320x224 | 256 | CPU | 90 | 34 | 10 | 2 | 91 | 34 | 10 | 2 |
| 320x224 | 256 | SGP | 22 | 10 | 3 | 0 | 33 | 16 | 5 | 1 |
| 320x224 | 256 | MIX | 45 | 20 | 6 | 1 | 68 | 33 | 10 | 2 |
| 320x224 | 65536 | all | - | - | - | - | - | - | - | - |

On both machines `CPU` draws the most sprites at every size and depth;
`MIX` reaches it only at 32x32 and 64x64 on the PC-88VA2. vaeg before
correction had the PC-88VA2's `MIX` above `CPU` at 16x16 to 64x64
(16 colours (76) / (46) / (17) / (5)) and its `CPU` at (101) / (43) /
(15) / (4) on both machines.

## vaeg after M165's corrections

M165 corrected vaeg's timing to these tables and to SGPMEM
(`demos/sgp-contention`): straight-line instruction fetch, the SGP's share
of the bus and its dependence on the SGP's source, the SGP's word cost and
its GVRAM source, TVRAM byte waits. The model, the full comparison and what
is still open are in
[va-bus-timing.md](../../docs/modernization/va-bus-timing.md). vaeg at 15
kHz, 320x200, PC-88VA (the PC-88VA2 the same where the machines are):

| Program | Row | 8x8 | 16x16 | 32x32 | 64x64 | Machine |
|---|---|---:|---:|---:|---:|---|
| CPUBENCH | 16 `COMPLD` | 183.8 | 77.0 | 27.0 | 8.7 | 179.6 / 77.9 / 27.7 / 8.8 |
| CPUBENCH | 256 `COMPLD` | 205.2 | 71.3 | 19.7 | 5.4 | 197.2 / 71.9 / 20.2 / 5.5 |
| CPUBENCH | 65536 `COMPLD` | 154.5 | 48.5 | 12.8 | 3.3 | 143.0 / 45.4 / 12.0 / 3.0 |
| CPUBENCH | 16 `LOOP` | 54.3 | 16.5 | 4.5 | 1.1 | 50.1 / 15.2 / 4.1 / 1.0 |
| SGPBENCH | 16 `RAM` | 51.9 | 28.0 | 10.6 | 3.1 | 51.2 / 28.0 / 10.8 / 3.2 |
| SGPBENCH VA2 | 16 `RAM` | 76.8 | 44.8 | 17.7 | 5.4 | 76.0 / 44.4 / 17.7 / 5.5 |
| SGPBENCH VA2 | 16 `GVRAM` | 78.9 | 48.0 | 19.9 | 6.2 | 78.9 / 48.0 / 20.0 / 6.3 |
| MIXBENCH | 16 `SGP1/4` | 199.8 | 83.2 | 28.9 | 9.2 | 189.1 / 82.8 / 29.3 / 9.1 |
| MIXBENCH VA2 | 16 `SGP1/4` | 188.4 | 81.0 | 28.6 | 9.0 | 184.8 / 82.4 / 29.6 / 9.4 |
| MIXBENCH VA2 | 16 `SGP1/2` | 152.8 | 81.0 | 29.6 | 9.2 | 147.9 / 82.4 / 30.9 / 9.6 |
| DBLBENCH | 16 `CPU` | 98 | 41 | 14 | 4 | 94 / 41 / 15 / 4 |
| DBLBENCH | 16 `MIX` | 54 | 31 | 12 | 3 | 52 / 29 / 11 / 3 |
| DBLBENCH VA2 | 16 `MIX` | 79 | 42 | 15 | 4 | 76 / 40 / 15 / 4 |

## vaeg at 24.8 kHz (for a measurement on the machines)

vaeg after the corrections, 320x200 (the 320x224 rows are `-` at 24.8
kHz). PC-88VA / PC-88VA2:

| Program | Row | 8x8 | 16x16 | 32x32 | 64x64 |
|---|---|---:|---:|---:|---:|
| SGPBENCH | 16 `RAM` | 56.1 / 83.2 | 30.5 / 48.7 | 11.5 / 19.3 | 3.3 / 5.9 |
| SGPBENCH | 16 `GVRAM` | 55.4 / 86.0 | 29.8 / 52.2 | 11.2 / 21.6 | 3.2 / 6.8 |
| SGPBENCH | 256 `RAM` | 45.5 / 69.6 | 20.6 / 33.7 | 6.5 / 11.3 | 1.7 / 3.1 |
| SGPBENCH | 65536 `RAM` | 33.4 / 52.6 | 12.0 / 20.6 | 3.5 / 6.0 | 0.8 / 1.6 |
| CPUBENCH | 16 `COMPLD` | 200.2 | 83.8 | 29.4 | 9.5 |
| CPUBENCH | 256 `COMPLD` | 223.5 | 77.7 | 21.5 | 5.9 |
| CPUBENCH | 65536 `COMPLD` | 168.3 | 52.8 | 13.9 | 3.6 |
| MIXBENCH | 16 `SGP1/4` | 217.6 / 205.5 | 90.6 / 88.1 | 31.6 / 31.2 | 10.0 / 9.8 |
| MIXBENCH | 16 `SGP1/2` | 112.3 / 167.1 | 61.1 / 88.5 | 23.1 / 32.3 | 6.8 / 10.1 |
| DBLBENCH | 16 `CPU` | 107 | 45 | 16 | 4 |
| DBLBENCH | 16 `SGP` | 28 / 42 | 16 / 25 | 6 / 10 | 1 / 3 |
| DBLBENCH | 16 `MIX` | 58 / 86 | 33 / 46 | 13 / 17 | 4 / 5 |

CPUBENCH is the same on both models. A VRTC is longer at 24.8 kHz (56.4
Hz against 61.5 Hz), so every value is about 9% higher than at 15 kHz.
