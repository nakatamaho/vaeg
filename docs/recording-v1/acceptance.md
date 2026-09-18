# Acceptance and failure tests

The task file selects checks from this catalog. New CTest names/labels here are
requirements to implement, not commands claimed to exist today. REC00 records
existing build/test commands; every subsequent report records concrete commands.
Add focused `recording` labels, with `romless`, `frontend`, `gpu`, and platform
labels as appropriate. No new check depends on private ROMs by default.

## Core and file tests

| ID | Required proof |
|---|---|
| C01 | Recording-OFF configures, builds, runs existing relevant tests with no FFmpeg installation. |
| C02 | Owned frames/slices survive producer buffer mutation and destruction; no borrowed GUI/GPU/audio pointers. |
| C03 | Checked arithmetic rejects malformed sizes/stride/time bases, zero rates, overflow and invalid counts. |
| C04 | C core headers compile as C99, frontend as existing C++17; no FFmpeg/STL leakage across C APIs. |
| C05 | Queue capacity is bounded in bytes/count; slow writer loses no accepted intervals; Stop wakes waits. |
| C06 | Start/stop/failure state transitions are idempotent; worker ownership and repeated sessions are safe. |
| V01 | FFV1 decode to RGB24 exactly equals expected post-OSD/pre-encode RGB24 at every pixel. |
| V02 | RGB565 conversion uses the existing canonical rule; guards/padding are excluded. |
| V03 | Red/blue corner identity, odd sizes, row padding, orientation, alpha normalization and random RGB pass. |
| V04 | FFV1 v3 and keyframe/seek behavior confirmed; missing codec/unsupported format is explicit failure. |
| A01 | Decoded PCM bytes equal selected final emulator PCM bytes; stereo order, clipping and endian cases pass. |
| A02 | Exact audio sample count for whole interval equals sum of arbitrarily partitioned chunks. |
| A03 | Application mute yields counted silence; no device/delayed device callback does not corrupt capture. |
| A04 | No double synthesis; recording transitions retain waveform cursor and guest sound state. |
| M01 | A successful file has exactly one FFV1 video and one PCM audio stream, valid dimensions/rate and usable cues. |
| M02 | Partial write/seek/header/trailer/flush/close/rename failures never report success or overwrite a file. |
| M03 | Unicode/space paths, existing targets and races, cancellation before first interval, large offsets pass. |
| M04 | Final decoded interval/frame count equals accepted count; no unreported tail loss on normal Stop/exit. |

## Timing and source tests

| ID | Required proof |
|---|---|
| T01 | Video IDs/PTS follow guest boundaries, not nominal FPS, VSync, renderer rate or host clock. |
| T02 | Multi-hour virtual rational schedule has no cumulative drift, overflows or split-block count changes. |
| T03 | Encoded flash/PCM-impulse fixtures at beginning/middle/end meet `qv + qa + 1/F` alignment bound. |
| T04 | Pause adds no guest duration; fast/slow producer runs yield identical decoded synthetic RGB/PCM content. |
| T05 | Start/stop mid-audio-block preserves the declared sample-grid offset; no independent clock-zeroing. |
| N01 | Native records every complete guest composition plus all enabled guest-area OSD, without CRT/host strips. |
| N02 | Original canonical buffer and existing QA/screenshot outputs remain unchanged with recording on/off. |
| N03 | Guest blanking/unchanged frames/OSD-only changes and actual supported guest modes are handled. |
| N04 | Native recording remains independent of active SDL/D3D11/OpenGL/Metal presentation choice. |
| D01 | Displayed is captured after final in-client GUI/OSD/status and before presentation, in drawable pixels. |
| D02 | Readback normalized bytes match the exact fixture target; no channel swap, flip, padding or gamma error. |
| D03 | Real backend with CRT on produces its actual filtered client image plus overlays, not Native fallback. |
| D04 | Geometry/DPI/minimize/device-loss/presenter-switch policy stops cleanly and restores UI state. |
| D05 | Capture does not render temporal shader history twice or make unrelated UI redraws timed guest frames. |

## Lifecycle, UI and release tests

| ID | Required proof |
|---|---|
| L01 | Pause, immediate stop, stop under pressure, quit, reset, state load, clock/rate change obey common cut. |
| L02 | Existing screenshots, input, audio playback, frame pacing and state-save regression tests still pass. |
| U01 | Exact two source labels, Native default, no OSD checkbox, truthful capability/error/finalizing states. |
| U02 | CLI invalid source/path/frame limit/unavailable capability fails early with nonzero status. |
| U03 | GUI errors keep emulation usable; output is never silently renamed/overwritten or switched to Native. |
| P01 | Recording-OFF overhead: no allocations/readbacks/worker/audio-production changes from recorder. |
| P02 | Slow encoder and high-resolution noisy RGB tests have bounded queues, observable costs and no file drops. |
| R01 | Enabled target packages resolve exact intended libraries; no runtime `ffmpeg` executable is required. |
| R02 | Release dependency/license/source/configuration notices are reviewed for the actual shipped binaries. |
| R03 | Native and Displayed recordings open in actual supported-platform VLC, play video/audio, pause and seek. |
| R04 | All platform-matrix cells have actual required evidence; unavailable cells stay explicitly unverified. |

## Synthetic fixture design

Use a seed-recorded frame generator with distinct corners, alternating red/blue
fine checkerboards, gradients, a sequence-ID patch, known text/rect OSD and
semantically separate status/menu areas. Give the background and overlays different
colors to expose wrong inclusion boundaries. Use edge alpha and odd row strides.
Do not generate only all-black frames or silence for a purported positive test.

Audio fixtures contain deterministic integer sample patterns, silence intervals,
channel-specific impulses and known saturation-boundary values. Align several
video flashes to exact sample-grid events at a rational synthetic frame rate.
The generator independently produces expected RGB24 and S16LE bytes and exact
timing metadata. Run those inputs through the actual production recorder backend,
not an external ffmpeg encoding command disguised as VAEG functionality.

Unit validators must accept valid input first, then each negative test mutates one
thing and asserts a stable intended error code. Use independent expectations;
do not generate the expected checksum by decoding the very output under test.

## External inspection examples

These commands inspect an already-generated public synthetic fixture. They do
not implement recording, prove its source tap, or replace the automated validator.
Use fresh output paths because `-n` intentionally refuses overwrite.

```sh
ffprobe -v error -show_format -show_streams -of json fixture.mkv
ffprobe -v error -select_streams v:0 -show_frames -of json fixture.mkv
ffprobe -v error -show_packets -of json fixture.mkv
ffmpeg -v error -xerror -i fixture.mkv -map 0:v:0 -map 0:a:0 -f null -
ffmpeg -v error -n -i fixture.mkv -map 0:v:0 -an -fps_mode passthrough \
  -pix_fmt rgb24 -c:v rawvideo -f rawvideo decoded.rgb
ffmpeg -v error -n -i fixture.mkv -map 0:a:0 -vn \
  -c:a pcm_s16le -f s16le decoded.pcm
```

Compare `decoded.rgb` and `decoded.pcm` with the independent fixture's expected
files. Do not add `-r`, an audio resampler, or default frame duplication to make
a mismatch disappear. RGB hashes apply to normalized decoded pixels, not the MKV
container bytes, which may vary with metadata/encoder builds. Guest/OSD source
hashes and post-encode hashes have different meanings.

## Human VLC checklist

Use the actual packaged executable to create both sources on its target OS.
Record VLC version/platform and open/play/audio/seek results. Check OSD inclusion,
Native absence of CRT/host strips, Displayed presence of active CRT/client status,
start/end, pause/resume of playback, seeks near the beginning/middle/end and no
obvious increasing A/V offset. Human playback does not replace exact decode tests.
Mobile VLC is not a promised release target unless separately tested.
