# Recording v1 handoff

Current state: REC01 machine gate PASS. Completed tasks: REC00/M100r1,
explicitly accepted by the maintainer on 2026-09-19, and REC01/M100r2.
Assigned branch: topic/m100r2-recording-contracts.

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
