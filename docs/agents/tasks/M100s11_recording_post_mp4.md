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

# M100s11 — post-recording MP4 export

## Assignment and gate

The maintainer requested background post-stop compression on macOS, Linux and
Windows, preferring OS encoding on macOS/Windows. G100s10 was explicitly accepted
and integrated before this task. This is the single new implementation milestone.
**G100s11 OPEN / HUMAN GATE.** No runtime success follows from a cross-build.

## Contract

- Keep the finalized guest-time FFV1/PCM MKV unchanged. Conversion is optional,
  never substitutes realtime recording, and runs after successful publication,
  including each completed Native split segment.
- Background H.264/AAC MP4, bounded worker/queue and conversion buffers, progress,
  success/failure/cancellation reporting; cancel outstanding conversions on exit.
- Preserve source timestamps (not wall time), full picture/aspect and audio.
  Even video dimensions, up to 1280x720, stereo 48 kHz AAC, no alpha in the upload.
- macOS AVFoundation/VideoToolbox and Windows Media Foundation encode through OS
  APIs; do not bundle their encoder implementations. Prefer those backends.
- Linux software encoding is an explicitly opted-in local-only static FFmpeg
  build using libx264 and native AAC. Default OFF; never build/publish it in CI or
  releases. Respect `AGENTS.md` local-codec policy, including license review.
- Final `.mp4` beside the MKV. Existing output (including symlinks) is an error,
  not an implicit overwrite. Work only inside an exclusively created temporary
  directory, publish without replacement, then clean up only this job's generated
  temporary file/directory. No guest media, user asset or original MKV is deleted.
- Guard default-OFF, explicit local enablement, known CI environment rejection,
  public/release preset rejection and static FFmpeg encoder allow-list. Static
  codec archives are inspected; lack of DLL imports is not sufficient evidence.

## Validation required before gate

- Focused passing fixtures and single-mutation negative tests with stable codes
  for export, no-overwrite, cancellation, missing backend and build policy.
- macOS native actual MP4 export/decode and MKV digest preservation; Linux private
  static export/decode plus normal build/export-OFF behavior; Windows strict
  MinGW compilation/clean static handoff and native Windows runtime human check.
- Recording ON/OFF regressions and applicable frontend selftests; repository
  encoding/EOL/case/Markdown/whitespace checks.
- Standard clean pushed-commit static MinGW handoff with executable/report paths
  and SHA-256. Demo/media/private-input gates remain required; mark unavailable
  checks NOT_RUN rather than inferring success from synthetic fixtures.
- No hosted CI iteration. Preserve task-specific logs, fixtures and expensive
  local dependency builds outside Git; document evaluated commit and worker
  identity. Do not publish private or codec-enabled binary/dependency artifacts.

## Status

Implementation and local synthetic validation complete; **G100s11 remains OPEN**.
[User/build guide](../../recording-v1/mp4-export.md),
[architecture/license decision](../DECISIONS/ADR-0018-post-recording-mp4.md),
[platform handoff](../../recording-v1/handoff.md).

Evaluated initial converter source is
[`67cf5889a4161fd9a3cde61fc290bfb37f7d23bc`](https://github.com/nakatamaho/vaeg/commit/67cf5889a4161fd9a3cde61fc290bfb37f7d23bc).
Completion records through `a899eddc` are documentation-only. Subsequent
maintainer-requested GUI/cleanup work is recorded below. Initial source/worker
identities and commands are retained in `build/m100s11-evidence/final/validation.json`, SHA-256
`685d53bd7230bc34cf537a01e600528e000960597580cdef40f76ffc5dc50cdb`.
The collector asserts that the evaluated code files still match Git and both
export workers still match their completed eight-case manifests. Raw logs,
fixtures, dependency builds and intermediate failures stay outside Git.

### Implementation delivered

- One in-process worker, four pending paths; progress/error/cancel UI and joined
  application shutdown. No external shell/encoder process, source replacement,
  shader replay or new host-audio-clock mode.
- OS-first AVFoundation and dynamically loaded System32 Media Foundation sinks.
  Linux libx264/native AAC is explicit, static and LOCAL_ONLY, never a release.
- Successful nonempty Native publication callback, including each completed
  split. Callback exceptions cannot turn a successful archive into a failure.
  Start-time option snapshot survives GUI recreation; INI preference persists.
- Exclusive sibling temporary directory (POSIX mode 0700), finalized output,
  no-replacement publication, and cleanup limited to this job's own artifacts.
- Default-OFF/ACK/static/Linux-only/CI/public guards, including a build-time
  dependency before compilation and archive inspection. A configured local tree
  cannot be imported into CI by skipping CMake configuration. Focused temporary
  interface-target tests prove that both CI and public-preset mutations prevent
  the command marker from running; no encoder implementation is built in those
  policy fixtures. Target OS, not the CMake script host OS, controls eligibility.

### Commands and results

All paths below are task-local; generated images/movies/binaries are untracked.
The Linux environment is Ubuntu 24.04 **arm64**, not an x86-64 runtime result.
Colima/daemon storage is under `build/cr10`, with its explicit project socket.

| Check | Command/profile | Result |
|---|---|---|
| Build policy | `python3 tests/frontend/recording/test_mp4_policy.py` | exit 0; 20 tests, including controlled configure/build-only failures |
| Static resolver | `python3 tests/frontend/recording/test_static_ffmpeg.py` | exit 0; 7 tests, exact `.a`, no shared fallback and ordinary x264 refusal |
| macOS recording ON | `cmake --build build/m100s11-mac-on -j8`; `ctest --test-dir build/m100s11-mac-on -L recording --output-on-failure` | both exit 0; CTest 27/0 failed |
| macOS recording OFF | same commands for `build/m100s11-mac-off` | both exit 0; CTest 18/0 failed |
| Linux normal ON/OFF | same commands for `build/m100s11-linux-on` / `-off` inside the local container | exit 0; CTest 26 / 18, 0 failed |
| Linux LOCAL_ONLY Release | same commands for `build/m100s11-linux-local` | exit 0; CTest 26, 0 failed; assertions actually enabled |
| Native macOS export | `python3 tests/frontend/recording/test_mp4_export.py --worker build/m100s11-mac-on/vaeg_recording_mp4_test --work /tmp/m100s11/mac-export-final-8` | exit 0; 8 cases |
| Linux static local export | same script, `--worker build/m100s11-linux-local/vaeg_recording_mp4_test --work /evidence/linux-export-final-8` in the container | exit 0; 8 cases |
| Windows compile/link | common/sink/test worker, MinGW C++17 `-Wall -Wextra -Wpedantic -Werror`, static `-municode` link | exit 0; system-only PE import audit exit 0 |
| Clean Windows handoff | `tools/release/build-mingw-static-container.sh --commit 67cf5889a4161fd9a3cde61fc290bfb37f7d23bc --recording --shader-sdk "$PWD/build/handoff/m100s10/preserved/dependencies/mingw-static"` | exit 0; fresh application objects, read-only audited shader SDK |
| Repository invariants | `check_encoding.py`, `check_eol.py`, `check_case.py`, `check_md_comments.py`, `git diff --check` | exit 0; encoding/EOL 0 findings |

Linux's pre-existing H.264 hardware probe is explicitly SKIPPED; the default-off
GL displayed fixture is DISABLED, not a new physical GL pass. Native MP4 tests
are explicit opt-in executions, not silently skipped codec claims in CI.

Each eight-case profile covers actual H.264/yuv420p + AAC/48 kHz/stereo output,
180 decoded video frames, time bounds, full decode/seek, source digest retention,
existing output, dangling symlink, cancellation, publication race, a relative
`file:` filename with an invalid unprefixed neighbor, wrong codec and invalid
header. Negative cases start with the proven passing archive/backend, mutate
one invariant, require the exact error code, and leave no owned temporary file.
The relative-name case is positive regression coverage, not a weakened rejection.

- macOS worker SHA-256:
  `edc136ca4b515ac9a171002e5b5bba34754e8ba8c07065d9824c4445b93ac454`.
  Fixture SHA-256 `c7f3fec588efd85799985fe20ae4940c2724fbe64af5921395ec6fe923f09980`.
  Audio SNR 41.79 dB; first-frame RGB RMSE 16.49 (lossy 4:2:0, threshold <20).
- Linux worker SHA-256:
  `2333f2a6ce7798bd7356ed7281a1d5cb416f96c271380d0725e707340a2b79ab`.
  Fixture SHA-256 `b32d30791bfe8127e098fe694660148c6f1d9382fdcf7fa8f5156530e36fb409`.
  Audio SNR 35.30 dB; RGB RMSE 15.35. FFmpeg 6's fixture uses VFW/FourCC;
  FFmpeg 8's uses native CodecID. Both mutate the actual codec field, not payload.
- Optimized macOS probe additionally converted 1921x1081 / 22.05 kHz input to
  1278x720 H.264 / 48 kHz stereo AAC, 180 frames and 3.000 s. Export/full decode
  exit 0; source stayed SHA-256
  `88fb76e050584a08eea90efa61211308519e51308d24e07c3878c47f2e62f1c2`.

The earlier ON/OFF macOS/Linux full `--selftest --no-cfg --no-bkupmem` matrix
returned exit 0. Final macOS ON/OFF full selftests also returned 0. Final Linux
ON/OFF/local logs each end `selftest: all tests passed`; the host client wrapper
hit its 100-second timeout while its daemon-side loop continued, so that wrapper
did not retain individual final child exit codes. Do not rewrite the client
TIMEOUT as an observed per-child exit 0. Retained complete logs and prior
explicit-exit results are in the evidence tree.

### Corrections and preserved intermediate results

- Release recording fixtures used `assert` for API calls as well as checks.
  `-DNDEBUG` elided sends/flushes, causing false passes or an EOF-loop spin.
  [`772c4d59`](https://github.com/nakatamaho/vaeg/commit/772c4d59b7343c3ab0c8f5df5a7d6c57ebbe0f16)
  forces assertions for recording test targets, adds a compile-time guard and
  timeout. The single-flag negative fixture requires
  `RECORDING_TEST_ASSERTIONS_REQUIRED`. This is test-harness correction, not a
  claimed guest/codec fix. The old spinning process was terminated; logs remain.
- Initial Linux full export reached five passing cases/quality checks, then its
  fixture generator assumed a newer CodecID layout. Corrected VFW/FourCC and
  variable `biSize` handling in
  [`ffbc0d7b`](https://github.com/nakatamaho/vaeg/commit/ffbc0d7bbab0a733e284c47d6c6c6fd9dac99f50).
  Completed cases were first reused with an unchanged worker/source, and only
  corrected cases executed. After the later real path-reader correction changed
  worker identity, both platforms legitimately ran the complete final 8 cases.
- The demonstrated relative filesystem-name/FFmpeg URL defect is fixed in the
  evaluated source and [permanent ledger](../../modernization/bug-fixes.md).
- Whole-manifest `python3 tools/repo/clang_format.py` exits 1 on inherited
  deviations in 24 files. Baseline comparison against `b50635b7` proves **zero new
  violations**; no unrelated line was reformatted. All seven new C/C++/ObjC++
  sources/headers pass clang-format 22. The checker now recognizes `.mm`.
- Preserve initial `../vaeg-mingw-65d5cdf8/` and final
  `../vaeg-mingw-67cf5889/`, all logs/SDKs and task work. The former is superseded,
  not the gate binary. No private-codec executable/SDK was copied to release
  staging, CI artifacts, public downloads or the usual release destination.

### Clean Windows artifact and remaining human gate

- Executable: `/Users/maho/work/pc88va/vaeg-mingw-67cf5889/output/vaeg.exe`
- Size: 59,201,085 bytes; SHA-256
  `d5bdfd0081b3763c0f7beff2d20c291027df4f04f86c9362374c4034a3df0507`.
- Report: `/Users/maho/work/pc88va/vaeg-mingw-67cf5889/output/BUILD-REPORT.md`,
  SHA-256 `8ed8cd4883a12d916f6165768fcbbe9e46a7f5d026bb96ec739c5a2e6d3b073e`.
- Release/recording ON, public policy, LOCAL_ONLY encoders OFF. Minimal LGPL
  FFmpeg encoder archive and PE imports audited; H.264/AAC implementation comes
  from Windows only at runtime. Compiler/linker checks are **not Windows runtime**.

**NOT_RUN / no acceptance inferred:** native Windows conversion/playback, Windows
missing-MF behavior, physical Linux GPU/Wayland, guest V3/demo/OS boot, MANDEL.BAS,
all demo distribution/payload comparisons and private-input utility FDD/SASI/
PSD/media regeneration from the clean clone. These media checks require the
maintainer's configured private inputs and still block the standard gate. No
image checksum is invented for an unperformed media build. Run each required
media command, record its actual exit/image digest and resolve any failure before
calling G100s11 passed. No hosted CI was requested or used as an iterative debugger.
No new legal/distribution approval, milestone merge or subsequent task is claimed.

### Maintainer-requested macOS hardware-encoder observation

On macOS 26.6.2 arm64, observed the actual Release export worker's VideoToolbox
session using a task-local dyld diagnostic shim, without changing encoder
settings or production sources. Queried `VTSessionCopyProperty` both after
creation and after the first successful `VTCompressionSessionEncodeFrame`:

- `kVTCompressionPropertyKey_UsingHardwareAcceleratedVideoEncoder`: **true**,
  query status 0.
- `kVTCompressionPropertyKey_EncoderID`:
  **`com.apple.videotoolbox.videoencoder.ave.avc`**, query status 0.
- Command: `DYLD_INSERT_LIBRARIES=/tmp/m100s11/vt-observe-frame.dylib
  build/m100s11-mac-release-probe/vaeg_recording_mp4_test --export
  /tmp/m100s11/hardware-observe-frame/fixture.mkv`; export exit 0, complete
  `ffmpeg -v error -xerror -i .../fixture.mp4 -f null -` decode exit 0.
- Worker SHA-256 `1fac9578c0be14182d4305a3f2caef89b379f01b0097de774fe5347fc2887252`;
  diagnostic shim `700961e5fde60dfbae5da15008bad9dbd3b9be55898519b2d991e363707475d6`;
  output `8f507c2e9bf49662940e54794c66af1877e916098aaf8b7013bf63c574c138ba`.
  Source stayed `88fb76e050584a08eea90efa61211308519e51308d24e07c3878c47f2e62f1c2`.

This confirms Apple's hardware AVC encoder for this particular conversion,
not Metal shader execution or a guarantee for every device/resolution/OS. AAC's
hardware implementation was not queried. The original diagnostic attempt using
`RTLD_NEXT` failed in the injected shim (exit 138); direct dyld interposition
corrected that diagnostic without modifying the application. Failed and
successful logs/shims remain under `/tmp/m100s11/`; no other expensive profile
was rerun and no gate/legal approval changes.

### Persistent compression notification follow-up

Approved after the maintainer requested visible background activity. Implemented
in [b60b2992](https://github.com/nakatamaho/vaeg/commit/b60b29925e68a9972fd144ddb5f541da7c0e7452):

- Bottom-right nonmodal panel, independent of recording settings/menu visibility.
  「圧縮中…」 percentage/bar, unknown-duration animation, queued count and cancel;
  「仕上げ中…」 reflects actual sink finalization, not an estimated 100 percent.
- Completion lasts six seconds (extended while hovered), with 「保存先を開く」.
  SDL opens a percent-encoded local directory URL, never a constructed shell
  command. POSIX/drive/UNC, spaces, UTF-8 and URL metacharacters are covered.
- Failure/rejection is sticky until closed, with optional details; cancellation
  is a short notice. Job/rejection identities distinguish repeated identical
  requests. Durable last-failure telemetry survives immediate next-job startup.
  UI lifecycle state survives GUI/renderer recreation and never steals focus on
  appearance. Window dimensions stay within the viewport with scroll fallback.
- Source MKV, no-overwrite behavior, codec settings, timing and Linux isolation
  remain unchanged. No new milestone or human acceptance is implied.

Final evaluated code:
[6de8b9085f83ddbbfe55dbe731ac27d465fda2df](https://github.com/nakatamaho/vaeg/commit/6de8b9085f83ddbbfe55dbe731ac27d465fda2df).
Evidence `build/m100s11-evidence/notification/validation.json`, SHA-256
`55de933a21069c353a1f503dd3dd0a83f2f410cddaec940f0e2d66443d69aaf0`.

| Follow-up command/profile | Observed result |
|---|---|
| `cmake --build build/m100s11-mac-on -j8`; `ctest --test-dir build/m100s11-mac-on -L recording --output-on-failure` | exits 0; 28 tests, 0 failed |
| same commands for `build/m100s11-mac-off` | exits 0; 19 tests, 0 failed |
| same commands for Linux normal ON/OFF/local in the task container (`-j4`) | exits 0; 26/18/26, 0 failed; existing skip/disabled qualifications unchanged |
| macOS ON/OFF and Linux LOCAL_ONLY full `--selftest --no-cfg --no-bkupmem` with dummy SDL/audio | explicit exits 0 at b60b2992; new lifecycle/layout test and all selftests pass |
| lifecycle/layout fixtures | timeout/no-resurrection/repeated job+notice/durable queued failure; seven states at 640x400, 320x240 and 320x480, scales 1/2 |
| `test_mp4_export.py`, final macOS worker, `/tmp/m100s11/notification-export-mac-pool-fix` | exit 0; all 8 actual export/negative cases |
| same export script, Linux local worker, `/evidence/notification-export-linux` | exit 0; all 8; final worker digest unchanged after Apple-only follow-up |
| `python3 tests/frontend/recording/test_mp4_policy.py`; `test_static_ffmpeg.py` | exits 0; 20 / 7 tests |
| new ARC normal/exception drain test, ON and OFF | exits 0; mutation only to rethrow inside pool fails exactly `MAC_POOL_EXCEPTION_RELEASE` |
| 64 focused real macOS cancellations on the same fixture | expected exit 1 / `MP4_CANCELLED`, source unchanged, no owned temporary directories |
| encoding/EOL/case/Markdown comments/whitespace; new sources' strict format | exits 0; whole format manifest still exits 1 only on inherited baseline, added-line GUI comparison has zero new findings |

The GUI selftests are synthetic ImGui layout/state checks, not proof of physical
GPU rendering or native Windows button interaction. Final actual export workers:
macOS `6dc974016c67232e8047a4fc3c305f242dc85955da81e3617782e28c2989e90c`,
Linux `4367126e4f3afca4926061e6f291e8ec9446ddbb7026d9dd6b825c75ffbae1e7`.
Linux completed b60b2992 results are reused only after verifying the final build
has that identical worker and unchanged platform-relevant code/configuration.
No unaffected expensive corpus/profile was rerun for report changes.

The first macOS run exposed a real cancellation cleanup defect: the validator
correctly rejected a retained empty job directory. A diagnostic captured errno
66 and native safe-save files. One-second retries did not fix it; releasing
members alone passed 32 larger-fixture cancels but still failed the original
full-profile cancel. Those attempts are preserved, not labeled successful fixes.
A weak-reference control then proved that C++ unwinding skips an autorelease
pool's drain. The final helper catches inside, drains normally and rethrows;
its single-mutation test and the full/focused exports above pass.
[Permanent correctness record](../../modernization/bug-fixes.md). No recursive
cleanup, file-protection bypass or relaxed validator assertion was introduced.

Updated clean Windows handoff (fresh clone, source 6de8b908, OS MP4 encoding ON):

- `/Users/maho/work/pc88va/vaeg-mingw-6de8b908/output/vaeg.exe`, 59,210,873 bytes.
- SHA-256 `71fc5afc7664a1b5e2ceda3b7eff0b94ac701e055d269635633cb988496142cf`.
- `/Users/maho/work/pc88va/vaeg-mingw-6de8b908/output/BUILD-REPORT.md`, SHA-256
  `c1a274cea9acf1e3ce363189fed16a378f538daba3ce7b0c7d4652e464567f00`.
- `tools/release/build-mingw-static-container.sh --commit
  6de8b9085f83ddbbfe55dbe731ac27d465fda2df --recording --shader-sdk
  "$PWD/build/handoff/m100s10/preserved/dependencies/mingw-static"`: exit 0.
  Static desktop dependencies, public/minimal FFmpeg archive registration audit
  and Windows-system-only PE imports pass; no bundled H.264/AAC implementation.

Retain all old binaries/build roots/evidence, including the simple-label
`d8f3ea8a` handoff; none is deleted or relabeled. This supersedes their UI, not
historical runtime evidence. **G100s11 remains OPEN**: native Windows execution,
physical platform, guest boot/demo/OS and all required private-media gates are
still NOT_RUN; no release/legal/distribution approval is inferred.

### Reported MP4_VIDEO_LAYOUT on native recordings

The maintainer's error is reproduced with a public fixture through vaeg's own
FFV1/PCM/Matroska path, without accessing or publishing the maintainer's clip.
Missing initial frame durations plus 1 ms timestamp quantization caused a false
comparison of 33 ms with an estimated previous end of 33.667 ms. The old worker
(SHA `1fac9578c0be14182d4305a3f2caef89b379f01b0097de774fe5347fc2887252`)
rejects that valid 640x400/60 Hz recording. This is not a demonstrated pixel-format
or damaged-media problem. [Permanent ledger](../../modernization/bug-fixes.md).

[3473d157](https://github.com/nakatamaho/vaeg/commit/3473d15759057ff2974cca74c6f015ef71003f0f)
uses one-frame lookahead and adjacent actual PTS to determine video duration,
retaining bounded memory and strict increasing timestamps. Only the final frame
uses a duration hint/nominal rate. True duplicate/backward timestamps are rejected
with `MP4_VIDEO_TIMESTAMP`, not accepted with an arbitrary tolerance.

The native timing regression uses the actual archive encoders/writer, not FFmpeg
CLI-generated input. Baseline: 60 frames, full decode, original PTS within 1 ms,
H.264/640x400 plus AAC/48 kHz/stereo, unchanged input. Two separate mutations alter
only the third video timestamp (duplicate/backward), updating the cluster CRC;
both require precisely `MP4_VIDEO_TIMESTAMP`, source retention, no MP4 or job temp.
Command per backend (exit 0, three cases):

```sh
python3 tests/frontend/recording/test_mp4_native_timing.py \
  --writer build/m100s11-mac-on/vaeg_recording_matroska_test \
  --worker build/m100s11-mac-on/vaeg_recording_mp4_test \
  --work /tmp/m100s11/video-layout/mac-native-final
# Linux local container: linux-local targets and /evidence/video-layout/linux-native-final.
```

Both original eight-case export profiles also pass with the new workers.
Recording CTest ON/OFF: macOS 28/19, Linux 26/18/local 26, exit 0/0 failed, prior
skip/disabled qualifications unchanged. Final macOS ON/OFF and Linux local full
selftests with dummy SDL/audio explicitly exit 0, including the new retry-field
layout. The policy/static resolver 20/7 results are reused: their source, CMake
policy, dependency identity and comparison contracts did not change.

[e6eb60aa](https://github.com/nakatamaho/vaeg/commit/e6eb60aa4eaf01ce8c2566e74eea00aa0cabcdec)
also allows an explicit existing MKV path in recording settings, so replacing the
application/restarting no longer prevents retrying a previously saved recording.
The field is separate from new-recording output, never starts/overwrites a
recording and uses the same protected background converter. Empty means this
process's last successful recording. This GUI-only change leaves the completed
conversion worker digests unchanged:

- macOS: `c2d9f43cad5e21ec88ebff8686590ecdc1f37648675eadec4965f81b6bc55e49`.
- Linux: `8adcb443c4e7cbdd9b106dda29bca25e90a292162d7585044ff133c3c5c3f560`.

Final source is e6eb60aa; evidence `build/m100s11-evidence/video-layout/validation.json`,
SHA-256 `e45575d2746f074223c3a6eb46c1c67ad4aab9480c9495fde6d46965ebbfce0c`.
Encoding/EOL/case/Markdown/whitespace checks pass; source/changed-line formatting
has no new findings. Whole-manifest inherited formatting exceptions are unchanged.

Updated clean Windows executable:
`/Users/maho/work/pc88va/vaeg-mingw-e6eb60aa/output/vaeg.exe`, 59,211,391 bytes,
SHA-256 `a7e0889bfe1bfcf224a685c8375c4c281dfb3651cb0ca4f41973e598e0674bcd`.
Report `/Users/maho/work/pc88va/vaeg-mingw-e6eb60aa/output/BUILD-REPORT.md`, SHA-256
`2db57eda465cf7244d74f2c01379739a36b680c5dc79763761ecf1a281eb93aa`.
The usual clean static helper with `--commit e6eb60aa4eaf01ce8c2566e74eea00aa0cabcdec
--recording --shader-sdk "$PWD/build/handoff/m100s10/preserved/dependencies/mingw-static"`
exits 0; minimal codec-registration/system-DLL import audits pass. Windows uses
OS encoders, not bundled static H.264/AAC. Keep the intermediate 3473d157 build
and all previous artifacts. Actual Windows/user-clip playback, private-media
and hardware acceptance remain unverified: **G100s11 OPEN**.

### Windows E_INVALIDARG report — diagnostic handoff, not a claimed fix

The maintainer confirms the original MKV exists and reports
`MP4_WINDOWS_API: 0x80070057`. This is an observed Windows conversion failure;
successful Windows export/playback remains unverified. The HRESULT alone does
not establish which call failed. Do not infer missing codecs, bad dimensions,
invalid timestamps, finalization or redundant close as the root cause.

[744fcbcb](https://github.com/nakatamaho/vaeg/commit/744fcbcb83e84afb7e85f03d03e84d162fca837c)
adds operation-specific HRESULT diagnostics: COM/startup, file/sink creation,
video/audio output/input negotiation, video/audio sample creation/writes,
`BEGIN_WRITING`, `FINALIZE` and `CLOSE_STREAM`. It changes no encoding settings,
sample contents/timing, file protection or cleanup behavior. The next required
check is to retry the same retained MKV and obtain its exact new error code.
[Unresolved ledger entry](../../modernization/bug-fixes.md).

Local checks: `x86_64-w64-mingw32-g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror
-Isdl2 -c sdl2/recording/recording_mp4_windows.cpp -o
/tmp/m100s11/windows-api-diagnostic/recording_mp4_windows.o`: exit 0. Source
clang-format 22, encoding/EOL/case/Markdown comments and whitespace: exit 0.
Clean helper `tools/release/build-mingw-static-container.sh --commit
744fcbcb83e84afb7e85f03d03e84d162fca837c --recording --shader-sdk
"$PWD/build/handoff/m100s10/preserved/dependencies/mingw-static"`: exit 0;
ordinary codec-registration and Windows-system-only import audits pass.

- `/Users/maho/work/pc88va/vaeg-mingw-744fcbcb/output/vaeg.exe`, 59,212,927 bytes,
  SHA-256 `d715c9ff6f99c37ad693416dfdaea08364ef9aebd4b426571c39eb47e1254a31`.
- `/Users/maho/work/pc88va/vaeg-mingw-744fcbcb/output/BUILD-REPORT.md`, SHA-256
  `4afd97d47ed770d67420494f4d08d8490488bcdcd00447799b4ac2ed91f4fd6b`.

Only `recording_mp4_windows.cpp` changed among build-relevant paths since the
preceding candidate. macOS/Linux workers and their compiled inputs still match
the completed eight/three-case evidence; no unaffected expensive profile was
rerun. No native execution of the diagnostic or correction is claimed. No
private media/name/hash was requested or published. Screenshot-rotation work
was paused without source edits to prioritize this failure. **G100s11 OPEN.**

### Finalize identified; source-derived native synthetic probe

The maintainer retried 744fcbcb and confirms exactly
`MP4_WINDOWS_API_FINALIZE: 0x80070057`. This excludes the subsequent close as
that returning call; it does not identify the underlying encoder/muxer cause.
WriteSample returning success does not establish that all queued samples encoded
successfully. The Microsoft [Finalize contract](https://learn.microsoft.com/en-us/windows/win32/api/mfreadwrite/nf-mfreadwrite-imfsinkwriter-finalize)
already includes end-of-segment markers; no missing-marker hypothesis, ignored
HRESULT, arbitrary codec change or claimed correction was introduced.

[cb0e2413](https://github.com/nakatamaho/vaeg/commit/cb0e2413f32cfa403d71a4af3a5e63ee9ed52ae5)
adds a separate synthetic Windows probe, not a vaeg replacement. The production
adapter and converter are unchanged. A pure generator extracts the production
NV12 converter verbatim and creates adapter copies with read-only pre/post
Finalize statistics and optional post-Finalize encoder-header queries. The
copies differ only in hardware-transform preference (plus symbol disambiguation
at compilation). Query failure cannot replace/suppress the original HRESULT.

Six focused content/generation tests pass: baseline exact variant delta and
pixel-function identity; separate single mutations remove/duplicate Finalize,
remove hardware preference, remove the pixel function and remove its boundary.
Each negative starts from a passing fixture and asserts the precise stable
error code. No Git history, compiler or large corpus can mask these checks.
Final x64 Ubuntu 24.04/GCC 13-posix compilation uses strict warning errors and
static runtimes, matching the application's compiler family. Commands/exit 0:

```sh
python3 tests/frontend/recording/test_mp4_windows_probe_build.py
# Existing task-local Ubuntu container, repository mounted at /work:
docker exec -w /work m100s11-linux python3 tools/release/build-mp4-windows-probe.py \
  --output-dir /work/build/m100s11-windows-probe-cb0e2413
docker exec -w /work m100s11-linux python3 tools/release/check-static-windows-imports.py \
  --binary build/m100s11-windows-probe-cb0e2413/mp4-windows-probe.exe
```

Import audit: kernel32.dll, msvcrt.dll, ole32.dll only; System32 MF dynamically
loads as in production. No FFmpeg, private SDK or bundled H.264/AAC implementation
is linked. Format/encoding/EOL/case/Markdown/whitespace checks pass. Initial
Python 3.9 write_text newline-argument and missing mfidl include failures remain
preserved in precommit output directories; the corrected tool compiles locally
and in Ubuntu. No unchanged emulator behavior profile was rerun.

Handoff executable:
`/Users/maho/work/pc88va/vaeg/build/m100s11-windows-probe-cb0e2413/mp4-windows-probe.exe`,
SHA-256 `1c308ff08fe6dc3bb713e7cd17104f3bf91c1df263a8c80e369ef3be3fd9b44a`.
Report: `/Users/maho/work/pc88va/vaeg/build/m100s11-windows-probe-cb0e2413/build-report.json`,
SHA-256 `c8b6b7561ed5e3a9d47829b97120b036ffaf9db8f8f0cfb6c584788e09916237`.
Build report records evaluated commit, input/generated-source hashes, compiler,
exact commands/exits and executable digest. This standalone probe does not need
or read ROMs, existing MKVs, private media or a GPU/window frontend. It writes
only into a newly exclusive directory and retains synthetic failed partials;
production failed-output cleanup is unchanged.

Native execution pending: sixteen combinations of 640x400/1278x720/400x640/
200x640, exact 60 Hz/Matroska-rounded PTS and hardware-transform ON/OFF. Each
contains 60 generated frames/48,000 stereo PCM sample frames. Request report.txt,
not a private clip. Hardware ON does not prove actual hardware use, and a PASS
only means API finalization/close/no-replace publication, not decode/quality or
milestone acceptance. Original recording remains available for a future tested
correction. App 744fcbcb still fails the reported conversion; **G100s11 OPEN**.

### Native probe reached redundant Close; ownership correction

The maintainer returns the probe console: all sixteen cases report exactly
`MP4_WINDOWS_API_CLOSE_STREAM: 0x80070057`, for both hardware preferences,
both timing profiles and all four geometries. In this control flow Finalize has
passed its error check; failure occurs only after writer release at explicit
byte-stream Close. This is a different observed stage from the earlier
application `FINALIZE` report. The report.txt statistics/headers, target-binary
hash and post-correction native run have not yet been supplied; do not invent
them, infer actual hardware encoding or claim MP4 decode/quality acceptance.
No private clip/path/identity is recorded here.

[9c7ea061](https://github.com/nakatamaho/vaeg/commit/9c7ea061fff736036d83fde8b12bb62c8f4fe09b)
corrects the demonstrated ownership mistake. Releasing the Sink Writer shuts
down its media sink/file stream; the checked subsequent Close rejected that
already-finalized output. Finish now checks Finalize, releases writer then
stream interface. Exceptional cleanup closes directly only when writer creation
never took ownership. Actual Finalize errors remain fatal, with the same source
preservation, exclusive temporary ownership and no-replacement publication.
No video/audio settings or timestamps changed.

Eight tests pass. In addition to the six generator contracts, portable
source-derived lifecycle tests copy the real finish method into an ownership
model and verify successful release order, genuine Finalize failure propagation
and cancellation before work. A passing baseline receives exactly one inserted
checked Close after writer release; it must fail specifically with
`MP4_WINDOWS_DOUBLE_CLOSE`, never an unrelated failure. Models/temporary binaries
and logs remain under `/tmp/m100s11-win-lifecycle-*`; these tests are not a
native MF substitute. Commands all exit 0:

```sh
python3 tests/frontend/recording/test_mp4_windows_probe_build.py
x86_64-w64-mingw32-g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -Isdl2 \
  -c sdl2/recording/recording_mp4_windows.cpp \
  -o /tmp/m100s11/windows-api-diagnostic/close-fix.o
tools/release/build-mingw-static-container.sh \
  --commit 9c7ea061fff736036d83fde8b12bb62c8f4fe09b --recording \
  --shader-sdk "$PWD/build/handoff/m100s10/preserved/dependencies/mingw-static"
docker exec -w /work m100s11-linux python3 tools/release/build-mp4-windows-probe.py \
  --output-dir /work/build/m100s11-windows-probe-9c7ea061
```

Clean static application and probe import audits pass; normal FFmpeg encoder
registration audit remains minimal. New Windows artifacts:

- `/Users/maho/work/pc88va/vaeg-mingw-9c7ea061/output/vaeg.exe`, 59,212,927 bytes,
  SHA-256 `a14cc4825885064ec28972d887a0ad21b73a896e65c571dbfe8955b6e8eadd9e`.
- `/Users/maho/work/pc88va/vaeg-mingw-9c7ea061/output/BUILD-REPORT.md`, SHA-256
  `aae67bb25801fe361334372a4501ef7c6ba4b1224fae2e9a8104817964a05c84`.
- `/Users/maho/work/pc88va/vaeg/build/m100s11-windows-probe-9c7ea061/mp4-windows-probe.exe`,
  SHA-256 `0c142c696033bd1c0da2684c609f6d31c8063495d47c45125d805c3701fd05c4`.
- `/Users/maho/work/pc88va/vaeg/build/m100s11-windows-probe-9c7ea061/build-report.json`,
  SHA-256 `77a31d29ecdb80ac40de5297f45859dd8cc898fc09e61d997c86db5205dedc7b`.

Only the Windows adapter changes production build inputs. macOS/Linux workers,
configuration contracts and completed behavioral evidence remain unchanged;
no unaffected expensive run or hosted CI was requested. Source format and
encoding/EOL/case/Markdown/whitespace checks pass. Preserve all prior binaries,
console evidence, partials and report.txt. Retry the retained MKV with the new
application and optionally rerun the new probe. Publication/playback/native
correction verification and the earlier Finalize report remain open, as do
private-media and other human gates: **G100s11 OPEN**.

### Maintainer feedback — corrected Windows compression succeeds

After the 9c7ea061 application handoff, the maintainer reports that compression
now succeeds. Record this as native Windows conversion success feedback, not
as a full milestone gate pass. Target-binary hash, output hash/frame counts,
full decode/seek, audio playback and the corrected sixteen-case probe result
have not been supplied. No private media/metadata was requested or committed.
The demonstrated redundant-close defect is recorded among corrected bugs in
the [permanent ledger](../../modernization/bug-fixes.md). The earlier `FINALIZE`
report no longer reproduces on this retry, but no independent cause or separate
Finalize/codec correction is asserted.

This update is documentation-only; application/probe artifacts and source remain
9c7ea061, with their previously recorded digests. Reuse existing machine evidence
rather than rebuild or rerun unchanged behavior. No hosted CI, merge, cleanup,
release/distribution approval or next milestone is authorized by this component
feedback. Playback and all other outstanding gates remain: **G100s11 OPEN**.
