#!/usr/bin/env bash
# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Run inside the pinned MinGW container. Keep source/relink materials outside
# the clean emulator checkout. A verified cache avoids rebuilding identical
# target libraries; a corrupt cache fails closed instead of being replaced.

set -euo pipefail

ffmpeg_verify_cache() {
	local directory=$1
	if ! (cd "$directory" && sha256sum --quiet -c manifest.sha256); then
		echo 'VAEG_FFMPEG_CACHE_HASH_MISMATCH' >&2
		return 1
	fi
	if ! cmp -s \
		<(cd "$directory"; find . -type f ! -name manifest.sha256 -print | sed 's#^./##' | LC_ALL=C sort) \
		<(cd "$directory"; cut -c67- manifest.sha256 | sed 's#^./##' | LC_ALL=C sort) ||
	   [[ -n $(find "$directory" -type l -print -quit) ]]; then
		echo 'VAEG_FFMPEG_CACHE_FILE_SET_MISMATCH' >&2
		return 1
	fi
}

ffmpeg_main() {
	local work=$1 cache=$2
	local version=8.1.2
	local digest=464beb5e7bf0c311e68b45ae2f04e9cc2af88851abb4082231742a74d97b524c
	local prefix=/usr/x86_64-w64-mingw32
	local archive=$cache/ffmpeg-$version.tar.xz
	local flags=(
		--prefix="$prefix" --target-os=mingw32 --arch=x86_64
		--enable-cross-compile --cross-prefix=x86_64-w64-mingw32-
		--disable-autodetect --disable-everything --disable-programs --disable-doc
		--disable-debug --disable-network --disable-gpl --disable-nonfree --disable-version3
		--enable-static --disable-shared --disable-avdevice --disable-avfilter
		--disable-swscale --disable-swresample --enable-w32threads --disable-pthreads
		--enable-encoder=ffv1,pcm_s16le --enable-decoder=ffv1,pcm_s16le
		--enable-muxer=matroska --enable-demuxer=matroska --enable-protocol=file
	)
	local identity key cached temporary
	mkdir -p "$cache" "$work/output"
	identity=$(printf '%s\n' "$digest" "${VAEG_FFMPEG_IMAGE_ID:?}" "${flags[@]}" \
		"$(x86_64-w64-mingw32-gcc --version)" "$(x86_64-w64-mingw32-ld --version)")
	key=$(printf '%s' "$identity" | sha256sum | cut -d' ' -f1)
	cached=$cache/$key
	if [[ ! -d $cached ]]; then
		if [[ ! -f $archive ]]; then
			curl --fail --location --retry 3 "https://ffmpeg.org/releases/ffmpeg-$version.tar.xz" \
				-o "$archive.download"
			printf '%s  %s\n' "$digest" "$archive.download" | sha256sum -c -
			mv "$archive.download" "$archive"
		fi
		if ! printf '%s  %s\n' "$digest" "$archive" | sha256sum -c -; then
			echo 'VAEG_FFMPEG_SOURCE_HASH_MISMATCH' >&2
			return 1
		fi
		tar -xf "$archive" -C "$work"
		mkdir -p "$work/ffmpeg-build"
		(cd "$work/ffmpeg-build"
			"$work/ffmpeg-$version/configure" "${flags[@]}" > "$work/output/ffmpeg-configure.log" 2>&1
			make -j8 > "$work/output/ffmpeg-build.log" 2>&1
			make install DESTDIR="$work/ffmpeg-stage" > "$work/output/ffmpeg-install.log" 2>&1
		)
		temporary=$(mktemp -d "$cache/.prepare-XXXXXXXX")
		cp -a "$work/ffmpeg-stage$prefix" "$temporary/prefix"
		cp "$archive" "$temporary/source.tar.xz"
		cp "$work/ffmpeg-build/config.h" "$temporary/config.h"
		printf '%s\n' "$identity" > "$temporary/build-identity.txt"
		(cd "$temporary"; find . -type f ! -name manifest.sha256 -print0 | sort -z | \
			xargs -0 sha256sum > manifest.sha256)
		mv -T "$temporary" "$cached"
	fi
	ffmpeg_verify_cache "$cached"
	if ! cmp -s "$cached/build-identity.txt" <(printf '%s\n' "$identity") ||
	   ! printf '%s  %s\n' "$digest" "$cached/source.tar.xz" | sha256sum -c -; then
		echo 'VAEG_FFMPEG_CACHE_IDENTITY_MISMATCH' >&2
		return 1
	fi
	mkdir -p "$work/ffmpeg-stage$prefix"
	cp -a "$cached/prefix/." "$work/ffmpeg-stage$prefix/"
	cp -a "$cached/prefix/." "$prefix/"
	cp "$cached/build-identity.txt" "$work/output/ffmpeg-build-identity.txt"
	cp "$cached/source.tar.xz" "$work/output/ffmpeg-$version.tar.xz"
	tar -xOf "$cached/source.tar.xz" "ffmpeg-$version/COPYING.LGPLv2.1" \
		> "$work/output/ffmpeg-copying-lgpl-2.1.txt"
	(cd "$cached/prefix"; sha256sum lib/libavcodec.a lib/libavformat.a lib/libavutil.a) \
		> "$work/output/ffmpeg-archives.sha256"
	printf 'FFmpeg cache: %s\n' "$key"
}

if [[ ${BASH_SOURCE[0]} == "$0" ]]; then
	ffmpeg_main "$@"
fi
