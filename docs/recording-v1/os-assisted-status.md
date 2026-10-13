<!-- Copyright (c) 2026 Nakata Maho. SPDX-License-Identifier: BSD-2-Clause -->
# OS-assisted recording — checkpoint evidence and precise handoff

## Latest main refresh (explicit maintainer request)

The maintainer subsequently requested another main merge. Starting HEAD was
`8a3c18f780573433a6c7cb028edaeeb1c1b08977`; `git fetch origin refs/heads/main`
succeeded and pinned `ecaacb4c604dc00ff3fbf72ddded57f549a28938` for this refresh.
Normal merge commit: `9c2890906683f4fed840b110ad5d84e8cd2f003c`, on the same
`topic/m100os-recording-resume` branch, with no conflicts. Its second parent is
the fetched main tip; the main-ancestor check passed. Local main was not moved,
and nothing was pushed. All three inherited dirty files remained byte-identical
before/after merging and validation; none was staged or committed.

Post-merge ON/OFF builds exited 0. Recording-label CTest with `--no-tests=error`
passed 20/20 ON and 12/12 OFF. Encoding, EOL, case and whitespace checks passed.
This validation includes the preserved dirty files, not a clean-release claim.
New runtime/performance/platform acceptance is not implied. Resume from this
merged tree. The original integration and checkpoint identities below remain
historical evidence, rather than a reason to rewind or repeat their work.

## Overall state

**IN_PROGRESS, not IMPLEMENTATION_COMPLETE and not RELEASE_VALIDATED.** This is
an implementation checkpoint, not a new recording proposal or human approval.
The existing GUI/CLI still starts the Native guest-time archive; Real-time and
window borders are not exposed as functional choices yet. No video-only mode
was enabled merely to make a demonstration. No remote publication.

Branch: `topic/m100os-recording-resume`.
Starting HEAD: `eb5517e94dc1fcfc012bce4d84f6ed18ce49c5f6`.
Pinned origin/main: `6c9fb34b5be21051740be3b7d120e8673097d265`.
Implementation base (normal merge): `d0de86910baaf9969f231954491e75676e3cf9a2`.
Latest evaluated source HEAD: `e4754b4995fae0527569db1c1f7d5131887d3aab`.

Evaluation includes three inherited unstaged files, still preserved and excluded
from every task-owned commit: `sdl2/librashader/backends/metal_bridge.mm`,
`sdl2/recording/recording_native_session.cpp`, and
`tests/frontend/recording/test_recording_native_session.cpp`. This is not a
clean-checkout evaluation of those inherited changes. See
[resume audit](resume-audit.md) for the authorized GUI/CPMVA resolution.

## Implemented checkpoints (not completed whole OSR units)

- **M100s1 / OSR02 partial**: `HostVideoSchedule` uses explicit absolute
  monotonic nanoseconds, 60Hz slot PTS and the same epoch for sample-grid mapping.
  No civil time or guest count. Delayed polls select the newest due slot and
  count skipped slots. `LatestImageMailbox` preallocates one owned pending image,
  producer try-locks without waiting, replaces only stale unencoded images and
  rejects stale slots even after consumption. Tests include a five-second pause,
  ten-hour independent expected clocks and 100,000 overload replacements.
  This does NOT yet implement CFR repetition/muxing or playback-clock observation.
- **M100s2 / OSR04 + OSR06 partial**: worker-only `H264Encoder` directly opens
  `h264_videotoolbox` with `allow_sw=0`, `require_sw=0`, `realtime=1`, NV12,
  1/60 codec timebase, GOP120 and no B-frames. There is no CPU fallback and no
  production subprocess. Its in-flight ledger is capped at eight frames; owned
  packet PTS/DTS are retained separately. Missing providers, invalid/odd sizes,
  unavailable hardware, open/encode/drain errors are explicit. Initial coded
  bounds are even extents up to 4096x4096; odd extents reject, not crop/stretch.
- The **existing** `FfmpegMatroskaWriter` now accepts a closed H.264 configuration
  as well as its unchanged default FFV1 contract. It reuses seekable exclusive
  partial-file publication and PCM; independently rescales PTS and DTS, checks
  post-header timebases, and stores provider/device/options. Archive RGB/PCM
  roundtrip regression tests still pass. H.264 SDR input is defined BT.709 matrix,
  limited range and BT.709 primaries/transfer; RGB conversion remains pending.
- **M100s5 / OSR10 partial**: `MacWindowCapture` implements ScreenCaptureKit's
  desktop-independent **single actual main-window** filter. Cocoa main-window ID
  and process ID must match. There is no display filter or substitute desktop
  path. Preflight consent is required; no automatic permission request. SDK and
  runtime geometry/shadow API guards make this adapter available on macOS14+
  without increasing the application's global minimum. Earlier-runtime support
  remains unfinished. Config: cursor off, OS audio off, queueDepth3, no scaling,
  no single-window shadow, neutral black background and explicitly verified SDR.
  Complete buffers are copied to preallocated owned BGRA, never retained after
  callback. Source geometry/status/stride/format and mach display timestamps are
  checked. Callback admission try-locks; overwrite/busy counts are distinct.
  Revocation/source failure/resize errors persist through Stop. Runtime decoration
  coverage is not inferred from compilation or successful stream startup.

## Actual hardware / synthetic MKV evidence

Reference environment: macOS26.6.2 arm64, AppleClang21.0.0.21000334,
SDL2 2.32.10, MacPorts FFmpeg8.1.2; libavcodec62.28.102,
libavformat62.12.102, libavutil60.26.102. Builds below are **Debug**, not Release
performance measurements. Installed FFmpeg is GPL/nonfree-enabled; it is a local
development dependency, NOT redistribution approval. No x264 provider/dependency
was added by this task, and OS encoding is not a legal exemption.

The actual VideoToolbox hardware-required session encoded and decoded 180 public
640x400 NV12 frames. FFmpeg8.1.2 `vtenc_configure_encoder` maps allow_sw=0 and
require_sw=0 to RequireHardwareAcceleratedVideoEncoder=true. This is the hardware
use evidence; the selected physical device/EncoderID is NOT exposed by this
libavcodec adapter and is honestly reported as OS-selected, not invented.
Reference source inspected (not copied/vendored): FFmpeg n8.1.2
`libavcodec/videotoolboxenc.c`.

Fixture: grayscale orientation bands and scrolling bars, NOT a complete color,
single-pixel text, CRT or actual guest/OSD quality review. Requested bitrate:
8,000,000 bit/s; configured context bitrate: 8,000,000 (not a guarantee of actual
output bitrate). Encoded payload: 16,208 bytes, two keyframes across three
seconds, measured video payload rate approximately 43,221 bit/s for this very
compressible fixture. All packets were decoded, PTS/DTS order and decoded PTS,
orientation, limited range/BT.709 metadata, count and inter-frame GOP checked.

`test_recording_h264_encoder --movie` writes the fixture through the production
H.264 adapter, existing PCM encoder and existing Matroska writer. No shell-made
movie substitutes for it. Source tag `native` is a synthetic backend descriptor,
NOT evidence of a live canonical VAEG tap. Audio is a public 48kHz stereo marker
fixture, NOT a claim of captured host playback. The resulting public file has
180 frames, exactly 144,000 PCM sample frames, duration3.000s and size595,731 bytes.
Post-header video/audio timebases are each 1/1000. Independent decoder checks
verify all frame timestamps against index/60, exact PCM markers, seek at2s,
orientation and EOF. This is NOT a 60-host-second or ten-minute A/V benchmark.

Public fixture, untracked: `build/osr-validation/public-h264-pcm-02.mkv`.
SHA256: `9a25958bd1c99de6826f36731c0c8811a316e0879683f919962e9de64868c38f`.
Do not overwrite or commit this generated file. Fixture01 is retained as earlier
intermediate evidence, not the final metadata contract.

Worker/test binary SHA256 at the evaluated sources:

- `build/macos-rec13-on/vaeg_recording_h264_encoder_test`:
  `be0b00477446239d14e043ddb637072fe2fdcd3f194f8eeee66de3b36b5f7b1f`.
- `build/macos-rec13-on/vaeg_recording_macos_window_capture_test`:
  `a8845fb0dd862a49ba6dd6f0b76590fb2370971b02d69927dbd999548273013d`.

## Executed commands / results

- `cmake --build build/macos-rec13-on -j 6`: exit0.
- `ctest --test-dir build/macos-rec13-on -L recording --output-on-failure`:
  exit0, **20/20 PASS**, including old FFV1 RGB/PCM and MKV lifecycle tests.
- `cmake --build build/macos-rec13-off -j 6`: exit0.
- `ctest --test-dir build/macos-rec13-off -L recording --output-on-failure`:
  exit0, **12/12 PASS**.
- OFF CMake generated build/flags/link files searched for libavcodec,
  ScreenCaptureKit and the new H.264/window adapter source names: no matches.
- `build/macos-rec13-on/vaeg_recording_h264_encoder_test --movie
  build/osr-validation/public-h264-pcm-02.mkv`: exit0, production fixture above.
- `python3 tools/recording/verify_h264_fixture.py
  build/osr-validation/public-h264-pcm-02.mkv`: exit0,
  `OSR_PUBLIC_MKV_AUDIO_TIMESTAMP_SEEK_PASS` and matching public SHA256.
- Repeating the movie command with the same destination: exit1, target_exists;
  SHA256 stayed identical. This exercises exclusive no-overwrite behavior.
- `build/macos-rec13-on/vaeg_recording_macos_window_capture_test --contracts`:
  exit0; no host UI/capture session or permission request.
- `build/macos-rec13-on/vaeg_recording_macos_window_capture_test`: exit77,
  `OSR_MAC_CONSENT_REQUIRED`. Actual capture is **NOT_RUN**, not PASS. No
  permission was requested or altered.
- Focused new targets built with Werror + ASan/UBSan in
  `build/osr-focused-sanitize`, then
  `ASAN_OPTIONS=detect_leaks=0 ctest --test-dir build/osr-focused-sanitize
  -R 'realtime_clock|h264|macos_capture_contracts' --no-tests=error
  --output-on-failure`: exit0, **4/4 PASS**, including real hardware codec probe.
  No macOS leak-detection claim. Initial configure omitted the required existing
  integration-trace option and failed; the empty test listing was NOT counted
  as success. Final configure explicitly sets VAEG_Z80_COMPAT_INTEGRATION_TRACE=ON.
- Focused capture source compiled with `-std=c++17 -fobjc-arc -Wall -Wextra
  -Werror`: exit0. Full Debug frontend still reports unrelated pre-existing
  kbdmap warnings; no unrelated fix was made.
- `tools/repo/check_encoding.py`, `check_eol.py`, `check_case.py` and
  `git diff --check`: exit0; encoding reports `0 finding(s)`.

Sanitizer configure (same checkout, no installations/global setting changes):

```sh
cmake -S . -B build/osr-focused-sanitize -DCMAKE_BUILD_TYPE=Debug \
  -DVAEG_ENABLE_RECORDING=ON -DVAEG_ENABLE_TESTS=ON \
  -DVAEG_Z80_COMPAT_INTEGRATION_TRACE=ON -DVAEG_ENABLE_LIBRASHADER=OFF \
  -DVAEG_ENABLE_ARCHIVE_DROP=OFF -DVAEG_WERROR=ON \
  -DCMAKE_OSX_SYSROOT="$(xcrun --show-sdk-path)" \
  -DCMAKE_C_FLAGS='-fsanitize=address,undefined' \
  -DCMAKE_CXX_FLAGS='-fsanitize=address,undefined' \
  -DCMAKE_OBJCXX_FLAGS='-fsanitize=address,undefined'
cmake --build build/osr-focused-sanitize --target \
  vaeg_recording_realtime_clock_test vaeg_recording_h264_encoder_test \
  vaeg_recording_macos_window_capture_test -j 6
```

## Platform/source matrix

| OS / backend | Encoder | Source/frame coverage | Implementation | Runtime evidence |
|---|---|---|---|---|
| macOS / CPU owned fixture | VideoToolbox hardware-required | synthetic NV12+PCM MKV | implemented codec/mux | PASS, fixture only |
| macOS / internal Native | VideoToolbox | Native live Real-time | coordinator/audio/conversion missing | NOT_RUN |
| macOS / SDL + Metal client | VideoToolbox | full client live Real-time | live connection missing | NOT_RUN |
| macOS / ScreenCaptureKit | VideoToolbox separately probed | actual main-window adapter, decoration coverage unvalidated | IMPLEMENTED_UNVERIFIED | BLOCKED by consent; capture NOT_RUN |
| Windows / WGC | MF hardware/vendor | Native/client/decorated Real-time | missing | NOT_RUN |
| Linux / Wayland portal + PipeWire; compositor not evaluated | VA-API/NVENC | Native/client/decorated Real-time | missing | NOT_RUN |
| Linux / X11; WM not evaluated | VA-API/NVENC | Native/client/reparented frame Real-time | missing | NOT_RUN |
| Existing Native archive | FFV1/PCM | existing guest-time Native | retained | synthetic regressions PASS; live manual gates pending |

No Release recording-off/FFV1/Native/client/decorated workload comparison, 60s
throughput test, ten-minute drift/resource test, real playback markers, VLC,
private guest integration, Win/Linux runtime or redistributed-dependency audit
is claimed. No before/after performance or numeric-budget PASS. Freeze actual
Release baseline conditions and numeric budgets before optimization; optimization
has not started. Missing platform implementation is not merely a missing test.

## Resume exactly here; do not restart or resynchronize

1. Read current instructions, this status, [plan](os-assisted-plan.md) and the
   retained three dirty source/test files. Keep their edits out of task commits.
   Do not reinstall seeds, replay REC milestones, push or refetch a moving main.
2. **M100s3 playback clock is the next technical implementation prerequisite.**
   Actual `sdl2/soundmng.c:sound_play_cb` supplies post-pack/mix S16 stereo, no
   frontend resampler/time-stretcher there; mute and guest pause pause the SDL
   device and therefore stop callbacks. Playback lead cannot be inferred from
   obtained samples or callback count. SDL2 release-2.32.10
   `src/audio/coreaudio/SDL_coreaudio.m:outputCallback` was inspected: it uses
   AudioQueue, can split application callbacks across queued buffers and an
   SDL_AudioStream, prequeues silence, then AudioQueueEnqueueBuffer. Its opaque
   AudioQueue and scheduling position are not exposed to sound_play_cb. Do NOT
   invent an AudioUnit timestamp seam or label callback arrival a device clock.
   Implement a narrowly owned/tested AudioQueue scheduling adapter (or another
   proven actual host-playback mapping) while preserving recording-off playback,
   one synthesis producer, bounded nonblocking copies, post-gain/mute signal,
   explicit silence causes and device/discontinuity graceful-stop behavior.
   Inspect actual installed AudioQueue headers first; do not hand-edit SDL or
   expose OS types across core C interfaces.
3. Connect the bounded worker/coordinator, PCM mapping, host-held video and
   actual Native/client sources (M100s4). Client REC15 token suppression is
   guest-boundary-specific: real-time must sample latest complete UI-only/pause
   redraws without changing the old archive seam or re-evaluating CRT history.
   Define/test RGB conversion and source/coded geometry; no crop or HDR coercion.
   Only then expose Real-time in the existing dialog with actual provider status;
   leave old `--record` scripts on guest time unless an explicit new mode is used.
4. ScreenCaptureKit runtime probe can be rerun by the maintainer in a lawful
   Screen Recording-consented environment; it never requests permission itself.
   Validate real title bar/borders, shadow/padding, Retina/resize/fullscreen,
   occlusion/minimize, revocation/close and timestamps before connecting or
   advertising the decorated checkbox. Current BGRA copy cost is unmeasured.
5. Continue independent Windows and Linux implementation against actual SDKs
   and toolchains; no stubs count as those platforms. Complete final lifecycle,
   stress, performance, packaging and manual ledger. Missing consent on this
   machine does not block unrelated implementation or imply human approval.
