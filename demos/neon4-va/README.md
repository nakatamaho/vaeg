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

# NEON4 PC-88VA faithful wireframe port

`neonva.asm` is a fresh 16-bit COM rewrite of the local PC-9801
`demos/NEON4_1_0` reference. The authored eight-chapter order is retained:
signal seed, facet assembly, material assembly, morph gate, raster transfer,
surface wave, grid arrival, and solid finale. The PC-98 GRCG/EGC/PEGC backend
is replaced by PC-88VA 320x200 4bpp Graphic 0/Graphic 1 composition and SGP
`LINE` command lists.

The scene coordinate system is reduced from the reference 640x400 space to
the VA 320x200 surface. Graphic 0 is cleared to black with SGP `CLS`; the
scene-specific carrier, ribbon, grid, and corona elements are emitted only by
their corresponding scene. There is no permanent checkerboard. Filled faces
are intentionally represented by painter-ordered wireframe edges because the
verified SGP interface supplies no flood-fill command in this demo.

## Build

```sh
NASM=/opt/local/bin/nasm demos/neon4-va/build.sh /private/tmp/neon4-va
```

The output is `/private/tmp/neon4-va/NEONVA.COM`. CMake also provides the
`neon4va_com` target when NASM is available:

```sh
cmake --build --preset linux-debug --target neon4va_com
```

## Run on VAEG

Install `NEONVA.COM` into a disposable copy of a PC-Engine system disk with
`tools/pc88va/pcengine_disk.py install`, boot the disk, and enter `NEONVA` at
the DOS prompt. The program uses DOS `INT 21h/AH=06h` with `DL=FFh` and exits
only when the returned character is ASCII `ESC` (`1Bh`). It restores the saved
VA video mode and palette before returning to DOS.

Rendering uses a main-RAM command list and the mandatory 58-byte SGP work
area. Each list begins with `SET_WORK`, clears the hidden Graphic 1 page with
`CLS`, emits the current scene's `SET_COLOR`/`LINE` records, and ends with
`END`. The hidden page is rendered before TSP VBLANK polling and DSA1 page
exchange.

The hardware evidence and conversion decisions are recorded in
`docs/modernization/neon4-pc98-va-faithful-audit.md`.
