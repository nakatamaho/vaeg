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

# CPU conformance checks

This page moved. The CPU checks and their boundaries are documented in:

- [docs/modernization/qa/README.md](qa/README.md): the three QA levels and
  the oracle order;
- [docs/modernization/qa/upd9002-native.md](qa/upd9002-native.md): the
  uPD9002 native mode against the SingleStepTests V20 corpus;
- [docs/modernization/qa/z80-family.md](qa/z80-family.md): the uPD9002
  uPD70008-compatible mode and the FDC uPD780C.

[M159 mode-transition coverage](upd9002-upd70008-mode.md#21-the-md-flag)
checks native PUSHF/interrupt high bits, compatible CALLN and interrupt
frames, a nested native IRET, same-SP MD=1 and relocated MD=0 returns,
RETEM, and DF across a saved-state round trip.

[M162 direction and FLAGS coverage](upd9002-upd70008-mode.md#direction-flag-and-live-flags-m162)
adds both DF seeds for all 16 compatible block opcodes, saved DF at a
mode-switching IRQ between repeat iterations, and the distinction between
native FLAGS bit 1 and saved PSW bit 1. DFPROBE v4 reported successful
DF=1 entry for all 12 BRKEM/BRKEM2 variants on both VA models, including a
forward-fetch control. The v3 stall remains unexplained. Low block I/O
flags remain an open conformance gap, separate from block direction.
