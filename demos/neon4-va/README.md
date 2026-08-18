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
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
-->

# NEONVA

`NEONVA.COM` is the first PC-88VA port of the geometric NEON4 demo. It keeps
the moving nested-solid scene but replaces the PC-9801 GRCG/EGC/PEGC renderer
with the VA SGP `SET_WORK`, `SET_COLOR`, `CLS`, and `LINE` command sequence.

Build with NASM:

```sh
./build.sh /tmp/neonva-build
```

Copy `NEONVA.COM` to a disposable VA guest disk and run it from DOS. A
headless launch can inject the DOS command with a two-line script:

```text
@wait 120
NEONVA
```

Then run VAEG with the normal VA ROM directory and the disposable disk:

```sh
VAEG_SCREEN_EXIT_MS=15000 \
  build/linux-debug/sdl2/vaeg --no-cfg --model va \
  --roms /path/to/roms --fdd1 /path/to/neonva-boot.d88 \
  --headless-input-script /path/to/neonva-input.txt
```

The expected screen is a patterned Graphic 0 background with three nested,
moving, coloured wireframe solids drawn by SGP. `ESC` restores the saved video
state and exits.

The demo targets the verified 320x200 4-bpp single-plane VA mode. The Graphic 0
checkerboard is a bring-up write; animated geometry is emitted only as SGP
commands. The command list and the 58-byte work area reside in main RAM, and
SGP address ports are written as words. This first port deliberately does not
use PC-98 GRCG, EGC, PEGC, INT 0Ah, direct OPNA probing, or a guessed 256-colour
path. Music and input beyond ESC are deferred until the video path is stable.

The original `demos/NEON4_1_0/` reference tree is not modified.
