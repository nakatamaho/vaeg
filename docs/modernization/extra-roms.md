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

# Extra ROMs: PC-8801 and PC-8001 families

M112 lets vaeg find user-supplied ROMs of the PC-8801 and PC-8001 families,
check them, name them, and remember which revision of each ROM to use. M113
starts the PC-8801 models from them ([PC-8801 machines](pc8801-machines.md));
the PC-8001 family follows in M114. Task:
[`M112`](../agents/tasks/M112_extra_roms.md).

No ROM is provided by this repository. Dump ROMs from hardware you own.

## Layout

The ROM directory is the one holding the VA ROMs (beside the executable, or
`--roms`). Below it:

```text
extra/
  PC-8801MA/
    n88.rom  n88_0.rom ... n88_3.rom  n80.rom  disk.rom
    kanji1.rom  kanji2.rom  jisyo.rom
  PC-8001/
    n80.rom  n80_v1.01.rom  n80_v1.02.rom  font.rom
```

- One directory per model, named after it: `PC-8001`, `PC-8001mkII`,
  `PC-8001mkIISR`, `PC-8801`, `PC-8801mkII`, `PC-8801mkIISR`,
  `PC-8801mkIITR`, `PC-8801mkIIFR`, `PC-8801mkIIMR`, `PC-8801FH`,
  `PC-8801MH`, `PC-8801FA`, `PC-8801MA`, `PC-8801FE`, `PC-8801MA2`,
  `PC-8801FE2`, `PC-8801MC`, `PC-98DO`, and (M113) `PC-88VA` and `PC-88VA2` for PC-8801-layout sets
  taken from the VA ROMs. Other directory names are listed as an unknown
  model.
- One file per role, named after the role (case does not matter).
- Revisions of a role: `<role>_v<label>.rom`, for example `n80_v1.01.rom`.
  The plain `<role>.rom` is the default.
- A combined 112 KiB or 88 KiB `PC88.ROM`, as used by other emulators, may
  be placed in a model directory instead; it is split (below). A plain
  `<role>.rom` takes precedence over the part of `PC88.ROM`, which in turn
  takes precedence over `<role>_v<label>.rom` files.

### Roles

| Role | Size | Contents |
|---|---|---|
| `n88` | 32 KiB | N88-BASIC main ROM (its banner gives the V1 version) |
| `n88_0` ... `n88_3` | 8 KiB each | N88-BASIC banks at 6000h; `n88_2` carries the V2 banner |
| `n80` | 32 KiB (24 KiB on the PC-8001) | N-BASIC |
| `n80_2` | 32 KiB | PC-8001mkII ROM (N-BASIC and N80-BASIC) |
| `n80_2sr` | 32 KiB | PC-8001mkIISR ROM |
| `n80_3` | 40 KiB | N80SR-BASIC |
| `disk` | 2 or 8 KiB | FDD unit ROM |
| `kanji1`, `kanji2` | 128 KiB each | JIS level 1 and level 2 kanji |
| `jisyo` | 512 KiB | kana-kanji dictionary |
| `cdbios` | 64 KiB | CD-ROM BIOS (PC-8801MC) |
| `font`, `font80sr`, `fonthira` | 2, 4, 2 KiB | 8x8 character ROMs of the PC-8001 family and the PC-8801 |

### Roles each model needs

The requirements follow MAME's ROM definitions, with two maintainer
corrections: the PC-8801 does not need `disk` (its FDD is the external
PC-80S31), and the PC-8801mkII needs `disk` and `kanji1` (every mkII model
carries the FDD ROM; a model 10 runs a drive once one is fitted).

| Model | Required |
|---|---|
| PC-8001 | `n80`, `font` |
| PC-8001mkII | `n80_2`, `font` |
| PC-8001mkIISR | `n80_2sr`, `n80_3`, `font`, `font80sr`, `fonthira` |
| PC-8801 | `n88`, `n88_0`, `n80`, `font` or `kanji1` |
| PC-8801mkII | `n88`, `n88_0`, `n80`, `disk`, `kanji1` |
| PC-8801mkIISR, mkIITR, mkIIFR | `n88`, `n88_0`-`n88_3`, `n80`, `disk`, `kanji1` |
| PC-8801mkIIMR, FH, MH, FA, FE, FE2 | as mkIISR, and `kanji2` |
| PC-8801MA, MA2 | as MH, and `jisyo` |
| PC-8801MC | as MA, and `cdbios` |
| PC-98DO | `n88`, `n88_0`-`n88_3`, `n80`, `disk` |
| PC-88VA, PC-88VA2 | `n88`, `n88_0`-`n88_3`, `disk`, `kanji1` |

A model with a missing role is shown as 不足あり; its ROMs are still listed,
and (M113) it can still be started as long as it has `n88`.

## Checks and names

Any revision is accepted. A ROM is checked only for the size of its role and
for not being blank (every byte equal); a file that fails, or whose name
names no role, is listed as not used, with the reason.

Each ROM is shown with, in order of preference:

1. MAME's BIOS label when MAME lists its SHA-1 with one (for example
   "N-BASIC v1.01");
2. its version banner, read from the ROM ("N-88 BASIC Version 1.9",
   "PC-8001 BASIC Ver 1.3 + N-80 BASIC Ver 1.0" when one ROM holds two
   BASICs);
3. its size.

When two files of a role would read the same but differ in content, the
first eight hex digits of their SHA-1 are added. Equal banners do occur
with different contents (two PC-8001 "Ver 1.0" ROMs, two N-BASIC "Ver 1.8"
ROMs, three N88 V2 "2.3" ROMs), so a banner alone does not name a revision.

MAME defines no ROMs for the mkIITR, FE and FE2 (their machine entries are
commented out), so their files are never shown as MAME matches; their
required roles follow the mkIISR and the FA.

### MAME's published values

`sdl2/extrarom_mame.inc` lists the SHA-1, machine, file name, BIOS label and
BAD_DUMP mark of every ROM in MAME's `src/mame/nec/pc8001.cpp`,
`src/mame/nec/pc8801.cpp` and `src/devices/bus/nec_fdd/pc80s31k.cpp`. It is
generated by `tools/pc88va/extrarom-mame-table.py` from those files at the
MAME revision recorded in its header; no MAME code is used. When one SHA-1
appears under several MAME machines, the entry of the model directory's own
machine is preferred.

A match is not marked; it names the ROM and is a reference only, not proof
of a good dump: MAME's declared values for the original PC-88VA's
`vafont.rom` and `varom00.rom` differ from confirmed readbacks
([VA ROM dump notes](va-rom-dump-notes.md)). A ROM MAME does not list is
accepted and marked "MAME と不一致"; one MAME marks BAD_DUMP is marked so.

## Combined PC88.ROM

Other PC-8801 emulators accept one combined file with this layout:

| Offset | Size | Part |
|---|---|---|
| 00000h | 32 KiB | `n88` |
| 08000h | 8 KiB | last 8 KiB of `n80` (6000h-7FFFh) |
| 0A000h | 8 KiB | copy of `n88` 6000h-7FFFh (not used) |
| 0C000h | 4 x 8 KiB | `n88_0`-`n88_3` |
| 14000h | 8 KiB | `disk`; a 2 KiB ROM is padded with FFh and is taken as 2 KiB |
| 16000h | 24 KiB | first 24 KiB of `n80` (0000h-5FFFh) |

An 88 KiB file ends before 16000h and has no `n80`. Blank parts are not
used. The original PC-88VA's ROM file of the same 112 KiB size does not
follow this layout; do not place it here.

## Selecting revisions

`模擬 -> 起動 -> PC-8801/8001 -> <model> -> ROM` lists the model's missing
roles and its files; the directory is scanned (SHA-1) each time the
起動 menu opens. A role with several files becomes a submenu of its
revisions; the choice is stored per model and role in the configuration key
`ExtraROMSelect` (`<model>/<role>=<revision>;...`). Hovering an entry shows
its file, size, SHA-1, banner and MAME name. A file changed after the scan
is not read until the menu is opened again.

The start entries of a model (M113) are in the same model submenu.

## Command line

`vaeg --list-extra-roms [--roms DIR]` prints the same information (one line
per file: role, file, size, SHA-1, name, MAME name, `*` for the file in use)
and exits.
