<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# REC02 exact recording clock arithmetic

Status: PASS

Feature ID and mapped legal M ID: REC02 / M100r3
Assigned branch: topic/m100r3-recording-clock
Base SHA: a36b9de404411e55e963550f4d2578e8febaea46
Evaluated source SHA: 8c40626f682d569668ba4ef574249f05498f8882
Evaluated binary/toolchain identity: macOS arm64, AppleClang 21.0.0.21000101,
CMake 3.31.12, Ninja 1.13.2, MacPorts SDL2 2.32.10
Evidence-only SHA: Not applicable
Task/spec version: docs/recording-v1/milestones/rec02-clock.md

## Scope and changes

REC02 adds exact integer/rational helpers for the future recording coordinator.
It does not add a recorder, FFmpeg/libav backend, worker, queue, video/audio
tap, CLI/UI command, or runtime capture.

Changed files:

* `sdl2/recording/recording_clock.hpp` declares checked guest-clock,
  sample-grid, wrap-extension and codec/muxer timestamp helpers.
* `sdl2/recording/recording_clock.cpp` implements those helpers with checked
  unsigned and signed 128-bit intermediates, explicit endpoint arithmetic and
  no floating-point accumulator.
* `tests/frontend/recording/test_recording_clock.cpp` covers multi-hour
  partition invariance, rational origin offsets, wrap behavior, invalid input,
  overflow and timestamp rescaling.
* `CMakeLists.txt` builds the helper with the frontend and registers the focused
  `recording;clock` CTest target.
* The recording workflow, REC goals/milestones, ID map, status, handoff and
  task wrapper now record the authorized automatic engineering progression and
  REC02 evidence. Human/release gate semantics remain unchanged.

Native and Displayed remain the only two eventual sources, both include enabled
guest-area OSD, and existing screenshots/QA outputs remain outside this change.
The helper is frontend-only, so the C99 core boundary and private-data rules are
unchanged.

## Implementation findings and decisions

The authoritative VA mapping remains the audited machine/TSP path: the normal
VA2 configuration uses the `pccore.realclock` derived from the model clock and
the TSP owns mode-dependent guest display timing. The frontend's nominal 60 Hz,
SDL callback cadence and host pacing are not exposed by the new API.

The synthetic test uses 48,000 samples per 1,001 guest ticks only to make
nonintegral partitioning and long-run drift visible. A three-hour interval
contains exactly 518,400,000 samples whether evaluated as one interval or as
17-tick pieces. Boundaries use `ceil` of the exact rational grid expression,
so adjacent interval counts telescope without per-frame rounding. A rational
origin offset is retained rather than reset at a recording boundary.

`WrapExtender` accepts a modulo counter, advances the epoch only across an
unambiguous forward wrap, and rejects small backward movement. Timestamp
rescaling takes explicit source and destination time bases and exposes floor
and ceil operations; it is not an FFmpeg type or dependency.

No production audio producer was changed. The existing callback/look-ahead
ownership finding remains open for REC10/REC11, as required by the recording
specification.

## Checks

| Check ID | Exact command/environment | Exit code / outcome | Evidence |
|---|---|---|---|
| REC02-CONFIG | `cmake -S . -B build/macos-rec02 -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/opt/local -DVAEG_ENABLE_TESTS=ON -DVAEG_Z80_COMPAT_INTEGRATION_TRACE=ON -DVAEG_ENABLE_LIBRASHADER=OFF -DVAEG_ENABLE_RECORDING=OFF` | 0 / PASS | Generated current-checkout macOS test build |
| REC02-CLOCK-CTEST | `cmake --build build/macos-rec02 --target vaeg_recording_clock_test vaeg_recording_c_contract_test vaeg_recording_contract_test -j2`; `ctest --test-dir build/macos-rec02 --output-on-failure -R 'vaeg_recording_(clock|c_contracts|contracts)'` | 0 / PASS | 3/3 tests passed; clock, contracts and romless labels |
| REC02-FRONTEND-BUILD | `cmake --build build/macos-rec02 --target vaeg_sdl2 -j2` | 0 / PASS | Frontend linked successfully; pre-existing unrelated compiler/linker warnings remain |
| REC02-ROMLESS | `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/macos-rec02/sdl2/vaeg --selftest --no-cfg --no-bkupmem` | 0 / PASS | Existing self-test reported all tests passed |
| REC02-SMOKE | `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/macos-rec02/sdl2/vaeg --smoke --no-cfg --no-bkupmem` | 0 / PASS | ROM-less smoke completed with expected missing-ROM warning |
| REC02-REPOSITORY | `python3 tools/qa/milestone_ids.py --selftest --audit --discover`; `python3 tools/repo/check_case.py`; `python3 tools/repo/check_encoding.py --expect utf8`; `python3 tools/repo/check_eol.py --enforce` | 0 / PASS | 49 strict milestone checks; 0 case, encoding and EOL findings |
| REC02-SCOPE | `git diff --cached --check` before commit | 0 / PASS | No whitespace errors in evaluated commit |

The focused test was initially corrected before the evaluated run: the
48,000/1,001 fixture reaches sample boundary 2 at tick 102, not tick 101.
The final evaluated test contains the independent arithmetic expectation.

## Integrity results

The three-hour synthetic schedule produced 518,400,000 samples for both the
whole interval and its changing 17-tick partition. No movie, PCM stream,
FFV1 output, PTS ledger, queue, readback or VLC playback was generated because
REC02 is arithmetic-only. Those fields are not applicable yet.

## Review and limitations

The implementation has no host-rate input, nominal-FPS input, floating-point
state, FFmpeg symbol, SDL/GPU object or C-core dependency. Overflow, zero
denominator, invalid range, ambiguous backward wrap and invalid modulo input
are explicit error outcomes. The full frontend build confirms the helper is
compiled as C++17 and the existing C99 contracts still build.

Windows, Linux, Metal, D3D11 and OpenGL recording runtime evidence is not
required for REC02 and remains NOT_RUN. Guest-time integration, audio
look-ahead reconciliation, bounded queues, encoding and all capture-source
proof remain later milestone work. No human or release validation is claimed.

## Gate and next task

Machine gate: PASS. REC02's required arithmetic, build, regression and
repository checks passed on the evaluated source SHA.

Human gate: NOT_REQUIRED for REC02. This is a machine-gated arithmetic task;
REC00's explicit human acceptance remains recorded separately.

Push/remote state under repository policy: local only; no push, merge or
publication was performed for this authorized run.

The next dependency-ready task is REC03/M100r4. Under the current
recording-v1 authorization, continue automatically after this report is
committed; do not interpret that progression as human approval or release
validation.
