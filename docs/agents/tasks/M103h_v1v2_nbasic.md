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


# M103h - N-BASIC in V1/V2 mode

Status: **in progress**

Series: V1/V2 mode (ADR-0016). Branch `topic/m103h-v1v2-nbasic` off
`main` at `98ac7a89013871a067ab9d1b3f3a7c1015a2789a` (M103a–M103g merged).
Commit prefix `M103h:`.

## Goal

Run PC-8001/8801 N-BASIC from a user-supplied N80 ROM (the VA carries no
N-BASIC): entered by a start-up option and, as on a PC-8801 in V1S mode,
by `NEW ON 1` from V1/V2 N-88 BASIC (maintainer requirement). N80SR
(N80 V2) and V1 media follow later.

## Background

- `[MEAS]` (maintainer) On a real VA, N-BASIC runs once its ROM image is
  copied to RAM and run in all-RAM mode (I/O 1987-11, "88VA plus N-BASIC").
- `[ROM]` `NEW ON 1` (extension ROM bank 2, 77A7h) needs IN 31h bit 7 = 1
  (V1) and bit 6 = 0 (standard speed); otherwise error 33.
- `[VA-TM]` IN 31h reads memory switch B1FC6h: MS27 V1/V2, MS26 unused and
  always 1. `[X88000]` on the PC-8801 bit 6 is high speed.
- `[ROM]` V1/V2 is B1FC5h bit 0 (1 = V1): the VA2 ROM writes B1FC5h to
  port 1C6h at reset (F000:12E4); port 150h then reports the mode and the
  ROM recomputes MS27 (F000:14BC). Before that it sets B1FC6h bit 7 and
  checks the record's checksum B1FCDh (F000:23B7).
- Experiment (temporary code, not committed): mapping the ROM at
  0000h–7FFFh under port 31h RMODE and jumping to 0000h, N-BASIC 1.8
  initialises the screen, then waits for the FDD subsystem during its
  boot-disk probe.

## Scope

1. **Mode selection.** Emulate > Z80 mode menu: V2 H, V2 S, V1 H, V1 S.
2. **N80 ROM option** and the N-BASIC memory map.
3. **Entry**: start-up option, and `NEW ON 1` in V1S.
4. **Boot-disk probe**: find why N-BASIC waits for the FDD subsystem.

## Gate G103h (human)

Standard V3 gate unchanged, plus: the Z80 mode menu selects V1/V2 (BASIC
banner Version 1.x / 2.x after reset); N-BASIC starts with a
user-supplied ROM, both from the start-up option and by `NEW ON 1` in
V1S, and runs a short program.

## Implementation progress

### Mode selection (scope item 1)

- Maintainer request: an Emulate > Z80 mode menu with V2 H, V2 S, V1 H and
  V1 S. This replaces the M103b decision of no user-facing V1/V2 choice:
  V1/V2 is now the VA's own memory switch (B1FC5h bit 0), which its setup
  stores, not an override. H/S is a vaeg extension (the VA reports H);
  S clears IN 31h bit 6 in 88 mode, without changing CPU timing.
- While testing, saved backup memory turned out to reset its memory
  switches at every boot: vaeg's checksum did not follow the ROM's rule
  (ledger entry). With the fix the record survives three boots in a row,
  and a V1 setting boots N-88 BASIC Version 1.9.
- Romless test: V1/V2 selection with the ROM-rule checksum, and S clearing
  MS26 only in 88 mode; it fails without either change.
