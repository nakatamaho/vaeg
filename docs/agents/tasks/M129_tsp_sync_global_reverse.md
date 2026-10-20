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

# M129 — TSP SYNC global reverse (RV)

Status: **implemented; G129 human gate pending.**

Branch `topic/m129-tsp-sync-reverse`; commit prefix `M129:`.

## Request

[PR #31](https://github.com/nakatamaho/vaeg/pull/31) (shitamo): vaeg ignored
uPD72022 `SYNC` byte 1 bit 3 (RV), the whole-screen text reverse, which the
VA ROMs set in V1/V2 mode for the uPD3301 START DISPLAY reverse bit (21h);
the PC-8801mkIISR demonstration's CARIGRAPH was shown with black and white
swapped.

The maintainer (2026-10-21): on PC-88VA and PC-88VA2 hardware CARIGRAPH is
black lines on white, as the PR says, but a PC-8801mkIISR recording shows it
white on black, so the two machines differ. A VA boot must behave like the
VA; a machine started from `模擬 -> 起動 -> PC-8801/8001` like the SR. Take
the PR's commit with its authorship, add the machine condition, a selftest
and the ledger entry, then close the PR with thanks. The PC-8001 family
always applies RV.

The blank white function keys the VA shows before the demonstration's title
are tracked separately in
[issue #50](https://github.com/nakatamaho/vaeg/issues/50).

## Findings

- `[MEAS]` vaeg trace, V2 boot of the demonstration: just before CARIGRAPH
  it executes `OUT (C),A` with A = 21h to port 51h; the VA ROM's trap
  handler then issues `SYNC` with byte 1 = 5Fh (RV set; 57h otherwise).
  Port 31h is 29h/2Fh: bit 4 clear, monochrome graphics. `OUT &H51,&H21`
  from N88-BASIC gives the same SYNC on the VA and VA2 ROMs, and
  `OUT &H51,&H20` clears it.
- `[MEAS]` (maintainer) PC-88VA and PC-88VA2 hardware: CARIGRAPH black lines
  on white. PC-8801mkIISR (recording): white lines on black.
- QUASI88 applies the 3301 reverse only while port 31h selects colour
  graphics; MAME `pc8001` flips every cell's reverse.
- Private references (PC-Techknow8801, the mkII/SR analysis manual, the
  machine-language graphics book, the N88-BASIC analysis manual) mention
  `OUT 81,33` reversing the screen but not its interaction with monochrome
  graphics.

## Changes

- [73b4ce80](https://github.com/nakatamaho/vaeg/commit/73b4ce80539870e8549b703f1f7340c31e636c93)
  (author shitamo, PR #31): RV reverses each text cell on top of its own
  reverse attribute in `makeline` and `makeline_3301`.
- [cfef554d](https://github.com/nakatamaho/vaeg/commit/cfef554dd6404eda761fee2336705c7c8e8a08ba):
  `memoryva_pc8801` marks a PC-8801 model from `extra/` (not the PC-88VA/VA2
  sets); such a model ignores RV in V1/V2 mode while port 31h bit 4 is
  clear. Selftest `TSP SYNC reverse`.
- V1/V2 decision X9, the TSP and PC-8801 machine documents, and the ledger.

## Gate G129 (human)

Standard V3 gate, plus: the PC-8801mkIISR demonstration's CARIGRAPH is black
on white booted as VA and VA2, and white on black on the PC-8801mkIISR model.

## Local verification

- `build/linux-debug/sdl2/vaeg --selftest`: exit 0, `TSP SYNC reverse ok`.
  Removing the PC-8801 condition makes it fail with "a PC-8801 applied RV
  with monochrome graphics".
- Linux release build, the demonstration with `--nowait`, screenshots at
  frames 18500/19000/19500: `--model va` and `--model va2` black on white;
  `--model va2 --extra-machine PC-8801mkIISR` (a ROM set under `extra/`)
  white on black.
- `linux-ci-gcc` build and `ctest`: 100% of 110 tests passed (skipped:
  the ROM- or corpus-dependent probes, as on `main`).
- `tools/repo/check_case.py`, `check_encoding.py`, `check_eol.py`,
  `check_md_comments.py`: exit 0. The pinned `clang-format-mp-22` is not
  installed here; `clang-format-18 --dry-run` reports nothing for the
  changed C files, as for their `main` versions. `git diff --check` clean.
