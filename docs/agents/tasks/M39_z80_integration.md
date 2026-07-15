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
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# M39: Integrate an opt-in replacement Z80 subsystem

Status: draft; blocked until the maintainer explicitly passes G38

Branch: `topic/m39-z80-integration`

## Goal and build selection

Connect the tested wrapper to the real subsystem while retaining legacy as the
default development fallback for this milestone only. Add target-local
`VAEG_Z80_CORE=legacy|suzukiplan`; never compile both implementations into one
production target. Both choices must build on Linux, macOS, and Windows-MinGW.
Do not flip the default, delete legacy files, replace disassembly, or alter
unrelated FDC, 8255, DMA, main CPU, sound, display, or pacing behavior.

Migrate `iova/subsystem.cpp` only at the approved seam. Preserve the class and
used signatures, eight-bit external ports, acceptance-time acknowledge,
level IRQ, clock behavior, and the callback-stale/return-fresh register mirror.

## State and sleep evidence

All saves occur only after the frontend/core frame call and `Z80C::Exec()`
return. Test same-build-family legacy fixtures and a legal old-emulator save
loading under new, new-to-new round trips, ordinary/HALT/external-WAIT/IRQ/EI
states, reachable zero and negative balance, nonzero last clock, and FDD
activity logically in progress across frame calls. Test a synthetic positive
revision-1 field only as codec coverage; current production `Exec()` does not
return positive credit. Do not save inside execution or callbacks.

Use bounded private trace instrumentation from M34 to record callback mirror
PC, live/return PC, sleep assertion, ATN wake, WAIT release, and IM0 bytes only
at actual acceptance. Do not change `0x1732` or `0x700e` unless trace evidence
proves preservation impossible; any correction needs focused tests and the
bug-fix ledger.

Create `docs/modernization/z80-integration.md` with selection commands,
normalized old/new traces, state results, clock deltas, sleep evidence, and
unresolved differences. Never commit private ROM/disk data or logs containing
it.

## QA and human gate G39

Public: invariants, both three-platform builds, all CTest/smoke, wrapper/ZEX,
and bounded differential tests. Maintainer-local for both selections: clean
VA/V3 boot, bundled demo, OS operations, FDD read/write, timing-sensitive disk,
normal VA idle, Sorcerian idle, ATN/8255 wake, legacy-save-to-new load during
FDD activity, repeated reset, and model changes. G39 requires explicit
maintainer confirmation under `suzukiplan`; build/ZEX alone is insufficient.
Push and report exact SHAs/results, then stop.
