# M44 uPD9002 serialized/runtime state boundary

Status: implementation complete; G44 is not declared by this report.

## State

Branch: `topic/m44-upd9002-state-boundary`  
Continuation starting SHA: `83858f26e0896e9cb2510e247443d4c0c2728fe6`
Current SHA at report preparation: `44aa172d7bfe1ba2fca65c7ba9eac2ad791fd82b`  
Baseline tag: `pre-upd9002-refactor` -> `91ec9a4c998928523360c37dab8d6ade8e698731`.

The worktree was clean before the report change. The branch descends from the
approved M43 SHA. No M45 work was started.

## Approved G41 baseline resolution

The approved legacy raw-`I286STAT` G41 baseline is
`dc8a72da974f0ea328613e480f1de662c28f4436`. This identity is unambiguous:
the M42 report names that full SHA as its accepted starting point, and the
local and remote annotated `pre-upd9002-series` tags peel to the same commit.
The G43 commit
`91ec9a4c998928523360c37dab8d6ade8e698731` and the
`pre-upd9002-refactor` tag are M42/M43 preservation references; they are not
substitutes for G41 in legacy raw-loader cross-version tests.

The authoritative scenario definitions are the committed M42 fixture
implementation and `tests/upd9002/state_fixtures_m42.txt`, whose SHA-256 is
`c8ed4bcf1a7df2a88964d71d85b846a6d7881f60a9233d8c9b787d3d5076f4fb`.
The cross-version matrix must reproduce those reset, `executed-3`, and
`cpu-shut-request` payloads rather than infer a scenario from comments or
redesign it from current code.

## ABI and ownership

`Cpu286StateCompat` is the 112-byte, alignment-4 serialization contract used
by the CPU286 section. On the supported GCC x86-64 little-endian ABI, recorded
offsets are: `r=0`, `es_base=28`, `cs_base=32`, `ss_base=36`, `ds_base=40`,
`ss_fix=44`, `ds_fix=48`, `adrsmask=52`, `prefix=56`, `trap=58`,
`resetreq=59`, `ovflag=60`, `GDTR=64`, `MSW=70`, `IDTR=72`, `LDTR=78`,
`LDTRC=80`, `TR=86`, `TRC=88`, `padding=94`, `cpu_type=96`, `itfbank=97`,
`ram_d0=98`, `remainclock=100`, `baseclock=104`, and `clock=108`.

`Upd9002RuntimeState` has the same measured size and alignment and currently
contains all active fields required by execution, reset, interrupt, DMA seams,
CPU_SHUT, and supported tests. `Cpu286CompatImage` owns an opaque complete
112-byte image. The execution core uses runtime state; inactive compatibility
bytes are not consulted for execution decisions.

## Load/save flow and atomicity

The `CPU286` statsave entry now uses `STATFLAG_CPU286`. Export starts from the
compatibility image and overlays active runtime fields. Import reads a complete
temporary image, checks section version and exact size, validates
`cpu_type == CPUTYPE_V30`, constructs temporary runtime state, and commits both
objects only after validation. The deterministic errors are:

* `CPU286 payload size is not 112 bytes`
* `CPU286 payload is truncated`
* `CPU286 cpu_type is not CPUTYPE_V30`

The statsave preflight validates the CPU286 section before other live sections
are committed. Tests cover non-V30, malformed-size, and truncated payloads and
verify no mixed state can resume. Rejecting non-V30 `cpu_type` is the sole
intentional externally visible behavior change permitted in M42–M49.

## Lifecycle and compatibility evidence

Initialization/reset clears the compatibility image and overlays active state,
retaining reset FLAGS `f002`. CPU_SHUT clears the historical byte range
`[0,96)` and preserves the documented FLAGS `0000` anomaly. The UPD9002 section
remains byte-identical. Deliberately non-canonical opaque bytes round-trip
unchanged on immediate save; reset and CPU_SHUT transformations match the
recorded G41/M42 behavior. The payload probe covers reset, fixed execution, and
CPU_SHUT fixtures and performs extracted CPU286/UPD9002 comparisons.

Cross-version probe coverage is present for the current ABI and accepted M42
fixtures. A detached G41 build comparison and hosted CI result remain human
review items; they are not represented as completed here.

## Baseline preservation

The pinned M43 identity remains unchanged:

* dataset: `ssts-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21-1d2e9c0e14101f05379d938245af68f3219c16f638fce019ad2a1946084930a4`
* known-gap selectors: 40; records: 68,626
* known-gap SHA-256: `11ec1496a96d661c0656720b13939b1ade3448b693b923f8acf36576cd7048c1`
* CI summary SHA-256: `a5db6a6cc82ae794fd2f60306c3d4a70136d6030e17e1aec733523bece864e31`
* full summary SHA-256: `dd3247774afe5c5a19228d3a08f01ac6f614e6b67a3a6f454c1abc58f3dbf3d9`

M42 dispatch, support-map, trace, ABI, state, and CPU_SHUT fixtures remain
unchanged in the commits reviewed for M44. Re-running the complete M43 CI/full
profiles and independent `--expect` reproduction is required for the G44 human
gate and is not claimed here.

## Validation run

## Trace-equivalence configuration correction

The earlier diagnosis that the canonical trace lacked origin fields was
incorrect. The committed M42 baseline contains `origin=cpu`, `origin=dma`, and
`origin=device` (SHA-256
`17e5383f0d3fce707a64d995cc9c13c6ae62a61462bf0df202ec523278587bba`). The
missing-origin output came from a stale/incorrectly configured test executable:
the build had `VAEG_ENABLE_TESTS=ON` but
`VAEG_Z80_INTEGRATION_TRACE=OFF`. Consequently the CPU trace seam did not emit
canonical origin records. The producer and baseline were not changed.

A fresh build configured with
`-DVAEG_ENABLE_TESTS=ON -DVAEG_Z80_INTEGRATION_TRACE=ON` generated the correct
trace and passed `vaeg_upd9002_trace_equivalence` (exit 0, 3.49 seconds). The
trace test therefore retains origin as part of semantic equality and continues
to compare ordering and event contents. Tests-disabled production builds keep
the trace option disabled. CMake now fails closed when
`VAEG_ENABLE_TESTS=ON` and `VAEG_Z80_INTEGRATION_TRACE=OFF`, with the diagnostic
`VAEG_ENABLE_TESTS requires VAEG_Z80_INTEGRATION_TRACE=ON; the trace-equivalence
test cannot use an uninstrumented executable`. A fresh tests-disabled configure
with both options OFF still succeeds. Runtime origin checks remain fail-closed
for missing CPU, DMA, or device records.

Successful commands:

```text
cmake -S . -B /tmp/vaeg-m44-tests -DVAEG_ENABLE_TESTS=ON -DCMAKE_BUILD_TYPE=Debug  (0)
cmake --build /tmp/vaeg-m44-tests -j2  (0)
ctest --test-dir /tmp/vaeg-m44-tests --output-on-failure -R 'vaeg_upd9002_(abi|state_boundary|state_payload_probe|statsave_boundary)'  (0; 3/3)
```

The initial production-only configure used the wrong option name and produced
an expected link failure from test-only objects; it was discarded and does not
alter the repository. The trace-equivalence configuration issue is resolved by
the fail-closed guard and trace-enabled target definitions described below.
Repository-wide encoding/EOL/case/unreferenced checks,
sanitizers, MinGW/Wine, hosted CI, and full M43 reproduction remain pending.

## G44 human-review checklist

* Run all standard repository and build matrices, including tests-disabled
  production isolation.
* Re-run M42 fixtures and both pinned M43 profiles with independent
  `--expect` output.
* Verify detached G41/current cross-version matrix in both directions.
* Review payload digests, CPU_SHUT anomaly, and rejection atomicity.
* Confirm final clean worktree and hosted CI result.

Current continuation status: the fail-closed CMake guard is committed in
`f72d3c60e602a94bb9e5a3a77537941116322609`. A fresh tests-enabled build with
trace ON passed trace-equivalence; a fresh tests-enabled build with trace OFF
fails at configure as required; a tests-disabled build with trace OFF configures
successfully. The trace implementation is now compiled only when trace is ON,
and tests-disabled symbol inspection shows no `upd9002_trace_*` or SSTS/test
seam symbols in the production executable. The pinned M43 CI/full baseline tests and M42 dispatch graph tests
both pass in the trace-enabled build. The detached G41 tree contains no
cross-version state probe equivalent to the M44 probe, so the full bidirectional
G41 matrix remains unexecuted and is a G44 blocker rather than an inferred pass.

## Trace no-op side-effect audit

All call sites in `sdl2/np2.c`, `i286c/i286c.c`, `i286c/v30patch.c`, and
`i286c/memory.c` were inspected. Trace arguments are constants, casts,
already-computed locals, or direct non-volatile reads. No argument contains an
increment/decrement, function call, assignment, volatile read, clock/queue/
memory/I/O mutation, or state update. Values needed by emulation are computed
before the trace call. The OFF build removes the implementation source and
uses no-op macros, while the ON build passed trace equivalence; no required
production side effect is suppressed.

The trace build's standalone ABI/state/payload targets were subsequently built
explicitly. The combined CTest selection (`abi`, `state_boundary`,
`state_payload_probe`, and `trace_equivalence`) then passed 4/4. No baseline
bytes changed.

## Additional validation status

Clang Debug configured with trace enabled and the three state targets passed
3/3. Clang AddressSanitizer/UndefinedBehaviorSanitizer built all three targets;
CTest passed 3/3 with `ASAN_OPTIONS=detect_leaks=0`. LeakSanitizer itself cannot
run under this sandbox's ptrace restrictions and is recorded as an environment
limitation. MinGW cross-configuration and the three state targets built
successfully. Wine execution was attempted with a dedicated prefix but failed
before test startup because wineserver could not create its runtime directory
under the restricted filesystem. Hosted CI, native Windows/macOS, and a full
detached G41 bidirectional matrix remain unavailable and are not claimed.

## Temporary G41 probe attempt

The exact G41 source is `pre-upd9002-series^{commit}` =
`dc8a72da974f0ea328613e480f1de662c28f4436`. A disposable `/tmp` tree was
created from that commit and the minimum M44 probe sources were copied without
committing or changing the G41 object. The combined copied-file patch input
digest is `537268e849a85aa4e7f6cd2e53a151e8378abce3c28f9558a1068fd0095b38ea`.

The probe cannot be compiled against G41: the G41 tree has no
`Cpu286StateCompat`, `Cpu286CompatImage`, or `Upd9002RuntimeState` definitions;
the compile fails on those missing types. Completing the probe would require
importing the M44 ABI/runtime ownership implementation, which is precisely the
state-boundary change under test. No valid G41-to-M44 or M44-to-G41 matrix is
claimed.

The M44 state tests provide deterministic rejection and opaque-byte coverage,
but do not expose per-region digest output for runtime, compatibility image,
live CPU, UPD9002, and unrelated machine state. A digest-backed atomicity
matrix and hosted/Wine execution therefore remain G44 blockers.

## Version-neutral container inspector

`tools/qa/statsave_sections.py` was added as a state-type-independent inspector
and same-size payload mutator. It parses the 48-byte container header and
16-byte section headers, validates padding and truncation, rejects duplicate or
missing `CPU286`/`UPD9002` tags, reports declared/actual sizes and SHA-256, and
only permits replacements with an identical declared size. A synthetic opaque
container passed inspection and replacement validation. No emulator state type
is imported. No real G41/M44 containers were available to feed the tool, so no
round-trip digest rows are claimed.

## Final black-box input audit

At HEAD `93ec1bf`, a repository-wide scan for `*.sav` and `*.state` inputs
found no state containers. The committed M42 fixtures are extracted CPU286 and
UPD9002 payload text, not complete state files, and cannot validate section
headers, padding, duplicate tags, or container metadata. No private state file
was opened or imported. Consequently the requested G41-to-M44 and M44-to-G41
round trips, opaque section mutation, and external section digest matrix cannot
be executed without an approved state-file artifact. Wine and hosted CI remain
environmental blockers. This report records the limitation rather than
fabricating matrix rows or changing any baseline.

The corrected black-box approach was evaluated. No G41 or M44 state files are
available in the repository or permitted public inputs, and the state container
header/section metadata is tied to the emulator's `NP2FHDR` implementation.
Therefore no external payload matrix can be honestly populated in this session
without private state artifacts or a new version-neutral extractor. No private
state was opened, copied, or committed, and no G41 source was modified.
