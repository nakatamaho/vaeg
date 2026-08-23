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

# GLASS ORBIT P2: PC-88VA implementation design

Status: design for review. No PC-88VA implementation is authorized by this
document. P3 and later remain blocked until the maintainer approves P2.

This design consumes the hardware contract in
[`va_video_contract.md`](va_video_contract.md). Its evidence tags have the
same meanings. In particular, VAEG execution is functional evidence only; it
does not establish real-PC-88VA timing or contention behavior.

## 1. Design goals and non-goals

The production renderer targets V3, G0, 640x200, single-plane packed 4bpp.
It renders the GLASS geometry and composition, not the PC-98 GRCG algorithm or
its exact pixels.

| Decision | P2 design consequence |
|---|---|
| Production drawing is SGP-only. | Clear, grid, cube edges, and face spans are emitted as SGP commands. |
| Stars remain CPU pixel writes. | The same star routine is used by both backends after SGP is idle. |
| The scene stays logical 640x400. | Every graphics primitive converts logical Y to physical Y once with an arithmetic right shift. No geometry routine is changed. |
| Faces use palette colors, not the original 75% dither. | One solid face color is selected per visible face; no original GRCG pattern path is ported. |
| OPNA is a later independent milestone. | P3--P5 have no sound dependency. |

The verification-only CPU backend exists solely to compare the pixels that
would be made by SGP clear/line/span operations. It is neither a release
renderer nor a fallback selected at runtime.

## 2. Deliverable layout and build boundary

P3 must add only new GLASS-specific files beneath `demos/va/glass-orbit/`.
The P0 geometry/data files remain the sole imported original-code closure.

| Planned file | Responsibility |
|---|---|
| `src/glass_orbit_va.asm` | VA payload entry, frame loop, static memory reservations, and compile-time backend selection. |
| `src/va_gfx.inc` | VA mode/framebuffer lifecycle and the four scene-facing graphics primitives. |
| `src/va_sgp.inc` | SGP command-list builder, submission, and idle poll. |
| `src/va_cpu_verify.inc` | Verification-only CPU implementation of clear, line, and span primitives. |
| `src/glass_geometry.inc`, `src/glass_data.inc` | Existing P0 geometry/data; no algorithmic rewrite. |
| `build.sh` | NASM builds selected with `VA_DRAW_BACKEND=sgp` or `VA_DRAW_BACKEND=cpu`. |
| `run-vaeg-*.sh` and QA checker | Local VAEG capture and deterministic backend comparison only after the payload ABI exists. |

The build selector is compile-time. It may not add a guest-visible command-line
option or runtime fallback that could silently avoid SGP.

## 3. Video and page model

### 3.1 Proposed G0 source layout

The framebuffer model is one 640x400 packed-4bpp G0 source with a 640x200
display window. The two 64,000-byte halves are logical display pages:

| Field | Page A | Page B |
|---|---:|---:|
| G0 source offset | `00000h` | `0FA00h` |
| SGP address | `200000h` | `20FA00h` |
| display height | 200 lines | 200 lines |
| source pitch | 320 bytes | 320 bytes |

The framebuffer descriptor must set a 320-byte, 400-line FB0 source and move
the coupled `OFY`/`DSA` values together to select its visible 200-line window.
Changing raw DSA alone is forbidden by this design. `[VA-TEKU:4.TXT §4.4.5]`

P3 GA-6 must prove this model with two deliberately different page colors and
a captured frame from each page. Until then, it is `[DERIVED]`, not a
real-hardware page-flip claim.

### 3.2 Mode lifecycle

The P3 implementation must use the dependency order fixed in P1:

```text
documented TSP SYNC profile
  -> display/drawing single-plane controls
  -> G0 640-dot, 200-line, 4bpp interpretation
  -> FB0 source/window descriptor
  -> palette and composition
  -> SGP work area
  -> render hidden page
  -> wait SGP idle
  -> CPU stars
  -> wait TSP VB
  -> coupled FB0 source-window exchange
```

The payload may not rely on a DOS prompt, PC-98 `INT 18h`, PC-98 GDC/GRCG
ports, or inherited graphics state.

**Approved P2 scope:** a PC-88VA Graphics BIOS call establishes the V3
640x200, single-plane, 4bpp display state before the payload configures its
framebuffer descriptors and submits SGP work. The same VA BIOS ownership is
used to leave graphics mode. This is an explicit dependency on the VA BIOS,
not an accidental dependency on DOS or a PC-98 BIOS. Once mode entry is
complete, all GLASS drawing remains direct SGP work plus the specified CPU
star writes; the payload must not call a graphics BIOS drawing primitive.

P3 must identify the exact VA Graphics BIOS entry, input/output register
contract, and error result from the local VA references before emitting the
call. If that contract cannot be established, P3 stops rather than substituting
raw TSP writes or a PC-98 video call. P1-U1 remains hardware-pending for the
underlying TSP profile, but it is outside the P3 mode-entry implementation
while this BIOS-owned boundary is in force.

## 4. Scene-facing graphics API

The following interface preserves the original scene's coordinate and color
ownership. It is a design interface, not code.

```asm
; Input coordinates are logical 640x400 unless explicitly noted.
va_gfx_init
va_gfx_leave
va_gfx_wait_vblank
va_gfx_flip

va_gfx_list_begin                 ; choose hidden G0 page and reset the list
va_gfx_clear_page                 ; SGP CLS in production
va_gfx_pixel_set                  ; CPU-only star pixel, even in SGP build
va_gfx_line_set                   ; SGP LINE in production
va_gfx_fill_triangle              ; SGP CLS spans in production
va_gfx_submit
va_gfx_wait_idle
```

`va_gfx_line_set` and `va_gfx_fill_triangle` convert each logical Y once. The
callers retain the original logical-coordinate values. `va_gfx_fill_triangle`
performs scan conversion on the CPU, but it emits only SGP span operations in
the production build; it does not write GVRAM from the CPU.

The frame-builder and execution APIs are separate. `va_gfx_submit` starts the
list through `0500h`--`0506h`; `va_gfx_wait_idle` polls `0506h` bit 0. No
primitive may secretly wait, because the pure geometry calculations can run
while the submitted list is executing.

## 5. Per-frame work and command-list budget

`glass_render_scene` currently represents one clear, 256 stars, 15 grid
lines, six conditionally visible quad faces (two triangles each), and 12 cube
edges. `[SRC:demos/neon/GLASS_1_0/GLASS_SCENE.INC]`

### 5.1 Logical work

| Element | Scene quantity | Production operation |
|---|---:|---|
| clear | 1 | `SET_COLOR`, `CLS` |
| stars | 256 records | CPU nibble write after SGP idle |
| grid | 1 horizon + 9 rays + 5 horizontal = 15 | `LINE` |
| cube edges | 12 | `LINE` |
| cube faces | 6 faces, each conditionally two triangles | CPU scan conversion plus SGP `CLS` spans |
| terminator | 1 | `END` |

The original face test culls each face before issuing its two triangle fills.
The allocation below deliberately uses the uncullable upper bound of all 12
triangles; it does not rely on a particular camera angle having only three
visible faces.

### 5.2 Encoded-list budget

The current command representation provides these encoded sizes:

| Command form | Bytes | Basis |
|---|---:|---|
| `SET_WORK` plus work address | 6 | one-time setup list |
| `SET_COLOR` | 4 | opcode + word color |
| `LINE` with mode and destination block | 16 | `[SRC:io/sgp.c:768-821]` |
| `CLS` with start address and word count | 10 | `[SRC:io/sgp.c:823-834]` |
| `END` | 2 | opcode only |

For a conservative 200 physical rows per triangle, the all-12-triangle
upper bound is 2,400 spans. With one `CLS` per span, this is 24,000 bytes.
Adding 27 line commands (432 bytes), six face-color changes (24 bytes), clear
(14 bytes), grid/edge color changes (20 bytes), and `END` gives 24,492 bytes.
This calculation is `[DERIVED]`; P3 must count every emitted byte and fail
closed before the buffer is overrun.

P2 therefore reserves **32 KiB** for the production command list. This is a
payload-RAM reservation, not a statement that hardware accepts a 32 KiB list;
the documented SGP list-length limit is unknown. The P3 overflow test must
stop before writing past the reserved buffer, report the attempted byte count,
and leave the displayed page untouched.

### 5.3 No timing claim

The list budget is a memory-size calculation only. It makes no assertion about
SGP duration, framerate, CPU/SGP overlap, or real hardware performance.
Those remain P1-U4/P1-U5 `[HARDWARE_PENDING]`.

## 6. CPU verification backend

Two NASM outputs will be made from identical geometry/data and the same
initial state:

| Build | Clear / line / triangle-span path | Star path | Role |
|---|---|---|---|
| `VA_DRAW_BACKEND=sgp` | SGP command list | CPU | production candidate |
| `VA_DRAW_BACKEND=cpu` | direct packed-GVRAM reference writes | CPU | verification harness only |

The CPU implementation must use the same clipping, Y conversion, palette
index, face-color map, and selected back-page offset as the SGP implementation.
It may be slow. It must not call SGP, inspect VAEG internals, or introduce a
different scene algorithm.

The comparison contract is a fixed initial state plus a fixed selected frame:

1. build both payload variants from the same source revision;
2. start from reset with the same mode, palette, page, and scene state;
3. capture framebuffer-only output at a specified completed frame;
4. compare raw packed pixels before host composition; and
5. retain a PNG only for human diagnosis when the raw comparison differs.

P7 may promote an accepted pair to a self-regression baseline. It may never
turn a VAEG result into a hardware golden or update a baseline automatically.

## 7. Frame pipeline

```text
select hidden G0 page
  -> reset and build SGP command list for clear/grid/faces/edges
  -> submit list
  -> compute next cube projection and advance star state (pure CPU work)
  -> poll SGP idle
  -> CPU-plot 256 stars on that completed hidden page
  -> poll TSP vertical blank
  -> exchange the coupled FB0 source-window fields
  -> repeat
```

The initial implementation must serialize star writes after idle. Moving them
earlier is prohibited unless a later real-hardware investigation resolves
P1-U4. The first displayed frame must be fully rendered before it is selected;
no uninitialized page may be flashed during startup.

## 8. Palette and face mapping

Palette index 0 is black. Indices 1--7 preserve the source's blue, red,
magenta, green, cyan, yellow, and white semantics for stars/grid/edges.
Indices 8--13 are six darker face colors corresponding to source face colors
1, 2, 4, 5, 3, and 6. Indices 14--15 are reserved.

The mapping table lives in the VA backend, not in the imported scene code:

```text
source face 1 -> VA palette 8
source face 2 -> VA palette 9
source face 4 -> VA palette 10
source face 5 -> VA palette 11
source face 3 -> VA palette 12
source face 6 -> VA palette 13
```

P3 GA-3 must display and capture all sixteen entries before faces use them.
The exact 75%-brightness RGB values are a visual-tuning input, not a hardware
fact.

## 9. Packed-span endpoint decision

`CLS` operates on words, while packed 4bpp uses four pixels per word. A
triangle scanline can have non-word-aligned endpoints.

The implementation keeps the logical span exact and treats word alignment as
an access detail.  Each scanline is an inclusive `[x_left,x_right]` range from
the `y+1/2` row sample, using `ceil()` on the left intersection and `floor()`
on the right intersection.  `CLS` writes only complete interior four-pixel
words; the first and last partial words are applied once with a masked CPU
read-modify-write after SGP completion.  Thus no pixel outside the logical
span is modified.  A span with `x_left > x_right` is empty and is discarded.

The separate SGP `LINE` commands remain the intended outline stage.  No
post-outline composition, bridge, patch, erase, or other geometry-specific
repair is permitted.

| Option | Behavior | Worst-case command-list effect | Consequence |
|---|---|---:|---|
| Exact endpoint RMW | **Selected.** Keep the logical span exact, emit complete interior words with `CLS`, then mask the two endpoint words. | bounded by one record per span | Preserves geometry while retaining the SGP-only interior-word path. |
| C: SGP endpoint `PATBLT` | Not selected for this port. Use `CLS` for full words and 1x1 `PATBLT` for each partial endpoint. | up to 86,400 additional bytes before shared setup in the uncullable bound | Preserves exact endpoint pixels but requires a documented PATBLT source/destination sequence and may exceed a practical list limit. |

The endpoint RMW is a memory-transaction operation after SGP completion; it
does not replace the SGP span path or alter the logical geometry. A later,
separately approved milestone may reconsider option C only with dedicated
PATBLT evidence.

## 10. Staged implementation and acceptance

| Stage | Change | Required proof before next stage |
|---|---|---|
| GA-1 | Bare entry, stack, and idle loop | Loader ABI is approved; VAEG reaches the expected idle point. |
| GA-2 | CPU full-screen packed-4bpp color | Captured framebuffer proves 640x200 bounds and nibble order. |
| GA-3 | Palette and color bars | All 16 palette colors are distinguishable. |
| GA-4 | TSP VB polling | Bounded capture shows periodic updates; no timing claim. |
| GA-5 | `SET_WORK -> SET_COLOR -> CLS -> END` | SGP and CPU clears produce identical raw framebuffer pixels. |
| GA-6 | Two source-window pages | Page A/B capture proves no stale or partial page is shown. |
| P4-1 | CPU verify renderer | Geometry is unchanged; fixed-frame raw capture is stable. |
| P4-2 | SGP line/span renderer | Same fixed frame exactly matches the CPU verifier. |
| P5 | Connect the complete scene | At least one intermediate capture is inspected; no PC-98 draw routine exists in the payload. |
| P6 | OPNA route chosen and implemented | Separate sound tests; never gate P3--P5. |
| P7 | Local QA integration | Fixed-frame self-regression and CPU/SGP comparison are repeatable; no automatic baseline update. |

Every capture test must distinguish raw framebuffer comparison from a composed
host screenshot. A screenshot can reveal visual defects, but it is not the
pixel oracle for the backend comparison.

## 11. P2 disposition

**GO WITH RESTRICTIONS.** The documented packed-4bpp, GVRAM, framebuffer, and
SGP contracts are sufficient to design P3. Actual implementation remains
blocked by both the required P2 approval and these explicit conditions:

1. implement the approved bare-payload loader/entry/stack contract (P1-U2)
   before its first VAEG run;
2. implement the approved VA Graphics BIOS V3 enter/leave boundary and cite
   its exact ABI before its first guest call;
3. retain P1-U4/P1-U5 as hardware-pending rather than performance claims.

P3 must stop at the first failed GA proof. It may not compensate with a PC-98
BIOS/GRCG path, a host-side renderer, or a runtime CPU fallback.

## 12. P4-2 implementation evidence (2026-08-23)

P4-2 implemented the SGP fixed-frame renderer described in section 10.  Its
production candidate emits `SET_WORK`, `SET_COLOR`, `CLS`, `LINE`, and `END`
only.  It keeps exact integer-X/half-row spans, applies only the general
partial-endpoint word RMW after the submitted SGP list completes, and redraws
the intended outline list.  There is no geometry-specific repair pass.

The CPU verifier and the SGP candidate completed separately at their expected
checkpoints.  A debug-harness capture of the complete 256 KiB GVRAM backing
store and of the composed screen was byte-identical between the two builds.
Two independent SGP candidate runs were also byte-identical.  The VA2/VA3 ROM
path reached the same SGP success marker and raw checksum.

This is a VAEG functional-regression result, not a measurement of SGP speed,
an assertion about real command-list limits, or PC-88VA hardware conformance.
The detailed command-list, capture, and comparison contract is in
[`glass_p4_sgp.md`](glass_p4_sgp.md).
