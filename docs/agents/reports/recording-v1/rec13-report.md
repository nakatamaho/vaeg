# REC13 report: integrated Native A/V session

Status: IMPLEMENTED_UNVERIFIED

Feature ID and mapped legal M ID: REC13 / M100r14
Assigned branch: `topic/m100r3-recording-clock`
Base SHA: `ec8ec57dcf18f6e448bf5301b97d6122676d0824`
Evaluated source SHA: `595d6ed2d575dcc3b3d36aa5dc5c554d4c4f8b79`
Evaluated binary/toolchain identity: macOS arm64, AppleClang, CMake,
MacPorts SDL2 and the repository's FFmpeg-enabled recording configuration
Evidence-only SHA, if applicable: Documentation commit immediately following
the evaluated source commits
Task/spec version: `docs/recording-v1/milestones/rec13-native-session.md`

No private ROM, media, absolute path, hash, screenshot or raw log is included
here.

## Scope and changes

REC13 connects the existing Native guest-boundary tap and REC11 guest-time
audio producer to one bounded session and the REC07 production MKV writer.
The implementation is private-controller-only; the public recording controls
remain a later milestone.

Changed files:

* `sdl2/recording/recording_native_session.cpp/.h` add the C-compatible
  frontend controller and the C++17 session owner. It obtains the canonical
  Native RGB24 descriptor, starts the indexed audio FIFO, derives one exact
  guest-clock/sample grid, copies complete Native frames and OSD snapshots,
  and owns the FFV1 v3 BGR0 plus PCM S16LE Matroska sink on the worker thread.
  The video packet PTS is the complete slice's begin tick and its duration is
  the exact slice interval, so the high-frequency VA clock cannot round a
  one-tick codec duration to zero.
* `sdl2/recording/recording_queue.cpp/.hpp` add cancellable bounded
  backpressure, worker finish/cancel hooks and sink failure detail propagation.
  The producer waits outside the core/audio lock and the worker drains or
  cancels without unbounded queue growth.
* `sdl2/scrnmng.c/.h` expose the current Native recording descriptor without
  changing the existing screenshot or QA capture paths.
* `machine/pccore.c` and `machine/statsave.c` stop an active session at reset,
  state-load and epoch transitions.
* `sdl2/np2.c` starts the private controller, commits after each completed
  guest execution boundary, requests a bounded frame-count stop and drains or
  cancels during normal/error shutdown. A start failure does not enter the
  normal run loop.
* `CMakeLists.txt` builds the optional session and its ROM-free integration
  test only when recording is enabled; recording-OFF still uses no FFmpeg
  session backend.
* `tests/frontend/recording/test_recording_queue.cpp` covers blocking
  backpressure and sink error detail. `test_recording_native_session.cpp`
  covers exact synthetic video/audio interval handoff, MKV creation, frame
  limit stop and epoch cancellation without publication.

The frozen user-facing source contract is unchanged: Native video and
Displayed video remain the only choices; both include enabled guest-area OSD;
Native is a recording-owned pre-CRT guest raster; Displayed remains the later
final-client presenter source. Existing guest-only screenshots, QA captures,
SDL/native presentation, recording-OFF behavior, C99 core boundaries and
private-data restrictions were preserved.

## Implementation findings and decisions

The production data flow is:

```text
guest screendispva boundary
    -> canonical RGB24 Native frame + REC08 OSD snapshot
    -> private staged complete slice

guest sound_sync
    -> indexed bounded S16 stereo FIFO
    -> exact [guest begin, guest end) sample range

complete owned video/audio slice
    -> bounded wait outside core/audio/GPU locks
    -> worker-thread FFV1/PCM encode
    -> seekable exclusive-partial MKV
    -> non-replacing final publication after trailer/flush
```

The first Native frame establishes the movie epoch and primes audio to the
ceiling sample boundary for that same guest tick. Each later frame closes the
previous interval. The writer receives the previous frame with PTS at the
interval begin and duration equal to the interval length. The audio FIFO is
read by sample index, never by callback arrival or host wall time. Reset,
state-load, clock/epoch changes, descriptor changes, tap rejection, audio
underflow and sink failure cancel the session and do not publish a final file.

The queue budget reserves active-interval and readback-staging storage and
limits admitted slices to eight plus the reserved byte budget. `wait_submit`
can be canceled by queue failure or session cancellation and does not wait
while holding the sound/core lock. The audio callback remains playback-only.

## Checks

| Check ID | Exact command/environment | Exit code / outcome | Evidence |
|---|---|---:|---|
| REC13-BUILD-ON | `cmake --build build/macos-rec13-on -j4` | 0 | Recording-enabled frontend and all configured targets built |
| REC13-BUILD-OFF | `cmake --build build/macos-rec13-off -j4` | 0 | Recording-disabled frontend and all configured targets built |
| REC13-SUITE-ON | `ctest --test-dir build/macos-rec13-on --output-on-failure -L recording` | 0; 14/14 passed | Existing recording contracts plus queue/session integration |
| REC13-SUITE-OFF | `ctest --test-dir build/macos-rec13-off --output-on-failure -L recording` | 0; 9/9 passed | Recording-OFF regression |
| REC13-WERROR | `cmake --build build/macos-rec13-werror --target vaeg_recording_native_session_test vaeg_recording_queue_test -j4 && ctest --test-dir build/macos-rec13-werror --output-on-failure -R '^(vaeg_recording_queue|vaeg_recording_native_session)$'` | 0; 2/2 passed | New C++ session and queue tests with Werror |
| REC13-SANITIZER | `cmake --build build/macos-rec13-sanitize --target vaeg_recording_native_session_test vaeg_recording_queue_test -j4 && ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 ctest --test-dir build/macos-rec13-sanitize --output-on-failure -R '^(vaeg_recording_queue|vaeg_recording_native_session)$'` | 0; 2/2 passed | ASan/UBSan; macOS leak detection disabled |
| REC13-ROMLESS-ON | `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy build/macos-rec13-on/sdl2/vaeg --selftest --no-cfg --no-bkupmem` | 0 | All selftests passed |
| REC13-ROMLESS-OFF | Equivalent command using `build/macos-rec13-off/sdl2/vaeg` | 0 | All selftests passed |
| REC13-SMOKE-ON/OFF | `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy .../vaeg --smoke --no-cfg --no-bkupmem` in both builds | 0/0 | Expected ROM-less warning; both smoke runs completed |
| REC13-LIVE-NATIVE | Private-ROM-root run with `VAEG_RECORDING_NATIVE_PATH=<private-temp>.mkv`, `VAEG_RECORDING_NATIVE_FRAMES=4`, dummy SDL video/audio, `--no-cfg --no-bkupmem --model va2` | 0 | Four complete Native intervals produced a normal final MKV; private input/output identity is intentionally redacted |
| REC13-MOVIE-STRUCTURE | `ffprobe -v error -count_frames -count_packets ...` on the private output | 0 | One FFV1/BGR0 640x400 stream and one PCM S16LE stereo 22050Hz stream; 4 video and 4 audio packets/decoded frames |
| REC13-MOVIE-DECODE | `ffmpeg -v error -i <private-output>.mkv -map 0:v:0 -f null -` and the equivalent audio command | 0/0 | Video and audio decoded without errors |
| REC13-REPO | `git diff --check` | 0 | No whitespace errors in the evaluated source changes |

The live run used private ROM input and a deterministic dummy host device. It
did not have the required guest OS/media demonstration with motion, sound and
enabled OSD, so it is source-integration evidence, not the REC13 human gate.
VLC was unavailable on this host; no VLC acceptance is claimed. The existing
`verify_movie.py` fixture verifier requires a manifest and expected RGB/PCM
fixtures, so it was not used as a claim about this private live output.

## Integrity results

The synthetic session test uses a 1000-tick guest clock and 100Hz audio grid;
three submitted boundaries produce exactly three audio frames and a normal
MKV, while an epoch change cancels and leaves no final publication. The live
four-interval output decoded as four 640x400 FFV1 frames. Its first four video
PTS values were 0, 18, 35 and 53 in the post-header millisecond stream time
base. The PCM packet payloads totalled 6252 bytes, exactly 1563 stereo S16
frames; no resampling or sample synthesis was performed by the recorder.

The bounded queue and focused tests cover ownership mutation, interval/sequence
continuity, byte/count budgets, blocking wakeup, cancellation and worker
failure. The live run did not publish a partial file; earlier pre-fix attempts
were canceled after exposing the zero-duration high-frequency-clock bug and
are not acceptance artifacts.

## Review and limitations

The producer performs only bounded frame/audio copies and queue admission on
the guest thread. Encoding, muxing, filesystem I/O and final publication run
on the worker. No audio callback, emulator-core lock or GPU lock performs
blocking queue work. The session is currently private-controller-only and
Native-only; REC14 must add the user-facing control without introducing a
third source or an OSD filter.

Only the macOS host was built and exercised here. Windows/MinGW, Linux,
Metal/D3D11/OpenGL presenter capture, physical audio devices, Displayed video,
VLC playback, guest OS/media motion and human acceptance remain unverified.
The normal full frontend Werror target is not claimed because the repository
still contains unrelated pre-existing warning failures; the changed recording
targets were built with Werror.

## Gate and next task

Machine gate: PASS for the REC13 engineering implementation and available
source-integration checks.

Human gate: PENDING. The required maintainer-approved guest demonstration and
explicit acceptance have not occurred.

Push/remote state under the current goal policy: local only; no push, merge or
publication performed.

Next dependency-ready REC task and host requirement: REC14 / M100r15 on the
same macOS host for the public recording control, while preserving this
report's pending human/release validation state.

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
