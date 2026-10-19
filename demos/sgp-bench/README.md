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

# SGP and sprite benchmarks

Two MS-DOS programs that measure the SGP and the main CPU on a PC-88VA or
PC-88VA2 (and in vaeg), with synthetic data:

- `SGPBENCH.COM` (`sgpbench.asm`): seventeen timings (CPU loop and fills,
  SGP empty list, CLS, 16x16 and 8x8 BITBLTs, CPU and SGP together) and 48
  BITBLT correctness cases with clipping, start dots, transparency and
  upward transfers; SPACE switches the result (front A) and the expected
  image (front B). Used for M128.
- `SPRBENCH.COM` (`sprbench.asm`): transparent square sprites of 16, 32,
  64 and 128 dots in 16 colours (two packed 4-bpp screens) and 256 colours
  (one 8-bpp screen), drawn by the CPU only (mask AND, data OR, word
  writes), by the SGP only (BITBLT 0105h from main RAM), and shared (25,
  50 or 75 of 100 sprites on one SGP list, the rest by the CPU at the same
  time). Microseconds a sprite, mean of two runs of 240 VRTCs, timed below
  a VRTC with a spin loop calibrated against it; the largest difference
  between the runs is shown. One sprite each by the SGP and the CPU is
  compared (`CHECK SGP=CPU`).

Both need a 15 kHz screen and memory up to segment 5000h, copy themselves
to 3000:0000 and do not return: restart the machine to leave. SGPBENCH
takes about 3.5 minutes, SPRBENCH about 6.

```sh
demos/sgp-bench/build.sh [--msdos4 msdos4-pc88va-2hd.d88] [OUTDIR]
```

`gen_data.py` writes the data includes (cases, expected image, font,
palette). With `--msdos4` (the FreeDOS-88VA msdos4-va.2 disk) the script
also writes `sgp-bench-msdos4.d88`, a vanilla MS-DOS 4.0 copy with both
programs; type `SGPBENCH` or `SPRBENCH`.

## SPRBENCH on hardware (2026-10-19)

Photographs: `docs/modernization/m129-photos/`. Microseconds a sprite;
both machines' two runs differ by at most 0.01 %; all checks OK.

PC-88VA:

| Colours, size | CPU | SGP25 | SGP50 | SGP75 | SGP |
|---|---:|---:|---:|---:|---:|
| 16, 16 | 642.8 | 521.4 | 398.5 | 445.2 | 591.8 |
| 16, 32 | 2428.6 | 1915.0 | 1397.6 | 1156.4 | 1535.7 |
| 16, 64 | 9538.4 | 7443.9 | 5342.4 | 3790.9 | 5021.5 |
| 16, 128 | 37847.2 | 29436.3 | 21011.0 | 13899.1 | 18412.7 |
| 256, 16 | 1228.0 | 975.9 | 722.0 | 647.5 | 859.1 |
| 256, 32 | 4785.6 | 3742.6 | 2696.2 | 1966.3 | 2604.7 |
| 256, 64 | 18941.9 | 14741.8 | 10534.3 | 7025.4 | 9310.7 |
| 256, 128 | 75505.7 | 58658.9 | 41797.8 | 26825.1 | 35539.3 |

PC-88VA2:

| Colours, size | CPU | SGP25 | SGP50 | SGP75 | SGP |
|---|---:|---:|---:|---:|---:|
| 16, 16 | 621.9 | 508.9 | 394.0 | 280.8 | 369.5 |
| 16, 32 | 2344.5 | 1858.4 | 1368.6 | 879.0 | 921.8 |
| 16, 64 | 9176.4 | 7207.2 | 5229.8 | 3253.2 | 2981.9 |
| 16, 128 | 36572.7 | 28633.7 | 20679.3 | 12725.3 | 10921.5 |
| 256, 16 | 1187.0 | 950.2 | 711.4 | 472.7 | 530.2 |
| 256, 32 | 4601.8 | 3623.6 | 2642.1 | 1660.6 | 1560.7 |
| 256, 64 | 18300.1 | 14337.7 | 10367.8 | 6398.8 | 5532.2 |
| 256, 128 | 73077.4 | 57158.3 | 41222.2 | 25286.1 | 21171.4 |

Hardware divided by vaeg (main after M128):

| Colours, size | VA: CPU | SGP50 | SGP | VA2: CPU | SGP50 | SGP75 | SGP |
|---|---:|---:|---:|---:|---:|---:|---:|
| 16, 16 | 1.62 | 1.42 | 1.06 | 1.56 | 1.56 | 1.06 | 1.05 |
| 16, 32 | 1.56 | 1.43 | 1.05 | 1.51 | 1.40 | 1.28 | 1.05 |
| 16, 64 | 1.55 | 1.38 | 1.03 | 1.49 | 1.35 | 1.20 | 1.05 |
| 16, 128 | 1.54 | 1.36 | 1.01 | 1.49 | 1.34 | 1.18 | 1.04 |
| 256, 16 | 1.57 | 1.47 | 1.04 | 1.52 | 1.45 | 1.24 | 1.04 |
| 256, 32 | 1.55 | 1.39 | 1.02 | 1.49 | 1.37 | 1.22 | 1.04 |
| 256, 64 | 1.54 | 1.37 | 1.01 | 1.49 | 1.35 | 1.19 | 1.04 |
| 256, 128 | 1.53 | 1.36 | 1.00 | 1.48 | 1.34 | 1.17 | 1.03 |

The SGP alone agrees within 6 % (M128). The CPU's sprite loop (`LODSW`,
`AND AX,[ES:DI]`, `OR AX,[SI]`, `STOSW` a word: main-RAM and GVRAM reads
and a GVRAM write) is 1.48-1.62 times slower on both machines than in
vaeg, and the shared columns inherit it. See M129 and issue #38.
