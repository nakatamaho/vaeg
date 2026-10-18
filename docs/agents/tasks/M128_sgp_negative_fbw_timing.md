<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
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

# M128 — SGP: undefined negative pitches and measured command costs

Status: **implemented on `topic/m128-sgp-negative-fbw`; G128 pending.**

## Request

The maintainer (2026-10-19): a PC-88VA port of an arcade game drew every
vertically reversed sprite garbled on the PC-88VA and the PC-88VA2, and ran
much slower than in vaeg, which showed neither. vaeg must reproduce the
hardware, with documentation. Evidence: SGPBENCH 2, a benchmark disk of
that port (MS-DOS 4.0, `SGPBENCH.COM`, synthetic data only; source in the
port's repository, `src/bench/sgpbench.asm`), photographed on both
machines (`docs/modernization/m128-photos/`).

## Findings

- `[MEASURED]` Correctness: of 48 BITBLT cases (16x16 packed 4bpp sources
  from main RAM, BITBLT 0105h or 0005h, clipping, every destination start
  dot), every case read upwards with the source FBW **FFF6h** (-8 as vaeg
  decodes it) is garbled on both machines (mostly colour 15 with a few
  dark rows); all other cases equal the expected image. vaeg (M109a
  decoding) drew them correctly. Together with SGPTEST T07 (FFF2h) and T09
  (FFF0h), which vary between runs and models (M109a), negative pitches
  whose low two bits are 10b, and FFF0h, do not step through the source on
  hardware; FFF3h and FFF4h (T06, T08) agree with vaeg.
- `[MEASURED]` Timing, microseconds per repetition (480 VRTCs), hardware
  against vaeg before M128:

| Test | VA hardware | vaeg VA | VA2 hardware | vaeg VA2 |
|---|---:|---:|---:|---:|
| CPU LOOP x1000 | 2260.2 | 2133.1 | 2260.2 | 2133.1 |
| CPU REP STOSW 35840 B to GVRAM | 13830.7 | 9225.8 | 13712.3 | 9225.8 |
| CPU REP STOSW 35840 B to RAM | 11952.2 | 6921.3 | 9499.4 | 6921.3 |
| CPU REP MOVSW 8192 B GVRAM to GVRAM | 8564.7 | 4783.7 | 8537.3 | 4783.7 |
| SGP empty list (start to idle) | 76.6 | 32.8 | 76.4 | 32.8 |
| SGP CLS 35840 B | 27613.9 | 13481.5 | 11934.4 | 9018.0 |
| SGP CLS 1280 B | 1075.6 | 526.8 | 508.6 | 363.8 |
| SGP 1 x 16x16 | 656.9 | 387.0 | 438.8 | 213.0 |
| SGP 32 x 16x16 | 18710.3 | 11863.7 | 11759.2 | 5958.3 |
| same, destination dot 1 | 18710.3 | 12512.5 | 11759.2 | 6275.8 |
| same, opaque (0005h) | 17002.1 | 10720.2 | 10792.4 | 5392.6 |
| SGP 32 x 8x8 | 10188.3 | 6815.3 | 6867.9 | 3425.1 |
| SGP 32 x 16x16, source in GVRAM | 18842.4 | 11863.7 | 10925.0 | 5958.3 |
| 32 x 16x16 started, then CPU STOSW GVRAM | 20120.6 | 12937.0 | 20273.4 | 12916.1 |
| same, CPU STOSW RAM | 18580.1 | 11863.7 | 16177.8 | 10620.7 |
| same, CPU LOOP x4000 | 18409.2 | 12115.0 | 14968.2 | 12115.0 |

  The SGP is about 1.6x (VA) and 2.0x (VA2) slower than vaeg for BITBLT,
  2.0x / 1.3x for CLS; the VA2's SGP is not twice the VA's (BITBLT 1.6x,
  CLS 2.3x). On the VA the CPU and the SGP overlap almost completely (the
  CPU's work hides under the SGP's); on the VA2 they slow each other more.
  The CPU's memory writes are also 1.5-1.7x slower than in vaeg (plain
  loops 6%): a separate main-CPU timing defect, not addressed here.

## Work

- Undefined pitches: a negative source FBW with low bits 10b, or FFF0h, is
  marked when decoded and its source reads give FFFFh, visibly wrong as on
  the machines (not a model of their output, which is not reproducible);
  the line step is unchanged. Selftest: FFF4h is -10, FFF6h marked with a
  step of -8, FFF2h/FFF0h marked, FFF3h/FFF4h/0006h/000Eh not.
  eebbefe9.
- Costs: the nominal charges are multiplied per model and class (setup:
  command fetch and descriptors; BITBLT/PATBLT words and lines; CLS
  words), fractions carried; LINE and SCAN unchanged (no measurement).
  Factors (x/256): VA 370 / 432 / 525, VA2 541 / 497 / 443, fitted to the
  16x16 and 8x8 BITBLT and the CLS rows.
  bbb5456d.

## Verification

- SGPBENCH 2 in vaeg after M128, vaeg / hardware: VA 16x16 1.012, 8x8
  1.022, opaque 1.000, CLS 35840 1.000, CLS 1280 0.962, GVRAM source
  1.005; VA2 1.012, 1.018, 0.999, 0.978, 0.907, 1.089. Destination dot 1
  remains 6-7% slower in vaeg (equal on hardware). The empty list (0.48)
  and the CPU-overlap rows on the VA2 (0.64-0.81) reflect the CPU timing
  defect above.
- The 28 cases read upwards with FFF6h now fail in vaeg as on hardware;
  the other 20 match.
- `--selftest` passes; `ctest` (linux-ci-gcc) 94 of 94; repository
  checkers pass.

## Gate G128 (human)

Standard V3 gate, plus: SGPBENCH 2 in vaeg's VA and VA2 modes shows the
upward cases garbled and timings close to the photographs.
