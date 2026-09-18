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
DAMAGES (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# REC01 recording-v1 contracts and compiled-OFF skeleton

Status: PASS

Feature ID and mapped legal M ID: REC01 / M100r2
Assigned branch: topic/m100r2-recording-contracts
Base SHA: 5f43757448ecad160733a73e9c119c476af3de33
Evaluated source SHA: f838d15c7c676bf70a6d19d41f7f8fe5da57c813
Evaluated binary/toolchain identity: macOS arm64, AppleClang 21,
CMake 3.31.12, Ninja, MacPorts SDL2 2.32.10
Evidence-only SHA: Not applicable
Task/spec version: docs/recording-v1/milestones/rec01-contracts.md

## Scope and changes

REC01 implements only the recording contract boundary and compiled-OFF
skeleton. It does not implement a recorder, live video/audio taps, queues,
workers, encoders, FFmpeg discovery, CLI/UI commands, or presenter readback.
The frozen contract remains exactly Native video and Displayed video, both with
enabled guest-area emulator OSD, and the eventual output remains MKV with FFV1
video and PCM audio.

Changed files:

* CMakeLists.txt adds VAEG_ENABLE_RECORDING, default OFF, target-scoped
  definitions, the inert facade sources and two focused tests.
* sdl2/recording/recording.h is the C99-compatible boundary. It contains no
  C++ or FFmpeg types.
* sdl2/recording/recording.c implements stable source, video, audio, copy and
  checked-size validation errors plus capability/status reporting.
* sdl2/recording/recording_contracts.hpp and recording_contracts.cpp provide
  private C++17 owned-frame storage and an inert status facade.
* tests/frontend/recording/test_recording_c.c compiles the C boundary as C99
  and exercises source, descriptor, overflow, audio and OFF-state failures.
* tests/frontend/recording/test_recording_contracts.cpp compiles the private
  C++17 layer and verifies owned-copy behavior and status reporting.
* docs/agents/tasks/M100r2_recording_v1_rec01.md is the assigned task wrapper.
* docs/recording-v1/status.md, handoff.md and id-map.md record REC00
  acceptance and REC01 completion.

No canonical raster, screenshot, QA, guest state, C core, audio generation
path, or existing presenter behavior was changed.

## Implementation findings and decisions

The C boundary uses value descriptors for the two source values, RGB565/RGB24
input descriptions and PCM S16LE stereo audio. Video validation rejects null
descriptors, zero dimensions, unknown pixel formats, short strides and
height-times-stride overflow. Audio validation rejects zero rates, non-stereo
layouts, non-16-bit samples and non-S16LE formats. The copy validator rejects
null nonempty input and size overflow.

OwnedVideoFrame validates before allocation, checks the storage multiplication
against size_t, catches allocation failure and copies source bytes into a
private vector. The caller's buffer may be destroyed or mutated after the copy;
the owned bytes are independent. The C API exposes no STL type, exception or
borrowed C++ object.

With VAEG_ENABLE_RECORDING=0, the status facade reports DISABLED. With
VAEG_ENABLE_RECORDING=1 at this checkpoint, it reports the build as enabled
but the backend as unavailable. This is deliberate: REC01 does not discover
or link FFmpeg. Both builds remain complete and do not silently pretend that
recording works.

The sources are frontend-only under sdl2/ and preserve the repository's C99
core/C++17 frontend boundary. No OSD rendering was duplicated; later Native
and Displayed milestones must consume the REC00 OSD map and include all
enabled guest-area OSD.

## Checks

| Check ID | Exact command/environment | Exit code / outcome | Evidence |
|---|---|---|---|
| REC01-OFF-CONFIG | env PKG_CONFIG_PATH=/opt/local/lib/pkgconfig:$PKG_CONFIG_PATH cmake -S . -B build/macos-rec01 -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/opt/local -DVAEG_ENABLE_TESTS=ON -DVAEG_Z80_COMPAT_INTEGRATION_TRACE=ON -DVAEG_ENABLE_LIBRASHADER=OFF -DVAEG_ENABLE_RECORDING=OFF | 0 / PASS | Reconfigured at evaluated source |
| REC01-OFF-BUILD | cmake --build build/macos-rec01 --target vaeg_recording_c_contract_test vaeg_recording_contract_test vaeg_sdl2 -j2 | 0 / PASS | Main OFF target and focused tests built |
| REC01-OFF-CTEST | ctest --test-dir build/macos-rec01 --output-on-failure -R '^(vaeg_recording_(c_contracts|contracts)|vaeg_romless_tests|vaeg_sdl_startup_viewport|vaeg_milestone_id_selftest)$' | 0 / PASS | 5/5 tests passed |
| REC01-OFF-NO-LIBAV | rg -ni 'ffmpeg|avcodec|avformat|avutil' build/macos-rec01/CMakeCache.txt build/macos-rec01/build.ninja | no matches | OFF generated files contain no FFmpeg/libav references |
| REC01-ON-CONFIG-BUILD | env PKG_CONFIG_PATH=/opt/local/lib/pkgconfig:$PKG_CONFIG_PATH cmake -S . -B build/macos-rec01-on -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/opt/local -DVAEG_ENABLE_TESTS=OFF -DVAEG_ENABLE_LIBRASHADER=OFF -DVAEG_ENABLE_RECORDING=ON; cmake --build build/macos-rec01-on --target vaeg_sdl2 -j2 | 0 / PASS | ON facade compiles without a backend |
| REC01-ON-NO-LIBAV | rg -ni 'ffmpeg|avcodec|avformat|avutil' build/macos-rec01-on/CMakeCache.txt build/macos-rec01-on/build.ninja | no matches | ON generated files contain no FFmpeg/libav references |
| REC01-FORMAT | clang-format-mp-22 -i on changed C/C++ files; git diff --check | 0 / PASS | clang-format 22.1.8; no whitespace errors |

The focused tests were labelled romless, recording and contracts. The existing
ROM-less self-test, startup viewport test and milestone ID self-test also
passed in the same test run. Existing compiler warnings and linker warnings
are unchanged and unrelated to this milestone.

## Integrity results

No movie or live capture was generated in REC01. Frame/sample counts, OSD
pixel inclusion, FFV1 round trips, PCM round trips, timestamps, queue
watermarks, VLC playback and GPU readback are not applicable to this
contract-only milestone and remain for later tasks.

## Review and limitations

The OFF build requires no FFmpeg installation, header, library or executable.
The ON build only exposes the inert capability state and intentionally reports
BACKEND_UNAVAILABLE. No silent ON-to-OFF recording fallback exists because no
recording start path exists yet.

Windows D3D11, Linux OpenGL and macOS Metal runtime evidence is not required
for REC01 and remains NOT_RUN in the platform matrix. Those backends are
covered by later Displayed milestones. REC01 also does not resolve the
audio callback look-ahead identified in REC00; it only establishes the
descriptor boundary that later timing/audio tasks must use.

## Gate and next task

Machine gate: PASS. C01, C02 ownership coverage, C03 and C04 were exercised
by the OFF/ON builds and focused C99/C++17 tests.

Human gate: NOT_REQUIRED. REC00's human acceptance was recorded on 2026-09-19.

The next dependency-ready task is REC02/M100r3. Stop here and do not implement
REC02 in this session.
