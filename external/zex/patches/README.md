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

# M140 FDC ZEX patch series

The stock files in `external/zex/` remain byte-identical to the pinned upstream
in [provenance.txt](../provenance.txt). These are **derived GPL-2.0-or-later
patches**, preserving the original Frank D. Cringle/J.G. Harston notices;
the diagnostic build selects the GPLv3 option supplied in [LICENSE.txt](../LICENSE.txt).
This README and JSON integrity metadata are vaeg-authored BSD-2-Clause material.
No ROM, private CP/M system file or private measurement is included here.

## Reproduction order

1. Verify `zexall.src` at SHA-256
   `a263efc67ed6f890268c6f9e00f7911d9376a6bc6ddaec5ce04e33a5f483733c`.
2. Translate the unmodified source with `translate_zmac()` in
   `tools/cpmva/zex/zexbuild.py`, the existing deterministic ZMAC/MAXAM to
   z80asm-1.8 syntax conversion. The translated input hash is pinned in
   `m140-fdc.json`. These patches apply **after translation**, not directly to
   `zexall.src`; their `a/` and `b/` names describe generated files only.
3. Apply `m140-fdc-zexall.patch`, with exact context and no fuzz. It is the
   complete full-run adaptation, including the monitor shim. It preserves
   all 67 groups/opcodes, flag masks and increment/shift vectors.
4. **Only for FDCONE smoke**, apply `m140-fdc-neg-smoke.patch` to that full
   output. It explicitly replaces the test list with NEG alone and changes
   the banner. It is never a full-suite result.
5. Assemble with pinned z80asm 1.8. The builder verifies patch bytes, each
   input and output against the SHA-256 chain in `m140-fdc.json`. It writes
   generated ASM/BIN/COM outside Git. Original vendor files are never edited.

The full patch changes ORG to 4000h, places a bounded stack at 7EFFh, disables
asynchronous interrupts, stops motors and clears the monitor's motor latch,
uses monitor output/return instead of CP/M BDOS/warm boot, and prints every
actual CRC beside its stock expectation. In addition to address relocation,
the `LD r,n` descriptor's base HL changes **0339h → 7339h** so `LD (HL),n`
does not write FDC ROM. This changes the CRC contract; stock-address CRC
mismatches alone do not demonstrate NEC behavior. No masks or scan vectors
are weakened to obtain PASS.

Existing `M139` strings are legacy v1 diagnostic/evidence identifiers from
before the maintainer reassigned the work to M140. They are deliberately
preserved so extracting this patch series produces **identical program bytes**.
Neither the prefix nor successful host execution identifies physical silicon.

Run [the M140 builder/tests](../../../tools/hardware/upd780/README.md), which
use these files directly rather than maintaining another substitution recipe.
See [M140](../../../docs/agents/tasks/M140_fdc_upd780_conformance.md) and
[monitor ABI](../../../docs/bios/fdc_monitor.md) for safety and acceptance.
Patches/diagnostics do not enter the vaeg application or public release archives.
The original stock and CP/MVA builders remain separate and unchanged.
