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
- In vaeg, M7c/M7d show wrong colours, identically under PC-Engine: not
  caused by this disk; not investigated here. (The 65536-colour
  pseudo-sprite demo showing one ball is by design: it starts with one
  sphere and UP/DOWN changes the count from 1 to 128; checked in vaeg on
  2026-10-13 after the maintainer pointed it out. Not a defect.)
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
- Demo build check (HEAD `ec74cdf5`, clean worktree): GLASS, NEON3 (both
  profiles), NEON4 (both), pseudo-sprite 16/256/65536 and the three
  wireframes rebuild byte-identical to the committed distribution disks;
  sgp-scan and the self-booting SGP disk build. ZUNDAORB cannot be rebuilt:
  its PSD and private profile tables are not in the repository (tables
  regenerated with the repository tools differ in 10659 bytes); pending a
  maintainer decision.
- `MANDEL.BAS` (`demos/mandelbrot/mandelbrot.bas`, CRLF) is on the
  PC-Engine all-demos disks (distribution and bootable builders) but not on
  the MS-DOS 4.0 disk, which has no BASIC (maintainer). In vaeg (VA2,
  PC-Engine 1.1) `LOAD "MANDEL.BAS"` and `RUN` from the ROM BASIC draw the
  set. On the rebuilt PC-Engine 1.1 all-demos disk all 17 programs start
  and keep drawing in vaeg (VA2); on the MS-DOS 4.0 disk likewise except
  GLASS and SGPD_7C, which stop after their second frame (deferred).
- AGENTS.md now requires every milestone to keep the demos buildable and
  run (maintainer request), with zundamon-orbit as a recorded exception.
  [4b32c85c](https://github.com/nakatamaho/vaeg/commit/4b32c85c984d0fdb9b357c56eabc9453003b69f5), [0af3fcf2](https://github.com/nakatamaho/vaeg/commit/0af3fcf2cfeda2978b941b395c551470892dbc94), [483ec18c](https://github.com/nakatamaho/vaeg/commit/483ec18c85275ed688e8f7df8997d2802cd16fb1).
- Utility media (maintainer request): VZ Editor 1.60 on the utility FDD and
  both SASI images; CP/M and all other SASI content kept. FDD drops
  2HCDRV.ZIP, FDFRMSRC.LZH, ISHARC.COM, SCHD.LOG and SCFORM.LOG (approved
  one by one). Found and fixed on the way: the documented `lhasa` broke the
  FDD build (lower-case names); `pcengine_disk.py` used cylinders 77-79,
  which PC-Engine never reads (a fuller FDD lost its CONFIG.SYS drivers);
  VZ stops on EMS when the EMS drivers are loaded (VZVA.DEF gets EM0); the
  SASI wrapper had passed six packages the stager never accepted since
  M97z4. The SASI images also gain the maintainer's Softlib list (PCP_7A,
  PCP_CA, FD98_232+FD_VA, VASG100, NYANCO25, MARINVA, FATMAP11). In vaeg:
  VZ runs from the FDD (VA, VA2) and VZ, FD and MARIN from the SASI image
  as C: (VA2). A plain PC-Engine 1.1 disk with VZ (not committed) was
  confirmed by the maintainer. The SASI images do not boot by themselves in
  vaeg with `--no-bkupmem` (not investigated; earlier records only claim a
  mount check). Commits: [38a2d0b4](https://github.com/nakatamaho/vaeg/commit/38a2d0b4f6db780d9693d91f051b3d5d92461816), [61309af7](https://github.com/nakatamaho/vaeg/commit/61309af765c16a61b4db7016d2fe76c2b0acfa6a), [0c307b84](https://github.com/nakatamaho/vaeg/commit/0c307b84872c6fa34d8ecc557b49dfd64bd3fbb2), [166f2ab2](https://github.com/nakatamaho/vaeg/commit/166f2ab2caf5320c00b9cbcfd45b43f77662f4a8), [5ad825a1](https://github.com/nakatamaho/vaeg/commit/5ad825a16076aa551e3ae8491698effc8c62b850), [77b01b2b](https://github.com/nakatamaho/vaeg/commit/77b01b2b3a35a73b28f3cc28c4b22a37f03a5176), [821e431e](https://github.com/nakatamaho/vaeg/commit/821e431e9bd87f0b51c76e8c6d1c46e33e45af91), [6830c7fa](https://github.com/nakatamaho/vaeg/commit/6830c7fa1151410369685db39319ee8b101a7d6c), [11960c68](https://github.com/nakatamaho/vaeg/commit/11960c68942b3c75626687c383b12ce2054880eb), [d78bea54](https://github.com/nakatamaho/vaeg/commit/d78bea546c95368d997f023aeb242fe8482c81c4).
- Hardware: GLASS from the rebuilt PC-Engine 1.1 all-demos disk (LINE
  directions re-encoded) runs correctly on the PC-88VA2 (maintainer,
  2026-10-09), and so do the 16-, 256- and 65536-colour SGPWIRE. The other
  programs on that disk are not yet reported.
- Hardware (PC-88VA2, PC-Engine 1.1, all-demos disk, maintainer
  2026-10-09): GLASS, NEON3, NEON4 16/65536, SGPWIRE 16/256/65536 and
  SGP655S run; SGP655S shows no FPS or count; SGP256S and SGP256T hang;
  ZUNDAORB returns to Ready and hangs (ledger open defect). Two demo faults
  seen on the hardware are fixed and reproduced in vaeg first: NEON3 did
  not leave on ESC (the looping build never polled it), and GLASS left the
  cursor magenta (its palette stayed set); GLASS, NEON3 and NEON4 now turn
  graphics off, re-initialise the palette ($PalInit) and select text-only
  composition on exit. In vaeg (VA2) all 17 programs of the rebuilt
  PC-Engine disk leave on ESC with the grey cursor and no graphics left.
  [8e3b97c5](https://github.com/nakatamaho/vaeg/commit/8e3b97c5d3cfb5d9df31ec88cd41f3e79f298b07), [cf48ab7f](https://github.com/nakatamaho/vaeg/commit/cf48ab7fb97e514e0d8281569d831cf4779317e8), [271cdec9](https://github.com/nakatamaho/vaeg/commit/271cdec953895678e05364c45eafca0a24b0d27c).
- ESC polling: the first NEON3 ESC fix stopped NEON3 under MS-DOS 4.0 (vaeg,
  compared with the previous disk), because INT 82h AH=00h waits on the
  Japanese front end, which MS-DOS does not install; GLASS stalled under
  MS-DOS for the same reason. Both now poll with AH=0Ah/09h like NEON4 and
  the SGP demos. In vaeg all 17 programs leave on ESC with a grey cursor and
  no graphics left, from PC-Engine 1.1 and MS-DOS 4.0 (VA2 and VA), except
  the known ones: SGPD_7C under MS-DOS, and GLASS/NEON3/NEON4 on the VA's
  PC-Engine 1.0. [97a5808a](https://github.com/nakatamaho/vaeg/commit/97a5808afa03346cc00b633396ade5f49055d4a4), [7aa5d32f](https://github.com/nakatamaho/vaeg/commit/7aa5d32f2583c56427d1cde2c9aeb13fe48f9b81).

- Tape (maintainer request, decision C11 revised): the 87 T88 images of
  the maintainer's PC-8801 tape collection record 1200 Hz for typically
  1.5 s and 2400 Hz for 0.5 s before the first data, and timed blank,
  space and mark blocks between data blocks (one image has about 5 s
  between its BASIC and machine-code parts). vaeg had dropped those blocks
  and used one second each for its leader. T88 blocks now take their
  recorded time (no data, carrier only during 2400 Hz), a raw image gets a
  1.5 s + 0.5 s leader, and fast load skips both. Selftest covers both
  (four mutations caught). Separately, one tape whose BASIC loader
  auto-types `mon`/`r` through the BASIC work area (F003h/F004h, E6CEh)
  did not start its machine-code part in vaeg; fixed below.
  [a06d55d8](https://github.com/nakatamaho/vaeg/commit/a06d55d8dbab00de9e0d2c4b52089f3b92f2f25f).
- Tape auto-load (maintainer: `LOAD "CAS:"` and `RUN` should load and
  start the game): the auto-type through F003h/F004h/E6CEh works in vaeg
  (checked with a one-line program; the VA ROM's keyboard interrupt at
  30ACh/34EAh implements it). The failure was BASIC reading one byte past
  its program: on tape that byte is a 00h from the following 1200 Hz gap,
  in vaeg it was the next block's 3Ah. T88 space blocks now deliver 00h,
  and fast load shortens blocks instead of dropping them; the game loads
  and starts with fast load off (about 3.5 minutes of loading) and on.
  Bug-fix ledger entry. [07d35f7c](https://github.com/nakatamaho/vaeg/commit/07d35f7ccc8e60b937e304c8290c05b3d9975166).
- Headless paste stall (found while checking a PC-8001 tape in N mode;
  maintainer asked for a fix): a `--headless-input-script` text line
  (and `@enter`) could stall for ever when a screenshot had been rendered
  just before it. The GUI only draws a frame at screenshot frames under the
  dummy video driver; such a frame could leave ImGui keyboard navigation on
  the main menu bar, so `WantCaptureKeyboard` stayed set and
  `gui_guest_keyboard_blocked()` paused the paste. Not specific to N mode
  (first reported here as an N-mode problem, which was wrong). Pastes
  started from scripts (`kbdpaste_start_text`, headless input and debug
  harness) no longer wait on the GUI; the GUI clipboard paste is unchanged.
  Selftest added (fails without the change); the stalling script now runs
  three times out of three. Tool-only; no guest-visible change.
  [ad2cacc8](https://github.com/nakatamaho/vaeg/commit/ad2cacc81e154651dbe7cfda9d45552afc50c206).
- PC-8001 tape check (maintainer's question): one T88 is a monitor-format
  machine-code tape (9000h-E8CCh, no BASIC header) for N-BASIC; in vaeg N
  mode (VA2, user N80 ROM) `mon`, `L`, `G9000` load and start it.
- Tenkey overlay (maintainer request): Device > Keyboard > "Tenkey overlay"
  now maps the cursor keys up/left/right/down to guest keypad 8/4/6/2
  instead of Y U I / H J K / N M , / . (keypad 7-9/4-6/1-3/0); the letter
  keys type letters again. ADR-0008 amended; selftest updated (fails with a
  letter key back in the table). [a55c447a](https://github.com/nakatamaho/vaeg/commit/a55c447ab833b83fc99018507ebaa0bdb5ab7769).
- File browsers and tape archives (maintainer request): the FDD and tape
  browsers list `.d88 .88d .zip .7z .lzh` and `.cmt .t88 .zip .7z .lzh` by
  default, with an All files (`*.*`) choice below the path. The tape menu
  and `--tape` accept a zip/7z/lzh archive and load its first `.cmt`/`.t88`
  (by name), extracted into the managed archive storage. Dropmedia selftest
  covers the tape extraction (fails when the tape filter takes disk images
  or the extractor ignores it; run in a build with LibArchive, since this
  host has no LibArchive development package and the default local presets
  skip the archive tests). Checked with a PC-8801 tape zip through
  `--tape`. The GUI browsers were not exercised (no display here).
  [2b42fa71](https://github.com/nakatamaho/vaeg/commit/2b42fa7140e649510381ee6e4df3a63e6b68a0c9), [d6bc7370](https://github.com/nakatamaho/vaeg/commit/d6bc7370a88bce6498853114161d67260c5d3aae), [c915b1ec](https://github.com/nakatamaho/vaeg/commit/c915b1ec91ad4ad6e333e59c0ecaab9186467d48).
- Tape checks (maintainer's PC-8801 tapes): one N88 tape (auto-load through
  the monitor `R` and `G9F00`) loads and runs with `LOAD "CAS:"` and `RUN`,
  fast load on and off. Another N88 tape's BASIC auto-types monitor command
  `O0,0` to read the machine code; the VA's N88 monitor (identical to the
  PC-8801MA2 N88.ROM in 32669 of 32768 bytes) does not read tape on `O`
  (it uses `R`), so `RUN` stops with `?`; typing `mon`, `R`, `GE2A0`, `R`,
  `GA000` by hand runs it. Not an emulation fault as far as determined.
  Rechecked in V1 (maintainer: these are V1 programs; on a real PC-8801
  they loaded, provided the tape image is faithful). Two such tapes, V1 S
  and V1 H on the VA: N-88 BASIC 1.9 loads the BASIC part; the auto-typed
  `o0,0` returns at once without reading tape, as in V2. `[ROM]` `MON`
  jumps through E826h, which switches port 31h to the N-BASIC ROM and
  enters its monitor at 6002h; the monitor's command table (6000h-7FFFh)
  and its `O` handler (port output: `OUT (C),L`) are identical in the
  maintainer's 1981 PC-8801 ROM and in the VA and VA2 ROMs, and the RAM
  pre-dispatch hook E66Fh is a bare `RET` in V1 and V2. `R` is the tape
  read. So with these ROMs `o0,0` writes 00h to port 00h and cannot load
  the machine-code blocks that follow the BASIC part on both tapes; the
  tape images (or the monitor they were written for) differ from what
  this ROM accepts. No vaeg defect found.
- HDD boot (maintainer: on hardware the setup menu's 起動ディスク must be
  固定ディスク; in vaeg it can be always on, like Main_RAM): flipping each bit
  of the memory-switch record B1FC0h-B1FC7h in turn (vaeg, VA2 ROM, SASI
  only) showed that only B1FC4h bit 7 boots the SASI disk (bits B1FC2h.2 and
  B1FC4h.4 only change the text layout). With it set, a bootable floppy in
  drive 1 still boots first, and with no fixed disk the floppy boots.
  `Boot_FixedDisk` (default on, menu デバイス > ハードディスクから起動) sets it
  and the checksum at startup/reset on a record with the BIOS signature.
  The utility SASI images now boot by themselves on the VA (PC-Engine 1.05)
  and the VA2 (1.1). Selftest added (three mutations caught). Not a defect
  fix; no ledger entry. [11e9fc33](https://github.com/nakatamaho/vaeg/commit/11e9fc330aa4c81e60217a663f045261aea51cf6).
  Correction, found the same day while starting V2 BASIC for a tape check:
  with the bit set and no media at all, the original VA stops at the
  "insert the correct disk" message instead of starting N88-BASIC, so
  "always on" was wrong. The bit now follows the media: set when a SASI or
  SCSI hard disk is attached, cleared otherwise. Checked on the VA and VA2
  with no media, the SASI image alone, and the SGP floppy plus the SASI
  image (the VA starts N88-BASIC with no media again; the VA2 shows the
  disk message with no media, as it did before and as observed on
  hardware). Selftest covers set and clear (three mutations caught).
  Wording corrected later: the VA with the bit set and no media hangs at a
  bare cursor (no message), and the maintainer notes that state cannot
  arise on hardware, where the setup cannot select a fixed disk without
  one; clearing the bit keeps vaeg in a reachable state. Maintainer
  decision: by default vaeg keeps the backup memory in accord with the
  configured machine (RAM size and hard-disk presence), and fixed-disk boot
  without a hard disk is never allowed: the bit is now cleared without a
  hard disk even with `Boot_FixedDisk=false`, which only stops vaeg from
  setting it ([18c1b2cc](https://github.com/nakatamaho/vaeg/commit/18c1b2cc46cffb12c1352a0dbcf074b7d8cc334f); checked on the VA with the bit preset,
  setting on and off, with and without the SASI image; selftest catches
  three mutations).
  [c4db4df9](https://github.com/nakatamaho/vaeg/commit/c4db4df93c7698e7589300a7b0414c87e3081f24).
- Hardware hangs, first step (stage-marker diagnostics were planned): a
  DIAG build of SGP256S that printed stage letters itself froze in vaeg,
  which led to the cause. The pseudo-sprite, wireframe and sgp-scan
  sources declared no CPU, so NASM emitted the 386 near conditional jump
  `0F 8x` for long `Jcc`s: four each in SGPD_7C/7D, one each in
  SGP256S/T (right after the first `INT 8Fh`), one in the sgp-scan bar
  program. The uPD9002 runs the 80186 instruction set; vaeg decodes
  `0F 85` as a one-byte reserved operation and runs out of step. Fixed
  with `cpu 186` in the nine sources ([eb953958](https://github.com/nakatamaho/vaeg/commit/eb953958c374fb2e1d47d46acb9896d1e9e3ba9e)); only those five
  programs change. The pseudo-sprite, all-demos and all-demos-msdos4
  disks are rebuilt and differ only in SGPD_7C, SGPD_7D, SGP256S and
  SGP256T ([0a1076bb](https://github.com/nakatamaho/vaeg/commit/0a1076bb1308cc850b4910562f87e8501eb25e1f)). In vaeg (VA2) the four draw from both
  PC-Engine 1.1 and MS-DOS 4.0; this also fixes the M7c/M7d wrong colours
  noted above (the old binaries show them, the new ones do not). Ledger
  entry added; whether this was the hardware hang is to be confirmed on
  the PC-88VA2. SGP655S showing no FPS or count is by design (the program
  has no overlay); ZUNDAORB has no `0F 8x` and stays open. Hardware disks
  rebuilt in `~/work/pc88va/m106-hw/`: `sgp-boot.d88` (b2be40bf),
  `pce-all.d88` (ce586557), `msdos4-all.d88` (e110cbc7).
- 386 near jump in vaeg (maintainer request): the uPD9002 now halts on
  `0F 80h`-`8Fh` instead of treating `0Fh` as a one-byte reserved
  operation (which had run the following bytes out of step: in the old
  SGPD_7C, `85 A3 00 B8` became a TEST that swallowed the start of
  `mov ax,0B00h`, so `INT 8Fh` was called with the wrong function and a
  stray byte was written). It stops like `HLT` but never resumes, and the
  first stop at each address is reported on stderr. [POLICY]: the real
  CPU's behaviour is unknown beyond the hardware hang. In vaeg the old
  SGPD_7C and SGP256S now stop at once; the rebuilt ones run. Test
  `vaeg_upd9002_m106_near_jcc` added (three mutations caught); 113 tests
  pass. [b5be04ff](https://github.com/nakatamaho/vaeg/commit/b5be04fff33316c035bb03e025f887612cf9b64e).
- Demo run check before the gate (AGENTS.md), HEAD `35dffb5f`, vaeg VA2:
  all 17 programs of the rebuilt PC-Engine 1.1 bootable all-demos disk and
  of the MS-DOS 4.0 all-demos disk start, draw (GLASS, NEON3 200/400,
  NEON4 16/65536, SGPD_7A/7B/7C/7D/7S, SGP256S/T, SGP655S, SGPWIRE
  16/256/65536, ZUNDAORB) and return to `Ready` or the DOS prompt on ESC;
  no `0F 8x` halt. `MANDEL.BAS` loads in N88-日本語 BASIC v3.1 (`basic`
  from PC-Engine, `LOAD "MANDEL.BAS"`, `RUN`) and draws the set from the
  top (followed for about 30000 frames with `--nowait`; the interpreted
  program takes long to finish). The SGPD_7C stall under MS-DOS, deferred
  earlier, no longer occurs (it was the `0F 8x` in the old binary).
  VA3DDEMO (JWasm, `.186`) contains no `0F 8x`; why it draws nothing in
  vaeg stays open. Ledger: GLASS/NEON3/NEON4 on the VA model and the VA
  model's SGPD_7A glyph blocks recorded as open defects, and the BEEP
  pitch report added to the port 40h entry ([35dffb5f](https://github.com/nakatamaho/vaeg/commit/35dffb5fbfd6a4b1aa1cc3cbb424a1dab942d29e)).
- Hardware results (maintainer, PC-88VA2, rebuilt `pce-all.d88`,
  2026-10-14): SGP256S and SGP256T now run, so the `0F 8x` near jump was
  the hang; the three SGPWIRE profiles run. ZUNDAORB returns to `Ready`
  with nothing shown: not a defect. It requires a 16 MiB I/O bank memory
  board (port `1D0h`, 128 banks of 128 KiB), which vaeg enables by default
  and the maintainer's VA2 lacks; with `Use_BMS_=false` vaeg does the same
  (PC-Engine: straight back to `Ready`; MS-DOS 4.0: `M98V_FAIL:
  predecessor BMS mapping probe failed.`). PC-Engine 1.1 shows nothing for
  `INT 21h` function `09h` (checked with a small COM; the ROM text BIOS
  `INT 83h` does print), which is why no message appeared. The
  requirement is now in the zundamon README; the open ledger entry is
  removed (its SGP256S/T part is fixed by the `0F 8x` entry, SGP655S was
  by design, ZUNDAORB is this requirement).
- Hardware results, continued (maintainer, PC-88VA2, 2026-10-14): the
  65536-colour pseudo-sprite, NEON4 and the wireframes run. The
  maintainer has no real SASI drive, so the HDD images are checked in
  vaeg only; the "HDD does not start" report was against an older vaeg
  build (before the fixed-disk boot setting).
- Hardware results with the MS-DOS 4.0 all-demos disk (maintainer,
  PC-88VA2, 2026-10-14): SGPWIRE 16/256/65536 and the 16-, 256- and
  65536-colour pseudo-sprites run. NEON3 and NEON4 draw, but text stays
  visible on the right while they run, and after they exit the text
  console is wrong: it scrolls only one line. In vaeg (same disk, VA2)
  NEON3 leaves its status text on screen after ESC and the prompt
  continues below it, but scrolling stays normal, so vaeg does not
  reproduce the one-line scroll. Open; not investigated yet (NEON's text
  console hides the system line with the Screen Editor `INT 94h` and
  restores it on exit, which may not suit MS-DOS 4.0).

