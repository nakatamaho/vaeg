# M44 uPD9002 serialized/runtime state boundary

Status: implementation complete; G44 is not declared by this report.

## State

Branch: `topic/m44-upd9002-state-boundary`  
Starting SHA: `91ec9a4c998928523360c37dab8d6ade8e698731`  
Current SHA at report preparation: `44aa172d7bfe1ba2fca65c7ba9eac2ad791fd82b`  
Baseline tag: `pre-upd9002-refactor` -> `91ec9a4c998928523360c37dab8d6ade8e698731`.

The worktree was clean before the report change. The branch descends from the
approved M43 SHA. No M45 work was started.

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
