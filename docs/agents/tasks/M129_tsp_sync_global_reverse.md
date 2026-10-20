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

## Media build regression

Clean clone of `43e63ee82ff7d590e0bd31fac396a9192db6f14a`, shared caches,
private system disks copied to task scratch (paths omitted):

| Command | Exit | Output SHA-256 |
|---|---:|---|
| `build-utility-disk.sh` (PC-Engine 1.1) | 0 | FDD `81f85d4231395d8ea2f6e995740823c0fa5b101dd9fe0c1151639d7838dded52` (4096 bytes free) |
| `build-sasi-utility-disks.sh` (1.05 VA, 1.1 VA2) | 0 | VA `6fc3052870b36ab5206fc536ac1a943b5acf8557860570c6b16459692fa0e934`, VA2 `d72e9e5e4c1c89f5ae909dc218b49c0a3a557daeba94be82311cec6ea2ad8e96` |
| `glass-orbit/build-d88.sh` | 0 | `2477a0bfe2b623fa9fd0c7c4f07bd8429d755aa613c91f49d636085b8bd967b7` |
| `neon3/build-d88.sh` | 0 | `1794434fa19732646a244cd6f53d6b91fe141de5ae606ad142e996a8f6610add` |
| `neon4/build-d88.sh` | 0 | `95d2e36ca185e9c5b74b662b84cf9cbbe244f6a811f80961a4abe5189daa3f2e` |
| `sgp-pseudo-sprite/build-d88.sh` | 0 | `8c3b0264f0070ca054ea7dd1326f3827b3efdb641535d2fbf00ebc10d4379a9b` |
| `sgp-wireframe/build-d88.sh` | 0 | `eaac64d8a1ca4ce2237e3f2a52608424b9500f775a963862c0cc571b9906d63b` |
| `zundamon-orbit/build-from-psd.sh` | 0 | `5b4380b108f7642b72112575e6fcd5697559fbc09dbbf8eba128d832742005a0` |
| `build-all-demos-distribution-disk.py` | 0 | `c15a0fea96cb63b3cac6c554c6258e3d30e5e2e6b169fdadbb26da915cc4ec69` |
| `build-all-demos-msdos4-disk.py` (`msdos4-va.2`) | 0 | `e88c0e0332094048bcf55b334cd0430fc1c77e158344b00e57946ad9bbc84a78` |
| `sgp-boot/build-boot-d88.py` | 0 | `67ead94dba0cee7aa9d9a0268cf1987bf3a2764fe1275e092b2e5ece06337154` |

Every file of every regenerated FAT image is byte-identical to the committed
`.d88.xz` (the remaining differences are directory dates); the
all-demos-msdos4 and SGP boot `.xz` are byte-identical. In vaeg (M129
release build) the utility FDD boots PC-Engine 1.1 on the VA2, and each SASI
image alone boots PC-Engine on its model (1.05 VA, 1.1 VA2) once a backup
memory record exists (`--no-bkupmem` leaves no record for the fixed-disk boot
bit, M106). M129 changes no demo or media source.

## Clean-checkout build

`tools/release/build-mingw-static-container.sh --commit
43e63ee82ff7d590e0bd31fac396a9192db6f14a` completed: commit embedded,
imports Windows system DLLs only. `vaeg.exe` SHA-256
`b4e39261d27d9f3454f72264204269427597ad263f475d091d5cb293312f7fd4`.
Later commits on this branch are documentation only.
