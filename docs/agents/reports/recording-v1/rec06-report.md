# REC06 report: PCM S16LE stereo encoder and exact sample round trip

Status: PASS

Feature ID and mapped legal M ID: REC06 / M100r7
Assigned branch: `topic/m100r3-recording-clock`
Base SHA: `c48b216b6cdb3a168c90f22a43111b2926689c6a`
Evaluated source SHA: `2c990ffa618c05e1a7acaf69bd23bfb9c708628b`
Evaluated binary/toolchain identity: macOS arm64, AppleClang 21.0.0.21000101, CMake 3.31.12, Ninja 1.13.2, MacPorts SDL2 2.32.10, FFmpeg 8.1.2
Evidence-only SHA, if applicable: Not applicable
Task/spec version: `docs/recording-v1/milestones/rec06-pcm.md`

## Scope and changes

REC06 adds a private C++17 PCM S16LE stereo encoder for final-mix-style signed
32-bit samples. It does not connect live sound generation, the SDL callback,
the recording queue, the Matroska muxer, or any user-facing control.

Changed files:

* `CMakeLists.txt` builds the PCM encoder only when
  `VAEG_ENABLE_RECORDING=ON` and registers its focused test under the existing
  optional FFmpeg targets.
* `sdl2/recording/recording_ffmpeg_pcm_encoder.hpp` defines the private
  encoder/configuration, declared origin offset, cumulative sample block and
  owned packet contracts without FFmpeg types in a C interface.
* `sdl2/recording/recording_ffmpeg_pcm_encoder.cpp` configures the
  capability-tested PCM S16LE codec at the caller's effective sample rate,
  reuses the existing `satuation_s16` final saturation helper, explicitly
  writes little-endian bytes, preserves block/sample-index PTS and drains or
  flushes with RAII ownership.
* `tests/frontend/recording/test_recording_ffmpeg_pcm_encoder.cpp` independently
  decodes packets and compares exact S16LE bytes, channel order, counts, PTS,
  durations and origin metadata across regular and irregular partitions. It
  also exercises malformed input and allocation/open/send/receive/flush paths.
* This REC06 task wrapper, report, status and handoff record the gate.

Native video and Displayed video remain the only two source choices, both with
enabled guest-area OSD. Native recording-owned composition, Displayed final
client composition, guest-time ownership, bounded queues, canonical
screenshots/QA, the C99 core boundary, private-data rules and existing playback
audio behavior are unchanged.

## Implementation findings and decisions

The encoder accepts a positive configured sample rate and records it as the
PCM stream rate; the test uses the audited default 22,050 Hz but the encoder
does not derive it from wall time, video frames or a hardcoded callback size.
The codec time base is one tick per audio sample. Stereo is fixed at two
channels and 16 bits, and the private info reports the exact declared origin
offset fraction.

Each input block declares an interleaved signed-32-bit pointer, byte capacity,
frame count and first cumulative sample index. The first index establishes the
stream position; every later block must begin at the immediately following
index. Sample count comes only from the declared block frame count. The
existing `satuation_s16` helper performs the same final -32768..32767 clamp as
the playback path, after which `AV_WL16` makes the stored PCM representation
explicitly little-endian. No new resampler or sound-chip path was added.

The send/receive path maps each packet back to its first sample index and
duration, reports missing/mismatched timestamps, drains `EAGAIN`, and retains
owned packet bytes. Flush drains to EOF, rejects leftover sample-ledger
entries, and rejects a second flush. The implementation refuses non-little-
endian hosts for this S16LE/native FFmpeg sample-format boundary; the supported
VAEG release platforms are little-endian.

## Checks

| Check ID | Exact command/environment | Exit code / outcome | Evidence |
|---|---|---:|---|
| REC06-ON-CONFIG | `cmake -S . -B build/macos-rec06-on -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/opt/local -DVAEG_ENABLE_TESTS=ON -DVAEG_Z80_COMPAT_INTEGRATION_TRACE=ON -DVAEG_ENABLE_LIBRASHADER=OFF -DVAEG_ENABLE_RECORDING=ON` | 0 | libavutil/libavcodec/libavformat 60.26.102/62.28.102/62.12.102 found |
| REC06-ON-BUILD | `cmake --build build/macos-rec06-on --target vaeg_recording_c_contract_test vaeg_recording_contract_test vaeg_recording_clock_test vaeg_recording_queue_test vaeg_recording_ffmpeg_capabilities_test vaeg_recording_ffv1_test vaeg_recording_pcm_test vaeg_sdl2 -j2` | 0 | PCM test and enabled frontend built |
| REC06-ON-TEST | `ctest --test-dir build/macos-rec06-on --output-on-failure -R '^vaeg_recording_'` | 0; 7/7 passed | Previous recording contracts plus FFmpeg capability, FFV1 and PCM tests |
| REC06-OFF-CONFIG/BUILD | `cmake -S . -B build/macos-rec06-off -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/opt/local -DVAEG_ENABLE_TESTS=ON -DVAEG_Z80_COMPAT_INTEGRATION_TRACE=ON -DVAEG_ENABLE_LIBRASHADER=OFF -DVAEG_ENABLE_RECORDING=OFF` followed by the four existing recording test targets and `vaeg_sdl2` | 0 | Recording-disabled frontend built; generated `CMakeCache.txt`/`build.ninja` contained no FFmpeg references |
| REC06-OFF-TEST | `ctest --test-dir build/macos-rec06-off --output-on-failure -R '^vaeg_recording_'` | 0; 4/4 passed | Recording-OFF regression |
| REC06-SANITIZER | Debug configure with `-fsanitize=address,undefined -fno-omit-frame-pointer` in C/C++ and executable linker flags, then `cmake --build build/macos-rec06-sanitize --target vaeg_recording_pcm_test -j2` and `ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 ctest --test-dir build/macos-rec06-sanitize --output-on-failure -R '^vaeg_recording_pcm$'` | 0; 1/1 passed | Normal and injected-failure ownership under ASan/UBSan; macOS leak detection was disabled |
| REC06-ROMLESS | `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/macos-rec06-off/sdl2/vaeg --selftest --no-cfg --no-bkupmem` and the corresponding `--smoke` command | 0 / 0 | Existing self-test passed; smoke retained only the expected missing-ROM warning |
| REC06-NO-SUBPROCESS | `rg -n 'system\(|popen\(|posix_spawn|CreateProcess|execvp|execl' sdl2/recording/recording_ffmpeg_pcm_encoder.cpp` | 1 (no matches) | Encoder does not launch an FFmpeg process |

The PCM test is a synthetic private-backend test. No claim is made for live
sound ownership, callback independence, Matroska playback, Windows/Linux
runtime or release-package dependencies in this milestone.

## Integrity results

The fixture contained 11 stereo sample frames with independent channel ramps,
impulses, exact silence and clipping-boundary values. It was encoded once as
irregular blocks of 3, 1, 4, 1 and 2 frames and once as one 11-frame block.
Both decoded streams exactly matched the independently packed S16LE bytes,
including stereo order and -32768/32767 saturation. Irregular packets retained
first indices 41, 44, 45, 49 and 50, PTS equal to those indices, and durations
3, 1, 4, 1 and 2; the single packet retained PTS 41 and duration 11. The
declared origin offset `1/44100` and configured rate 22050 were preserved.
No sample count is based on callback arrival or video-frame count.

There is no MKV stream count, seekability, trailer, final-file, audio/video
alignment or live guest-waveform result yet because REC06 deliberately stops
at the PCM encoder boundary.

## Review and limitations

FFmpeg headers remain in the private C++ implementation and test only. The
recording C header and core-facing interfaces contain no FFmpeg or STL types.
The existing C99 saturation helper is reused, the packed output is copied into
owned FFmpeg packets, and failed setup/send/receive/flush paths leave no
borrowed encoder objects. The optional target remains disabled by default.

The encoder is not connected to `sound_sync`, `sound_pcmlock`, the audio
callback, guest-time producer ownership, mute/device-absence policy, queue
assembly, Matroska, cancellation, UI or A/V synchronization. REC10/REC11 and
later milestones must perform that integration without duplicating synthesis
or silently discarding existing look-ahead.

## Gate and next task

Machine gate: PASS.
Human gate: NOT_REQUIRED for this machine-only checkpoint; no human or release
validation is claimed.
Push/remote state under repository policy: local only; no push, merge or
publication performed.
Next dependency-ready REC task and host requirement: REC07 / M100r8, using the
optional FFmpeg development backend and the current FFV1/PCM components.
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
