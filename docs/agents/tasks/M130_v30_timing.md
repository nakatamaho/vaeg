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

Status: **implemented (V30 timing; CPU/SGP contention); G130 human gate pending.**

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

Decision (maintainer, 2026-10-23): no option to keep the old timing; vaeg
follows the VA and VA2 as closely as possible. Faster guest code is what
`--cpumult` is for.

6. CPU/SGP contention (#38; maintainer request, 2026-10-23, after the
   two-screen benchmark on both machines), with a demo that repeats the
   measurement.

Out of scope: cycle-exact bus timing, the FDC subsystem CPU.

## Gate (proposed)

G130: on a clean static build, BENCH, SGPBENCH (CPU rows) and SPRBENCH
(CPU column) in vaeg within 10 % of the VA and VA2 photographs, per
model; the standard gate (V3 boot, bundled demos, an OS); demos and media
regression as in AGENTS.md.

## Result

Commits on `topic/m130-wcu-memory-waits`: the WCU (402e92ed), the task
file, the V30 timing (2fbdb54d) and the bus hold of waits (d5feb5c0).

- `[CODE]` `cpu/upd9002/upd9002_mn.c`, `_f6.c`, `_fe.c`, `_rp.c`,
  `_core.c`: about 340 counts replaced by the uPD70116's (byte or even
  word, prefetched); segment and REP prefixes 2; NMI 38, INT 49, BRK 38,
  RETI 27. `memory.c`: an odd-address word is two bus cycles (+4); the
  WCU's programmed waits for CPU data cycles and instruction fetch; DMA
  and host-side accesses (`_nowait`, `meml_*`) take none. `upd9002_core.c`:
  the prefetch queue. `memoryva/gvramva.c`: GVRAM read wait 7, writes
  spaced. `io/upd9002_regs.*`: FFF4h-FFF6h (state section `UPD9WCU`; a
  state without it keeps no programmed waits).
- `[MEASURED]` vaeg divided by the photographs (more is faster):

| Benchmark | VA | VA2 | before M130 |
|---|---|---|---|
| BENCH (GG test disk, 10 loops) | 0.95-1.04 | 0.95-1.09 | 1.5-2.6 |
| SGPBENCH CPU rows (LOOP, STOSW GV/RAM, MOVSW GV-GV) | 0.95-1.07 | 0.95-1.07 | 1.06-1.8 |
| SPRBENCH CPU column (8 sizes) | 1.10-1.13 | 1.10-1.13 | 1.48-1.62 |

  SPRBENCH's remaining 8 clocks a word (four memory accesses a word) are
  left open in #47 (maintainer, 2026-10-23); the likeliest cause, a data
  cycle waiting for a prefetch cycle in progress, needs a hardware
  measurement to separate from the loops that match.
- `[MEASURED]` A game port that drew 59 frames a second in vaeg before M130
  draws 44 on average (minimum 22) in play now; the VA and VA2 showed
  23-56.
- Fixtures: `tests/upd9002/trace_baseline.txt` (8 of 52 lines, the
  `consumed`/`remain` fields only; DIVU r8 14 to 19) and
  `state_fixtures_m42.txt` (two rows' clock); their protected hashes
  updated. ctest 112/112.

## CPU/SGP contention

Branch `topic/m130-sgp-contention` (on `topic/m130-wcu-memory-waits`).

- `[MEASURED]` GG's BENCH2 (600 VRTCs a row; photographs of the VA and
  VA2, 2026-10-23): two screens, G0 over G1, cost the CPU nothing; an SGP
  list of 30 16x16 sprites takes 17.8 ms (VA) and 11.0 ms (VA2), the same
  while the CPU works; the CPU meanwhile keeps 0.66 (VA) / 0.48 (VA2) of
  its main RAM work and 0.73 / 0.46 of its G1 writes. SGPBENCH (M128) and
  SPRBENCH (M129) give the VA2 0.43-0.57 for a compute loop, STOSW and 16-
  to 128-dot sprites; on the VA they bound it below 0.35.
- `[CODE]` `io/sgp.c`: the fixed four CPU clocks per SGP memory access
  removed; `sgp_step()` stretches each CPU instruction executed while the
  SGP is busy by share / (1 - share), share 80/256 (VA) and 136/256 (VA2),
  remainder carried (`sgp.cpudelay`, `sgp.cpudelayfrac` in the state's
  spare bytes). A per-access charge fitted to the 16x16 lists (6 and 7
  clocks) made SPRBENCH's SGP75 column 1.10-1.28 slow at 64 and 128 dots;
  the time share does not.
- `[MEASURED]` vaeg divided by the machines:

| Benchmark | VA | VA2 | before |
|---|---|---|---|
| BENCH2 CPU with the SGP (N, P), after baseline | 0.96-1.04 | 0.98-1.05 | 1.07-1.19 / 1.43-1.50 |
| SGPBENCH overlap rows (time) | 1.02-1.03 | 0.88-1.03 | 0.94-1.03 / 0.80-0.90 |
| SPRBENCH SGP25-75 (time) | 0.90-0.99 | 0.90-1.00 | 0.86-0.99 / 0.85-1.01 |

  SPRBENCH's split columns now lie between its CPU column (0.89-0.91,
  #47) and its SGP column (0.95-1.00) at every size.
- `demos/sgp-contention`: SGPCONT.COM, the same 18 rows as BENCH2 with
  the same loops, segments and lists, standalone (`build.sh`, font from
  `gen_font.py`); in vaeg it gives the figures of its README. Its own run
  on the machines is pending. Not on the all-demos disks (no committed
  disk changes).

## Media and demo regression

- Utility FDD (`build-utility-disk.sh`, PC-Engine 1.1) from a clean clone
  of d5feb5c0: exit 0, 4096 bytes free; it differs from one built from
  `main` (e5727963) only in 280 directory time fields (file contents
  identical). SHA-256 `2a283385524fbce2d9515ab74c2f9176cc0d2fd8197d132da381efa087e2d699`.
- `build-sasi-utility-disks.sh` stopped on this host: it read the three
  CP/M images only from `~/88VA/images/cpm`, which is absent here, although
  their sources are cached. Fixed (maintainer request, 2026-10-23): without
  the images and without `--cpm-*-d88`, the script builds them offline with
  `tools/cpmva/install_cpmva.py` from
  `~/.cache/vaeg/auto-generated-pc88va-utility-media/cpm` (CPMVA.LZH,
  cpm2-asm.zip, bdsc-all.zip, vt100-games-*.zip), the VA2 source as boot
  disk. With PC-Engine 1.05 (VA) and 1.1 (VA2) and the utility FDD above:
  exit 0; VA `e8835d144c3e915a60b726a82f198c8a0a321fba8ebf203926b6a1da7a9ca78f`, VA2 `eef165ebd5984fb60f5970e7bed73517ea92860002fe3a319549a17ad6ec372a`. Booting them in vaeg
  needs a backup memory record with the fixed-disk boot (M106), left to
  the gate. `vaeg.exe` does not depend on this script.
- Demos: M130 changes no demo source or builder (`demos/` and `tools/` as
  on `main` except the QA hash table), so M129's byte-identical results
  stand. In vaeg with M130, 15 of the 17 programs of all-demos-msdos4 draw;
  ZUNDAORB (atlas in conventional RAM at 40000h fails) and SGP655S (one
  ball) look the same with `main`'s vaeg.
