# REC05 report: FFV1 RGB encoder and exact pixel round trip

Status: PASS

Feature ID and mapped legal M ID: REC05 / M100r6
Assigned branch: `topic/m100r3-recording-clock`
Base SHA: `074af6efe45aa1a4610f96232902706de9208e0c`
Evaluated source SHA: `9c462a968a45f8559734710f7aeb1f1348628dae`
Evaluated binary/toolchain identity: macOS arm64, AppleClang 21.0.0.21000101, CMake 3.31.12, Ninja 1.13.2, MacPorts SDL2 2.32.10, FFmpeg 8.1.2
Evidence-only SHA, if applicable: Not applicable
Task/spec version: `docs/recording-v1/milestones/rec05-ffv1.md`

## Scope and changes

REC05 adds a private C++17 FFV1 video encoder for owned packed RGB frames. It
does not connect the emulator, produce an MKV, encode audio, add UI, or change
screenshots and QA captures.

Changed files:

* `CMakeLists.txt` builds the encoder only when `VAEG_ENABLE_RECORDING=ON` and
  registers its focused test with the existing optional FFmpeg targets.
* `sdl2/recording/recording_ffmpeg_video_encoder.hpp` exposes a private-safe
  C++ API with stable error values, owned packet data and no FFmpeg types in a
  C-facing interface.
* `sdl2/recording/recording_ffmpeg_video_encoder.cpp` owns the FFmpeg codec
  context, frame and packet with RAII; selects BGR0, FFV1 version 3, GOP 1,
  and slice CRC; converts RGB24 and little-endian RGB565 while respecting
  source/destination stride; and preserves sequence/PTS associations.
* `tests/frontend/recording/test_recording_ffmpeg_video_encoder.cpp` decodes
  packets with the FFV1 decoder and compares normalized RGB24 bytes against
  independent expected fixtures. It also exercises invalid input and
  allocation/open/send/receive/flush failures.
* This REC05 task wrapper, report, status, handoff and ID map record the gate.

Native video and Displayed video remain the only two source choices, both with
enabled guest-area OSD. Native recording-owned composition, Displayed final
client composition, PCM/guest-time accounting, bounded queues, the C99 core
boundary, private-data rules and existing screenshot/QA contracts are
unchanged.

## Implementation findings and decisions

The encoder uses the FFV1 codec returned by `avcodec_find_encoder` and opens a
packed `AV_PIX_FMT_BGR0` context. It verifies `level == 3`, `gop_size == 1`,
the packed pixel format and `slicecrc == 1` after opening. Each submitted
frame is marked as an I picture and has duration one in the caller-provided
time base. Codec extradata is copied into the private encoder information so
the later muxer/decoder boundary can initialize FFV1 without relying on a
packet-local header.

RGB24 input is interpreted as R,G,B and repacked to B,G,R, with the fourth
byte and all destination padding initialized to zero. RGB565 input is read as
little-endian 5/6/5 values and expanded with the audited integer mapping
`component * 255 / max_component`. The test fixtures cover dimensions 7x5 and
3x2, nonblack channel corners, fine color transitions, seeded varied pixels,
row padding and odd/small dimensions. The decoder normalizes BGR0 back to
RGB24 and matched the expected bytes for all three frames in both fixtures.

The send/receive path drains `EAGAIN`, maps every packet PTS to its pending
sequence, and reports timestamp, allocation and packet-copy failures instead
of silently dropping output. Flush sends the null frame, drains through EOF,
checks that no pending ledger entry remains, and is idempotently rejected on a
second call. Pending ledger registration occurs before the send call so an
allocation failure cannot leave an accepted frame without an association.

The local MacPorts FFmpeg development libraries are the same 8.1.2
GPL/nonfree-enabled development build audited in REC04. This is development
test evidence only; it is not release-package license evidence.

## Checks

| Check ID | Exact command/environment | Exit code / outcome | Evidence |
|---|---|---:|---|
| REC05-ON-CONFIG | `cmake -S . -B build/macos-rec05-on -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/opt/local -DVAEG_ENABLE_TESTS=ON -DVAEG_Z80_COMPAT_INTEGRATION_TRACE=ON -DVAEG_ENABLE_LIBRASHADER=OFF -DVAEG_ENABLE_RECORDING=ON` | 0 | All optional FFmpeg targets found |
| REC05-ON-BUILD | `cmake --build build/macos-rec05-on --target vaeg_recording_c_contract_test vaeg_recording_contract_test vaeg_recording_clock_test vaeg_recording_queue_test vaeg_recording_ffmpeg_capabilities_test vaeg_recording_ffv1_test vaeg_sdl2 -j2` | 0 | Encoder test and enabled frontend built |
| REC05-ON-TEST | `ctest --test-dir build/macos-rec05-on --output-on-failure -R '^vaeg_recording_'` | 0; 6/6 passed | Existing contracts/clock/queue, FFmpeg capability and FFV1 tests |
| REC05-OFF-CONFIG/BUILD | `cmake -S . -B build/macos-rec05-off -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/opt/local -DVAEG_ENABLE_TESTS=ON -DVAEG_Z80_COMPAT_INTEGRATION_TRACE=ON -DVAEG_ENABLE_LIBRASHADER=OFF -DVAEG_ENABLE_RECORDING=OFF` followed by the four existing recording test targets and `vaeg_sdl2` | 0 | Recording-disabled frontend built; generated `CMakeCache.txt`/`build.ninja` contained no FFmpeg references |
| REC05-OFF-TEST | `ctest --test-dir build/macos-rec05-off --output-on-failure -R '^vaeg_recording_'` | 0; 4/4 passed | Recording-OFF regression |
| REC05-SANITIZER | Debug configure with `-fsanitize=address,undefined -fno-omit-frame-pointer` in C/C++ and executable linker flags, then `cmake --build build/macos-rec05-sanitize --target vaeg_recording_ffv1_test -j2` and `ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 ctest --test-dir build/macos-rec05-sanitize --output-on-failure -R '^vaeg_recording_ffv1$'` | 0; 1/1 passed | Normal and injected-failure cleanup under ASan/UBSan; macOS leak detection was disabled |
| REC05-ROMLESS | `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/macos-rec05-off/sdl2/vaeg --selftest --no-cfg --no-bkupmem` and the corresponding `--smoke` command | 0 / 0 | Existing self-test passed; smoke retained only the expected missing-ROM warning |
| REC05-NO-SUBPROCESS | `rg -n 'system\\(|popen\\(|posix_spawn|CreateProcess|execvp|execl' sdl2/recording/recording_ffmpeg_video_encoder.cpp` | 1 (no matches) | Encoder does not launch an FFmpeg process |

The FFV1 test is a synthetic private-backend test. No claim is made for a
Windows/Linux runtime, a real SDL presenter, VLC playback or release-package
dependencies in this milestone.

## Integrity results

The RGB24 7x5 and RGB565 3x2 fixtures each encoded three frames. The output
contained three nonempty packets, all keyframes, with sequences 100, 101, 102
and PTS 200, 201, 202. Independent decoder output matched the expected
normalized RGB24 byte arrays exactly for every frame. The configured GOP was
one, FFV1 level/version three and slice CRC were verified, and no YUV fallback
was used. There is no file-level frame count, seekability, trailer, PCM sample
count or A/V alignment result yet because REC05 does not include the muxer or
audio path.

## Review and limitations

FFmpeg headers are confined to the private C++ implementation and test. The
public recording C header and core-facing interfaces remain free of FFmpeg
types. Codec/frame/packet ownership is RAII-based, source and destination
strides are explicit, and packet output is copied into owned vectors before
return. The optional target remains disabled by default.

The encoder is not yet connected to Native or Displayed video, OSD composition,
guest-time production, audio, the bounded worker queue, Matroska, cancellation,
geometry-change policy or the UI. Those are later REC milestones. No private
ROM/media names, paths, hashes, screenshots or logs are included here.

## Gate and next task

Machine gate: PASS.
Human gate: NOT_REQUIRED for this machine-only checkpoint; no human or release
validation is claimed.
Push/remote state under repository policy: local only; no push, merge or
publication performed.
Next dependency-ready REC task and host requirement: REC06 / M100r7, using the
optional FFmpeg development backend and the current recording contract.
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
