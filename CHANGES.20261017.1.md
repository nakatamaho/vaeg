<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# 88VA Eternal Grafx Rel.20261017.1

This release follows Rel.20261017 with support for D88 files that hold
several disks.

## Multi-image D88 files

- Opening a D88 file that holds several disks mounts its first disk.
  FDD > FDD1 ディスク and FDD2 ディスク list the disks of the file, by number
  and the name stored in the file, and put the chosen one in the drive; a
  second disk of the file in FDD1 can go into FDD2 the same way.
- The FDD menu shows the disk in use, for example `[2/3 NAME]`.
- The chosen disk is kept over a reset, in `vaeg.cfg` (`FDD1IMAGE`,
  `FDD2IMAGE`), in save states and in 最近の起動. Writes go to that disk.
- `--fdd1-image N` and `--fdd2-image N` choose a disk from the command line.

## Fixes

- A save state, a reset, or the next start no longer replaces the second
  disk of a multi-image D88 with its first.
- Release notes and several documents rendered blank on GitHub because their
  license comment was not closed; they are fixed, and a repository check now
  catches it.

ROM images, fonts, and private disk images are not included. Supply the
model-specific PC-88VA ROM files separately.
