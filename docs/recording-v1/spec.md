# Recording v1 specification

Status: proposed implementation contract approved in scope by the current user
request; actual source integration and milestone-number allocation require REC00.
The requirements below are design choices for this feature, not assertions about
an uninspected local checkout. `sources.md` distinguishes public observations.

## S1. Scope and identifiers

Implement one recorder with exactly two source choices. UI text must be exactly
`Recording source`, `Native video`, and `Displayed video`. Use CLI values `native`
and `displayed`. Do not expose `raw` or `presented` as additional sources. Existing
classes named NativePresenter refer to native GPU presentation, **not** to the
Native recording source; keep those concepts separate.

Every completed successful recording is a standard local seekable Matroska file
with exactly one FFV1 video stream and one PCM audio stream. Recording support is
an optional build dependency. No `ffmpeg` subprocess is needed at runtime: use
libavformat, libavcodec, and libavutil behind a private backend. External `ffmpeg`
and `ffprobe` are verification tools only.

## S2. Native video

Capture the complete guest raster produced by VAEG's existing composition,
including text, graphics, sprites, border/blanking pixels already represented in
that raster, and any other emulated visible layers. This is not VRAM, a single
plane, a GPU upload-padding rectangle, a scanline work buffer, or a shadow buffer
including memory guards.

Copy the completed canonical raster into recording-owned storage. Convert its
existing pixel representation to opaque 8-bit RGB using the existing documented
canonical conversion. Then draw the enabled guest-area OSD from an immutable
snapshot. Do not apply CRT effects, curvature, masks, artificial scanlines,
window scaling, aspect correction, or a new deinterlacer. Do not invent missing
hardware overscan or reinterpret VA interlace/field composition.

The coded width/height are the canonical dimensions established in REC00. Do not
hardcode them from a guessed VA mode or force a 320-wide mode into an unrelated
new canvas. The public tree currently has a 640-by-400 canvas, but the checked-out
composition descriptor is authoritative. Existing conversion and field semantics
must be reused. Native uses sample aspect ratio 1:1 in v1: no hidden presentation
stretch in player metadata. Document this explicitly for non-square-pixel uses.

The pre-OSD canonical image must remain byte-for-byte unchanged. Screen-shot,
QA, debug capture, shader-input, and guest state contracts must not change merely
because movie recording was enabled. If an enabled OSD obscures a guest pixel,
that underlying pixel is not recoverable from the movie.

## S3. Displayed video

Capture VAEG's real final main-window **client** image, after all application
rendering and before presentation hands it to the host compositor. Include the
currently active CRT result, scaling/aspect/letterboxing, enabled OSD, in-client
status/menu bars, in-client popups, and any application-rendered cursor. Exclude
OS decorations/title bar, desktop, system cursor planes, external windows, and
native OS dialogs. Never add OS screen-recording permissions or APIs.

Capture drawable pixels, not logical UI points. Do not render a second independent
CRT pass for the recorder: temporal history, frame numbers, dithering, and GUI
state could differ. Read the actual final target, or use a shared final target
that is rendered once and copied identically to the window and recorder. A shared
target is permitted only with demonstrated parity and without changing the live
renderer's public behavior. Never silently substitute the Native image when a
Displayed backend is unavailable.

Displayed is sampled at guest frame boundaries in emulator time. It is not a
wall-clock screencast: UI-only redraws while emulation is paused do not become
extra timed movie frames. Fast-forward may produce more final client images than
the physical monitor can scan out; the recorded source is the application image,
not a claim about photons physically shown on the monitor.

## S4. OSD and status: fixed behavior, not a new checkbox

OSD means emulator overlays belonging to the guest-image area. Both sources burn
in all such overlays currently enabled by ordinary VAEG settings. Do not force all
diagnostics to be visible. Do not introduce an `Include OSD` checkbox,
`--record-osd` option, hidden clean source, recording-only overlay filter, or a
mandatory watermark. Ordinary OSD enable/disable controls still work.

Classify existing layers in REC00, by their actual layout and drawing path:

| Layer | Native | Displayed |
|---|---|---|
| Guest raster and guest-drawn text | Include | Include |
| Enabled emulator OSD over guest content | Include, composed at native resolution | Include as actually drawn |
| A status item genuinely drawn as guest-area OSD | Include | Include |
| Separate host status/menu strip outside the guest raster | Exclude; do not append a strip | Include when part of the client |
| In-client configuration/menu dialogs | Exclude | Include |
| OS/native separate dialogs, decorations, desktop | Exclude | Exclude |

The same snapshot supplies semantic content to live OSD and Native composition.
Map OSD anchors and glyph sizes to the native coordinate system without scaling
the guest image or downsampling a screenshot of the live UI. Native need not have
byte-identical glyph rasterization to a scaled GPU overlay; content, ordering,
visibility, clipping, and anchoring must agree. Reuse the existing font assets;
never copy private guest font ROMs into the implementation or public tests.

## S5. Video representation and losslessness

The stored video is FFV1 version 3, 8-bit RGB, with all frames independently
seekable (`gop_size = 1`) and slice CRC enabled where the supported encoder
provides the documented v3 option. Use a supported packed RGB format with explicit
byte order, preferably BGR0 on the supported little-endian targets. BGRA with
opaque alpha is an acceptable capability-tested fallback. Do not assume that an
SDL integer pixel-format name implies identical in-memory FFmpeg channel order.

A reversible channel permutation or padding byte change is allowed. RGB-to-YUV
conversion, chroma subsampling, limited-range remapping, tone mapping, palette
reduction, and lossy compression are not. Normalize to RGB24 for exact round-trip
comparison; padding bytes and unused alpha are not guest pixels. Preserve the
actual 8-bit Displayed source bytes, including shader rounding already present.
No claim of cross-GPU bitwise equality is made.

For RGB565 input, use the source's established expansion rule; do not choose a
new rounding rule for movie recording. Preserve full numerical RGB range. Set
metadata only when it is accurate and supported; unspecified primaries/transfer
are better than incorrectly labelling everything BT.709 or limited-range YUV.
Reject unsupported HDR/high-bit-depth presentation instead of silently reducing it.

## S6. Audio

Record the final emulator-produced audible PCM, never loopback/system audio.
In the inspected public frontend this is signed 16-bit stereo after the existing
saturation operation. The initial implementation therefore uses PCM S16LE stereo
at the effective configured VAEG output sample rate. Do not hardcode 44100, derive
the rate from the OPNA chip clock, or add a second resampler. REC00 verifies the
local format; a different canonical format requires an explicit documented
contract adjustment before encoding, not an unnoticed conversion.

Retain all sources already mixed by VAEG, including mechanical/mixed sources if
present in that final stream. Do not add a new microphone, MIDI device recorder,
or sound chip. Existing per-source/application volume applies. Application mute
or Sound Disabled produces zero samples of the correct duration while advancing
emulated audio state; OS/device mute and output routing do not affect the movie.
Device absence is not application mute. The recorder must generate valid audio
without a working playback device, including in ROM-free fixture tests.

During recording, synthesis has one owner driven by guest time; the SDL audio
callback consumes already-produced playback samples and never synthesizes extra
recorded samples. Recording must not call chip generation a second time or
advance sound state twice. Preserve the non-recording legacy path until the
recording path is proven; switching mode must be safe at an explicit boundary.
See `timing-audio.md` for the mandatory two-step refactor and entry reconciliation.

## S7. Timing and completeness

Use the emulator's actual video timing and a common monotonic guest-time domain.
Do not use the UI nominal-FPS constant, round to 60/59.94, or timestamp with host
wall time, VSync, or audio callback invocation time. Each captured complete guest
frame is associated with an exact rational guest timestamp and a sequence ID.
Unchanged guest frames still occupy their intervals and carry the appropriate OSD.

Audio uses a cumulative integer sample position and the actual sample rate. Keep
a shared recording epoch and a declared initial sub-sample offset; do not
independently zero unrelated audio and video clocks. Use integer/rational
arithmetic and checked overflow. Pause freezes recording time. Fast-forward and
slow host execution change production speed, not the encoded timeline.

Matroska timestamps are quantized to the muxer's actual stream time bases. Preserve
exact internal clocks, inspect stream time bases after writing the header, and
rescale absolute endpoints once. A common FFmpeg Matroska time base is 1/1000 s;
exact PCM sample count does not imply sub-millisecond container timestamps.
Do not patch the muxer to pretend otherwise. Acceptance distinguishes sample
integrity, bounded container quantization, and cumulative drift.

No intentional video/audio drops, duplicate samples, zero-filled error concealment,
or loss of intermediate guest frames is allowed on a successful recording.
Bounded queues apply backpressure outside core/audio/GPU locks. When an unrecoverable
capture/encode/I/O error occurs, stop recording, report failure, preserve the
partial artifact, and leave emulation usable. Playback-device underruns under host
load are separate from movie integrity and must not be encoded as guest silence.

## S8. Session lifecycle and geometry

One active recording. Source, coded dimensions, sample rate/format, and presenter
identity are fixed for the file. Start at a well-defined completed guest-frame
boundary and stop at a common completed capture interval. A start followed by stop
before any interval is accepted creates no successful empty movie. Do not force
emulation forward solely to stop while paused.

Normal stop drains accepted work, flushes both encoders, writes the Matroska
trailer/cues, closes I/O, and only then reports success. Destruction must not race
callbacks or retain dangling pixel/GUI/GPU memory. Stop is idempotent and can be
requested during start, queue pressure, pause, or finalization.

For Native, host window resize is harmless. If the canonical descriptor changes,
stop before accepting the incompatible interval. A guest mode switch that retains
the same descriptor continues normally. For Displayed, v1 does not resample old
and new client sizes into one fixed canvas: disable avoidable geometry-changing
controls while recording, and finalize on an actual drawable-size/DPI/fullscreen
change. Window move with unchanged drawable size may continue. Zero drawable size,
minimization that prevents acquisition, device loss, or presenter replacement ends
Displayed recording visibly; it must not silently become Native.

Reset, state load/rewind, emulated clock change, audio-format change, and presenter
switch requests finalize recording before that operation is applied. Disk/media
changes may continue when timing and descriptors stay valid. State save may continue
but must not serialize recorder state or private output paths into guest state.
No auto-segmentation or recorder pause/resume mode in v1.

## S9. Files, build, and UI

Only local filesystem `.mkv` output. Never overwrite an existing recording.
Use an exclusive-created sibling partial file and non-replacing finalization.
Do not call a shell or interpret the path as a network/protocol URL. Support spaces
and Unicode, including Windows paths, via the repository's approved path layer.
See `ffmpeg-backend.md` for exact failure and file-ownership requirements.

Add `VAEG_ENABLE_RECORDING` (default OFF) and target-scoped FFmpeg discovery. OFF
must configure/build/run without FFmpeg headers, libraries, or executables. ON
requires all three libraries and the necessary codecs/muxer with actionable
errors. Preserve existing release artifacts; recording-enabled packaging is
explicit and must pass the repository's distribution/license gate.

UI: Start Recording opens a save/source dialog; Stop Recording remains reachable.
Show recording elapsed **guest** time, finalizing state, and errors. Before a
Displayed backend is proven/available, show the exact label but disable selection
with a reason. The completed v1 must support SDL fallback, Windows D3D11, Linux
OpenGL, and macOS Metal where those presenters are supported in the checkout.

Public CLI: `--record FILE.mkv`, `--record-source native|displayed`, and
`--record-frames N` for a positive completed-interval limit. Default source is
Native. The frame limit is not FPS and not an emulation instruction limit. These
options must coexist with, not reinterpret, existing headless/frame-run options.
CLI errors are nonzero; a GUI error must not terminate a healthy emulator.

## S10. Evidence and completion

Public tests use synthetic raster/OSD/PCM generators without ROMs, disks, private
fonts, or network access. Validate decoded RGB bytes, PCM sample bytes/counts,
frame order, timestamps, final duration, source completeness, lifecycle, failure
paths, and FFmpeg-OFF behavior. Include actual VLC open/play/audio/seek testing
for released platform builds. Compilation alone is not GPU runtime validation.

Do not claim that a synthetic encoder fixture proves emulator integration, that
an SDL dummy image proves Displayed capture, or that a successful Linux build
proves D3D11/Metal. Required unavailable evidence is `IMPLEMENTED_UNVERIFIED` or
`BLOCKED`, never PASS. Preserve the existing repository's human gates.
