# REC00 audit record

Status: WAITING_HUMAN. REC00 is complete as an audit/documentation change and
stops at the declared human gate. No recording production code was changed.

## Checkout and workflow

The audit was performed in the assigned checkout on branch
topic/rec00-recording-audit, from evaluated source commit
1544a1b8e19cc324c203a94c4e055361abb57d8b. Existing unrelated untracked work
was preserved and is not included in the REC00 commit. The applicable
repository instructions are the root AGENTS.md, docs/agents/ROADMAP.md, and
docs/agents/CONVENTIONS.md.

The current roadmap and discovered tasks contain no M100 or M101 assignment.
The repository milestone validator accepts the legal lettered form M100r1
through M100r28. REC00 reserves that range for this feature family; the
complete mapping is recorded in id-map.md. The roadmap was not rewritten to
register a future milestone.

The recording-v1 documentation package supplied under docs/recording-v1/ was
inspected as task input. REC00 adds only the audit/report/wrapper and updates
the package's audit state. No REC01 wrapper or implementation file was
created.

### Baseline environment and commands

The following baseline was run before the documentation-only changes:

    cmake --preset macos-macports
    cmake --build --preset macos-macports -j2
    SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/macos-macports/sdl2/vaeg --selftest --no-cfg --no-bkupmem
    SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/macos-macports/sdl2/vaeg --smoke --no-cfg --no-bkupmem
    python3 tools/qa/milestone_ids.py --selftest --audit --discover
    python3 tools/repo/check_case.py
    python3 tools/repo/check_encoding.py --expect utf8
    python3 tools/repo/check_eol.py --enforce

All commands exited successfully. The self-test reported
selftest: all tests passed; the smoke run completed with its expected
ROM-less missing-ROM warning. The milestone validator reported 49 strict
self-tests, tasks=141 reports=160 roadmap_rows=100, and no audit error.
Case, encoding and EOL validators reported zero findings/violations.

The available host was macOS arm64 with AppleClang 21, CMake 3.31.12,
MacPorts SDL2 2.32.10 and the macos-macports build preset. FFmpeg command
line tools were present for inspection, but no libav recording integration is
currently discovered in CMake. VLC was not available. Windows and Linux
runtime presenters were not available in this audit session. These facts are
environment evidence only; they are not recording acceptance.

## Video source map

### Canonical guest raster

The canonical guest surface is owned by sdl2/scrnmng.c:

* SCRNMNG_CANVAS_WIDTH and SCRNMNG_CANVAS_HEIGHT are 640 by 400.
* scrnmng_create allocates the shadow surface with a one-pixel left guard and
  a 16-bit pitch based on (width + guard) * 2.
* scrnmng_surflock exposes the shadow pointer, pitch, 16-bit format and active
  dimensions. The active pixel base used by upload/readback paths is the
  shadow pointer after the guard pixel.
* scrnmng_makepal16 and sdrawva.c establish the existing RGB565 palette
  expansion. The guest raster is composed by scrndrawva.c and driven by
  machine/pccore.c:drawscreenva.

The guest graphics layers (text, sprites, G0 and G1) are guest content and
belong in the canonical raster. They are not separate recorder OSD layers.
Native recording must copy this raster before CRT processing, window scaling,
letterboxing or host GUI composition. It must not reuse the existing
guest-only screenshot output in a way that changes screenshot/QA behavior.

### Frame and presenter boundaries

machine/pccore.c:drawscreenva is the completed guest drawing boundary. It
updates TSP timing, composes/rasterizes the guest image and increments
drawcount. The `screendispva` event path invokes it with a guest-boundary flag;
`pccore_redraw` invokes the same composition with that flag clear. The REC12
tap is therefore emitted only from the guest display event, after the complete
guest draw, and not from an explicit host redraw. sdl2/np2.c:run_guest_frame
calls the guest execution and then enters GUI/presenter work.
scrnmng_framedisp_tick is host pacing, not a guest-time source.

The SDL path is assembled in sdl2/scrnmng.c: upload to the SDL texture,
viewport/effect processing, overlays and SDL presentation. The native
presenter is owned by sdl2/librashader/native_presenter.h and its backend
bridges. D3D11 uses a staging readback with row-pitch handling in
d3d11_bridge.cpp; Metal uses a completed blit/readback path in
metal_bridge.mm; OpenGL owns context/readback state in gl_bridge.cpp. These
are Displayed-video integration points, not proof that a readback is currently
available for recording.

The existing display screenshot path crops the menu strip and has separate
readback behavior. It remains unchanged. The existing G75 rendered capture is
QA-only and is not a recording implementation.

### Required source classification

| Source | Recording image boundary | Required contents | Explicit exclusions |
|---|---|---|---|
| Native video | Complete 640x400 canonical guest raster, copied before CRT/window scaling | Guest content plus every enabled guest-area emulator OSD, composed on a recording-owned copy | CRT, scaling, letterbox, title/OS decorations, host menu/status/dialog UI |
| Displayed video | Final completed main-window client composition before Present/swap | Active CRT/effect, viewport/letterbox, guest-area OSD, visible in-client GUI, menu/dialog/status regions | OS window decorations, desktop and external dialogs |

The two choices are exactly the user-facing Native video and Displayed video.
There is no clean third source and no recording OSD checkbox.

## OSD and status map

Classification is by the actual draw path, not by the word “status”.

| Actual layer | Local owner / control | Guest-area or host UI | Native | Displayed |
|---|---|---|---|---|
| Video information overlay | sdl2/scrnmng.c:842-907, VAEG_DISPINFO_VIDEO | Guest-area emulator OSD | Include when enabled on recording-owned copy | Include when enabled |
| Framebuffer information overlay | sdl2/scrnmng.c:909-987, VAEG_DISPINFO_FRAMEBUFFER | Guest-area emulator OSD | Include when enabled on recording-owned copy | Include when enabled |
| Native overlay primitive entry | sdl2/scrnmng.c:989-995 and native presenter overlay plumbing | Guest-area overlay path | Include semantic enabled guest-area content, not live GUI draw data | Include in final client composition |
| FDD/FPS/CPU/SGP/frame title text | sdl2/scrnmng.c:543-582 | Host title/OS decoration | Exclude | Exclude with OS decorations |
| ImGui main menu bar | sdl2/gui/gui.cpp:4120-4134 | In-client GUI | Exclude | Include when visible |
| Fullscreen hint | sdl2/gui/gui.cpp:4136-4148 | In-client GUI | Exclude | Include when visible |
| ImGui dialogs | sdl2/gui/gui.cpp:4150-4164 | In-client GUI | Exclude | Include when visible |
| GUI/status state | sdl2/gui/gui.cpp:199-303, screen/info menus | In-client GUI/status | Exclude | Include when visible |
| Screenshot-only graphics analysis | scrnmng_draw_graphics_analysis and guest screenshot path | Screenshot annotation, not live OSD | Exclude | Exclude unless actually rendered in final client |
| Startup splash | sdl2/scrnmng.c startup path | Pre-guest presentation | Exclude | Exclude from a guest recording interval |

The Native implementation must snapshot enabled guest-area OSD semantically and
render it on an owned copy. It must not implement a live ImGui software
renderer or insert host menu/status strips. Displayed acquisition belongs after
the final in-client overlay/GUI pass and before Present/swap.

### REC08 snapshot extraction result

The current frontend has two true guest-area diagnostic OSD classes: Video
information and Framebuffer information. `sdl2/scrnmng.c` now builds one
`VAEG_RECORDING_OSD_SNAPSHOT` for the current `drawcount` before either the
native or SDL overlay path draws. The snapshot contains fixed owned copies of
all formatted lines, enabled/visible state, output-space rectangle, clipping
rectangle, top-right anchor, integer scale and background/text alpha. The
public getter copies the complete value object for later Native/Displayed
recording producers; no pointer into GUI draw data or a transient formatter
buffer escapes.

Disabled classes remain represented with `enabled=0` and no borrowed text. A
class that is enabled but cannot fit the current output retains its semantic
content with `visible=0`, matching the existing renderer's no-draw boundary.
The title-bar FDD/FPS/CPU/SGP/frame text, ImGui menus/dialogs/fullscreen hint,
native OS decorations and screenshot-only graphics analysis remain outside this
guest-area snapshot. Existing drawing consumes the copied snapshot without
changing its order, geometry or alpha convention.

### REC09 native composition result

`scrnmng_capture_native_recording_frame` is the frontend boundary for the
Native source. It reads `scrnmng.shadow` after the one-pixel RGB565 left guard,
validates the active pitch and source extent, and copies only the canonical
guest width/height into a bounded owned RGB24 frame. The conversion reuses the
existing integer RGB565 expansion rule and declares sample aspect ratio 1:1.
The source buffer is never modified.

The helper then consumes the REC08 value snapshot and renders the enabled
Video information layer followed by the enabled Framebuffer information layer
at native scale 1 and the top-right anchor. It uses the existing 8x8 font,
black background/text alpha conventions and clips every write to the owned
frame. The live output-space `visible` bit is not used to omit an enabled
native layer: it only reflects whether the current window could display that
layer. CRT, scaling, letterbox, host title/menu strips, ImGui and native OS
decoration are not part of this frame.

The deterministic REC09 fixture passed in both recording ON and OFF builds,
including source guard/padding isolation, no-OSD byte identity, alpha, order,
clipping and malformed-input rejection. This helper is not yet called by live
recording; video-clock and audio ownership work remain later milestones.

### REC12 native tap result

`sdl2/recording/recording_native_tap.c` and its C-compatible header define the
bounded handoff contract used by the next coordinator. A live tap callback
receives the guest frame sequence, a contiguous `[begin_ticks, end_ticks)`
interval, the native descriptor generation, an owned RGB24 frame valid for the
duration of the callback, and the matching REC08 OSD snapshot. The callback
must retain data by copying or moving it before return; no tap-owned pointer
escapes the callback lifetime. Sequence, guest-clock monotonicity, descriptor,
frame geometry/pitch, OSD token identity and callback acceptance are checked;
failure deactivates the tap and remains observable through its error API.

`machine/pccore.c:screendispva` calls `drawscreenva(TRUE)`. After the full
`scrndrawva_draw` operation and `drawcount` update, it captures the native
frame with `scrnmng_capture_native_recording_frame_with_snapshot` and submits
the frame/snapshot pair to the tap. `pccore_redraw` calls `drawscreenva(FALSE)`
and does not submit a recording frame. If normal host drawing is skipped while
the tap is active, the composition path is still forced so a guest display
boundary cannot be omitted by frame-skip policy.

The REC12 tap and capture path passed the focused and full recording suites in
both recording ON and OFF builds, changed-unit Werror compilation, the focused
sanitizer test and ROM-less self-tests at evaluated source commit
`415ca5f114b27990daa04ce157f4f27616ac98b2`. No live MKV, coordinator, UI,
Displayed/GPU presenter, cross-platform runtime, human or release validation
was performed by REC12.

## Video clocks and ownership

The machine master clock is selected by machine/pccore.c from the model base
clocks in machine/pccore.h; for the standard VA2 configuration the base clock
is 3,993,600 Hz with multiplier 2, yielding a 7,987,200 Hz pccore.realclock.
The CPU event scheduler in machine/nevent.c owns CPU_CLOCK, CPU_BASECLOCK,
CPU_REMCLOCK and event decrementing. `pccore_guest_clock()` extends the 32-bit
CPU counter into a monotonic 64-bit domain for recording handoff arithmetic
and clears its extension state on machine reset. The extension is not
serialized; REC13 and later coordinators must stop safely before reset/state-
load transitions rather than treating the counter as continuous across them.

Guest display timing is generated by the TSP in io/tsp.c, not by the
frontend's nominal 60 Hz value. The TSP selects mode-dependent pixel clocks,
line counts, horizontal clocks and total frame timing, then calls
timing_setrate and schedules display/vsync events. drawscreenva and
screenvsyncva are therefore the authoritative guest video-boundary integration
points. Model, clock, mode, interlace and reset/state-load changes must be
treated as recording descriptor changes.

The frontend's SDL tick, frame skip, nowait and callback cadence are host
presentation policy. They must not determine recorded time or cause a complete
guest interval to be omitted.

## Audio ownership and timing

The current audio path is:

1. machine/pccore.c accepts 11025, 22050 and 44100 Hz and initializes the sound
   board.
2. sound/sound.c:sound_create owns an interleaved signed 32-bit stereo stream
   and a reserve sized to 100 ms.
3. sound/sound.c:sound_changeclock derives the sound clock from
   pccore.realclock / 25 and the output sample grid from rate / 25.
4. sound/sound.c:sound_sync converts elapsed guest clock into generated samples
   and calls streamprepare.
5. When recording is inactive, sdl2/soundmng.c:sound_play_cb calls
   sound_pcmlock, converts/clips to AUDIO_S16SYS, and supplies stereo samples
   to SDL. When recording is active, the callback consumes the separate
   playback buffer only. `soundmng_getspec` retains the requested and obtained
   rate, format, channels and buffer size as a plain `SOUNDMNGSPEC` for the
   future recorder owner; it does not expose SDL types through the core.

The ymfm bridge is in sdl2/ymfmbridge.cpp. The local YM2203 clock is 3,993,552
Hz and the YM2608 clock is 7,987,104 Hz. In the vendored
external/ymfm/src/ymfm_opn.h, both paths report these exact source rates under
the current fidelity policy:

| Chip | Minimum fidelity | Medium fidelity | Maximum fidelity |
|---|---:|---:|---:|
| YM2203 | 166,398 Hz | 332,796 Hz | 998,388 Hz |
| YM2608 | 166,398 Hz | 332,796 Hz | 998,388 Hz |

The current default is minimum fidelity. The ymfm bridge generates/averages
toward the configured output rate; its source-rate reporting and the existing
sound stream's sample accounting must be reconciled before recording production
is added. The oscillator/native chip rates must not be mistaken for the public
recording sample grid.

### Look-ahead finding

Before REC11, sound_pcmlock could call streamprepare(remain - reserve) from the
audio callback when the remaining stream was beyond the reserve, while
sound_sync also generated based on guest elapsed clock. REC11 now establishes a
recording-active single producer: sound_sync generates the interval once,
copies final S16 samples into the indexed bounded recording FIFO, and the SDL
callback consumes playback data only. Arming copies the already-generated
pending tail into that FIFO, and a full recording FIFO fails the recording
session before synthesis rather than dropping a claimed sample or waiting
under the core/audio lock. Device absence and mute remain counted timeline
conditions.

The required v1 common epoch is a guest-clock/sample-grid relation. Audio
samples are selected by exact rational grid positions for each completed guest
interval; callback arrival is not a timestamp source.

### REC10 seam and handover observation

REC10 extracted `vaeg_sound_generate_s32_stereo`,
`vaeg_sound_lookahead_frames` and `vaeg_sound_pack_s16_or_silence` as small
C99 operations. They own no chip state or clock, allocate no memory and take
no locks. `sound_sync` remains the guest-time generation path, while
`sound_pcmlock` can still generate callback look-ahead beyond the reserved
tail. The seam preserves the existing callback order, zeroing, generated
lengths, S16 saturation and source-NULL silence behavior.

`SOUNDSTREAMSTATE` is a value observation containing the configured rate,
buffer/reserve/remain/pending counts, cumulative generated samples since the
last reset, a reset epoch, `lastclock`, lock state and checked accounting
overflow. `SNDCSEC` remains the core stream lock and the existing `locks` guard
prevents nested PCM-lock ownership. A future handover must stop or pause the
device and reconcile generated, reserved and pending data outside the audio
callback/core lock; the observation API is not a callback-safe recursive
handover protocol and does not provide a reset-to-zero shortcut.

REC10 adds no recording feed. REC11 establishes the recording-active producer,
the bounded indexed S16 FIFO and playback-only callback boundary. It also
retains the obtained device description and reconciles the pending look-ahead
at arming. The source-NULL and mute handling preserve counted silence, while
the missing-device path keeps the sound core usable. The recorder coordinator
still must consume this API and connect it to the REC07 MKV writer at a safe
guest-time boundary.

## Validation, risks and human gate

The baseline build and ROM-less self-tests above establish that the audit
checkout builds and that the pre-existing QA entry points still pass. They do
not establish FFV1 encoding, MKV seekability, audio/video alignment, native or
displayed readback, or any Windows/Linux/real-GPU behavior. No movie was
generated because REC00 forbids implementation.

Open implementation risks are:

* the recorder coordinator must drain the bounded audio FIFO at safe guest-time
  boundaries and fail cleanly if it cannot keep up;
* the obtained SDL device format and any later conversion state need an
  explicit recorder owner;
* each native presenter needs a final-client capture proof, including Metal
  completion/storage and D3D11 row-pitch/format handling;
* geometry, clock, audio-format and presenter changes need a safe-stop boundary;
* bounded backpressure must be cancellable without taking audio, emulator-core
  or GPU locks;
* native and displayed OSD must remain distinct without changing existing
  screenshots or QA captures.

REC00 machine evidence was accepted on 2026-09-19. REC10 machine evidence is
complete for the seam scope at evaluated source commit
`5e2823a437b4082b5b41f5f7d89e7a1073e738bd`; REC11 machine evidence is complete
at evaluated source commit
`4165d9fda677ac0a9ff3ed954c0100d0a988dcfc`; REC12 machine evidence is
complete at evaluated source commit
`415ca5f114b27990daa04ce157f4f27616ac98b2`. REC13 is now implemented at its
machine gate; these statements are engineering evidence only, not human or
release validation.

## REC13 integrated Native session result

REC13 connects the REC12 Native tap and REC11 audio FIFO in
`sdl2/recording/recording_native_session.cpp`. The private controller starts
after `pccore_reset` from `sdl2/np2.c`, reads the Native descriptor from
`sdl2/scrnmng.c`, observes `SOUNDSTREAMSTATE` and `SOUNDRECORDINGSTATE`, and
starts `sound_recording_start` before the tap. `run_guest_frame` calls the
session after `pccore_exec`; the tap itself is still emitted only from the
completed `screendispva` guest boundary in `machine/pccore.c`.

The first accepted Native frame is staged. At the next guest boundary the
coordinator closes `[previous_end_ticks, current_end_ticks)`, selects the
exact rational sample range from the REC10/REC11 grid, copies the RGB24 frame,
audio and REC08 snapshot into a `CaptureSlice`, and admits it through
`RecordingSession::wait_submit`. The queue's wait is outside the core/audio
lock and is canceled by sink failure, reset, state-load or epoch change.
The worker owns `FfmpegVideoEncoder`, `FfmpegPcmEncoder` and
`FfmpegMatroskaWriter`; the final path is published only after a successful
trailer, flush and close. Video PTS is the slice begin tick and packet
duration is the complete guest interval, rather than the encoder's default
one-tick duration.

The actual OSD layers remain the two REC08 guest-area diagnostic classes:
Video information and Framebuffer information. REC13 carries the matching
snapshot produced by `scrnmng_capture_native_recording_frame_with_snapshot`
on the Native recording-owned copy; it does not add an OSD recording option,
omit enabled guest OSD, include host title text or include ImGui. Displayed
video and final-client presenter capture are unchanged and remain later
milestones.

The private controller accepts a maintainer-provided output path and optional
positive frame limit through environment variables. This temporary control is
not public UI and does not publish private media identities. A live
ROM-only/deterministic-device run produced four Native intervals. Independent
ffprobe/ffmpeg checks found one FFV1 v3-compatible BGR0 640x400 stream and one
PCM S16LE stereo 22050Hz stream, four packets/decoded frames in each stream,
video PTS beginning at zero, and 1563 stereo PCM frames. This is source and
writer integration evidence only: no guest OS/media motion run with enabled
OSD, physical device, Displayed capture, VLC playback or human gate was
performed.

The evaluated implementation commits are
`7b530d79` and
`595d6ed2d575dcc3b3d36aa5dc5c554d4c4f8b79`. The enabled recording suite
passed 14/14 and the disabled suite passed 9/9, with focused Werror and
ASan/UBSan queue/session tests passing 2/2 each. REC13 is therefore
IMPLEMENTED_UNVERIFIED rather than human-approved or release-complete.
