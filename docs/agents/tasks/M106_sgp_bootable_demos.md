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
  lamp bar remains (`FrontPanelMode`). With the maintainer's approval the
  branch history was rewritten to drop the image files from every commit
  and force-pushed (old head `e3763f13`, new head `f317986c`); the commit
  messages and the removed NOTICE text remain. Commits of that range no
  longer build on their own, as their CMake files name the missing images.
- Hosted CI had failed the ASan and Windows jobs on every push since
  M103g, unnoticed. Causes, both in tests: the M103i/M103j selftests for the
  monitor switch, the cassette and port 40h sound used the I/O tables after
  the previous test's `pccore_term()` (ASan heap-use-after-free; a segfault
  on Windows), and the EOL checker's own test wrote `.gitattributes` with
  `write_text`, which writes CRLF on Windows. Each selftest now starts and
  ends its own machine, and the attributes are written as bytes. Locally the
  ASan build (with CI's `detect_leaks=0`) passes all 108 tests and reports
  the use-after-free again with the selftest fix reverted.
  [ded95a4b](https://github.com/nakatamaho/vaeg/commit/ded95a4b3b38f0b9e5b140f163e6badf8c87dc3d),
  [6e30eb67](https://github.com/nakatamaho/vaeg/commit/6e30eb676b7d27c875094e513c17c1e1dd3d1eab).
- With the native CRT presenter, enlarging the window did not enlarge the
  picture (maintainer report): the lamp bar grows with the window width, and
  each change of its height re-applied the configured window size, pulling
  the window back. The window is now refitted only when the panel setting,
  display mode or presenter changes.
  [af505697](https://github.com/nakatamaho/vaeg/commit/af5056978f21f7826146028ec17b8b101726009a).
- Turning the lamp bar on reset the window to the configured scale
  (maintainer report). Showing or hiding it now adds or removes its rows from
  the current window; the saved window size (`GUI_win_cx`/`GUI_win_cy`)
  excludes them, the custom window size likewise, and a scale change
  includes them. The bar stays on by default (`FrontPanelMode=1`).
  [e99c8c64](https://github.com/nakatamaho/vaeg/commit/e99c8c6493397f0cb78368ce19747be05e8d7dc5).
- MS-DOS bootable demo disks (maintainer request): `pcengine_disk.py
  vanilla` and `install` also accept the PC-88VA MS-DOS 2.0 and 4.0 system
  disks of the FreeDOS-88VA project (MIT-licensed releases `msdos2-va.1` and
  `msdos4-va.1`), recognised by their boot parameter block and IO.SYS at
  cluster 2. A vanilla MS-DOS copy keeps the volume label, IO.SYS, MSDOS.SYS,
  COMMAND.COM, CONFIG.SYS, AUTOEXEC.BAT, LICENSE.TXT and README.TXT in their
  root order. PC-Engine output is unchanged (byte-identical vanilla disks
  from 1.05 and 1.1). Every demo disk builder (all demos, the SGP disk via
  `--system-source`, and the per-demo `build-bootable-d88.sh`) therefore
  takes either system. In vaeg all 17 programs of the all-demos disk run
  from MS-DOS 4.0 on the VA and the VA2 (the zundamon demo shows its picture
  within 6000 frames of being started, as under PC-Engine, while it loads
  its atlas), with the same known colour and
  65536-colour faults as under PC-Engine; MS-DOS 2.0 was spot-checked.
  Synthetic-disk tests: `tests/pc88va/test_pcengine_disk_msdos.py`.
  [f86b201b](https://github.com/nakatamaho/vaeg/commit/f86b201bc8da888a584a6d9d0ca056c3e5e3c9e3),
  [089f4579](https://github.com/nakatamaho/vaeg/commit/089f4579d1fede61b14f3727e2b09754a2ac989e).
- Committed bootable MS-DOS 4.0 disk (maintainer-approved exception to the
  bootable-disk rule, recorded in `AGENTS.md`): `demos/disks/all-demos-msdos4.d88.xz`
  (144780 bytes, SHA-256
  `db80607c4ca720399aee9fc473cf90897fbad3413b9b7aab0f930d4b7cc3d75e`; raw D88
  SHA-256 `fecd48b65b2c0feea4e590657318c4cb8bed2909581c4058087c095ee3af8e53`).
  Built by `tools/pc88va/build-all-demos-msdos4-disk.py` from the pinned
  `msdos4-va.1` image with fixed directory time stamps; two builds give the
  same bytes and the `.xz` round-trips. Its validator checks the release's
  boot sector and kept files by hash and every demo file against the
  distributions, with one error code per mutation
  (`tests/pc88va/test_all_demos_msdos4_disk.py`). In vaeg the committed image
  runs all 17 programs on the VA and the VA2.
  [100825e8](https://github.com/nakatamaho/vaeg/commit/100825e83fa3e8b8dbf1ab3982c5a2b24acb056e),
  [5dcff541](https://github.com/nakatamaho/vaeg/commit/5dcff541b18b97c0d450198e27a1fef0686e9ca1),
  [1de95914](https://github.com/nakatamaho/vaeg/commit/1de95914bd3ad44ea972fd5e6be4fdc9cdcf992d).
- SGP LINE directions (maintainer photo, PC-88VA2, PC-Engine 1.1, GLASS):
  the hardware draws `0800h` right to left and `0400h` bottom to top, the
  reverse of the Technical Manual's table that M97b had adopted. vaeg is
  fixed and the demos are re-encoded: GLASS, NEON4, the three wireframes,
  the 65536-colour pseudo-sprite and sgp-scan; each changed COM differs
  from the old one only in the two direction immediates. The GLASS, NEON4,
  pseudo-sprite, wireframe, all-demos and all-demos-msdos4 disks are
  rebuilt (all-demos-msdos4.d88.xz now SHA-256
  `ff06ce7d03ee5401b30a5a691fe1399347bb9de4cc16db17e730c88f3678d87d`, raw
  `685fea156e753c511568cfc6a2b832d242bb885beb89bfd364484a425259e540`). On
  a PC-Engine 1.1 all-demos disk in vaeg (VA2) every program draws as
  intended except NEON3, whose payload could not be rebuilt here (ledger
  open defect). Photo: `docs/modernization/m106-photos/va2-pcengine-glass-line.jpg`.
  On the original VA model the ROM's PC-Engine 1.0 starts instead and
  GLASS, NEON4 and zundamon do not run, with the old disks as well; not
  investigated. The MS-DOS 4.0 stall of GLASS and SGPD_7C is deferred.
  [cc915511](https://github.com/nakatamaho/vaeg/commit/cc9155117e433ce333a2451ffcf1325202209aba), [ef036c12](https://github.com/nakatamaho/vaeg/commit/ef036c12154d3ae4919da2c744dce600f286d01f), [a958fb1e](https://github.com/nakatamaho/vaeg/commit/a958fb1eae08c82e2fbc48de267fde7a16587f30).
- MS-DOS 4.0 preview 2 (maintainer request): the exception and the builder
  are pinned to release `msdos4-va.2` (image SHA-256
  `7c4b141d31e0034120e0b07eb93b9bea808e1c54822e48e15189cc7398385db0`; of
  the kept files only the boot sector and IO.SYS differ from preview 1).
  `demos/disks/all-demos-msdos4.d88.xz` is rebuilt (144620 bytes, SHA-256
  `0b0f14433a5f2ec84d89ba7c8da405d5eba902bf874d4c9f9fc43185cb343b02`; raw
  `29de3a2ae2e17e4125189a1bf8111aff1903f8b50ce7535249ada0e9eb5a8657`); two
  builds give the same bytes and the preview 1 image is refused. In vaeg
  (VA and VA2) all 17 programs start and draw as under preview 1; GLASS and
  SGPD_7C still stop after their second frame under MS-DOS (deferred).
  [91f9362b](https://github.com/nakatamaho/vaeg/commit/91f9362be7582cb09607c534ea09810c9bd13500), [639c963d](https://github.com/nakatamaho/vaeg/commit/639c963d6c36b8a8d444888f3b8e113576561c11).
- NEON3 rebuilt (maintainer: plan B; the private `neon3_1_5/98/` copy is
  lost). The six NEON RELAY 3 ver1.5 files the port includes are tracked
  unmodified in `external/neon3-1.5/` (SimK; modified BSD licence by the
  author's statement on the np21w download page; ADR-0017). The public
  ver1.5 matches every source line the port documents cite but differs from
  the lost copy in parts of the city geometry and is 128 bytes larger, so
  the port's unused diagnostic reserve shrank by 128 bytes; NEON200/NEON400
  are 57405 bytes as before. `demos/neon3/build-d88.sh` now builds the
  distribution disk. The rebuilt payloads also carry the M97 text repaint
  and loop changes made after the old distribution. In vaeg (VA2, PC-Engine 1.1 and MS-DOS
  4.0) both profiles draw the city with correct lines. The NEON3 open defect
  is closed. all-demos-msdos4.d88.xz SHA-256 is now
  `1baf360b6c012d7655edb47fe41a3a96bbd60c435258c84bb9d3aa4c0840f8d6`.
  [ed3e052c](https://github.com/nakatamaho/vaeg/commit/ed3e052c96e6fc92b30fffd7c5aca6bf2b6b5af3), [42d45dd7](https://github.com/nakatamaho/vaeg/commit/42d45dd771827a67cdc754916a1bccf88245233a), [a55238ff](https://github.com/nakatamaho/vaeg/commit/a55238ff454e63bb6130037913e37df26b16f7b5), [2e8ed22b](https://github.com/nakatamaho/vaeg/commit/2e8ed22b0b5980f81bac4ec01c1fb08d75425335).
- The source-archive check (`tests/z80_compat/check_zex_archive.py`, run by
  the hosted conformance job) rejected the new `external/neon3-1.5/` root;
  it is now recorded there with ADR-0017, with a test.
  [91a46f4f](https://github.com/nakatamaho/vaeg/commit/91a46f4fbc5de0016dd191fd33c81eed68fb6399).
- NEON3 `TOTAL FRAMES (HEX)` shows its value again (maintainer request): the
  length of one pass of the looping timeline, 1800h; M97 had dropped the
  value when it added the loop. The payloads stay 57405 bytes. The
  maintainer confirmed `Copyright (c) SimK` in `external/neon3-1.5/LICENSE.txt`.
  all-demos-msdos4.d88.xz SHA-256 is now
  `79676befbbbaf4a1c04424c06f5c10128dba64fd447655d26ca3d7525b768d85`.
  [417e3909](https://github.com/nakatamaho/vaeg/commit/417e39093f058a3feb2f3a9e85aed91b4bdd42fb), [eac394e3](https://github.com/nakatamaho/vaeg/commit/eac394e32474d241c4c6e0cbeea4a9f52ecacdaa).
