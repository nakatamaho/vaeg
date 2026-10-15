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

# PCG-8100/8800 (M115)

The PCG-8100 (PC-8001) and PCG-8800 (PC-8801) add 256 user-definable 8x8
characters and a three-voice tone generator (an 8253). vaeg models them in
V1/V2 mode, for the VA's own V1/V2/N modes and for the PC-8801 and PC-8001
machines (M113, M114). Task: [`M115`](../agents/tasks/M115_pcg.md).

Turn it on with `模擬 -> 機種の構成 -> PCG-8100/8800 (V1/V2)` (`PCG=true`
in the configuration); its tone volume is `音 -> 個別の音量 -> PCG`
(`PCG_vol`).

## Ports (OUT)

| Port | Meaning |
|---|---|
| 00h | data |
| 01h | address bits 7-0 |
| 02h | address bits 15-8; bits 3, 6, 7 enable the tone of counters 0, 1, 2 |
| 03h | bits 1-0: codes 00h-7Fh shown from PCG (bit 1) bank (bit 0); bits 3-2: the same for 80h-FFh; bit 4: bank written |
| 0Ch-0Eh | 8253 counters 0-2 |
| 0Fh | 8253 mode word |

Whenever address bit 12 is set, a write to 00h-02h stores the data, or
with address bit 13 the ROM's byte, at address bits 9-0 (character
`(code & 7Fh)`, row 0-7) of the bank written. Both banks start as the ROM's
codes 80h-FFh, and codes 80h-FFh are shown from bank 0.

A counter sounds a square wave of 4 MHz / count while its enable bit is
set; counts up to 100h are silent and 0 counts 65536.

## Source

Rewritten in C from X88000 1.5.3 by Manuke (public domain): `PC88Pcg.cpp`,
`X88000.cpp` (`PcgSoundOutput`) and `PC88Z80Main.cpp` (port map); see
ADR-0016 "Derivations". One deviation: an 8253 MSB-only load sets only the
MSB.
