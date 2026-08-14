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
# PC-88VA SGP Pseudo-Sprite Demo

## Current milestone

This directory currently implements M5: multiple animated pseudo-sprites
rendered into a hidden Graphic 1 page, followed by a VBLANK-synchronized
DSA1 page exchange. The M3 transparent-BITBLT and M4 animation gates
passed before this double-buffer code was added.
G5 has passed. On the evaluated VAEG setup, the 24x24 scene measured about
57 FPS with 26 active spheres and about 28 FPS with 27; that step is the
current M5 workload limit and is not presented as a hardware maximum.
The 16-bit real-mode DOS `.COM` program keeps the verified 320x200, 16-color,
4-bpp, single-plane, two-screen configuration. Its M5 scene contains:

- a static CPU word-filled checkerboard in Graphic 0;
- two 32,000-byte Graphic 1 pages: one displayed and one rendered off-screen;
- 1-1024 independently moving 24x24 pre-rendered shaded balls, initially 16,
  transferred from main RAM by SGP BITBLT commands; and
- an eleven-glyph `FPSnnn Cnnnn` counter at the upper right, also transferred
  from main RAM by SGP BITBLT commands.

The 16 pre-rendered bitmap variants use 16 different HSV color phases; all
sprite records reuse those variants in painter order. A shared 4-bpp palette
cannot contain transparent index 0, two
checkerboard shades, dedicated shadow and highlight colors, and 16 additional
opaque hues at once. The demo therefore stores 12 saturated HSV wheel anchors
at 30-degree intervals and constructs four intermediate phases by dithering
adjacent anchors inside the RAM bitmaps. Palette indices 13-15 provide the
neutral shadow, light checkerboard, and white highlight. This gives 16
visually distinct, colorful ball bitmaps while preserving the single shared
16-color palette and color-0 transparency.

Every ball uses a pre-rendered 24x24 raster sphere with a white upper-left
highlight, a smooth body gradient, a dark lower-right edge, and a transparent
silhouette. Color index 0 surrounds the sphere. BITBLT mode `0105h` skips
those source-zero pixels, and G1 composition also treats color 0 as transparent,
so the Graphic 0 checkerboard remains visible through and around every ball.

Each 18-byte sprite record stores `x`, `y`, `vx`, `vy`, `width`,
`height`, source pitch, bitmap offset, and painter priority. The CPU updates
coordinates, bounces sprites at the display edges, and emits descriptors into
a RAM command buffer. It never clears G1 or copies, masks, or shades sprite
pixels. The record array is already in priority order: later records are drawn
later and their nonzero pixels appear above earlier records. The first two
balls move toward each other on the same path to provide a repeatable overlap
test. PC-88VA cursor Up or `+` increases the active prefix of the array up to
1024 records; cursor Down or `-` decreases it to one. Key sensing and retrieval
use the documented keyboard BIOS primitives, so no PC-compatible extended-key
sequence is assumed.

The M5 program uses these hardware interfaces, all traced in the
[M1 investigation](../../../docs/modernization/sgp-pseudo-sprite-investigation.md):

| Purpose | Interface used by M5 |
|---|---|
| Mode | Graphics BIOS `INT 8fh`, function 0, `BX=e00eh`, `CX=0404h` |
| Buffer geometry check | Graphics BIOS function 7 for screens 0 and 1 |
| Layer order | Graphics BIOS function 3, `CX=0034h` (G1 above G0) |
| CPU G0 aperture | `a000:0000`; the CPU does not write G1 after mode setup |
| GVRAM mapping | Port `0153h`: single-plane mode and system-memory bank 4 |
| CPU write mode | Port `0580h`: CPU data write for the static G0 background |
| Composition transparency | Ports `0124h`/`0126h`: G0 opaque, G1 color 0 transparent |
| SGP command address | Ports `0500h-0503h` |
| SGP control/status | Ports `0504h` and `0506h` |
| Frame pacing and flip | TSP status port `0142h`, bit 6, then FB1 DSA ports `022eh-0230h` |
| SGP destination | draw page A `220000h` or page B `227d00h`, pitch 160 bytes |
| Ball-count input | Keyboard BIOS `INT 82h`, functions `0ah` and `09h`; Up scan `3ah`, Down scan `3dh` |
| FPS time base | Calendar BIOS `INT 8ch`, function `02h`; display pixels are still rendered by SGP |

`0034h` uses graphics BIOS layer identifiers, not the raw
`COLCOMP=00abh` register value described in the M1 report. The BIOS owns the
complete mode and composition transition.

## M5 SGP command list

The 33,142-byte maximum command buffer, 58-byte work area, 1,024 sprite
records, 16 pre-rendered 24x24 HSV bitmaps, and 11 FPS/count glyph bitmaps
reside inside the DOS-loaded COM image in main RAM. DOS chooses the load
segment, so every frame the program converts relevant `DS:offset` values to
physical addresses.

~~~text
SET_WORK
    writable 58-byte work-area physical address
SET_COLOR
    0000h
CLS
    hidden G1 draw page at 220000h or 227d00h, 00003e80h words
repeat for the active 1-1024 sprite records in painter order:
    SET_SOURCE
        4 bpp, start dot 0, 24x24, pitch 12
        selected HSV shaded-ball main-RAM physical address
    SET_DESTINATION
        4 bpp, start dot (x AND 3), 24x24, pitch 160
        hidden-page base + y * 160 + ((x AND fffch) / 2)
    BITBLT
        mode 0105h: forward source copy, source color 0 skipped
repeat for the eleven FPS/count glyphs:
    SET_SOURCE
        4 bpp, start dot 0, 4x7, pitch 2
        glyph main-RAM physical address
    SET_DESTINATION
        G1 upper-right glyph position, pitch 160
    BITBLT
        mode 0105h: forward source copy, source color 0 skipped
END
~~~

At the initial 16-ball setting this is 443 command words and 85 SGP opcodes,
including 27 BITBLTs. At 1,024 balls it is 16,571 command words and 3,109
opcodes, including 1,035 BITBLTs. The CPU starts the list and polls busy
bit 0 until END
clears it. The complete list is rendered into the page opposite the current
display page. Only after SGP completion does `flip_draw_page` wait for the
non-VBLANK-to-VBLANK transition and write the three DSA1 bytes. It then
toggles the page variables, so the next CLS and all BITBLTs target the new
hidden page. Startup renders both pages before enabling display.

## Milestone source ladder

Educational NASM excerpts for M2 through M5 are preserved in
[`milestones/`](milestones/). They are deliberately short, annotated
excerpts rather than untested standalone binaries; the buildable source is
[`sgp_sprite_demo.asm`](sgp_sprite_demo.asm).

## Build

Configure a normal VAEG preset, then build the dedicated guest target. For
example on a MacPorts development machine:

~~~sh
cmake --preset macos-macports
cmake --build --preset macos-macports --target sgpsprite_com
~~~

The output is:

~~~text
build/macos-macports/guest/sgpdemo.com
~~~

The source can also be assembled independently:

~~~sh
nasm -f bin tools/pc88va/sgp-pseudo-sprite/sgp_sprite_demo.asm \
  -o /tmp/sgpdemo.com
~~~

No generated `.COM` file belongs in the source tree.

## Disposable disk installation

Always operate on a copy of a user-supplied PC-Engine D88 image:

~~~sh
python3 tools/pc88va/pcengine_disk.py vanilla \
  --source /path/to/pcengine-system.d88 \
  --output /tmp/sgpdemo-test.d88
mkdir -p /tmp/sgpdemo-payload/root
cp build/macos-macports/guest/sgpdemo.com \
  /tmp/sgpdemo-payload/root/SGPDEMO.COM
python3 tools/pc88va/pcengine_disk.py install \
  --image /tmp/sgpdemo-test.d88 \
  --payload /tmp/sgpdemo-payload
~~~

Launch the configured VAEG executable with the disposable image, for example:

~~~sh
build/macos-macports/sdl2/vaeg --fdd1 /tmp/sgpdemo-test.d88 --fdd2 none
~~~

At the DOS prompt, run:

~~~text
SGPDEMO
~~~

## Expected M5 result

The screen must show the Graphic 0 checkerboard with at least 16 pre-rendered
shaded spheres spanning the HSV wheel and moving independently above it.
`FPSnnn Cnnnn` must be visible at the upper right after the first complete
measurement interval. Every sphere must retain the bright upper-left
highlight, dark lower-right body, and cast shadow. The checkerboard must
remain visible through each source-zero corner and everywhere Graphic 1 was
cleared to color 0.

The two balls on the central horizontal path must periodically overlap. During
an overlap, nonzero pixels from the later record must appear above those from
the earlier record while transparent pixels still reveal the lower ball or
checkerboard. Every ball must bounce at the screen edges.

Cursor Up or `+` must add balls one at a time to a maximum of 1024. Cursor Down
or `-` must remove balls one at a time to a minimum of one. Newly enabled balls
continue from their stored initial or last-active state.

The M4 single-buffer baseline intentionally rendered into the displayed
page and could show transient clearing or tearing. M5 must not show that
visible clear: the hidden page is fully rendered before its VBLANK DSA1
exchange. Pressing ESC must return to the DOS prompt without a hang.
The program restores the saved BIOS mode and resets the standard palette;
arbitrary application palette contents cannot be preserved because VAEG does
not expose palette readback.
