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

# NEON4 PC-9801 to PC-88VA: GDC/EGC-to-SGP audit

This report is the evidence freeze for the second faithful-port attempt. The
untracked `demos/NEON4_1_0/` source and the local `docs/98io/` and
`docs/tekumani/` references are read-only. The previous 320x200 prototype is
not used as an implementation base.

## 1. Source inventory

| Original path | Drawing responsibility | VA disposition |
|---|---|---|
| `NEON4_16.ASM` | Startup, IRQ2 scheduler, DOS input, page exchange | Keep DOS entry/ESC; replace IRQ2 and PC-98 video entry |
| `VIDEO4_LOW.INC` | GRCG setup, EGC probe, page access, clear, line, horizontal span, rectangle fill | Replace every hardware write with SGP command emission; retain operation-level API names only in the port comments |
| `GEOM4_LOW.INC` | 80286-safe triangle scan conversion and Bresenham line generation | Use SGP LINE for edges/spans; use SGP PATBLT for axis-aligned fills |
| `GEOM4_CORE.INC` | Triangle spans, colour selection, raster deformation | Preserve scene geometry and colour decisions; emit SGP descriptors instead of CPU pixel writes |
| `FRAME_RENDER4_LOW.INC` | Select scene, clear page, render, dirty bookkeeping, text update | Build a complete main-RAM SGP list, wait for completion, then flip during VBLANK |
| `SCENE4_256.INC` and `DATA4.INC` | Eight scene order, objects, timing and palette families | Preserve the eight 384-frame chapters and scene-specific carriers/grid |
| `TEXT4_LOW.INC`, sound includes | PC-98 text VRAM and direct OPN probing | Do not use PC-98 text VRAM or guessed sound ports in the graphics gate |

The original 16-colour source is labelled GRCG/EGC rather than a native GDC
command-list renderer. The PC-98 `io_disp.txt`, `io_pmc.txt`, and
`io_agdc.txt` material documents the separate text/GDC, GRCG/EGC, and AGDC
families. Therefore “GDC conversion” in this port means converting the
original display-controller drawing operations to the VA SGP drawing engine;
it does not mean translating PC-98 register numbers to VA ports.

## 2. PC-9801 versus PC-88VA

| Capability | PC-9801 reference | PC-88VA evidence | Port rule |
|---|---|---|---|
| 640x400 16-colour surface | GRCG planar VRAM at PC-98 windows; optional EGC VRAM copy (`VIDEO4_LOW.INC`, `docs/98io/io_disp.txt`, `io_egc.txt`) | Graphics BIOS `INT 8Fh` mode function; 640x400/4bpp is documented in `docs/tekumani/606GRP.TXT` and the VA mode report | Select VA 640x400, 4bpp, single-plane G0; no A000/B000 or GRCG/EGC ports |
| Commanded line | CPU Bresenham loop writes VRAM through `line_set16` | SGP command `0009h` LINE with colour, direction bits, and a destination descriptor (`io/sgp.c`, `docs/modernization/upd92017-sgp.md`) | Emit LINE descriptors in a main-RAM command list |
| Horizontal span | CPU `hline_set16*` writes packed words | SGP LINE with height 1; SGP performs the packed-pixel writes | Convert every span to a horizontal LINE record |
| Solid rectangle | CPU loop over `hline_set16*` | SGP PATBLT `0008h` repeats a 1bpp all-one source and expands SET COLOR; SGP CLS is only a contiguous word fill | Convert `fill_rect` to PATBLT, not a CPU pixel loop |
| Triangle/polygon fill | CPU scan converter emits one horizontal span per row | No verified polygon-fill command in the SGP command set; SCAN is documented but unimplemented in vaeg | Preserve edge and span geometry; use SGP LINE spans, with an explicit limitation for any unfilled face |
| Page storage | PC-98 page selectors and VRAM apertures | 640x400/4bpp is 128000 bytes; two G0 pages fit exactly in the 256KiB GVRAM range; G0 DSA is the VA display-start register | Use G0 page A at SGP `200000h`, page B at `220000h`, pitch 320 bytes, DSA0 flip |
| Video mode | PC-98 GDC/CRTC and IRQ2 timing | `INT 8Fh` function 0; 640x400/4bpp is `BX=A000h` for one displayed single-plane G0, `CL=4`; 640x400 timing is selected by the matching VA TSP profile | Use the BIOS mode call, then set only documented G0 framebuffer fields |
| Composition | PC-98 text/graphics priority and GDC state | VA composition identifiers text=1, sprite=2, G0=3, G1=4; `CX=0003h` selects G0 alone | Compose G0 only; do not add a checkerboard or hidden G1 layer |
| Clear | GRCG black colour plus CPU REP writes | SGP CLS `000Ah` writes a contiguous word range | Clear both G0 pages with SGP CLS before drawing |
| Synchronization | IRQ2/INT 0Ah handler in the original | TSP VBLANK status is exposed by the current VA path; display-start changes must be synchronized | Poll VBLANK status, then write DSA0 as WORD writes |
| Input | DOS `INT 21h/AH=06h/DL=FFh`, original exits on any key | DOS console path is available to a COM | Accept only returned ASCII ESC (`1Bh`) and restore the saved VA mode |

## 3. Verified SGP command conversion

The command list is in main RAM and begins with the mandatory 58-byte
`SET_WORK` record. Descriptors use the SGP packed format:

```text
descriptor word: (x & 3) << 4 | pixel-mode (1 = 4bpp)
width, height:    pixel extents
FBW:              320 bytes for 640x400 4bpp
address low/high: even SGP physical address
```

The port uses these command records:

| Original operation | SGP sequence | Destination |
|---|---|---|
| `clear_graphics_frame16` | SET COLOR 0; CLS; | hidden G0 page, 64000 words |
| `line_set16` / `line_set16_same` | SET COLOR; LINE mode; destination descriptor | hidden G0 page |
| `hline_set16*` | SET COLOR; LINE with `height=1` | hidden G0 page |
| `fill_rect` | SET COLOR; SET SOURCE (1bpp, 1x1 all-one word); SET DEST; PATBLT COPY | hidden G0 page |
| `set_display_page` | WORD writes to FB0 DSA ports after VBLANK | visible G0 page |

SGP LINE direction bits are taken from the LINE-specific VAEG profile:
`LINE_HD=0800h`, `LINE_VD=0400h`. Generic BITBLT names in `sgp.h` are not
substituted. All four endpoint directions are covered by the port's line
emitter tests and asymmetric scene geometry.

## 4. 640x400 memory map

```text
GVRAM SGP address range: 200000h-23FFFFh (256KiB)
G0 page A:               200000h-21FFFFh, 640x400x4bpp, FBW=320
G0 page B:               220000h-23FFFFh, 640x400x4bpp, FBW=320
G0 DSA0 page A:          000000h
G0 DSA0 page B:          020000h
```

This is a capacity calculation supported by the VA GVRAM map and the
documented 640x400/4bpp buffer format. It is not a claim about real-board
throughput. The implementation verifies it against the current VAEG GVRAM
decoder before the human gate.

## 5. Adversarial review

| Proposal | Attack | Resolution |
|---|---|---|
| Reuse the old 320x200 port | It violates the explicit 640x400 requirement and changes authored geometry | Start from `origin/main`; use 640 logical coordinates and 320-byte pitch |
| Translate PC-98 GDC/GRCG ports numerically | Same-looking port numbers do not identify equivalent VA hardware | Remove all PC-98 register writes; use only documented VA BIOS, framebuffer and SGP interfaces |
| Use G1 because it was convenient before | G1 has model-specific restrictions and its 640/4bpp two-screen buffer width is constrained | Use one-screen G0 mode with two 640x400 G0 pages and DSA0, and document the ownership |
| Clear with `REP STOSW` | This is CPU pixel/word rendering and perturbs the measured path | Use SGP CLS for both pages |
| Claim SGP polygon fill from the existence of SCAN | vaeg marks SCAN as unimplemented and its conditions are unresolved | Use PATBLT for rectangles and LINE spans for polygon faces; label the remaining fill difference |
| Use a permanent checkerboard | It was not in the original NEON4 scene background | G0 is black; carrier, raster and grid elements are scene-local |
| Use INT 82h or an IRQ hook for ESC | It is not the original DOS exit path and can hide input failures | Use DOS INT 21h function 06h and test AL=1Bh |
| Call VAEG screenshot a hardware result | Emulator timing and VAEG mode acceptance do not prove board behavior | Report VAEG evidence separately and stop at the human gate |

## 6. Open evidence

- The exact function-0 mode word is derived from the VA manual bit fields:
  single-plane (`bit 15`), one screen (`bit 14=0`), display enabled
  (`bit 13`), G0 width 640 (`bit 3=0`), 400 lines (`bits 1:0=00`), giving
  `A000h`. VAEG acceptance is a required machine test.
- `SCAN_RIGHT/LEFT` are documented but currently TODO in vaeg; they are not
  required for the line/PATBLT port.
- Original direct OPN/OPL paths are outside the graphics correctness gate.
