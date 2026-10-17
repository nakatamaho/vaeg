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
# Local FFV1-enabled MinGW build

## Artifact and source

- Evaluated source: `d63955c00cbb31d249be767f8b3f8f6014e3542e` on the recording
  task branch, including published main `f81b1af9ae820b15c7ac29ac2fe29d8a641883bc`.
- Executable: `../vaeg-mingw-recording-74c3355d/output/vaeg.exe`.
  The directory name records the starting commit, not the final evaluated SHA.
- SHA256: `3ff27618c74735a7fb4da15bc36a3601e0499bbfa320554040e06ed81b58a723`.
- Size: 58,835,809 bytes; PE32+ x86-64 Windows GUI executable.
- Embedded identity: full evaluated SHA and build date `20261017`.
- Release `mingw-cross`, `VAEG_ENABLE_RECORDING=ON`, tests OFF for the application.
  FFV1/PCM/Matroska archive recording is enabled, not Windows real-time H.264.
- Dedicated source clone remained clean. Three inherited unstaged files were
  excluded and preserved byte-identically in the original checkout.
- Previous recording-OFF executable remains unchanged, SHA256
  `06f1626325e4ac4a968fa7c9fd95cf9352de5eeeca8e460ef98da7a6eab21500`.
- LOCAL_ONLY: no push, publication, main reset, private asset access or bundling.

## Dependencies and reproduction

Pinned Ubuntu24.04 image:
`ubuntu@sha256:008173c23f95b170204355c12626cb5a965d779a7e1283b09e9cffbb1bf33ca3`.
The compiler container was Linux aarch64; target was x86_64 Windows with
GCC13-posix, CMake3.28.3 and NASM2.16.01. No MSVC or C++/WinRT is required.

FFmpeg8.1.2 source: <https://ffmpeg.org/releases/ffmpeg-8.1.2.tar.xz>.
SHA256: `464beb5e7bf0c311e68b45ae2f04e9cc2af88851abb4082231742a74d97b524c`.
Downloaded over HTTPS; the recorded digest pins the retained archive, not an
independent signature attestation. No hand edits to FFmpeg or vendored code.

Configure flags, from a separate FFmpeg build directory:

```sh
../ffmpeg-8.1.2/configure \
 --prefix=/usr/x86_64-w64-mingw32 --target-os=mingw32 --arch=x86_64 \
 --enable-cross-compile --cross-prefix=x86_64-w64-mingw32- \
 --disable-autodetect --disable-everything --disable-programs --disable-doc \
 --disable-debug --disable-network --disable-gpl --disable-nonfree --disable-version3 \
 --enable-static --disable-shared --disable-avdevice --disable-avfilter \
 --disable-swscale --disable-swresample --enable-w32threads --disable-pthreads \
 --enable-encoder=ffv1,pcm_s16le --enable-decoder=ffv1,pcm_s16le \
 --enable-muxer=matroska --enable-demuxer=matroska --enable-protocol=file
make -j6
make install DESTDIR=/work/ffmpeg-stage
```

Only the FFmpeg encoder pair FFV1/PCM S16LE is enabled; decoding/demuxing is
retained for synthetic tests. FFmpeg reports LGPL2.1-or-later, with libavutil
60.26.102, libavcodec62.28.102 and libavformat62.12.102. No external encoders,
network stack, FFmpeg command-line programs, GPL or nonfree components.

The staged target libraries were installed into the isolated container target
sysroot. VAEG used `mingw-cross -DVAEG_ENABLE_RECORDING=ON` and a task-local
`PKG_CONFIG_EXECUTABLE` wrapper invoking `pkg-config --static`. The existing
MinGW toolchain's target-only pkg-config directory was retained; no host FFmpeg
libraries or CMake warning/standard weakening were used.

Static SDL2/LibArchive/zlib/xz sources and the completed pinned shader library
were reused from the previous task-local build without modifying it. Shader
archive SHA256:
`09b2aa5dc11a4af040b24c655d445e272642010e0b35323d7de7e7edb005a9a5`.
See the local `toolchain.txt` for the authoritative digest. Completed FFmpeg
archives were reused across the source correction; no Rust or FFmpeg rebuild
was used as an iterative debugger. Exact initial/resume/final helper scripts,
configure/build logs and archive digests remain in the task work directory.

## Demonstrated build failure and correction

The initial recording-ON compile failed in `recording_native_session.cpp`:
`sdl2/compiler.h` defined function-like `min`/`max` macros before libstdc++
headers. Diagnostics explicitly identified the definitions and collisions with
standard declarations and `numeric_limits<T>::max()`.

[Correction](https://github.com/nakatamaho/vaeg/commit/e56f950f3afd0eda9821b02d9dde8cfc7cacdd2a):
retain the macros for C only and use explicit `std::min` in the sole in-tree
C++ consumer, `ymfmbridge.cpp`. Add C and C++ header-order regression tests.
The inherited native-session file was not edited. This is a build-only fix,
not a claimed guest-visible correctness correction.
[Canonical formatting follow-up](https://github.com/nakatamaho/vaeg/commit/d63955c00cbb31d249be767f8b3f8f6014e3542e).

## Verification and limits

- Final application build/link and unchanged production Windows-system DLL
  import validator: exit0. A PATH-local cross `objdump` avoids the previously
  demonstrated ARM-native objdump format limitation, without changing checks.
- No FFmpeg, SDL2, libgcc, libstdc++ or winpthread runtime DLL imports. Required
  Windows system DLLs include bcrypt. Built-in FFV1/PCM encoder and Matroska
  muxer symbols and the full evaluated SHA are present in the executable.
- Output copy matches the linked artifact byte-for-byte; identity, size and SHA
  were checked. Original failed compile log is retained, not overwritten.
- Original macOS working tree: recording ON/OFF build/test passed 22/22 and
  14/14, including new C/C++ header tests. It includes the inherited unstaged
  audio edits, unlike the Windows source clone; do not conflate their coverage.
- Final-source Debug Windows test build: 20 standalone executables covering
  21 registered recording/header tests compiled with assertions enabled and
  static runtimes. Compilation is not test execution.
- Auxiliary amd64 Wine-container setup failed before running any test: the
  Python package post-installation script segfaulted (exit139); apt returned100.
  Exact `wine-apt.log` was retained. Wine recording tests remain NOT_RUN; this
  failure is not attributed to the Windows binary or FFV1.
- Changed manifest sources pass `clang-format-mp-22 --dry-run --Werror`.
  Full-manifest checking reports additional pre-existing formatting differences
  in unrelated CPU/tests sources; those were not mass-reformatted.
- Encoding/EOL/case and local whitespace checks passed. No hosted CI run.
- Native Windows execution, guest boot, demo/media operations and human release
  gates remain NOT_RUN. No Windows hardware encoder/capture implementation was
  added or claimed.

## Packaging status

Local maintainer test artifact, **not distribution/release validated**. Static
FFmpeg needs corresponding-source and relinking review under LGPL, not merely a
license notice. Original FFmpeg archive/source, exact configuration, installed
static libraries/headers, VAEG source clone/object files/Ninja link commands and
seed shader/dependency materials are retained for that review. FFmpeg license
texts and existing third-party notices accompany the EXE in `output/`. Do not
publish the standalone EXE as an approved distribution based on this build alone.
