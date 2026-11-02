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

# M144 — V1/V2-only ports undecoded in V3 mode

Status: **implemented; G144 human gate pending.**

Branch `milestone/m144-v1v2-port-decode`; commit prefix `M144:`. (M143 is
used twice, by `topic/m143-runtime-speed` and `milestone/m143-wsre-w1`.)

## Request

The maintainer (2026-11-02), from the WSR-E work: vaeg answered E8h/E9h
(kanji ROM) in V3 mode although the manual marks them V1/V2-only (the W1PORT
probe recorded `00E8h = DRIVEN 00`). Fix the main unit's mode-dependent
decode, including the other V1/V2-only ports, following mode switches; not
a WSR-E-specific workaround.

## Findings

- `[VA-TM]` V1/V2-only ports in 0000h-00FFh: 31h, 34h, 35h, 5Ch-5Fh, 70h,
  71h, 78h, E2h, E3h, E4h, E6h, E8h, E9h, ECh, EDh. The I/O map reserves
  1000h-FEFFh as the user (expansion) area.
- `[CODE]` all were registered unconditionally (`memctrlva.c`, `cgromva.c`,
  `pic.c`); none checked the mode. vaeg's F0h/F1h (V1/V2 dictionary) the
  same.
- `[MEAS]` vaeg trace with the gate logging blocked accesses: none in the
  VA/VA2 V3 boots (PC-Engine 1.05/1.1), the V1/V2 BASIC boots, MS-DOS 4.0
  (all-demos-msdos4), the PC-8801mkIISR demonstration and N mode, so the ROM
  does not program these ports before entering V1/V2 mode.

## Changes

- [ddf9a3b6](https://github.com/nakatamaho/vaeg/commit/ddf9a3b6a1aa8ef6d2fba0c945abc09f789728ae): `memctrlva_attachout_v1v2()` / `memctrlva_attachinp_v1v2()`
  register the ports behind a gate on `memoryva_88_mode`, checked at each
  access; in V3 a read returns FFh (not driven) and a write is ignored.
  Selftest `V1/V2 port decode`; the 8214, PC-8801/8001 machine and V1/V2
  state tests set V1/V2 mode for the ports they use.
- V1/V2 decision T4, the ledger.

## Gate G144 (human)

Standard V3 gate (V3 boot, bundled demos, an OS), plus V1/V2: N88-BASIC on
VA and VA2, the PC-8801mkIISR demonstration, a V1/V2 game (Testament), N
mode, a PC-8801 model.

## Local verification

- `--selftest`: exit 0 (`V1/V2 port decode ok`; mutations: reads always
  passed -> "driven in V3 mode", writes always passed -> "took a write in
  V3 mode").
- Linux release: VA and VA2 V3 PC-Engine boots, VA2 V1/V2 BASIC, the
  PC-8801mkIISR demonstration, MS-DOS 4.0, N mode, the PC-8801MA model and
  Testament (both disks) reach their screens.
- `linux-ci-gcc` ctest: 146 of 146 after the BRKEM test reads 70h/5Ch in
  V1/V2 mode (it had read them in V3 mode right after a reset).
