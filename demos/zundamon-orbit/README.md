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

The final private demo is `ZUNDAORB.COM` plus a replaceable `ZUNDAMON.BIN`
30-scale VA8 atlas. The artwork is converted from one PSD composite to a
96x128 source with 256 possible VA8 values. The PSD and standalone generated
binaries are local files and are not stored in Git.
The source material permits redistribution, commercial use, and modification;
credit and Niconico content-tree registration are optional. The PSD itself is
omitted because of its size.

Runtime requirement (M109): 512 KiB of main RAM. The atlas is loaded into
conventional RAM at `40000h`-`5FFFFh` and SGP reads it there, so no I/O bank
memory board is needed (earlier builds required a 16 MiB board at port
`1D0h`, which the maintainer's PC-88VA2 lacks). If DOS did not give the
program memory up to `60000h` it reports `M109_FAIL: needs conventional RAM
up to 60000h for the atlas.` and exits. On a 24.8 kHz display the 200-line
picture is shown with each line doubled (port `100h` RSM = 01).

## Build

The artwork is the first standing pose (the layers visible as stored) of
`ずんだもん立ち絵素材2.3.psd`, a free character material whose terms permit
redistribution (maintainer, M109). The profile tables made from it (depth,
HUD and status) are tracked in [`256/ida/`](256/ida/). One command rebuilds
the distribution disk from the PSD:

```sh
demos/zundamon-orbit/build-from-psd.sh [--python PYTHON] \
  /path/to/ずんだもん立ち絵素材2.3.psd \
  /path/to/pcengine110-bootonly.d88 /private/tmp/zundamon-orbit.d88
```

It makes `ZUNDAMON.BIN` with ImageMagick `convert` when present, otherwise
with psd-tools (`PYTHON` must import `psd_tools` and `PIL`; the two decoders
scale differently, so the committed disk is the psd-tools result), checks
that the tracked tables regenerate from it, and writes
`demos/disks/zundamon-orbit.d88.xz`. The step-by-step commands below remain.

Requirements: Python 3, NASM, and ImageMagick `convert` or psd-tools.

Build the replaceable atlas:

```sh
python3 demos/zundamon-orbit/tools/build_zundamon_orbit_psd_atlas.py \
  [--decoder convert|psd-tools] /path/to/zundamon.psd /path/to/zundamon.bin
```

Build `ZUNDAORB.COM` and an optional listing:

```sh
M98Y_PROFILE=private \
M98Y_PRIVATE_PROFILE_DIR=demos/zundamon-orbit/256/ida \
M98Y_PRIVATE_ATLAS=/path/to/zundamon.bin \
  demos/zundamon-orbit/256/build.sh \
  /path/to/ZUNDAORB.COM /path/to/ZUNDAORB.LST
```

Create a local boot disk from the PC-Engine 2HD template. The disk receives
`ZUNDAORB.COM` and `ZUNDAMON.BIN`:

```sh
M98Y_PROFILE=private \
M98Y_PRIVATE_PROFILE_DIR=demos/zundamon-orbit/256/ida \
  demos/zundamon-orbit/build-local-d88.sh \
  /path/to/pcengine110-bootonly.d88 /path/to/zundamon.bin \
  /path/to/ZUNDAORB.d88
```

To create the freely distributable non-bootable data disk, use the same
private profile and atlas with `build-d88.sh`. This writes the compressed
companion to `demos/disks/zundamon-orbit.d88.xz`:

```sh
M98Y_PROFILE=private \
M98Y_PRIVATE_PROFILE_DIR=demos/zundamon-orbit/256/ida \
  demos/zundamon-orbit/build-d88.sh \
  /path/to/pcengine110-bootonly.d88 /path/to/zundamon.bin \
  /private/tmp/zundamon-orbit.d88
```

The distribution disk contains only `ZUNDAORB.COM` and `ZUNDAMON.BIN`; it is
non-bootable and has no PC-Engine system files.

`ZUNDAORB` starts with four instances. `/N1`-`/N64`, `A`/`Z`, `Q`/`E`,
`W`/`S`, `O`/`P`, `LEFT`/`RIGHT`, `SPACE`, and `ESC` control the demo.

The remaining scripts under `tools/` are regression and asset-format tools;
they are not additional demos or required runtime files.
