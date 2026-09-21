# Recording v1 handoff

Current state: REC09 machine gate PASS. Completed tasks: REC00/M100r1,
explicitly accepted by the maintainer on 2026-09-19, REC01/M100r2,
REC02/M100r3, REC03/M100r4, REC04/M100r5, REC05/M100r6, REC06/M100r7 and
REC07/M100r8, REC08/M100r9 and REC09/M100r10. Assigned branch:
topic/m100r3-recording-clock.

## REC09 completion

* `scrnmng_capture_native_recording_frame` copies the completed canonical
  640-by-400 RGB565 guest raster after its left guard into bounded, owned RGB24
  storage with the existing channel expansion and 1:1 sample aspect ratio.
* The helper consumes the REC08 immutable snapshot and composes enabled
  `VIDEO_INFO` then `FRAMEBUFFER_INFO` at native scale 1, top-right anchor,
  native clipping and established alpha/text conventions. It never writes back
  to the guest surface and never includes CRT, scaling, host strips or GUI.
* Focused ON/OFF tests cover both OSD classes, source guards and mutation
  isolation, no-OSD byte identity, alpha, clipping and validation failures.
  Live recording is not connected until the guest video-clock task.

## REC09 evidence

Evaluated source commit:
1aa9f6f78dd968bdabfa068c24e32b4fa24403ca.

On macOS arm64 with AppleClang 21.0.0.21000101, CMake 3.31.12, Ninja 1.13.2
and MacPorts SDL2 2.32.10, the native composition fixture passed in ON and OFF
builds, with Werror and ASan/UBSan. The enabled recording suite passed 10/10
and the disabled suite passed 6/6. Both frontend builds, ROM-less selftest and
smoke runs passed. Case, UTF-8, EOL, whitespace and milestone-ID validators
passed. No Windows/Linux runtime, presenter readback, live movie, audio
integration, VLC, human or release validation is claimed.

REC09 is the native composition checkpoint only. The next dependency-ready
task is REC10/M100r11.

## REC08 completion

* The two actual guest-area diagnostic OSD classes are represented by one
  fixed-size `VAEG_RECORDING_OSD_SNAPSHOT` associated with the current guest
  `drawcount`: Video information and Framebuffer information.
* The snapshot copies formatted strings, enabled/visible flags, output-space
  rectangles, clipping rectangles, top-right anchors, scale and background/text
  alpha. Disabled or too-small layers remain represented without borrowed
  pointers. Host title text, ImGui UI, native decorations and screenshot-only
  analysis remain excluded according to the audit.
* Existing native and SDL overlay functions consume the snapshot while keeping
  their previous order, geometry, alpha and no-draw boundaries. The public
  getter returns a complete value copy for later recording producers.

## REC08 evidence

Evaluated source commit:
e4766a4173c4a9c69e483eb0e7bdb3887da02382.

On macOS arm64 with AppleClang 21.0.0.21000101, CMake 3.31.12, Ninja 1.13.2
and MacPorts SDL2 2.32.10, the focused snapshot test passed with Werror and
under ASan/UBSan (macOS leak detection disabled). The enabled recording suite
passed 9/9 and the recording-disabled suite passed 5/5; the existing frontend
builds passed in both configurations. ROM-less selftest and smoke passed, and
the existing display-capture selftest exercised the unchanged SDL overlay
rendering path. Repository ID discovery/audit also passed.

REC08 is an OSD semantic/extraction checkpoint only. It does not establish
Native recording-owned raster composition, Displayed final-client readback,
guest-time audio production, UI lifecycle, platform presenter behavior, VLC
playback or release validation. REC09/M100r10 is now complete at its machine
gate.

## REC07 completion

* `FfmpegMatroskaWriter` creates one FFV1 video stream and one PCM S16LE
  stereo stream in an explicit Matroska context. It reads the actual stream
  timebases after the header and converts complete encoded intervals once from
  their codec/sample epochs.
* A custom seekable AVIO uses an exclusive sibling partial file and a
  non-replacing final publication. Existing targets and partials survive
  failures; public-safe source/OSD/format/build tags are written into the
  container.
* The synthetic fixture decodes exact RGB and PCM bytes, checks seekability,
  metadata, packet ordering and boundary counts, and exercises lifecycle and
  I/O/publication failure injection. `tools/recording/verify_movie.py` provides
  an independent ffprobe/ffmpeg verification path.

## REC07 evidence

Evaluated source commit:
2a3c78583767624aea239c37cb8b9c1beb5a9f7c.

On macOS arm64 with AppleClang 21, CMake 3.31.12, Ninja 1.13.2, MacPorts SDL2
2.32.10 and FFmpeg 8.1.2, the enabled recording suite passed 8/8, the disabled
suite passed 4/4 with no generated FFmpeg references, and the combined fixture
passed under ASan/UBSan with macOS leak detection disabled. The fixture was
independently inspected with ffprobe and `verify_movie.py`; it contained exactly
one FFV1 stream, one PCM S16LE stream, three decoded 5x3 frames and 4800 exact
stereo samples. Unicode/space paths and non-replacing partial ownership were
tested. ROM-less self-test and smoke passed for both ON and OFF builds.

REC07 is synthetic backend evidence only. It does not establish live guest-time
production, OSD composition, presenter readback, UI lifecycle, VLC playback,
Windows/Linux runtime or release dependency compliance. Read-only filesystem,
multi-gigabyte sparse-file and platform-specific handle validation remain
explicit later fault/platform work. REC08/M100r9 is now complete at its machine
gate.

## REC06 completion

* `FfmpegPcmEncoder` is a private C++17 PCM S16LE stereo component at the
  caller's effective output rate. It accepts variable frame blocks with a
  cumulative first sample index and declared origin offset.
* The existing `satuation_s16` final-mix clamp is reused, then each sample is
  explicitly packed little-endian. Packet PTS, first index, duration and owned
  bytes preserve the sample ledger.
* The focused decoder test covers independent channel patterns, silence,
  impulses, clipping boundaries, irregular blocks, a final short block, an
  equivalent whole-block partition and allocation/open/send/receive/flush
  failures.

## REC06 evidence

Evaluated source commit:
2c990ffa618c05e1a7acaf69bd23bfb9c708628b.

On macOS arm64 with AppleClang 21, CMake 3.31.12, Ninja 1.13.2, MacPorts SDL2
2.32.10 and FFmpeg 8.1.2, the enabled build and all seven recording tests
passed. The disabled build and all four recording-OFF tests passed, and its
generated build files had no FFmpeg references. The PCM test passed under
AddressSanitizer and UndefinedBehaviorSanitizer with macOS leak detection
disabled. Existing ROM-less self-test and smoke passed with the expected
missing-ROM warning.

REC06 is synthetic encoder evidence only. It does not establish live sound
ownership or callback independence, MKV output, A/V alignment, platform
runtime or release dependency compliance. The next dependency-ready task is
REC07/M100r8.

## REC05 completion

* `FfmpegVideoEncoder` is a private C++17 RAII component using the optional
  FFmpeg targets. It emits packed BGR0 FFV1 level/version 3 with GOP 1 and
  slice CRC enabled; it does not invoke an external process.
* RGB24 and little-endian RGB565 input are copied into owned encoder frames
  with explicit stride handling and initialized padding. Packet bytes are
  copied into owned vectors and associated with the submitted sequence and PTS.
* The focused decoder test covers nonblack corners, varied seeded pixels,
  padding, odd/small dimensions, exact normalized RGB24 comparison and
  allocation/open/send/receive/flush failures.

## REC05 evidence

Evaluated source commit:
9c462a968a45f8559734710f7aeb1f1348628dae.

On macOS arm64 with AppleClang 21, CMake 3.31.12, Ninja 1.13.2, MacPorts SDL2
2.32.10 and FFmpeg 8.1.2, the enabled build and all six recording tests
passed. The disabled build and all four recording-OFF tests passed, and its
generated build files had no FFmpeg references. The FFV1 test passed under
AddressSanitizer and UndefinedBehaviorSanitizer with macOS leak detection
disabled. Existing ROM-less self-test and smoke passed with the expected
missing-ROM warning.

REC05 is synthetic backend evidence only. No MKV file, audio path, live guest
tap, OSD composition, presenter readback, VLC playback, Windows/Linux runtime
or release dependency audit is claimed. The next dependency-ready task is
REC06/M100r7.

## REC04 completion

* `VAEG_ENABLE_RECORDING` remains OFF by default. When enabled, CMake discovers
  target-scoped libavutil, libavcodec and libavformat imported targets; when
  disabled, the frontend does not include or link the FFmpeg capability source.
* The private C++ capability adapter reports stable structured failures for
  unsupported versions, missing FFV1/PCM encoders, missing Matroska muxing,
  unsupported BGR0/S16 formats, missing FFV1 slice CRC/version 3 and muxer
  incompatibility. It uses the current supported-config API with a private
  compatibility branch for older headers.
* The actual development libraries passed the FFV1, PCM S16LE, Matroska, BGR0,
  packed S16, slice CRC and FFV1 v3 checks. The test mutates each capability
  independently so failure reasons cannot mask one another.
* No C99 interface contains an FFmpeg type. No recorder, subprocess, swscale,
  resampler, live tap, file output or UI was added.

## REC04 evidence

Evaluated source commit:
2258aa0a73533691305d7b48d44f924b6303ec51.

On macOS arm64, AppleClang 21, CMake 3.31.12 and Ninja 1.13.2, the enabled
frontend and capability test built and all five recording tests passed. The
disabled frontend built without generated FFmpeg references and all four
recording-OFF tests passed. A deliberately empty target pkg-config environment
stopped enabled configuration with a specific missing `libavutil` diagnostic.
ROM-less self-test and smoke, milestone-ID, case, UTF-8 and EOL validators also
passed. The local development FFmpeg is a GPL/nonfree-enabled MacPorts build;
it is not release-package compliance evidence.

REC04 remains a capability boundary only. Actual encoding, MKV lifecycle,
guest-time audio/video production, presenter readback, platform runtime and
release dependency audits remain later tasks.

## REC02 completion

* `vaeg::recording::SampleGrid` computes exact sample-grid boundaries from
  guest ticks, integer rates and a bounded rational origin offset.
* Interval sample ranges are formed from absolute endpoint boundaries, so
  arbitrary producer partitioning telescopes to the same count as one whole
  interval. No nominal frame rate, host clock or floating-point accumulator is
  involved.
* Checked 128-bit intermediate arithmetic rejects zero denominators, invalid
  ranges and representable-result overflow. `WrapExtender` converts an
  audited modulo counter to monotonic guest ticks and rejects ambiguous
  backward movement.
* Explicit floor/ceil timestamp rescaling keeps codec-time and muxer-time
  units separate without linking FFmpeg. The helper is frontend-only and does
  not change the guest scheduler or audio producer.
* Focused tests cover a three-hour 48,000/1,001 synthetic schedule, changing
  17-tick partitions, origin offsets, wrap extension, invalid inputs and
  timestamp rescaling.

## REC02 evidence

Evaluated source commit:
8c40626f682d569668ba4ef574249f05498f8882.

The macOS arm64 AppleClang build and focused recording tests passed. The
existing ROM-less self-test and smoke run passed; smoke reported only the
expected missing-ROM warning. Repository case, UTF-8, EOL and milestone-ID
validators passed. The REC02 synthetic rate is a fixture only. The actual VA
clock mapping remains the audited TSP/pccore mapping in `audit.md`; no hardware
timing claim is inferred from 48,000/1,001.

REC02 does not add a recorder, FFmpeg backend, video/audio tap, queue, UI or
runtime capture. Audio look-ahead reconciliation and source integration remain
the subjects of REC10/REC11.

## REC03 completion

* `CaptureSlice` copies the complete video storage and interleaved stereo S16
  payload into private vectors before admission. Producer buffer mutation or
  destruction cannot affect the queued item.
* `BoundedSliceQueue` has checked slice-count and byte budgets. The configured
  active-interval and readback-staging reservations are charged before any
  queued bytes, and admission returns `WouldBlock` without waiting.
* Admission requires contiguous sequence IDs, guest endpoints, audio sample
  ranges, descriptor values and descriptor-generation identity. Invalid
  sequence, descriptor, interval and audio gaps have distinct errors.
* `SliceWorker` owns its sink and drains a close, propagates sink failure,
  never detaches, and uses condition-variable predicates for data, capacity,
  cancellation and failure wakeups. No FFmpeg, file I/O, SDL or audio callback
  is involved.
* `RecordingSession` exposes Preparing, Armed, Recording, Finalizing, Failed
  and Canceled behavior with idempotent Stop and explicit terminal
  acknowledgement. Queue admission remains nonblocking.

## REC03 evidence

Evaluated source commit:
e7f53b3e1f245dd9f546ba8cf16e46ccaf30469b.

The macOS arm64 AppleClang build, four focused recording tests, full frontend
build, ROM-less self-test and smoke run passed. The queue test also passed under
AddressSanitizer and UndefinedBehaviorSanitizer; macOS does not support
`ASAN_OPTIONS=detect_leaks=1`, so the focused rerun used `detect_leaks=0` and
is not a leak-detection claim. The expected ROM-less missing-ROM warning was
unchanged. Repository validators passed.

REC03 remains synthetic. It does not connect live guest video, audio production,
OSD, presenters, FFmpeg, or user-facing lifecycle controls. Those integrations
remain later milestones.

## REC01 completion

* The production frontend now includes a small recording contract facade under
  sdl2/recording/, with a C99 boundary and a private C++17 ownership layer.
* VAEG_ENABLE_RECORDING defaults OFF and is supplied as a target-scoped
  definition. Both OFF and ON configurations compile without a recording
  backend.
* The OFF facade reports disabled; the ON facade reports that the backend is
  unavailable. No FFmpeg/libav headers, types, libraries or subprocesses were
  added.
* Deterministic validation covers the two source values, dimensions, pixel
  formats, stride and checked storage-size arithmetic, PCM S16LE stereo
  descriptors, and stable error names.
* OwnedVideoFrame copies caller data into private storage. The C++ facade does
  not expose STL types through the C boundary.
* Focused C99 and C++17 tests are registered under the recording/contracts
  labels.
* No live video/audio tap, queue, encoder, worker, CLI command, UI command or
  screenshot/QA path was added.

## Evidence

Evaluated source commit:
f838d15c7c676bf70a6d19d41f7f8fe5da57c813.

The macOS OFF test build and the ON compile check passed. The focused contract
tests, existing ROM-less self-test, startup viewport test and milestone ID
self-test passed. The OFF and ON CMake build files contain no FFmpeg/libav
references. Existing unrelated untracked work was preserved and excluded.

REC01 is host-independent. Windows D3D11, Linux OpenGL and macOS Metal
recording runtime evidence remains NOT_RUN because those are later Displayed
milestones, not REC01 acceptance evidence.

## Gate and next task

Machine gate: PASS. The required REC01 checks were run on macOS arm64 with
AppleClang 21, CMake 3.31.12, MacPorts SDL2 2.32.10 and Ninja.

Human gate: NOT_REQUIRED for REC01. REC00 human acceptance is recorded above.

The next dependency-ready task is REC04/M100r5. Under the current
recording-v1 authorization, continue automatically after the REC03 report is
recorded; later tasks must preserve the two-source OSD-inclusive contract and
must add no FFmpeg dependency until their own milestone requires it.
