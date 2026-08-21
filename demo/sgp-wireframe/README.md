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

# SGP wireframe demo

`SGPWIRE.COM` is a 16-bit real-mode PC-88VA visual test for the SGP `LINE`
command. It displays a regular tetrahedron, cube, regular dodecahedron, and
regular icosahedron in 640x400 16-color single-plane mode. Every edge is drawn
by SGP `LINE`; the CPU only rotates and projects vertices and builds the
main-RAM command list.

The solids rotate on two axes and pulse at independent rates. Edges nearer the
viewer use a bright palette index and edges farther away use a dim index. This
depth cue is intentional: the SGP has no documented general polygon or flood
fill command, so this test does not claim hardware polygon filling.

Graphics BIOS defines one 640x800 Graphic 0 framebuffer. Its two 640x400 4-bpp
halves exactly fit in 256,000 of the 262,144 single-plane GVRAM bytes. The SGP
clears the hidden half, redraws a dark reference grid and all four solids, and
then selects that half through DSA0 during vertical blank. The command list
begins with `SET WORK`, and the command-address and display-start ports are
accessed as words for real-hardware safety.

Build with:

```sh
NASM=/opt/local/bin/nasm demo/sgp-wireframe/build.sh /tmp/SGPWIRE.COM
```

Copy `SGPWIRE.COM` to a bootable PC-88VA DOS disk and run it from the command
line. Press Escape to restore the previous video mode and return to DOS.

## 256-color variant

`SGP256.COM` is a separate direct-color variant. It selects a 320x400
single-screen, single-plane display mode with G0 at 8 bpp, registers one 320x800 G0
framebuffer (the full 256000-byte single-plane G0 allocation), assigns G0 to
the direct-color priority path, and uses its two 128000-byte halves as pages
at SGP addresses `200000h` and `21f400h`.
G1 is disabled in this variant, so the two-screen G1 384-pixel alignment
constraint is not involved.
The 320-pixel logical viewport is expanded by the VA display path to the
640-pixel host surface. The vertex projection therefore doubles its vertical
excursion so that the wireframe solids retain a near-square appearance.
The existing 16-color `SGPWIRE.COM` and its Graphic 0
implementation are not changed. The 8-bit line colors use the PC-88VA 3:3:2
RGB encoding; no palette BIOS calls are required for this variant.

Build it with:

```sh
NASM=/opt/local/bin/nasm demo/sgp-wireframe/build256.sh /tmp/SGP256.COM
```
