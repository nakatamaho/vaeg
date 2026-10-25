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

# M133 — Compatible-mode timing against the PC-88VA and PC-88VA2

Status: **started; measuring.** Z80TIME 2 is built and checked in vaeg. Its
run on the machines is pending.

Branch: `milestone/m133-compat-timing`. Commit prefix: `M133:`.
Baseline: `d94188e8` (G132 accepted and merged).

## Request and scope

The maintainer (2026-10-25): calibrate vaeg's timing of the uPD9002's
compatible (uPD70008) mode to the machines (issue #62), after M132.

- `[MEASURED]` Z80TIME 1 (issue #62, photographs of the VA and VA2):
  - Compatible mode takes about 1.35 times vaeg's Z80 counts for DJNZ,
    memory accesses and CALL/RET, but less for `ADD A,B` (3.1 against 4)
    and `LD A,(IX+d)` (12.6 against 19).
  - CALLN + IRET takes 72 clocks (vaeg 65); BRKEM + RETEM about 90 with
    the loop (vaeg 74).
  - The VA's middle-partition wait applies to compatible-mode data.
  - Native code agrees within 0.90-1.05 (M130).
- `[CODE]` The instruction core is the vendored `external/suzukiplan-z80`,
  which charges the Z80's cycles through `consumeClock` (not to be edited).
  The adapter `cpu/upd9002_upd70008.cpp` receives them in `CompatCounter`;
  M132 added the H/S scale there (`pccore_compat_clocks`) and keeps the
  native memory and I/O waits apart.

Eight instructions do not determine a table for the whole instruction set.
Z80TIME 2 (`demos/z80-timing`) therefore measures 46 instructions and
classes one at a time: eight copies a pass, so one instruction is the row
less the base, divided by eight. It covers:

- loads, ALU, 16-bit arithmetic, exchanges;
- (HL), (DE), (nn) and (IX+d) accesses, with data and code above and below
  40000h;
- PUSH/POP, CALL/RET, JR/JP taken and not taken, RET not taken;
- CB and DD CB instructions, NEG, LDIR;
- CALLN, BRKEM/RETEM;
- native references.

## Plan

1. Z80TIME 2 on the VA and VA2 (photographs; the counts are also in GVRAM
   for vaeg's runs).
2. A cost model in the adapter. The core's cycles for each instruction are
   replaced by the measured cost of its class (opcode, prefix, taken or
   not), with the VA's waits still charged natively and M132's H/S scale
   applied on top.
3. Selftests for the model; Z80TIME 1 and 2 in vaeg within 10 % of the
   photographs on both models; ctest; the ledger; the demo and media
   regression.

## Gate (proposed)

G133: on a clean static build, Z80TIME 2's compatible-mode rows in vaeg
within 10 % of the VA and VA2 photographs, per model; the native rows
unchanged; the standard gate (V3 boot, bundled demos, an OS); demos and
media regression as in AGENTS.md.

## Work log

- Z80TIME 2 written (`demos/z80-timing`, from the `bench/z80time`
  version); in vaeg at N = 60 every row runs on both models; the VA shows
  its wait for HI data and HI code.
