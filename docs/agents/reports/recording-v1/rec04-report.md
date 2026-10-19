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
# REC04 optional FFmpeg discovery and capability checks

Status: PASS

Feature ID and mapped legal M ID: REC04 / M100r5
Assigned branch: `topic/m100r3-recording-clock`
Base SHA: `0d36d29d`
Evaluated source SHA: `2258aa0a73533691305d7b48d44f924b6303ec51`
Evaluated binary/toolchain identity: macOS arm64, AppleClang 21.0.0.21000101, CMake 3.31.12, Ninja 1.13.2, MacPorts SDL2 2.32.10, FFmpeg 8.1.2
Evidence-only SHA, if applicable: Not applicable
Task/spec version: `docs/recording-v1/milestones/rec04-ffmpeg-capabilities.md`

## Scope and changes

REC04 adds only the optional frontend FFmpeg dependency boundary and a private
capability probe. `VAEG_ENABLE_RECORDING` remains OFF by default.

Changed files:

* `CMakeLists.txt` discovers target-scoped `libavutil`, `libavcodec` and
  `libavformat` imported targets only when recording is enabled, adds the
  private probe only to the enabled frontend, and registers its focused test.
* `sdl2/recording/recording_ffmpeg_capabilities.hpp` declares public-safe
  version, inventory and stable error-value data without any FFmpeg type.
* `sdl2/recording/recording_ffmpeg_capabilities.cpp` privately adapts FFmpeg
  capability enumeration and probes FFV1, PCM S16, Matroska, BGR0, slice CRC
  and FFV1 version 3.
* `tests/frontend/recording/test_recording_ffmpeg_capabilities.cpp` checks
  one-controlled-mutation failures and the actual development-library probe.
* The REC04 task wrapper, ID map, status, handoff and report record this gate.

The exact Native video / Displayed video choices, guest-area OSD inclusion,
canonical screenshot and QA contracts, C99 boundary, bounded queue and exact
guest-time requirements are unchanged. No recorder, MKV file sink, live tap,
UI command, swscale/resampler, subprocess or FFmpeg type was added to a C
interface.

## Implementation findings and decisions

The enabled build uses `find_package(PkgConfig)` and required imported targets
for all three libraries. This follows the existing SDL/pkg-config convention
and leaves target selection to the active toolchain rather than adding global
include or link flags. The disabled configure path does not inspect FFmpeg and
the disabled build has no FFmpeg source, header or library reference in its
generated CMake files.

The tested development API baseline was:

| Library | Tested version | Minimum major enforced by the private probe |
|---|---:|---:|
| libavutil | 60.26.102 | 58 |
| libavcodec | 62.28.102 | 60 |
| libavformat | 62.12.102 | 60 |

The private adapter uses `avcodec_get_supported_config` for current codec
headers and retains a private `pix_fmts`/`sample_fmts` compatibility branch for
older headers in the supported major range. The current build proved the
following required capabilities: FFV1 encoder, PCM S16LE encoder, Matroska
muxer, FFV1 BGR0 pixel format, packed PCM S16 sample format, Matroska acceptance
of both codecs, FFV1 `slicecrc`, and FFV1 level/version 3. The probe also opens
a small BGR0 FFV1 context with `gop_size=1` while checking the required options;
it does not encode or write a recording file yet.

The local MacPorts FFmpeg development build is configured with GPL and nonfree
components. It is development/test input only. No FFmpeg binary, library or
license claim is added to a release package by REC04; release-enabled packaging
must later use the separately audited minimal configuration required by the
recording-v1 packaging milestone.

The production backend contains no process-launch API and does not discover or
invoke an `ffmpeg` executable. The external executable was used only to inspect
the development environment's advertised FFV1, PCM S16LE and Matroska entries.

## Checks

| Check ID | Exact command/environment | Exit code / outcome | Evidence |
|---|---|---:|---|
| REC04-ON-CONFIG | `cmake -S . -B build/macos-rec04-on -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/opt/local -DVAEG_ENABLE_TESTS=ON -DVAEG_Z80_COMPAT_INTEGRATION_TRACE=ON -DVAEG_ENABLE_LIBRASHADER=OFF -DVAEG_ENABLE_RECORDING=ON` | 0 | All three imported libav targets found |
| REC04-ON-BUILD | `cmake --build build/macos-rec04-on --target vaeg_recording_ffmpeg_capabilities_test vaeg_sdl2 -j2` | 0 | Enabled frontend and probe linked |
| REC04-ON-TEST | `ctest --test-dir build/macos-rec04-on --output-on-failure -R '^vaeg_recording_'` after building all five recording test targets | 0; 5/5 passed | C99/C++17 contracts, clock, queue and FFmpeg capability probe |
| REC04-OFF-BUILD | `cmake -S . -B build/macos-rec04-off -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/opt/local -DVAEG_ENABLE_TESTS=ON -DVAEG_Z80_COMPAT_INTEGRATION_TRACE=ON -DVAEG_ENABLE_LIBRASHADER=OFF -DVAEG_ENABLE_RECORDING=OFF` followed by `cmake --build build/macos-rec04-off --target vaeg_sdl2 -j2` | 0 | `rg` over `CMakeCache.txt` and `build.ninja` found no FFmpeg headers/libraries/source references |
| REC04-OFF-TEST | `ctest --test-dir build/macos-rec04-off --output-on-failure -R '^vaeg_recording_'` after building the four OFF recording test targets | 0; 4/4 passed | Recording-OFF regression |
| REC04-MISSING | Empty pkg-config directory, SDL2 config pinned separately: `PKG_CONFIG_LIBDIR=/private/tmp/vaeg-rec04-empty-pkgconfig PKG_CONFIG_PATH=/private/tmp/vaeg-rec04-empty-pkgconfig cmake -S . -B build/macos-rec04-missing-4 -G Ninja -DCMAKE_BUILD_TYPE=Debug -DSDL2_DIR=/opt/local/lib/cmake/SDL2 -DCMAKE_PREFIX_PATH=/nonexistent -DVAEG_ENABLE_TESTS=ON -DVAEG_Z80_COMPAT_INTEGRATION_TRACE=ON -DVAEG_ENABLE_LIBRASHADER=OFF -DVAEG_ENABLE_RECORDING=ON` | 1 as required | Configuration stopped at required `libavutil` with a specific missing-package diagnostic |
| REC04-ROMLESS | `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/macos-rec04-off/sdl2/vaeg --selftest --no-cfg --no-bkupmem` and the corresponding `--smoke` command | 0 / 0 | Existing self-test passed; smoke retained only its expected missing-ROM warning |
| REC04-REPO | `python3 tools/qa/milestone_ids.py --selftest --audit --discover`; `python3 tools/repo/check_case.py`; `python3 tools/repo/check_encoding.py --expect utf8`; `python3 tools/repo/check_eol.py --enforce` | 0 / 0 / 0 / 0 | 49 strict milestone checks; zero case, encoding or EOL findings |
| REC04-NO-SUBPROCESS | `rg -n 'system\\(|popen\\(|posix_spawn|CreateProcess|execvp|execl|ffmpeg( |$)' sdl2/recording/recording_ffmpeg_capabilities.cpp` | 0 from the guarded check | No runtime process-launch API found |

A preliminary CTest invocation before all existing test executables were built
reported four `Not Run` tests; it was not used as evidence. The complete
target build and the two final CTest runs above produced the stated results.

## Integrity results

No recording file is produced in REC04. Frame/sample counts, PTS alignment,
queue high-water, seekability, trailer integrity, partial-file behavior and
RGB/PCM round trips are not applicable and remain REC05+ evidence.

## Review and limitations

The dependency is optional and target-scoped. FFmpeg headers are confined to
the private C++ implementation file; the C99 recording header and core-facing
interfaces remain free of FFmpeg types. Capability failures have stable names
for missing encoder, muxer, pixel/sample format, FFV1 option and muxer
compatibility cases, and the focused test mutates one inventory item at a time
to keep those reasons distinct.

This is not yet a recording backend. No actual Matroska file, encoder packet,
audio/video timing, native/displayed readback, VLC playback, Windows/Linux
runtime, cross-compilation target, or release-package license audit was tested.
The current development libraries are not suitable evidence for release
packaging because their build includes GPL/nonfree components. Those limitations
are intentionally deferred to the corresponding later milestones.

## Gate and next task

Machine gate: PASS.
Human gate: NOT_REQUIRED for this machine-only checkpoint; no human or release
validation is claimed.
Push/remote state under repository policy: local only; no push, merge or
publication performed.
Next dependency-ready REC task and host requirement: REC05 / M100r6, a host
with the enabled FFmpeg development backend.
