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

# Windows/macOS static FFmpeg and librashader defaults

Maintainer request: static FFmpeg by default on Windows/macOS, and static
librashader by default on macOS as well as Windows. Implementation:
[86535d23e8641d6f67f4e5f3683f23d69c622812](https://github.com/nakatamaho/vaeg/commit/86535d23e8641d6f67f4e5f3683f23d69c622812).

`VAEG_STATIC_FFMPEG` and `VAEG_STATIC_LIBRASHADER` default ON for Windows/Apple.
The FFmpeg resolver substitutes exact .a archive paths for avformat/avcodec/
avutil in pkg-config's ordered static closure, including Apple Framework pairs.
It fails closed if an archive is missing even when a shared library is present.
Other native link flags keep their order. Existing librashader archive/pin/
notice audit is retained. Default recording OFF does not discover/link FFmpeg.
Existing CMake caches require explicit ON when they retain older OFF values.
Linux defaults are unchanged; an explicit developer dynamic opt-out exists.

Current dependency policy below is unchanged. The later [main-synchronized
handoff](recording-ui-followup.md#current-main-synchronized-recording-candidate) supersedes the
artifact hashes in this packaging checkpoint, without changing static defaults.

## SDL2/LibArchive static-default checkpoint

[0455d0c94e7018fdf2db4f4574b0e98088f658e0](https://github.com/nakatamaho/vaeg/commit/0455d0c94e7018fdf2db4f4574b0e98088f658e0)
extends desktop defaults to fetched static SDL2 and the fetched static
LibArchive/zlib/xz stack on macOS. The legacy-named macos-macports and macos-asan
presets now explicitly enable them too, updating older cached OFF values when
using those presets. Existing explicit developer opt-outs remain possible.
No shared SDL/archive fallback on the static path. Linux defaults unchanged;
Windows already used this same static dependency path and stays unchanged.

Pinned SDL2 2.32.10, LibArchive3.8.7, zlib1.3.1 and xz5.8.3 sources are reused
from the existing checksum-pinned FetchContent cache, copied into task-local
build sources, not re-downloaded or hand-edited. A content manifest confirms
all4202 copied files remain identical after configuring/building. No new vendor
code or binaries enter Git. SDL2/LibArchive feature scope/options are unchanged.

New fixture5/5 includes actual option declarations for fresh Windows/macOS/Linux
contexts and verifies all five static/fetch defaults; archive-resolution negative
cases retain passing-before-one-mutation and exact error codes. Actual static
SDL/archive ON selection:25 passed, one H.264 hardware probe skipped. Static
preset/Metal loader, exact FFV1/PCM/native runtime integration and ROM-less tests
are included. OFF16/16 and final Release executable selftest pass. Logs:
`/tmp/m100s2/sdl-archive-*.log`; exact configure commands and source identities
are retained in the task-local build directory. Windows86535d23 artifact is
unchanged: default values/target path remain identical, no new Windows runtime
claim or unnecessary rebuild. Earlier profiling remains a historical checkpoint.

**macOS handoff at this packaging checkpoint** (later REC timer supersedes it):
`build/macos-recording-static-release/sdl2/vaeg`,38697320 bytes, SHA256
`4f5b01c7b28705ef04834cbe21e7860e8fcacba0e2a4194e3b3586d64b3ddc4f`.
Base0455d0c9 plus unchanged three-file inherited snapshot9e123689… (full identity
below/adjacent report), not a clean-commit artifact. Release arm64, recording ON,
trace/performance instrumentation/tests OFF. `otool -L` now resolves every import
to `/usr/lib/` or `/System/Library/`: no SDL2/LibArchive/FFmpeg/librashader/zlib/xz
or MacPorts iconv dylib dependency. Iconv is the Apple system library, not a new
statically bundled dependency. No MacPorts runtime installation is required by
these imports. App packaging, old-macOS runtime compatibility, live Metal/audio,
media/demo/human and redistribution/relinking legal approvals remain separate.
No new private-media access, inherited edit/commit, hosted CI, push or main change.

## Previous FFmpeg/librashader-only checkpoint checks

- Isolated CMake fixture4/4: archive chosen beside dylib/so, ordered private
  dependency closure, one missing-archive mutation, empty metadata mutation,
  incomplete Framework metadata mutation. Every negative starts with the same
  passing configuration and asserts its specific stable machine-readable code.
- Real macOS ON selection:25 tests pass, one H.264 hardware probe is skipped;
  includes exact FFV1/PCM, native session/queue/mux runtime integration,
  static-preset, static Metal function-table loading and ROM-less GUI/FPS tests.
- Recording OFF16/16, including standalone static-resolver fixture.
- Final arm64 Release executable passes ROM-less selftest with dummy SDL/audio.
  `otool -L` shows **no avcodec/avformat/avutil/librashader dylibs**.
- Windows static Release and unchanged system-DLL PE audit pass with the default
  FFmpeg option ON. Existing audited Rust/shader and FFmpeg archives are reused.
- Initial link exposes split `-framework CoreVideo` metadata incorrectly treated
  as `-lCoreVideo`; fixed as a grouped Framework item and protected by fixtures.
  An old test tree also fails because its cached CLT linker cannot read the
  current SDK's TAPI architecture. Preserve its log; use the existing current
  Xcode test tree instead, without weakening checks or compiler settings.
- macOS compatibility workflow now prepares the same pinned static librashader
  archive/notices as the existing release workflow. No hosted CI is requested.

Local logs/commands: `/tmp/m100s2/static-*.log`; pinned native FFmpeg build commands
and exits: task-local `ffmpeg-lgpl-static-build.json`. Neither private input paths
nor raw guest evidence is published. No unchanged expensive profiling rerun:
previous dynamic-library throughput is a historical checkpoint, not attributed
to this static target. No native Windows/live Metal performance gate is claimed.

## Previous FFmpeg/librashader-only local artifacts

Working-tree builds based on86535d23 plus unchanged/uncommitted inherited sources:

- macOS `build/macos-recording-static-release/sdl2/vaeg`,36313944 bytes, SHA256
  `e92d74dcacb136b4ce10a52653fd06e33f938822ffccbd7e63e56f5f530dc727`.
  Full three-file source snapshot SHA256
  `9e123689c9bac9053acf2b00a0aee9729e0589578e4b49d5aac8d5d407e7c7f0`.
- Windows task-local `output/vaeg.exe` and `output/vaeg-audio-local-test.exe`,
  58846417 bytes, SHA256
  `69df6d74f0be73ac291fc24fe9b939ab9f5d61edde23444cd2fc90b7dfd127fe`.
  Unchanged audio patch SHA256
  `0925091601467304336586c3bdf66b3d7cc08328afd5b9797edc190c7c1a2386`.
  Metal excluded. Previous105e5cac binary/report preserved under that suffix.

macOS: arm64 Release, recording/static FFmpeg/static librashader ON, CPU trace/
performance instrumentation/tests OFF. Minimal LGPL2.1+ FFmpeg8.1.2, same pinned
archive SHA256 `464beb5e7bf0c311e68b45ae2f04e9cc2af88851abb4082231742a74d97b524c`;
GPL/nonfree/version3/autodetect/network disabled, only FFV1/PCM/Matroska/file.
Native static FFmpeg is built separately, leaving the earlier shared checkpoint
untouched. Existing audited librashader87e8a97b archive/notices are reused, not
rebuilt or modified. Archive paths and digests are in the adjacent build report.

At that earlier checkpoint, only those **two libraries** were changed; local
SDL2/LibArchive/iconv remained shared. The current follow-up above supersedes
that external shared-library requirement; Apple Frameworks/system libraries
remain dynamic. Not a standalone portable .app or a macOS13 support guarantee
(MacPorts dependencies have newer minimum versions). Static LGPL redistribution
still needs corresponding source/notices and user relinking materials; no legal/
release approval, clean-checkout human/media gate or OS-assisted completion.
Three inherited files remain byte-identical/uncommitted. LOCAL_ONLY, no push.
