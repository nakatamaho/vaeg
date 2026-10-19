# REC09 report: native frame copy and OSD compositor

Status: PASS

Feature ID and mapped legal M ID: REC09 / M100r10
Assigned branch: `topic/m100r3-recording-clock`
Base SHA: `e051d4397c05ab175e69efb6cb72a072677c71a7`
Evaluated source SHA: `1aa9f6f78dd968bdabfa068c24e32b4fa24403ca`
Evidence-only SHA, if applicable: Documentation commit immediately following the evaluated source commit
Task/spec version: `docs/recording-v1/milestones/rec09-native-composition.md`
Evaluated binary/toolchain identity: macOS arm64, AppleClang 21.0.0.21000101,
CMake 3.31.12, Ninja 1.13.2, MacPorts SDL2 2.32.10

## Scope and changes

REC09 adds the Native recording frame boundary without starting live recording.
The frontend can now copy the completed canonical guest raster into recording-
owned RGB24 storage and compose the enabled guest-area OSD on that copy.

Changed files:

* `sdl2/recording/recording_native_composition.h` and `.c` add the C99-owned
  frame type, checked RGB565 source descriptor, established RGB565-to-RGB24
  conversion, native 1:1 sample-aspect metadata and CPU OSD compositor.
* `sdl2/scrnmng.c` and `sdl2/scrnmng.h` expose a frontend helper that reads the
  active canonical shadow raster after its left guard, obtains the REC08 value
  OSD snapshot and composes it into a separate owned frame. It does not write
  back to the guest producer surface.
* `tests/frontend/recording/test_recording_native_composition.c` covers RGB
  conversion, source/guard isolation, owned-storage mutation, both audited OSD
  classes, draw order, top-right native anchoring, text/background alpha,
  bottom clipping, no-OSD behavior and invalid source/snapshot inputs.
* `CMakeLists.txt` adds the always-available C99 helper and its ROM-free CTest.
* `docs/agents/tasks/M100r10_recording_v1_rec09.md` assigns the legal M ID.

Native and Displayed remain the only two recording sources. This checkpoint
does not add a third source, a recording OSD checkbox, a screenshot mode,
window scaling, CRT processing, host GUI/status strips, FFmpeg use or live
recorder integration. Existing guest-only screenshots and QA captures remain
on their existing paths.

## Implementation findings and decisions

The integration point is `scrnmng_capture_native_recording_frame` in
`sdl2/scrnmng.c`. The canonical producer is the existing 640-by-400
`scrnmng.shadow` surface allocated by `scrnmng_create`; the source pointer is
advanced by the one-pixel 16-bit left guard and the pitch is retained. The
validated source-size calculation includes only the active final row and never
reads row padding or guard bytes.

The copied frame is an independent packed RGB24 allocation with a bounded
256 MiB size check, exact 1:1 sample aspect ratio and the same integer channel
expansion as the existing `frame_conversion.cpp` RGB565 path. The canonical
shadow buffer is therefore unchanged if the recording-owned frame or its OSD
changes.

The compositor consumes the immutable REC08 snapshot's copied semantic lines.
It renders `VIDEO_INFO` first and `FRAMEBUFFER_INFO` second, at native scale 1
with the existing top-right anchor and 4-pixel inter-layer separation. Enabled
layers are included even when their output-space `visible` flag was false due
to a too-small live presentation; native recording has the complete canonical
extent and must not inherit a window-size omission. Text uses the existing
`fontdata_8` asset, black background alpha and the established pale-yellow text
color. Every pixel write is clipped to the owned frame. No ImGui object, SDL
renderer, GPU resource, or borrowed formatter string enters the frame.

The helper rejects zero/short/overflowing source geometry, busy output,
malformed frame descriptors, invalid snapshot geometry/layers and missing font
data. The later guest-time video task will call this helper at a completed
interval boundary; no producer, clock, audio or queue behavior was changed in
REC09.

## Checks

| Check ID | Exact command/environment | Exit code / outcome | Evidence |
|---|---|---:|---|
| REC09-FOCUSED-ON | `cmake --build build/macos-rec07-on --target vaeg_recording_native_composition_test -j2 && ctest --test-dir build/macos-rec07-on --output-on-failure -R '^vaeg_recording_native_composition$'` | 0; 1/1 passed | RGB/OSD/guard/validation fixture |
| REC09-FOCUSED-OFF | `cmake --build build/macos-rec07-off --target vaeg_recording_native_composition_test -j2 && ctest --test-dir build/macos-rec07-off --output-on-failure -R '^vaeg_recording_native_composition$'` | 0; 1/1 passed | Recording-disabled build still exercises the C99 helper without FFmpeg |
| REC09-WERROR | `cmake --build build/macos-rec08-werror --target vaeg_recording_native_composition_test -j2` | 0 | New C99 helper/test compiled with `VAEG_WERROR=ON` |
| REC09-SANITIZER | `cmake --build build/macos-rec08-sanitize --target vaeg_recording_native_composition_test -j2 && ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 ctest --test-dir build/macos-rec08-sanitize --output-on-failure -R '^vaeg_recording_native_composition$'` | 0; 1/1 passed | AddressSanitizer/UndefinedBehaviorSanitizer; macOS leak detection disabled |
| REC09-SUITE-ON | `ctest --test-dir build/macos-rec07-on --output-on-failure -L recording` | 0; 10/10 passed | REC01–REC09 recording suite |
| REC09-SUITE-OFF | `ctest --test-dir build/macos-rec07-off --output-on-failure -L recording` | 0; 6/6 passed | Recording-OFF regression and native helper test |
| REC09-BUILD-ON | `cmake --build build/macos-rec07-on --target vaeg_sdl2 -j2` | 0 | Enabled frontend built |
| REC09-BUILD-OFF | `cmake --build build/macos-rec07-off --target vaeg_sdl2 -j2` | 0 | Disabled frontend built without changing the FFmpeg-off contract |
| REC09-ROMLESS | `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy build/macos-rec07-on/sdl2/vaeg --selftest --no-cfg --no-bkupmem`; same with `--smoke`, and repeated with `build/macos-rec07-off/sdl2/vaeg` | 0 / 0 / 0 / 0 | Existing selftest and smoke passed in both configurations; expected ROM-less diagnostics only |
| REC09-REPO | `git diff --check && python3 tools/repo/check_case.py && python3 tools/repo/check_encoding.py --expect utf8 && python3 tools/repo/check_eol.py --enforce && python3 tools/qa/milestone_ids.py --selftest --audit --discover` | 0 | No whitespace/case/encoding/EOL violations; 49 strict ID checks; tasks=151, reports=160 |

The enabled and disabled CMake configurations were already established with
the repository-required `VAEG_Z80_COMPAT_INTEGRATION_TRACE=ON` option. Existing
frontend warnings were unchanged and are not REC09 failures.

## Integrity results

The focused fixture uses a guarded, padded RGB565 source and verifies that the
owned frame contains the expected red/blue/green edge pixels while source
mutation does not change the copied frame. It verifies the canonical 1:1 SAR,
24-by-32 fixture dimensions, exact integer alpha results, video-before-
framebuffer order, top-right text pixels and a framebuffer panel clipped at
the bottom edge. A no-OSD snapshot leaves the copied image byte-identical to
the conversion baseline. Short pitch, insufficient source extent, output reuse
and mismatched snapshot dimensions return stable errors before an invalid read
or a second allocation.

No live recording frame count, audio sample count, PTS, queue high-water mark,
MKV output, presenter readback or cross-backend pixel comparison applies yet;
those are deliberately deferred to the clock, displayed-source and recorder
integration milestones.

## Review and limitations

The new production path is C99 under `sdl2/recording/` and exposes only plain
data through the existing C frontend boundary. It allocates only when a future
recording producer calls the helper; existing presentation and screenshot
paths do not call it. The owned output is bounded and freed on composition
failure. No audio callback, guest scheduler, GPU/GUI lock or FFmpeg type is
involved.

This is machine-only macOS evidence. No live ROM/media recording, Displayed
SDL/D3D11/OpenGL/Metal readback, Windows/Linux runtime, VLC playback, human
demonstration or release validation is claimed. The next dependency-ready
work is the guest video-clock integration; native composition must remain
separate from displayed capture and exact audio accounting.

## Gate and next task

Machine gate: PASS.
Human gate: NOT_REQUIRED for this machine-only checkpoint; no human or release
validation is claimed.
Push/remote state under repository policy: local only; no push, merge or
publication performed.
Next dependency-ready REC task and host requirement: REC10 / M100r11 on the
same macOS host, subject to its prerequisite audit and actual source checks.

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
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->
