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

# M120 — multi-image D88 files, and Rel.20261017.1

Status: **complete; G120 passed on 2026-10-17; merged to `main` at [bc5d913](https://github.com/nakatamaho/vaeg/commit/bc5d9134a9c271ecdb24c23a59dd3880334ffb25)**

## Request

The maintainer asked (2026-10-17) for support of D88 files that hold
several disks back to back, released as Rel.20261017.1. The TSP `MODE`
8-14 pattern work moves to M121.

## Before M120

M103f made the core read disk N of such a file (`fddfile.num`), reachable
only through `--fdd1-image`/`--fdd2-image`. Every GUI path mounted disk 1;
save states, the GUI reset and `vaeg.cfg` kept only the path, so a second
disk came back as the first (bug-fix ledger entry). Formatting a disk of a
multi-image file was already refused.

## Work

- `fddd88_images`: the number of complete images in a D88 file and their
  header names; a cut or partial image at the end is not counted.
  `fdd_diskimage` and `diskdrv_pendingimage` give the disk in or entering
  a drive.
- Save states store the disk number in bits 15-8 of the disk record's flag
  word; older states read as the first disk.
- `vaeg.cfg`: `FDD1IMAGE`, `FDD2IMAGE` (0 = first); `--fddN-image` and
  `--fddN` override them for the session only.
- GUI: FDD1/FDD2 ディスク list the disks of the file in the drive and of a
  multi-image file in the other drive (number and name); choosing one
  mounts it. The FDD menu shows `[k/N NAME]`. Resets, 最近の起動 (a
  thirteenth field `first,second`; older lines read as disk 1) and
  dropped/opened files (disk 1) keep the number. The per-drive file
  histories stay as paths.
- Release name: a `rel-YYYYMMDD[.N]` tag on the commit names the build
  (`Rel.20261017.1`); without one the commit date is used as before.

## Verification

- Selftests `D88 multi-image` (count, names, trailing bytes, a cut second
  image) and `D88 multi-image state`, each mutation-checked; every one of
  the four code commits builds and passes the selftest alone; CTest 94/94.
- GUI (Linux, offscreen) with a two-disk D88 (PC-Engine 1.1 and the
  utility disk, names set): the FDD menu shows `[1/2 PCENGINE A]`;
  FDD2 ディスク lists FDD1's file; choosing disk 2 writes `FDD2FILE` and
  `FDD2IMAGE=1`; a new start shows `[2/2 UTILITY B]` and PC-Engine lists
  the utility disk's `BIN` on B:. Copying a file to B: changed only the
  second image of the file.
- Release name: tags `rel-20261017.1` → `Rel.20261017.1`, `rel-20261018`
  → `Rel.20261018`, a malformed tag or none → the commit date.

- Media build regression at `48cc1d7e` (clean clone): the five demo
  `build-d88.sh`, `build-from-psd.sh`, the SGP boot disk, all-demos and
  all-demos-msdos4 exit 0 with every file identical to the committed images
  (SGP boot D88 and all-demos-msdos4 `.xz` byte-identical); the utility
  disk (4096 bytes free) and both SASI images build.
- Clean-checkout MinGW build of `48cc1d7e`, with a local (unpushed) tag
  `rel-20261017.1` on it: `vaeg.exe` reports `Rel.20261017.1`, SHA-256
  `121df04279211e80934bf0f8daaffd573d1c7f33a9c60c13c88177cd8471aacc`.

## Follow-up: the second disk in FDD2 (maintainer, 2026-10-17)

The maintainer opened a two-disk game file in FDD1 and found FDD2 empty.
As PC-8801 software expects disk B in drive 2, a multi-image D88 opened
(menu, history, browser) or dropped (also from an archive) into FDD1 now
puts its second disk in FDD2 as well; 最近の起動 still restores exactly
what it recorded
([7c7f7c16](https://github.com/nakatamaho/vaeg/commit/7c7f7c16f5a6843674b756e28dc14e6513c61920)). Checked with a
copy of that file: the FDD menu shows `[1/2 DISK A]` and `[2/2 DISK B]`,
`vaeg.cfg` gets `FDD2IMAGE=1`, and the game starts and runs its opening.
The drop path was built and reviewed but not exercised in the GUI test.

Gate build after the follow-up: clean-checkout MinGW build of `04a5a461`
(local tag `rel-20261017.1`), `vaeg.exe` reports `Rel.20261017.1`, SHA-256
`49b09400fdac6edd867a591ddaceb381932e7d4c7ef08ca217e4b6fa81ef17a1`.

## Gate G120 (human)

Standard gate, plus: a multi-image D88 shows its disks under FDD1/FDD2
ディスク, a game's disk change works by choosing the next disk, the choice
survives a reset, a restart and a save state, and the built executable
reports `Rel.20261017.1`.

### G120

- Passed on 2026-10-17 (maintainer) with the Windows build
  `49b09400fdac...17a1` (SHA-256, static MinGW) from
  [04a5a461](https://github.com/nakatamaho/vaeg/commit/04a5a4619c9d527d69467d2c7e5ae2a4a39cf8fc):
  a two-disk game file opened in FDD1 starts with disk B in FDD2 and plays.
