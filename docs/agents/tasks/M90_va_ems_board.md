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
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# M90: Enable the VA EMS board

Status: **in progress**

Predecessor: G89 passed; M89 is integrated into `main` at
`5b4a22ba4e8a4fc7ef44f3d2dfcfe4c1001cde97`.

Branch: `topic/m90-va-ems-board`

Commit prefix: `M90:`

Candidate gate: `G90`

## Goal

Restore the retained EMS page-frame implementation as a configurable PC-88VA
expansion board, and provide a reproducible supplemental disk containing the
redistributable EMMVA adapter and RDEMS utility in extracted, usable form.

## Required behavior

- Expose `Device / EMS Board...` immediately below `I/O Bank Memory...`.
- Keep the existing persisted `ExMemory` key and state-save sections.
- Treat `ExMemory=0` as disabled and values 1 through 13 as installed
  capacity in 1MB units. A clean configuration retains the existing 1MB
  default.
- Apply changed EMS capacity through the normal guest-reset path. Cancel must
  leave the configuration and running machine unchanged.
- Attach the retained `08E1H`, `08E3H`, `08E5H`, `08E7H`, and `08E9H`
  EMS interface to the active VA I/O dispatcher and compatibility dispatcher.
- Preserve four 16KB page-frame windows at `C0000H`, `C4000H`, `C8000H`,
  and `CC000H`; target zero restores the ordinary memory mapping.
- Keep EMS and I/O Bank Memory independently configurable.
- Extend the ROM-less selftest for disabled behavior, VA I/O reachability,
  target/page mapping, capacity bounds, reset mapping, and configuration
  round trip.
- Update `tools/pc88va/build-softlib-archive-disk.sh` to retain the verified
  EMMVA15A and RDEMS152 archives and extract their redistributable drivers and
  documentation to appropriate supplemental-disk directories.
- Build media only from a disposable copy of the maintainer-supplied
  PC-Engine 1.1 source D88; never modify or track the source D88 or generated
  media.
- Document the hardware model, EMMVA installation order, RDEMS dependency,
  source links, generated contents, and separately supplied EMM-manager
  requirement in `docs/modernization/pc88va-hdd-software-environment.md`.

## Non-goals and invariants

- Do not add 128KB EMS-capacity increments; M90 uses 1MB units.
- Do not bundle or download a commercial EMM manager such as EMM4J or MELEMM.
  EMMVA is an adapter around such a driver, not a complete EMM manager.
- Do not add an emulator-private EMS API, BIOS service, or direct guest-memory
  injection path.
- Do not alter BMS numbering, main-memory capacity, binary payloads, CPU
  semantics, or unrelated storage and display behavior.
- Do not edit `CONFIG.SYS` automatically on the supplemental data disk.

## Commit order

1. M90 task and ROADMAP definition.
2. EMS core/VA-I/O connection, GUI/configuration, and automated tests.
3. Supplemental-disk builder and EMS software-environment documentation.
4. M90 validation record and gate handoff.

## Automated validation

- Run the ROM-less selftest and focused EMS configuration/mapping checks.
- Generate the supplemental disk from a disposable source-D88 copy, validate
  its FAT contents, and compare the source D88 before and after.
- Run Linux Debug and CI builds/tests plus the MinGW cross-build.
- Run repository invariant and diff checks.
- Confirm commit scope/order and that no generated binary asset is tracked.

## Human gate G90

From a clean checkout and clean configuration:

1. Complete the standard VA gate: V3 mode, bundled demo, OS boot, and simple
   operations.
2. Confirm EMS Board appears below I/O Bank Memory, defaults to 1MB, accepts
   1 through 13MB, persists, and resets only on applying a change.
3. With a separately supplied compatible EMM manager, load `EMMVA01.SYS`,
   the EMM manager, then `EMMVA02.SYS`; confirm the configured capacity and
   distinct data in more than one 16KB page.
4. Load RDEMS152 and confirm RAM-disk read/write operation.
5. Enable I/O Bank Memory concurrently and confirm both mechanisms work.
6. Disable EMS Board and confirm normal V3/OS operation remains intact.

G90 passes only when the maintainer explicitly says so.
