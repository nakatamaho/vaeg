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


# M106 - Self-booting SGP demo disk

Status: **in progress**

Series: V3 series (maintainer request, 2026-10-08). Branch
`topic/m106-sgp-bootable-demos` off `main` at
`6c9fb34b`. Commit prefix `M106:`.

## Goal

The SGP demos (pseudo-sprites and wireframe, 16/256/65536 colours) are
PC-Engine `.COM` programs that have not run on the maintainer's PC-88VA2.
Following the boot method of other self-booting PC-88VA software (the ROM
loads one sector to `3000:0000`; that sector loads the rest with the ROM
floppy disk BIOS), build a disk that boots the demos with no operating
system, entirely from source in the repository, so that real-hardware runs
exclude PC-Engine as a cause.

## Deliverables

1. `demos/sgp-boot/ipl.asm` (boot sector), `loader.asm` (menu, loader,
   minimal `INT 21h`/`INT 20h`), `build-boot-d88.py` (reproducible D88
   builder that writes outside the repository), `README.md`.
2. No change to the demo sources; the bootable disk is never committed.
3. Front panel below the screen (maintainer request, 2026-10-08): a slim
   bar of lamps, FDD access lamps lit while a drive is accessed (red 2D/2DD,
   green 2HD) and the V1/V2/V3 mode lamps from port 1CDh.

## Gate G106 (human)

Standard V3 gate unchanged, plus: the built disk boots to the menu in vaeg
and on the PC-88VA2, and the demos run from it (results on hardware are
recorded whatever they are). The lamp bar appears below the screen, the access
lamp lights while a disk is read (red for 2D/2DD, green for 2HD) and the
mode lamp follows V1/V2/V3.

## Implementation progress

- IPL, loader and builder: [76b60381](https://github.com/nakatamaho/vaeg/commit/76b6038119f6b3639f8750fddcfd5a0fcfbff887).
  The builder assembles all eleven demo images with their own scripts and
  writes a reproducible 2DD D88 (two runs give identical images).
- Checked in vaeg (VA and VA2 ROMs): boot to the menu, each of the eleven
  demos runs, ESC ends it, its summary prints through the loader's
  `INT 21h` function `09h`, and a key returns to the menu.
- Found while checking: the keyboard BIOS function `00h` waits on the
  Japanese front end, which only an operating system installs, so without
  one it never returns; the loader uses `09h` (the demos already do).
- In vaeg, M7c/M7d show wrong colours and the 65536-colour pseudo-sprite
  demo shows one ball, identically under PC-Engine: not caused by this disk;
  not investigated here.
- 2HD by default (maintainer request): 77 x 2 x 8 x 1024 bytes, disk mode
  23h; `--format 2dd` keeps the first layout byte for byte. All eleven demos
  checked from the 2HD disk with the VA2 ROM, and spot checks with the VA
  ROM; on the VA model the M7a FPS counter glyphs are solid blocks, the same
  under PC-Engine.
- PC-Engine bootable disk with the same eleven demos (maintainer request):
  `--pcengine-source` installs them in `16\`, `256\` and `65536\` on a copy
  of the user's system disk; checked in vaeg (VA2): PC-Engine boots and
  `16\SGPD_7A`, `256\SGP256T` and `65536\SGPWIRE` run.
- Front panel: artwork, decoder and panel
  [d6e6a15c](https://github.com/nakatamaho/vaeg/commit/d6e6a15c7bc161477d0ae27c48acc27fc0dc89fd),
  [669fdb05](https://github.com/nakatamaho/vaeg/commit/669fdb058c289923eec8bacc60f6a2f41d6f649a),
  [cb9d61a2](https://github.com/nakatamaho/vaeg/commit/cb9d61a2351fd72bf343eef7b9958423d6e537b8).
  PNG is not decodable by SDL2, so a small inflate/PNG decoder
  (`sdl2/pngdecode.c`) reads the embedded images. `[ROM]`-observed mode
  lamps: port 1CDh bit 4 = V1, 5 = V2, 6 = V3 (a V3 boot lights V3, a V2
  boot V2; while booting the ROM writes 111b, which the existing decode
  shows as all three lit). Romless test: all three images decode at their
  sizes with every lamp on a dark lens, panel heights (202/253 rows at
  640), a guest picture unchanged by the reserved rows, and a software-
  rendered panel with drive 1 red (2DD) / green (2HD), V2 lit and V1 dark;
  it fails with the 2HD colour forced off. Window drawing is checked by the
  maintainer (no display in the build environment).
- The full drawing was too busy for everyday use (maintainer, 2026-10-08):
  the default is now a simple bar of lamps (FD1, FD2, V1, V2, V3; 14 rows per
  640 dots), with the drawing kept as a menu choice. The romless test also
  renders the bar on a software renderer (drive 1 lit, V1 dark; fails with
  the mode lamps forced on).
  [dd7995b4](https://github.com/nakatamaho/vaeg/commit/dd7995b475c1868d67322a0d799186a235a2fbd3).
- The drawing of the machine's front, added first and later drawn under the
  native CRT presenter too, was withdrawn by the maintainer (2026-10-08):
  its rights (trademarks and the appearance of the product) are unclear. The
  three images, the PNG decoder and the drawing mode are removed; only the
  lamp bar remains (`FrontPanelMode`). The images stay in this branch's
  history until the maintainer decides how to handle it.
