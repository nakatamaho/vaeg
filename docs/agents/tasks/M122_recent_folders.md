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
POSSIBILITY OF SUCH DAMAGE.
-->

# M122 — separate recent folders for the FDD, HDD and tape file windows

Status: **complete; G122 passed on 2026-10-17; merged to `main` at [0e923b4](https://github.com/nakatamaho/vaeg/commit/0e923b428a88dd02bb306b82261a240be73226a3)**

## Request

The maintainer (2026-10-17): Tape > 読み込み… offers the disk folders under
最近のフォルダ, but tapes are kept elsewhere. The FDD, HDD and tape windows
should each have their own; a window without its own folder borrows, in
the order FDD, HDD, tape; with none at all the default is used.

## Before M122

One shared list (`dir` lines in `vaeg-history.txt`, 5 folders) received
the folder of every FDD, HDD and tape opened. The start folder came from
the medium in use, else `GUI_fdddir` (FDD and tape windows) or
`GUI_hdddir` (HDD), else the home directory.

## Decisions (maintainer)

- The medium in use still decides first.
- `GUI_fdddir` and `GUI_hdddir` are dropped (no longer read or written).
- The old shared list is split by the files used in each folder (below).
- Another window's folders are only used for the start folder; the
  最近のフォルダ list shows the window's own folders only.

## Work

- `vaeg-history.txt` lines `dirfdd`, `dirhdd`, `dirtape` (5 each). FDD
  opens, new FDD images and dropped disks add to `dirfdd`; SASI/SCSI opens
  and new images to `dirhdd`; tape loads and recordings to `dirtape`.
- Start folder: the folder of the medium in use, else the newest existing
  folder of the window's own list, else of the FDD, HDD and tape lists in
  that order (skipping its own), else the home directory. The new FDD,
  SASI and SCSI image dialogs use the same rule.
- Old `dir` lines: a folder goes to each of the FDD, HDD and tape lists
  whose history holds a file in it; a folder of none goes to the FDD list.
  Written in the new form at the next save.
- Selftest `recent folders` (order rules and the split), mutation-checked.

## Gate G122 (human)

Standard gate, plus: Tape > 読み込み… lists only tape folders and opens in
the last tape folder; with no tape folder yet it opens in the FDD folder;
FDD and HDD windows keep their own folders.

### G122

- Passed on 2026-10-17 (maintainer) with the Windows build
  `bbfe44b6f037...2a09` (SHA-256, static MinGW) from
  [497e6154](https://github.com/nakatamaho/vaeg/commit/497e6154e672e5ad6a587b37a4408d4fe2801f97).
  Released as Rel.20261017.2.
