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

## Gate G106 (human)

Standard V3 gate unchanged, plus: the built disk boots to the menu in vaeg
and on the PC-88VA2, and the demos run from it (results on hardware are
recorded whatever they are).

## Implementation progress

