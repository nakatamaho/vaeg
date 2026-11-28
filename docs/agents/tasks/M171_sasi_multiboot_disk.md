<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
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

# M171 — SASI multiboot disk: PC-Engine 1.05 / PC-Engine 1.1 / MS-DOS 4.0 VA

**Status:** candidate ready for G171; human gate pending. See the
[M171 candidate report](../reports/m171_sasi_multiboot_disk.md). Branch
`milestone/m171-sasi-multiboot` from `main` (`b3be1b9e`); merge only after G171.

## Why

FreeDOS-88VA MS-DOS 4.0 VA milestone D5 (`docs/D5-dual-boot.md` in
https://github.com/FreeDOS-88VA/MS-DOS, branch `release/msdos-va`, merged
at `7a11162`) adds a SASI boot menu: one NEC-layout volume holds PC-Engine
1.05 (for the VA), PC-Engine 1.1 (for the VA2) and MS-DOS 4.0 VA; the menu
shows the model, waits 10 s and boots the last choice; each system has its
own CONFIG.SYS/AUTOEXEC.BAT. Building such a disk by hand takes eight
steps across three systems. The maintainer wants VAEG to build it.

## Scope

- A **new, separate** builder. The existing VA/VA2 utility SASI disks
  (`tools/pc88va/build-sasi-utility-disk.py`, `build-sasi-utility-disks.sh`)
  and their outputs stay unchanged.
- Inputs supplied by the user (never committed, never downloaded by the
  tool): PC-Engine 1.05 and 1.1 system D88s, an MS-DOS 4.0 VA distribution
  D88 with D5.1 or later, compatible with D5.1's long-label menu format.
- Output: an HDI with the D5 layout and all three entries registered,
  labels `PC-Engine 1.05 (VA)` and `PC-Engine 1.1 (VA2/3)`, default size
  40 MB (5, 10, 20 selectable), MS-DOS as the first default unless an
  option says otherwise. Optional per-system CONFIG.SYS/AUTOEXEC.BAT
  inputs. Preserve or assign FAT write timestamps for host-inserted files.
- The D5 on-disk state (BOOTMENU.DAT, BOOTMENU.SYS, IPL, parked names)
  must be what MS-DOS 4.0 VA's own `SYS` would produce. Prefer producing
  it with the product itself (drive the documented D5 procedure headless
  in VAEG: PC-Engine install, MS-DOS `SYS C:` capture, `SYS C: /P`, second
  PC-Engine install, capture) over re-implementing the state format on the
  host; host-side steps may stand in for the PC-Engine `SYS C:` runs if
  they write exactly what that SYS writes (reuse `pcengine_disk.py`).
  Document which steps run in the guest and what the user needs (VAEG
  binary, ROMs).
- Validate the result: FAT check, `SYS C: /L` lists both entries, every
  entry boots.

## Checks and G171

- Automated (ROM-free where possible): unit tests of the host-side parts;
  a mutation that drops a parked file or corrupts BOOTMENU.DAT is caught
  by the validator.
- Guest check (VAEG VA and VA2): the built 5 MB and 40 MB images boot to
  the menu showing the model; `1`, `2`, `3` boot 1.05 (`Ready`), 1.1
  (`Ready`), MS-DOS (`C>`); timeout boots the last choice; MS-DOS
  `CHKDSK C:` clean; `SYS C: /L` lists the entries.
- Repository checks, CMake/CTest as applicable.
- G171 human gate: the maintainer builds the image with the documented
  command and boots it. Open a PR; do not merge.
