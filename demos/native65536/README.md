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
-->

# 640x200 65536-colour pictures (M149)

The two pictures of [`demos/hyperframe+`](../hyperframe+/README.md) (the
ray-traced spheres and the Mandelbrot close-up), shown by the PC-88VA's own
graphics without any extra board: Graphic 0, single plane, 640x200, 16 bpp
direct colour (G6 R5 B5 words), under MS-DOS 4.0 in V3 mode.
Task: [`M149`](../../docs/agents/tasks/M149_hyperframe_640x400_display.md).

| File | What |
|---|---|
| `vashow.asm` | `VASHOW.COM` (NASM). Sets the 640x200 16-bpp Graphic 0 screen with the video BIOS (`INT 8Fh` ScnMode `BX=A002h`, `CL=16`; DefBuf 640x200; DefWin 200 lines; Compose G0 only), then `GRMODE=B462h`, `GRRES=0303h`, `FBW=1280`, `FBL=DSH=200`, direct colour. Reads the picture in 32 KiB pieces and copies each to GVRAM through the bank-4 window at `A0000h` (port `153h` = `54h`, write mode `580h` = `10h`), switching the window in only for the copy. A key restores the video state it found. Without a file name it shows the first `*.G16` of the current directory. |
| `vashow.txt` | `VASHOW.TXT` on the disks. |
| `build-d88.py` | Renders the pictures with `../hyperframe+/raytrace.py` and `mandel.py --height 200` (the same 4:3 views at 640x200, pixels 2.4 times as tall as wide, 3x3 supersampling), reduces them to G6 R5 B5 with a 4x4 ordered dither (integer arithmetic) and builds both disks and their `.d88.xz`. |
| `va65536-tetra-msdos4.d88.xz` | Bootable disk with `TETRA.G16`. |
| `va65536-mandel-msdos4.d88.xz` | Bootable disk with `MANDEL.G16`. |

## The disks

The allow-listed distribution images in this directory are:

```text
va65536-tetra-msdos4.d88.xz
va65536-mandel-msdos4.d88.xz
```

Each boots by itself under the maintainer-approved MS-DOS 4.0 exception of
`AGENTS.md`, like [`demos/disks/all-demos-msdos4.d88.xz`](../disks/README.md):
its system is the FreeDOS-88VA project's MIT-licensed MS-DOS 4.0 for the
PC-88VA (release `msdos4-va.2`, `msdos4-pc88va-2hd.d88`, SHA-256
`7c4b141d31e0034120e0b07eb93b9bea808e1c54822e48e15189cc7398385db0`), with
the release's `A:\LICENSE.TXT`. Everything else is built from this
directory's and `demos/hyperframe+`'s sources. Listing:

```text
A:\IO.SYS, A:\MSDOS.SYS, A:\COMMAND.COM          MS-DOS 4.0 system
A:\AUTOEXEC.BAT, A:\CONFIG.SYS                    MS-DOS start-up files
A:\LICENSE.TXT, A:\README.TXT                     MS-DOS release notices
A:\VASHOW.COM                                     vashow.asm
A:\VASHOW.TXT                                     vashow.txt
A:\TETRA.G16      (va65536-tetra only)            raytrace.py --height 200
A:\MANDEL.G16     (va65536-mandel only)           mandel.py --height 200
```

Rebuild both (the raw D88s must be outside the repository; NASM on the
PATH or in `NASM`):

```sh
python3 demos/native65536/build-d88.py --output-dir /tmp/va65536-disks
```

The builder uses the checks of `demos/hyperframe+/build-d88.py`: it refuses
any other MS-DOS source image, gives directory entries a fixed time stamp,
checks that the image holds only the release's boot sector and kept MS-DOS
files plus `VASHOW.COM`, `VASHOW.TXT` and its picture (error codes
`M149_VASHOW_*`), and checks the `.xz` round trip. `--check` rebuilds and
compares with the committed images instead of writing them. The rendering
uses floating point; the committed images were built on macOS arm64.

## Running it

Write a disk out, for example `xz -dc
demos/native65536/va65536-mandel-msdos4.d88.xz > /tmp/va65536-mandel.d88`,
boot it in drive 1 in V3 mode, then run `VASHOW`. A key returns to MS-DOS.

- vaeg: the picture fills the screen; the rendered output matches the
  dithered G6 R5 B5 data to within one level of 255 on every sampled pixel.
- A real PC-88VA: not yet tried. The 640-dot 16-bpp setting (`GRRES`
  `0303h`, `FBW` 1280) follows vaeg's model and the 320x200 65536-colour
  demos' measured setup; whether the machine fetches 640 16-bpp dots per
  line is to be confirmed on hardware.
