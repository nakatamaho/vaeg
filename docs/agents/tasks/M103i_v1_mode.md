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


# M103i - V1 mode

Status: **in progress**

Series: V1/V2 mode (ADR-0016). Branch `topic/m103i-v1-mode` off `main` at
`093385e922f3bcc07b15ce5340310c61ba1d0841` (M103a–M103h merged). Commit
prefix `M103i:`.

## Goal

Run PC-8801 V1-mode software (N88-BASIC V1 and V1 media) on the emulated
VA and VA2 and compare it with the real machine (maintainer request:
"see what the real machine does"). Differences found are fixed or
recorded as decisions; vaeg follows the real VA, not a PC-8801.

## Background

- V1/V2 is the VA's memory switch B1FC5h bit 0, selected in Emulate >
  Z80 mode (M103h, decision C4); port 150h follows it at reset on both
  models (C8). N88-BASIC Version 1.x starts in V1 on both models
  (maintainer check, G103h).
- `[POLICY]` H/S is a vaeg extension (C6): the VA always reports high
  speed, and its compatible-mode CPU speed does not change with S.
  Whether V1 S software timed for a 4 MHz PC-8801 runs too fast on a real
  VA is one of the questions for the real machine.
- Before M103i no V1 media had been run (decisions register, open items).
- N80SR and the PC-8001mkII (N80) mode are not pursued (maintainer
  decision, 2026-10-05): on a real VA neither display is reachable even
  with a custom I/O-trap handler (study in
  [`v1v2-n-basic-mode.md`](../../modernization/v1v2-n-basic-mode.md#8-n80sr-and-n80-modes-not-pursued)).

## Scope

1. **Media.** V1 media from the maintainer, referred to by neutral test
   identifiers (V1-A, V1-B, …) in tracked files.
2. **Comparison.** Each title in V1 S and V1 H on VA and VA2 against the
   maintainer's real-machine observations (photos, timing, sound).
3. **Fixes** for differences with a demonstrated cause, with ledger
   entries; other differences become decisions or open observations.

## Gate G103i (human)

Standard V3 gate unchanged, plus: the V1 media chosen by the maintainer
behave in vaeg as on the real VA, or each difference is fixed or
recorded.

## Implementation progress

- Task started; V1 media supplied by the maintainer (V1-A: a PC-8801mkII
  system disk; V1-B: a game collection; V1-C, V1-D: single games).

### V1-A demonstration: `Disk I/O error` at its second BLOAD (real VA too)

- Symptom: in V1, loading V1-A's demonstration program and running it
  stops with `Disk I/O error` at its second `BLOAD`; an 8801 emulator
  runs it. `[MEAS]` The real VA shows the same error at the same line
  (maintainer, 2026-10-05), so vaeg matches the hardware. Not a defect.
- `[ROM]` Cause: the program's first `BLOAD ...,R` installs a fast loader
  that writes 34h bytes of its own code to the FDD subsystem at
  7F60h–7F93h with subsystem command 0Ch (B,C = address, D,E = count;
  0Ch at 053Bh in both ROMs), code that calls the sub-ROM's 06AFh
  (identical in both ROMs). On a PC-8801 that area is free scratch. The
  VA subsystem ROM differs from the PC-8801MA2 `DISK.ROM` in 865 bytes;
  its additions use 7F67h/7F68h: 7F67h is set to FFh by VA-only command
  25h (1607h) and cleared by 26h (1615h), and while it is nonzero the
  disk commands fail (1600h returns carry; for example command 0Eh,
  0D62h → 1465h → error) and the motor-on step is skipped (1741h,
  1764h). The uploaded code puts 40h at 7F67h, so the next disk access
  fails.
- Experiment (not a real-machine configuration): with the PC-8801MA2
  `DISK.ROM` as the subsystem ROM, the program passes that line, shows
  its wait message, and later fails with `Disk I/O error` in its
  unpacking routine. Not investigated further; a real VA never gets
  there.

### V1-C (single game)

- Headless, V1 H, no configuration file: boots from the disk on VA and
  VA2, shows its title, starts with SPACE and plays round 1 (score
  counts, enemies move).
- Maintainer check (vaeg, 2026-10-05): runs; "too high" (open
  observation, meaning to be confirmed: speed or sound pitch). Accepted
  for now.

### Monitor switch (maintainer request)

- `[VA-TM]` DIP switch SW1 selects the monitor (port 40h bit 1: 0 =
  24.8 kHz, 1 = 15.7 kHz). vaeg fixed it at 24 kHz; Emulate > モニタ now
  selects 24 kHz or 15 kHz (`Monitor15kHz`) and resets. The ROMs program
  the 15 kHz timing themselves, and vaeg's existing 15.98 kHz path draws
  200 lines with blank odd rasters. Headless: V1-C on VA and VA2 and the
  VA2 V3 start screen show that layout.
