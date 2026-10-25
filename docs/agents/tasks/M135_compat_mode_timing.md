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

# M135 — Compatible-mode timing against the PC-88VA and PC-88VA2

Status: **implemented; G135 human gate pending.** Z80TIME 2 photographed
on the VA and VA2; the measured cost model is in the adapter.

Branch: `milestone/m133-compat-timing`. Commit prefix: `M135:`.
Baseline: `d94188e8` (G132 accepted and merged).

Renumbered from M133 (maintainer, 2026-10-25): while this work ran, M133
(PR #56) and M134 were accepted and merged on main. The branch name and
the commits up to 764d31c4 keep the former number.

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

G135: on a clean static build, Z80TIME 2's compatible-mode rows in vaeg
within 10 % of the VA and VA2 photographs, per model; the native rows
unchanged; the standard gate (V3 boot, bundled demos, an OS); demos and
media regression as in AGENTS.md.

## Work log

- Z80TIME 2 written (`demos/z80-timing`, from the `bench/z80time`
  version); in vaeg at N = 60 every row runs on both models; the VA shows
  its wait for HI data and HI code.
- `[MEASURED]` Z80TIME 2 on the machines (photographs 2026-10-25, N =
  1200; the counts are in `docs/modernization/upd9002-upd70008-mode.md`):
  - Compatible mode does not follow the Z80's counts. Register
    instructions take 2-3 clocks (`ADD HL,DE` 2.9, `EXX` 2.1). Memory
    accesses take about 11 (`LD A,(HL)` 10.9), transfers 12.5-15
    (`JR` 12.5, `JP` 15.2), `CALL` + `RET` 41.5, `LDIR` 24 a byte.
  - 16-bit operands are one bus cycle. `LD HL,(nn)`, `PUSH` and `POP`
    wait once on the VA, as `LD A,(nn)` does.
  - Code above 40000h costs the VA about half a clock a byte: one wait
    an aligned word.
  - CALLN + native IRET take 73.6 clocks; BRKEM + RETEM with LOOP 96.8.
- `[CODE]` `cpu/upd9002_upd70008.cpp`:
  - `CompatInstructionCost` classifies each instruction by its prefix
    and opcode. The class gives the measured cost, in 1/256 clock with
    the fraction carried, and the not-taken cost for conditional
    transfers (taken when the next PC is not after the instruction).
    The core's Z80 cycles are only counted, and are charged instead for
    I/O, HALT, interrupt acknowledge and unknown encodings.
  - Unmeasured classes take the nearest measured one; the source names
    the measured rows with "M".
  - Memory waits are still charged natively, except for an odd
    instruction byte (fetched with the word before it) and the second
    byte of a word operand at an even address.
  - `cpu/z80_compat_cpu.*` reports whether a read is a code read
    (`CodeRead`) and whether the core is halted (`Halted`), for the
    uPD9002 adapter only; the FDC's uPD780 is unchanged.
  - `cpu/upd9002/upd9002_core.c`: BRKEM and CALLN charge 47 clocks (were
    38), RETEM 37 (was 27).
  - M132's H/S scale (`pccore_compat_clocks`) applies to the measured
    cost as it did to the core's.
- `[MEASURED]` vaeg divided by the machines, per row of the whole block:
  - Z80TIME 2: all 50 rows within 0.98-1.03 on both models, except the
    native `MOV AL,(BX)` row (0.95, unchanged since M130).
  - Z80TIME 1 (N = 300): the 11 compatible-mode rows within 0.96-1.02.
  - Before M135, compatible-mode rows ranged from 0.57 to 3.9 per
    instruction.
- ctest 119/119 (`VAEG_ENABLE_TESTS`, with the integration trace);
  `--selftest` passes (NOP: H 19, S 38, S50 76 clocks for ten).

- `[MEASURED]` A port that runs three Z80 programs in compatible mode
  (stage 1 frames 1250-2250, renders a second) draws about the same as
  before M135: VA2 model 53.9 (was 54.2), VA model 52.2 (was 52.0).
  Its Z80 code mixes instructions that M135 makes slower (memory
  accesses, transfers, CALL/RET) with instructions it makes faster
  (register and 16-bit arithmetic, exchanges).
- Static Windows handoff of 65788372 (`build-mingw-static-container.sh`,
  exit 0): `vaeg-mingw-65788372/output/vaeg.exe`, SHA-256
  `153cf12376e522408fb29e5a25319ba55c7465b0fde6bdf671ac66e08e9ceda1`.
- Demos: no demo source or builder changes, except the new
  `demos/z80-timing`, which is on no committed disk. The media and demo
  regression of AGENTS.md is still to run before G135.
