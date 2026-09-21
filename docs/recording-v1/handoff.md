# Recording v1 handoff

Current state: REC02 machine gate PASS. Completed tasks: REC00/M100r1,
explicitly accepted by the maintainer on 2026-09-19, REC01/M100r2 and
REC02/M100r3. Assigned branch: topic/m100r3-recording-clock.

## REC02 completion

* `vaeg::recording::SampleGrid` computes exact sample-grid boundaries from
  guest ticks, integer rates and a bounded rational origin offset.
* Interval sample ranges are formed from absolute endpoint boundaries, so
  arbitrary producer partitioning telescopes to the same count as one whole
  interval. No nominal frame rate, host clock or floating-point accumulator is
  involved.
* Checked 128-bit intermediate arithmetic rejects zero denominators, invalid
  ranges and representable-result overflow. `WrapExtender` converts an
  audited modulo counter to monotonic guest ticks and rejects ambiguous
  backward movement.
* Explicit floor/ceil timestamp rescaling keeps codec-time and muxer-time
  units separate without linking FFmpeg. The helper is frontend-only and does
  not change the guest scheduler or audio producer.
* Focused tests cover a three-hour 48,000/1,001 synthetic schedule, changing
  17-tick partitions, origin offsets, wrap extension, invalid inputs and
  timestamp rescaling.

## REC02 evidence

Evaluated source commit:
8c40626f682d569668ba4ef574249f05498f8882.

The macOS arm64 AppleClang build and focused recording tests passed. The
existing ROM-less self-test and smoke run passed; smoke reported only the
expected missing-ROM warning. Repository case, UTF-8, EOL and milestone-ID
validators passed. The REC02 synthetic rate is a fixture only. The actual VA
clock mapping remains the audited TSP/pccore mapping in `audit.md`; no hardware
timing claim is inferred from 48,000/1,001.

REC02 does not add a recorder, FFmpeg backend, video/audio tap, queue, UI or
runtime capture. Audio look-ahead reconciliation and source integration remain
the subjects of REC10/REC11.

## REC01 completion

* The production frontend now includes a small recording contract facade under
  sdl2/recording/, with a C99 boundary and a private C++17 ownership layer.
* VAEG_ENABLE_RECORDING defaults OFF and is supplied as a target-scoped
  definition. Both OFF and ON configurations compile without a recording
  backend.
* The OFF facade reports disabled; the ON facade reports that the backend is
  unavailable. No FFmpeg/libav headers, types, libraries or subprocesses were
  added.
* Deterministic validation covers the two source values, dimensions, pixel
  formats, stride and checked storage-size arithmetic, PCM S16LE stereo
  descriptors, and stable error names.
* OwnedVideoFrame copies caller data into private storage. The C++ facade does
  not expose STL types through the C boundary.
* Focused C99 and C++17 tests are registered under the recording/contracts
  labels.
* No live video/audio tap, queue, encoder, worker, CLI command, UI command or
  screenshot/QA path was added.

## Evidence

Evaluated source commit:
f838d15c7c676bf70a6d19d41f7f8fe5da57c813.

The macOS OFF test build and the ON compile check passed. The focused contract
tests, existing ROM-less self-test, startup viewport test and milestone ID
self-test passed. The OFF and ON CMake build files contain no FFmpeg/libav
references. Existing unrelated untracked work was preserved and excluded.

REC01 is host-independent. Windows D3D11, Linux OpenGL and macOS Metal
recording runtime evidence remains NOT_RUN because those are later Displayed
milestones, not REC01 acceptance evidence.

## Gate and next task

Machine gate: PASS. The required REC01 checks were run on macOS arm64 with
AppleClang 21, CMake 3.31.12, MacPorts SDL2 2.32.10 and Ninja.

Human gate: NOT_REQUIRED for REC01. REC00 human acceptance is recorded above.

The next dependency-ready task is REC02/M100r3. Do not start REC02 in this
session. Later tasks must preserve the two-source OSD-inclusive contract and
must add no FFmpeg dependency until their own milestone requires it.
