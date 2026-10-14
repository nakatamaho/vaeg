<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# Self-booting SGP demo disk

This directory builds a 2HD disk (or, on request, a 2DD disk) that boots the SGP demos directly from the
PC-88VA ROM, with no PC-Engine, MS-DOS or other operating system. It follows
the boot method of other self-booting PC-88VA software, so a demo that fails
on real hardware can be tried without any operating-system code in the way.

Everything on the disk is assembled from source in this repository:

| File | Role |
|---|---|
| [`ipl.asm`](ipl.asm) | Boot sector. The ROM reads it to `3000:0000` and jumps there; it reads the loader with the ROM floppy disk BIOS (`INT 80h`) to `2000:0000`. |
| [`loader.asm`](loader.asm) | Menu and loader. It lists the demos, reads the chosen `.COM` image to `4000:0100` with a minimal PSP and runs it as DOS would. |
| [`build-boot-d88.py`](build-boot-d88.py) | Builds the demos with their own build scripts, assembles the IPL and loader, and writes the D88. |

## Building

```sh
python3 demos/sgp-boot/build-boot-d88.py --output /tmp/sgpboot.d88
```

This writes a 2HD image; `--format 2dd` writes a 2DD one with the same
contents. NASM and Python 3 are required (`NASM=/path/to/nasm` selects
another NASM).
The output is reproducible: the same sources give the same image. It is a
bootable validation disk, so it must be written outside the repository and
must not be committed (`AGENTS.md`); the builder refuses a path inside it.

## PC-Engine or MS-DOS bootable disk

For comparison, the same builder also makes an ordinary bootable disk with
the same eleven demos from a system disk: the user's own PC-Engine 1.05/1.1
system disk (private media), or the PC-88VA MS-DOS 2.0 or 4.0 disk of the
FreeDOS-88VA project (MIT-licensed; for example `msdos4-pc88va-2hd.d88` of
release `msdos4-va.2` at <https://github.com/FreeDOS-88VA/MS-DOS/releases>):

```sh
python3 demos/sgp-boot/build-boot-d88.py \
  --system-source /path/to/msdos4-pc88va-2hd.d88 --output /tmp/sgp-dos.d88
```

(`--pcengine-source` is the former name of `--system-source`.) It copies the
system disk with `tools/pc88va/pcengine_disk.py vanilla`, which keeps the
PC-Engine system files, or MS-DOS's system files, `CONFIG.SYS`,
`AUTOEXEC.BAT`, `LICENSE.TXT` and `README.TXT`, and installs `16\`, `256\`
and `65536\` (the pseudo-sprite and wireframe programs of each colour depth,
as on the distribution disks). At the prompt run, for example,
`16\SGPD_7A` or `65536\SGPWIRE`. The disk holds the system's files, so it
stays outside the repository; directory time stamps make two builds differ
in a few bytes.

## Running

Insert the disk in drive 1 and turn the PC-88VA on (or reset). In vaeg:

```sh
vaeg --model VA2 --fdd1 /tmp/sgpboot.d88
```

The menu lists:

| Key | Demo |
|---|---|
| A-D | pseudo-sprites, 16 colours, M7a-M7d (`SGPD_7A`-`SGPD_7D`) |
| E | pseudo-sprites, 16 colours, scrolling background (`SGPD_7S`) |
| F, G | pseudo-sprites, 256 colours, static and scrolling (`SGP256S`, `SGP256T`) |
| H | pseudo-sprites, 65536 colours (`SGP655S`) |
| I, J, K | wireframe, 16, 256 and 65536 colours (`SGPWIRE`) |

ESC in a demo ends it; its summary is printed and a key returns to the menu.

## Disk layout and ROM interfaces

- 2HD (default): 77 cylinders x 2 heads x 8 sectors x 1024 bytes (MFM,
  `N = 3`), disk mode `23h`. 2DD: 80 cylinders x 2 heads x 9 sectors x
  512 bytes (`N = 2`), disk mode `12h`. The builder passes the geometry to
  both assembly sources as `SECTORS`, `SECTOR_N` and `DISK_MODE`.
- Logical sector `n` is track `n / SECTORS` (cylinder x 2 + head), sector
  `n mod SECTORS + 1`.
- Sector 0: IPL. Then 8 KiB reserved for the loader (8 sectors on 2HD, 16
  on 2DD). After it: the demo images, each starting on a sector boundary, in
  menu order; the loader's catalog (`catalog.inc`, generated) holds each
  first sector and length.
- `INT 80h` (floppy disk BIOS): `0Ah` disk mode, `01h` read (drive 1, MFM)
  with up to four tries and `06h` recalibrate in between.
- `INT 83h` (text BIOS): `2Ah` initialise, `17h` clear, `08h` locate, `00h`
  character output.
- `INT 82h` (keyboard BIOS): `0Ch` clear the queue, `09h` read a key code.
  Function `00h` waits on the Japanese front end, which only an operating
  system installs, so the loader does not use it.
- `INT 21h` and `INT 20h` are provided by the loader for the demos, which
  are DOS `.COM` programs: `09h` prints a `$`-terminated string through the
  text BIOS, `4Ch` and `INT 20h` return to the menu, and any other function
  returns `CF = 1`, `AX = 1`. The demos call only `09h` and `4Ch`; for
  everything else they use the ROM BIOS and the hardware directly.

## Status

Checked in vaeg, 2HD and 2DD, with the VA2 ROM (all eleven demos) and the VA
ROM (spot checks): the disk boots to the menu, the demos run, ESC ends each
one, and the menu returns. In vaeg the M7c and M7d variants show wrong
colours, the 65536-colour pseudo-sprite demo shows one ball, and on the VA
model the FPS counter glyphs are solid blocks; all look the same under
PC-Engine, so they are properties of those demos in vaeg, not of this disk.
Real PC-88VA hardware has not been tried yet.
