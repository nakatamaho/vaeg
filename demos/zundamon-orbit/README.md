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

# Zundamon orbit final demo

The final private demo is `IDAORB.COM` plus a replaceable `ZUNDAMON.BIN`
30-scale VA8 atlas. The artwork is converted from one PSD composite to a
96x128 source with 256 possible VA8 values. The PSD and generated binaries
are local files and are not stored in Git.

## Build

Requirements: Python 3, ImageMagick `convert`, NASM, and a local private
profile containing the generated depth, HUD, and status tables.

Build the replaceable atlas:

```sh
python3 demos/zundamon-orbit/tools/build_zundamon_orbit_psd_atlas.py \
  /path/to/zundamon.psd /path/to/zundamon.bin
```

Build `IDAORB.COM` and an optional listing:

```sh
M98Y_PROFILE=private \
M98Y_PRIVATE_PROFILE_DIR=/path/to/private/profile \
M98Y_PRIVATE_ATLAS=/path/to/zundamon.bin \
  demos/zundamon-orbit/256/build.sh \
  /path/to/IDAORB.COM /path/to/IDAORB.LST
```

Create a local boot disk from the PC-Engine 2HD template. The disk receives
`IDAORB.COM` and `ZUNDAMON.BIN`:

```sh
M98Y_PROFILE=private \
M98Y_PRIVATE_PROFILE_DIR=/path/to/private/profile \
  demos/zundamon-orbit/build-local-d88.sh \
  /path/to/pcengine110-bootonly.d88 /path/to/zundamon.bin \
  /path/to/IDAORB.d88
```

`IDAORB` starts with four instances. `/N1`-`/N64`, `A`/`Z`, `Q`/`E`,
`W`/`S`, `O`/`P`, `LEFT`/`RIGHT`, `SPACE`, and `ESC` control the demo.

The remaining scripts under `tools/` are regression and asset-format tools;
they are not additional demos or required runtime files.
