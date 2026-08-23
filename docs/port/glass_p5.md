<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN
NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# GLASS ORBIT P5 complete-scene connection

P5 connects the retained GLASS scene data to the accepted P4 SGP renderer. A
P5 build emits the perspective grid and rotating cube faces through one SGP
command list, applies exact endpoint RMW, emits the outline once through a
second SGP list, and then plots the 256 retained stars through the CPU GVRAM
aperture. The frame counter and star scroll state advance once per completed
frame, and the PC-88VA Keyboard BIOS ESC path leaves the graphics mode through
the existing loader continuation.

The P4 fixed-frame source remains the regression reference. P5 is enabled only
by compiling `glass_orbit_p5_sgp.asm`, which wraps the common P4 backend with
`GLASS_P5=1`; it does not add a second polygon rasterizer or a host renderer.
No DOS `INT 21h` service is used by the payload.

## Scope and non-claims

This stage proves scene connectivity and functional VAEG rendering only. It
does not claim SGP cycle accuracy, a real-PC-88VA timing result, or hardware
equivalence. P5 uses the GA-6-proven two-half FB0 source window as a real
render/display pair: the target half is selected by SGP base `200000h` or
`20FA00h`, CPU endpoint/stars use the corresponding `A000h` or `AFA0h` GVRAM
aperture segment, and `$RollTo` changes the displayed source only after the
SGP lists and CPU stars are complete. The first frame is built while graphics
are disabled, then one complete hidden page is selected at the next VBLANK.
This prevents the visible page from being cleared or repaired during a frame.

The wrapper exposes explicit `FRAME_READY`, geometry-complete, star-complete,
and SGP-idle state bytes. `FRAME_READY` is set only after both SGP lists and
the CPU star pass finish, checked across the VBLANK wait, and cleared only
after `$RollTo` selects the completed target half. The endpoint path remains
an exact masked RMW for partial words; it is not a full-word paint followed by
an erase. The temporal checker therefore treats endpoint/interior word
ownership and page-ready ordering as separate invariants.

The P5 wrapper compiles the common backend at stage 2. Consequently the first
list contains grid and face spans only; the outline list is submitted exactly
once after endpoint RMW. Stage 3 is intentionally not used by P5 because it
would enqueue the outline twice.

## Build

```sh
NASM=nasm demos/va/glass-orbit/build-p5-sgp.sh /absolute/path/GLASSP5S.COM
```

The generated COM and raw payload are local artifacts. A local bootable image
can be made from a PC-Engine validation template with
`build-p5-sgp-bootable-d88.sh`; it must not be committed or pushed.

## Temporal QA

`glass_orbit_p5_temporal.debug` captures three post-`FRAME_READY` checkpoints
at `3000:177D`, after the page selection and graphics enable, before the next
hidden-page build.
The screenshots are therefore presented frames, not construction snapshots.
`tools/verify-p5-temporal.py` checks the one-list/one-outline call graph, the
shared exact-span alignment and slope matrices, and optional 256-KiB raw GVRAM
capture sizes/page activity. With `--registers`, it also requires every
checkpoint to be at the post-present `3000:177D` address. Its
`temporal_overfill=0` result is a write-rule invariant derived from the exact
endpoint partition; it is not a claim about cycle timing or real-hardware
conformance.

## P5 acceptance

The VAEG capture must show the horizon/rays/grid, 256-star sky, and the
rotating filled cube with its intended outline colors. The raw GVRAM capture
must remain free of the P4 endpoint audit area, and the existing P4 face and
CPU/SGP checks remain the correctness oracle for the face renderer.
