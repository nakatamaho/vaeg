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
# Z80 production integration

M39 adds a build-time production selection while deliberately retaining the
legacy core as the default:

```sh
cmake -S . -B build/z80-legacy -G Ninja \
  -DVAEG_Z80_CORE=legacy -DVAEG_ENABLE_TESTS=ON
cmake -S . -B build/z80-suzukiplan -G Ninja \
  -DVAEG_Z80_CORE=suzukiplan -DVAEG_ENABLE_TESTS=ON
cmake --build build/z80-legacy
cmake --build build/z80-suzukiplan
```

Only `legacy` and `suzukiplan` are accepted. The legacy selection compiles
`cpucva/z80c.cpp`; the new selection compiles `cpucva/z80_core.cpp` and
`cpucva/z80_legacy_state.cpp`. They are mutually exclusive in the production
`vaeg_va` target. There is no runtime toggle. Standalone conformance and M38
differential targets remain independent of this production choice.

The new selection keeps the existing consumer-facing `Z80C` signatures and C
bridge in `iova/subsystem.cpp`. Third-party and STL types do not cross that
seam. Every wrapper I/O callback masks the external port to eight bits, IRQ
remains a level, and the acknowledge port is read only when the core accepts a
maskable interrupt. The legacy `z80diag.cpp` remains the production decoder
for both choices through a small separately compiled callback bridge. M39 does
not replace or modify the disassembler.

## ROM-less integration evidence

With `VAEG_ENABLE_TESTS=ON`, `vaeg --selftest` runs the selected implementation
through the real `Subsystem`, I/O, clock, and state-save bridge. It covers:

- ordinary execution, live PC, and return-fresh public register mirror;
- callback-stale public PC at both VA and Sorcerian SLEEP_HACK addresses;
- `Wait(true)`, clock draining, ATN wake, `Wait(false)`, and resumed execution;
- the focused EI-immediately-before-VA-sleep hypothesis;
- DI suppression and exactly one acceptance-time IM1 acknowledge read;
- retained revision-1 input, same-core round trip, HALT, external WAIT, IRQ
  level, EI inhibition, and signed clock fields;
- rejected revision propagation through the complete `statsave_load()` path;
- one production `OUT (0xf4),0x5a`, save/load between CPU calls, and proof
  that the event is not duplicated on resume; and
- continued use of the legacy diagnostic decoder under both selections.

The synthetic sleep trace has the same observed contract for each core. At
the IN callback, live PC has advanced past `db fe` while public PC still names
the instruction address. At `Exec()` return both PCs agree. WAIT then drains
the next slice without advancing PC; ATN releases WAIT and the following slice
resumes at the next opcode. The production constants `0x1732` and `0x700e`
are unchanged.

The legacy core fuses EI and its following instruction inside one private
step. The new core can return immediately after EI, so the focused synthetic
case reaches the SLEEP_HACK callback on the following call with the required
stale public PC. This confirms the M38 hypothesis without changing either
magic constant. Private guest testing remains mandatory because the synthetic
case cannot prove the actual firmware path.

## State boundary and error handling

The production save boundary remains after `Z80C::Exec()` returns. No callback
or partial instruction is serialized. The revision-1 image remains 68 bytes
and represents architectural state, HALT, external WAIT, external level IRQ,
`remainclock`, and `lastclock`; new-core bit 2 retains frame-boundary EI
inhibition. The subsystem mirrors restored external WAIT only for tracing and
wake diagnostics; the core's restored state remains authoritative.

`subsystem_savecpustatus()` and `subsystem_loadcpustatus()` now return success
or failure. `statsave.c` propagates a codec rejection instead of continuing as
if an unsupported Z80 revision had loaded successfully. The ROM-less test
copies a valid state, changes only the embedded Z80 revision, and requires the
top-level load to fail before loading the valid state again.

## Optional private trace

Trace instrumentation is disabled and absent from normal builds. Enable it
only in a separate diagnostic tree:

```sh
cmake -S . -B build/z80-private-trace -G Ninja \
  -DVAEG_Z80_CORE=suzukiplan \
  -DVAEG_Z80_INTEGRATION_TRACE=ON
cmake --build build/z80-private-trace
./build/z80-private-trace/sdl2/vaeg --fdctrace
```

The existing FDC trace stream then adds deterministic Z80 `Exec()` entry and
return, live/public PC, IRQ level, acceptance byte, masked IN/OUT, WAIT
assertion/release, and state-boundary records. It contains no host pointer or
timestamp. Formatting and file I/O can perturb host wall-clock scheduling, so
it is diagnostic evidence rather than a claim of uninstrumented wall-clock
performance. Emulated clock values and ordered device events remain explicit.
Private filenames, paths, hashes, screenshots, raw traces, saves, and media
stay outside Git. Tracked records use neutral stable identifiers only.

## G39 private disposition

M38's exact `0xf4` reproducer remains applicable: corrected JR timing can move
an otherwise identical FDD output to a later `Exec()` slice. M39 adds no
legacy per-opcode cycle emulation. The
[private integration manifest](z80-private-integration.md) passed under both
production selections. Boot, read/write, representative and timing-sensitive
loaders, SLEEP_HACK, WAIT wake, and state-load behavior converged without a
guest-visible failure.

The representative trace emitted the same 21 ordered port-`0xf4` writes and
an exact 164-record common prefix of completed FDC commands. A fixed
wall-clock sample caught the new core in a later intermediate loader scene,
then sufficient execution reached the same stable menu. This remains an
accepted scheduling difference, not a lost, duplicated, reordered, or
permanently divergent device effect. The current trace has no dedicated DRQ
edge record, so DMA transfer metadata, IRQ/acknowledge events, final data, and
guest-visible completion provide the recorded evidence instead.

G39 passed on 2026-07-15. The default remains `legacy`, and M40 has not
started.
