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

# M112 — Extra ROMs for further machines

Status: **in progress (planning)**

## Request

Read user-supplied ROMs of the PC-8801 and PC-8001 families, as planned in
M110, so that later milestones (M113, M114) can emulate those machines.
Started on the maintainer's instruction before the M110 and M111 gates;
based on the M111 branch.

## Background (public sources)

ROM identities come only from MAME's published ROM definitions (SHA-1
values imported as data, below). No ROM image, and no hash other than
MAME's published values, is recorded in this repository.

- One machine does not mean one ROM. MAME lists three PC-8001 N-BASIC
  revisions (v1.01, v1.02, v1.10) and two FDD-unit ROMs of each size
  (2 KiB "disk" and "mkIIFR", 8 KiB "MA" and "mkIIMR"). Equal version
  banners can hide different contents, so a banner alone does not name a
  revision.
- The BASIC ROMs carry readable banners: "N-88 BASIC Version x.y" in the
  N88 main ROM (V1) and its fourth bank (V2), "PC-8001 BASIC Ver",
  "N-80 BASIC Ver", "N80SR-BASIC Ver".
- Other PC-8801 emulators also accept one combined 112 KiB `PC88.ROM`:
  N88 32 KiB, N-BASIC's last 8 KiB, a copy of N88's last 8 KiB, the four
  8 KiB N88 banks, DISK (8 or 2 KiB), N-BASIC's first 24 KiB.

## Decisions

- [POLICY] Layout: next to the executable (or the `--roms` directory),
  `extra/<machine>/<role>.rom` with the roles `n88`, `n88_0`-`n88_3`,
  `n80`, `n80_2`, `n80_3`, `disk`, `kanji1`, `kanji2`, `jisyo`, `cdbios`,
  `font`; machine directory names are the model names (`PC-8801MA`,
  `PC-8001mkIISR`, ...). A combined `PC88.ROM` in the directory is split
  with the layout above.
- [POLICY] Any revision is accepted; a ROM is checked only for its size
  and for not being blank. Its name comes from its banner, with the first
  eight hex digits of its SHA-1 when banners repeat.
- [POLICY] Revisions are selectable (maintainer, 2026-10-15): further
  files of a role, named `<role>_v<label>.rom` (for example
  `n80_v1.0a.rom`), appear as choices under 起動 for that machine; the
  plain `<role>.rom` is the default.
- [POLICY] Known dumps are named from MAME's published ROM definitions
  (maintainer, 2026-10-15): `src/mame/nec/pc8801.cpp`, `pc8001.cpp` and
  `src/devices/bus/nec_fdd/pc80s31k.cpp`, read at mamedev/mame
  `49dc0db2787a89d33de7982b9112fc394a1bd3b3`. A ROM whose SHA-1 MAME lists
  is shown with MAME's machine and BIOS label (for example "N-BASIC
  v1.01") and its BAD_DUMP mark; any other ROM is accepted and shown with
  its banner as unverified. Only MAME's public SHA-1 values are imported;
  SHA-1 values are identifiers, not ROM contents, and MAME's code is not
  used.
- [POLICY] Required roles follow MAME's ROM definitions with two
  maintainer corrections (2026-10-15): the PC-8801 does not need `disk`
  (its FDD is the external PC-80S31), and the PC-8801mkII needs `disk` and
  `kanji1` (every mkII model carries the FDD ROM; a model 10 runs a drive
  once one is fitted, and the kanji ROM is on board). MAME notes that its
  mkII dump holds only the main ROMs.
- [POLICY] The maintainer recalls four PC-8001 N-BASIC revisions; MAME
  lists three, so MAME's revision labels are shown as MAME's, not as
  established facts.
- [POLICY] A MAME match is shown as "MAME と一致", not as a verified good
  dump, and a mismatch is not treated as a bad dump: MAME's declared SHA-1
  values for the original PC-88VA (`vafont.rom`, `varom00.rom`) differ from
  the maintainer's own confirmed readbacks
  ([`va-rom-dump-notes.md`](../../modernization/va-rom-dump-notes.md)), so
  MAME's values can be wrong (maintainer, 2026-10-15). vaeg's VA ROM check
  keeps its own confirmed values.

## MAME's definitions (2026-10-15)

- MAME marks the three PC-8001 N-BASIC ROMs and the N80SR ROM BAD_DUMP;
  such ROMs are still used, with that mark shown.
- MAME notes that its PC-8801mkII dump holds only the main CPU ROMs; its
  other mkII ROMs are taken from the PC-8801 and the mkIISR.
- MAME has no PC-98DO set in these files; PC-98DO ROMs are accepted and
  shown with their banners.
