<!-- Copyright (c) 2026 Nakata Maho. SPDX-License-Identifier: BSD-2-Clause -->
# M100s2 — hardware H.264 / Matroska checkpoint

Contract: [OS-assisted plan](../../recording-v1/os-assisted-plan.md). Reuse the
existing libav boundary, seekable exclusive Matroska I/O and PCM encoder; retain
FFV1 tests and default behavior. First locally executable provider is macOS
VideoToolbox. Hardware-required open plus public encode/decode are mandatory;
provider listing is insufficient. Do not silently fall back or treat this as
live emulator A/V integration, distribution approval or another OS's proof.

Checks: H.264 config/decoder probe, production H.264/PCM public MKV fixture,
independent verifier, path collision preservation, archive ON/OFF regressions,
focused Werror/ASan/UBSan, repository encoding/EOL/case and whitespace checks.
Evidence and handoff: [status](../../recording-v1/os-assisted-status.md).
Sequential task-local progress authorized; LOCAL_ONLY, no routine approval stop.

Maintainer-requested build follow-up: the separate FFV1-enabled MinGW artifact,
minimal LGPL FFmpeg build, C/C++ header portability correction, checks and
runtime/distribution limits are recorded in
[MinGW FFV1 build evidence](../../recording-v1/mingw-ffv1-build.md). This enables
the existing archive path, not the unfinished Windows real-time H.264 provider.

Further maintainer feedback requests automatic dated output under `recordings/`,
clockwise video rotation, Japanese start/stop/settings and a RECORDING lamp left
of KANA/CAPS. Implemented with RGB rotation and panel-position tests; see
[follow-up evidence](../../recording-v1/recording-ui-followup.md). The separately
reported movie-only red tint remains OPEN pending affected-player/source
comparison; exhaustive RGB565 and independent Windows-produced public MKV
checks pass, but are not a private-guest reproduction or a human Windows gate.

Later feedback changes the label to REC, moves it left and requests a single
start/stop toggle. VLC shows only three colors, with white appearing yellow.
The FFV1 container now explicitly declares full-range RGB/8-bit metadata;
packet-identical public before/after fixtures are retained. A working-tree test
build also includes the unchanged inherited audio-session patch: committed
baseline reproduces `audio_clock_origin_invalid` in an advancing-arm fixture,
whereas the local snapshot arms and completes sample intervals. This is not a
clean-commit build, none of the three inherited files was changed/committed,
and native Windows/VLC confirmation is pending. Exact identities and evidence
are in the [current follow-up](../../recording-v1/recording-ui-followup.md).

The maintainer confirms the RGB-tagged file does not improve VLC colors. Keep
that defect OPEN; compare controlled BGR0/BGRA files without changing production
packing. Both CLI fixtures decode to all original RGB frames; BGRA alpha is255.
A separate native C libVLC probe also times out during library load, providing
no decoded-frame/player evidence. Recording settings are corrected in
[ee5ebc8d](https://github.com/nakatamaho/vaeg/commit/ee5ebc8d1c4b5efbbf2489639803f1b0b3abf8ee):
viewport/font-aware opening size, wrapped/stacked fields and scrollable content
above a fixed action row. ROM-less layout regression (five viewport/font
profiles, remembered tiny size and long status) passes1/1; ON/OFF builds and
recording tests pass22/22,14/14; MinGW Release/import audit exit0. Latest test
binary still includes the unchanged/uncommitted audio patch, not a clean-commit
or native Windows gate result. No demos/media gate, hosted CI, push, main
change or private asset access was performed.

The maintainer confirms BGRA displays seven colors while BGR0 fails with three.
[Opaque-BGRA archive fix](https://github.com/nakatamaho/vaeg/commit/a5f5b3fb36c51c843f9598036d4537e1afc4033f)
updates encoder, capability/info/muxer contracts and exact RGB/alpha tests;
ON/OFF22/22,14/14 and three focused Wine tests pass. Windows Release-object
640x400 generation is blocked by a Wine allocator assertion; a CRT-only test
also fails at24MiB aligned allocation (8/16MiB pass). Low-address test-driver
experiments fail loading and give no validation credit; production PE policy
is unchanged. Native macOS application objects generate640x400/180 frames and
144,000 stereo PCM frames with independently exact RGB/alpha255/PCM. This is
not Windows runtime proof. Artifact identities, every retained failed attempt
and the remaining new-app/native Windows gate are in the current follow-up;
the tint ledger remains OPEN. No inherited dirty file was changed/committed.

Title-FPS follow-up: the pre-existing main gated FPS measurement on the separate
FRAME visibility switch. Reproduce FPS-only0.0 with120 draws/2s, then decouple
measurement in [9b0c44d7](https://github.com/nakatamaho/vaeg/commit/9b0c44d7c2d9044834f7078f0d41790f2c688d61).
The SDL-title regression covers all four FPS/FRAME visibility combinations;
macOS ROM-less1/1, OFF build and MinGW build/import audit pass. About2s initial
measurement delay and paused0.0 remain valid. Recorder code/input identities are
unchanged, so prior expensive/recording evidence is reused with its limitations.
This remains a working-tree audio-patch test build, not a native Windows gate.

Screen-change follow-up:
[105e5cac](https://github.com/nakatamaho/vaeg/commit/105e5cac38962c5e408891a98f8620f61edd733b)
implements the maintainer-approved automatic stop on screen-setting changes:
save only complete accepted intervals, stop recording, continue the guest. A
neutral15kHz private case reproduced the old exit1 with/without rotation, rather
than an ASan memory fault. Real GUI-start automation with final minimal-LGPL
Release objects reaches1200 frames after save/stop, and the movie independently
decodes with CRC/video/audio checks. Raw asset identities/evidence stay private.
ON24/24, OFF15/15, ROM-less1/1, focused ASan/UBSan2/2 pass; Windows static build,
import audit, integration cross-build and focused Wine policy pass. CLI batch
failure remains fail-closed. Unrelated full-guest UBSan diagnostics remain OPEN.

[Evidence, exact contract and artifact identities](../../recording-v1/screen-change-stop.md)
also records the arm64 Release handoff, the corrected pinned minimal shared
LGPLFFmpeg dependency selection, and controlled Debug/Release RGB565/session/
encoder/mux profiling. Final18 synthetic movies decode3240 frames/2592000 stereo
sample frames exactly. Processing throughput is not live guest/display FPS;
previous slow live recording is not explained by Debug alone. Existing inherited
sources remain unchanged/uncommitted. LOCAL_ONLY, no hosted CI, no media/demo or
human gate, no legal/release approval and no OS-assisted completion claimed.

Static-default follow-up: the maintainer requests Windows/macOS static FFmpeg
and macOS static librashader. [86535d23](https://github.com/nakatamaho/vaeg/commit/86535d23e8641d6f67f4e5f3683f23d69c622812)
sets desktop defaults, resolves exact FFmpeg archives/private flags, fails closed
without archives and keeps existing librashader archive/license audit. No
recording-OFF FFmpeg discovery. Isolated resolver4/4, real static ON25 passed
plus one H.264 hardware skip, OFF16/16, final Release selftest and Win static
build/import audit pass. Static preset/Metal loader tests pass; macOS has no
FFmpeg/librashader dylib dependency, while other local dependencies stay shared.
[Static artifact/command/check scope](../../recording-v1/static-linking.md).
Cached Rust archive/notices reused, minimal native FFmpeg built static from the
same pinned source. Compatibility workflow now prepares its pinned static
librashader instead of relying on the former dynamic default. No hosted CI run,
new private asset access, inherited edit/commit, push or gate completion.
