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

# M113 — PC-8801 machines

Status: **complete; G113 passed on 2026-10-15**

## Request

Run the PC-8801 family as machines of their own, from the ROMs found by
M112 under `extra/<model>/`, starting with the PC-8801MA. Started on the
maintainer's instruction before the M110, M111 and M112 gates; based on the
M112 branch.

## Goal

With a ROM set matching MAME's `pc8801ma`, choosing PC-8801MA under
模擬 -> 起動 starts the machine, **N88-BASIC V2 reaches its prompt**, and a
D88 disk **boots from the built-in FDD**.

## Scope

- 起動 -> PC-8801/8001 (追加 ROM): a complete PC-8801 model directory gets
  start entries (V2, V1, N where the model has them); the revisions chosen
  in M112 are used. Models with missing roles cannot be started.
- A plain PC-8801 machine built on vaeg's V1/V2-mode implementation,
  without the VA-only parts: ROM layout (`n88`, banks `n88_0`-`n88_3`,
  `n80`), memory and banking ports (31h, 32h, 71h, E2h/E3h), text and
  graphics display, keyboard, kanji ROM ports (E8h-EFh), dictionary banks
  (F0h/F1h), sound as fitted to the model, and the FDD unit behind ports
  FCh-FFh running `disk.rom` on the uPD780C, as the VA's FDD subsystem
  does.
- The PC-8801 without `disk.rom` starts with no FDD unit (its FDD is the
  external PC-80S31).
- A PC-8801-layout set derived from the PC-88VA/VA2 ROMs as V1/V2 mode
  shows them (`n88`, banks, `disk`, `kanji1`, `kanji2`, `font`, `jisyo`;
  no N-BASIC) is accepted as a model directory and can be started.
- Kanji level 2 in V1/V2 mode: `io/cgromva.c` `cgrom88_offset` inverts
  byte-offset bit 14 for both levels. For level 2 the non-inverted order
  matches a kanji2 ROM matching MAME's `pc8801ma`, glyph for glyph. Check
  which order the VA uses and correct vaeg if it is wrong (bug-fix ledger
  entry if so).

Out of scope: PC-8001 family (M114), PCG-8100/8800 (M115), the MC's
CD-ROM, PC-98DO's PC-98 side.

## References

- X88000 1.5.3 by Manuke, public domain: behavioural reference as in
  ADR-0016 (source archive `x88_1_5_3_src.tar.gz`, SHA-256
  `66bc0b69d5a394e2195b5e277093eebcc045fbb3930c03637aa106394ae83cf3`).
  Derivations are recorded in ADR-0016 "Derivations".
- QUASI88 0.7.4 by showtime
  (<https://www.eonet.ne.jp/~showtime/quasi88/>, `quasi88-0.7.4.zip`,
  SHA-256 `e61f110ae2adc3ee10b0e6718f58e60f796a272c25309fd0950e47e70c2844e1`):
  second behavioural reference. Its core is under a modified BSD licence;
  its sound output (MAME/XMAME, fmgen) is not used. Code derived from it
  keeps its copyright notice and licence in the file header.
- MAME's published ROM definitions (M112) for roles and sizes only.

## Decisions

- [POLICY] Approach A (maintainer, 2026-10-15): a PC-8801 model runs in the
  VA's V1/V2 mode with the ROM images V1/V2 mode reads replaced by the
  model's ROMs; the VA firmware still starts the machine and the VA ROM
  set stays required. Starting a PC-8801 without the VA firmware (B) or a
  separate PC-8801 hardware model (C) is left to later milestones.
- [POLICY] No test ROM, test disk or third-party pseudo BIOS is added
  (maintainer, 2026-10-15); verification is selftests on synthetic data
  and the human gate.
- [POLICY] Base model: the VA for the `PC-88VA` set, the VA2 for every
  other model (OPNA sound; its first half is OPN-compatible). Modes: V2
  from the mkIISR on, H (8 MHz) from the FH/MH on, N with `n80`.
- [POLICY] The mkIITR is added as a model directory (maintainer,
  2026-10-15). MAME has the mkIITR, FE and FE2 only as commented-out
  machines without ROM definitions; their roles follow the mkIISR and the FA.
- [POLICY] The PC-8801 without `disk.rom` has no FDD unit: ports FCh-FFh
  read FFh and the unit does not run. Absent extension banks, kanji ROMs
  and dictionary read FFh.

- [POLICY] Maintainer review (2026-10-15): a model starts with any missing
  role except `n88`; 起動 shows 自動 without its fallback; the PC-8801/8001
  submenu drops "(追加 ROM)"; a model submenu has the VA's mode labels
  (V2 H ... N) and a ROM submenu; MAME matches are not marked, mismatches
  are ("MAME と不一致"); the ROM directory path is no longer shown in the
  menu. 画面: the monitor submenu shows its frequency (モニタ: 24 kHz) and
  V1/V2 表示 is its own 画面 submenu. New FDD images are all unformatted
  D88 (2HD 1.2 MB and 2DD 640 KB added to 2DD 720 KB, 2D 320/360 KB); the
  MS-DOS formatted images are no longer offered in the GUI.
- [POLICY] The guest's FORMAT rewrites every track with its own layout
  (PC-Engine formats a 640 KB image as 720 KB; maintainer, 2026-10-15), so
  the GUI offers one unformatted image per media type: 2HD (77 x 2 x 8 x
  1024), 2DD (80 x 2 x 9 x 512) and 2D (40 x 2 x 16 x 256). The other
  layouts stay in `fdd/newdisk.c` and its selftest.

## Implementation

- `bios/romva.c`: `romva_machine_hook`, called at the end of
  `romva_initialize()`; resets the kanji override and the FDD unit.
- `sdl2/extrarom.c`: model profiles (base, V2, H), `extrarom_apply()`
  (the replacement above; falls back to the VA ROMs with a warning when
  the directory is no longer complete), `PC-88VA`/`PC-88VA2` model entries.
- `io/cgromva.c`: `cgrom88_kanji[2]`, PC-8801 kanji ROM images read in
  MAME's port order; level-2 order fix (bug-fix ledger).
- `io/subsystemmx.c`: `subsystemmx_set_absent()` (no FDD unit).
- Configuration `ExtraMachine`; `--extra-machine <model>` (session only).
- GUI: start entries per model under 起動 -> PC-8801/8001 (追加 ROM);
  choosing a VA entry clears the model; 最近の起動 records it; the window
  title and 情報 -> More show it.
- Documentation: [`pc8801-machines.md`](../../modernization/pc8801-machines.md),
  [`extra-roms.md`](../../modernization/extra-roms.md), README.

## Verification

- Selftests on synthetic data only; no ROM image, test ROM or test disk is
  added to the repository. `PC-8801 machine`: replaced images, extension
  bank placement, 2 KiB FDD ROM repetition, no FDD unit without `disk.rom`
  (ports read FFh), absent banks, kanji ROMs in port order, 8x8 characters
  from `kanji1.rom`, missing dictionary, 自動 started as V1/V2, an
  incomplete directory leaving the VA ROMs, model profiles.
  `V1/V2 ROM ports`: the level-2 kanji order. Mutation check: 16 of 16
  mutants of `sdl2/extrarom.c`, `io/cgromva.c` and `io/subsystemmx.c`
  killed.
- With a ROM set matching MAME's `pc8801ma`: V2 BASIC reaches `Ok` with
  version 2.3, a V2 disk boots through the model's own FDD ROM, and N mode
  starts N-BASIC 1.8, from the command line and from the 起動 menu.
- Regression: the VA and VA2 boot PC-Engine in V3 and reach `Ok` in V2 as
  before; ctest 92 of 92.
- G113 human gate: with a ROM set matching MAME's `pc8801ma`, V2 BASIC
  reaches its prompt and a disk boots; V1 and N where present; the VA and
  VA2 machines and their V1/V2/N modes still behave as before (regression);
  the clean-checkout MinGW build and the media build regression of
  AGENTS.md.
- Media build regression at `12857cab`, clean clone, shared cache (the
  following commit changes only selftest includes): `build-utility-disk.sh`
  (PC-Engine 1.1) exit 0, 4096 bytes free, boots in vaeg;
  `build-sasi-utility-disks.sh` exit 0, each image boots alone on its model
  with `\BIN\V480PAT.COM`; the five demo `build-d88.sh`, `build-from-psd.sh`,
  the SGP boot disk, all-demos and all-demos-msdos4 exit 0. Every file in
  every regenerated image is identical to the committed images; the
  remaining differences are directory dates only; the SGP boot and
  all-demos-msdos4 `.xz` are byte-identical.
- Clean-checkout MinGW build of `ad514bef` (after the menu review):
  `vaeg.exe` SHA-256
  `19f55ec48838876da9b77509041ddae03eaee149c9097ee944ead7f7654e64b4`,
  imports Windows system DLLs only. The menu review changes no media
  builder, so the media build regression above stands.

### G113

- Passed on 2026-10-15 (maintainer), with the Windows build
  `19f55ec4...64b4` (SHA-256, static MinGW) from
  [ad514bef](https://github.com/nakatamaho/vaeg/commit/ad514bef7673da8606f4019a5720588a506dedae):
  a set matching MAME's `pc8801ma` starts V2 H at `Ok` (2.3), boots a V2
  disk and starts N-BASIC; the PC-8801 and PC-88VA2 sets start; a model
  with missing roles starts; choosing the VA under 起動 returns to the VA;
  a new unformatted 2HD image is formatted by the guest. The later commit
  [9876038e](https://github.com/nakatamaho/vaeg/commit/9876038e97ec060e4580ff380be35853dc9ee5fc)
  only moves モニタ and V1/V2 表示 within 画面, as requested in the review.
