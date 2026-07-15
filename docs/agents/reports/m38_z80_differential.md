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
# M38 Z80 differential evidence

Date: 2026-07-15

Status: implemented; **G38 blocked** by one unallowlisted FDD-visible
clock-scheduling divergence; stopped before M39

## Starting state and scope

The branch was created as `topic/m38-z80-differential` from exact approved
G37 commit `9599e5c9b91ef545e6e7dea5957681e8ce30b3a4`. The required starting
commands returned:

```text
$ git status --short
?? "PC-Engine 1.05(86U13).d88"
?? docs/modernization/PCEPAT.DOC
?? docs/tekumani/
?? his.txt
$ git rev-parse HEAD
9599e5c9b91ef545e6e7dea5957681e8ce30b3a4
$ git branch --show-current
topic/m38-z80-differential
```

All four paths remain untracked and untouched. The optional M34 report was
absent. All other required inputs were read in the requested order. No ROM,
disk image, or frozen-reference input was used.

The implementation commits before documentation closure are:

```text
8923817 M38: correct wrapper state exposed by differential probes
37ffe3c M38: add normalized Z80 differential harness
```

The documentation closure commit and exact ending SHA are reported in the
final G38 handoff; a commit cannot embed its own immutable SHA.

## Runner architecture and trace schema

The harness uses three independent executables:

- `vaeg_z80_legacy_trace` links `cpucva/z80c.cpp` and `z80diag.cpp`;
- `vaeg_z80_new_trace` links the standalone
  `vaeg_z80_wrapper_no_functional` library;
- `vaeg_z80_trace_compare` parses both traces and reports the first normalized
  difference.

The old and new `Z80C` definitions never enter one process or translation
unit. There is no class-renaming macro. Both runners compile the same scenario
descriptions, memory patches, event scripts, seeds, and clock slices from
`trace_common.cpp`.

The `vaeg-z80-trace-v1` text schema uses LF, stable field order, fixed-width
lower-case hexadecimal, and no host pointers, timestamps, or temporary paths.
Each test header records its identifier, seed, initial program bytes, and
numeric action script. Ordered event records identify memory read/write,
masked I/O read/write, interrupt acknowledge, NMI, state save, and state load.
I/O and acknowledge events also record callback-time live and public PC.
Checkpoint records contain:

- AF, BC, DE, HL, IX, IY, SP, and architectural PC;
- alternate AF, BC, DE, and HL;
- I, complete R, R low seven bits, R bit 7, IFF1, IFF2, and IM;
- HALT, external WAIT, external IRQ, and EI inhibition;
- authoritative live PC and public mirrored PC;
- `remainclock`, `lastclock`, and cumulative consumed clocks;
- full-memory and selected device/stack memory hashes;
- checkpoint index, reason, and normalized/non-normalized status.

The comparator matches `test/reason/occurrence`, not private decoder step
count. Events from non-normalized EI checkpoints accumulate into the next
normalized checkpoint. Architectural fields, R bits 0-6, R bit 7, memory
hashes, and event order are strict except for an exact allowlist entry. Clock
and remaining-balance differences go to the cycle report and become failures
when a later normalized checkpoint proves an external scheduling effect.

## Corpus

The directed corpus has 61 scenarios and 64 normalized checkpoints. It covers
loads, arithmetic/flags, taken and untaken branches, CALL/RET/PUSH/POP,
alternate exchange, IX/IY, address wrapping, memory ordering, LDI/LDD/LDIR/
LDDR, CB/ED/DD/FD/DDCB/FDCB, `LD A,R`, RETN/RETI, immediate and `(C)` I/O,
all eight directed block-I/O forms, DI/EI, changed acknowledge data,
deasserted and persistent IRQ, IM0 `00`, `7f`, RST, multi-byte and prefix
forms, IM1, ordered IM2, HALT entry/wake, NMI, NMI priority, and WAIT resume.

The state corpus has 22 scenarios and 26 normalized checkpoints. It loads all
15 retained M34 revision-1 fixtures, then covers positive/zero/negative clock
state, HALT, external WAIT, restored asserted IRQ, and the focused EI
inhibition save/load boundary. It compares decoded/resumed behavior, not raw
new-versus-old serialized bytes, and does not ask the legacy core to load a
new EI-bit image.

The public generated corpus uses fixed seeds `0x4d383001`, `0x4d383002`,
`0x4d383003`, and `0x4d383004`, with 128 bounded defined base-instruction
cases per seed: 512 cases/checkpoints. The trace header preserves the exact
first failing generated program if a regression occurs. Prefix and interrupt
behavior stay in the directed suite so a broad generated allowlist cannot
hide their classified differences. The longer local run uses 4,096 cases per
seed: 16,384 cases/checkpoints.

## Wrapper corrections supported by minimal probes

M38 corrected only the standalone vaeg wrapper and its focused tests. The
production core is unchanged.

| Finding | Minimal probe | Disposition |
|---|---|---|
| The third-party core increments R for the first opcode but not a following CB/ED/DD/FD M1 fetch. | `cb 00`, R=`ff`; raw IM0 `cb` plus memory `00` | Adapter tracks only a proven prefix first byte and increments R on its following M1 callback. No extra memory read occurs. |
| HALT idle reads did not increment R. | HALT followed by one 4-clock idle slice | Adapter increments R before each complete third-party HALT-idle execution. |
| RETI did not copy IFF2 to IFF1; RETN with clear IFF2 set IFF1. | `ed 4d` and `ed 45`, stack return word, both IFF2 values | Adapter restores IFF1 from IFF2 at instruction completion. |
| `LD A,I`/`LD A,R` changed only PV and left the other defined flags stale. | `ed 5f`, R=`ff`, F carry set | Adapter materializes S/Z/5/H/3/PV/N while retaining carry. |
| New synchronous NMI refreshed public PC, unlike the legacy split `inst`/mirror model. | NMI at PC `2222` | Adapter updates architectural mirror fields but retains the pre-NMI public PC until `Exec()` returns. |

The prefix/opcode tracking exists only while the core is executing a complete
instruction. No instruction or callback is active at the approved frame save
boundary, so no new revision-1 field is needed. Wrapper tests cover normal and
`Z80_NO_FUNCTIONAL` configurations. Raw and wrapper ZEX remain green.

## Evidence-backed allowlist

The comparator contains exact scenario/checkpoint sets, not opcode ranges.
Every identifier below emitted at least one accepted field or event. The
generated corpus emitted none.

| Stable ID | Minimal input and first difference | Classification and safety | FDD relevance |
|---|---|---|---|
| `vaeg-m38-halt-pc-model` | `76`; live/public PC old=opcode, new=post-HALT | Approved explicit HALT-PC translation; architectural decoded PC matches. | Representation only. |
| `vaeg-m38-halt-fetch-model` | restored HALT then one slice; old rereads opcode, new performs post-PC idle read | Internal HALT representation. Current subsystem reads are ROM/RAM/`ff`, with no memory-mapped device side effect. | No current device effect; clocks are separate. |
| `vaeg-m38-legacy-stack-write-order` | `cd 34 12`, `c5`, RST/IRQ/NMI pushes; old low then high, new high then low | Legacy `Push` writes a little-endian word after subtracting SP; new uses Z80 bus order. Final bytes/SP match. Current `Subsystem::Write8` only stores `0x4000-0x7fff` RAM and has no MMIO callbacks. | No current FDD I/O event; bus-order correction retained. |
| `vaeg-m38-legacy-skipped-branch-operand-read` | `20 7f` with Z set; old skips the displacement callback | Legacy omits a real operand read. Current Z80 memory map has no read-side-effect MMIO; architectural result matches. | None in current map. |
| `vaeg-m38-legacy-xf-exchange` | `08`, AF=`1234`, AF'=`abcd`; old result `abe5`, new `abcd` | Legacy lazy `xf` leaks across EX AF,AF'. Architectural AF is authoritative by ADR-0011. | Deliberate architectural correction. |
| `vaeg-m38-legacy-ei-fusion-deassert` | `fb 00`, asserted IRQ, slice 1, deassert, slice 4; old acknowledges, new does not | Legacy fuses EI's following instruction and makes API deassertion impossible at that boundary. New behavior is the approved level/EI contract. | Deliberate interrupt correction. |
| `vaeg-m38-legacy-interrupt-r` | NOP plus accepted IRQ, NMI, or persistent reacceptance; old omits acknowledge M1 R increment | New core increments R on interrupt/NMI acknowledge; low seven bits are recorded and bit 7 stays strict. | Architectural correction; software reading R can observe it. |
| `vaeg-m38-legacy-halt-r` | loaded HALT plus accepted IRQ; old R=`01`, new R=`02` | New counts HALT idle and interrupt-acknowledge M1 cycles. | Architectural correction. |
| `vaeg-m38-legacy-block-memory-flags` | `ed a0/a8/b0/b8`, A=`12`, data=`34`; old F=`05`, new F=`25` | Legacy omits defined/undocumented transfer-result flag materialization; new result is retained with ZEX conformance evidence. | Guest flags can differ; deliberate correction. |
| `vaeg-m38-legacy-block-io-flags` | `ed a2/aa/b2/ba/a3/ab/b3/bb`, B=`02` | Legacy updates only Z/N; new core materializes block-I/O flags and passes ZEX. | FDD code can observe flags; deliberate correction. |
| `vaeg-m38-ei-save-boundary-scheduling` | `fb 00`, save/load after slice 1 | Old has already fused NOP/IRQ; new saves `execEI`. After reload the next instruction, ack count/value, stack result, and architectural state agree apart from separately classified R/stack order. | Required production-reachable state boundary. |

EI-following I/O/acknowledge callbacks can see different stale public PC
values because the new core can return immediately after EI. In the exact
allowlisted probes the callbacks are port `0xf0` and the acknowledge port;
neither production handler consumes mirrored PC. A hypothetical EI directly
before the production SLEEP_HACK `IN 0xfe` remains a G39 private-system risk,
not an observed M38 result.

## Cycle evidence and unresolved G38 failure

The generated [cycle report](../../modernization/z80-cycle-deltas.md) lists
29 normalized directed/state checkpoints with a nonzero consumed/balance
delta. The generated corpus has zero cycle-delta groups. Examples include
taken JR legacy/new `7/12`, RET `4/10`, repeated block I/O `16/21`, DDCB
`16/20`, IM1 acceptance `17/12`, and legacy HALT wake `77/12`.

G38 is blocked by the separate unallowlisted scheduling suite:

```text
initial: A=5a, PC=0000
bytes:   18 02 00 00 d3 f4
slices:  1, 7
first divergent checkpoint: cycle-fdd-io-scheduling/fdd-io-slice/0
legacy: PC=0006, R=02, reads 0004:d3 and 0005:f4, OUT f4:5a
new:    PC=0004, R=01, no memory or I/O event in that slice
```

The source explanation is exact: legacy charges 7 clocks for taken JR, while
the new core charges 12. Production `Subsystem::Out(0xf4)` calls
`fdcsubsys_o_dskctl`. The cycle difference therefore changes an FDD-visible
I/O event's `Exec()` return boundary. The M38 clock policy defines that as a
failure. The comparator exits 1 with five unresolved differences (PC, R,
live PC, public PC, and events). No allowlist entry suppresses it.

## Validation evidence

### Native Linux

| Exact command | Result |
|---|---|
| `cmake --preset linux-ci-gcc -DVAEG_ZEX_ARTIFACT_DIR=/tmp/vaeg-m36-z80.7UocP2/zex-cache` | PASS; all five configured ZEX artifacts reverified |
| `cmake --build --preset linux-ci-gcc` | PASS |
| `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ctest --test-dir build/linux-ci-gcc --output-on-failure` | PASS, 17/17; raw ZEXDOC 72.58 s, raw ZEXALL 71.06 s, wrapper ZEXDOC 141.02 s, wrapper ZEXALL 140.75 s, differential 3/3, ROM-less selftest 1.24 s |
| `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/linux-ci-gcc/sdl2/vaeg --smoke` | PASS, reduced ROM-less smoke |
| `cmake --preset linux-ci-clang && cmake --build --preset linux-ci-clang` | PASS |
| `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ctest --test-dir build/linux-ci-clang --output-on-failure` | PASS, 13/13 |
| `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/linux-ci-clang/sdl2/vaeg --smoke` | PASS, reduced ROM-less smoke |
| `cmake --preset linux-ci-asan && cmake --build --preset linux-ci-asan` | PASS |
| `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/linux-ci-asan --output-on-failure` | PASS, 13/13; differential 3/3 under ASan/UBSan |
| `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ASAN_OPTIONS=detect_leaks=0 ./build/linux-ci-asan/sdl2/vaeg --smoke` | Process PASS; reproduced the pre-existing signed-overflow/shift UBSan backlog in `tms3631c.c`, `psggenc.c`, `psggeng.c`, and `parts.c` |
| final all-suite CMake-script command with `SUITE=all`, `CASES=128` | PASS; 595 scenarios, 602 normalized checkpoints, 58 exact accepted matches |
| long CMake-script command documented in `BUILD.md`, `SUITE=generated`, `CASES=4096` | PASS; 16,384 cases/checkpoints, zero allowlist matches, about 3 seconds locally |
| direct legacy/new `--suite scheduling`, then comparator | Expected gate failure; five unresolved FDD-scheduling differences, exit 1 |

### MinGW and Wine

| Exact command | Result |
|---|---|
| `cmake --preset mingw-cross && cmake --build --preset mingw-cross` | PASS, cross-compile only |
| `WINEDEBUG=-all WINEPREFIX=/tmp/vaeg-m38-wine WINEPATH=/usr/lib/gcc/x86_64-w64-mingw32/13-win32 wine64 build/mingw-cross/vaeg_z80_wrapper_default.exe` | PASS, all 10 grouped wrapper tests |
| same command with `vaeg_z80_wrapper_no_functional_test.exe` | PASS |
| Wine legacy/new/comparator commands for directed, state, and generated suites | PASS: 64, 26, and 512 normalized checkpoints respectively; generated uses zero allowlist entries |
| `WINEDEBUG=-all WINEPREFIX=/tmp/vaeg-m38-wine WINEPATH=... SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy wine64 build/mingw-cross/sdl2/vaeg.exe --smoke` | PASS, reduced ROM-less smoke |

Wine is execution of a cross-built Windows binary, not hosted native Windows.

### Repository, archive, provenance, and hosted CI

The final repository checks, archive exclusion audit, vendored hashes,
production-selection proof, and hosted GitHub Actions results are recorded in
the final G38 handoff after the documentation commit. Native macOS and native
Windows are not available locally and are not claimed from the Linux host.

## Verified facts, limitations, and hypotheses

### Verified facts

- The normal directed, state, public generated, and long generated corpora
  pass with no unresolved architectural, bus, interrupt, WAIT, HALT, or
  resumed-state difference outside the exact classifications above.
- All 15 retained revision-1 fixtures load in both runners. External WAIT,
  IRQ, clock signs, HALT, and EI inhibition resume at completed instruction
  boundaries.
- Every external test I/O event is eight-bit masked. Acknowledge events remain
  distinct and IM2 order is acknowledge, vector low, vector high.
- The standalone wrapper corrections do not require a third-party edit or a
  revision-1 extension.
- Production still compiles the legacy `cpucva/z80c.cpp`; the new wrapper is
  absent from production sources and `iova/subsystem.cpp` is unchanged.
- The scheduling reproducer changes the timing of an actual FDD control-port
  call, so G38 is not passed.

### Accepted differences

The eleven stable IDs in the allowlist table are classified with minimal
inputs and exact checkpoint sets. They are legacy defects, deliberate
architectural corrections, or externally inert representation/scheduling
differences. None is used by the generated corpus.

### Known limitations

- The public corpus is bounded and is not exhaustive instruction fuzzing.
- Cycle identity is deliberately not asserted. Twenty-nine normal
  checkpoints have deltas, and the FDD scheduling case proves at least one is
  externally consequential.
- Private subsystem ROM and disk behavior is not tested or committed. G39 is
  the intended private-system comparison milestone, but it cannot start
  while G38 is blocked.
- Native hosted platform results depend on the pushed CI run and are not
  inferred from cross-compilation or Wine.

### Unresolved issue

`cycle-fdd-io-scheduling` is unresolved and blocks G38. A maintainer decision
must not simply relabel it as an inert cycle delta: resolution needs an
approved clock-compatibility policy or implementation that addresses the
FDD-visible `Exec()` scheduling difference.

### Hypotheses

- Other recorded cycle deltas may move additional FDD events between frame
  calls; the one minimal `0xf4` case proves the risk class but does not
  enumerate every affected instruction sequence.
- EI immediately before either SLEEP_HACK `IN 0xfe` address may expose the
  new intermediate public-mirror boundary. No private ROM evidence was used,
  so this remains a hypothesis for later authorized work.

M39 production selection, private differential work, disassembler work, and
all approved legacy deletions were deliberately not started.
