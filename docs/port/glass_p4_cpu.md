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

# GLASS ORBIT P4-1 CPU verification renderer

## Scope

P4-1 introduces the verification-only `GLASSP4C.COM` fixed-frame renderer.
It uses the already accepted V3 Graphics BIOS setup from GA-2 through GA-6,
then writes the documented G0 packed-4bpp CPU aperture directly. It does not
start the SGP, call a graphics-BIOS drawing primitive, use a PC-98 I/O path,
or use MS-DOS `INT 21h`.

The source imports no drawing code from GLASS ORBIT. It calls only the P0
geometry/data closure: `glass_compute_cube` produces the original logical
640 by 400 projected vertices and `glass_face_is_visible` retains the
original winding-based back-face decision. The CPU renderer itself owns new
clear, line, solid-triangle scan conversion, clipping, packing, and palette
mapping routines.

This is a test harness, not a release renderer and not a runtime fallback.
P4-2 must implement the corresponding SGP commands and compare them with this
fixed reference. Stars, grid, animation, page exchange, and OPNA are outside
P4-1.

## Fixed state and primitives

The program fixes `render_frame_counter` to zero, clears the 640 by 200 page,
computes the cube once, fills camera-facing quads as two CPU triangles, then
draws all twelve retained cube edges. Each primitive accepts logical 640 by
400 Y values and halves Y exactly once at its own drawing boundary.

| Primitive | P4-1 implementation |
| --- | --- |
| clear | `REP STOSW` across exactly `7D00h` G0 packed words |
| pixel | direct high-nibble-first 4bpp read-modify-write |
| line | independently written integer Bresenham rasterizer |
| triangle | independently written scanline interpolation and clipped CPU spans |

The face-color map is the approved P2 mapping: source values `1,2,4,5,3,6`
become VA palette entries `8,9,10,11,12,13`. P4-1 uses a temporary dark colour
profile for those entries. The profile is an emulator-visible test palette,
not a real-hardware palette assertion or final artistic tuning.

Evidence: `[VA-TEKU:606GRP.TXT sections 6.6.2-6.6.3, functions 0, 8, 9, 11]`,
the tracked [VA video contract](va_video_contract.md), and the accepted
GA-2/GA-3/GA-6 source boundaries. Local documentation is cited in this report,
never from source comments.

## Raw stability witness and visible diagnostic

After drawing, the guest calculates a rolling checksum over the raw 64,000
byte packed G0 page and places it in `BX` at the fixed checkpoint
`2000:0200`. The initial two VAEG runs produced `BX=416Eh`. This is a raw-page
stability witness, **not a replacement for a full raw framebuffer capture**.
P4-2 must add the CPU-versus-SGP raw-pixel comparison; neither P4-1 result
proves PC-88VA hardware conformance.

The host checker also requires byte-identical composed 640 by 422 BMPs from
two fresh runs and verifies a bounded, multicolour cube-shaped foreground
envelope. The BMP makes human inspection possible but is not the pixel oracle.
The black intervening output rows are VAEG's current 200-line composition
representation, not a PC-88VA timing claim.

## Local execution

`build-p4-cpu-bootable-d88.sh` creates a local bootable disk from a
maintainer-local PC-Engine template. The resulting D88 is never committed.

```text
demos/va/glass-orbit/run-vaeg-p4-cpu.sh \
  SOURCE_BOOTABLE_2HD.d88 VAEG ROM_DIRECTORY OUTPUT_DIRECTORY
```

The runner captures `glass-p4-cpu.registers.tsv` and
`glass-p4-cpu.screen.bmp`. Validate two independent capture directories with:

```text
python3 demos/va/glass-orbit/tools/verify-p4-cpu-capture.py FIRST SECOND
```

The checker has no baseline-update capability. Its accepted checksum and
visible envelope are source-controlled; changing either requires review.

`tools/repo/find_unreferenced.py --report` reports the P4-1 NASM sources as
outside the production CMake graph. `build-p4-cpu.sh` and the local loader are
their explicit build ownership; the report is a candidate list, not a deletion
verdict.

## Boundary

P4-1 establishes only that the new CPU verification implementation is stable
under VAEG and visibly draws the retained fixed cube geometry. It does not
make an SGP claim, a performance claim, an input/exit claim, or a real-PC-88VA
compatibility claim. Those remain later staged work.
