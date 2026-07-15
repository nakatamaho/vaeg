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
# M36: Vendor the approved Z80 core and add standalone conformance

Status: draft; blocked until the maintainer explicitly passes G35

Branch: `topic/m36-z80-vendor-conformance`

## Goal and boundaries

Vendor only the immutable MIT commit approved at G35 under
`external/suzukiplan-z80/` and prove it independently. Do not write the vaeg
wrapper, connect it to the emulator, change the selected core, migrate state,
replace disassembly, or delete legacy files.

## Vendoring and notices

Import only required source/license files without hand edits. Add `VERSION`
recording upstream and fork repositories, upstream base SHA, exact vendored
SHA, issue/PR, tag, MIT license, and extension summary. Preserve `LICENSE.txt`
verbatim. Update the third-party notice and ADR, distinguishing upstream/fork
MIT code, vaeg BSD-2-Clause code, and external GPL test inputs. A later source
change requires re-vendoring another immutable commit in its own commit.

## Standalone validation

Add target-local compile tests for normal callbacks, `Z80_NO_FUNCTIONAL`,
debug, and supported release-oriented feature switches on Linux GCC/Clang,
macOS, and Windows-MinGW. Mirror the acceptance callback, level IRQ, raw IM0,
IM1/2, EI, HALT, and NMI tests required at G35 so a bad import cannot pass.

Add a deterministic headless CP/M-style runner under `tests/z80/`: 64 KiB,
load address `0x0100`, sufficient BDOS call-5 output handling, bounded timeout,
clear pass/fail parsing, and diagnostic state on failure. Run ZEXDOC and ZEXALL
from the immutable M34 URLs. The dedicated CI job must verify all M34 SHA-256
values before execution, support a user-provided offline cache, and prove no
fetched `.cim`, `.src`, or license artifact enters source/release archives.
Do not link GPL test inputs into vaeg.

## Checks and G36

Run repository invariants, all existing builds/CTest/smoke, three-platform
compile tests, focused interrupt tests, both ZEX programs, and a release-
archive exclusion test. G36 is machine plus maintainer review and requires the
emulator still to use the legacy core. Push, report exact vendored hashes,
commands, results, changed files, and SHAs, then stop.
