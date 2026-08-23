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

# GLASS ORBIT P0 source extraction

## Scope

This document records only P0 of the GLASS ORBIT PC-88VA port.  The source
material is `demos/neon/GLASS_1_0`, specifically its 80286/default
640x200 scene definition.  P0 does not investigate PC-88VA video hardware,
does not write PC-88VA I/O ports, does not initialize a graphics mode, and
does not emit SGP commands.

The original work is credited as follows:

```text
Developed by ChatGPT Plus
Supervised by SimK, Neko Project 21/W Developer
```

The extracted code is marked `Ported By Maho Nakata, 2026`.  No original COM,
listing, MIDI, S98, ROM, disk image, or other binary asset is copied to the
port destination.

## Explicit P0 boundary

The maintainer required that P0 import **no drawing routines**.  Therefore
`glass_render_scene` is not transferred: it is drawing orchestration rather
than geometry.  There are no empty graphics primitive stubs in this tree.

The P0 harness reaches geometry through this path:

```text
start
  -> glass_geometry_step
       -> glass_advance_scene_tick
       -> glass_compute_cube
            -> get_sin / get_cos / glass_q14_from_dxax
       -> glass_face_is_visible
```

`math_q14_from_dxax` and `math_q8_from_dxax` are also retained verbatim from
`VIDEO286.INC` for later geometry users.  The original cube routine retains
its own `glass_q14_from_dxax` helper, so P0 makes no algorithmic substitution.

## Imported closure

| Destination | Original source | Content | P0 status |
|---|---|---|---|
| `src/glass_geometry.inc` | `GLASS_SCENE.INC:57-67, 92-102, 257-382, 463-517` | Q14 helper, star-scroll tick, cube rotation/projection, face culling | imported without drawing calls |
| `src/glass_geometry.inc` | `VIDEO286.INC:1075-1107` | sine/cosine lookup and Q14/Q8 helper routines | imported |
| `src/glass_data.inc` | `GLASS_SCENE.INC:547-1118` | scene state, grid, cube vertices/projected records/faces/edges, 256 star records | imported as data only |
| `src/glass_data.inc` | `GLASS_DATA.INC:178-210` | 256-entry Q14 sine table | imported |

The star table retains all 256 six-byte records.  NASM assertions check its
1536-byte size and the sine table's 512-byte size at assembly time.

## Deliberately excluded code

| Original area | Reason |
|---|---|
| `glass_draw_stars` | calls the original pixel primitive |
| `glass_draw_grid` | calls the original line primitive |
| `glass_draw_cube_pattern` | calls the original triangle-fill primitive |
| `glass_draw_cube_edges` | calls the original line primitive |
| `VIDEO286.INC` video functions and primitives | PC-98 GDC/GRCG/VRAM implementation is reference-only |
| `GLASS286.ASM` DOS loop/options, text, AFS, guards | not geometry/data |
| `GLASS_OPNA.INC` and `OPL3.INC` | deferred sound milestone |

## Build

P0 is a flat NASM binary build only.  It does not prove that a PC-88VA can
run or display the result.

```sh
NASM=/opt/local/bin/nasm \
  demos/va/glass-orbit/build-p0.sh /private/tmp/glass_orbit_p0.bin
```

The P0 source deliberately has no `org` directive: the eventual payload load
origin is P1-8 work.  The harness initializes its code/data segment, executes
one geometry step, and then idles.  It contains no graphics output and no DOS,
BIOS, TSP, SGP, or OPNA operation.

## Local VAEG smoke wrapper

P0 itself deliberately makes no loader ABI claim.  A separate local-only
`GLASSP0.COM` wrapper uses the already-supported PC-Engine DOS COM path only
to execute the same geometry/data closure once under VAEG.  It does not call a
graphics BIOS service, configure video, access an I/O port, create an SGP
list, or establish the future bare-payload entry convention.

Create a bootable local image from a maintainer-local PC-Engine 2HD template:

```sh
NASM=/opt/local/bin/nasm \
  demos/va/glass-orbit/build-bootable-d88.sh \
  /path/to/pcengine-bootable.d88 \
  /private/tmp/glass-orbit-p0-bootable.d88
```

The generated D88 retains the template's non-free PC-Engine system files.  It
is a local validation artifact only and must not be committed or distributed.
At the DOS prompt, execute `GLASSP0`. The local wrapper returns to the DOS
prompt only after the shared geometry closure has returned.

For a bounded VAEG smoke run, the standard headless keyboard path types that
command and captures the resulting DOS screen:

```sh
NASM=/opt/local/bin/nasm \
  demos/va/glass-orbit/run-vaeg-smoke.sh \
  /path/to/pcengine-bootable.d88 \
  /absolute/path/to/vaeg \
  /absolute/path/to/rom-directory \
  /private/tmp/glass-orbit-p0-smoke
```

The captured screen must visibly show the initial `Ready` prompt, the
`GLASSP0` command, and a second `Ready` prompt. This is solely a VAEG execution
smoke: it proves that the local DOS wrapper reaches the geometry closure and
returns to the command processor. It is not evidence for the P1-8
bare-payload convention, 640x200 packed 4bpp, V3 display setup, SGP behavior,
or real PC-88VA hardware compatibility.

## P0 verification and non-claims

P0 passes when NASM builds the binary and static inspection confirms that no
retained source contains a drawing primitive call or PC-98 graphics I/O.

P0 does **not** establish any of the following:

- PC-88VA V3 640x200 mode availability or packing;
- GVRAM addressability or double buffering;
- palette semantics;
- SGP command construction or completion behavior;
- timing or real-hardware compatibility;
- OPNA I/O addresses or audio playback.

Those items remain P1/P2 work and must not be implemented until their
documentation and design gates are approved.
