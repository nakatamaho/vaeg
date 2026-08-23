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

# GLASS ORBIT P4-2 SGP fixed-frame renderer

## Scope

P4-2 adds `GLASSP4S.COM`, the production-candidate fixed-frame renderer for
the retained GLASS ORBIT cube geometry.  It uses the same accepted VA Graphics
BIOS setup as P4-1, then emits one main-RAM SGP command list:

```text
SET_WORK
SET_COLOR + CLS                    full 640 by 200 G0 page
SET_COLOR + CLS spans              visible cube faces
SET_COLOR + LINE                   all twelve cube edges
END
```

The guest never writes the G0 CPU aperture before the list has completed.
CPU work is confined to preserved geometry, face scan conversion, command-list
construction, and the post-completion checksum.  Stars, grid, animation,
double buffering, and OPNA remain later work.

The physical G0 assumptions are the accepted P1/P2 contract:

| Item | P4-2 use | evidence status |
| --- | --- | --- |
| page base | `200000h` | `[SRC:io/sgp.c]` functional VAEG mapping |
| page size | `7D00h` words | `[DERIVED]` 640 × 200 × 4bpp |
| pitch | 320 bytes | `[DERIVED]` packed 4bpp geometry |
| list launch | `0500h/0502h`, control `0504h`, busy `0506h bit 0` | `[SRC:io/sgp.c]`, `[VA-TEKU]` cross-check in `va_video_contract.md` |

This is VAEG functional evidence.  It does not establish PC-88VA SGP timing,
contention, command-list length limits, or hardware conformance.

## Span and line ownership

The logical span and the packed-word transaction are separate.  Each
CPU-converted triangle span is represented as an exact inclusive
`[x_left,x_right]` range using a row sample at `y+1/2`, with `ceil()` for the
left edge and `floor()` for the right edge.  The SGP receives only complete
interior four-pixel words.  The first and last partial words are applied once
with a masked CPU read-modify-write after the SGP list completes; pixels
outside the logical span are preserved.  This is a memory-access detail and
never rounds the polygon geometry.  An empty span (`x_left > x_right`) is
discarded rather than swapped into an out-of-polygon pixel.

The edge-only SGP list is then redrawn as the intended wireframe stage.  There
is no post-outline bridge, patch, erase, or geometry-specific repair stage.
P4-2 does not use `PATBLT` for endpoint or registration cases.

`LINE` descriptors retain logical X, halve logical Y once, use mode `0005h`,
add `HD`/`VD` only for negative direction, and use the 320-byte G0 pitch.  The
descriptor format is independently exercised by the existing SGP wireframe
demo.  The P4 CPU verifier implements the same descriptor traversal as an
independent direct-pixel routine so its exact result is meaningful for this
emulator regression test.

## Builder defect found and corrected

The first P4-2 candidate used `DI` both as the SGP command-list cursor and as
the triangle scan converter's temporary vertex pointer.  On the first face
span, the list writer therefore overwrote temporary geometry, preventing the
builder from reaching its pre-submit checkpoint.  This was a guest test
program ownership bug, not an SGP or framebuffer implementation conclusion.

The cursor now lives in `glass_p4_sgp_list_cursor`, and the word emitter saves
and restores `DI`.  The staged list tests then completed as follows:

| stage | list contents | result |
| --- | --- | --- |
| 1 | work setup, black full-page `CLS`, `END` | reaches `AX=4753h` |
| 2 | stage 1 plus visible-face spans | reaches `AX=4753h`; 3134-byte list |
| 3 | stage 2 plus all cube-edge lines | reaches `AX=4753h`; 3338-byte list; visible closed cube |

`GLASS_P4_SGP_STAGE=1` through `5` selects only a diagnostic build subset.
Stage 3 is the candidate output; stage 4 is outline-only and stage 5 is the
independent packed-word calibration fixture.  The list reservation remains
32 KiB and fails closed before an overrun.

## Local execution and comparison

Build a local bootable validation disk from the maintainer's local template:

```text
demos/va/glass-orbit/build-p4-sgp-bootable-d88.sh SOURCE_BOOTABLE_2HD.d88 OUTPUT.d88
```

The generated D88 contains non-free PC-Engine files and is local-only.  It
must not be committed.  At its prompt, run `GLASSP4S`; the completed frame
waits for ESC through the VA Keyboard BIOS and returns through the local
loader continuation.

For bounded VAEG capture:

```text
demos/va/glass-orbit/run-vaeg-p4-sgp.sh SOURCE_BOOTABLE_2HD.d88 VAEG ROM_DIRECTORY OUTPUT_DIRECTORY
```

The runner defaults to VAEG's explicit `--sgp 16` functional acceleration so
the bounded capture completes.  That flag is not a performance measurement or
a hardware timing setting.  Set `VAEG_P4_SGP_SPEED=model` to exercise the
model-default rate, or set `VAEG_P4_MODEL=va2` for VAEG's VA2/VA3 ROM path.

The debug capture format now accepts `gvram`, which writes a local 256 KiB
VAEG GVRAM snapshot in addition to `registers`, `tvram`, and `screen`.  It is
debugger output only and changes no emulated device state.  Compare one CPU
capture and two fresh SGP captures with:

```text
python3 demos/va/glass-orbit/tools/verify-p4-backends.py CPU_DIR SGP_DIR SGP_REPEAT_DIR
```

The comparator rejects a missing checkpoint, wrong success marker, wrong raw
checksum, missing 256 KiB snapshot, unequal complete GVRAM, unequal composed
screen, or an unequal repeat SGP run.  It has no baseline-update mode.

The accepted fixed-frame values are:

| build | checkpoint | marker | raw checksum |
| --- | --- | --- | --- |
| CPU verifier | `3000:0200` | `4750h` | `7ACEh` |
| SGP candidate | `3000:0280` | `4753h` | `7ACEh` |

The capture comparison validates VAEG internal consistency only.  The same
payload and command sequence still require a future real-PC-88VA observation
before they can be described as hardware-correct.  The registration checker
is separate from face geometry: it compares the face-only capture with an
independent pixel-center oracle, then compares the final capture against the
actual outline-only SGP raster and reports visible gaps, leaks, and vertex
junctions independently.
