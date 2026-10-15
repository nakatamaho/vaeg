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

# M113 — PC-8801 machines

Status: **planned; not started**

## Request

Run the PC-8801 family as machines of their own, from the ROMs found by
M112 under `extra/<model>/`, starting with the PC-8801MA. Started on the
maintainer's instruction before the M110, M111 and M112 gates; based on the
M112 branch.

## Goal

With a ROM set matching MAME's `pc8801ma`, choosing PC-8801MA under
模擬 -> 起動 starts the machine, **N88-BASIC V2 reaches its prompt**, and a
D88 disk **boots from the built-in FDD**.

## Scope

- 起動 -> PC-8801/8001 (追加 ROM): a complete PC-8801 model directory gets
  start entries (V2, V1, N where the model has them); the revisions chosen
  in M112 are used. Models with missing roles cannot be started.
- A plain PC-8801 machine built on vaeg's V1/V2-mode implementation,
  without the VA-only parts: ROM layout (`n88`, banks `n88_0`-`n88_3`,
  `n80`), memory and banking ports (31h, 32h, 71h, E2h/E3h), text and
  graphics display, keyboard, kanji ROM ports (E8h-EFh), dictionary banks
  (F0h/F1h), sound as fitted to the model, and the FDD unit behind ports
  FCh-FFh running `disk.rom` on the uPD780C, as the VA's FDD subsystem
  does.
- The PC-8801 without `disk.rom` starts with no FDD unit (its FDD is the
  external PC-80S31).
- A PC-8801-layout set derived from the PC-88VA/VA2 ROMs as V1/V2 mode
  shows them (`n88`, banks, `disk`, `kanji1`, `kanji2`, `font`, `jisyo`;
  no N-BASIC) is accepted as a model directory and can be started.
- Kanji level 2 in V1/V2 mode: `io/cgromva.c` `cgrom88_offset` inverts
  byte-offset bit 14 for both levels. For level 2 the non-inverted order
  matches a kanji2 ROM matching MAME's `pc8801ma`, glyph for glyph. Check
  which order the VA uses and correct vaeg if it is wrong (bug-fix ledger
  entry if so).

Out of scope: PC-8001 family (M114), PCG-8100/8800 (M115), the MC's
CD-ROM, PC-98DO's PC-98 side.

## References

- X88000 1.5.3 by Manuke, public domain: behavioural reference as in
  ADR-0016 (source archive `x88_1_5_3_src.tar.gz`, SHA-256
  `66bc0b69d5a394e2195b5e277093eebcc045fbb3930c03637aa106394ae83cf3`).
  Derivations are recorded in ADR-0016 "Derivations".
- QUASI88 0.7.4 by showtime
  (<https://www.eonet.ne.jp/~showtime/quasi88/>, `quasi88-0.7.4.zip`,
  SHA-256 `e61f110ae2adc3ee10b0e6718f58e60f796a272c25309fd0950e47e70c2844e1`):
  second behavioural reference. Its core is under a modified BSD licence;
  its sound output (MAME/XMAME, fmgen) is not used. Code derived from it
  keeps its copyright notice and licence in the file header.
- MAME's published ROM definitions (M112) for roles and sizes only.

## Verification

- Selftests on synthetic data only (ROM mapping, banking, ports); no ROM
  image, test ROM or test disk is added to the repository.
- G113 human gate: with a ROM set matching MAME's `pc8801ma`, V2 BASIC
  reaches its prompt and a disk boots; V1 and N where present; the VA and
  VA2 machines and their V1/V2/N modes still behave as before (regression);
  the clean-checkout MinGW build and the media build regression of
  AGENTS.md.
