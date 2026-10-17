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

Status: **implemented; G120 human gate pending**

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

## Gate G120 (human)

Standard gate, plus: a multi-image D88 shows its disks under FDD1/FDD2
ディスク, a game's disk change works by choosing the next disk, the choice
survives a reset, a restart and a save state, and the built executable
reports `Rel.20261017.1`.
