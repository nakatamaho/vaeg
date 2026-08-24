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
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# GLASS ORBIT

The authoritative PC-88VA port is the P5 SGP scene with OPNA/YM2608 audio.
Earlier GA, P0, and standalone P4 milestone payloads are intentionally not
kept in this directory.

Build the final verification loader with:

```sh
NASM=nasm demos/glass-orbit/build-p5-sgp.sh /absolute/path/GLASSP5S.COM
```

`build-p5-sgp-bootable-d88.sh` creates a local bootable validation disk from
a supplied template. The bootable image is a local artifact and must not be
committed. `run-vaeg-p5-sgp.sh` is the final VAEG capture/temporal-QA entry
point.

The shared SGP backend and exact 4bpp span/convex-polygon helpers under
`src/` are implementation dependencies of P5, not separate demo variants.
The audio path is OPNA/YM2608 only; OPL is intentionally out of scope.
