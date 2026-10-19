# REC07 report: seekable Matroska writer and combined A/V fixture

Status: PASS

Feature ID and mapped legal M ID: REC07 / M100r8
Assigned branch: `topic/m100r3-recording-clock`
Base SHA: `9379f14d8db175666ebffd0a2d375c274a49a036`
Evaluated source SHA: `2a3c78583767624aea239c37cb8b9c1beb5a9f7c`
Evaluated binary/toolchain identity: macOS arm64, AppleClang 21.0.0.21000101, CMake 3.31.12, Ninja 1.13.2, MacPorts SDL2 2.32.10, FFmpeg 8.1.2 (`libavutil` 60.26.102, `libavcodec` 62.28.102, `libavformat` 62.12.102)
Evidence-only SHA, if applicable: Not applicable
Task/spec version: `docs/recording-v1/milestones/rec07-matroska.md`

## Scope and changes

REC07 adds the private C++17 Matroska output boundary that combines the REC05
FFV1 and REC06 PCM packet contracts into one normal local seekable MKV. It does
not connect live guest video/audio production, presenter readback, OSD drawing,
the recording queue, UI, or the SDL audio callback.

Changed files:

* `CMakeLists.txt` adds the optional Matroska writer and its ON-only focused
  CTest target. Recording-OFF still excludes all FFmpeg sources.
* `sdl2/recording/recording_ffmpeg_matroska.hpp` defines the private writer,
  complete-interval input, metadata-safe configuration, stream-timebase result
  and stable error names without crossing a C core interface.
* `sdl2/recording/recording_ffmpeg_matroska.cpp` creates an explicit Matroska
  context, both codec-parameter streams, custom seekable AVIO over an exclusive
  local file descriptor, post-header timebase capture, one-time timestamp
  conversion, bounded complete-interval interleaving, trailer/flush/close and
  non-replacing final publication. POSIX publication uses `link` followed by
  removal of the owned partial; Windows uses `MoveFileExW` without replace.
* `tests/frontend/recording/test_recording_ffmpeg_matroska.cpp` builds a
  production-encoder A/V fixture, writes independent RGB/PCM expectations and
  a timing manifest outside the repository, decodes the resulting MKV with
  libavformat/libavcodec, checks stream metadata/seekability and exercises
  target/partial ownership, cancellation and create/write/seek/header/trailer/
  flush/close/publication failures.
* `tools/recording/verify_movie.py` is an independent ffprobe/ffmpeg validator
  for the public fixture contract. It checks the two streams, packet ordering,
  metadata, decoded RGB/PCM bytes and exact frame/sample counts. It is a test
  tool only; production recording does not launch an FFmpeg process.
* `docs/agents/tasks/M100r8_recording_v1_rec07.md` assigns the reserved legal
  task ID.

Native and Displayed remain the only two source choices, both with enabled
guest-area OSD. Native recording-owned composition, Displayed final-client
composition, guest-time audio ownership, bounded queues, canonical screenshots
and QA, the C99 core boundary, private-data rules and legacy playback paths are
unchanged.

## Implementation findings and decisions

The writer receives complete encoded intervals rather than individual live
callbacks. It rejects empty intervals, malformed packets, non-contiguous PCM
sample indices and packet payload sizes that do not equal the declared stereo
S16 duration. Video and audio packets are ordered by their common relative
guest-time epoch before `av_interleaved_write_frame`; no stream is supplied as
an unbounded independent producer.

The writer uses the encoder timebases `1/30` for the synthetic video and
`1/48000` for the synthetic PCM, then reads the actual stream timebases after
`avformat_write_header`. FFmpeg 8.1.2's Matroska muxer returned `1/1000` for
both streams. Video and audio epoch offsets are separate configuration values;
the PCM origin fraction is included in the timestamp calculation and public
format metadata is limited to source, OSD inclusion, recording format and a
public build identity.

The final filename must be local and end in `.mkv`. The writer creates a unique
sibling partial with `O_EXCL`/the Windows equivalent through a custom AVIO
handle, so FFmpeg never interprets the filesystem path as a URL and no
check-then-open overwrite is used. Existing targets and partial names are
preserved. A completed partial is published without replacement; failures
retain nonempty partial data and never report success. A cancel before any
interval does not create a successful empty movie.

## Checks

| Check ID | Exact command/environment | Exit code / outcome | Evidence |
|---|---|---:|---|
| REC07-ON-CONFIG | `cmake -S . -B build/macos-rec07-on -G Ninja -DVAEG_ENABLE_RECORDING=ON -DVAEG_ENABLE_TESTS=ON -DVAEG_Z80_COMPAT_INTEGRATION_TRACE=ON -DVAEG_WERROR=OFF` | 0 | All three target-scoped FFmpeg libraries found |
| REC07-ON-BUILD | `cmake --build build/macos-rec07-on -j2` | 0 | Enabled frontend and all recording tests built |
| REC07-ON-TEST | `ctest --test-dir build/macos-rec07-on -R '^vaeg_recording_' --output-on-failure` | 0; 8/8 passed | Contracts, clock, queue, capability, FFV1, PCM and Matroska tests |
| REC07-FIXTURE | `VAEG_RECORDING_KEEP_FIXTURE=1 ./build/macos-rec07-on/vaeg_recording_matroska_test` | 0 | 5x3 RGB fixture and 48000 Hz stereo PCM fixture generated and decoded |
| REC07-PROBE | `ffprobe -v error -show_format -show_streams -of json <temporary-fixture>/fixture.mkv` | 0 | Exactly one FFV1 video stream and one PCM S16LE audio stream; both stream timebases `1/1000`; Matroska metadata present |
| REC07-INDEPENDENT-VERIFY | `python3 tools/recording/verify_movie.py --movie <temporary-fixture>/fixture.mkv --manifest <temporary-fixture>/timing-manifest.json --expected-rgb <temporary-fixture>/expected.rgb --expected-pcm <temporary-fixture>/expected.pcm` | 0; PASS | ffprobe/ffmpeg decode, packet ordering, RGB/PCM comparison and count checks |
| REC07-OFF-CONFIG/BUILD | `cmake -S . -B build/macos-rec07-off -G Ninja -DVAEG_ENABLE_RECORDING=OFF -DVAEG_ENABLE_TESTS=ON -DVAEG_Z80_COMPAT_INTEGRATION_TRACE=ON -DVAEG_WERROR=OFF` followed by `cmake --build build/macos-rec07-off --target vaeg_sdl2 vaeg_recording_c_contract_test vaeg_recording_contract_test vaeg_recording_clock_test vaeg_recording_queue_test -j2` | 0 | Recording-disabled frontend built; generated CMake files contain no FFmpeg references |
| REC07-OFF-TEST | `ctest --test-dir build/macos-rec07-off -R '^vaeg_recording_' --output-on-failure && ! rg -ni 'ffmpeg|avcodec|avformat|avutil' build/macos-rec07-off/CMakeCache.txt build/macos-rec07-off/build.ninja` | 0; 4/4 passed | Recording-OFF regression and no generated FFmpeg reference |
| REC07-SANITIZER | Sanitized ON configure with `-fsanitize=address,undefined` in C/C++ and executable linker flags, then `cmake --build build/macos-rec07-sanitize --target vaeg_recording_matroska_test -j2` and `ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 ./build/macos-rec07-sanitize/vaeg_recording_matroska_test` | 0 | Matroska fixture and injected failures passed under ASan/UBSan; macOS leak detection was disabled |
| REC07-ROMLESS | `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/macos-rec07-on/sdl2/vaeg --selftest --no-cfg --no-bkupmem` and the corresponding `--smoke` command; repeated for `build/macos-rec07-off` | 0 / 0 for ON and OFF | Existing self-test passed; smoke retained only its expected missing-ROM warning |
| REC07-PYTHON | `PYTHONPYCACHEPREFIX=/private/tmp/vaeg-rec07-pycache python3 -m py_compile tools/recording/verify_movie.py && python3 tools/recording/verify_movie.py --help >/dev/null` | 0 | Validator syntax and CLI help |
| REC07-REPO | `git diff --check`, `python3 tools/repo/check_case.py`, `python3 tools/repo/check_encoding.py --expect utf8`, `python3 tools/repo/check_eol.py --enforce` | 0 | No whitespace, case, UTF-8 or EOL violations |

The fixture path is intentionally represented by `<temporary-fixture>` here;
the generated MKV, expected files and manifest were temporary synthetic test
outputs and were not committed.

## Integrity results

The fixture contains three independent 5x3 RGB frames and three contiguous
stereo PCM blocks of 1600 frames each. Decoded FFV1 pixels exactly equal the
independent expected RGB bytes for all 45 pixels per frame. Decoded PCM exactly
equals the independently saturated and packed 4800-frame S16LE stream, with
both channels and clipping boundaries preserved. The output has exactly three
video packets and three audio packets, with a 100 ms common endpoint. The
fixture's video boundaries are 0, 1/30 and 2/30 seconds and its audio block
starts are sample indices 0, 1600 and 3200, so the marker boundaries coincide
before Matroska's 1 ms timestamp quantization; ffprobe reports 0, 33 and 67 ms
for both streams.

The normal file is seekable and readable after final publication. Existing
final targets and an existing requested partial path retain their sentinel
contents. Injected create, write, seek, header, trailer, flush, close and
publication failures produce no final success and retain any nonempty owned
partial. The no-interval cancel path does not publish an empty MKV.

## Review and limitations

FFmpeg headers and types remain private to the C++ frontend implementation and
tests; no FFmpeg or STL type crosses the C99 boundary. The custom AVIO performs
bounded writes, seek/tell/size, flush and close on an OS file handle. The
writer's only buffered input is one complete interval's packet vectors; it does
not allocate or perform I/O in an audio callback or GPU lock.

This milestone is synthetic backend evidence only. It does not prove live
guest-time production, OSD snapshot/composition, SDL/D3D11/OpenGL/Metal
Displayed readback, UI lifecycle, VLC playback, Windows/Linux/macOS runtime
packaging or FFmpeg release-license compliance. Read-only filesystem behavior,
multi-gigabyte sparse-file behavior and platform-specific Unicode handle
runtime remain explicit later platform/fault-validation work. No human gate or
release validation is claimed.

## Gate and next task

Machine gate: PASS. M01-M04, T03, V01 and A01 are covered by the synthetic
combined fixture and its independent decode/content checks.

Human gate: NOT_REQUIRED for this machine-only checkpoint; no human or release
validation is claimed.
Push/remote state under repository policy: local only; no push, merge or
publication performed.
Next dependency-ready REC task and host requirement: REC08 / M100r9, same
optional FFmpeg development backend; it must add the immutable OSD snapshot
contract without connecting a live source prematurely.
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
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->
