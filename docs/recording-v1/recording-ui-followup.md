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
# Native FFV1 recording controls and tint follow-up

## Current viewport-fit settings and color-packing comparison

The maintainer confirms the RGB-tagged public fixture still shows the same
three colors in VLC. **RGB metadata did not resolve the tint.** No new channel,
matrix/gamma correction or production pixel-format change is made here.

[Settings-layout fix](https://github.com/nakatamaho/vaeg/commit/ee5ebc8d1c4b5efbbf2489639803f1b0b3abf8ee):
open centered at 52x38 font units, bounded to 95 percent of the viewport on each
appearance (overriding a remembered tiny size). Descriptions wrap; field labels
sit above full-width inputs. Settings/status scroll in a child while start/close
buttons remain in the footer. ROM-less regression passes 1/1 with five
viewport/font profiles, long status, a seeded 128x72 window and scroll-end/
footer-bounds assertions. ON/OFF builds and recording/header tests pass 22/22
and 14/14. These are layout assertions, not a native Windows visual gate.

Current working-tree test executable: `output/vaeg-audio-local-test.exe` (also
`output/vaeg.exe`), 58,843,761 bytes, SHA256
`a5c2565cb3405dd2500838af36cdfb210dd2fa99e1cef095855197ed1c1453f7`.
Base `ee5ebc8d1c4b5efbbf2489639803f1b0b3abf8ee` plus the same inherited audio
patch `0925091601467304336586c3bdf66b3d7cc08328afd5b9797edc190c7c1a2386`;
not a clean-commit build. All three inherited files remain byte-identical and
uncommitted; Metal is excluded. Windows build/import audit passes. Previous
snapshot archived as `output/vaeg-audio-local-8a7ca19a.exe`.

Controlled, video-only developer fixtures use the same macOS FFmpeg8.1.2 CLI,
source RGB bytes, dimensions 640x400, 180 frames/60Hz, FFV1 v3/GOP1/slice-CRC,
threads1, full-range RGB metadata and muxer. Only requested packed pixel format
differs; these are **not production Windows encoder/muxer object fixtures**:

- `output/public-vlc-packing-bgr0.mkv`, SHA256
  `db3188f491d657c3bf20ba5e15b2a82113f4e1b062763904fbdfd14520abb27e`:
  independently decoded RGB24 matches all 180 source frames exactly.
- `output/public-vlc-packing-bgra.mkv`, SHA256
  `b811f4f267684d2b0cb44dc429b7ecfadaa2ca03b5ca88815dd3b3801891f3f6`:
  independently decoded BGRA matches all 180 expected frames exactly,
  including alpha255 for every pixel. Both files report the intended format.
- Worker/source/corpus/contract/target-policy identities, exact commands, exit
  statuses and output digests are retained in
  `output/public-vlc-packing-validation.json`. Reference bars are white, yellow,
  cyan, green, magenta, red, blue, black. Compare **both** in the affected VLC;
  a good BGRA file alone would not establish a packing cause if BGR0 also works.
- VLC display status is **NOT_RUN locally**. A separate native C libVLC callback
  probe, avoiding Python and using lazy binding, also times out loading the
  installed library, before any decoded frame. No consent/security/driver
  setting was changed. No affected-player color claim follows from this probe.

## Previous REC/toggle, RGB headers and local audio snapshot

The maintainer reports only yellow/red/green in VLC, including white appearing
yellow, and inability to start the previous Windows build with the reported
`audio_clock_origin_invalid` error. The VLC tint remains OPEN; there is no
private-source or native Windows reproduction here.

- [REC/toggle implementation](https://github.com/nakatamaho/vaeg/commit/3778086325f3d4c2b644720efdd5245b7e19b8d2):
  one checked `録画開始／停止` menu item, disabled during finalization; REC replaces
  RECORDING, with the indicator another 12 scale-units left of the lock lamps.
- [RGB-header correction](https://github.com/nakatamaho/vaeg/commit/8a7ca19ab8fb0adb2a6740b4a327bedfc1b1b9a7):
  FFV1 Matroska codec parameters explicitly declare full-range RGB and 8-bit
  components. Previously these properties were absent. This corrects a proven
  metadata omission, **not a proven root cause of the reported VLC tint**.
  No matrix/gamma/channel or pixel-value correction was applied; H.264 remains
  limited-range BT.709.
- Current **working-tree test build**, not a clean-commit build:
  `../vaeg-mingw-recording-74c3355d/output/vaeg-audio-local-8a7ca19a.exe`,
  58,840,570 bytes, SHA256
  `bbecff82bdccf0c5ad73d6a6177b79cfb2b0167ef6f98c9248c5919ee6ca7bae`.
- Evaluated base: `8a7ca19ab8fb0adb2a6740b4a327bedfc1b1b9a7`, plus the
  **unchanged, uncommitted** inherited native-session implementation/test files.
  Patch SHA256: `0925091601467304336586c3bdf66b3d7cc08328afd5b9797edc190c7c1a2386`.
  All three inherited files remain byte-identical; none was committed. The Metal
  change is excluded. Base SHA alone does **not** identify this binary's source.
- A focused Windows fixture advances the clock while capture is armed. The
  committed implementation reproduces exactly `audio_clock_origin_invalid`;
  the local snapshot successfully arms and completes generated-sample intervals.
  Both assertions pass under Wine. This supports the snapshot fix but does not
  establish the user's actual guest/runtime cause. Workers, source patch,
  commands, logs and digest manifest `audio-origin-comparison.json` are retained
  outside Git in the task build directory. Full Wine Unicode-path tests remain
  failing; focused success is not full-suite or native Windows validation.
- Public file `output/public-ffv1-rgb-tagged.mkv`, SHA256
  `56406010c206855f29a2b10436f655d71da0bef10c3da31fa61c0d3c067bdc5d`, is
  generated by the new Windows Release encoder/muxer objects. All 360 encoded
  video/PCM packet payload hashes and timestamps equal the previous public
  fixture. Independent decoding matches all 180 original RGB frames exactly;
  independent stream inspection now reports `pc`, `gbr`, 8-bit. VLC confirmation
  is pending. The macOS libVLC callback probe timed out loading the library,
  before any decoded frame; it provides no player-color evidence.
- macOS ON/OFF builds pass; recording/header tests pass **22/22**, **14/14**;
  ROM-less panel test **1/1** passes with the new lamp position. Windows Release
  build/import audit and Debug test compilation pass. Completed static FFmpeg
  and shader dependencies are reused. No hosted CI, push, private asset access
  or clean-checkout/human-gate claim.

## Historical clean df965588 Windows artifact

- Source: `df9655883275f662216dc8febe1b0da5f3189928`.
- Executable: `../vaeg-mingw-recording-74c3355d/output/vaeg-df965588.exe`.
- SHA256: `13bd765fa218ea4e8fe6c32a1ce382dbf6f802ea1bb8fae6ef46eaa9c4a14e9e`.
- Size: 58,839,792 bytes. Static MinGW x86-64 Release, recording ON, application
  tests OFF. Same pinned minimal FFmpeg8.1.2, Ubuntu24.04 image and shader
  archive as the [initial FFV1-enabled build](mingw-ffv1-build.md).
- Completed dependencies were reused; no FFmpeg/Rust rebuild, hosted CI, push,
  private integration asset access, save-state change or main reset.
- The source clone remained clean. The original three inherited dirty files
  remained byte-identical and were excluded from this executable.
- The previous executable is retained as `output/vaeg-d63955c0.exe`, SHA256
  `3ff27618c74735a7fb4da15bc36a3601e0499bbfa320554040e06ed81b58a723`.

## Requested controls

[Implementation](https://github.com/nakatamaho/vaeg/commit/df9655883275f662216dc8febe1b0da5f3189928):

- **録画開始** starts immediately using current settings; **録画停止** requests
  finalization. Start/stop/status/settings text is Japanese; canonical source
  labels remain Native video / Displayed video. Displayed is still unavailable.
- Default output uses the existing executable-relative app-directory helper to
  create `recordings/`. An unwritable executable directory falls back to the
  user directory, as for screenshots. The settings dialog explains this.
- Each start selects
  `vaeg-recording-YYYYMMDD-HHMMSS-<10-digit uptime ms>-<3-digit counter>.mkv`.
  Existing names are skipped, and final exclusive/non-replacing I/O remains in
  force. Manual paths remain available; no existing recording is overwritten.
- **詳細設定** exposes auto/manual output, Native source, OSD inclusion,
  following screen rotation and optional positive frame-count limit. Settings
  are session-local and disabled while recording; no fake bitrate/quality or
  unsupported codec option is offered. FFV1 remains lossless RGB with PCM.
- Clockwise rotation physically turns RGB pixels, swaps coded dimensions/stride
  and descriptor generation, then composites upright OSD in output coordinates.
  An asymmetric six-color test verifies orientation, four-turn identity, aspect
  swap, frame-token preservation and invalid-input behavior.
- Stream dimensions cannot change mid-file. Changing screen rotation while
  following it finalizes the current recording; restart records the new
  orientation. It does not silently reinterpret old packet geometry.
- **RECORDING** is left of KANA/CAPS: red while active, amber during finalization,
  dim while idle. Existing locks remain right-aligned. Mode lamps are omitted
  in narrow portrait bars to prevent overlap; drive/recording/lock lamps remain.
  The software-renderer selftest checks the new idle lamp's position/color.

## Movie-only red tint: unresolved

The maintainer confirms normal live display but red-tinted saved video. No
private movie/screenshot was inspected. There is no demonstrated root cause and
**no speculative color matrix/channel swap/gamma correction** in this build.
The issue remains in the [open ledger](../modernization/bug-fixes.md#m100s2--saved-ffv1-video-has-a-red-cast-on-windows).

[Additional regression coverage](https://github.com/nakatamaho/vaeg/commit/1c2d859e32dee6b72327b351886aabd2a65dffd8)
encodes and decodes all 65,536 RGB565 values with padded rows. RGB primary/random
fixtures and this exhaustive test pass on macOS and in the Windows test binary
under Wine. They do not exercise a private guest or a specific affected player.

A public 640x400, 180-frame/3-second color-bar movie was generated by a Windows
helper linked to the **exact Release encoder/PCM/Matroska object files** of this
application. It contains no guest or private pixels. Filename:
`output/public-ffv1-color-bars-release.mkv`, SHA256
`6aa33d405adf65adcb755f068e041b307d1cb73f893b48d7ea1e8079d55cc74f`.
Independent macOS FFmpeg decoding matched every byte of all 180 RGB24 frames to
the generated source, plus all 144,000 stereo PCM sample frames to silence.
Thus this public Release encoding/muxing path preserves colors exactly; it does
not establish the private capture source or affected player's interpretation.
Ask for player identity and whether this public pattern also appears tinted.

## Machine checks and limitations

- macOS recording ON/OFF builds: exit0; recording/header CTest: 22/22 and 14/14.
- macOS ROM-less/selftest CTest: 1/1, including software front-panel layout.
- MinGW Release application build/link and unchanged static import audit: exit0.
  No extra FFmpeg DLL is required; only allowed Windows system imports.
- Windows Debug test executables rebuilt from the evaluated source. Six focused
  Wine9 tests passed: C/C++ headers, exhaustive FFV1, native RGB composition/
  rotation, capability probe and PCM. Exact commands, worker/output SHA256 and
  exits are in local `output/ui-followup-wine-results.json`.
- Wine was repaired only in the task-owned prefix using unmodified installed
  Wine builtin DLLs; no host system/driver/toolchain replacement. The separate
  Matroska Unicode-directory test still fails under that incomplete prefix
  before encoding. Its failure remains preserved; no complete Windows-suite
  or native Windows GUI/media/guest-boot pass is claimed.
- Public generation source/helper commands, expected RGB, verifier outputs and
  original/resumed build logs remain outside Git in the task work directory.
- Encoding/EOL/case and diff whitespace checks passed. Only modified ranges
  were format-adjusted; unrelated pre-existing manifest differences remain.
- LOCAL_ONLY. Human Windows UI/rotation/recording validation is pending; red tint
  is OPEN. Static FFmpeg distribution/relinking review is still outstanding.
