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

# M111 — Boot modes: machine ▶ mode

Status: **in progress**

## Request

The maintainer asked (2026-10-15) to go on with M111 as planned in M110:
起動 as machine ▶ mode, with 自動, V3 and V1/V2 chosen at boot, and N in the
same list. M111 starts before G110 passed, on the maintainer's instruction;
it is based on the M110 branch and is rebased if G110 needs changes.

## Findings

- `[ROM]` Both ROMs test port 40h bit 3 (SW7, active low) before the V3
  boot search: VA F000:0AEF, VA2 F000:136C. SW7 OFF (bit 3 = 1) skips the
  search and starts V1/V2; that is how N mode (C9) already starts without
  a disk. With SW7 ON and no V3 IPL, the original VA falls back to V1/V2
  and the VA2 asks for a disk.
- No ROM path refuses V1/V2 when a disk has no V3 IPL. A "forced V3" mode
  therefore has no direct hardware counterpart; the PC key held at power-on
  enters the V3 setup instead.

## Work

- `BootMode` (0 自動, 1 V1/V2, 2 N) replaces `V1V2_NMode`. 自動 is today's
  behaviour; V1/V2 makes SW7 read OFF from reset until the first
  compatible-mode entry, so V2 H/S and V1 H/S start V1/V2 with a V3 disk or
  with no disk, and N-88 BASIC still boots a V1/V2 disk. The 自動 label shows
  the V1/V2 mode used for non-V3 disks. Recent boots record the mode.
- Checked in vaeg (VA2): 自動 with no disk asks for a disk and boots
  PC-Engine from its disk; V2 H with no disk and with the PC-Engine disk
  starts N-88 BASIC. Selftest `boot mode` (fails with SW7 left alone for
  V1/V2).

## Decisions

- [POLICY] No V3 entry (maintainer, 2026-10-15): no ROM path refuses V1/V2
  for a disk without a V3 IPL, and a V3 disk already boots V3 under 自動.
  The alternatives, the V3 setup (PC key held at power-on) or a vaeg-side
  refusal of the V1/V2 hand-off, were not taken.
- [POLICY] The machine stays in `pc_model`; only the boot mode got a new key
  (`BootMode`). M110's plan of a `Machine=` key is dropped as it would only
  rename `pc_model`.
