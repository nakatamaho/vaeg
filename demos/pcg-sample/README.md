<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# PCG-8100/8800 sample

[`pcg.bas`](pcg.bas) defines four user characters with the PCG-8100
(PC-8001) or PCG-8800 (PC-8801), shows them, and plays a scale on 8253
counter 0 while a two-character figure walks across the screen. It runs in
N-BASIC and in N88-BASIC (V1/V2), in vaeg (V1/V2 or N mode, or a PC-8801 /
PC-8001 machine, with 模擬 -> 機種の構成 -> PCG-8100/8800 (V1/V2) turned
on; see [PCG](../../docs/modernization/pcg.md)) and on hardware with a
PCG board.

- Lines 100-120 write the characters 80h-83h: data on port 00h, address on
  01h/02h, then address bit 12 (port 02h bit 4) set in one write and
  cleared in the next. Leaving bit 12 set while the next data and address
  are written works on QUASI88 and vaeg but stores each row into the
  previous address on emulators that write on every port write.
- Line 140 sets counter 0 to mode 3 (square wave) and opens its gate (port
  02h bit 3); lines 150-160 load each note's count (about 3.9936 MHz /
  frequency); line 190 closes the gate.
- The DATA lines are the four 8x8 characters (a figure in two halves, a
  heart, a face) and the counts of C5 to C6.

Paste the file into BASIC (編集 -> 貼り付け) and type `RUN`.
