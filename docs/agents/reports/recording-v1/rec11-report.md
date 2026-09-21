# REC11 report: guest-time recording audio producer

Status: PASS

Feature ID and mapped legal M ID: REC11 / M100r12

Assigned branch: `topic/m100r3-recording-clock`

Base SHA: `a3be74700fff0bb4b9b5a14979f33a462d865924`

Evaluated source SHA: `4165d9fda677ac0a9ff3ed954c0100d0a988dcfc`

Evidence-only SHA, if applicable: Documentation commit immediately following the evaluated source commit

Task/spec version: `docs/recording-v1/milestones/rec11-audio-tap.md`

Evaluated binary/toolchain identity: macOS arm64, AppleClang 21.0.0.21000101,
CMake 3.31.12, Ninja 1.13.2, MacPorts SDL2 2.32.10

## Scope and changes

REC11 establishes a recording-only guest-time audio ownership path. While it is
active, `sound_sync` is the only synthesis producer. It generates the existing
mixed S32 stereo stream once, converts that result to final S16 stereo in a
bounded owned FIFO, and maintains a separate bounded playback buffer. The SDL
callback consumes playback data or playback-only underrun silence; it does not
advance synthesis or the recording sample cursor.

Changed files:

* `sound/sound_capture.h` and `.c` add the C99 bounded interleaved-S16 FIFO.
  Appends are atomic with respect to capacity, preserve sample indices and
  generation epoch, support counted mute silence and expose nonblocking reads.
* `sound/sound.c` adds recording arming, stop/release, state/read APIs,
  guest-time chunked generation, playback-only consumption, transition failure
  on reset/clock epoch changes, and playback monitor drop/underrun counters.
  Arming copies already-generated pending playback samples into the recording
  FIFO rather than regenerating or silently discarding them.
* `sound/sound.h` exposes only plain C data and functions for the recording
  owner; no C++ or FFmpeg type crosses the core boundary.
* `sdl2/soundmng.c` switches the device callback to the playback-only reader
  while recording and keeps the legacy callback path when recording is off. It
  also retains the obtained SDL format in a plain `SOUNDMNGSPEC` observation.
* `sdl2/soundmng.h` declares the obtained-device observation without exposing
  SDL types through the sound-core interface.
* `CMakeLists.txt` builds the FIFO as core code and adds its ROM-free CTest.
* `tests/frontend/recording/test_sound_capture.c` tests delayed consumption,
  wraparound, mute silence, exact sample indices/epoch and atomic capacity
  rejection.
* `docs/agents/tasks/M100r12_recording_v1_rec11.md` assigns the legal M ID and
  records the machine-gate result.

Native video and Displayed video remain the only recording sources. Native OSD,
Displayed composition, screenshots, QA captures, ymfm internals, chip clocks,
the output sample-rate policy and existing recording-OFF behavior are not
changed by this milestone.

## Implementation findings and decisions

The legacy path still runs through `sound/sound.c:sound_sync` to
`streamprepare`, and `sdl2/soundmng.c:sound_play_cb` still uses
`sound_pcmlock`/`sound_pcmunlock` when recording is inactive. The new active
path is:

```text
guest sound_sync
    -> one REC10 S32 generator dispatch
    -> bounded final S16 recording FIFO
    -> recording owner reads indexed blocks

same generated block
    -> bounded playback FIFO
    -> SDL callback consumes only, or emits playback-only underrun silence
```

`sound_recording_start(capacity_frames)` first stops the SDL device, takes the
REC10 stream accounting under `SNDCSEC`, and rejects a locked transition, an
invalid capacity, an accounting overflow or a capacity too small for existing
pending playback. Existing pending S32 frames are copied once into the
recording FIFO. The FIFO's first sample is
`generated_sample_count - pending_samples`, so the handover retains the
already-generated samples and exposes their exact index; the coordinator can
apply the declared start boundary without resetting chip phase or pretending
that the pending tail starts at sample zero.

During recording, `sound_recording_generate_locked` checks the entire requested
guest-time sample interval before invoking any chip callback, then processes it
in fixed 512-frame chunks. If the bounded recording FIFO cannot accept the
complete interval, recording is failed before synthesis and the legacy path is
used to keep emulation audio usable. If an unexpected post-generation
accounting or append failure occurs, recording is failed and the generated
samples are never reported as a successful partial recording. Playback may
drop stale monitoring frames when its separate FIFO is full; those counters
are not recording drops.

Application mute still runs the synthesis callbacks and appends counted zero
frames to the recording FIFO. A missing SDL device leaves the sound core and
recording producer usable. `soundmng_getspec` reports whether a device exists
and, when it does, the obtained rate, format, channels and buffer size for the
later recording descriptor owner.

Stop makes the producer inactive while retaining the FIFO for draining;
release frees it only after the owner is finished. Reset and clock changes mark
the retained session with an epoch error instead of replaying or reinitializing
sound hardware. The callback contains no encoder, file I/O, recording queue
wait or unbounded allocation.

This milestone does not yet connect `sound_recording_read` to the REC07 MKV
writer or the interval coordinator. It provides the ownership boundary needed
by those later tasks; no live movie is claimed.

## Checks

| Check ID | Exact command/environment | Exit code / outcome | Evidence |
|---|---|---:|---|
| REC11-FIFO-ON | `cmake --build build/macos-rec07-on --target vaeg_sound_capture_test -j2 && ctest --test-dir build/macos-rec07-on --output-on-failure -R '^vaeg_sound_capture$'` | 0; 1/1 passed | Owned S16 FIFO, delayed read, wraparound, mute, sample indices/epoch and atomic capacity failure |
| REC11-FIFO-OFF | Equivalent command in `build/macos-rec07-off` | 0; 1/1 passed | Recording-disabled configuration exercises the C99 ownership primitive |
| REC11-WERROR | `cmake --build build/macos-rec08-werror --target vaeg_sound_capture_test -j2 && ctest --test-dir build/macos-rec08-werror --output-on-failure -R '^vaeg_sound_capture$'`; direct Werror compilation of changed `sound/sound.c` and `sdl2/soundmng.c` from the generated Ninja commands | 0; 1/1 passed | New FIFO test and changed production C units compile with warnings as errors |
| REC11-SANITIZER | `cmake --build build/macos-rec08-sanitize --target vaeg_sdl2 vaeg_sound_capture_test -j2 && ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 ctest --test-dir build/macos-rec08-sanitize --output-on-failure -R '^vaeg_sound_capture$'` | 0; 1/1 passed | AddressSanitizer/UndefinedBehaviorSanitizer; macOS leak detection disabled |
| REC11-SUITE-ON | `ctest --test-dir build/macos-rec07-on --output-on-failure -L recording` | 0; 12/12 passed | All available enabled recording tests, including REC10 and REC11 |
| REC11-SUITE-OFF | `cmake --build build/macos-rec07-off --target vaeg_sdl2 vaeg_sound_capture_test -j2 && ctest --test-dir build/macos-rec07-off --output-on-failure -L recording` | 0; 8/8 passed | Recording-OFF regression suite |
| REC11-ROMLESS-NO-AUDIO | `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=invalid build/macos-rec07-on/sdl2/vaeg --selftest --no-cfg --no-bkupmem` | 0; all selftests passed | Audio subsystem/device failure logged as a warning; emulator selftest remained usable |
| REC11-BUILD-ON | `cmake --build build/macos-rec07-on --target vaeg_sdl2 -j2` | 0 | Enabled frontend build |
| REC11-BUILD-OFF | `cmake --build build/macos-rec07-off --target vaeg_sdl2 -j2` | 0 | Disabled frontend build |
| REC11-REPO | `git diff --check`, repository case/UTF-8/EOL validators and `python3 tools/qa/milestone_ids.py --selftest --audit --discover` after the evidence update | 0 expected | Public documentation and ID topology validation |

The full `build/macos-rec08-werror` frontend target remains blocked by
pre-existing warnings treated as errors in unrelated `io/np2sysp.c` and
`sound/vermouth/midinst.c`; the changed sound units were separately compiled
with the same generated Werror commands. This is not attributed to REC11.

## Integrity results

The FIFO test appends a clipped S32 stereo block, reads only part of it, then
appends a muted block and verifies the remaining sample indices, channel order,
clipping and exact zero samples. It consumes wrapped storage and confirms the
next block begins at the exact cumulative sample index and retained epoch.
An over-capacity append changes neither pending count nor destination data.

Production generation uses one REC10 callback dispatch per generated block and
the sound state counter accounts every generated frame. Recording samples are
read through an explicit nonblocking pull API with cumulative produced and
consumed counts. No live interval count, movie PTS, A/V alignment, queue
high-water mark, or encoded PCM byte comparison applies until the recorder
coordinator consumes this API.

## Review and limitations

The callback path does not encode, write files, wait for recording capacity or
allocate recording storage. Recording storage and the 512-frame scratch block
are allocated only during the explicit start transition, not in the recording-
off path or audio callback. A full recording FIFO fails the recording session
instead of dropping samples or blocking under `SNDCSEC`; the future coordinator
must drain at safe guest-time boundaries and treat the failure as a failed
recording. Playback monitoring drops/underruns are separately observable.

The active handover is implemented in the sound core but is not yet called by
the public Start Recording UI because that coordinator does not exist in this
checkout. No FFV1/PCM/MKV file was generated from a real guest workload, and
no SDL/D3D11/OpenGL/Metal displayed path, Windows/Linux runtime, physical audio
device, VLC playback, human demonstration or release validation is claimed.

## Gate and next task

Machine gate: PASS.

Human gate: NOT_REQUIRED for this machine-only producer checkpoint; no human or
release validation is claimed.

Push/remote state under repository policy: local only; no push, merge or
publication performed.

Next dependency-ready REC task and host requirement: REC12 / M100r13 on the
same macOS host. REC12 can add the native guest-frame tap while retaining this
audio ownership boundary.

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
