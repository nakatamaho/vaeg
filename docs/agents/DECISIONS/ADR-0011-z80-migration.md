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
# ADR-0011: Replace the M88-derived Z80 subsystem through a gated migration

## Status

Accepted at G34. The M35 extension is technically verified, but G35 remains
blocked until its format patch is published as an approved immutable upstream
or fork commit and the full-suite dual-callback test wording is resolved. M36
remains forbidden until the maintainer passes G35.

## Decision

Use `suzukiplan/z80` release `1.10.0`, commit
`e3926769a790fab0af1c34a5540e317f8d4f0ddc`, as the replacement base. The
upstream repository is `https://github.com/suzukiplan/z80`; its code is MIT
licensed, Copyright (c) 2019 Yoji Suzuki. At the inspected revision:

| Item | SHA-256 |
|---|---|
| `LICENSE.txt` | `ca7261ecf96ab7fea40c4c66aeb644710d210bd71418d285a9dd0098a7bddff1` |
| `z80.hpp` | `4d2f76f6fae1a4650d4392d2bce067384716e3206b61891855953a4edb89467e` |

MIT-licensed third-party code is acceptable. Vendored upstream or fork code
retains its MIT notice; vaeg-owned new files use BSD-2-Clause. C++17 is
permitted only for approved CPU backends and thin adapters under `cpucva/`.
No C++ STL or third-party type may cross the consumer-visible subsystem,
debugger, C bridge, or state-save interfaces. Other new emulator-core code
remains C99 unless separately approved.

Development is upstream-first. M35 must propose a general-purpose minimal
patch against the exact base above. If it is not merged, the approved fallback
is an immutable, accessible, MIT-licensed fork commit containing only the
required extension and tests. A vendored copy must never be hand-edited.

## Legacy provenance and deletion boundary

The active source and compiler-dependency inventories find exactly these seven
M88/cisc-derived Z80 files:

```text
cpucva/types.h
cpucva/z80.h
cpucva/z80if.h
cpucva/z80c.h
cpucva/z80c.cpp
cpucva/z80diag.h
cpucva/z80diag.cpp
```

This is the final approved deletion list. M34 does not delete any file. No
additional active M88-derived Z80 file was found. Frozen reference consumers
remain untouched and are not an active build obligation.

## Consumer-visible contract

Keep the class name `Z80C` and source-compatible signatures for every method
used by the active tree: constructor/destructor, `Init`, `Exec`, `Reset`,
`IRQ`, `Wait`, `GetStatusSize`, `SaveStatus`, `LoadStatus`, `GetReg`, and the
temporary `GetDiag()->Disassemble` bridge. Retain `GetPC`, `SetPC`, and `NMI`
as required architectural services for the replacement contract even though
no active external caller was found. `GetDiag` may be removed after M40 moves
the bridge. The public `TestIntr`, `GetWaits`, `IsIntr`, dump controls, and
statistics API have no active consumer and are not required after cutover.

Independently author the replacement contracts. Familiar names may be kept to
minimize consumer edits, but no M88 comments, layout, macro collection, or
implementation structure may be copied. Use these vaeg-owned component names:

```text
cpucva/z80_bus.h
cpucva/z80_registers.h
cpucva/z80_legacy_state.h
cpucva/z80_legacy_state.cpp
cpucva/z80_core.h
cpucva/z80_core.cpp
cpucva/z80_disasm.h
cpucva/z80_disasm.cpp
```

`z80_bus.h` owns fixed-width memory, eight-bit external I/O, clock, and clock-
counter contracts. `z80_registers.h` owns the vaeg public mirror. The codec
owns the revision-1 byte image; the runtime must not use that compiler-shaped
image as its internal state. `z80_core.*` owns `Z80C` and the third-party
adapter. `z80_disasm.*` is introduced only in M40.

The public mirror returned by `GetReg()` is refreshed when `Exec()` returns,
as today. It is intentionally stale within callbacks. `GetPC()` and state save
use authoritative live state. State save materializes architectural flags.

## Execution, bus, and interrupt contract

`Exec()` adds unsigned `now - lastclock` to the signed remaining-clock value.
If external WAIT is set, it consumes all positive credit without executing.
Otherwise it retires complete instructions while credit is positive; the final
instruction can overshoot into zero or a negative debt. It then refreshes the
public mirror and stores `lastclock`. All memory addresses wrap at 16 bits.
Every external IN, OUT, and block-I/O port is passed to vaeg as `port & 0xff`.

IRQ is an external level: assertion persists until vaeg deasserts it. The
acknowledge callback is invoked exactly once when a maskable interrupt is
actually accepted, after the current instruction and EI inhibition, and never
for assertion alone or NMI. IM1 reads and ignores the byte; IM2 uses it as the
vector low byte. IM0 treats it as the raw first opcode: it is not fetched from
memory and does not advance PC, while all operands or following prefix bytes
are fetched normally from current PC. The focused practical acceptance set is
the reset value `0x7f`, arbitrary values written by subsystem port `0xf0`,
RST-family values, and `0x00`. A maintainable normal-decoder injection supports
multi-byte and prefixed values too; test CB, ED, DD, FD, DDCB, and FDCB in M35.
No unsupported byte may silently become a no-op or fixed RST.

## Required M35 third-party extension

The inspected core's `generateIRQ()` is a one-shot latch, its IM0 path calls
`RST(interruptVector)` rather than decoding a raw opcode, and it has no
acknowledge callback. Therefore the unmodified revision cannot implement the
approved contract. M35 must add, in upstream style:

```text
setIRQLine(bool asserted)
an unsigned-eight-bit interrupt-acknowledge callback
raw first-opcode dispatch through the normal decoder at IM0 acceptance
```

The existing one-shot APIs must retain their semantics. The new level state
must be inspectable/restorable, directly or through the setter. The callback
must work with both normal `std::function` and `Z80_NO_FUNCTIONAL` function-
pointer configurations. IM0/IM1/IM2 callback timing, EI inhibition,
deassertion, persistent assertion, HALT wake, raw multi-byte/prefix decode, and
NMI non-acknowledge need focused upstream tests.

The inspected public register state already exposes the architectural
registers, I/R, IFF, IM, HALT flag, and `execEI`; no broader import/export API
is needed. Current vaeg has synchronous NMI and no pending-NMI caller or saved
condition, so no pending NMI state is required.

## M35 verified extension result

The focused extension was implemented and tested from the exact selected base
as local commit `b4a0a5a238fecc280781e6fe5719faf0eafcd667`. It adds
`setIRQLine`, `isIRQLineAsserted`, an interrupt-acknowledge callback for both
callback configurations, and normal-decoder IM0 injection. It retains the
one-shot behavior for existing users that do not opt into the new APIs. The
level line is inspectable and restorable, so M35 found no need for a broader
state API.

The MIT license blob is unchanged. M35 also corrected this ADR's former
copyright claim: this file previously said `2021-2023`, while the selected
base's `LICENSE.txt:3` and `z80.hpp:6` both say `Copyright (c) 2019 Yoji
Suzuki`. This documentary correction does not change the license decision.

An accessible maintainer fork was unavailable, so the directly applicable
[M35 format patch](../reports/m35_suzukiplan_irq_extension.patch) is the
approved fallback artifact for review. Its SHA-256 is
`d8624085139ef4e7b400b918b2b498e79bea1af4a1942e4ac935545846e746a4`.
The complete provenance, source references, and test record are in the
[M35 evidence report](../reports/m35_suzukiplan_irq_extension.md). G35 cannot
pass until the maintainer supplies or approves an immutable accessible commit.

M35 also found that the existing `test/` suite cannot be compiled wholesale
with `Z80_NO_FUNCTIONAL`: `test/test-checkreg-on-callback.cpp:88` passes a
capturing lambda to the function-pointer constructor. The exact selected base
fails identically before reaching any extension code. Focused extension tests
pass in both configurations, the complete existing `test/` suite passes in
its normal configuration, and the upstream ZEX harness passes in its declared
`Z80_NO_FUNCTIONAL` configuration. If G35's wording requires every legacy
test harness to run in both configurations, that condition is not met and
requires a separately approved harness-port expansion; it must not be reported
as green silently.

## Frame-boundary revision-1 state

Production save is initiated by the GUI only after `pccore_exec()` returns.
At the Z80 section callback, no `Z80C::Exec()` or Z80 callback is active and a
complete instruction boundary has been reached. Preserve that scheduler; do
not serialize instruction- or callback-internal microstate.

The same-build-family revision-1 target is the existing 68-byte image on the
verified little-endian 64-bit GCC/Clang/MinGW-family layouts. Decode and encode
it by explicit offsets. Map architectural registers, I/R, IM, IFF1/IFF2,
HALT, external WAIT, IRQ level, `remainclock`, and `lastclock`. Translate HALT
PC explicitly: the legacy core stores the HALT opcode address, while the
selected core's PC is after HALT. Legacy-to-new adds one modulo 16 bits;
new-to-revision-1 subtracts one while halted.

The selected core can return from its execution call with `execEI` set when
the slice ends immediately after EI. The legacy image has no named EI field,
but its saved `wait` byte uses only bits 0 (HALT) and 1 (external WAIT). Reserve
bit 2 in new-core revision-1 images for reachable EI inhibition. Old fixtures
have it clear; new-to-new round trips retain it. This does not promise that a
new-core image loads in an old release. M37 must prove this focused mapping on
every supported build family before integration.

The legacy implementation never returns positive `remainclock` from normal
`Exec()` or its external-WAIT branch with the current positive multiplier: it
runs or drains until the value is non-positive. Zero and negative values are
production-reachable and must round trip. This corrects the contrary
positive/zero/negative assertion in the M34 task and migration master; it does
not authorize a scheduler change. A codec should still preserve any signed
32-bit input byte-for-byte, including positive values, for defensive legacy
compatibility tests.

## ZEX acquisition

Use the candidate commit's `test-ex` artifacts only in dedicated conformance
CI or from an offline user cache. Fetch by immutable raw GitHub URL, verify
SHA-256, and never put the fetched artifacts in source or release archives.
Record and make available the corresponding source and license:

| File | SHA-256 |
|---|---|
| `zexdoc.cim` | `b3015112a99bb72273e0cacde7c7549eb9840ba996af76f7bf7992ef7d6e2f90` |
| `zexall.cim` | `fbb1bb5d46f61c33ea6841a71f2b23c49b9b62410ce6ed4e57b7d9b2e7b437e0` |
| `zexdoc.src` | `0e2e7d05e5dd27c932de64d4c3711351f53388ed02d2e99e2e706ef6216ca9b3` |
| `zexall.src` | `a263efc67ed6f890268c6f9e00f7911d9376a6bc6ddaec5ce04e33a5f483733c` |
| `test-ex/LICENSE.txt` | `e57f1c320b8cf8798a7d2ff83a6f9e06a33a03585f6e065fea97f1d86db84052` |

The `.src` notices identify Frank D. Cringle (1994), modifications by J.G.
Harston (2002), and GPL-2.0-or-later. The bundled license text is GPLv3; retain
both notices in acquisition metadata. These programs are executed as separate
test inputs, not linked into vaeg. The policy is lawful and reproducible.

## Disassembler and release policy

The selected revision has execution-time debug strings but no public,
side-effect-free reusable decoder. M40 will independently author a vaeg BSD-2-
Clause disassembler from documented Z80 encodings. It must not copy M88/cisc
or GPL opcode tables.

During M39 and M40 only, explicit build selection may keep legacy and new cores
for differential and private-system evidence; legacy remains the default.
M41 has no fallback: the new core becomes the only production path, the build
switch is removed, the seven approved files are deleted, and source/release
archives are audited for their absence. Historical Git objects are out of
scope.

## M35 feasibility recommendation

**TECHNICAL GO; HOLD AT G35.** The focused extension meets the contract and is
suitable for a minimal MIT fork. Do not start M36 until the format patch has an
approved immutable accessible commit, the full-suite dual-callback wording is
resolved, and the maintainer explicitly passes G35.

| Area | Finding |
|---|---|
| Interrupt contract | Feasible only with the focused level-line and acceptance callback extension. |
| State import/export | Feasible from public register state plus restorable level line; no broad state API is needed. |
| Revision-1 save | Feasible at the verified frame boundary with an explicit 68-byte codec, HALT-PC translation, and reserved EI bit. |
| Practical IM0 | Feasible by routing the supplied first byte through the normal decoder; current upstream RST-only handling is insufficient. |
| Disassembler | Feasible as an independent BSD-2-Clause vaeg component in M40. |
| Licensing/provenance | MIT base/fork and BSD-2-Clause vaeg adapter are compatible and precisely pinnable. |
| Test acquisition | ZEXDOC/ZEXALL have an immutable, hash-verified GPL test-input path with source and offline cache support. |

## Consequences and unresolved risks

- `xf` is not initialized by legacy `Reset()` but influences saved AF and is
  serialized. Fixtures deliberately execute `XOR A` first. This is a legacy
  defect, not a behavior contract; it remains open and is not fixed by M34.
- Genuine revision-1 images are compiler-shaped. Local native GCC/Clang and
  cross-target layout probes agree, but Windows and macOS CI must execute the
  fixture target before M37 freezes per-family codecs.
- Actual private-ROM IM0 acceptance bytes are not yet observed. M39 must log
  only at real acceptance; static code permits every byte.
- SLEEP_HACK depends on the stale callback-time `GetReg()->pc` mirror and
  requires private VA, Sorcerian, and ATN wake testing at G39.
- The M34 task/master claim that positive remaining credit can be present
  after `Exec()` conflicts with current code; the verified zero/negative
  boundary governs implementation.
- The migration master says M33 is in the roadmap, but current `ROADMAP.md`
  has no M33 row. M33 commits exist in history. M34 leaves that completed work
  and identifier untouched rather than inventing an entry.
