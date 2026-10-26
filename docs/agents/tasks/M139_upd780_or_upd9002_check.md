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

# M139 — uPD780 or uPD9002 identification investigation

Status: findings accepted; implementation authorized by the maintainer's
subsequent request to select uPD9002 on VA/VA2 and uPD780 on other machines.
CPU-role implementation and local unit/state validation complete. Final G139
is BLOCKED: the private guest passes CPU identification but stops later; demo
compatibility, media regression and clean MinGW handoff are not accepted.
Branch: `topic/m139-upd780-or-upd9002-check`.
Base: `5d4dafffd4e1fe07422f940a6e51dea09b677ab5`.

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
