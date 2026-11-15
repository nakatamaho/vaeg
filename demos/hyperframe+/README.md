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

# HyPER-FRAME+ samples (M149)

Two 640x400 full-colour pictures for the Digital Arts HyPER-FRAME+ frame
buffer, each sent from a PC-88VA in V3 mode (MS-DOS 4.0) through I/O only.
Task: [`M149`](../../docs/agents/tasks/M149_hyperframe_640x400_display.md).

| File | What |
|---|---|
| `raytrace.py` | Ray tracer (Python standard library only). A mirror sphere rests on red, green and blue spheres; the four touch at the vertices of a regular tetrahedron in the middle of a checkerboard tile floor. 640x400, 3x3 supersampling, composed for a 4:3 monitor. Writes `TETRA.RGB` (R G B bytes, rows top to bottom, 768000 bytes) and optionally a PPM. |
| `mandel.py` | A Mandelbrot-set close-up of Seahorse valley (centre -0.743643887037151 + 0.131825904205330i, width 1.65e-4, 2400 iterations), smooth escape-count colouring through a blue-white-gold cyclic palette, 3x3 supersampling. Writes `MANDEL.RGB` in the same format. |
| `hfshow.asm` | `HFSHOW.COM` (NASM). Initialises the board as the JPGVHF viewer does, sends the picture row by row (Y once per row; X, then R, G, B per pixel), waits for a key and stops the board (D0=00, EE=00). Without a file name it shows the first `*.RGB` of the current directory. `HFSHOW /STAY` leaves it displaying; `HFSHOW file` shows a given 640x400x24 picture. |
| `hfshow.txt` | `HFSHOW.TXT` on the disk. |
| `build-d88.py` | Builds both bootable disks and their `.d88.xz` (below). |
| `hfshow-msdos4.d88.xz` | Bootable disk with `TETRA.RGB`. |
| `hfmandel-msdos4.d88.xz` | Bootable disk with `MANDEL.RGB`. |

## The disks

A picture is 768000 bytes, and a 2HD disk has room for one beside MS-DOS,
so each picture has its own disk. The allow-listed distribution images in
this directory are:

```text
hfshow-msdos4.d88.xz
hfmandel-msdos4.d88.xz
```

Each boots by itself under the maintainer-approved MS-DOS 4.0 exception of
`AGENTS.md`, like [`demos/disks/all-demos-msdos4.d88.xz`](../disks/README.md):
its system is the FreeDOS-88VA project's MIT-licensed MS-DOS 4.0 for the
PC-88VA (release `msdos4-va.2`, `msdos4-pc88va-2hd.d88`, SHA-256
`7c4b141d31e0034120e0b07eb93b9bea808e1c54822e48e15189cc7398385db0`), with
the release's `A:\LICENSE.TXT`. Everything else is built from this
directory's sources. Listing:

```text
A:\IO.SYS, A:\MSDOS.SYS, A:\COMMAND.COM          MS-DOS 4.0 system
A:\AUTOEXEC.BAT, A:\CONFIG.SYS                    MS-DOS start-up files
A:\LICENSE.TXT, A:\README.TXT                     MS-DOS release notices
A:\HFSHOW.COM                                     hfshow.asm
A:\HFSHOW.TXT                                     hfshow.txt
A:\TETRA.RGB      (hfshow-msdos4 only)             raytrace.py --samples 3
A:\MANDEL.RGB     (hfmandel-msdos4 only)           mandel.py --samples 3
```

Rebuild both (the raw D88s must be outside the repository; NASM on the
PATH or in `NASM`):

```sh
python3 demos/hyperframe+/build-d88.py --output-dir /tmp/hfshow-disks
```

The builder refuses any other MS-DOS source image, gives directory entries
a fixed time stamp, checks that the image holds only the release's boot
sector and kept MS-DOS files plus `HFSHOW.COM`, `HFSHOW.TXT` and its
picture, built here (error codes `M149_HFSHOW_*`), and checks the `.xz`
round trip. `--check` rebuilds and compares with the committed image
instead of writing it. The pictures use floating point (`math.sqrt`,
`math.log`, `**`); the committed images were built on macOS arm64, and a
host whose libm rounds differently could change single bytes of a picture.

## Running it

Write a disk out, for example `xz -dc
demos/hyperframe+/hfmandel-msdos4.d88.xz > /tmp/hfmandel.d88`, boot it in
drive 1 in V3 mode, then run `HFSHOW`.

- vaeg: enable Device - HyPER-FRAME+... and choose "HyperFrame+ ON" (the
  board's output over the whole window) or "右上に出力" (top right). The
  picture appears; a key returns to the MS-DOS screen. In headless batch
  runs, about 1 run in 24 ended with "the picture file is shorter than
  768000 bytes" (a DOS read failure); see the M149 task file.
- A real PC-88VA with the board: the transfer works (M149 measured the
  writes and read-backs), but the board's output needs its main RGB input
  (CN2) connected, which has not been done yet; see the task file.
