<!-- Copyright (c) 2026 Nakata Maho. SPDX-License-Identifier: BSD-2-Clause -->
# OS-assisted extension: implementation checkpoints

This extends the existing recorder; REC00--REC15 and their evidence are not
replayed or renumbered. Integration identities and preservation are in
[resume-audit.md](resume-audit.md). Only this recording task overrides older
FFV1-only/no-OS-capture and routine human-gate progression rules. LOCAL_ONLY;
real consent, technical prerequisites and final manual gates still apply.

## Contract

Exactly Native video / Displayed video. Displayed has a frame choice to include
real title bar/borders; Native disables it. Archive remains FFV1/PCM/MKV, exact
RGB, guest time, no drops and existing CLI defaults. New Real-time means hardware
H.264/PCM/MKV on a shared monotonic host timeline, 60 Hz sampling, repeated held
images through pause, bounded latest-frame admission and no guest backpressure.
No hardware provider success means unavailable, never CPU/FFV1 fallback.

Client means the complete final application client image, not guest viewport.
Decorated means actual window-specific OS capture, never monitor/root desktop or
imitated chrome. Initially decorated is real-time only. No new OSD filtering,
shader-history evaluation, canonical raster mutation or archive resampling.

## Legal checkpoint map

Unused M100s suffixes are separate from reserved M100r1--M100r28.

| Checkpoint | Feature units | Scope / status |
|---|---|---|
| M100s1 | OSR01, OSR02 partial | host schedule and bounded latest-owned image tests; implemented, host audio mapping pending |
| M100s2 | OSR04, OSR06 partial | direct-libav VideoToolbox H.264/PCM/MKV public probe PASS; live recording pending |
| M100s3 | OSR02, OSR03 | audited playback clock adapter and coordinator; NOT_STARTED |
| M100s4 | OSR08 | Native/client real-time + existing UI/CLI integration; NOT_STARTED |
| M100s5 | OSR10 | ScreenCaptureKit main-window adapter IMPLEMENTED_UNVERIFIED; consent blocks runtime |
| M100s6 | OSR05, OSR09 | MinGW-compatible Windows providers/capture; NOT_STARTED |
| M100s7 | OSR07, OSR11, OSR12 | Linux VA-API/NVENC, portal/PipeWire and X11; NOT_STARTED |
| M100s8 | OSR13--OSR15 | lifecycle, stress, performance, packaging and validation; NOT_STARTED |

Split each compiling checkpoint further if needed. Provider name enumeration,
mocked clocks, dummy rendering and cross-builds are not hardware/runtime proof.

## Initial source audit and remaining prerequisites

`sdl2/soundmng.c:sound_play_cb` supplies final S16 stereo at SDL_MIX_MAXVOLUME.
SDL requests no frequency/format changes; no playback resampler/time-stretcher
exists in that function. Nonarchive playback invokes `sound_pcmlock`; archive
uses `sound_playback_read_s16`. Device pause suppresses callbacks on mute and
guest pause. SOUNDMNGSPEC gives obtained format/rate/buffer size, but no proven
hardware presentation position or latency observation. Callback arrival/count
alone is NOT sufficient. A tested platform playback-scheduling adapter or a
conservative unavailable capability is needed before claiming real-time A/V.
Do not attach the archive FIFO to the new host timeline or enable video-only UI.

The new encoder initially accepts owned NV12 SDR frames on its worker. Matrix:
BT.709 limited range, BT.709 primaries/transfer; caller conversion must match.
Odd dimensions must be preserved via tested padding metadata or explicitly
rejected, not cropped. Rate control starts with a requested bitrate, not CRF;
measurement at actual sizes is pending. Hardware opens use no software fallback.

## Evidence and performance

Host: macOS arm64, macOS 26.6.2; development MacPorts FFmpeg (GPL/nonfree enabled)
is NOT an approved redistributed dependency. OFF must retain no FFmpeg dependency.
Existing baseline ON/OFF build and recording tests passed as recorded in audit.

Before optimization freeze Release reference conditions and numeric budgets from
measured recording-off/FFV1 workloads. No actual application baseline measurements yet; no numeric
performance or file-size PASS, no budget relaxation, and no optimization claim.
Required runs remain 60 host seconds per profile and 10 minutes A/V drift/resource
stress, real guest markers, pause/mute/speed/overload/device-loss tests, real
window decorations and consent/lifecycle tests. Keep per-platform evidence;
VLC picture/sound/seek/EOF and clean deployment remain pending.

Current execution evidence and precise next steps:
[os-assisted-status.md](os-assisted-status.md). No full OSR unit completion is
implied by the compiling sub-checkpoints above.

## Named checkpoint checks

Build `build/macos-rec13-on` and `build/macos-rec13-off`; run their recording-label
CTest sets, focused new tests and repository encoding/EOL/case checks plus
`git diff --check`. Werror/sanitizer focused targets when applicable. Keep the
three inherited unstaged source/test files out of task-owned commits. No push.
