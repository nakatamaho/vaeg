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

# M172 — HOSTDRV design: host-folder drive for MS-DOS 4.0 VA through the DOS redirector

**Status:** G172 human gate passed on 2026-11-27 (maintainer: design approved; decisions (a)–(d) as recommended) at `f99f066a`. Branch `milestone/m172-hostdrv-design` from `main` (`b3be1b9e`).
**Maintainer-approved exception (2026-11-27):** M172 began while G171 was pending; this did not accept G171.

- [Design](../research/m172_hostdrv_design.md)
- [Research report and probe build](../reports/m172_hostdrv_research.md)
**Maintainer-approved exception (2026-11-27):** M172 may begin while G171
is pending. This neither accepts G171 nor waives G172 or any merge or
release approval. The one-task-per-session rule still applies.

## Why

HOSTFAT (M54/M55) gives PC-Engine a read-only FAT12 snapshot of a host
folder: changes need a rebuild and reset, nothing can be written, at most
1024 entries, ASCII 8.3 names only. That design follows from PC-Engine
lacking the MS-DOS network-redirector bridge (M56). FreeDOS-88VA MS-DOS
4.0 VA (https://github.com/FreeDOS-88VA/MS-DOS, `release/msdos-va`) is a
real MS-DOS 4.0 kernel with the INT 2FH AH=11H redirector interface; its
MIT-licensed source documents the SDA, CDS, SFT and redirector calls.

## Plan (maintainer decisions)

- Guest: a small resident program (working name `HOSTDRV.COM`) for
  MS-DOS 4.0 VA that claims a free drive letter as a redirected drive and
  services INT 2FH AH=11H, talking to VAEG through the emulator-private
  07EDH/07EFH channel (new versioned commands). It refuses to install when
  the channel is absent (real hardware).
- Host: VAEG maps each request onto host file operations of a selected
  folder; live view (no snapshot, no reset); Shift-JIS ↔ UTF-8 names with
  deterministic 8.3 aliases only where needed.
- Milestones: **M172 design and research only (this task)**; then a
  read-only implementation; then writes. The protocol and the guest
  program live in VAEG; the MS-DOS distribution may ship the program as
  an option. HOSTFAT stays for PC-Engine.

## Scope of M172 (no product implementation)

1. **DOS 4.0 facts from source** (cite file/line in
   `v4.0/src/DOS/*.ASM`, `v4.0/src/INC/*.INC`): every INT 2FH AH=11H
   subfunction DOS 4.0 issues and when; register/stack/SDA conventions
   per call (current path, FCB/name buffers, SFT pointer, search data);
   SDA layout and how to find it (INT 21H/AX=5D06H, and DOS 4.0's
   AX=5D0BH if present); CDS entry size and fields (and how to find the
   list of lists); SFT layout for redirected files; what DOS 4.0 adds over
   3.x (extended open AX=6C00H, large-volume fields, IFS hooks) and how a
   redirector must answer them.
2. **Guest probe** (non-resident, like M56's `R2FPROBE`): on MS-DOS 4.0 VA,
   print the DOS version, SDA address/size, list-of-lists, CDS entry size
   and LASTDRIVE, and confirm `INT 2FH/AX=1100H` behaviour; restores every
   vector. Build/check script like `tools/pc88va/hostfs/check_probe.py`.
3. **Protocol design** for the 07EDH/07EFH channel: discovery/version,
   request/response layout, buffers (size limits, DMA-free copy through a
   guest buffer), handle model, errors mapped to DOS error codes,
   read-only flag, save-state policy (open host handles), concurrency.
4. **Host design**: folder selection and persistence (like HOSTFAT
   settings), path containment and link policy (reuse M55 rules), name
   mapping (Shift-JIS, 8.3 aliases, collisions, case), timestamps,
   attribute mapping, free-space reporting, Windows/macOS/Linux specifics.
5. **Guest design**: install/uninstall, drive-letter choice, memory size
   target, coexistence with JFP/FEP, COMMAND, FCB programs, `SUBST`/`JOIN`
   interaction, behaviour on real hardware.
6. **Milestone split and gates** for read-only and write phases.

Deliverables: `docs/agents/research/m172_hostdrv_design.md` (English) with
the above, the probe source and checker under `tools/pc88va/hostdrv/`,
and a report under `docs/agents/reports/`. Follow AGENTS.md on media and
test-result disclosure (no statements of ROM/disk use or outcomes in Git
without maintainer-approved wording).

## G172

Human gate: the maintainer reviews the design document and approves (or
amends) the protocol and milestone split. Open a PR; do not merge before
G172.
