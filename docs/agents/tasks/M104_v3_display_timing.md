<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# M104 - V3 display timing: non-native line counts and line doubling

Status: **in progress**

Series: V3 series, first milestone (maintainer decision 2026-10-06: the
display-timing work takes M104; SGP moves to a later milestone). Branch
`topic/m104-undocumented-tsp-display-semigraphics` (renamed from
`topic/m104-v3-display-timing` on 2026-10-06) off `main` at
`9ff34397f24af238f239540d96c526b28871debb`. Commit prefix `M104:`.

## Goal

Measure on real hardware which TSP `SYNC` timings beyond the documented
200/204/400/408-line profiles a PC-88VA displays, record the results with
`[MEAS]`, and make vaeg show them: 480-line frames (VIEW480), 15 kHz frames
of up to 256 lines (320x240), and 24.8 kHz line doubling (port `0100h`
RSM = 01) beyond 200 lines.

## Background

- `[VA-TM]` 4.9.4: 15 kHz cannot show 400/408 lines; at 24.8 kHz a 200/204-
  line graphics screen has blank odd rasters (RSM = 00) or each line on an
  even/odd raster pair (RSM = 01). Graphics resolutions are 640/320 dots and
  200/204/400/408 lines.
- uPD72022 data book (via [`upd72022-tsp.md`](../../modernization/upd72022-tsp.md)
  10.1): at most 512 active lines; `VS >= 4`, `BBR + BBL >= 2`, top
  blanking plus border at least 10h without sprites and 20h with sprites.
- VIEW480 (mami, 1992; Softlib 2-404) switches a 640 x 401-480 picture to a
  480-line 24.8 kHz frame with bottom blanking and sync of 1 line each, below
  those minimums.
- vaeg (main at the branch point): headless screenshots of these timings
  stay 640 x 422 (400 display lines); a 480-line frame and a 15 kHz
  240-line frame are not shown in full. Where the limit lies is part of
  scope 3.

## Scope

1. **Test program** (done in this milestone): `tools/pc88va/vtiming/
   v480pat.asm` (2-clause BSD, independent of VIEW480) and a disk builder.
2. **Measurements**: results in this file and in
   [`pc88va-video-modes.md`](../../modernization/pc88va-video-modes.md),
   tagged `[MEAS]`.
3. **vaeg**: output height from the TSP frame (`VAD`, RM, monitor switch);
   15 kHz frames up to 256 lines; 24.8 kHz frames up to 480 rasters;
   line doubling with RSM = 01 beyond 200 lines; graphics fetch extent below
   `VAD` as measured; `HAD` narrowing only if the measurement shows it.
4. **Tests**: romless tests feeding the measured `SYNC`/`GRMODE` settings.

## Gate G104 (human)

Standard V3 gate unchanged, plus: in vaeg, `VIEW480`, `V480PAT 240 T W`,
`V480PAT 240 U W` and `V480PAT 232 D` show what the real machine showed.

## Implementation progress

- Test program and builder committed:
  [`tools/pc88va/vtiming/README.md`](../../../tools/pc88va/vtiming/README.md).
- `[MEAS]` First hardware round (2026-10-05/06): recorded in
  [`pc88va-video-modes.md` section 12](../../modernization/pc88va-video-modes.md#12-measured-non-native-timings).
  Machine: PC-88VA2. Open: where the 240-line pictures lose their last
  line; the horizontal result of `N`.
- Line ruler added to V480PAT for the second round.
- `[MEAS]` Second round (2026-10-06):
  [`pc88va-video-modes.md` 12.1](../../modernization/pc88va-video-modes.md#121-second-round-line-ruler),
  with photographs. At 15.98 kHz graphics show `TBL + VAD - 37` lines; at
  24.8 kHz with RSM = 01 graphics stop advancing after line 200; graphics
  are not clipped by the TSP `HAD`; `240 T` (640 dots) follows the same
  rule. Open: whether 15 kHz graphics are shifted by `37 - TBL` against
  text.
