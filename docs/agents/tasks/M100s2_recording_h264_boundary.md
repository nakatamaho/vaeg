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

Further request: macOS SDL2/LibArchive static by default too.
[0455d0c9](https://github.com/nakatamaho/vaeg/commit/0455d0c94e7018fdf2db4f4574b0e98088f658e0)
selects existing pinned static SDL2/LibArchive/zlib/xz paths and updates Mac
presets, without changing Linux defaults or the already-static Windows path.
Source-cache copies are byte-verified unchanged (4202 files), no new downloads
or vendored edits. Default/resolver fixture5/5; actual static SDL/archive ON
25 pass/one hardware skip, OFF16/16, final Release selftest pass. Final Mac
imports are exclusively Apple system libraries/Frameworks, no MacPorts dylibs.
Updated artifact/hash/logs in the [static handoff](../../recording-v1/static-linking.md).
Windows86535d23 artifact unchanged; no redundant build or profiling rerun.
No inherited edit/commit, private-media access/publication, hosted CI/push,
clean-checkout/media/demo/human or legal gate claimed.

REC time follow-up: [f32f9d33](https://github.com/nakatamaho/vaeg/commit/f32f9d336e2d35eb5d0dea2e3397f80b29a1ca96)
shows movie elapsed MM:SS beside the REC lamp, from existing session ticks and
guest clock, not host wall time. Pause does not inflate it; restart is00:00;
minutes do not wrap at60/100. KANA/CAPS right alignment retained, timer-aware
narrow/long-duration lamp layout. Arithmetic edge cases and real pixel/glyph/
red-amber/reset tests cover320/400/640/1280. Final extended ROM-less tests pass
ON/OFF; full ON selection25 pass/one hardware skip, OFF16/16; static Mac Release
selftest and Win Release/import audit pass. Production formatter also passes
isolated Wine tests with explicit unused panel stubs; initial link failure is
retained, no native Windows GUI credit. [Artifact/evidence](../../recording-v1/recording-ui-followup.md#current-rec-movie-time-display).
No protected source edits/commits, private-media access, hosted CI/push or gates.

One-click follow-up: approved `[録画開始/録画停止] [down arrow]` top controls in
[e71f8396](https://github.com/nakatamaho/vaeg/commit/e71f83966b9c0819f89a0c29698daf955e64bd87).
Existing start/current settings and asynchronous save/stop are retained; success
opens no dialog, errors show diagnostics/settings. Separate popup never toggles
recording. Controls lead the bar when portrait/large-font preceding menus would
clip them. View emits intent only; actual ImGui mouse press/release tests cover
5 viewports/fonts x5 idle/active/saving/capability states, exactly one toggle,
disabled states, independent arrow/settings and bounds. ON25 pass/one hardware
skip; OFF recording16/16 plus ROM-less1/1; final static arm64 Release selftest,
MinGW Release/import audit pass. Source change only gui.cpp; unchanged expensive
backend/profiling evidence reused with original identities/scope, not new native
Windows/live A/V acceptance. [Handoff](../../recording-v1/recording-ui-followup.md#current-one-click-recording-controls).
Binaries/reports updated, priorf32 checkpoint retained. Inherited sources remain
byte-identical/uncommitted; no MP4 export, new private-media access, hosted CI,
push/main movement or human/media/demo/legal gate claimed.

Automatic split follow-up: maintainer approves native geometry/timing changes,
15/24kHz and manual90-degree rotation by separate files, not a fixed/padded canvas.
[5b0eb6c1](https://github.com/nakatamaho/vaeg/commit/5b0eb6c129a6a4e01d24d757b2dc55d53fd49fdc)
keeps FIFO/grid/cursor through same-epoch handoff, commits the old tail atT,
seeds new snapshot atT, resets only the per-file queue/mux/video descriptor.
Reset-driven monitor changes close the old epoch and rearm at the first new
frame. Cumulative REC time/global frame limit survive parts. Existing target
collisions preserve old finalized files and sentinel; empty parts never publish.
GUI opts in, CLI/environment batches do not. [Contract/evidence](../../recording-v1/screen-change-stop.md#current-automatic-splitting-policy).
ON25 pass/one skip; OFF recording16/16 plus ROM-less1/1; strict focused ASan/
UBSan4/4;18 synthetic movies/61 exact frames/48800 stereo samples independently
decoded with no same-epoch boundary loss/duplication. Initial private candidate
exposed immutable FIFO first_sample vs read-cursor confusion; corrected primer
never re-discards after a same-epoch split, tests now model real FIFO semantics.
Private GUI-start geometry run and separate final Release actual rotation/reset
helper probes reach1200/exit0 with decodable parts. One combined harness never
reached its third GUI callback; a separate second-callback reset probe fixes
that test condition, not production code. No DIGDUG-specific/native Windows/live
Metal/audio-device or whole-guest strict UBSan acceptance. MinGW Release/import
and full-worker cross-build pass. Final arm64 static Release selftest passes.
User permits owned native session/test additions while preserving inherited edits;
owned-only source/test workers pass, three-way inheritance removal/reapply is
byte-exact; only owned changes committed, Metal untouched. [Artifacts](../../recording-v1/recording-ui-followup.md#current-automatic-recording-splits).
Raw private commands/media/logs/hashes stay outside Git. No push/main movement,
hosted CI, MP4 addition or human/media/demo/legal gate.
Final [9147dfa5](https://github.com/nakatamaho/vaeg/commit/9147dfa530d2a486e0ca304cb5b3576fbba8e41b)
separates reset auto-split from generic state-load/rewind cancellation; a valid
active fixture + that single generic notification asserts cancellation/no movie,
not unintended restart. Affected ON/OFF/ROM-less, strict sanitizer4/4 and owned-
only workers pass again. Final static Release artifacts embed9147dfa5.

Pre-gate main synchronization: maintainer requests main ->
`topic/m100os-recording-resume` first, holding the earlier r2 integration request.
Fetched main `02ce6d474c6ad50173d109893023f584e7032ca7` merges without conflicts
(8087/recent-boot changes included). Original3 inherited working files remain
byte-identical and unstaged; no inherited changes enter the merge. Local ON
selection26 passed/one H.264 hardware skip, OFF13/13 including8087/ROM-less,
encoding/EOL/case/Markdown-comment and staged whitespace checks pass. Release
handoff is refreshed after the merge commit. No new Metal/backend/codec policy
change or main-branch update. The maintainer's macOS practical lossless-recording
check is still pending; this sync is NOT a passed gate, and main incorporation
must wait for explicit acceptance. Current recorder is FFV1 lossless compression,
not raw uncompressed video.
Merge commit [2bd194b9](https://github.com/nakatamaho/vaeg/commit/2bd194b96352cb7ea4af8f56767074a4896c6d9c)
parents050f5ad2 +02ce6d47. Final Mac static Release is38717112 bytes/SHA256
`f5d19553a59259c3285ec4f98ad2cd7606e308c94136d692d11a790b79da1f88`;
selftest0/system-only imports. Windows refresh cannot start because the local
Docker daemon socket is unavailable, not a compilation failure;9147 executable/
report preserved. [Candidate and pending gate](../../recording-v1/recording-ui-followup.md#current-main-synchronized-recording-candidate).
No private-media/performance rerun or gate pass; r2 integration remains held.

Maintainer requests a repeat MinGW build after deleting Colima. Recreated only
dedicated profilevaeg-build (VZ/Rosetta), preserved original Docker context;
pinned Ubuntu24.04 linux/amd64 container restored using cached FFmpeg archives/
librashader/SDL2/LibArchive/zlib/xz. Source1b30383ddaf9ea5bcd59a7c2e672bb4fa64e8543
contains code2bd194b9 plus docs. Build0, system-DLL import audit0, embedded full
commit verified; Release/recordingON/staticFFmpegON. Current executable58880144
bytes/SHA256 `474b5c390a0e4dab27d34017f5a6641ac7a2404a88cc7a867b3ba6deb3da2a7d`.
Identical audio-local alias checked; inherited3 files byte-identical,2 audio files
match Windows source; unchanged inherited diffddd3f6b3… and no Metal patch there.
Exact Docker/bootstrap commands, exit status, dependency/log/worker digests in
local mingw-rebuild-validation.json. Prior9147 binary/report retained. Profile
stopped after build to free host resources. No source edits, private-media/test
rerun, native Windows/performance acceptance or gate pass. Repo validators pass;
only this task and current-handoff/status evidence updated. Main/r2 unchanged.
