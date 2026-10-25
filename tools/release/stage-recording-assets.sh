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

# Supply the pinned FFmpeg source, license and configuration, application
# source and rebuilding instructions with every statically linked package.
set -euo pipefail
if [[ $# != 2 ]]; then
    echo "usage: $0 RECORDING_SDK PACKAGE_DIRECTORY" >&2
    exit 2
fi
repo="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
sdk="$(cd "$1" && pwd)"
package="$(cd "$2" && pwd)"
if ! git -C "${repo}" diff --quiet HEAD --; then
    echo "RECORDING_RELEASE_SOURCE_DIRTY: commit tracked build inputs before staging" >&2
    exit 1
fi
python3 - "${sdk}/source/ffmpeg-8.1.2.tar.xz" <<'PY'
import hashlib
import pathlib
import sys
expected = '464beb5e7bf0c311e68b45ae2f04e9cc2af88851abb4082231742a74d97b524c'
if hashlib.sha256(pathlib.Path(sys.argv[1]).read_bytes()).hexdigest() != expected:
    raise SystemExit('RECORDING_RELEASE_SOURCE_HASH: FFmpeg source checksum mismatch')
PY
python3 "${repo}/tools/repo/check_mp4_codec_archive.py" \
    --nm "${NM:-nm}" --archive "${sdk}/stage/lib/libavcodec.a"
mkdir -p "${package}/licenses" "${package}/recording-source"
cp "${sdk}/source/ffmpeg-8.1.2/COPYING.LGPLv2.1" "${package}/licenses/ffmpeg-lgpl-2.1.txt"
cp "${sdk}/source/ffmpeg-8.1.2.tar.xz" "${package}/recording-source/"
cp "${sdk}/build/ffbuild/config.mak" "${package}/recording-source/ffmpeg-config.mak"
cp "${sdk}/build/config.h" "${package}/recording-source/ffmpeg-config.h"
cp "${sdk}/archive-audit.txt" "${package}/recording-source/"
cp "${repo}/docs/recording-v1/release-rebuilding.md" "${package}/recording-source/README.md"
git -C "${repo}" archive --format=tar.gz --prefix=vaeg-source/ HEAD \
    > "${package}/recording-source/vaeg-source.tar.gz"
(
    cd "${package}/recording-source"
    python3 - <<'PY'
import hashlib
import pathlib
paths = sorted(p for p in pathlib.Path('.').iterdir()
               if p.is_file() and p.name != 'SHA256SUMS')
pathlib.Path('SHA256SUMS').write_text(''.join(
    hashlib.sha256(p.read_bytes()).hexdigest() + '  ' + p.name + '\n' for p in paths))
PY
)
