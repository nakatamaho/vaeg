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
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->

# Faithful NEON4 VA port master plan

## Scope

Rewrite the local PC-9801 `demos/NEON4_1_0` geometric-solid demo as a
PC-88VA 16-bit NASM COM. Preserve the eight-scene order, scene timing,
geometry families, palette intent, and scene-specific carrier/grid elements.
Replace only hardware-specific backends: GRCG/EGC/PEGC, PC-98 VRAM windows,
IRQ2/INT 0Ah, direct PC-98 sound probing, and PC-98 text VRAM.

The original untracked source and local evidence documents are read-only.
Generated COM/D88 files stay outside Git. Do not modify ROM or disk images.

## Step files

| Step | Detail file | Deliverable |
|---|---|---|
| 00 | `.plan/neon4-va-faithful-00-audit.md` | Baseline and source/evidence freeze |
| 01 | `.plan/neon4-va-faithful-01-video.md` | VA mode, black G0, palette, composition, teardown |
| 02 | `.plan/neon4-va-faithful-02-geometry.md` | Eight-scene scaled wireframe SGP renderer |
| 03 | `.plan/neon4-va-faithful-03-input.md` | DOS ESC path and optional VA Music BIOS boundary |
| 04 | `.plan/neon4-va-faithful-04-build-d88.md` | NASM/CMake build and disposable D88 |
| 05 | `.plan/neon4-va-faithful-05-iterate.md` | Repeated VAEG launch/debug loop and human gate |

## Global rules

- Do not edit `demos/NEON4_1_0/`, `docs/98io/`, `docs/tekumani/`, ROMs, or
  disk images.
- Do not add PC-98 ports, PEGC/GRCG/EGC registers, INT 0Ah handlers, or
  guessed VA registers.
- Do not draw animated pixels with CPU loops. SGP command lists and the
  58-byte work area must be in main RAM.
- Use WORD writes for SGP command pointer ports. Initialize `SET_WORK` before
  every list. Never write the active list while SGP is busy.
- Use black G0 as the base. Draw carrier/background geometry only in scene
  routines that correspond to the original source; no permanent checkerboard.
- Use DOS `INT 21h/AH=06h/DL=FFh` polling and ASCII ESC (`1Bh`) for exit.
- Source/comments/diagnostics are English. New files carry the 2-clause BSD
  header. Keep generated artifacts out of Git.

## Adversarial plan review

| Threat | Mitigation |
|---|---|
| A lower model ports only a cube and calls it NEON4 | Require all eight scene routines and a source-to-scene mapping table |
| A lower model restores the checkerboard from the previous prototype | Explicit black-G0 rule and scene-specific background rule |
| ESC appears to work only under a host keyboard injection | DOS INT 21h path plus a bounded VAEG input test and manual ESC gate |
| SGP LINE descriptor is emitted in the wrong position | Step 02 specifies `LINE`, mode, then six descriptor words, with a static parser check |
| Geometry overflows the 320x200 page | Scale coordinates first, clamp/reject every endpoint, and assert page bounds in build-time data |
| Music failure blocks graphics | Step 03 makes Music BIOS optional and initializes it only after graphics is visible |
| VAEG success is reported as hardware success | Step 05 separates emulator launch, screenshot evidence, and human hardware gate |

## Completion

The task is complete only when the VA COM builds, the eight scenes run in a
fresh VAEG session, scene-specific elements are visible without a permanent
checkerboard, DOS ESC exits and restores video, the disposable D88 contains the
COM, and the report distinguishes proven VA behavior from deferred 8bpp/music
work. The final step repeats build/launch/fix until the program is launchable;
it must not stop at the first assembler success.

## Execution status

- [x] 00 audit and adversarial plan review saved on `M97a`.
- [x] 01 VA video mode, black G0, palette, composition, teardown implemented.
- [x] 02 all eight scene routines and SGP LINE command generation implemented.
- [x] 03 DOS ESC polling path implemented; VA Music BIOS remains optional and
      is deliberately not allowed to block graphics bring-up.
- [x] 04 NASM build script, CMake `neon4va_com` target, and disposable D88
      procedure verified.
- [ ] 05 final repeated launch/debug loop and maintainer hardware gate.
