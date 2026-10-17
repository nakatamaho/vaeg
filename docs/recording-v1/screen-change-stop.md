<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->
# Screen-change recording stop and macOS Release handoff

## Correction and contract

Evaluated implementation:
[105e5cac38962c5e408891a98f8620f61edd733b](https://github.com/nakatamaho/vaeg/commit/105e5cac38962c5e408891a98f8620f61edd733b).

A neutral private integration case, `rotation-15k-a`, reproduces the reported
exit: boot at15kHz, start Native recording, then the guest changes video mode.
The same case without recording reaches1200 guest frames. With recording,
both right rotation and no rotation exit1 after `descriptor_changed`, followed
by `native_tap_stopped`. This demonstrated exit is not a segmentation fault:
the frontend treated a recording failure as a fatal emulator failure.

The maintainer approves automatic recording stop on screen-setting changes.
Interactive recording now saves only already accepted complete intervals,
stops before applying screen settings, and continues the guest. It never
appends a changed descriptor, silently resizes, auto-segments, or restarts.
Start a new recording after the change. If there is no complete interval,
there is no movie to publish; write/finalization failures remain errors.

Covered frontend settings: rotation, scale/aspect/scaling/effect, explicit
display-mode/monitor requests, renderer/filter/preset changes, live CRT controls,
monitor frequency/digital RGB/15kHz fill, V1/V2 presentation, rendered OSD/layer
visibility, menu height and front-panel changes. Idle notifications are no-ops.
Title-only FPS/FDD toggles do not alter Native video. Guest-driven coded-size
changes use the same safe stop after the typed descriptor rejection, before
its generic cancellation path. CLI/environment batch capture deliberately keeps
its existing nonzero failure and no-published-movie contract; a frame limit
still terminates a batch, while a GUI frame limit stops recording only.

The runtime policy is a small C99 frontend module. Existing inherited audio
implementation/tests and Metal bridge remain byte-identical and uncommitted.

## Verification

- Original SDL ASan/UBSan synthetic presentation/capture probe:112 transitions,
  including320x224 and320x448, right/none, no address error. This alone did not
  reproduce the actual guest exit.
- Real private case: no-recording control completes1200 frames; old batch error
  path exits1 with/without rotation. Raw commands/media/logs/captures remain
  private, outside Git; no private asset paths, names or hashes are published.
- Policy-injection probes first verify save plus1200-frame continuation. Those
  are explicitly not GUI-entry proof.
- Final GUI-start automation invokes the **real GUI start routine** on a drawn
  frame with normal interactive runtime policy, not `--record` or a batch-policy
  override. Using final minimal-LGPL Release application objects except that
  start-invocation wrapper, it starts, auto-stops/saves at the mode change,
  reaches frame1200 and exits0. The saved pre-change movie independently decodes
  video/audio with FFV1 CRC checking. This is headless software presentation,
  not a physical mouse click, hardware Metal presentation or native Windows run.
- Recording ON tests24/24; OFF15/15; ROM-less GUI/FPS suite1/1. New runtime policy
  and real encoder/queue/mux integration tests pass2/2 under ASan/UBSan with
  halt-on-error. Re-linking against the committed native-session implementation
  without the inherited audio patch also passes the new integration contract.
- Final minimal-LGPL recording suite24/24; final Release executable itself passes
  `SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy vaeg --no-cfg --selftest`.
- Windows static Release build and system-DLL import audit pass. Policy worker
  compiles with `-Wall -Wextra -Werror` and passes Wine. Full Windows integration
  compiles; its runtime is not credited because the previously demonstrated
  Wine24MiB allocator failure still blocks full-size validation.
- Full guest sanitizer checks are **not clean UBSan**: existing signed shift/
  overflow diagnostics in PSG, PRNG and the shared vendor instruction backend
  stop the first halt-on-error attempt. A separate report-only UBSan run keeps
  ASan halt-on-error enabled to reproduce the recording exit and continuation.
  These unrelated diagnostics are unresolved, not explained away or fixed here.

## Performance scope

Final comparison uses the same pinned minimal LGPLFFmpeg8.1.2 for both Debug
and Release application objects,640x400,180 intervals,3 trials per pattern.
It includes RGB565 conversion, ownership, native tap/session, bounded queue,
worker, FFV1, PCM, mux/finalize, synthetic source generation and hashing. It
excludes guest instruction execution, real audio device, renderer and visible
OSD/rotation. Median unpaced processing throughput, **not guest/display FPS**:

| Synthetic input | Debug | Release |
| --- | ---: | ---: |
| Static color bars |251.92fps|573.01fps|
| Moving pattern |184.00fps|288.18fps|
| Changing RGB565 noise |74.34fps|87.80fps|

All18 final movies independently decode every source RGB byte by stream digest,
every alpha255 byte and every PCM sample exactly:3240 video frames and2592000
stereo sample frames. Earlier MacPorts-only checkpoints are retained separately,
not attributed to the final dependency set. The measurements do not establish
why the maintainer's previous live recording was unusably slow. Debug alone is
not a demonstrated explanation, especially because ordinary emulation was fine.

## Local test artifacts

Both are working-tree test builds, not clean-commit/release approvals.
Base105e5cac plus inherited snapshots:

- macOS arm64 Release: `build/macos-recording-lgpl-release/sdl2/vaeg`,
  8946264 bytes, SHA256
  `29fba0de417e5bd02519e96f3a51bd5e5e28e5b884867da066b2ddaf0c1c131a`.
  Includes all three inherited files; full patch SHA256
  `9e123689c9bac9053acf2b00a0aee9729e0589578e4b49d5aac8d5d407e7c7f0`.
- Windows static Release: `output/vaeg.exe` / `output/vaeg-audio-local-test.exe`
  in the existing task-local MinGW handoff,58846417 bytes, SHA256
  `bc3aedd236e753052688be02b4b85dc63c2acef8841618440fa613822ba41c8d`.
  Audio patch only; SHA256
  `0925091601467304336586c3bdf66b3d7cc08328afd5b9797edc190c7c1a2386`.
  Metal is excluded. Previous9b0c44d7 executable/report are archived.

macOS uses Release `-O3`, arm64, recording ON, tests/CPU trace/performance
instrumentation OFF, optional native presenter compiled ON. It is a local
executable with shared dependencies, not a standalone `.app`. SDL2/LibArchive
come from the current MacPorts installation and declare macOS26 minimums;
setting the executable deployment target13 does not prove macOS13 compatibility.
Optional librashader runtime/shader assets are not bundled by this build.

The installed MacPorts FFmpeg reports `nonfree and unredistributable`; artifacts
linked to it are excluded from the handoff. A dependency audit also catches the
preset prepending its own pkg-config path, and the corrected resolver pins the
new prefix. The final executable and all three FFmpeg dylibs resolve only to
our task-local minimal LGPL build. Source8.1.2 archive SHA256:
`464beb5e7bf0c311e68b45ae2f04e9cc2af88851abb4082231742a74d97b524c`.
GPL/nonfree/version3/autodetect/network/programs/filter/device/scale/resample
are disabled; FFV1/PCM S16LE encode/decode, Matroska mux/demux and file protocol
only, shared arm64. Each FFmpeg library reports LGPL2.1-or-later. Source archive,
configuration, logs, libraries and licenses stay in the task-local build cache.
No FFmpeg hardware encoder or software fallback is introduced; OS H.264 remains
separate. This audit is not complete redistribution/release legal approval.

Evidence identities, exact public worker commands and output digests are retained
in task-local `capture-lgpl-*.json` and the adjacent artifact reports. Private
commands and raw captures remain only in the private task directory. No hosted
CI, media/demo gate, release approval, real Metal/audio-device performance gate,
long A/V run or OS-assisted feature completion is claimed.
