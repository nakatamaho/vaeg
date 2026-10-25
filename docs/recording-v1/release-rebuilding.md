<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

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

# Rebuilding the recording-enabled public release

Public release jobs explicitly enable recording and static FFmpeg, and disable
local-only H.264/AAC encoders. FFmpeg 8.1.2 is built with GPL, nonfree,
version3, autodetection and network support disabled. Only FFV1/PCM encoders
and decoders, the Matroska muxer/demuxer and file protocol are enabled.
The libavcodec archive is audited at SDK creation, application configure/build
and package staging. No dependency on a system FFmpeg package is used.

The FFmpeg portion is LGPL-2.1-or-later. Each package includes:

- `licenses/ffmpeg-lgpl-2.1.txt`;
- `recording-source/ffmpeg-8.1.2.tar.xz`, the exact unmodified source;
- the target's generated `ffmpeg-config.h` and `ffmpeg-config.mak`;
- `vaeg-source.tar.gz`, the full tracked application source at the build commit;
- this guide, the archive-audit result and source-material checksums.

The complete application source is supplied so the executable can be rebuilt
and relinked with a modified FFmpeg; application object files are not required
for this route. Do not restrict recipients' exercise of the LGPL rights,
including modification and reverse engineering for debugging modifications.
Keep the license, corresponding source and rebuilding materials with any
redistribution. This describes the engineering provision, not a legal or
patent-clearance determination.

## Build and relink

Unpack `vaeg-source.tar.gz` and enter `vaeg-source`. The root README,
`CMakePresets.json` and `.github/workflows/release.yml` describe the compiler,
platform tools and pinned SDL2/LibArchive/librashader dependencies. Use the
release tag/embedded commit to identify the original build. The tracked
source includes the dependency fetch/build scripts and notices.

Use a modern Bash (the release builders use Bash 5). On macOS, install it
with `brew install bash` and invoke `"$(brew --prefix)/bin/bash"` rather than
the system's Bash 3.2: the tagged SDK helper uses an empty array with `set -u`,
which Bash 3.2 rejects. The macOS package includes this prerequisite in
`recording-source/BUILD-ENVIRONMENT.txt` as well.

For a native Linux, macOS or MSYS2 MINGW64 build, prepare the original minimal
FFmpeg SDK with:

```sh
bash tools/release/build-recording-ffmpeg.sh build/recording-sdk
export PKG_CONFIG_LIBDIR="$PWD/build/recording-sdk/stage/lib/pkgconfig"
export PKG_CONFIG_PATH="$PKG_CONFIG_LIBDIR"
```

The script verifies the downloaded source against SHA-256
`464beb5e7bf0c311e68b45ae2f04e9cc2af88851abb4082231742a74d97b524c`.
A copy of that same archive is in this package, independent of the download
server. To use modified FFmpeg, unpack the supplied archive, apply your
changes, and configure/build/install it into your own prefix using the options
in the script and the supplied target configuration as a reference. The
original-source checksum guard belongs to the unmodified release helper;
rebuilding modified source is performed directly with FFmpeg's configure/make.
Set `PKG_CONFIG_LIBDIR`/`PKG_CONFIG_PATH` to the modified installation's
`lib/pkgconfig` instead. Do not substitute a different library ABI.

After preparing the platform's pinned librashader SDK as in the workflow,
configure vaeg using `linux-ci-gcc`, `mingw-ci` or `macos-ci`, with the same
platform-specific librashader arguments as the workflow, plus:

```text
-DVAEG_ENABLE_RECORDING=ON
-DVAEG_STATIC_FFMPEG=ON
-DVAEG_PUBLIC_BUILD=ON
-DVAEG_ENABLE_LOCAL_MP4_ENCODERS=OFF
```

Build that preset and run CTest before replacing your executable. The static
resolver selects the exact FFmpeg archives from your pkg-config prefix; the
normal archive policy rejects H.264/AAC and any encoder outside the ordinary
FFV1/PCM allow-list. A modified library must preserve the expected APIs and
that public-build policy. No signing key or activation is required to run a
rebuilt executable. Native Windows execution is distinct from cross-compiling.

Windows/macOS MP4 conversion calls encoders provided by the OS. Linux public
builds have no MP4 encoder backend. Neither the source bundle nor these
instructions enable or include the separately isolated local-only encoders.
