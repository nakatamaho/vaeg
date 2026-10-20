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

# M129 — SGP and sprite benchmarks as a demo

Status: **implemented on `topic/m129-sgp-sprite-bench`; G129 pending.**

## Request

The maintainer (2026-10-19): add the sprite benchmark (SPRBENCH) with its
source under `demos/`, by pull request, and file an issue stating that
vaeg should give the hardware results.

## Work

- `demos/sgp-bench/`: `sgpbench.asm` (SGPBENCH 3, the M128 evidence),
  `sprbench.asm` (SPRITE BENCH 2), `gen_data.py` (data includes),
  `build.sh` (both programs, optionally an MS-DOS 4.0 disk from the
  msdos4-va.2 image with `tools/pc88va/pcengine_disk.py`), `README.md`
  (use and the hardware results). Synthetic data only. Both programs as
  built here are byte-identical to the ones photographed on hardware
  (SGPBENCH.COM SHA-256 `62c429d2...`, SPRBENCH.COM `cb8d6c94...`).
- Hardware results of SPRBENCH on the PC-88VA and PC-88VA2 recorded in the
  README, photographs in `docs/modernization/m129-photos/`.
- The demo is not on the all-demos disks (their committed payloads stay
  unchanged); it has no `build-d88.sh`.

## Findings

- `[MEASURED]` CPU sprite drawing (per word: main-RAM read of mask and
  data, GVRAM read and write) is 1.48-1.62x slower on both machines than
  in vaeg; the VA2's CPU is only about 3 % faster than the VA's here. SGP
  alone: within 0-6 % of vaeg (M128 calibration).
- `[MEASURED]` CPU and SGP together: on the VA the shared columns are close
  to full overlap (16 colours, 128 dots, SGP75: 13899 us against the SGP
  share alone, 0.75 x 18413 = 13810); on the VA2 they slow each other
  (12725 against max(0.25 x 36573, 0.75 x 10922) = 9143).
- Defect: issue (CPU memory access timing), see "Issue" below.

## Verification

- `build.sh --msdos4` from the worktree: both programs byte-identical to
  the photographed ones; the disk boots in vaeg (VA model, main after
  M128), `SPRBENCH` typed at the prompt runs to its end: all checks OK, no
  SGP timeout, 16 colours 16 dots CPU 397.7 / SGP 558.5 us (hardware
  642.8 / 591.8). SGPBENCH in vaeg fails the same 24 cases as hardware
  (M128).
- Repository checkers.

## Gate G129 (human)

Standard V3 gate, plus: `demos/sgp-bench/build.sh --msdos4 ...` builds the
disk, and SPRBENCH runs in vaeg on both models.
