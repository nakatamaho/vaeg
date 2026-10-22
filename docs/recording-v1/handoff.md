# Recording v1 handoff

## M100s11 accepted handoff — G100s11 PASSED

The maintainer explicitly replied **"passed!"** to the final Windows gate
checklist: V3/demo/OS/simple operations, recording/compression, video/audio
playback, seek/end playback and original MKV retention. The accepted source
closure is [fef92deabbe6de4f95008692ee41026e5a7d530e](https://github.com/nakatamaho/vaeg/commit/fef92deabbe6de4f95008692ee41026e5a7d530e),
which differs from the following clean Windows build only in four Markdown
files. This is maintainer-reported acceptance, not independent target-digest or
frame/sample/quality evidence. Previous component acceptance is retained.

Accepted clean static recording-enabled Windows build:

- Source [16ed03ee87e3de5f384a6a8dbce80520574a88ea](https://github.com/nakatamaho/vaeg/commit/16ed03ee87e3de5f384a6a8dbce80520574a88ea).
- `/Users/maho/work/pc88va/vaeg-mingw-16ed03ee/output/vaeg.exe`, 59,218,214 bytes.
- SHA-256 `7111621cddc2069911159d3fda25c62d83f6cc57c0099249a0525a90d070a5b7`.
- `/Users/maho/work/pc88va/vaeg-mingw-16ed03ee/output/BUILD-REPORT.md`, SHA-256
  `53949965b4b909b9662d1ca45850735eca9a245e1bf2d39100c85f4db1edcc44`.

Standard helper exit 0; static/import/codec audits pass, OS MP4 ON and local-only
static encoders OFF. The app's production inputs are unchanged from the accepted
screenshot build. Native Windows acceptance is the maintainer's report above;
no new agent-observed native execution or target-side hash collection is claimed.

All six component distributions now have clean-clone source-build evidence,
including the supplied PSD, and their payloads match the committed archives.
The five unchanged prior source builds are reused by identity, not rebuilt again.
Both aggregates have been rebuilt with the fresh PSD component: all-demos matches
all 19 payloads; all-demos-msdos4 matches all 25 files and the whole raw image.
Self-booting SGP raw-image equality and utility FDD build/boot/DIR also pass.
Prior ROM-aware macOS dummy/software runs visibly draw all 17 programs on each
OS after correct-CWD/loading-bound follow-ups, boot/select the SGP demo, render
the bundled VA demo title and start MANDEL.BAS (partial rows, not full completion).
An additional fresh-PSD PC-Engine run visibly draws Zundamon at frames 4000/6000.
No full 34-program rerun is claimed; unchanged payloads and byte-identical MS-DOS
media retain the previous evidence.
These agent observations do not establish physical GPU or human-gate acceptance.

**CP/M/SASI follow-up completed:** the maintainer supplied four locked CP/M
archives and accepted local-use notices. The installer generates tools/source/dev
images (exit 0), and a ROM-aware macOS run reaches CP/M 2.2 and its DIR listing.
The supplied `PCODESRC.LZH` matches the pinned cache copy; missing extra packages
are fetched from static publisher URLs and match the unchanged builder hashes.
Both SASI images now build (exit 0) and boot alone on their emulated VA/VA2 models,
accept DIR and return Ready (both exit 0). These use explicit isolated backup
paths so normal BIOS fixed-disk selection initializes; earlier `--no-bkupmem`
attempts are retained as unsuccessful fixtures, not boot passes.

**Media-source/input blockers are resolved; G100s11 PASSED.** Physical Linux
GPU/Wayland and Windows missing-MF behavior remain unverified, as explicitly
disclosed with the final checklist. Human playback/seek acceptance is not an
automated Windows decode/quality corpus or a native 16-case probe report.
CP/M/SASI checks above use macOS SDL dummy/software, not physical hardware or
new Windows execution. No release, legal/distribution clearance or next task
follows from this gate.
Full commands, image hashes, preserved failures and scope are in the
[M100s11 task](../agents/tasks/M100s11_recording_post_mp4.md).
Local evidence `build/m100s11-media-gate-16ed03ee/validation.json`, SHA-256
`23d51328d7b179ef2dab477ed2cd47c2632d26aea08f9908da9a9d246c3de1c2`.
Follow-up evidence `build/m100s11-media-gate-16ed03ee/cpm-sasi-followup.json`,
SHA-256 `3572a2f0b0550360308f52a5186bbcbd3e24991f4784ff274c90915908b001da`.
The original index is retained as historical; its CP/M/SASI blockers are resolved
by this supplement. No generated disk, private capture or license cache is
published. PSD/source-closure evidence
`build/m100s11-media-gate-16ed03ee/psd-followup.json`, SHA-256
`d47450fea9280f633e4ee629307c9fe4f8cb2066d8954be2e3815a658ade61ee`.
The exact reproducing PSD/assembler versions are recorded in the task and
[demo build guide](../../demos/zundamon-orbit/README.md); unsuccessful alternate
toolchains remain preserved. **G100s11 PASSED; normal main integration authorized.**
Preserve all previous artifacts/build roots/SDKs and user inputs below as already
requested. Older OPEN checkpoints remain historical, not current gate status.

Integrated with `--no-ff` at
[9920f86d9f776b205c5e3bd42a40a63725e8af0a](https://github.com/nakatamaho/vaeg/commit/9920f86d9f776b205c5e3bd42a40a63725e8af0a),
retaining the accepted M129 main changes. New macOS recording ON/OFF integration
builds and full CTest pass (140/131 listed, zero failures, nine declared skips
each); repository/native invariants pass. Index
`build/m100s11-integration/validation.json`, SHA-256
`b124721dc8241d77989e4984d9b15529cc9ef1c16f32744b1512ed246038295b`.
See the task for dependency-test counts, preserved runner invocation failures and
precise reuse scope. The accepted Windows artifact above remains in its original
location; no integration Windows build/runtime or unrelated export/media rerun is
inferred. Retained build roots/SDKs/evidence are exempt from cleanup as requested;
only the contained local task branches and clean temporary merge worktree are
removed after push. The primary checkout stays, on main.

## Screenshot rotation component accepted — 97259966

Maintainer-added gate: at display right 90 degrees, both screenshot menu entries
must save that orientation; turning back must restore the normal orientation.
The unprocessed/analysis screenshot paths now rotate their whole guest-resolution
composition; normal displayed readback is already oriented and is not turned
again. Native/Displayed recording and state thumbnails are unchanged.

Latest clean static recording-enabled Windows binary:

- Source [9725996637c8f52e4870a560e39a75cbc2514a02](https://github.com/nakatamaho/vaeg/commit/9725996637c8f52e4870a560e39a75cbc2514a02).
- `/Users/maho/work/pc88va/vaeg-mingw-97259966/output/vaeg.exe`, 59,218,214 bytes.
- SHA-256 `7c4930593c4b23b4a6708c2e7c3a0d8391c461429d0a8d8f29614950494de0a3`.
- `/Users/maho/work/pc88va/vaeg-mingw-97259966/output/BUILD-REPORT.md`, SHA-256
  `d63c51eabba72b32a60c111e7ff9ae79b4d6bffa0b561577c7237ac3fd5c76bb`.

The maintainer reports "OK passed" for the requested screenshot rotation check
(`画面 > スクリーンショットを保存` and its `加工前` entry, 0 -> right 90 -> 0).
This is component visual acceptance after the above Windows handoff, not an
independently collected target digest, pixel corpus or every-backend result.
Pictures must follow the screen without reversal/double turn;
normal displayed images retain their current window/letterbox canvas, whereas
unprocessed screenshot dimensions swap at 90. The screenshot component gate is
now passed. Previous Windows MP4 compression/audio acceptance remains recorded for
9c7ea061; the converter worker/source is unchanged by this screenshot correction.

Three focused SDL tests and five application profiles (macOS ON/OFF, Linux
ON/OFF/local) pass 18 saved PNG/BMP round trips per profile and a displayed SDL
quadrant/crop check. Negative tests isolate exactly one missing rotation or
transpose. Full/regression CTest results, skips, interrupted ranges and recovered
Docker exit evidence are in the task. Evidence index
`build/m100s11-evidence/screenshot-rotation/validation.json`, SHA-256
`73d5e499014cdef33f508fb5a5fbed6ee61b60de2511f4a8addcc7d7ff856c28`.
The maintainer's visual acceptance is separate from these synthetic fixtures;
no other GPU/backend, guest/private-media or overall gate acceptance is inferred.
**G100s11 OPEN.** Older artifacts and runtime reports follow; preserve them.

## Previous Windows stream-ownership correction — compression success reported

After the 9c7ea061 handoff, the maintainer reports that compression succeeds.
This confirms the reported Windows conversion failure no longer reproduces on
that retry. The maintainer additionally confirms audio playback. No independent
target-binary digest, full decode/seek or corrected sixteen-case probe result
has been provided. Those remain pending;
this is component feedback, **not a passed G100s11 gate**. No rebuild or unchanged
behavioral rerun is required for this documentation-only record.


The maintainer's preceding synthetic run reached `CLOSE_STREAM: 0x80070057` in
all sixteen cases, **after** passing the Finalize check. Releasing the writer
shuts down its owning file sink; the checked second Close incorrectly rejected
that finalized output. [9c7ea061](https://github.com/nakatamaho/vaeg/commit/9c7ea061fff736036d83fde8b12bb62c8f4fe09b)
now releases writer then byte-stream interface without re-closing. Failure/cancel
cleanup closes directly only if no writer took ownership. Genuine Finalize
failure remains fatal; no codec settings or source recordings are changed.

Updated application (not just a diagnostic):

- `/Users/maho/work/pc88va/vaeg-mingw-9c7ea061/output/vaeg.exe`, 59,212,927 bytes.
- SHA-256 `a14cc4825885064ec28972d887a0ad21b73a896e65c571dbfe8955b6e8eadd9e`.
- `/Users/maho/work/pc88va/vaeg-mingw-9c7ea061/output/BUILD-REPORT.md`, SHA-256
  `aae67bb25801fe361334372a4501ef7c6ba4b1224fae2e9a8104817964a05c84`.

Replace vaeg.exe, then retry the retained MKV through 「圧縮する録画 (.mkv)」.
Compression succeeds and audio playback is confirmed by the maintainer. The earlier `FINALIZE` report is no longer reproduced, but
its independent cause was not established; do not conflate it with the probe's
demonstrated `CLOSE_STREAM` failure.

Updated optional synthetic probe, using the same corrected source:

- `/Users/maho/work/pc88va/vaeg/build/m100s11-windows-probe-9c7ea061/mp4-windows-probe.exe`
- SHA-256 `0c142c696033bd1c0da2684c609f6d31c8063495d47c45125d805c3701fd05c4`.
- `/Users/maho/work/pc88va/vaeg/build/m100s11-windows-probe-9c7ea061/build-report.json`,
  SHA-256 `77a31d29ecdb80ac40de5297f45859dd8cc898fc09e61d997c86db5205dedc7b`.

Eight generation/lifecycle tests, strict MinGW compilation and both builds/import
audits pass. Lifecycle tests copy the actual finish method into a portable model;
a single re-close mutation produces exactly `MP4_WINDOWS_DOUBLE_CLOSE`. This is
not a native MF test. Post-correction Windows compression success is now reported;
audio playback is confirmed, while full decode/seek remains unverified. Keep earlier reports/partials and all
artifacts. **G100s11 OPEN.**

## Previous Windows finalization probe

The maintainer confirms `MP4_WINDOWS_API_FINALIZE: 0x80070057` with the
744fcbcb application. Finalize is the returning API, but may expose a delayed
encoder failure; this does not demonstrate the underlying cause. Do not suppress
the HRESULT or publish an incomplete MP4. The application is unchanged.

A separate **synthetic diagnostic**, not a replacement vaeg.exe, is available:

- Source: [cb0e2413f32cfa403d71a4af3a5e63ee9ed52ae5](https://github.com/nakatamaho/vaeg/commit/cb0e2413f32cfa403d71a4af3a5e63ee9ed52ae5).
- `/Users/maho/work/pc88va/vaeg/build/m100s11-windows-probe-cb0e2413/mp4-windows-probe.exe`
- SHA-256 `1c308ff08fe6dc3bb713e7cd17104f3bf91c1df263a8c80e369ef3be3fd9b44a`.
- Build report: `/Users/maho/work/pc88va/vaeg/build/m100s11-windows-probe-cb0e2413/build-report.json`,
  SHA-256 `c8b6b7561ed5e3a9d47829b97120b036ffaf9db8f8f0cfb6c584788e09916237`.

Copy the probe alone to a writable Windows folder (prefer a short path), then
run it. It creates an exclusive `mp4-probe-*` directory under its working folder,
retains every result and pauses for Enter when launched without arguments.
Return **report.txt**, not ROMs, original recordings or other private media.
It never reads those assets. Successful API finalization produces `.mp4`; failed
synthetic files remain `.partial.mp4` for diagnosis, never as an accepted export.

The 16 cases compare 640x400, 1278x720, 400x640 and 200x640; exact 60 Hz versus
Matroska-ms-rounded timestamps; hardware-transform preference ON versus OFF.
Each has 60 moving-gradient frames and one second of stereo 48 kHz generated
PCM. The builder extracts the production NV12 conversion verbatim and generates
two adapter copies: common read-only finalization probes plus the single hardware
preference difference and factory-symbol disambiguation. It records received,
encoded/processed counts before/after Finalize and optional post-Finalize encoder
header queries, with individual HRESULTs. Query failure never replaces the
original failure. ON is a preference, **not proof of actual hardware encoding**.
A probe PASS means finalization/close/no-replace publication succeeded, not
verified decode, quality, guest compatibility or a passed milestone gate.

Ubuntu 24.04 GCC 13-posix x64 MinGW (matching the application's toolchain family),
strict warnings/static link and system-only import audit pass. Imports are
kernel32.dll, msvcrt.dll and ole32.dll; MF loads from System32 as in production.
No FFmpeg, bundled H.264/AAC encoder, private SDK or media is included. Native
probe execution remains NOT_RUN. Six isolated generation-contract tests pass;
prior failing build attempts are retained. **G100s11 OPEN.**

## Previous Windows diagnostic — conversion E_INVALIDARG remains unresolved

The maintainer reports `MP4_WINDOWS_API: 0x80070057` with the original MKV
retained. This is a **failed runtime attempt**, not a successful Windows gate.
The current diagnostic adds per-operation error labels without changing codec
settings, sample data/timestamps or publication/cleanup behavior. Reuse the same
MKV via 「圧縮する録画 (.mkv)」 and report the new error, for example
`MP4_WINDOWS_API_FINALIZE: 0x80070057` (example, not an observed stage).

- Source: [744fcbcb83e84afb7e85f03d03e84d162fca837c](https://github.com/nakatamaho/vaeg/commit/744fcbcb83e84afb7e85f03d03e84d162fca837c).
- Executable: `/Users/maho/work/pc88va/vaeg-mingw-744fcbcb/output/vaeg.exe`
- Size: 59,212,927 bytes; SHA-256
  `d715c9ff6f99c37ad693416dfdaea08364ef9aebd4b426571c39eb47e1254a31`.
- Report: `/Users/maho/work/pc88va/vaeg-mingw-744fcbcb/output/BUILD-REPORT.md`,
  SHA-256 `4afd97d47ed770d67420494f4d08d8490488bcdcd00447799b4ac2ed91f4fd6b`.
- Strict MinGW compile (`-Wall -Wextra -Wpedantic -Werror`) and clean static
  handoff/import/ordinary-codec audits pass. OS H.264/AAC enabled, no bundled
  H.264/AAC implementation. No actual diagnostic execution is claimed yet.
- Only the Windows adapter changed; previous macOS/Linux export worker/source
  identities are unchanged. Preserve prior results rather than rerunning them.

This is **diagnosis, not a tested correction**. G100s11 remains OPEN. Historical
converter validation and the preceding Windows review candidate follow.

## Active M100s11 — optional post-recording MP4

[Usage/local-build guide](mp4-export.md),
[task, commands and limits](../agents/tasks/M100s11_recording_post_mp4.md),
[ADR-0018](../agents/DECISIONS/ADR-0018-post-recording-mp4.md).
**G100s11 OPEN: no Windows runtime or guest/media acceptance is inferred.**

Keep the original FFV1/PCM MKV and optionally create a separate H.264/AAC MP4
following successful stop/publication (including each nonempty split). One
background job/four pending paths; a bottom-right notification shows progress,
finishing, cancellation and six-second completion with an open-folder button.
Failures stay until dismissed, including the latest failed queued job. Recording
settings also retain detailed status. The start-time choice persists in INI. Exit
cancels/joins conversions; sources remain. Existing MP4/symlink targets are never
replaced. This does not enable realtime H.264/host-audio-clock recording.

- **macOS:** AVFoundation system encoding. Actual native eight-case export,
  full decode/seek, source preservation, audio/picture comparisons and an
  optimized 1921x1081 -> 1278x720 / 22.05 -> 48 kHz conversion pass.
- **Windows:** dynamically loaded System32 Media Foundation, no bundled encoder
  implementation. Strict MinGW compilation/static worker link and clean Release
  application build pass; actual Windows encoding/playback remains NOT_RUN.
- **Linux:** actual eight-case export/decode passes with the explicitly opted-in
  LOCAL_ONLY static FFmpeg libx264/native AAC build on Ubuntu 24.04 **arm64**.
  Normal builds default OFF; all CI/public presets and cached-tree builds reject
  local-only enablement. The private binary/SDK stays only in its local build
  tree, not release staging or the usual release destination.

### Windows clean-build artifact

Evaluated pushed source:
[`e6eb60aa4eaf01ce8c2566e74eea00aa0cabcdec`](https://github.com/nakatamaho/vaeg/commit/e6eb60aa4eaf01ce8c2566e74eea00aa0cabcdec).
Includes the simple 「圧縮する」 label, persistent notification, actual-writer
60 Hz timestamp correction and explicit saved-MKV retry after restarting.
Later completion documentation does not change build-relevant inputs.

- Executable: `/Users/maho/work/pc88va/vaeg-mingw-e6eb60aa/output/vaeg.exe`
- SHA-256: `a7e0889bfe1bfcf224a685c8375c4c281dfb3651cb0ca4f41973e598e0674bcd`
- Size: 59,211,391 bytes.
- Report: `/Users/maho/work/pc88va/vaeg-mingw-e6eb60aa/output/BUILD-REPORT.md`
- Fresh clone/application objects, pinned container, recording ON, static desktop
  dependencies/minimal LGPL FFmpeg, Windows-system-only imports. Completed shader
  dependency SDK reused read-only/audited. LOCAL_ONLY encoders OFF; FFmpeg encoder
  registrations checked before linking, not inferred from missing codec DLLs.

macOS development application: `build/m100s11-mac-on/sdl2/vaeg` (arm64, Debug,
shared host dependencies, not a portable release). Linux LOCAL_ONLY application:
`build/m100s11-linux-local/sdl2/vaeg` (arm64, private validation only).
No private-codec artifact is published or copied to a handoff/release directory.

Current native-timing/retry evidence: `build/m100s11-evidence/video-layout/validation.json`,
SHA-256 `e45575d2746f074223c3a6eb46c1c67ad4aab9480c9495fde6d46965ebbfce0c`.
Actual vaeg-writer 60 Hz export and two precise timestamp negatives pass on
macOS/Linux, in addition to the existing eight-case export profiles. Original
MKVs can be retried via 「圧縮する録画 (.mkv)」 with an unquoted full path.
The old `MP4_VIDEO_LAYOUT` error was reproduced on a valid native recording;
adjacent PTS now determine frame intervals without false overlap rejection.

Previous notification/cleanup evidence: `build/m100s11-evidence/notification/validation.json`,
SHA-256 `55de933a21069c353a1f503dd3dd0a83f2f410cddaec940f0e2d66443d69aaf0`.
Initial converter evidence remains at `build/m100s11-evidence/final/validation.json`,
SHA-256 `685d53bd7230bc34cf537a01e600528e000960597580cdef40f76ffc5dc50cdb`.
The follow-up records explicit-exit GUI selftests, a complete eight-case export
per backend and 64 macOS cancellations after correcting exception/pool lifetime.
Earlier timeout and failed/incomplete cleanup attempts remain preserved.
Recording ON/OFF suites, policy/static-resolution fixtures and repository
invariants pass. Whole-repository format check retains only demonstrated
baseline deviations, with zero new violations. See the task for full selftest
exit/timeout distinctions; do not turn a client timeout into an observed exit 0.

Before acceptance: actual Windows normal/missing-MF conversion and playback,
physical platform checks, V3 boot/demo/OS operations and all mandatory private
utility/SASI/demo/media regeneration remain to be done. No legal/distribution
approval or M100s11 merge is claimed. Earlier accepted handoffs follow.

## Active M100s10 — Windows/MinGW and Linux Displayed archive

Maintainer-requested continuation after G100s9:
[M100s10 task and validation](../agents/tasks/M100s10_recording_displayed_windows_linux.md).
Client Displayed FFV1/PCM is implemented for Windows D3D11/common SDL and Linux
native OpenGL/common SDL. Linux now composes the real GUI and reads its final
back buffer before swap; Windows reuses bounded staging. No second shader pass,
Native substitution, decorated OS capture or realtime H.264 is added.

Evaluated Release/build source:
[ce62cacd](https://github.com/nakatamaho/vaeg/commit/ce62cacd32885f5980c98d264c6e857b8c9c2795),
branch `m100s10-displayed-windows-linux`. Later evidence-only documentation does
not change the application inputs.

- **Windows x64:** `/Users/maho/work/pc88va/vaeg/build/handoff/m100s10/windows-x64/vaeg.exe`,
  SHA-256 `22959a47da4fcf2cd8dd896c87ad41bb3e53d9725f81c67ba396782ad8b4d6a8`.
  Report `/Users/maho/work/pc88va/vaeg/build/handoff/m100s10/windows-x64/BUILD-REPORT.md`.
  Clean pushed clone, Release, recording ON, fully static desktop dependencies
  and Windows-system-only imports. Completed audited shader SDK reused read-only,
  no application objects reused. Verified retained copy/report:
  `build/handoff/m100s10/windows-x64/`.
- **Linux arm64:** `/Users/maho/work/pc88va/vaeg/build/handoff/m100s10/linux-arm64/vaeg`,
  SHA-256 `063af9827e8584339ffbee4fc64dd49d792c4425de914a52cb2339c58295a307`.
  Report in that directory: `BUILD-REPORT.md`. Ubuntu 24.04 Release, recording ON;
  system SDL2/LibArchive/FFmpeg dependencies, not x86-64 or a static distribution.
  Actual native GL GUI startup and four-frame Displayed FFV1/PCM save/decode pass
  with intentionally empty ROMs, Xvfb/Mesa software GL and dummy audio.

Linux/macOS recording ON/OFF suites pass; Linux full selftests, real GL exact
capture/pack-state/error/off tests, focused ASan/UBSan and Linux/MinGW strict
compiles pass. Linux FFmpeg 6 AVIO signature and tiny multi-slice image failures
were corrected without relaxing exact decoded RGB/alpha assertions. Detailed
commands, failed-attempt logs, worker/output identities and limits are retained
in the task and `build/m100s10-evidence/validation.json`.

**G100s10 PASSED by explicit maintainer acceptance** (`m100s10-displayed-windows-linux
gate passed`) of the ce62cacd/37338696 handoff. No new platform checklist or
machine evidence was supplied. Earlier open-gate statements are historical;
acceptance does not convert unverified checks into machine passes.
Native Windows execution/playback, physical Linux GPU,
shader-runtime/Wayland coverage, guest boot/demo/OS and required private media/
PSD/DOSBox regeneration are NOT_RUN/unverified, not inferred from builds or
synthetic startup. No release/legal/distribution approval or next milestone is
implied. Previous G100s9 acceptance and its historical handoffs remain below.

The accepted M100s10 branch is main-integrated at
[25bbca7d](https://github.com/nakatamaho/vaeg/commit/25bbca7daa096610c59c8152cdd037feb65d6ae3).
This also preserves a concurrent main CPU/selftest change; the accepted ce62cacd
binaries are not a fresh build of that new main. Before scoped cleanup, complete
SDK/FFmpeg/evidence outputs were retained under `build/handoff/m100s10/preserved/`
with a verified path/digest map in `preservation.json`. Historical reports retain
their original paths; current Windows handoff paths above point to verified copies.

## Active M100s9 follow-up

[M100s9](../agents/tasks/M100s9_recording_presenter_readback.md) is now accepted:
the maintainer explicitly said `gate passed` after the a4c0b6a0/03faab3c handoff.
This covers Native/Displayed archive recording and the saved GUI source choice.
No platform-specific checklist or new local media evidence was supplied; preserve
NOT_RUN/BLOCKED machine results rather than converting acceptance into test logs.
Gate closure includes a no-ff main merge and scoped cleanup; no next milestone
implementation is started here.
The maintainer requested macOS actual Displayed output first, independently
selectable from Native, with FFV1 and hardware H.264 as separate compression
choices; Windows/MinGW and supported Linux devices follow. Branch publication is
authorized; the historical LOCAL_ONLY text below is not a current push ban.
G100s9 acceptance authorizes the normal main integration and scoped cleanup.
[Gate record](https://github.com/nakatamaho/vaeg/commit/1325121eb0ffa107a9eacfc19615292788673d7a)
was followed by the
[no-ff main merge](https://github.com/nakatamaho/vaeg/commit/4b4870281afed01e723834e5f36075f68635340f).
The merge tree equals the accepted gate-record tree; changes since evaluated
a4c0b6a0 are documentation-only, so completed behavioral/dependency runs are reused.
Accepted and earlier handoff outputs, the complete audited expensive shader SDK,
FFmpeg dependency stage and exact logs are retained under
`build/handoff/m100s9-accepted/` with verified copies in `preservation.json`.
Historical reports/evidence indexes retain their original build-tree paths; the
preservation manifest maps those paths to the retained outputs. The current
Windows executable below is a byte-identical retained copy, not a new main build.
The three dedicated M100s9 static MinGW build trees and temporary merge worktree/
local topic branches are cleaned only after the gate/merge/roadmap push and clean/
origin-main containment checks. The primary checkout, unrelated local main commit
and M100r2 worktree, remote topic, shared/VM caches, evidence directories and private
scratch of unestablished provenance are kept; no supplied assets are removed.
Realtime H.264/Media Foundation, other platforms/devices and legal/distribution
approval remain separate. Earlier evidence/open-gate statements below are historical.

**Displayed FFV1 implementation and the G100s9 human acceptance are complete.**
Current evaluated source is
[a4c0b6a0](https://github.com/nakatamaho/vaeg/commit/a4c0b6a02fdd616071044c39d49c1fe6c1ccb162),
which persists the GUI Native/Displayed preference without automatic recording.
Both production config reloads and two Release restarts/CLI override isolation
pass, alongside ON/OFF/full-selftest/focused-sanitizer and four changed-TU strict
checks. Unavailable saved Displayed choices stay selected/unavailable; --no-cfg
and CLI preference isolation retain their existing policies. Its main integration is
[bc0198b9](https://github.com/nakatamaho/vaeg/commit/bc0198b9dbdd919d1046bd94081812affd18194c),
importing main [f25cda38](https://github.com/nakatamaho/vaeg/commit/f25cda385965d765441db9d6498fd40beae0a285)
at the maintainer's request. This preserves M125 quick saves/loads and recording
state-load cancellation. Original feature source [73948eaa](https://github.com/nakatamaho/vaeg/commit/73948eaa511d3a97896155a1d05af864e818b535)
includes owned final-client acquisition, guest-time PCM integration, fixed source,
metadata, lifecycle/error handling and enabled GUI/CLI selection. Native keeps
its existing auto-split path; Displayed stops/saves on orderly geometry/reset.
No substitute guest image, second CRT evaluation or fixed output canvas is added.
The independently verified Metal font-atlas fix is adopted at
[863c61f2](https://github.com/nakatamaho/vaeg/commit/863c61f205af46fcda8814c975d09e239b470eca).

Local checks: ON 37 PASS + two opt-in SKIP / OFF 29 PASS, focused ASan/UBSan 4 PASS,
ten strict compile variants PASS. Explicit actual Metal synthetic GUI/font,
filtered/unfiltered final readback and exact FFV1/opaque-alpha/PCM comparison PASS.
The real Debug and static Release frontends also save Native/Displayed four-frame
movies with frameskip 4 using intentionally empty ROMs. This is public frontend
startup/timing evidence, not a commercial ROM, guest OS/demo or audio-device PASS.

Current artifacts (evaluated preference fix a4c0b6a0; applications rebuilt/retested):

- macOS: `/Users/maho/vaeg/build/macos-recording-static-release/sdl2/vaeg`,
  SHA-256 `b401e3953869b8f8e451d95578f063b8f4b694211c8f1ac3eb81b17840ca97e6`;
  report `/Users/maho/vaeg/build/m100s9-source-pref/macos-build-report.md`.
  Release/arm64/recording ON, static desktop dependencies and Apple-only dynamic
  imports; existing build directory, not a new-clone macOS artifact. Dependency
  macOS-version/alignment warnings mean older-macOS runtime remains unverified.
- Windows cross-build: `/Users/maho/vaeg/build/handoff/m100s9-accepted/a4c0b6a0/output/vaeg.exe`,
  SHA-256 `b3595e5c02c558502fe18c91021db461b537717cafa02f40c6f7214598ff3e4d`;
  report `/Users/maho/vaeg/build/handoff/m100s9-accepted/a4c0b6a0/output/BUILD-REPORT.md`.
  Clean pushed-source clone, recording ON, static dependencies/runtimes, embedded
  commit/date and Windows-system-only imports verified. The new helper exits 0;
  only the independently validated unchanged shader SDK is reused by an explicit
  task-local adapter of the temporary inner script, never application objects.
  The initial 73948eaa helper/SDK-staging failure logs remain historical evidence.
  The target-aware auditor correction is
  [1bbf8cad](https://github.com/nakatamaho/vaeg/commit/1bbf8cadf925caabb0d822379e51695f8998a115).
  This is not native Windows runtime/playback proof.

Current preference evidence/worker/source/contract/command/output identities:
`build/m100s9-source-pref/validation.json`, SHA-256
`a1c4ad1dcc452efb98094bdb1515f6632c2c19481e0ff9e475bf179ccd89615b`.
ON 37 PASS + 2 SKIP / OFF 29 PASS, full ON/OFF selftests, 4 focused sanitizer
checks, 4 changed-TU strict compiles and two Metal Release restarts/CLI isolation
PASS. Clean MinGW OFF helper and fresh ON application build/import audits exit 0.

Earlier main-refresh evidence and inherited-warning details:
`build/m100s9-main-refresh/validation.json`, SHA-256
`269650d4099ff8904b6018d6c00e0ce7f62202b8094b36c692e2042cedd71f42`.
Full Debug ON/OFF selftests pass with isolated TMPDIRs, including quick saves;
Release full selftest also passes. An initial parallel run raced on the legacy
fixed dropmedia temp directory; its failed logs remain. Recording-focused strict
checks pass; an additional kbdmap strict check exposes a pre-existing selftest
cleanup warning in byte-identical main code, not a newly claimed Werror PASS.
Historical `build/m100s9-completion/` evidence and paired Metal workers remain
retained without unnecessary baseline rebuilds; that baseline aborts
with the missing-texture GetTexID assertion, corrected worker exits 0. The prior
accepted macOS executable and OFF MinGW artifact are also retained. Dedicated
checkout-local m100s9 Colima/Docker storage is stopped, not removed; unrelated
environments/assets/branches were not deleted.

G100s9 is closed by explicit maintainer acceptance. The previously blocked local
system-media/PSD/DOSBox clean-clone regeneration checks have not been rerun; no
image hashes or V3/demo/OS/playback execution details are invented. Empty-ROM and
synthetic tests remain frontend evidence only. Release/distribution is not approved.
Realtime H.264 stays disabled: M100s3/M100s4 still require actual host playback/audio
mapping, not the archive FIFO. Windows Media Foundation, Linux devices and older
macOS remain separate work; no follow-on implementation starts in this closure.

## Current OS-assisted continuation

The integrated recording extension now continues under the maintainer's
recording-only sequential/LOCAL_ONLY authorization. Current source identities,
implemented host schedule, VideoToolbox H.264/PCM/MKV and ScreenCaptureKit
checkpoints, executable evidence and exact next steps are in
[os-assisted-status.md](os-assisted-status.md) and
[os-assisted-plan.md](os-assisted-plan.md). Real-time live A/V and platform
validation are unfinished. The REC history below is preserved; its former
next-task blocker is not a blanket stop on independent OS-assisted work.

Current state: REC15 is implemented with its deterministic machine checks, but
real SDL renderer runtime evidence is pending. Completed tasks: REC00/M100r1,
explicitly accepted by the maintainer on 2026-09-19, REC01/M100r2,
REC02/M100r3, REC03/M100r4, REC04/M100r5, REC05/M100r6, REC06/M100r7 and
REC07/M100r8, REC08/M100r9, REC09/M100r10, REC10/M100r11, REC11/M100r12,
REC12/M100r13, REC13/M100r14, REC14/M100r15 and REC15/M100r16. Assigned branch:
topic/m100r3-recording-clock.

## REC15 completion

* `sdl2/recording/recording_displayed_capture.c` validates the fixed SDL
  RGBA32/top-down descriptor and normalizes the borrowed readback to owned
  RGB24 bytes.
* `sdl2/scrnmng.c` reads the actual SDL renderer drawable immediately before
  `SDL_RenderPresent`, after GUI and enabled guest-area OSD composition. The
  capture has one bounded reusable buffer, a renderer generation and a
  `drawcount` token; unchanged tokens do not create duplicate UI-only frames.
  Geometry, presenter, dummy-driver, readback, callback and allocation failures
  are explicit and fail closed.
* Existing guest-only screenshots and QA captures remain on their original
  paths. REC16 still owns connecting this seam to the Displayed recording
  session and MKV writer.

## REC15 evidence

Evaluated source commit: `0d804a59` (`M100r16: isolate SDL readback selftest`),
following `84c1b85a` (`M100r16: add Displayed SDL readback seam`).
The recording-enabled and disabled frontend builds passed. The focused
contract test passed in both builds, under Werror and under ASan/UBSan. The
affected ON/OFF recording regression sets passed 8/8 and 7/7. The ROM-less
dummy SDL selftest passed, including the labelled final-client fixture with
guest, menu, OSD and status regions, exact normalized-byte comparison, token
suppression, geometry stop and dummy rejection.

The isolated Displayed probe also could not complete: Cocoa window setup lost
the host UI-service connection. This is not treated as real-renderer evidence;
REC15 remains `IMPLEMENTED_UNVERIFIED`. The Werror full frontend is still
affected by the unrelated existing `io/np2sysp.c` unused `str_pentium`
warning. No Displayed MKV, Windows/Linux presenter, GPU backend, human or
release validation is claimed.

Under the authorized progression exception, the next implementation candidate
is REC16/M100r17, but its REC15 dependency is not technically ready until a
usable real SDL renderer runtime is available.

## REC14 completion

* `--record FILE.mkv` now starts the same Native session used by the GUI;
  `--record-source native|displayed` has exactly the two specified source
  values, Native is the default, and `--record-frames N` requires a positive
  count. Invalid extensions, existing targets and unavailable sources fail
  before output creation.
* The GUI Recording menu and dialog use the exact `Native video` and
  `Displayed video` labels. Displayed is visibly disabled with its current
  unavailability reason; no third source or OSD recording filter was added.
* Stop Recording is available from the active menu, including host-only redraws
  while the guest is paused. The UI reports guest elapsed time, accepted frame
  intervals and finalizing/success/error state. Output validation and the
  session controller remain outside the C core and use the existing bounded
  Native A/V session.

## REC14 evidence

Evaluated source commit: `ec966b9b61d487014aabd8cc3184bc3ef0c55a06` (`M100r15: implement Native recording controls`).

The macOS recording-enabled suite passed 15/15 and the disabled suite passed
10/10. Both frontend builds passed, both ROM-less selftests and smoke runs
passed, and the CLI parser, invalid/off-build and unavailable-Displayed paths
were exercised. The focused session fixture covers invalid extension, existing
target, Unicode path, double start, finite interval count, complete manual
stop and Displayed-source rejection. Repository case, encoding, EOL,
whitespace and milestone-ID checks passed.

No real guest OS/media demonstration with enabled OSD and sound, Displayed
final-client capture, Windows/Linux runtime, VLC playback, or maintainer human
acceptance is claimed. REC14 remains `IMPLEMENTED_UNVERIFIED` with its human
gate pending. REC15/M100r16 has since been implemented, but its real SDL
renderer evidence is unavailable; REC16/M100r17 is not dependency-ready.

## REC13 completion

* The private Native session controller starts from a maintainer-only
  environment hook, obtains the canonical Native RGB24 descriptor and the
  current indexed audio FIFO, and derives one guest-clock/sample epoch.
* Each guest display boundary is copied into a recording-owned staged frame
  with its REC08 OSD snapshot. A later boundary closes the interval, selects
  the exact audio sample range, and submits one owned complete slice through a
  bounded cancellable queue. No callback timestamp or host frame pacing is
  used.
* The worker writes FFV1 v3 BGR0 video and PCM S16LE stereo to a seekable MKV
  partial file and publishes only after trailer/flush success. Video PTS is
  the slice begin tick and duration is the full guest interval; this is
  required because the VA guest clock is much finer than a millisecond.
* Reset, state-load, audio epoch/clock changes, descriptor invalidation, queue
  failure and normal/error shutdown stop or cancel the session. Existing
  screenshots, QA captures, Displayed video and recording-OFF paths remain
  unchanged.

## REC13 evidence

Evaluated source commits:
`7b530d79` (`M100r14: connect Native recording session`) and
`595d6ed2d575dcc3b3d36aa5dc5c554d4c4f8b79`
(`M100r14: align Native video interval timestamps`).

The macOS recording-enabled suite passed 14/14 and the disabled suite passed
9/9. The focused Werror and ASan/UBSan queue/session tests passed 2/2 each;
both frontend builds and ROM-less selftest/smoke runs passed. A private
ROM-only dummy-device run produced and decoded four Native intervals as one
FFV1/BGR0 640x400 stream plus one PCM S16LE stereo 22050Hz stream. No private
input identity is recorded here.

The live run is engineering source-integration evidence only. It did not
include a guest OS/media demonstration with motion, sound and enabled OSD;
Displayed video, physical audio, VLC, Windows/Linux presenters and the human
gate remain pending. VLC was unavailable on the evaluated host.

REC13 is implemented at its machine gate. Under the current authorized
recording-v1 progression exception, REC14/M100r15 may proceed without treating
this pending human gate as approval or release validation.

## REC12 completion

* The `screendispva` completed guest-display event now marks the Native tap
  boundary. The tap requests canonical composition even when normal host draw
  skipping would omit it; `pccore_redraw` is explicitly not a timed boundary.
* Each accepted capture carries the owned RGB24 Native frame, matching REC08
  OSD snapshot, `drawcount` sequence, exact extended guest-clock interval and
  canonical descriptor generation. The tap rejects descriptor, sequence, time,
  frame and callback failures closed.
* Existing screenshot/QA and presentation paths remain unchanged. REC13 still
  owns the integrated audio/common-epoch coordinator, MKV writer connection,
  lifecycle and public Start/Stop control.

## REC12 evidence

Evaluated source commit:
415ca5f114b27990daa04ce157f4f27616ac98b2.

On macOS arm64 with AppleClang 21.0.0.21000101, CMake 3.31.12, Ninja 1.13.2
and MacPorts SDL2 2.32.10, the Native tap passed focused ON/OFF, Werror and
ASan/UBSan tests. The enabled recording suite passed 13/13 and the disabled
suite passed 9/9. Both frontend builds and ROM-less no-audio selftests passed.
The changed production C units compiled with generated Werror commands. No
live MKV, public UI, Displayed/GPU backend, Windows/Linux runtime, VLC, human
or release validation is claimed.

REC12 is complete at its machine gate. REC13/M100r14 is now implemented at its
machine gate; see the REC13 section above for the remaining validation limits.

## REC11 completion

* Recording-active `sound_sync` is now the sole synthesis producer. It emits
  one guest-time S32 block through the REC10 seam, converts it to final S16
  stereo and appends it to an owned bounded FIFO with sample indices and epoch.
* The SDL callback consumes a separate playback buffer only while recording;
  it does not synthesize, encode, wait for recording capacity or allocate.
  Mute and missing-device operation preserve the recording timeline as counted
  silence.
* Recording arming reconciles already-generated pending playback samples into
  the recording FIFO. Reset and clock changes invalidate the retained session
  with an epoch error. A full recording FIFO fails closed rather than dropping
  a claimed recording sample or blocking under the core/audio lock.
* `SOUNDMNGSPEC` records the requested and obtained SDL device description for
  the later recorder owner. The recorder coordinator and MKV connection remain
  future work.

## REC11 evidence

Evaluated source commit:
4165d9fda677ac0a9ff3ed954c0100d0a988dcfc.

On macOS arm64 with AppleClang 21.0.0.21000101, CMake 3.31.12, Ninja 1.13.2
and MacPorts SDL2 2.32.10, the focused FIFO test passed in ON, OFF, Werror and
ASan/UBSan builds. The enabled recording suite passed 12/12 and the disabled
suite passed 8/8. Both frontend builds and the ROM-less no-audio selftest
passed. The changed production C units also compiled with the generated Werror
commands. The complete Werror frontend target remains affected by unrelated
pre-existing warnings in `io/np2sysp.c` and `sound/vermouth/midinst.c`.
Repository validators passed for the evidence update. No live MKV, presenter,
Windows/Linux runtime, physical audio, VLC, human or release validation is
claimed.

REC11 is complete at its machine gate. REC12/M100r13 is now complete at its
machine gate; the next dependency-ready task is REC13/M100r14.

## REC10 completion

* The legacy stereo S32 generator dispatch, reserved-tail look-ahead
  calculation and S16/silence packing now share small C99 operations without
  changing callback order, generated lengths, saturation, chip state or SDL
  callback byte accounting.
* `SOUNDSTREAMSTATE` observes rate, buffer/reserve/remain/pending counts,
  cumulative generated samples, reset epoch, `lastclock`, lock state and
  checked accounting overflow. It is an observation for a later handover, not
  a recording cursor reset or a callback timestamp.
* Existing guest-time `sound_sync` and callback look-ahead are intentionally
  still both present. REC11 must reconcile already-generated and reserved
  data, pending PCM and lock ownership before adding one recording producer.

## REC10 evidence

Evaluated source commit:
5e2823a437b4082b5b41f5f7d89e7a1073e738bd.

On macOS arm64 with AppleClang 21.0.0.21000101, CMake 3.31.12, Ninja 1.13.2
and MacPorts SDL2 2.32.10, the focused generator test passed in ON, OFF,
Werror and ASan/UBSan builds. The enabled recording suite passed 11/11 and the
disabled suite passed 7/7. Both frontend builds and ROM-less selftest/smoke
passed, and the disabled build contained no FFmpeg references. Repository
case, UTF-8, EOL, whitespace and milestone-ID validators passed. No live
recording, host-device-format handover, Windows/Linux runtime, VLC, human or
release validation is claimed.

REC10 is complete at its machine gate. REC11/M100r12 is now complete at its
machine gate; the next dependency-ready task is REC12/M100r13.

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

REC09 is the native composition checkpoint only. REC10/M100r11,
REC11/M100r12 and REC12/M100r13 are now complete at their machine gates; the
next dependency-ready task is REC13/M100r14.

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
