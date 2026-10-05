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
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->
# V480PAT: PC-88VA display-timing test pattern

`V480PAT.COM` is a PC-Engine (V3 mode) program that draws a 16-colour test
pattern in graphics screen 0 and can switch the display to non-standard line
counts by sending its own TSP `SYNC` vector. It was written for vaeg M104 to
measure, on real hardware, which display timings the PC-88VA and a monitor
accept, and how the graphics fetch behaves outside the documented 200/204/
400/408-line modes. The source, [`v480pat.asm`](v480pat.asm), is distributed
under the repository's two-clause BSD terms.

It is an independent program. It complements, but does not contain,
[VIEW480](http://www.pc88.gr.jp/softlib/index.php?action=list_file&anum=2&gnum=404)
(mami, 1992), which shows a 640x480 picture by reprogramming the TSP for 480
lines; VIEW480 is not redistributed here. The development-disk builder
builds `VIEW480.COM` from its public source archive (see
[`pc88va-utility-media.md`](../../../docs/modernization/pc88va-utility-media.md)).

> [!WARNING]
> Non-standard `SYNC` timings can be out of range for a monitor. Fixed-
> frequency CRTs in particular should not be left driven by an out-of-range
> signal. Every switching option restores the previous timing on any key;
> if the screen is lost, press a key or switch the machine off.

## Building

The CMake build assembles it with NASM to `build/<preset>/guest/v480pat.com`;
the source is restricted to 8086/80186 instructions (`cpu 186`), so an
assembler choice that would emit 80386 forms fails the build. A PC-Engine 1.1
test disk is made from the user's own system disk:

```sh
tools/pc88va/build-vtiming-disk.sh \
    --source /path/to/pc-engine-1.1-system.d88 \
    --output /path/to/vtiming.d88 \
    [--view480 /path/to/VIEW480.COM]
```

The system disk is private media; keep both images outside Git.

## Usage

```text
V480PAT [lines] [S|T|U|D] [W] [N]
```

| Argument | Effect |
|---|---|
| `lines` | 1-480 (default 480). Graphics screen 0 is set up through the graphics BIOS (`INT 8Fh`) as single-plane 4 bits/pixel, 640 dots, with a frame buffer `lines` high, and the pattern is drawn directly into GVRAM. |
| none of S/T/U/D | The pattern is left on the normal screen (400 lines visible), for VIEW480 or a capture. |
| `S` | 24.8 kHz: VIEW480's sequence with `VAD = lines`; above 400 lines the bottom blanking and sync become 1 and 1 (VIEW480's values), below that the ROM's 7 and 8 stay. The frame grows or shrinks with `lines`. |
| `T` | 15.98 kHz, monitor switch at 15 kHz. Graphics in 200-line mode (one raster per line). `lines` 4-256, rounded down to even. Top blanking 32, sync 4, bottom blanking at least 2 and grown to keep a frame of at least 262 lines: the uPD72022 data-book minimums with sprites. |
| `U` | As `T` with top blanking 16 (the data-book minimum without sprites). |
| `D` | 24.8 kHz, 320 x `lines` (1-240) line-doubled: graphics in 200-line mode with port `0100h` RSM = 01 (non-interlaced mode 1), the TSP frame at 2 x `lines` rasters; above 400 rasters bottom blanking 2 and sync 4. |
| `W` | Graphics screen 0 at 320 dots (`0102h` bit 4) with `S`, `T`, `U` or none; `D` is always 320. |
| `N` | With `S`, `T`, `U` or `D`: TSP horizontal active `HAD` 159 -> 127 (128 TCK: 256 dots at 320, 512 at 640) and 16 TCK added to each of `LBR` and `RBR`, keeping the line length. Two white two-dot marks show the edges of that window. |

Any key restores the timing (and RSM for `D`). The graphics mode chosen for
the pattern is left in place, so after `D` or `T` the pattern stays in
200-line mode.

## Timings

Frame totals follow `TBL + TBR + VAD + BBR + BBL + VS`; rates use vaeg's
dot clocks (24.83 kHz and 15.98 kHz lines) and are derived values, not
measurements.

| Command | Lines (top / active / bottom / sync) | Frame | Vertical rate |
|---|---|---:|---:|
| normal 400-line screen | 25 / 400 / 7 / 8 | 440 | 56.4 Hz |
| `VIEW480` | 25 / 480 / 1 / 1 | 507 | 49.0 Hz |
| `V480PAT 240 S` | 25 / 240 / 7 / 8 | 280 | 88.7 Hz |
| `V480PAT 232 D` | 25 / 464 rasters / 2 / 4 | 495 | 50.2 Hz |
| `V480PAT 240 D` | 25 / 480 rasters / 2 / 4 | 511 | 48.6 Hz |
| normal 15 kHz 200-line screen | 37 / 200 / 15 / 8 | 260 | 61.5 Hz |
| `V480PAT 240 T W` | 32 / 240 / 2 / 4 | 278 | 57.5 Hz |
| `V480PAT 240 U W` | 16 / 240 / 2 / 4 | 262 | 61.0 Hz |
| `V480PAT 224 U W` | 16 / 224 / 18 / 4 | 262 | 61.0 Hz |

## Reading the pattern

- Left half: 40-line colour bands (colours 1-14 repeating). Right half:
  sixteen vertical bars, colours 0-15.
- White full-width lines every 100 lines and on the last line; red lines at
  lines 399/400, or 199/200 at 320 dots.
- Ruler: from line 192 down to the larger of `lines` and 264, a bar at the
  left edge. GVRAM is cleared that far first, so the ruler below the last
  line shows how far the display reads graphics past it. Its length is
  `(y mod 8) + 1` steps (8 dots at 640, 4 at 320); its colour gives
  `y div 8`:

| Colour | Lines | | Colour | Lines |
|---|---|---|---|---|
| white | 192-199, 256-263 | | cyan | 224-231 |
| red | 200-207, 264-271 | | blue | 232-239 |
| yellow | 208-215 | | magenta | 240-247 |
| green | 216-223 | | light grey | 248-255 |

  The pattern repeats every 64 lines (480-line runs continue it). The last
  visible line is the last bar's colour and length; for example a green bar
  four steps long is line 219.

## Results

See [M104](../../../docs/agents/tasks/M104_v3_display_timing.md) and
[`pc88va-video-modes.md`](../../../docs/modernization/pc88va-video-modes.md#12-measured-non-native-timings)
for the measured results.
