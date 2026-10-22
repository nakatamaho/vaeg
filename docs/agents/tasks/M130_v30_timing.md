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

# M130 — Main-CPU timing: V30 clock counts and programmed memory waits

Status: **in progress.**

## Request

The maintainer (2026-10-23): fix #53 (main-CPU code runs 1.5-2.6x faster
than on the PC-88VA and PC-88VA2 for every instruction class). Sources:
the NEC μPD70116 data sheet and the V20/V30 user's manual (Oct 1986),
supplied privately (`pc88va-private-docs/v30/`); the V50 data sheet for the
wait control unit.

## Findings so far

- `[CODE]` `cpu/upd9002/` charges the 80286 counts inherited from Neko
  Project II's i286 core (MOVSW 5, REP MOVSW 5 + 4n, STOSW 3, ADD r16,mem 7,
  Jcc taken 7).
- `[DOC]` V30 (μPD70116, word at an even address, instruction prefetched):
  MOV reg,mem 11, MOV mem,reg 9, ADD/INC/DEC reg 2, Jcc taken 14 / not
  taken 4, REP MOVBKW 11 + 8 a word (16 odd-odd, 12 odd-even). The
  manual's counts assume a prefetched instruction; a bus cycle is 4 clocks.
- `[MEASURED]` BENCH (#53; VA2, standard RAM, 1024 iterations of body +
  `INC DI; DEC CX; JNZ`, 30 VRTCs = 4,003,800 clocks with the VRTC handler):
  per iteration 22.3 clocks for `ADD AX,BX` (V30 table: 20; vaeg: 13), 32.9
  for `ES: MOV AL,[DI]` (V30: 31). `REP MOVSW` main RAM: 9.0 clocks a word
  on the VA2 (V30: 8; vaeg: 4), 11.2 on the VA (one extra wait per bus
  cycle in 40000h-DFFFFh). SGPBENCH (M128): `LOOP` x1000 18.1 clocks a
  pass on both machines.
- `[DOC]` BNN technical manual 3.5: waits — standard RAM 0, extended RAM
  (VA) 1, bank RAM 1, TVRAM at least 2 (usually 7-13), GVRAM at least 1.

## Scope

1. The WCU (FFF4h-FFF6h): programmed memory waits by partition (done,
   d214a27d): the VA's middle partition 1 wait, the VA2 none.
2. Instruction clock counts from the μPD70116 tables in place of the 80286
   ones, for every opcode the core executes (native mode; compatible mode
   keeps its uPD70008 counts).
3. Prefetch/bus effects as far as needed to meet the measurements (a
   refill cost after taken branches, odd-address word accesses).
4. GVRAM and TVRAM CPU waits calibrated to the measurements.
5. Calibration against BENCH (#53), SGPBENCH and SPRBENCH (#38, #47);
   the benchmarks kept runnable; ledger entries; `ctest`.

Out of scope: cycle-exact bus timing, CPU/SGP contention (#38 remains for
it), the FDC subsystem CPU.

## Gate (proposed)

G130: on a clean static build, BENCH, SGPBENCH (CPU rows) and SPRBENCH
(CPU column) in vaeg within 10 % of the VA and VA2 photographs, per
model; the standard gate (V3 boot, bundled demos, an OS); demos and media
regression as in AGENTS.md.
