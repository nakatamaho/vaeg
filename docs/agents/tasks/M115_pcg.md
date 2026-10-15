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

# M115 — PCG-8100/8800

Status: **implemented; G115 human gate pending**

## Request

PCG-8100 (PC-8001) and PCG-8800 (PC-8801), ported from X88000, with the
source credited (maintainer, 2026-10-14 plan; started 2026-10-16). Based on
the M114 branch.

## Source

X88000 1.5.3 by Manuke, public domain (PDS, as its `X88000Src.txt`
states), archive `x88_1_5_3_src.tar.gz`, SHA-256
`66bc0b69d5a394e2195b5e277093eebcc045fbb3930c03637aa106394ae83cf3` (ADR-0016).
`io/pcg88.c` is rewritten in C from `PC88Pcg.cpp` and the PCG parts of
`X88000.cpp` and `PC88Z80Main.cpp`; the file header names them, and
ADR-0016 "Derivations" records the derivation.

## Decisions

- [POLICY] The PCG is a configuration option (`PCG`, 模擬 -> 機種の構成 ->
  PCG-8100/8800 (V1/V2)), off by default; it acts in V1/V2 (88) mode only,
  on OUT ports 00h-03h and 0Ch-0Fh, and needs no reset. One model serves
  both boards; the PCG-8100 and PCG-8800 share the port layout.
- [DERIVED] (X88000) Both PCG banks start as the 8x8 ROM's codes 80h-FFh;
  codes 80h-FFh are shown from bank 0 and codes 00h-7Fh from the ROM until
  port 03h changes it. A port 00h-02h write stores the data (or, with
  address bit 13, the ROM byte) at address bits 9-0 of the bank chosen by
  port 03h bit 4 whenever address bit 12 is set.
- [DERIVED] (X88000) A counter sounds a square wave of 4 MHz / count while
  its port 02h bit (3, 6, 7) is set; counts up to 100h are silent and 0 is
  65536. [POLICY] Deviation: an MSB-only load (RL = 10) sets only the MSB;
  X88000 falls through into the LSB/MSB load.
- [POLICY] PCG characters are drawn as 8x8 glyphs (rows doubled at 24 kHz,
  as the PC-8801-style display does). Tone volume `PCG_vol` (音 -> 個別の音量).
- [POLICY] The PCG state is saved in state files (section `PCG88`).

## Implementation

- `io/pcg88.c`, `io/pcg88.h`; reset and port binding in `io/iocore.c`; the
  font copy after the ROMs are loaded (`romva_initialize`); the tone stream
  in `sound/fmboard.c`; glyphs in `vram/maketextva.c`.
- Configuration `PCG`, `PCG_vol`; GUI items above; state section `PCG88`.
- Documentation: [`pcg.md`](../../modernization/pcg.md).

## Verification

- Selftest `PCG-8100/8800`: initial banks, character writes and the write
  bit, ROM copy, bank select and port 03h, 8253 LSB/MSB, MSB-only and BCD
  loads, short counts silent, tone enable bits, tone output, nothing while
  disabled or outside 88 mode. Mutation check: 12 of 12 mutants of
  `io/pcg88.c` killed.
- With a ROM set matching MAME's `pc8001`, a BASIC program that writes AAh
  into the eight rows of code 80h shows `CHR$(128)` as vertical stripes
  with the PCG on and as the ROM glyph with it off.
