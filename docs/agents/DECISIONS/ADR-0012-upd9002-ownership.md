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
# ADR-0012: Assign uPD9002 core and register ownership

## Status

Proposed at M42 for G42 human review. This ADR does not authorize M43 or any
later milestone.

## Context

The active PC-88VA main CPU is a C implementation under `i286c/`. It combines
an inherited 80286-oriented base decoder, run-time V30 patches, ABI-shaped raw
save state, and the separate built-in register object at I/O port `0xfff0`.
The supported machine always selects the V30-compatible native path, while the
source still exposes historical 286 and assembly-core choices. The M42
inventory and immutable dispatch baseline are needed before ownership can be
separated without changing behavior.

## Decision

### Ownership and final names

The instruction engine owns the future `upd9002_core_*` namespace. The
`0xfff0` built-in register block currently in `iova/upd9002.*` owns the future
`upd9002_regs_*` namespace. These are separate components; a future aggregate
`Upd9002Device` is outside this series.

The final instruction-core directory is `cpu/upd9002/`. Active file moves and
public renames occur only in M49. Internal static opcode-handler identifiers
and `I286_*` implementation helpers may remain when changing them would alter
the immutable source-level dispatch identity or add cosmetic risk.

The expected public mapping is:

| Current API | Final API / disposition | Owner milestone |
|---|---|---|
| `i286c_initialize` | `upd9002_core_initialize` | M49 |
| `i286c_deinitialize` | `upd9002_core_deinitialize` | M49 |
| `i286c_reset` | `upd9002_core_reset` | M49 |
| `i286c_shut` | `upd9002_core_shut` | M49 |
| `i286c_setextsize` | `upd9002_core_set_ext_size` | M49 |
| `i286c_setemm` | `upd9002_core_set_emm` | M49 |
| `i286c_interrupt` | `upd9002_core_interrupt` | M49 |
| `v30c_step` | `upd9002_core_step` | M49 |
| `v30cinit` | `upd9002_dispatch_initialize` | M49 |
| `i286c_step` | removed after native invariant | M45 |
| `i286c`, `v30c` | remove block executors | M46 |
| `i286c_intnum`, `i286c_selector`, REP helpers | internalize or retain under the graph/name exception | M47–M49 evidence |
| `upd9002_reset`, `upd9002_bind`, `upd9002` | future `upd9002_regs_*` names | M49 |

The one authoritative final execution primitive is
`upd9002_core_step()`. A future `upd9002_core_run(cycle_budget)` may loop that
primitive, but no run-budget API is implemented in M42–M49.

### State model

Three state concepts remain distinct:

* `Cpu286StateCompat` is the exact ABI-specific byte layout stored by the
  literal `CPU286` section.
* `Upd9002RuntimeState` is every field read, written, or address-taken by
  active reset, shutdown, interrupt, DMA-coupled execution, and instruction
  handlers.
* `Cpu286CompatImage` is the opaque serialization shadow that retains a full
  imported or canonical `CPU286` payload, including inactive fields and
  padding.

The instruction engine may access only `Upd9002RuntimeState`; the future state
adapter exclusively owns `Cpu286StateCompat` and `Cpu286CompatImage`. Import
must stage and validate the complete payload before commit. Export starts from
the compatibility image and overlays runtime-owned ranges. Reset and shutdown
apply the exact byte-range effects captured by M42 fixtures.

Compatibility is limited to the same ABI/toolchain family already implied by
raw structure serialization. The M42 Linux x86_64 GCC 15.2.0 fixture records a
112-byte, 4-byte-aligned `CPU286` payload and a 16-byte, 1-byte-aligned
`UPD9002` payload. No cross-endian or new cross-ABI guarantee is introduced.
The literal section names, section version zero, sizes, and loader-facing
layouts remain unchanged throughout this series.

M44 will add the only intentional externally visible behavior change in the
series: a serialized `CPU286` payload with `cpu_type != CPUTYPE_V30` is
rejected atomically before resume. This is a narrow compatibility firewall and
must not be generalized.

### Dispatch identity

Runtime table construction remains. M42 records two different artifacts:

* `upd9002_final_dispatch_graph.csv` is the immutable final source-level graph
  for six runtime roots and every recursively reachable secondary table.
* `upd9002_dispatch_provenance_m42.csv` records base entries, patch operations,
  and explicit DIV/IDIV replacements at M42.

The final graph must remain byte-identical through G49. Construction
provenance may change only when an approved later task changes dead base
construction while preserving the final graph. Runtime pointer values,
addresses, symbolizers, and function-pointer object hashes are not identity.

### Native-mode and shutdown invariants

All supported active CMake presets define `USE_I286C` on `vaeg_core` and reach
`v30c_step()` for VA machines. `USE_I286C=off`, `i286x_step()`, `v30x_step()`,
and the assembly tree are frozen unsupported reference configurations.

The current CPU_SHUT path is an explicit initializer anomaly: it clears bytes
only through `offsetof(I286STAT, cpu_type)`, retains the tail beginning with
`cpu_type`, invokes the 286-style register initializer, and therefore leaves
upper FLAGS unlike native reset. The anomaly does not enable 286 dispatch and
must remain byte-identical to the M42 shutdown fixture.

### Milestone ownership

* M45 owns removal of `i286c_step()` and the supported-selector branch after
  native-only evidence.
* M46 owns removal of `i286c()` and `v30c()` and construction normalization.
* M47 inventories and isolates the partial NP2 80286 protected-mode cluster.
* M48 may delete only the dependency-closed protected-mode groups explicitly
  approved at G47.
* M49 owns active moves, public renames, register-model renames, and final
  repository guards.

NP2 80286 protected-mode handlers, state, and helpers remain present through
G47. M42 inventories them only. No deletion is inferred from native
unreachability or a symbol name.

### External oracle and non-goals

SingleStepTests V20 is an external semantic oracle only for the intersection
of the supported uPD9002/V52 native profile and faithfully representable V20
records. M42 records the internal support map but does not download, vendor,
classify, or baseline the corpus. Broader V20/V30 family membership does not
prove target support. Known missing instruction forms are target gaps, not
failures or implementation requests.

The following remain out of scope: uPD9002 compatibility mode, missing
instructions, timing changes, prefetch modeling, performance optimization,
multithreading, a run-budget API, and state version changes.

### Baseline and tags

`pre-upd9002-series` is immutable and points to the accepted G41 SHA
`dc8a72da974f0ea328613e480f1de662c28f4436`. M42 adds graph, trace, harness,
ABI, reset, fixed-execution, and CPU_SHUT baselines. M42 must not create
`pre-upd9002-refactor`. That second tag is created only after M43 has completed
and G43 has been explicitly accepted.

## Consequences

The series can separate runtime state and remove dormant implementation only
against reproducible identity and behavior evidence. Raw save compatibility
remains deliberately ABI-scoped. The CPU_SHUT anomaly and current missing
instructions remain visible known constraints rather than opportunistic fixes.
