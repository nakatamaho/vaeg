# REC10 report: audio production seam with legacy behavior preserved

Status: PASS

Feature ID and mapped legal M ID: REC10 / M100r11

Assigned branch: `topic/m100r3-recording-clock`

Base SHA: `fb93f96901842ccbc9b05981bd1e755c45f2f4f7`

Evaluated source SHA: `5e2823a437b4082b5b41f5f7d89e7a1073e738bd`

Evidence-only SHA, if applicable: Documentation commit immediately following the evaluated source commit

Task/spec version: `docs/recording-v1/milestones/rec10-audio-seam.md`
Evaluated binary/toolchain identity: macOS arm64, AppleClang 21.0.0.21000101,
CMake 3.31.12, Ninja 1.13.2, MacPorts SDL2 2.32.10

## Scope and changes

REC10 extracts the smallest reusable C99 operation around the existing sound
generator, reserve-tail look-ahead and SDL PCM packing paths. It records the
existing stream state needed for a later handover, but does not connect a live
recorder or change the guest sample grid.

Changed files:

* `sound/sound_generation.h` and `.c` add the C99 generator dispatch,
  reserved-tail calculation and S32-to-S16/silence packing helpers. They do
  not allocate, lock, throw, touch chip state or own a clock.
* `sound/sound.c` routes the existing `streamprepare` and WAV generation
  through the helper, keeps callback order and generated lengths, preserves
  the existing `sound_sync` and callback look-ahead behavior, and adds a
  checked cumulative generated-sample observation plus reset epoch.
* `sound/sound.h` exposes the plain-data `SOUNDSTREAMSTATE` observation API;
  no C++ or FFmpeg type crosses the C boundary.
* `sdl2/soundmng.c` uses the shared S16/silence helper while preserving the
  SDL callback's byte count and `sound_pcmunlock` lifecycle.
* `tests/frontend/recording/test_sound_generation.c` compares the seam with
  the direct legacy reference for callback order, sample values, saturation,
  truncation, silence and look-ahead boundaries.
* `CMakeLists.txt` builds the focused ROM-free test and labels it
  `romless;recording;sound`.
* `docs/agents/tasks/M100r11_recording_v1_rec10.md` records the completed
  machine-gate state.

Native and Displayed remain the only two recording sources. Native/OSD,
Displayed final-client composition, screenshots and QA captures are unchanged.
No recording producer, host callback tap, new sample-rate algorithm, ymfm
change, chip reset or MKV integration is part of this checkpoint.

## Implementation findings and decisions

The pre-existing generation and packing call sites were traced as follows:

* `sound/sound.c:streamprepare` zeroes the interleaved stereo S32 stream and
  invokes the sound-chip callbacks in the established table order.
* `sound/sound.c:sound_sync` converts guest-clock elapsed time through the
  existing `clockbase`/`hzbase` relation and calls `streamprepare` while the
  core sound critical section is held.
* `sound/sound.c:sound_pcmlock` can generate `remain - reserve` frames when
  the callback-visible stream is beyond its reserved tail, then advances the
  existing `lastclock` cursor. This legacy look-ahead remains present and is
  deliberately not treated as a recording timestamp.
* `sdl2/soundmng.c:sound_play_cb` packs signed 32-bit stereo into the SDL
  signed 16-bit stereo format and emits silence when no PCM source is
  available.
* `sound/sound.c:sound_changeclock` establishes the existing output-grid
  fields, while `soundcfg.lastclock` remains the legacy guest-clock cursor.
  `SNDCSEC_ENTER/LEAVE` protects stream access; the existing `locks` guard
  prevents nested PCM-lock ownership.

The new `SOUNDSTREAMSTATE` reports the configured rate, current buffer,
reserve, remaining and pending counts, cumulative generated samples since the
last stream reset, reset epoch, `lastclock`, lock state and accounting overflow.
The cumulative counter is checked and saturating. It is an observation for
REC11 handover, not permission to reset a recording cursor to zero or to use
the audio callback as a time source.

The source generator remains driven by guest-time `sound_sync`, with the
existing callback look-ahead still able to fill the reserve. A future owner
must pause or stop the device and reconcile already generated, reserved and
pending data outside the callback/core lock before it starts recording. The
actual SDL obtained format is still not retained as a recording descriptor.
This checkpoint therefore preserves the known limitation instead of claiming
that audio ownership has already been solved.

## Checks

| Check ID | Exact command/environment | Exit code / outcome | Evidence |
|---|---|---:|---|
| REC10-FOCUSED-ON | `cmake --build build/macos-rec07-on --target vaeg_sound_generation_test vaeg_sdl2 -j2` followed by `ctest --test-dir build/macos-rec07-on --output-on-failure -R '^vaeg_sound_generation$'` | 0; 1/1 passed | Generator order/count, saturation, silence, look-ahead and boundary fixture |
| REC10-FOCUSED-OFF | `cmake --build build/macos-rec07-off --target vaeg_sound_generation_test vaeg_sdl2 -j2` followed by the same focused CTest | 0; 1/1 passed | Recording-disabled seam and no-FFmpeg configuration |
| REC10-WERROR | `cmake --build build/macos-rec08-werror --target vaeg_sound_generation_test -j2` followed by focused CTest | 0; 1/1 passed | New C99 seam/test compiled with warnings as errors |
| REC10-SANITIZER | `cmake --build build/macos-rec08-sanitize --target vaeg_sound_generation_test -j2 && ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 ctest --test-dir build/macos-rec08-sanitize --output-on-failure -R '^vaeg_sound_generation$'` | 0; 1/1 passed | AddressSanitizer/UndefinedBehaviorSanitizer; macOS leak detection disabled |
| REC10-SUITE-ON | `ctest --test-dir build/macos-rec07-on --output-on-failure -L recording` | 0; 11/11 passed | REC01–REC10 enabled recording suite |
| REC10-SUITE-OFF | `ctest --test-dir build/macos-rec07-off --output-on-failure -L recording` | 0; 7/7 passed | Recording-disabled regression suite |
| REC10-BUILD-ON | `cmake --build build/macos-rec07-on --target vaeg_sdl2 -j2` | 0 | Enabled frontend build |
| REC10-BUILD-OFF | `cmake --build build/macos-rec07-off --target vaeg_sdl2 -j2` | 0 | Disabled frontend build |
| REC10-OFF-FFMPEG | `! rg -ni 'ffmpeg|avcodec|avformat|avutil' build/macos-rec07-off/CMakeCache.txt build/macos-rec07-off/build.ninja` | 0 | No FFmpeg references in the disabled build configuration |
| REC10-ROMLESS-ON | `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy build/macos-rec07-on/sdl2/vaeg --selftest --no-cfg --no-bkupmem` and the same command with `--smoke` | 0 / 0 | Existing ROM-less selftest and smoke entry points passed |
| REC10-ROMLESS-OFF | Equivalent `--selftest` and `--smoke` commands using `build/macos-rec07-off/sdl2/vaeg` | 0 / 0 | Recording-disabled ROM-less regression passed |
| REC10-REPO | `git diff --check && python3 tools/repo/check_case.py && python3 tools/repo/check_encoding.py --expect utf8 && python3 tools/repo/check_eol.py --enforce && python3 tools/qa/milestone_ids.py --selftest --audit --discover` | 0 | Repository validators passed after the evidence update; discovery includes M100r11 |

No real device, Windows/Linux runtime, host-format handover, live recording,
VLC playback or human/release validation is claimed. The focused test is a
synthetic C99 fixture and does not prove A/V synchronization or physical audio
behavior.

## Integrity results

The focused fixture invokes two callbacks in the original order, verifies the
requested/generated frame count and compares all interleaved S32 samples with
the direct legacy reference. It verifies signed-16 saturation, exact byte
truncation, source-NULL silence, no-source/device output behavior and the
reserve-tail boundary at zero, equal, below and above reserve. Null and
overflow-sized inputs return safely.

The state observation is cumulative since `streamreset`; its epoch changes on
reset so a later owner can reject stale state. No live recorder sample count,
PTS, queue high-water mark, A/V cut or movie file applies yet. The new seam
does not change the existing sound clock or callback cadence.

## Review and limitations

The production seam is C99 and remains outside the ymfm core. It performs no
allocation or locking in the generator/packing operation. Existing core-lock
ownership is preserved, and the observation API is intended to be called from
an external handover path rather than recursively while the audio callback
holds `SNDCSEC`.

The remaining REC11 prerequisite is a single recording audio producer that
reconciles sound already generated by `sound_sync`, callback look-ahead,
reserve contents, pending PCM and `lastclock` without making device latency
the recording clock. The actual obtained SDL device format and any conversion
state also still need an explicit owner. Mute/device absence semantics require
the later recording integration; the source-NULL helper test is only the
existing silence path.

## Gate and next task

Machine gate: PASS.

Human gate: NOT_REQUIRED for this machine-only seam; no human or release
validation is claimed.

Push/remote state under repository policy: local only; no push, merge or
publication performed.
Next dependency-ready REC task and host requirement: REC11 / M100r12 on the
same macOS host, after this evidence commit.

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
