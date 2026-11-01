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
POSSIBILITY OF SUCH DAMAGE.
-->

# M143 — WSR-E stage W1: the VA side of the port path and the board probe

Status: **draft; not started.** Starts only when the maintainer assigns
it. The number is provisional (M142 is the last completed milestone);
renumber the file, branch and prefix if the maintainer chooses another.

Branch `milestone/m143-wsre-w1`; commit prefix `M143:`.

Plan: [`docs/modernization/wsr-e-pseudo-sprite-plan.md`](../../modernization/wsr-e-pseudo-sprite-plan.md),
stage W1. Read sections 5, 7, 8, 9 and 11 before starting.

## Request

The maintainer (2026-11-02): extend `demos/sgp-pseudo-sprite/65536` to the
Melco WSR-E (CL-GD5428) on the PC-88VA; collect test programs in
`demos/wsr-e-pseudo-sprite`; show machine, FPS and ball count as the other
demos do; MS-DOS 4.0; split into milestones; record timing benchmarks
exactly; implement the board in vaeg in parallel.

W1 answers two questions before any VRAM or BLT work:

1. Does the VA drive the low bytes of the WSR-E ports (E0h, E1h, E8h) in
   V3 mode, and does port 152h read back what is written?
2. Is a GD542x answering at 4nE0h through the VA slot, and do word OUTs
   reach it as index + data?

## Known before the stage

- `[DOC]` VA I/O 1000h-FEFFh is the user area for expansion boards; E0h
  and E1h are unused; E2h-E9h, ECh, EDh are V1/V2 only (Technical Manual
  2.2).
- `[DOC]` Port 152h bit 3 = 1 releases E0000-EFFFFh to the slot (3.3);
  the ROM reads 152h as a word.
- `[OTHER-BOARD]` AGS_CLGD.COM (GA-98NB, same port map): wake-up
  46E8h=18h, 42E0h=01h, 46E8h=08h, 42E0h=E3h, 5AE0h=00h; SR06 probe by
  `OUT 44E0h,AX` (1206h) and `IN 45E0h` = 12h; word OUTs to 44E0h, 4EE0h,
  54E0h; VRAM at E000h through GR09/GR0A.
- `[MEASURED]` (getromva comparison, 2026-11-01) an undriven VA address
  reads back code bytes near the reading instruction (`AA 0E`).
- `[CODE]` vaeg's iocore decodes all 16 address bits and returns FFh for an
  unattached port; `iocore_out16` writes port and port+1 as two bytes.
- `[CODE]` vaeg's port 152h (`io/memctrlva.c`) keeps `rom0_bank`; banks
  8-9 come from `varom08.rom` (all FFh: an empty slot).

## Scope

### A. Programs (`demos/wsr-e-pseudo-sprite/`)

Already drafted (assemble, and pass `smoke.py` in every port model):
`build.sh`, `wsre.inc`, `tests/w1port.asm`, `tests/w1probe.asm`,
`smoke.py`, `README.md`. Review them against the plan, then:

1. Keep the CSV format of plan section 7 exactly; it is the interface
   between the machines, vaeg and the host scripts.
2. Add `tools/wsre/compare_logs.py` (or a place the maintainer prefers):
   reads two `WSRLOG.TXT` files and prints, per row, the machine value,
   the vaeg value and the ratio for timing rows; for check rows, equal or
   not, with `FF` and `FLOAT ...` treated as the same class.
3. Add the programs to the MS-DOS 4.0 all-demos disk only if the
   maintainer asks; otherwise `build.sh --msdos4` makes a separate disk.

### B. vaeg

1. Setting: `np2cfg.wsreenable` (default 0), INI key `WSRE`, as
   `mpuenable` in `machine/pccore.*` and `sdl2/ini.c`. A GUI switch only
   if the maintainer asks.
2. `cbus/wsre.c`, `cbus/wsre.h`, registered in `cbus/cbuscore.c`
   (reset/bind only when enabled):
   - ports 40E0h + n*100h (n = 0..F) as VGA 3C0h + n; 54E0h/55E0h CRTC;
     5AE0h status; 46E8h setup/enable (keep the value, readable);
   - board ports 40E1h, 42E1h, 51E1h, 59E1h, 5BE1h: keep written values;
     reads return what W1PROBE photographs show (until then FFh, marked
     `[OPEN]` in the code);
   - word writes: because `iocore_out16` splits them, a write to 44E1h,
     4EE1h or 54E1h is the data of the paired index register (45E0h,
     4FE0h, 55E0h). If W1PROBE shows the machine behaves otherwise,
     follow the machine;
   - nothing in the memory map yet (W2).
3. `vram/gd5428.c`, `vram/gd5428.h`: register files SR 00-1F, GR 00-3F,
   CRTC 00-3F, attribute 00-14 (with the index/data flip-flop), DAC
   index/data, pixel mask; SR06 reads 12h after 12h is written and 0Fh
   otherwise; CR27 = the value W1PROBE photographs show (98h is assumed
   for the 5428 until then, `[OPEN]`); extended registers read as written.
   No display, no BLT.
4. State: a new section `WSRE` saved only when the board is enabled; a
   state without it loads with the board absent. Nothing changes when the
   board is disabled.
5. Tests: unit tests for SR06, the word split, GR09/GR0A read-back, CR27;
   a headless run of W1PORT and W1PROBE (board off and on) whose logs are
   compared with checked-in expected logs, ignoring the build id and R0
   counts.

Out of scope: the E window (W2), BLT (W3), timing of board I/O (W4),
display and relay (W5).

## Expected results

| Row | VA | VA2 | vaeg |
| --- | --- | --- | --- |
| W1PORT P 00E0, 00E1, 00E8 (no board) | FF or FLOAT | FF or FLOAT | FF |
| W1PORT B152 SAME, SLOT, RESTORE | PASS | PASS | PASS |
| W1PORT R0 counts | photograph | photograph | 0.9-1.1 of the machine (M130 calibrated) |
| W1PROBE SR06B 12 / 00 | 12 / 0F | 12 / 0F | 12 / 0F |
| W1PROBE SR06W 12 / 00 | photograph | photograph | as the machine |
| W1PROBE CR27 | photograph | photograph | as the machine |
| W1PROBE GR09B/GR0AB, GR09W/GR0AW | photograph | photograph | as the machine |
| W1PROBE P 40E1h ... 5BE1h | photograph | photograph | as the machine |

Fill the VA and VA2 columns from the logs; a row that differs from the
expectation is a finding, not a failure of the task: record it here and
in the plan's section 9, then make vaeg follow it.

## Gate G143 (human)

Standard V3 gate, plus:

- W1PORT without the board, W1PORT with the board, and W1PROBE, on the
  PC-88VA and the PC-88VA2: photographs showing machine, build id and
  display rate, and `WSRLOG.TXT` from the disk;
- vaeg with the board off runs W1PORT and W1PROBE (exit 0 and exit 2);
  with the board on, both logs match the machines' logs row by row
  (`compare_logs.py`), R0 within 0.9-1.1;
- vaeg with the board off behaves as before M143: demos and media
  regression of `AGENTS.md`, state files of M142 still load.

## Procedure on the machines (for the maintainer)

1. Build: `NASM=nasm demos/wsr-e-pseudo-sprite/build.sh --msdos4
   msdos4-pc88va-2hd.d88 OUTDIR`, write the D88 to a floppy.
2. Boot it in V3 mode (the log is written to the current drive; the disk
   must not be write-protected).
3. Without the board: `W1PORT`; photograph.
4. With the board: `W1PORT`, then `W1PROBE`; photograph each.
5. Return `WSRLOG.TXT` (or the whole disk image).

W1PORT writes port 152h with interrupts off for a few instructions. W1PROBE
writes 46E8h, 42E0h, 5AE0h, SR06, GR09 and GR0A, never 40E1h, so the
monitor stays on the VA.

## Local verification (to fill)

- `build.sh` exit status, NASM version, SHA-256 of both COMs.
- `smoke.py` with the default, `--float self`, `--float next` and `--board`
  models: exit codes 0/0/0 (W1PORT) and 2/2/0 (W1PROBE).
- vaeg: unit tests, headless runs, ctest totals, clean static MinGW build
  (absolute path of `vaeg.exe` for the gate).

## Open

- Does the VA slot pass 16-bit I/O cycles (SR06W, GR09W rows)?
- Melco values of the board ports; meaning of 40E1h bit 1.
- CR27 of the WSR-E's 5428 (98h assumed).
