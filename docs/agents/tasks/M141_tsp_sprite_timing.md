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


# M141 — TSP sprites and TVRAM as on the machines

Status: **G141 human gate passed on 2026-10-29** (the maintainer's
"gate passed", with TSPSPR screen 5 on the VA and VA2/3 models from the
9b60f9c1 static build, every row as in the table below).

Branch `milestone/m141-tsp-sprite-timing`; commit prefix `M141:`.

## Request

Issue #67: `demos/tsp-sprites` (TSPSPR, PR #68) was photographed on the
PC-88VA and PC-88VA2 (2026-10-27, maintainer). The maintainer asked for
vaeg to follow what they show, after a port of an arcade game started
using the hardware sprites.

## Measured (TSPSPR, both machines)

- No sprite after SYNC and SPRON without DSPON (vaeg showed them).
- A raster line takes about 300 bytes of image counting 4 a sprite: about
  24 sprites of 8 bytes a line, 15 of 16, 8 of 32, all 31 of 4; the same
  bytes give the same count in monochrome and 16 colours. Later sprites of
  the line are not drawn. SPROV: 20h with overlapping sprites.
- TVRAM: a CPU word access about 13 (VA) / 12 (VA2) clocks more than main
  RAM, byte loops not slower; no dependence on sprites, side or SYNC.
- The status's VB: 8.6 % of a frame on the VA whatever the SYNC, 2.0 % on
  the VA2 (its vertical sync).
- A SPRON about 3.4k (VA) and 3.7k (VA2) clocks.

## Changes

- [442364c1](https://github.com/nakatamaho/vaeg/commit/442364c162ae1a6f520c51d9896c78a18eb76650):
  - `vram/makesprva.c`: sprites only while `dspon`; a line's sprites
    drawn from sprite 0 while they fit `SPRVA_LINE_BYTES` (300), each
    `SPRVA_LINE_COST` (4) plus its image bytes; collisions (by group with
    SPRON.GR) and sprite-over (more than HSPN + 1) set SC and SPROV's byte
    at the next frame.
  - `io/tsp.c`: SPROV (81h) returns the byte through 146h with OBF and
    clears SC; IBF held 814 (VA) / 884 (VA2) clocks after each byte; on the
    VA the status's VB lasts 23 rasters from the sync's start.
  - `memoryva/memoryva.c`: TVRAM word accesses 13 (VA) / 12 (VA2) clocks.
  - `io/tsp.h`: `sprov` and `result` in the reserved bytes (state size
    unchanged).
  - selftest `VA sprite line`; `idp_m69_status_composition` reads the
    status after the IBF hold.
- Merge of `demos/tsp-sprites` (PR #68).
- [c3c238de](https://github.com/nakatamaho/vaeg/commit/c3c238de): state
  thumbnails (quick saves) turned as the screen is shown; save times shown
  to the second (maintainer's request during G141). Selftest: the
  screenshot rotation case also checks a turned thumbnail.
- [f8a74126](https://github.com/nakatamaho/vaeg/commit/f8a74126abe8dfbb0abd7613cc60f2e4e30f20b2):
  debug script `load-state RESOURCE`.
- [14864dc9](https://github.com/nakatamaho/vaeg/commit/14864dc9): the
  debug harness selftest declares the `load-state` line's resource and
  counts the action (f8a74126 left `--selftest` failing).
- [9b60f9c1](https://github.com/nakatamaho/vaeg/commit/9b60f9c1): save
  times no longer flicker between two seconds (the file clock's offset
  from the system clock taken once, not each frame; found during G141).
- `upd72022-tsp.md` section 13.5, the ledger.

## vaeg divided by the photographs (TSPSPR screen 5, means)

| Rows | VA | VA2 |
|---|---|---|
| TVRAM words (I-P, U) | 0.97-1.01 | 1.01-1.03 |
| TVRAM bytes (E-H, V) | 0.92-1.02 | 0.98-1.09 |
| VB (Q, R, S, W) | 0.99-1.06 | Q 1.45; R, S, W 1.00-1.05 |
| SPRON (T) | 1.00 | 1.00 |

Byte rows were the same before (the loop itself). Open: VA2 row Q
(128-byte chunks written to TVRAM in VB).

Sprites a line (screen 3, N = 31): C16 24, C32 14, M32 31, C64 8, M64 24,
M128 14 in vaeg; the photographs about 24-25, 15-16, 31, 7-8, 23-24, 15-16.

## Gate G141 (human)

Standard V3 gate, plus: with the screen turned 90 degrees a quick save's
thumbnail is upright, and the state list shows seconds; plus TSPSPR from `demos/tsp-sprites` on the VA and VA2
models: screen 1 boxes aligned, screen 3 sprites missing past the line's
limit as above with OV 20, screen 5 as in the table.

## Local verification

- `--selftest` (SDL dummy video): exit 0, `VA sprite line ok`.
- `linux` ctest with `VAEG_ENABLE_TESTS`: 120 of 120.
- A port using the hardware sprites (private build): 59.8-60.0 renders a
  second on stage 1 and a late stage on both models, as before M141.
- Media build regression reused: the media builders and committed demo
  disks are unchanged since
  [bd781886](https://github.com/nakatamaho/vaeg/commit/bd78188638dfcd3bed41c696434fc245277adedd)
  (recorded in M134); since then `demos/z80-timing` (M135) and
  `demos/tsp-sprites` (here) were added, both on no committed disk;
  `demos/tsp-sprites/build.sh` builds (exit 0).

## Clean-checkout build

`tools/release/build-mingw-static-container.sh --commit
9b60f9c1`: exit 0; `vaeg.exe` SHA-256
`9004e88ed6371734aaeb020856065eafb0f10e6a5d6eb1773d87d4b3d67905e7`
(earlier: f8a74126's `494dd379...`, a750ed28's `e3e16b38...`).

## Open (found during G141)

- The VRTC interrupt and the TSP status's VB are out of phase (issue #74).
- A state saved by the Windows build does not load in the Linux build: the
  DISK section's paths are `MAX_PATH` bytes (260 on Windows, 4096 on Linux),
  so the section sizes differ and the check fails. Not fixed here.
