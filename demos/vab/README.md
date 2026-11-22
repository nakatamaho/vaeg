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
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Video Art Board samples (M160)

A self-booting 2D disk that shows two 320x200 65536-colour pictures on the
NEC PC-8801-17 Video Art Board, for a PC-8801 or a PC-88VA in V1/V2 mode.
The scenes are this directory's own, different from the HyPER-FRAME+
pictures of [`demos/hyperframe+`](../hyperframe+/README.md).
Task: [`M160`](../../docs/agents/tasks/M160_vab_demo.md).

| File | What |
|---|---|
| `raytrace.py` | Ray tracer (Python standard library only). A gold mirror sphere in a ring of seven rainbow-coloured spheres on a terracotta-and-cream checkerboard under a sunset sky. 320x200, 4x4 supersampling, composed for a 4:3 monitor. Writes `RING.RGB` (R G B bytes, rows top to bottom, 192000 bytes). |
| `mandel.py` | A Mandelbrot-set close-up of Elephant valley (centre 0.2852 + 0.0117i, width 0.0072, 6000 iterations), smooth escape-count colouring through a violet-rose-orange-cream cyclic palette, 4x4 supersampling. Writes `ELEPHANT.RGB` in the same format. |
| `build-d88.py` | Assembles the IPL (a Z80 program written out in the script), converts both pictures to the board's format and builds the disk and its `.d88.xz`. |
| `vab-samples.d88.xz` | The disk. |

## The disk

The distribution image allowed in this directory is:

```text
vab-samples.d88.xz
```

It has no file system and no system files; every byte is built here:

```text
track 0 sector 1          IPL, build-d88.py (loaded at C000h by the boot ROM)
linear sectors 1-512      RING.RGB as board memory, raytrace.py --samples 4
linear sectors 513-1024   ELEPHANT.RGB as board memory, mandel.py --samples 4
other sectors             zero
```

A picture is the board's 128 KiB: 320x200 words, row by row, bits 15-10
green, 9-5 red and 4-0 blue, little endian, ordered-dithered from the 8-bit
render, then zero padding.

The IPL finds the board's card among the extended-RAM card positions (port
E3h, read-back of port B4h), turns its display on (B4h = 47h), reads 512
sectors into the board's four 32 KiB banks through the N88-BASIC ROM's sector
read routine (369Ah, the same entry X88000's Video Art Board sample uses)
and the extended-RAM write window (E2h = 10h), waits for a key, loads the
next picture and so on. Without the board it prints `NO VIDEO ART BOARD
(PC-8801-17)` and stops.

Rebuild (the raw D88 must be outside the repository):

```sh
python3 demos/vab/build-d88.py --output-dir /tmp/vab-disk
```

`--check` rebuilds and compares with the committed image instead of
writing it (error codes `M160_VAB_*`). The pictures use floating point; a
host whose libm rounds differently could change single bytes of a picture.

## Running it

Write the disk out, for example `xz -dc demos/vab/vab-samples.d88.xz >
/tmp/vab-samples.d88`, and boot it in drive 1.

- vaeg: turn on "PC-8801-17 ビデオアートボード" in the menu, choose V1/V2 mode
  (V2 H or V2 S) or a PC-8801 machine, and reset. The ray-traced picture
  appears while it loads; any key switches to the Mandelbrot picture and
  back.
- Real hardware has not been tried. The board is a PC-8801 bus board; the
  disk is a 2D (40-cylinder) image.
