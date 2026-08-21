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

# 65536-color single-page pseudo-sprite

`SGP655S.COM` is a compact direct-color pseudo-sprite teaching demo. It uses
one 320x200 visible Graphic 0 page backed by a 320x400, 16-bpp source surface.
The lower source half is reserved but is not used for page exchange.

The frame is rebuilt in place with SGP commands:

1. one linear `CLS` of the visible 320x200 page;
2. an SGP `LINE` grid background; and
3. four moving, shaded 16x16 orbs through transparent 16-bpp `BITBLT`.

There is no G1 page, no DSA page flip, and no CPU pixel loop. The single-page
choice is intentional: it demonstrates direct-color SGP operations with the
smallest possible framebuffer state. A visible redraw can tear if the SGP
workload exceeds one field; this is the tradeoff for avoiding a second page.

Build:

```sh
NASM=/opt/local/bin/nasm demo/sgp-pseudo-sprite/65536/build.sh /tmp/SGP655S.COM
```

The program uses the same verified direct-color setup as the 65536-color
wireframe track: `GRMODE=0xb462`, `GRRES=0x1313`, `FBW=640`, `FBL=400`, and
`DSH=200`. SGP command and display-start registers are written as words.
