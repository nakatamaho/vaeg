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
# M42 uPD9002 inventory and harness evidence

## Scope and source identity

M42 starts at `dc8a72da974f0ea328613e480f1de662c28f4436` on
`topic/m42-upd9002-adr-inventory-harness`. The annotated
`pre-upd9002-series` tag peels to that exact commit. The approved v6 plan has
SHA-256
`07c53e542adbe838de3d79999aa3d7acebf7a15f85f4d17aa0f5a5e50a293938`.
ADR-0012 was the next unused ADR number.

This milestone records evidence and test-only instrumentation. It does not
change dispatch construction, instruction behavior, timing, save-state layout,
public core names, or active implementation files. It neither acquires the
SingleStepTests corpus nor begins M43.

Two current-document contradictions were verified rather than silently
assumed:

* `docs/agents/ROADMAP.md` at the starting commit described M42 as an
  unscheduled Z80 performance investigation. The directly authorized v6 plan
  and M42 task instead define the uPD9002 evidence milestone. The roadmap is
  corrected by M42; the historical unused task is not deleted.
* The v6 plan refers to a pre-existing `golden_smoke.sh`/M23 golden. No such
  tracked script or dedicated M23 golden exists at the starting commit.
  Current M23 coverage is the ROM-less `--selftest` formatted-D88/raw-image
  output. M42 compares those pre-existing checkpoint lines byte-for-byte and
  does not invent or rewrite an M23 golden.

## Public and externally linked core inventory

Declarations are in `i286c/cpucore.h:202-218`; definitions and current active
callers are:

| Symbol | Definition | Active use |
|---|---|---|
| `i286core`, `iflags` | `i286c/i286c.c:11-13` and opcode sources | register/state macros and flag calculation throughout `i286c/`; raw `CPU286` save uses `i286core.s` |
| `i286c_initialize` | `i286c/i286c.c:96` | `CPU_INITIALIZE`, called by `pccore.c:273`; also initializes V30 tables through `v30cinit()` |
| `i286c_deinitialize` | `i286c/i286c.c:152` | `CPU_DEINITIALIZE`, called by `pccore.c:331` |
| `i286c_reset` | `i286c/i286c.c:177` | `CPU_RESET`, called by `pccore.c:398,411` and `statsave.c:1533` |
| `i286c_shut` | `i286c/i286c.c:197` | `CPU_SHUT`, reached through the reset-request helper at `pccore.c:1078-1084,1131` |
| `i286c_setextsize` | `i286c/i286c.c:203` | `CPU_SETEXTSIZE`, called by `pccore.c:399,412` and `statsave.c:1534` |
| `i286c_setemm` | `i286c/i286c.c:224` | `CPU_SETEMM`, called by `io/emsio.c:18,24,64` |
| `i286c_intnum` | `i286c/i286c.c:242` | `INT_NUM` macro in `i286c/i286c.mcr:510`, used by instruction/exception paths |
| `i286c_interrupt` | `i286c/i286c.c:263` | `CPU_INTERRUPT`, called by PIC at `io/pic.c:87,97,165,175`; trap paths also call it |
| `i286c` / `CPU_EXEC` | `i286c/i286c.c:289` | dormant block-executor branch at `pccore.c:1122-1129`, excluded by the local `SINGLESTEPONLY` define |
| `i286c_step` | `i286c/i286c.c:318` | non-V30 half of the active source selector at `pccore.c:1207-1212`; not selected by supported VA reset state |
| `v30c` / `CPU_EXECV30` | `i286c/v30patch.c:1411` | dormant block-executor branch at `pccore.c:1122-1129` |
| `v30c_step` | `i286c/v30patch.c:1440` | supported VA execution at `pccore.c:1214-1219` |
| `v30cinit` | `i286c/v30patch.c:1389` | called only by `i286c_initialize` at `i286c/i286c.c:148` |
| `i286c_selector` | `i286c/i286c_ea.c:977` | `SEGSELECT` plus protected/system handlers in `i286c/i286c_0f.c` |
| `i286c_cts` | `i286c/i286c_0f.c:259` | secondary 80286 system-op dispatch; retained graph edges from REP roots |

The REP helpers declared later in `i286c/i286c.h` (`ins*`, `outs*`, `movs*`,
`lods*`, `stos*`, `cmps*`, and `scas*`) are cross-translation-unit instruction
helpers consumed by the base and V30 patch units. The effective consumer macros
are `CPU_INITIALIZE`, `CPU_DEINITIALIZE`, `CPU_RESET`, `CPU_INTERRUPT`,
`CPU_EXEC`, `CPU_EXECV30`, `CPU_SHUT`, `CPU_SETEXTSIZE`, and `CPU_SETEMM` at
`i286c/cpucore.h:291-300`. M42 does not rename or internalize any symbol.

For completeness, the exact helper exports are `i286c_rep_insb`,
`i286c_rep_insw`, `i286c_rep_outsb`, `i286c_rep_outsw`, `i286c_rep_movsb`,
`i286c_rep_movsw`, `i286c_rep_lodsb`, `i286c_rep_lodsw`, `i286c_rep_stosb`,
`i286c_rep_stosw`, `i286c_repe_cmpsb`, `i286c_repne_cmpsb`,
`i286c_repe_cmpsw`, `i286c_repne_cmpsw`, `i286c_repe_scasb`,
`i286c_repne_scasb`, `i286c_repe_scasw`, and `i286c_repne_scasw`; declarations
are `i286c/i286c.h:135-152` and definitions are in `i286c/i286c_rp.c`.
`i286cea_initialize` and the externally declared base/group arrays
`i286op`, `i286op_repe`, `i286op_repne`, `c_op8x*`, `sft_*`, and
`c_ope0xf6/f7/fe/ff_table` are translation-unit seams used by the constructor
and handlers. Their exact slot contents and caller edges are enumerated rather
than summarized in the generated graph.

## Build and execution selection

`CMakeLists.txt:131-143` unconditionally places the C implementation in
`vaeg_core`, and `CMakeLists.txt:420` defines `USE_I286C` target-locally. No
configure preset overrides it. Consequently `linux-debug`, `linux-release`,
`linux-ci-gcc`, `linux-ci-clang`, `linux-asan`, `linux-ci-asan`, `mingw-cross`,
and the hosted equivalents all compile the same C core. `SUPPORT_V30ORIGINAL`
is also a fixed core definition (`CMakeLists.txt:622` for the ABI probe and the
ordinary core definition alongside `USE_I286C`).

`SINGLESTEPONLY` is locally defined in `pccore.c:1121`, so supported execution
uses the step branch. Reset writes `CPUTYPE_V30` for every VA model at
`pccore.c:402-407`; the selected supported function is therefore
`v30c_step()`. `i286x_step()`, `v30x_step()`, `USE_I286C=off`, and the assembly
tree are frozen reference configurations and are not reachable from a
supported preset. The source selector remains present for later milestones.

## CPU_TYPE and state load flow

`CPU_TYPE` aliases `i286core.s.cpu_type` (`i286c/cpucore.h:274`). Its complete
active flow is:

* reset configuration first clears it, then selects V30 for a V30 DIP or any
  VA model (`pccore.c:402-407`);
* `i286c_reset` saves the byte, zeroes the full state, restores it, and selects
  V30 or 286 register initialization (`i286c/i286c.c:177-194`);
* `i286c_shut` intentionally clears only `[0, offsetof(cpu_type))` and retains
  `cpu_type` and its tail (`i286c/i286c.c:197-200`);
* execution reads it in the step selector (`pccore.c:1207-1219`);
* device/diagnostic consumers read it in `io/printif.c:42`,
  `io/np2sysp.c:65`, and `generic/np2info.c:150`;
* interrupt code does not change it;
* save and load include it as byte 96 of the raw `CPU286` payload;
* the G41 loader validates section version and size only, not `cpu_type`.

The state table records `CPU286` version 0 as a raw `STATFLAG_BIN` of
`sizeof(CPU_STATSAVE)` (`statsave.tbl:112`) and `UPD9002` version 0 as a raw
16-byte register object (`statsave.tbl:198`). `statsave_check()` applies
version-and-exact-size checking to BIN sections (`statsave.c:1426-1459`).
`statsave_load()` requires the first section to match, resets live components,
and then reads sections through existing raw loaders (`statsave.c:1502-1544`);
it has no CPU-type preflight and can modify earlier live sections before a
later failure. M42 preserves these rules. Atomic V30-only rejection belongs
only to M44.

## Raw state-member use matrix

The G41/current Linux x86_64 GCC 15.2.0 ABI is little-endian,
`sizeof(I286STAT) == 112`, `_Alignof(I286STAT) == 4`,
`sizeof(UPD9002) == 16`, and `_Alignof(UPD9002) == 1`. The complete raw CPU
range classification is:

| Bytes | Member / overlay | Active effects and evidence |
|---:|---|---|
| 0-27 | `r`, overlaid `I286REG8`/`I286REG16` | all architectural registers, FLAGS, and IP are read/addressed/written by handlers and macros; reset/full load writes the range; shutdown clears it |
| 28-31 | `es_base` | segment/address calculation, selector loads, reset/load; shutdown clears |
| 32-35 | `cs_base` | fetch, interrupts, trace, selector loads, reset/load; shutdown clears |
| 36-39 | `ss_base` | stack, interrupts, selector loads, reset/load; shutdown clears |
| 40-43 | `ds_base` | data addressing and selector loads; reset/load; shutdown clears |
| 44-47 | `ss_fix` | fixed segment/address helper state; direct harness and instructions read/write; reset/load; shutdown clears |
| 48-51 | `ds_fix` | fixed segment/address helper state; direct harness and instructions read/write; reset/load; shutdown clears |
| 52-55 | `adrsmask` | every memory-address mask and A20 update; reset initializes it; load restores it; shutdown clears then `i286c_initreg` sets it |
| 56-57 | `prefix` | segment-prefix source/handler state; reset/load; shutdown clears |
| 58 | `trap` | STI/CLI, trap execution and interrupt flow; reset/load; shutdown clears |
| 59 | `resetreq` | scheduler reads/clears it before CPU_SHUT; instruction paths can set it; reset/load; shutdown clears because scheduler clears first |
| 60-63 | `ovflag` | V30 step temporarily materializes O_FLAG here (`v30patch.c:1445-1454`); reset/load; shutdown clears |
| 64-69 | `GDTR` | descriptor selection and 0F system instructions (`i286c_0f.c`); reset/load; shutdown clears |
| 70-71 | `MSW` | protected-mode selector/system instruction branch and LOADALL; reset/load; shutdown clears |
| 72-77 | `IDTR` | system instructions/LOADALL; reset/load; shutdown clears |
| 78-79 | `LDTR` | system instructions/LOADALL; reset/load; shutdown clears |
| 80-85 | `LDTRC` | descriptor selection and LOADALL; reset/load; shutdown clears |
| 86-87 | `TR` | system instructions; reset/load; shutdown clears |
| 88-93 | `TRC` | system instructions/LOADALL; reset/load; shutdown clears |
| 94-95 | explicit `padding[2]` | no semantic field access; included in whole-object zero, raw save/load, fixture comparison, and shutdown range clear |
| 96 | `cpu_type` | selection/reporting flow above; full reset preserves around ZeroMemory; raw load restores; shutdown deliberately retains |
| 97 | `itfbank` | interface-bank state exposed by macro; raw load/reset; shutdown deliberately retains |
| 98-99 | `ram_d0` | RAM-window state exposed by macro; raw load/reset; shutdown deliberately retains |
| 100-103 | `remainclock` | scheduler/instruction/DMA cycle balance; raw load/reset; shutdown deliberately retains |
| 104-107 | `baseclock` | scheduler cycle slice; raw load/reset; shutdown deliberately retains |
| 108-111 | `clock` | cumulative CPU clock; raw load/reset; shutdown deliberately retains |

Whole-state address-taking occurs through `CPU_STATSAVE` in `statsave.tbl` and
the M42 fixture/harness snapshots. `i286c_reset` uses full `ZeroMemory`; the
shutdown range write is exactly `[0,96)`. The fixture records show reset FLAGS
`f002` but CPU_SHUT FLAGS `0000`, while the retained tail remains unchanged.
This documented initializer anomaly is compatibility behavior, not a defect
fix.

The exact ABI offsets are permanently recorded in
`tests/upd9002/abi_g41.txt`. A detached G41 build with the same compiler and
definitions produced the same 112/4 and 16/1 sizes and all offsets. The M42
fixture reader was also compiled as a test-only probe against the detached G41
core; all three recorded payload rows matched exactly. That probe was not
committed to or used to alter the G41 source history.

## Dispatch inventory and support handoff

`tools/qa/upd9002_dispatch.py` parses initializer arrays and every statement in
`v30cinit()` fail-closed. It produces:

* 1,272 immutable final-graph rows spanning 58 root, secondary, group, and
  handler-followed table nodes;
* 1,040 construction-provenance rows: four 256-slot roots and two eight-slot
  F6/F7 roots; 954 unchanged base entries, 82 patch operations, and four
  explicit DIV/IDIV replacements;
* a 1,296-entry M43 handoff map: 1,024 primary root forms, 256 resolved 0F
  second-byte forms, and 16 F6/F7 ModR/M groups; 788 are currently implemented
  and 508 are explicit known target gaps;
* a 156-case direct-harness manifest covering patched roots, all active 0F
  second-byte handlers, F6/F7 groups, DIV/IDIV normal and fault boundaries,
  memory, I/O, and O_FLAG restoration.

The six runtime-built roots are `v30op[256]`, `v30op_repne[256]`,
`v30op_repe[256]`, `v30op_repc[256]`, `v30ope0xf6_table[8]`, and
`v30ope0xf7_table[8]`. The generator models all CopyMemory, patch-list,
reserved-REPC fill, and explicit replacement operations at
`i286c/v30patch.c:1389-1409`. It follows every statically discoverable
secondary edge rather than stopping at those roots. Selftests prove failure on
incomplete parsing, missing/duplicate slots, cardinality mismatch, unknown
edges, and nondeterministic output.

The stable C99 test API in `tests/upd9002/direct_harness.h` loads fixed-width
CPU/RAM/program values and returns fixed-width CPU, RAM hash, step count, and
termination values. Emulator globals are private to its implementation. Each
manifest case is run twice and compared exactly. No missing instruction is
implemented or treated as a test failure.

## Retained 80286 protected-mode cluster

The retained cluster consists primarily of `i286c/i286c_0f.c`, descriptor
addressing in `i286c/i286c_ea.c`, the selector and interrupt macros in
`i286c/i286c.mcr`, the base/REP entries in `i286c/i286c_mn.c`, and the GDTR,
MSW, IDTR, LDTR/LDTRC, TR/TRC state fields. It includes SLDT/STR/LLDT/LTR,
VERR/VERW, SGDT/SIDT/LGDT/LIDT, SMSW/LMSW, LOADALL, descriptor helpers,
`cts0_table`, and `cts1_table`.

The native `v30op[0x0f]` is patched to `v30_ope0x0f`, but REP-root 0F entries
retain `i286c_cts`, so the source-level recursive graph still records both CTS
tables and their helpers. This is inventory, not a removability decision.
M47 owns dependency isolation and M48 may delete only an explicitly approved
dependency-closed list.

## Trace and state baselines

`--trace-cpu N` emits a fixed `upd9002-trace-v1` schema with lowercase fixed
widths, instruction bytes, start/end register/base/cycle values, ordered event
sequence numbers, actual CPU fetch/memory-write/eight-bit-I/O events, and
explicit DMA/device scheduler checkpoints. The default is off. Two identical
eight-step runs compare byte-for-byte, and a trace-disabled run reaches the
same direct-harness, state, device, cycle, framebuffer, formatted-D88, and raw
image selftest checkpoints.

The committed raw fixtures are:

* reset: CPU286 112 bytes, UPD9002 16 bytes, `CS:IP=f000:fff0`, FLAGS `f002`;
* executed-3: `mov ax,1234h; inc ax; nop`, `AX=1235`, `IP=2005`, with exact
  cycle and device payloads;
* CPU_SHUT/reset request: the real scheduler condition and macro path, retained
  tail, `CS:IP=f000:fff0`, and anomalous FLAGS `0000`.

The production reset-request branch and the fixture share the static
`pccore_process_cpu_reset_request()` helper. A test-only wrapper is compiled
only with `VAEG_ENABLE_TESTS`; normal builds add no callable test seam. This is
a mechanical extraction of the existing check/clear/CPU_SHUT sequence, not a
new reset path.

The starting G41 tree contains no dedicated M23 golden file. Its complete
15-test ROM-less CTest suite passes in the detached worktree. The current
pre-existing selftest checkpoint subset, including the M23 D88/IMG lines, is
byte-identical; M42 adds only separately named uPD9002 output. New baseline
files are `upd9002_final_dispatch_graph.csv`,
`upd9002_dispatch_provenance_m42.csv`, `upd9002_support_map_m42.csv`,
`harness_manifest.csv`, `trace_baseline.txt`, `state_fixtures_m42.txt`, and
`abi_g41.txt`.

## SingleStepTests identity and known gaps

No SingleStepTests dataset is acquired, read, hashed, classified, or committed
in M42. Therefore M42 has no dataset commit or content identity to report.
M43 must pin and record that identity before using the external V20 oracle.

Known gaps are the 508 explicitly classified target-gap forms in the support
map, the retained protected-mode cluster pending M47, and the existing
ABI-specific raw serializer with no CPU-type preflight. Detailed device-origin
trace records are checkpoints in M42; DMA transactions are represented by a
deterministic scheduler-origin checkpoint unless a tested instruction causes a
real transfer. None of these gaps authorizes semantic work in M42.

## G42 review disposition

The automated evidence is intended for human G42 review. Human review must
confirm the ADR ownership, state matrix, final graph/provenance distinction,
M43 support-map/direct-API handoff, preserved CPU_SHUT anomaly, and standard
behavior gate. This report does not claim that G42 passed and does not
authorize M43.
