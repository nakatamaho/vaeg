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
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# M142 — gate preparation and actual full-state checks

**M142 IN_PROGRESS; G142 NOT_READY.** This advances the
[task](../tasks/M142_runtime_portability.md), not human acceptance. No merge,
release, hosted CI or additional physical measurement is performed.

The later [fresh demo/media follow-up](m142_demo_media_regression.md) adds nine
distribution closures, 34 drawing cases, MANDEL progress and the self-boot SGP
menu. It retains utility generation/boot as BLOCKED/NOT_RUN and does not change
the evaluated worker/build identities or historical results below.

Subsequent [utility closure](m142_utility_media_regression.md) completes required
FDD/both SASI generation, readback and four standalone boot contracts. The later
[authorized save-restoration correction](m142_save_portability_audit.md#authorized-restoration-correction-and-verification)
changes application/format behavior: source `0f3cbe20`, fresh complete
**155 PASS / 16 SKIP / 0 FAIL**, and actual earlier-file loads with BMS-setting
mismatches. Old SAV compatibility is intentionally removed. The identities,
166-entry recovery and old format/sizes below are historical and are not
relabelled as corrected-source results. Windows/Linux exchange, actual
presenter checks and G142 acceptance remain unestablished.

## Identities and boundaries

- Native worker: clean pushed source
  [`0d02bc849b5c4fb4f45d21c13716735e77578d37`](https://github.com/nakatamaho/vaeg/commit/0d02bc849b5c4fb4f45d21c13716735e77578d37),
  SHA-256 `24f53861e4a81f0914d489c353921113cc81e0aa1f730f00b9a9b07deea70700`.
  Release QA build, recording/static minimal FFmpeg ON, bundled encoders OFF,
  shaders OFF, SDL dummy. This is not a personal desktop or presenter build.
- Final reusable content validator:
  [`ea62eda1eb2d1d3abed11f72af2e5e7899868652`](https://github.com/nakatamaho/vaeg/commit/ea62eda1eb2d1d3abed11f72af2e5e7899868652).
- Complete Windows cross-build: fresh clone and application objects at the
  same `ea62eda1` source, audited existing isolated SDK only. Recording/shaders
  ON, native Media Foundation MP4 support, bundled encoders OFF; no Colima,
  port installation or global toolchain changes. Exit 0; encoder archive,
  system-DLL imports and embedded identity audits PASS.
- Windows display: `Rel.20261030 (M142-dev, ea62eda1)`; executable SHA-256
  `d9c3bbdc36d88e5cc23a3587f2cbe7870594af2c55634dfe97e4e6f32b2b132a`.
  `vaeg.exe`, `BUILD-REPORT.md`, identity/SDK/import evidence and notices are
  retained together in the local `mingw-gate-prep/output/` handoff directory.
  Windows execution remains NOT_RUN. Later changes are test scheduling and
  documentation, not new application objects; this artifact is not relabeled.

The Windows build command (exit 0) is:

```sh
python3 tools/release/build-mingw-macports.py \
  --commit ea62eda1eb2d1d3abed11f72af2e5e7899868652 \
  --work-dir "$SCRATCH/mingw-gate-prep" --sdk-dir "$SCRATCH/mingw-sdk" --jobs 4
```

## Actual native full-state and process-restart contract

[`runtime_state_roundtrip.py`](../../../tools/qa/runtime_state_roundtrip.py)
uses the bounded `save-state ID` harness action and production load/save paths.
For each profile it boots the source-built V128A program, saves before Q,
captures its subsequent map-entry registers and TVRAM/GVRAM, advances the guest,
loads the saved state and compares the same subsequent guest point. It also
saves the restored state and restarts a separate worker with the same INI.
These are actual state files, not just a serializer buffer or screen image.

| Existing INI condition | EXTMEM payload | BMSMEM payload | Full baseline state bytes |
|---|---:|---:|---:|
| Both expansion keys absent | 0 | 0 | 2111040 |
| Explicit EMS/BMS OFF | 0 | 0 | 2111040 |
| EMS 13 MiB, BMS ON/128 units at 01D0h | 13631488 | 16777216 | 32519776 |

All **three continuation checks and three process restarts PASS** under this
contract. Native registers, TVRAM and GVRAM agree after the actual load. Clock
samples also happen to agree in all three cases; cycle-exact prefetch-queue
restoration is expressly outside the contract. DISK v1 remains 41080 bytes.
Disabled expansion sections are omitted; the enabled state adds 29 MiB plus
two 16-byte section headers: exactly **30408736 bytes**. Explicit settings
remain present/effective on restart; absent settings remain valid without
forcing a configuration rewrite. This does not test changing/saving controls
through the GUI, all memory sizes, all media types or Windows/Linux exchange.
The restart workers above check configuration/expansion persistence after a
fresh boot; they do not load the earlier saved file in that new process.
The subsequent [save audit](m142_save_portability_audit.md) adds four explicit
new-process prior-file loads under its own source/evidence identity, without
retroactively enlarging this earlier contract.

A separate passing two-save fixture precedes two single-mutation integration
negatives: pre-existing destination and invalid/path ID. They reject with
exact `M142_DEBUG_STATE_EXISTS` / `M142_DEBUG_STATE_ID`; the existing state is
byte-identical afterward and no escaped file is created. The C harness also
rejects six invalid/device IDs from passing-ID controls. The independent
Python content selftest has **10 exact-code single-mutation negatives**:
`python3 tools/qa/runtime_state_roundtrip.py --selftest` exits 0. The focused
`vaeg_runtime_state_contract` CTest also exits 0 (1/1).

### Preserved failures and explicit reuse

The initial integration exposed three validator assumptions, not guest-state
failures: zero-sized expansion sections need not exist, unchanged absent INI
keys need not be written on exit, and enabled expansions add section headers
as well as RAM. The failed invocations remain recorded as exits 1 with
`M142_STATE_EXPANSION_SIZE`, `M142_STATE_CONFIG` and
`M142_STATE_ENABLED_SIZE_DELTA`. The correction is `ea62eda1` above.

All six profile worker runs had completed before the third validator rejection.
Their worker/output hashes were rechecked; the corrected content functions
were applied to registers, captures, three states per profile and INI values.
Finalization then ran the passing save fixture and two rejection workers.
The final independent content recheck exits 0. Intermediate runner/finalizer
sources are retained and match their previously recorded digests. This is
**completed executions plus corrected validation/reuse**, not a claim that the
original all-in-one invocation exited 0. No unchanged guest profiles were
rerun merely to replace a failed report. Private inputs, states, identities,
raw captures, command transcripts and failed/successful logs remain out of Git.

## Full local CTest and focused recovery

`ctest --test-dir "$SCRATCH/native" --output-on-failure -j4` ran all **166**
entries: **148 PASS / 16 SKIP / 2 FAIL**, exit 8. Exact failures were retrieved:

1. Milestone discovery interpreted the current roadmap's hardware-bank label
   `G1/long-blank` as a gate ID. Use unambiguous `GVRAM/long-blank`, without
   weakening the gate validator or changing historical accepted gates:
   [`f8b570127a9d7656bd19fda6c21f8d50b788a0c4`](https://github.com/nakatamaho/vaeg/commit/f8b570127a9d7656bd19fda6c21f8d50b788a0c4).
2. The complete romless worker stopped in the dropmedia selftest. The trace
   equivalence test was concurrently launching complete selftests without the
   lock already shared by the other two full-selftest CTest entries. Those
   processes use the same destructive dropmedia temporary directory. Extend
   the existing resource lock to trace equivalence:
   [`cb64067b520c1a3535c24122156afb858489868e`](https://github.com/nakatamaho/vaeg/commit/cb64067b520c1a3535c24122156afb858489868e).
   Shared scratch is a demonstrated concurrency hazard; the initial log does
   not identify the exact failing archive assertion, so no narrower root cause
   or guest archive-handling correction is claimed.

Regenerate CTest definitions without rebuilding/relabeling the unchanged
native worker, then run the affected set with `-j4`:

```sh
ctest --test-dir "$SCRATCH/native" --output-on-failure -j4 \
  -R '^vaeg_(milestone_id_selftest|romless_tests|upd9002_trace_equivalence|m75_transfer_info_compiled)$'
```

Exit 0, **4/4 PASS**, 148.73 seconds. The three complete-selftest entries now
execute serially under the shared lock. Aggregate **150 PASS / 16 SKIP / 0
FAIL with unchanged-worker reuse**, not a second fresh whole-suite run.
Skips remain one H.264 hardware probe, one SST corpus, six assembled FDC
payload tests, two assembled main-CPU probes, private BASIC boot and five
xz shell/generated-data cases; none becomes PASS. No hosted CI was requested.

## Remaining gate blockers and not-run scope

- **#75 mechanism OPEN / correction NOT_IMPLEMENTED.** The additional
  [REP alignment accounting](m142_tvram_vb_events.md#rep-alignment-accounting)
  explains a software-side GVRAM cost difference, not the physical TVRAM
  limiter. No wait, VB duration/phase, REP core or VA hold was tuned.
- **Actual Cocoa FPS/layout capture BLOCKED:** CoreGraphics reports zero
  active displays; screen-capture permission and accessibility trust are both
  false. No approved fixed-target final-client reference/capture is supplied
  by dummy SDL. This does not transfer routine visual QA to the maintainer.
- **Windows runtime and Windows/Linux state exchange BLOCKED/NOT_RUN:** the
  cross-compiler is available, not an executable Windows test target. No
  claim about actual Windows presentation or bidirectional state exchange.
- Full clean-clone utility/demo media rebuilds, payload comparisons, all live
  demo/OS cases and MANDEL remain **NOT_RUN for this checkpoint**. Do not reuse
  historical G139 results as new M142 passes. These checks still block G142;
  the complete behavior candidate is not final while #75 is unresolved.

The state/defaults coverage above and complete Windows build are useful
advances, but they do not meet all M142 requirements. **No human gate, merge
or release is requested or inferred.**
