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

Accepted through G38; M39 is implemented but its private-system gate is
pending. The maintainer
approved the reproducible downstream patch and documented callback test matrix
as sufficient M35 provenance on 2026-07-15. M36 reproduced and vendored the
approved tree, G36 passed, and the maintainer explicitly authorized M37. The
standalone M37 implementation and all configured hosted-platform jobs passed.
M38 has a deterministic old/new harness. Its FDD-visible slice shift remains
documented, and the maintainer-approved eventual-convergence test proves that
the external effect is delayed but not lost, duplicated, reordered, or
permanently divergent. G38 passed under the revised clock policy. M39 now has
an opt-in production build and public ROM-less evidence, but no private boot,
FDD, SLEEP_HACK, WAIT-wake, or active-transfer state result is claimed.

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

Development remains upstream-first where practical. For M35, the maintainer
approved a reproducible downstream patch against the exact base above as
sufficient provenance; an immutable public upstream or fork commit is not
required. A vendored copy must never be hand-edited. Before vendoring, M36 must
apply the approved patch to the approved base, reproduce the expected tree,
and record the base SHA, patch SHA-256, and resulting tree hash in the vendored
provenance file.

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

The directly applicable
[M35 format patch](../reports/m35_suzukiplan_irq_extension.patch) is the
approved downstream provenance artifact. Its SHA-256 is
`d8624085139ef4e7b400b918b2b498e79bea1af4a1942e4ac935545846e746a4`.
The complete provenance, source references, and test record are in the
[M35 evidence report](../reports/m35_suzukiplan_irq_extension.md). A clean
`git am` from base `e3926769a790fab0af1c34a5540e317f8d4f0ddc` reproduced tested
commit `b4a0a5a238fecc280781e6fe5719faf0eafcd667`'s tree
`8a606eb39332a6e79b69bb62d9dedca042b923dc`; MIT verification passed.

M35 also found that the existing `test/` suite cannot be compiled wholesale
with `Z80_NO_FUNCTIONAL`: `test/test-checkreg-on-callback.cpp:88` passes a
capturing lambda to the function-pointer constructor. The exact selected base
fails identically before reaching any extension code. Focused extension tests
pass in both configurations, the complete existing `test/` suite passes in
its normal configuration, and the upstream ZEX harness passes in its declared
`Z80_NO_FUNCTIONAL` configuration. The maintainer accepted this matrix for
G35: baseline-incompatible legacy tests need not be rewritten when the
limitation is reproduced and documented.

## M36 verified vendoring and standalone conformance

M36 cloned the public upstream repository into a clean temporary checkout,
checked out base `e3926769a790fab0af1c34a5540e317f8d4f0ddc`, computed the approved
patch SHA-256, and applied it with `git am`. A plain application created a new
commit ID because Git used the current committer timestamp, but its tree was
the required `8a606eb39332a6e79b69bb62d9dedca042b923dc`. Repeating the clean
application with `git am --committer-date-is-author-date` reproduced historical
tested commit `b4a0a5a238fecc280781e6fe5719faf0eafcd667` exactly. Thus the approved
commit is reproducible when commit metadata is deterministic, while the tree
is the time-independent vendoring identity.

The only copied upstream files are `LICENSE.txt`, `z80.hpp`, and
`test/test-interrupt-extension.cpp`. Their Git blob IDs are respectively
`a4cbbf62b0edaf761ef48556c7a2e50bb3b4817f`,
`b4e1f9d357ff4076c9e4c2a9cb4b8ae64d273865`, and
`3708c77fabbfef85bd68da99fc7cef068af4ec55`. Byte comparisons against the
reproduced tree pass. `provenance.txt` is vaeg-authored provenance, not a
modified upstream source. It was initially named `VERSION`, but the first
hosted macOS build proved that this path shadows libc++'s `<version>` header on
the runner's case-insensitive filesystem: Apple Clang diagnosed the provenance
comments at the former `external/suzukiplan-z80/VERSION:3`, `:4`, and `:5` as
invalid preprocessing directives. Renaming only the vaeg-authored metadata
removed that collision without changing an upstream byte. The MIT license
remains byte-identical with SHA-256
`ca7261ecf96ab7fea40c4c66aeb644710d210bd71418d285a9dd0098a7bddff1`.

The vendored header is compiled only by standalone test targets. Four target-
local configurations cover normal callbacks, `Z80_NO_FUNCTIONAL`, the default
debug API, and release-oriented disabled debug/breakpoint/nest-check/exception
features. The focused 21-case interrupt extension runs in normal and
`Z80_NO_FUNCTIONAL` configurations. The raw core is not present in
`VAEG_CORE_SOURCES`, and the production emulator continues to compile
`cpucva/z80c.cpp` and `cpucva/z80diag.cpp`.

The vaeg-owned ZEX runner uses deterministic 64 KiB memory, loads at `0x0100`,
provides the required CP/M CALL-5 services, and enforces emulated-clock and
wall-clock limits. Its acquisition helper pins the approved base URLs and
verifies the two `.cim` files, two GPL `.src` files, and license. These external
inputs remain outside Git and release packages. Archive checks reject both
their expected names and their content hashes.

## M37 standalone wrapper and revision-1 codec result

M37 added the independently authored `z80_bus.h`, `z80_registers.h`,
`z80_legacy_state.*`, and `z80_core.*` components named above. The public
surface retains the used `Z80C` constructor, destructor, `Init`, `Exec`,
`Reset`, `IRQ`, `Wait`, status methods, `GetReg`, and placeholder `GetDiag`,
plus the approved architectural `GetPC`, `SetPC`, and `NMI`. `TestIntr`,
`GetWaits`, `IsIntr`, dump controls, and statistics were not reproduced. No
STL or third-party type crosses the consumer-visible headers.

The wrapper owns a public register mirror separate from the authoritative
third-party register state. `GetReg()` returns that mirror; `Exec()` refreshes
it only on return, while `GetPC()` and status save use the authoritative live
PC. A synthetic port callback at the production SLEEP_HACK PC proves that the
callback sees the old mirror PC while `GetPC()` sees the advanced PC. Memory,
I/O, level IRQ, acceptance acknowledge, and fine-grained clock callbacks are
adapted without a wrapper-side interrupt approximation. NMI remains the
synchronous vaeg operation and copies IFF1 to IFF2 before clearing IFF1.

The revision-1 codec is an explicit 68-byte little-endian representation. It
does not reinterpret an input buffer as runtime state. Its offsets remain AF
0, HL 4, DE 8, BC 12, IX 16, IY 20, SP 24, alternate pairs 28/32/36/40, PC 44,
I/R-low/R7/IM/IFF1/IFF2 at 48/49/50/51/52/53, IRQ 56, wait 57, `xf` 58,
revision 59, remaining clock 60, and last clock 64. All 15 retained M34 images
load. HALT load adds one to the legacy opcode PC and save subtracts one from
the authoritative post-HALT PC. Architectural AF is authoritative; load
ignores the legacy lazy-flag `xf`, and save writes deterministic F bits 3 and
5. The ordinary retained fixture therefore normalizes its historical `xf`
byte rather than reproducing uninitialized state.

Every retained fixture has wait bit 2 clear. A clear bit imports no EI
inhibition; a new boundary immediately after EI exports `execEI` in bit 2.
Reloading that image executes the required following instruction before an
asserted level IRQ is accepted. HALT, external WAIT, and EI occupy independent
bits 0, 1, and 2. The focused continuation test produces the same next
instruction, memory/I/O trace, architectural result, status image, and clock
balance before and after reload. IRQ assertion is inspectable in the image,
restored through the approved core setter, accepted as a level, and cleared
from later images only when vaeg deasserts it.

Two standalone libraries compile the wrapper with normal callbacks and
`Z80_NO_FUNCTIONAL`; both use target-local release-oriented core definitions.
The wrapper unit suite passes in both configurations. The separate wrapper
ZEX runner passes ZEXDOC and ZEXALL. These libraries are not members of
`VAEG_CORE_SOURCES`: the emulator still builds `cpucva/z80c.cpp`, and no
production selection or guest-visible path changed in M37.

## M38 differential result and passed gate

M38 uses three standalone processes: the legacy runner, the new-wrapper
runner, and a comparator. Both runners consume the same compiled scenario,
memory-patch, IRQ/NMI/WAIT, input, acknowledge, save/load, and clock-slice
descriptions. They emit `vaeg-z80-trace-v1` records with fixed lower-case
fields, full and selected memory hashes, ordered memory/I/O/acknowledge/NMI
events, callback-time live/public PC, authoritative state, clock balance, and
normalized checkpoint reasons. The comparator never assumes that an old
private step equals one new instruction; EI intermediate returns are retained
for diagnostics but compared only at the next declared normalized boundary.

The harness exposed third-party behaviors that the vaeg adapter can correct
without editing vendored bytes. The adapter now counts the second M1 fetch for
CB/ED/DD/FD prefixes, including raw IM0 prefixes, and counts HALT idle M1
cycles. It restores IFF1 from IFF2 after RETN/RETI and materializes the defined
S/Z/5/H/3/PV/N flag result after `LD A,I` and `LD A,R`. Its synchronous NMI
still updates architectural state immediately but retains the legacy stale
public PC until `Exec()` refreshes the mirror. These are instruction-boundary
adapter fields only; they are inactive at the approved frame save boundary
and are not added to revision-1 state.

The normal comparison has evidence-backed exact classifications for the
legacy HALT-PC/fetch representation, low/high stack write order, skipped
untaken-branch displacement read, lazy-`xf` exchange result, fused-EI
deassertion behavior, missing interrupt-acknowledge R increments, block
memory/I/O undocumented flags, and EI save-boundary event placement. The
complete identifiers and minimal bytes are in the
[M38 evidence report](../reports/m38_z80_differential.md). There is no broad
opcode allowlist and the generated corpus matches without any allowlist.

The slice-exact evidence remains unallowlisted. With bytes
`18 02 00 00 d3 f4`, initial A=`0x5a`, and slices `1,7`, legacy's 7-clock taken
JR reaches and writes FDD control port `0xf4` in the second `Exec()`; the new
core's architectural 12-clock JR leaves it at `0x0004` with no write. This
remains a reproducible FDD-visible slice assignment difference and is not
relabelled as externally inert.

The approved convergence continuation gives both runners the same additional
`4,1` clock slices. At 13 total clocks each trace contains the same ordered
reads and exactly one `OUT (0xf4),0x5a`; AF, BC, DE, HL, IX, IY, SP, PC,
alternate registers, I/R, interrupt state, memory hashes, device-visible
values, and `lastclock` agree. Only the classified architectural cycle count,
remaining balance, and earlier slice assignment differ.

G38 blocks on lost, duplicated, reordered, or permanently divergent external
effects. A slice-boundary shift caused by a verified architectural timing
correction may pass when eventual convergence is proven. Every such shift is
still a mandatory M39 private integration risk. M39 must stop production
integration if real VA/FDD testing finds a transfer failure, timeout,
corrupted data, changed IRQ/DRQ sequencing, or broken save/load behavior.
M38 adds no legacy per-opcode timing emulation. G38 passed and work remains
subject to the M39 private-system gate.

## M39 production integration

M39 adds one build-time CMake cache selection with exactly
`VAEG_Z80_CORE=legacy|suzukiplan`; `legacy` remains the default. The
production `vaeg_va` target compiles `cpucva/z80c.cpp` only for `legacy`, or
`cpucva/z80_core.cpp` plus `cpucva/z80_legacy_state.cpp` only for
`suzukiplan`. It never links both `Z80C` implementations, and there is no
runtime toggle. The vendored tree and approved M35 patch remain byte-unchanged.

The C subsystem seam and consumer signatures remain stable. The new selection
uses the same memory, eight-bit port, clock, level IRQ, and acceptance-time
acknowledge interfaces. Because M40 is not authorized, both selections still
compile the legacy `z80diag.cpp`; the new selection calls it through an
independent function-pointer memory bridge in a separate translation unit.
No M88 declaration, type, or disassembler object crosses the new wrapper's
consumer-visible interface.

The production state bridge now propagates `SaveStatus()` and `LoadStatus()`
failure through `statsave.c`. A copied valid state with only embedded Z80
revision changed to 2 is rejected by the top-level load path. Successful load
also refreshes the subsystem's diagnostic WAIT mirror from revision-1 byte 57
bit 1; the restored CPU state remains authoritative. The status size, offsets,
HALT translation, external IRQ level, EI bit 2, and frame-call boundary are
unchanged.

The first M39 MinGW build of the suzukiplan production selection exposed a
portable-adapter defect: `iova/subsystem.cpp` still obtained the non-standard
`uint` alias transitively from the legacy header, while the independently
authored wrapper deliberately does not define that alias. The minimal
reproducer was the normal `mingw-cross` production build with
`VAEG_Z80_CORE=suzukiplan`, which failed before linking at the `Subsystem`
memory and I/O overrides. Those adapter signatures and helpers now use
`std::uint32_t`, matching `z80_bus.h`; both MinGW selections build and execute
their ROM-less integration selftests under Wine. No vendored or legacy-core
source was changed.

ROM-less tests under both production selections preserve the VA and Sorcerian
SLEEP_HACK constants. At the callback, live PC is after `IN (0xfe)` while the
public mirror still contains `0x1732` or `0x700e`; at `Exec()` return the
mirror catches up. WAIT drains clocks, ATN releases it, and execution resumes.
The focused synthetic EI-before-VA-IN case confirms the M38 scheduling
hypothesis under the new core without changing a magic constant. The tests
also require exactly one production `OUT (0xf4),0x5a` across a state boundary.

These results do not discharge the M38 timing risk. The committed private
manifest requires separate legacy/new VA/V3 boot, FDD read/write and
timing-sensitive loaders, IRQ/DRQ/`0xf4` evidence, actual VA and Sorcerian
sleep/wake paths, and legacy-to-new/new-to-new state during logical FDD
activity. Missing or reordered events, timeout, corruption, changed IRQ/DRQ,
failed sleep/wake, or altered completion after load blocks G39. The default
cannot change until the maintainer explicitly passes that gate.

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

## M35 feasibility recommendation (historical G35 output)

**G35 PASSED; M36 WAS FEASIBLE.** The focused extension, approved downstream
patch provenance, license verification, and accepted test matrix met G35. The
maintainer subsequently authorized M36; this paragraph is retained as the
historical G35 recommendation rather than a current authorization statement.

| Area | Finding |
|---|---|
| Interrupt contract | Feasible only with the focused level-line and acceptance callback extension. |
| State import/export | Feasible from public register state plus restorable level line; no broad state API is needed. |
| Revision-1 save | Feasible at the verified frame boundary with an explicit 68-byte codec, HALT-PC translation, and reserved EI bit. |
| Practical IM0 | Feasible by routing the supplied first byte through the normal decoder; current upstream RST-only handling is insufficient. |
| Disassembler | Feasible as an independent BSD-2-Clause vaeg component in M40. |
| Licensing/provenance | MIT base plus the approved hash-verified downstream patch and BSD-2-Clause vaeg adapter are compatible and precisely reproducible. |
| Test acquisition | ZEXDOC/ZEXALL have an immutable, hash-verified GPL test-input path with source and offline cache support. |

## Consequences and unresolved risks

- `xf` is not initialized by legacy `Reset()` but influences saved AF and is
  serialized. Fixtures deliberately execute `XOR A` first. This is a legacy
  defect, not a behavior contract; it remains open and is not fixed by M34.
- Genuine revision-1 images are compiler-shaped. Local native GCC/Clang and
  cross-target layout probes agree, but Windows and macOS CI must execute the
  fixture target before M37 freezes per-family codecs.
- Actual private-ROM IM0 acceptance bytes are not yet observed. Optional M39
  tracing logs them only at real acceptance; static code permits every byte.
- ROM-less M39 tests preserve SLEEP_HACK's stale callback-time
  `GetReg()->pc` mirror, both constants, clock drain, and ATN wake. Actual VA
  and Sorcerian asset testing remains required at G39.
- The M34 task/master claim that positive remaining credit can be present
  after `Exec()` conflicts with current code; the verified zero/negative
  boundary governs implementation.
- The migration master says M33 is in the roadmap, but current `ROADMAP.md`
  has no M33 row. M33 commits exist in history. M34 leaves that completed work
  and identifier untouched rather than inventing an entry.
