#!/usr/bin/env bash
# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
# IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

# Native Linux/macOS/MSYS2-MinGW recording SDK. No system FFmpeg fallback,
# external codecs or local-only encoders. Keep the source for redistribution.
set -euo pipefail
if [[ $# != 1 ]]; then
    echo "usage: $0 NEW_OUTPUT_DIRECTORY" >&2
    exit 2
fi
if [[ -e "$1" ]]; then
    echo "RECORDING_SDK_EXISTS: output must be a fresh directory: $1" >&2
    exit 1
fi
repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
mkdir -p "$1"
output="$(cd "$1" && pwd)"
mkdir -p "${output}/source" "${output}/build" "${output}/stage"
archive="${output}/source/ffmpeg-8.1.2.tar.xz"
curl -fLsS https://ffmpeg.org/releases/ffmpeg-8.1.2.tar.xz -o "${archive}"
python3 - "${archive}" <<'PY'
import hashlib
import pathlib
import sys
expected = '464beb5e7bf0c311e68b45ae2f04e9cc2af88851abb4082231742a74d97b524c'
if hashlib.sha256(pathlib.Path(sys.argv[1]).read_bytes()).hexdigest() != expected:
    raise SystemExit('RECORDING_SDK_SOURCE_HASH: FFmpeg source checksum mismatch')
PY
tar -xJf "${archive}" -C "${output}/source"
platform_args=()
prefix="${output}/stage"
case "$(uname -s)" in
    MINGW*|MSYS*)
        prefix="$(cygpath -m "${prefix}")"
        platform_args+=(--target-os=mingw32 --arch=x86_64 --cc=gcc --cxx=g++) ;;
    Darwin|Linux) ;;
    *) echo "RECORDING_SDK_PLATFORM: unsupported native platform" >&2; exit 1 ;;
esac
cd "${output}/build"
"${output}/source/ffmpeg-8.1.2/configure" \
    --prefix="${prefix}" \
    --disable-shared --enable-static --enable-pic \
    --disable-programs --disable-doc --disable-debug --disable-asm \
    --disable-autodetect --disable-network --disable-gpl --disable-nonfree \
    --disable-version3 --disable-everything \
    --enable-encoder=ffv1,pcm_s16le --enable-decoder=ffv1,pcm_s16le \
    --enable-muxer=matroska --enable-demuxer=matroska --enable-protocol=file \
    "${platform_args[@]}" > "${output}/configure.log" 2>&1
jobs="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)"
make -j"${jobs}" > "${output}/build.log" 2>&1
make install >> "${output}/build.log" 2>&1
python3 "${repo}/tools/repo/check_mp4_codec_archive.py" \
    --nm "${NM:-nm}" --archive "${output}/stage/lib/libavcodec.a" \
    > "${output}/archive-audit.txt" 2>&1
printf '%s\n' "${output}/stage"
