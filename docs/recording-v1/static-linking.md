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

## Checks

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

## Current local artifacts

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

This request changes those **two libraries**, not all macOS dependencies. Current
local SDL2/LibArchive/iconv remain shared; Apple Frameworks/system libraries also
remain dynamic. Not a standalone portable .app or a macOS13 support guarantee
(MacPorts dependencies have newer minimum versions). Static LGPL redistribution
still needs corresponding source/notices and user relinking materials; no legal/
release approval, clean-checkout human/media gate or OS-assisted completion.
Three inherited files remain byte-identical/uncommitted. LOCAL_ONLY, no push.
