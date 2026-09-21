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
# REC03 bounded recording queue and fake worker

Status: PASS

Feature ID and mapped legal M ID: REC03 / M100r4
Assigned branch: topic/m100r3-recording-clock
Base SHA: f0903fd6
Evaluated source SHA: e7f53b3e1f245dd9f546ba8cf16e46ccaf30469b
Evaluated binary/toolchain identity: macOS arm64, AppleClang 21.0.0.21000101,
CMake 3.31.12, Ninja 1.13.2, MacPorts SDL2 2.32.10
Evidence-only SHA: Not applicable
Task/spec version: docs/recording-v1/milestones/rec03-queue.md

## Scope and changes

REC03 implements synthetic, frontend-only ownership and queue/lifecycle
infrastructure. It deliberately does not connect the queue to the guest video
raster, OSD, sound producer, SDL/GPU presenters, FFmpeg or a user-facing
recording command.

Changed files:

* `sdl2/recording/recording_queue.hpp` declares owned `CaptureSlice`, the
  bounded queue, worker/sink boundary and session state machine.
* `sdl2/recording/recording_queue.cpp` implements checked ownership copies,
  descriptor/sequence/endpoint admission, byte/count reservations,
  condition-variable wakeups, a non-detached sink worker and lifecycle
  transitions.
* `tests/frontend/recording/test_recording_queue.cpp` exercises producer
  mutation isolation, queue bounds, reserved capacity, distinct admission
  errors, cancellation wakeup, slow finalization, failure and repeated Stop.
* `CMakeLists.txt` links the frontend and focused test to the platform thread
  target and registers `vaeg_recording_queue` under the recording/queue label.
* The REC03 task wrapper, ID map, status, handoff and report record this gate.

The exact two-source Native/Displayed and guest-area OSD contract is unchanged.
No C99 core interface receives C++ containers, thread objects or FFmpeg types.

## Implementation findings and decisions

`CaptureSlice::create` copies the canonical video storage described by the
existing descriptor and interleaved stereo S16 samples into private vectors.
The queue accepts only one contiguous sequence: sequence IDs, guest interval
endpoints, audio sample starts/counts, descriptor values and descriptor
generation must match the configured next values. A zero-length interval is
rejected; zero audio frames remain valid for a nonzero guest interval.

The queue budget accounts for `active_interval_bytes` and
`readback_staging_bytes` before queued slice bytes. Admission is strictly
nonblocking and returns `WouldBlock` at either the count or byte limit. The
worker drains accepted slices after producer close. Cancellation clears pending
work and wakes both data and space waiters; sink errors wake producers and keep
the session terminal until explicit acknowledgement.

The session has explicit Preparing, Armed, Recording, Finalizing, Canceled and
Failed states. Stop from Preparing is safe, Stop from Armed/Recording closes and
drains, repeated Stop is harmless, and a blocked sink exposes Finalizing while
the caller waits outside the queue lock. The worker owns its sink and is always
joined; it is never detached.

## Checks

| Check ID | Exact command/environment | Exit code / outcome | Evidence |
|---|---|---|---|
| REC03-CONFIG-BUILD | `cmake -S . -B build/macos-rec02 -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/opt/local -DVAEG_ENABLE_TESTS=ON -DVAEG_Z80_COMPAT_INTEGRATION_TRACE=ON -DVAEG_ENABLE_LIBRASHADER=OFF -DVAEG_ENABLE_RECORDING=OFF`; `cmake --build build/macos-rec02 --target vaeg_recording_clock_test vaeg_recording_c_contract_test vaeg_recording_contract_test vaeg_recording_queue_test vaeg_sdl2 -j2` | 0 / PASS | Focused tests and frontend linked on current source |
| REC03-CTEST | `ctest --test-dir build/macos-rec02 --output-on-failure -R '^vaeg_recording_(clock|c_contracts|contracts|queue)$'` | 0 / PASS | 4/4 tests passed |
| REC03-SANITIZER-CONFIG | `cmake -S . -B build/macos-rec03-sanitize -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/opt/local -DVAEG_ENABLE_TESTS=ON -DVAEG_Z80_COMPAT_INTEGRATION_TRACE=ON -DVAEG_ENABLE_LIBRASHADER=OFF -DVAEG_ENABLE_RECORDING=OFF -DCMAKE_C_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'`; `cmake --build build/macos-rec03-sanitize --target vaeg_recording_queue_test -j2` | 0 / PASS | Sanitizer queue binary built |
| REC03-SANITIZER-QUEUE | `ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 ctest --test-dir build/macos-rec03-sanitize --output-on-failure -R '^vaeg_recording_queue$'` | 0 / PASS | Queue ownership/lifecycle test passed under ASan/UBSan |
| REC03-ROMLESS | `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/macos-rec02/sdl2/vaeg --selftest --no-cfg --no-bkupmem`; `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/macos-rec02/sdl2/vaeg --smoke --no-cfg --no-bkupmem` | 0 / PASS | Existing self-test all passed; smoke completed with expected missing-ROM warning |
| REC03-REPOSITORY | `python3 tools/qa/milestone_ids.py --selftest --audit --discover`; `python3 tools/repo/check_case.py`; `python3 tools/repo/check_encoding.py --expect utf8`; `python3 tools/repo/check_eol.py --enforce` | 0 / PASS | 49 strict milestone checks; 0 case, encoding and EOL findings |
| REC03-SCOPE | `git diff --cached --check` before implementation commit | 0 / PASS | No whitespace errors in evaluated source commit |

The first sanitizer invocation used `ASAN_OPTIONS=detect_leaks=1`; Apple’s
AddressSanitizer reported that leak detection is unsupported on this platform.
It was rerun with leak detection disabled and passed. This report does not
claim a leak-detector result.

## Integrity results

The bounded fixture reserves 64 bytes for active/readback staging, admits a
12-byte owned slice under a 128-byte total budget, and returns `WouldBlock` at
the one-slice count limit. Accepted normal slices are drained and counted by
the fake sink; a failing sink reports `SinkFailure` and leaves the session
failed rather than claiming success. Mutation checks observe original copied
video and PCM values after producer buffers change.

No movie, encoded PCM, FFV1 round trip, guest-time capture, OSD pixel result,
GPU readback, VLC playback or platform runtime evidence is applicable to this
synthetic checkpoint.

## Review and limitations

The queue has no unbounded container growth, detached thread, file operation,
audio callback wait or GPU lock. All blocking waits use condition-variable
predicates and explicit close/cancel/failure wakeups. A sink that blocks inside
its own write cannot be forcibly interrupted; the worker remains joinable and
the session exposes Finalizing until the sink returns.

The queue is not yet connected to live production. Audio look-ahead ownership,
Native/Displayed taps, OSD composition, FFmpeg capabilities and actual target
platform behavior remain later work. Windows, Linux and macOS GPU recording
runtime evidence is not required for REC03 and remains NOT_RUN.

## Gate and next task

Machine gate: PASS. REC03's required queue, lifecycle, build, regression,
sanitizer and repository checks passed on the evaluated source SHA.

Human gate: NOT_REQUIRED for REC03. No human or release validation is claimed.

Push/remote state under repository policy: local only; no push, merge or
publication was performed.

The next dependency-ready task is REC04/M100r5. Under the current
recording-v1 authorization, continue automatically after this report is
committed while preserving the declared gate semantics.
