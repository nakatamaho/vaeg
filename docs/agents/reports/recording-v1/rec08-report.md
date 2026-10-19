# REC08 report: guest-area OSD inventory and immutable snapshot

Status: PASS

Feature ID and mapped legal M ID: REC08 / M100r9
Assigned branch: `topic/m100r3-recording-clock`
Base SHA: `c537651802f9d2b76cfd6176e5eca7463b9e715c`
Evaluated source SHA: `e4766a4173c4a9c69e483eb0e7bdb3887da02382`
Evidence-only SHA, if applicable: Documentation commit containing this report
Task/spec version: `docs/recording-v1/milestones/rec08-osd-snapshot.md`
Evaluated binary/toolchain identity: macOS arm64, AppleClang
21.0.0.21000101, CMake 3.31.12, Ninja 1.13.2, MacPorts SDL2 2.32.10

## Scope and changes

REC08 inventories every actual guest-area emulator OSD path found in the REC00
audit and extracts an immutable semantic snapshot without adding a recording
checkbox or changing the Native/Displayed source contract.

Changed files:

* `sdl2/recording/recording_osd_snapshot.h` and `.c` define the C99 fixed-size
  snapshot and its fail-closed validation/copy boundary. A snapshot owns all
  line text and value fields; it contains no transient pointers.
* `sdl2/scrnmng.c` builds one snapshot per guest `drawcount` and output size,
  invalidates it at the start of a new presentation, and makes both existing
  native and SDL diagnostic overlay functions consume the copied layers. The
  public getter returns a complete value copy for later recording producers.
* `sdl2/scrnmng.h` exposes the value-copy getter to the frontend recording
  integration without crossing a C99/C++17 boundary.
* `tests/frontend/recording/test_recording_osd_snapshot.c` covers ownership,
  text updates, enabled/disabled layers, partial/full/zero alpha, top-right
  anchors, clipping rectangles and validation failures.
* `CMakeLists.txt` adds the always-available C99 source and focused CTest.
* `docs/recording-v1/audit.md` records the implementation inventory and the
  explicit host/UI exclusions. The task wrapper, status, handoff and this
  report record the M100r9 evidence.

The inventory is:

| Actual path | Control | Classification | Snapshot result |
|---|---|---|---|
| `scrnmng_draw_video_info_overlay` | `VAEG_DISPINFO_VIDEO` | guest-area emulator OSD | `VIDEO_INFO`, copied lines and layout |
| `scrnmng_draw_framebuffer_info_overlay` | `VAEG_DISPINFO_FRAMEBUFFER` | guest-area emulator OSD | `FRAMEBUFFER_INFO`, copied lines and layout |
| `scrnmng_draw_native_overlays` | native overlay primitive path | drawing entry, not a third semantic class | consumes the same two copied layers |
| `scrnmng_update_title` | FDD/FPS/CPU/SGP/frame flags | OS title decoration | excluded |
| `gui_draw` / `gui_render` | ImGui menu, hint and dialogs | in-client GUI | not guest-area OSD; Displayed remains responsible later |
| `scrnmng_draw_graphics_analysis` | screenshot-only path | screenshot annotation | excluded from both recording OSD sources |

Native and Displayed remain the only two recording sources, and both include
enabled guest-area OSD. This milestone does not yet compose a Native recording
raster or acquire a Displayed final-client readback. Existing guest-only
screenshots and QA captures remain on their original paths.

## Implementation findings and decisions

`machine/pccore.c:drawscreenva` increments `drawcount` at the completed guest
display boundary. REC08 uses that value as the frame token. The snapshot also
stores guest dimensions, actual SDL output dimensions, layer enabled/visible
state, output-space rectangle, output clip rectangle, top-right anchor, integer
font scale, background alpha, text alpha and copied lines. The current live
layout uses output coordinates because it must preserve the existing SDL/native
overlay geometry; later Native composition can use the stored anchor and clip
semantics when mapping to the canonical guest raster.

The two diagnostic layers are always represented in the snapshot. A disabled
layer has `enabled=0`, `visible=0` and no borrowed line data. An enabled layer
that cannot fit the current output has `enabled=1`, `visible=0`, preserving the
existing no-draw boundary while retaining its semantic state. Cache reuse is
guarded by frame token, guest/output dimensions and the display-info flags, so
a same-frame menu toggle cannot reuse stale text or enabled state.

The existing overlay order and calculations are unchanged: video information is
drawn first, framebuffer information follows it, each with the former alpha,
padding, line order, integer scale and bounds check. The snapshot is created on
the renderer/GUI owner thread. No ImGui draw-data pointer, GUI object, or stack
formatter buffer is stored or returned.

## Checks

| Check ID | Exact command/environment | Exit code / outcome | Evidence |
|---|---|---:|---|
| REC08-ON-BUILD | `cmake --build build/macos-rec07-on -j2` | 0 | Enabled frontend and recording targets built; incremental rerun after source commit had no pending work |
| REC08-ON-TEST | `ctest --test-dir build/macos-rec07-on -R '^vaeg_recording_' --output-on-failure` | 0; 9/9 passed | Existing REC01–REC07 suite plus OSD snapshot test |
| REC08-OFF-BUILD | `cmake --build build/macos-rec07-off --target vaeg_sdl2 vaeg_recording_c_contract_test vaeg_recording_osd_snapshot_test vaeg_recording_contract_test vaeg_recording_clock_test vaeg_recording_queue_test -j2` | 0 | Recording-disabled frontend and focused tests built |
| REC08-OFF-TEST | `ctest --test-dir build/macos-rec07-off -R '^vaeg_recording_' --output-on-failure && ! rg -ni 'ffmpeg|avcodec|avformat|avutil' build/macos-rec07-off/CMakeCache.txt build/macos-rec07-off/build.ninja` | 0; 5/5 passed | Recording-OFF regression and no generated FFmpeg references |
| REC08-WERROR | `cmake -S . -B build/macos-rec08-werror -G Ninja -DVAEG_ENABLE_RECORDING=OFF -DVAEG_ENABLE_TESTS=ON -DVAEG_Z80_COMPAT_INTEGRATION_TRACE=ON -DVAEG_WERROR=ON && cmake --build build/macos-rec08-werror --target vaeg_recording_osd_snapshot_test -j2 && ctest --test-dir build/macos-rec08-werror -R '^vaeg_recording_osd_snapshot$' --output-on-failure` | 0; 1/1 passed | New C99 snapshot source/test pass with warnings as errors |
| REC08-SANITIZER | `cmake -S . -B build/macos-rec08-sanitize -G Ninja -DVAEG_ENABLE_RECORDING=OFF -DVAEG_ENABLE_TESTS=ON -DVAEG_Z80_COMPAT_INTEGRATION_TRACE=ON -DVAEG_WERROR=OFF -DCMAKE_C_FLAGS='-fsanitize=address,undefined' -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined' -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined' && cmake --build build/macos-rec08-sanitize --target vaeg_recording_osd_snapshot_test -j2 && ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 ./build/macos-rec08-sanitize/vaeg_recording_osd_snapshot_test` | 0 | Ownership/copy and validation test passed; macOS leak detection disabled |
| REC08-ROMLESS-ON | `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy build/macos-rec07-on/sdl2/vaeg --selftest --no-cfg --no-bkupmem` and `... --smoke --no-cfg --no-bkupmem` | 0 / 0 | Existing selftest, including display-capture/overlay checks, and smoke passed; expected synthetic diagnostics remain |
| REC08-ROMLESS-OFF | Same commands using `build/macos-rec07-off/sdl2/vaeg` | 0 / 0 | Recording-OFF selftest and smoke passed |
| REC08-REPO | `python3 tools/repo/check_case.py && python3 tools/repo/check_encoding.py --expect utf8 && python3 tools/repo/check_eol.py --enforce && git diff --check` | 0 | No case, encoding, EOL or whitespace violations |
| REC08-ID | `python3 tools/qa/milestone_ids.py --selftest --audit --discover` | 0 | 49 strict checks passed; discovery found the updated REC08 wrapper/report set |

The first Werror configure attempt omitted the repository-required
`VAEG_Z80_COMPAT_INTEGRATION_TRACE=ON` option and was rejected by CMake before
compilation. The command was corrected and the focused Werror check above
passed; this was an invocation correction, not a source failure.

## Integrity results

The snapshot test verifies that changing the input line arrays after the build
does not change the stored text. It verifies both semantic classes, disabled
and clipped visibility, output clipping rectangles, top-right anchors and
alpha values of zero, partial and full strength. Invalid layer count and
duplicate class inputs fail with stable error codes. The output type contains
fixed arrays only, so no borrowed pointer can escape.

The existing `scrnmng_display_capture_selftest` passed in both frontend builds.
It still checks the baseline/effect crop behavior, repeated overlay rendering
stability and all four video/framebuffer diagnostic flag combinations. The
guest-only screenshot and graphics-analysis functions were not changed.

## Review and limitations

The snapshot builder and test are C99 and remain within the SDL2/frontend C
boundary. There is no FFmpeg dependency, allocation, file I/O or logging in
the snapshot producer. The fixed arrays are bounded by the actual four VA
framebuffers and their three diagnostic lines each; formatted source strings
are copied before the formatter's stack storage expires.

This is a machine-only semantic checkpoint. No live ROM/media recording, Native
recording-owned canonical raster, Displayed SDL/D3D11/OpenGL/Metal readback,
audio producer, UI lifecycle, Windows/Linux runtime, VLC playback, human gate
or release validation is claimed. Platform-specific and cross-backend evidence
remains for its later milestones.

## Gate and next task

Machine gate: PASS. The focused ownership/semantic tests, ON/OFF recording
regressions, sanitizer check, ROM-less frontend checks and repository validators
passed.

Human gate: NOT_REQUIRED for this machine-only checkpoint; no human or release
validation is claimed.
Push/remote state under repository policy: local only; no push, merge or
publication performed.
Next dependency-ready REC task and host requirement: REC09 / M100r10, on the
same macOS host unless its task requires a different platform capability.

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
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
-->
