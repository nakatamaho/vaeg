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

# M139 — main CPU roles and FDC uPD780 conformance

Status: findings accepted; implementation authorized by the maintainer's
subsequent request to select uPD9002 on VA/VA2 and uPD780 on other machines.
CPU-role implementation and local unit/state validation complete. Final G139
is BLOCKED: the private guest passes CPU identification but stops later; demo
compatibility, media regression and clean MinGW handoff are not accepted.
The maintainer additionally requested FDC-uPD780 ZEXALL, instruction-difference
documentation/upstream implementation and speed measurements in this milestone;
the hardware-facing harness/images and speed kernels are now implemented and
software-validated; physical execution/reference approval/measurements remain
NOT_RUN, and no NEC-specific upstream correction is established.
Branch: `topic/m139-upd780-or-upd9002-check`.
Investigation base: `5d4dafffd4e1fe07422f940a6e51dea09b677ab5`.
Implementation source: [3053ee657eecf0c20aca3b967fa864620b1a2305](https://github.com/nakatamaho/vaeg/commit/3053ee657eecf0c20aca3b967fa864620b1a2305).
The development worker was built from the implementation working tree before
that commit; its embedded base identity is not relabeled as a clean build.

## Implementation authorization

The maintainer accepted the diagnosis and explicitly extended M139 to automatic
CPU-role selection: VA/VA2 use uPD9002 native/compatible execution plus the FDC
uPD780; other selected machines use a separate main uPD780 plus the FDC uPD780.
The two uPD780 instances share instruction semantics, not registers, buses or
clocks. Preserve VA native firmware bootstrap/device bridging where the existing
board implementation needs it; do not expose VA-only instructions as guest
uPD780 instructions. Validate role selection, flags, timing, reset and real
state-file round trips, then reproduce the unmodified guest progressing beyond
its rejection. The standard implementation/media/clean-build gate applies;
no gate acceptance is inferred from the investigation's PASS results below.

## Additional authorization: physical FDC uPD780 conformance and speed

The maintainer explicitly notes that the uPD780 available for this physical
measurement is in the FDC. Running CP/MVA on the uPD9002, a main-CPU guest on
an emulated non-VA model, or a flat-memory host runner does **not** measure
that physical uPD780. Keep the existing software-role selection and its
validation separate from silicon identification and physical measurements.

Deliverables, all still open unless an explicit result below says otherwise:

1. Preserve the stock 67-group ZEXALL host baseline with the production
   `Upd780ConfigureCore`, pinned corpus and expectation identities. This tests
   the current Zilog-model contract, not NEC hardware. Preserve/reuse matching
   completed expensive evidence rather than rerunning unchanged workers.
2. Establish a safe RAM upload, launch, result retrieval and recovery protocol
   on the actual FDC. The current emulator maps 8 KiB ROM at 0000h–1FFFh and
   16 KiB RAM at 4000h–7FFFh; this is source evidence, not an independently
   verified physical memory specification. The prior RAM-upload observation
   in [M103i](M103i_v1_mode.md#v1-a-demonstration-disk-io-error-at-its-second-bload-real-va-too)
   is a lead, not a complete execution/recovery contract. Respect live firmware
   work areas, stack and mailbox ownership. Do not patch or replace ROMs, and
   do not conduct drive writes during a diagnostic takeover. A main-CPU loader
   may save returned records only after the normal FDC service is restored.
3. Build a reproducible FDC-resident ZEXALL adaptation, or explicitly classified
   split variants if necessary. Stock ZEXALL assumes writable low memory,
   CP/M vectors/stack and the fixed `msbt` state address. Verify every generated
   read/write/fetch and stack footprint fits the usable FDC map. Reassembly or
   relocation can change register/operand addresses, undocumented flags and
   CRCs: never classify a relocated/stock CRC difference alone as a NEC rule.
   Use byte-identical target images on hardware and host, retain stock/ported
   coverage correspondence, independent Zilog reference checks and all 67
   group outcomes. Preserve ZEX's GPL notices; do not modify the vendored
   original or silently reduce masks/vectors to obtain a match.
4. Add focused flag, undocumented-opcode and boundary probes where ZEXALL has
   known gaps. Use explicit initial states and complete final-state records;
   observed crashes/incomplete output are not successful group comparisons.
5. Measure speed with separate bounded, RAM-resident kernels. Report ZEXALL
   elapsed time separately from instruction timings. Keep upload, CRC/log
   output and disk I/O outside the timed interval; calibrate completion and
   notification overhead with the same empty path. Use several iteration
   counts, repetitions and timer resolution/error bounds to separate slope
   from fixed overhead. Include register/ALU, memory, taken/untaken branch,
   stack and prefix cases, without uncontrolled device accesses. Use an
   independently specified board timer/reference, not the emulated CPU's
   nominal MHz or host wall time. Check main H/S/CPU-speed changes alter the
   observer overhead rather than being mistaken for FDC-clock changes.
6. Document observations separately for physical FDC uPD780, physical uPD9002,
   stock Zilog expectations and host models. Reduce each demonstrated mismatch
   to an isolated regression before changing suzukiplan. Work in an out-of-tree
   upstream checkout and reproduce vendored changes through the pinned
   provenance/patch procedure; never hand-edit `external/`. Preserve Zilog and
   uPD9002 behavior and keep any NEC rule per-instance. FDC measurements alone
   do not identify every non-VA main-CPU silicon variant or authorize silently
   applying newly found FDC quirks to another role through the shared helper.

Physical-FDC execution, physical reference approval and hardware recovery remain
**BLOCKED / not established**. No real uPD780 result, speed figure or upstream
correction is claimed. The executable harness is **implemented** and its mapped
host/ROM-backed software integration is described below, not hardware accepted. See the
[Z80-family QA contract](../../modernization/qa/z80-family.md#m139-fdc-upd780)
for the comparison and timing distinctions. This extension does not remove the
existing red-screen, media-regression or clean-build gate requirements.

### Implemented FDC controller, mapped images and speed kernels

The maintainer subsequently requested implementation and supplied private input
`fdc-cpm-01`. [Build/run instructions](../../../tools/hardware/upd780/README.md)
and [canonical monitor ABI analysis](../../bios/fdc_monitor.md) accompany the
new reproducible tool. It adds `FDCONE.COM`, `FDCPRB.COM`, `FDCZEX.COM`, help
and the pinned GPL license to a **copy**, preserving all original files.
The existing main-CPU ZEXALL remains untouched and is not FDC evidence.

- The real monitor transport uses commands 0Bh/0Ch/0Dh and its output/command
  service vectors, not test-only RAM injection. Save 4000h–7EFFh, upload the
  bounded image and zero the rest of that diagnostic range for deterministic
  initial RAM, stop motors during execution, receive records, restore and
  byte-verify the saved range, then permit BDOS log creation. ROM and the live
  7Fxx work/monitor-stack area are not replaced. An unknown-state error halts
  without disk I/O and requires reset. The main controller owns its own stack
  and restores original SP/default DMA on ordinary return.
- Full mapped ZEX retains all 67 groups, flag masks and scan vectors. The
  explicit descriptor-base operand change 0339h → 7339h avoids an FDC-ROM
  write; this and relocation change the comparison contract. Every actual
  CRC is printed alongside `STOCK=`, never relabeled as stock conformance.
  `FDCONE` is separately labeled NEG-only smoke, not full coverage.
- The AF probe records requested/readback AF and full BIT/AND/SCF/CCF results
  for 256 input pairs. Further focused undocumented-opcode/boundary cases,
  independent mapped Zilog controls and physical NEC rules remain open.
- Eleven separate timing bodies use 60000×N iterations, N=1/4/16, two
  repetitions, plus an empty control: 67 timing records. They cover register,
  ALU, memory, loop branch, stack pair, indexed BIT, SCF/CCF and NOP/NOP8.
  This is not a complete independent taken/untaken opcode matrix. Both VRTC
  edges are counted on the main CPU; no reference Hz or physical MHz is
  guessed. Affine slopes, a branch baseline and explicit nonuniform-edge
  quantization bounds separate loop/notification overhead. An independently
  approved physical timer contract is still required for absolute conversion.

The mapped worker, private-input runner and content/allocator negatives are
independently callable. Software integration checks actual upload/launch,
completion, verified recovery, persisted complete records, unchanged existing
file contents and the post-return DIR/prompt; zero exit or a screenshot alone
is insufficient. Current exact identities/results are recorded in the follow-up
validation record. These additions do not resolve the broader blocked G139.

### FDC implementation software-validation record

Evaluated hardware-tool source:
[aa76276241476d317a323c428256336d769117fc](https://github.com/nakatamaho/vaeg/commit/aa76276241476d317a323c428256336d769117fc).
The final private handoff is generated from that pushed commit and is
byte-identical to the tested working image, all five added files and all three
subsystem BINs. Its image/private-input digests, exact commands, assembler
identity and raw captures remain outside Git. No ROM, original input media or
vendored instruction implementation is changed.

Commands (neutral explicit bindings):

```sh
python3 tools/hardware/upd780/build.py --source-disk "$PRIVATE_INPUT" \
  --output-dir "$HANDOFF" --cache-dir "$ARCHIVE_CACHE"
cmake --build "$BUILD" --target vaeg_upd780_fdc_probe_runner
ctest --test-dir "$BUILD" -R '^vaeg_upd780_fdc_' --output-on-failure
"$BUILD/vaeg_upd780_fdc_probe_runner" "$HANDOFF/fdczex.bin"
python3 tools/hardware/upd780/integration.py --worker "$WORKER" \
  --roms "$STAGED_ROMS" --image "$WORKING_IMAGE" \
  --build-report "$RECIPE" --output "$CASE_OUTPUT" \
  --case probe --standard false
# Also probe/standard=true, smoke/standard=true, and an existing-log case.
```

Build and final focused CTest exit 0: **2/2 PASS**, 2.79 s; the content and
allocator suite has **16 passing tests**. The payload integration checks the
256-row independent Zilog AF formula, all 67 exact model-T timing rows, NEG
smoke and a passing mapped-write fixture with one unmapped-address mutation.
Content negatives isolate missing/duplicate/corrupt rows, completion/recovery,
inactive/nonlinear timers, invalid timer reference and terminal contracts;
allocator checks include second-half directory ownership, multi-extent text,
existing-name refusal and one geometry mutation. Error-code assertions use the
smallest content/map layer and do not depend on current Git history.

The production-configured mapped full ZEX worker exits 0 after **all 67 actual
CRC records** and `Tests complete`: 104.40 s host wall time, model clock
`46736000002`. This is **not 67 stock-CRC PASSes or physical timing**. No
out-of-map access occurs. Its output is byte-identical to the earlier exploratory
mapped worker's output; the production-configured worker was evaluated once
because that worker identity changed. The unchanged stock baseline above is
not rerun.

| Public software identity | SHA-256 |
| --- | --- |
| Production mapped worker | `4f8a14266448b76d82490d87dd48cc4733f42e8b46ff77b135647f9f49541fce` |
| Full mapped ZEX payload | `e674749a5caf28e83dc00cd8a1f2265f2db672e271516b90e3768a0039783e57` |
| AF/speed payload | `ac9bd9245aed4b9c767b5a878197770d4f861c13564d452e8569140f7cc8ef09` |
| NEG-only payload | `f854b305b214259b813ed736a4b8b10404bd121feb5bfdefd479c7d544446f7b` |
| Mapped full-ZEX stdout | `ee6271acc2df49b4e773227a728a06afa552d0eaaf5f72eaed1e54503073ee6d` |

Private-input integration uses the unchanged development application worker
`710171ed4aaf4ecab06d4224895b7c17ebf6230118b2254b1d7ffd07abad7bdd`
from the earlier implementation record. No emulator execution source changes
in this hardware-tool increment; a changed tool/document does not justify
repeating unchanged emulator builds or the earlier expensive tests. This
worker's embedded pre-commit identity remains explicitly **not a clean gate
candidate**. Target: VA2, 88-mode CP/M, CPU_Speed=100, isolated config/home,
no second FDD/SASI, SDL dummy video/audio, a ROM set matching MAME's `pc88va2`.

| Software case | Semantic result |
| --- | --- |
| AF/speed, standard=true (S) | Complete 256 AF/67 timing rows, zero model AF differences; recovered, saved, existing files unchanged, healthy DIR/prompt; PASS after contract-only revalidation below |
| AF/speed, standard=false (H) | Same checks PASS; no physical timing claim |
| NEG smoke | One explicitly classified CRC (`d638dd6a`), recovered/persisted, existing files unchanged, healthy DIR/prompt; PASS after terminal-contract revalidation below |
| Existing-log refusal | Refusal before takeover; all files/log bytes unchanged and DIR/prompt healthy; PASS |

S/H fitted slopes differ by at most **0.10 VRTC edges per 60000-iteration N
unit**, within the analytic 8/15-edge comparison bound. This checks observer
coupling in this software target at CPU_Speed=100; other CPU-speed settings,
physical H/S behavior, a physical reference frequency, real uPD780 speed and
physical recovery remain NOT_RUN. No dummy output is promoted to physical or
presenter/window coverage.

Development failures are preserved, not relabeled or discarded:

- The first mapped full run exposed the stock `LD r,n` descriptor's ROM write;
  the declared 0339h → 7339h operand port removes it without dropping a group,
  opcode vector or flag mask. This is a portability adaptation, not a NEC rule.
- A prototype borrowed the caller's main stack and produced unhealthy
  post-return DIR output. The final image-backed private stack and common DMA
  cleanup give a healthy return in the semantic cases. No untraced exact
  overwrite address or emulator root cause is asserted.
- An initial two-edge middle-residual contract incorrectly treated the two
  VRTC transitions as uniform. The independent nonuniform-phase derivation is
  ±2 edges per measurement, four-edge middle residual, **tighter** repeat
  bound 8/15. An analytic passing boundary fixture plus one-edge negative
  exercises that bound. The S run's original FAIL and exact original validator
  source are retained; unchanged digest-bound captures are revalidated rather
  than rerunning the guest.
- The initial terminal checker used BASIC's F3C8h layout instead of the
  fixture's F300h CP/M layout. Both original rejection records remain; the
  fixed-layout passing/one-mutation test and the same unchanged captures
  establish the corrected DIR/prompt verdict. Revalidation is a new report,
  not an overwrite of original failed evidence.
- Initial setup used a legacy assembler helper that placed a compiled helper
  in the shared archive cache. The new builder isolates compilation/executables
  in task output and uses the shared cache for locked archives only. The
  initially generated cached executable is retained pending explicit cleanup
  approval; no shared-cache deletion or private-input modification is performed.

Final repository checks exit 0: encoding, LF, case (0 findings), Markdown
comments (0 findings), 49-case strict milestone selftest, milestone audit,
new C++ clang-format and staged diff checks. The prior whole-manifest baseline
formatting failure is not promoted to PASS. No hosted CI or source-unchanged
full-emulator/media rerun is requested. The standard G139 remains BLOCKED by
the later private guest stop, physical/independent conformance work and the
uncompleted standard demo/media/clean-MinGW acceptance requirements.

### Stock host ZEXALL baseline result

The existing role runner is byte-identical to the preserved M138 runner
(SHA-256 below), but no reusable completed local role-ZEXALL run was found.
A bounded run is therefore performed once for this newly requested baseline.
Evaluated source: `24ad2f0c6e3a1d3c4a75bd90cca075121b00f331`;
subsequent scope/report edits do not change its build inputs or worker.

```sh
python3 tests/z80_compat/fetch_zex.py --output-dir "$CORPUS"
"$BUILD/vaeg_z80_compat_role_zex_runner" --role upd780 \
  --expect tests/z80_compat/upd780_zexall_expected.txt \
  --max-seconds 600 "$CORPUS/zexall.cim"
```

Both exit 0. The pinned stock corpus completes with **67/67 groups OK**,
`Tests complete`, and `PASS expectations=67 clocks=46740000000`.
The clocks are the runner's model accounting, not physical FDC timing.
Host wall time is 98.25 s and is **not** a uPD780 speed measurement.

| Evidence identity | SHA-256 |
| --- | --- |
| Host role runner | `40dc2655e2b139e0b0ba37fcf761fe8d48fc520287b21fd530f436fdd5b25aaa` |
| Stock ZEXALL corpus | `fbb1bb5d46f61c33ea6841a71f2b23c49b9b62410ce6ed4e57b7d9b2e7b437e0` |
| uPD780 role expectation contract | `82e2be848c4fdb4c7278d6cb504338d38c268b2b39d15639dbfd4fca8d3230c2` |
| Standard output | `f827ef78cbdbbf5dc35000a44e9fc74ac5662010ec61cce63f5dd50786529f67` |
| Standard error (empty) | `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` |

The exact command, worker/corpus/contract identities, flat-64-KiB host target
policy, exit status, duration and output digests are retained in task scratch.
This is **PASS for the current host Zilog-model contract only**. FDC-mapped
execution was subsequently implemented and software-validated as described
above; physical uPD780 matching and physical speed remain NOT_RUN/BLOCKED. No real-machine difference has been inferred from it.

## Implemented CPU roles

| Selected/applied machine | Main guest instruction core | FDC instruction core |
| --- | --- | --- |
| VA / VA2, including their extra-ROM entries | uPD9002 native and its independent uPD70008-compatible instance | independent uPD780 |
| PC-8801 / PC-8001 family | independent main uPD780 | independent FDC uPD780 |

`MainCpuBridge` retains the existing board firmware bootstrap/device bridge,
but owns separate compatible and main-uPD780 instances. The main uPD780 calls
exactly the FDC's `Upd780ConfigureCore`; it does not share the FDC's registers,
bus, clock counter or interrupt state. Selection uses the successfully applied
machine flags, not a pending configuration name; VA fallback after a failed
ROM selection therefore remains VA. Reset clears both main instances; state
load initializes the appropriate instance from the applied machine.

Non-VA guest instructions use ordinary core T states instead of M135's VA
cost table. H/S, CPU multiplier and speed remain in the main clock domain;
FDC timing is unchanged. Byte-bus accesses do not share the VA word-fetch wait.
VA-only CALLN/RETEM interception is disabled on the main uPD780; undefined ED
instructions use the same behavior as the FDC rather than M102's native-vector
reads. VA/VA2 retain their measured profiles and native transition handling.
This does not replace the entire ROM-substituted board with an independent
PC-8801 chipset or establish physical-device timing equivalence for its
remaining VA firmware services.

`MCPUROLE` is a new one-byte, version-1 state section. Preflight rejects a
present malformed section with `M139_MAIN_CPU_STATE_PAYLOAD`, or a different
current CPU family with `M139_MAIN_CPU_STATE_MACHINE_MISMATCH`, before any
machine state is reset. It identifies CPU family, not an exact ROM/machine
revision. The `UPD9Z80` name/layout remain unchanged. Legacy files without
`MCPUROLE` continue using the currently applied machine, so callers must still
select the same machine/ROMs; their missing identity cannot prove a match.

## Implementation validation

### Local build and tests

A new out-of-tree macOS Release build succeeds with recording ON, static
FFmpeg ON, local bundled MP4 encoders OFF and tests/integration trace ON.
It uses the already reviewed minimal recording SDK. Librashader is OFF in this
software-integration build; no CRT/GPU/presenter coverage is claimed.

```sh
PKG_CONFIG_PATH="$RECORDING_SDK/lib/pkgconfig" cmake -S . -B "$BUILD" \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_DEPLOYMENT_TARGET=26.6 \
  -DVAEG_ENABLE_RECORDING=ON -DVAEG_STATIC_FFMPEG=ON \
  -DVAEG_ENABLE_LOCAL_MP4_ENCODERS=OFF -DVAEG_ENABLE_LIBRASHADER=OFF \
  -DVAEG_ENABLE_TESTS=ON -DVAEG_Z80_COMPAT_INTEGRATION_TRACE=ON
cmake --build "$BUILD" -j 8
ctest --test-dir "$BUILD" -j 8 --output-on-failure
```

Build/configure exit 0. The initial configure omitted required integration
trace and correctly failed; enabling the required test instrumentation resolved
that setup failure without disabling tests. Full CTest exits 0: **138 PASS,
10 SKIP, 0 FAIL** (148 registered tests, 97.50 s). Skips are the unbound
private BASIC case, optional assembler probes/SST corpus/hardware encoder
probe and unavailable shell compression fixtures, not compatibility passes.

New `main_cpu_roles.c` fixtures exercise VA → PC-8801 → PC-8001 → VA → PC-8801
reset selection, BIT/H, four-T-state NOP, S scaling to eight clocks, S plus
50% speed to sixteen, undefined ED isolation and no CALLN/RETEM on uPD780.
The final H/S/speed fixture additions are followed by a focused rebuild and
CTest of `vaeg_upd9002_brkem_upd70008` and `vaeg_romless_tests`: both PASS,
exit 0 (30.26 s). Other emulator instruction inputs were unchanged. A final
post-format focused recheck also passes, exit 0 (30.27 s); the bounded guest
probe is repeated on that development worker rather than silently promoting
an earlier worker's private results.

New ROM-less integration cases write and load actual state files for both
CPU families, execute BIT after restoration, and compare FDC state before and
after main-CPU execution/restoration. Each cross-family negative first passes
preflight, changes only the current CPU role, requires the exact stable mismatch
code, and proves failed load did not reset the PC. Legacy same-machine files
with only `MCPUROLE` removed also load. Existing VA CALLN/RETEM, flags, timing,
FDC, state and recording checks remain in the full suite.

### Private guest: CPU identity PASS; demo drawing FAIL

The unmodified fixture is run with MA selected and staged inputs. A bounded
existing M74 debug script observes the guest's own check, rather than injecting
replacement identification code. It captures H=1 after BIT, and its trace
records the identification routine writing **16** instead of the original 0.
Thus the specific VA-rejection condition is corrected in actual guest execution,
not just in an isolated core probe. The capture then reaches a later guest
fatal-stop path. All scripts, raw traces/registers/captures, worker digest and
exact commands remain in task scratch outside Git. The final development
worker SHA-256 is
`710171ed4aaf4ecab06d4224895b7c17ebf6230118b2254b1d7ffd07abad7bdd`;
its bounded probe exits 0 with both H=1/result=16 checks passing and the
later stop captured. These are development-run results, not clean-candidate
or complete-guest acceptance. Earlier worker results remain separately bound
to their own digests.

The later screen remains solid red at frames 5400, 9000 and 12000. A bounded
trace establishes that the guest is in its own terminal self-loop, not merely
waiting for the next animation frame. Explicitly requesting three BASIC file
channels instead of accepting the boot prompt's default does not resolve it.
The cause of this later stop is **not established**; it is not attributed to
the FDC, ROMs, timer, I/O bridge or CPU timing without further evidence.
No disk/ROM patch or guest-check bypass was used. The staged disk remains
byte-identical to the original after these probes. This is SDL dummy/software
content evidence, not an actual presenter/window or physical-display check.

One diagnostic script attempt lacked an event-bound capture and another lacked
a pre-created output directory; both setup failures are retained. The corrected
bounded script exits 0 and captures both the successful CPU check and later
stop; zero exit is not promoted to a demo PASS.

### Implementation repository checks

Encoding, EOL, case, Markdown-comment, strict milestone selftest (49 cases),
milestone audit and `git diff --cached --check` all pass with exit 0. The full
clang-format manifest check exits 1 on pre-existing baseline violations. A
line-mapped comparison against investigation commit `f9dc961d` establishes zero
new formatting findings in the changed source files; the new test is fully
formatted with clang-format-mp-22. Unrelated baseline lines are not reformatted.
That is not a claim that the full formatter passes.

### Gate still blocked

Do not merge or call M139 complete. Continue investigating the later guest
stop locally; no routine screenshot inspection or diagnosis is delegated to
the maintainer. The implementation gate still requires private-guest drawing,
the standard VA/V3/demo/OS checks, bundled demo/media build regressions, and a
clean static MinGW handoff of the pushed candidate. Those checks have not been
replaced by the CPU/unit PASS results above. No hosted CI, release publication,
private-asset upload or milestone-directory cleanup is requested. Progress
commits use `[skip ci]` because this is not yet a gate-ready review candidate;
do not use hosted CI to investigate the private guest stop.

## Original investigation request and scope

Investigate why private fixture `ma-detect-01` rejects the selected PC-8801MA
machine as a VA. The maintainer proposes `upd780_or_upd9002_check` as the task
name; it is a working hypothesis, not an established defective subsystem.

- Identify the guest's rejection decision and its inputs.
- Separate ROM identity, hardware I/O identity, main-CPU instruction semantics
  and FDC behavior. The main uPD9002/uPD70008-compatible adapter is not the FDC
  uPD780C, even if guest identification distinguishes NEC CPU families.
- Examine the current M113 ROM-substitution policy and M101/M102 CPU profiles.
- Keep the original disk and private ROM inputs read-only. Put generated
  copies, traces, captures and disassemblies outside Git in task scratch.
- Publish independently written findings using neutral input identifiers;
  do not publish private asset paths, hashes, raw software or ROM excerpts.
- Do not patch the disk, bypass its check, alter emulation, change the release
  or start implementation. A later correction needs evidence and approval.

## Original investigation gate (findings accepted)

The original G139 checkpoint was human review of the findings and proposed correction boundary.
This report-only checkpoint is not an emulator-change/build acceptance gate.
Run repository encoding/EOL/case/comment and milestone checks for the report;
record actual executed probes separately from static deductions. Do not
request hosted CI or repeat unrelated expensive media/CPU tests for docs.

## Findings at the investigation baseline

### Conclusion

The rejection is a main-CPU flag-identity mismatch in the ROM-substituted
PC-8801MA configuration, not an FDC uPD780C failure. Fixture `ma-detect-01`
constructs a short CPU-identification routine in RAM, calls it, and rejects
execution when its result identifies the uPD9002-compatible flag behavior.
The relevant operation is `BIT` on the accumulator: the routine captures F
and tests H. There is no port access or ROM-identity read in that routine.
The BASIC caller tests the returned RAM byte; it does not ask the FDC to
identify the main CPU.

For this instruction, documented Z80 behavior sets H. M101's measured
uPD9002 rule R3 instead clears H. The current implementation deliberately
implements R3; that is correct for VA/VA2. However, selecting PC-8801MA through
M113 substitutes ROMs while retaining the VA main-CPU execution adapter and
its unconditional uPD9002 profile. Consequently software can still identify
the underlying VA CPU even while the machine selection says PC-8801MA.

This establishes the cause of this specific rejection, not the completeness
of a proposed PC-8801 CPU model or an instruction-by-instruction description
of every NEC Z80-compatible part.

### Static evidence

- [M113](M113_pc8801_machines.md) describes ROM substitution over the VA V1/V2
  implementation, rather than an independent PC-8801 hardware core.
- [`Upd9002Upd70008Compat::InitializeCore`](../../../cpu/upd9002_upd70008.cpp)
  calls the main-CPU configuration helper without a selected-PC-8801 profile
  branch. [`Upd9002Upd70008ConfigureCore`](../../../cpu/upd9002_upd70008_core_config.cpp)
  unconditionally selects `Z80CompatFlagProfile::kUpd9002`.
- [M101 R3](M101_upd9002_flag_profile.md#background) records the H=0 behavior
  for register and memory BIT instructions. This is not an M135 timing
  remainder or I/O-wait inference.
- [`Upd780ConfigureCore`](../../../cpu/upd780/upd780_core_config.cpp) separately
  selects the Zilog profile for the FDC instance. That configuration is not
  the main CPU used to execute this guest routine. As M101 explains, the FDC
  profile is not a claim that all NEC undocumented flags match Zilog.
- Static inspection of the private fixture established the identification
  operation and the BASIC rejection condition. Raw program bytes,
  disassemblies, directory listings and asset identities remain private.

### Executed evidence

**Independent CPU probe: PASS.** A scratch-only, independently authored
single-instruction probe runs `BIT 0,A` through the repository's shared CPU
wrapper. It tests A=00/01/80/FF and initial H=0/1, eight cases per profile.
The production main-CPU configuration returns H=0 in all eight cases; the
Zilog reference profile returns H=1 in all eight. Both make zero I/O calls.
No private guest code is compiled into this probe, and no production CPU
configuration or emulator source is changed.

```sh
c++ -std=c++17 -O2 -I. -Iexternal/suzukiplan-z80 \
  "$SCRATCH/profile-probe.cpp" cpu/z80_compat_cpu.cpp \
  cpu/z80_compat_state.cpp cpu/upd9002_upd70008_core_config.cpp \
  -o "$SCRATCH/profile-probe"
"$SCRATCH/profile-probe"
```

Both commands exit 0. The probe output is:

```text
reference-Zilog cases=8 H=16 IO=0 PASS
production-uPD9002 cases=8 H=0 IO=0 PASS
```

**Selected-machine guest reproduction: PASS for reproducing the defect, not
for demo compatibility.** Reused the published macOS arm64 Rel.20261025
worker from tagged application source
`4fb3c44052fcccab7e47830ca1945d1498469bb2` (worker SHA-256
`fadf82cad2fbb823b8a5e3281e3ddda8a70a0d3d2e41d2c57412afc350824293`).
A `git diff --quiet` of that tag against the evaluated base above, restricted
to `cpu/`, `io/`, `machine/`, `memoryva/`, `sdl2/` and the vendored shared
instruction core, exits 0. The relevant
emulator source is unchanged; a new build is unnecessary for this report.

The exact executed command, input bindings, worker/output digests and content
results are retained in task scratch. Its neutral equivalent is:

```sh
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy "$WORKER" \
  --roms "$STAGED_ROMS" --cfg "$LOCAL_CONFIG" --no-bkupmem \
  --extra-machine PC-8801MA --model va2 --nowait --mute \
  --fdd1 "$STAGED_MEDIA" --fdd2 none --sasi1 none --sasi2 none \
  --screen-tvram-dump "$OUTPUT/tvram.bin" \
  --screenshot "5400:$OUTPUT/rejected.png" \
  --headless-input-script "$PRIVATE_INPUT_SCRIPT"
```

The isolated config requests V2/H; the private script answers the disk BASIC
file-count prompt, starts the supplied program, and only after rejection
prints its existing identification-result byte. It does not patch memory,
change the disk, or bypass the guest check. The process exits 0 in 31.42 s.
Semantic checks require all of:

1. the startup log explicitly selects PC-8801MA;
2. the TVRAM contains the guest's VA-rejection message;
3. the subsequent BASIC query reports the identification result as zero;
4. BASIC has returned to its `Ok` prompt;
5. the worker exits successfully.

All five pass. This is guest-visible evidence using SDL dummy/software
output, not a presenter, Cocoa-window, physical GPU, or demo-drawing PASS.

Initial setup captures stopped at the disk BASIC file-count prompt or before
a scripted command's terminating Enter; they are not rejection evidence.
The first content check also treated the diagnostic FF cursor byte following
`Ok` as a printable CP932 character and failed the prompt assertion. Reusing
the same unchanged dump with the existing ASCII `screen_rows` decoder resolves
that checker error; neither the guest nor the expensive run was changed or
repeated for that correction. Both results and the original outputs are kept.

The original disk and ROM inputs were only read; the worker uses staged
copies and isolated config/home/output directories. A final byte comparison
of the staged disk against the original exits 0: the reproduction did not
change its contents either. No private outputs or identities are committed
or uploaded.

### Original proposed correction boundary — subsequently authorized

Do not remove R3 globally, special-case this program, patch its disk, or change
the FDC CPU. A later approved correction should give the selected PC-8801
main CPU appropriate instruction/flag semantics while preserving measured
uPD9002 behavior on VA/VA2. The Zilog baseline is sufficient to explain this
BIT distinction; it is not independently validated as a complete MA CPU model.

Before implementing a machine-aware profile, review the VA native/compatible
bridge used during ROM-substituted boot, reset and state restore, alternate
register/flag transfers, undefined-opcode/native-call handling (M102), and the
VA-specific timing model (M135). Merely flipping one H flag at the guest's
identification address would hide this symptom without correcting the model.

A correction gate must cover at least this unmodified guest starting and
drawing on selected MA, preserved rejection/VA flag behavior on actual VA/VA2
configurations, machine switch/reset/state restoration, and unchanged FDC
behavior. None of that post-correction acceptance is claimed here.

### Repository checks

Executed locally, all exit 0:

| Command | Output |
| --- | --- |
| `python3 tools/repo/check_encoding.py --expect utf8` | `0 violation(s)` |
| `python3 tools/repo/check_eol.py --enforce` | `0 violation(s)` |
| `python3 tools/repo/check_case.py` | `0 finding(s)` |
| `python3 tools/repo/check_md_comments.py` | `0 finding(s)` |
| `python3 tools/qa/milestone_ids.py --selftest` | 49 strict acceptance/rejection checks passed |
| `python3 tools/qa/milestone_ids.py --audit` | canonical identifier/tooling audit passed |
| `git diff --cached --check` | no findings |

Only this task, its roadmap row and the open-defect ledger entry are staged.
No emulator or private artifact is staged. No hosted CI, emulator rebuild,
media rebuild or new release is requested by this investigation-only gate.
