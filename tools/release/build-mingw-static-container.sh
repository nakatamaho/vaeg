#!/usr/bin/env bash
# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
# TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Clean, fully static MinGW-w64 build of vaeg.exe in an isolated container.
#
# usage: tools/release/build-mingw-static-container.sh [--commit REV] [--work-dir DIR] [--recording]
#
# REV defaults to HEAD. DIR defaults to ../vaeg-mingw-<short-sha> beside the
# repository and must not exist. The script clones REV into DIR/source (no
# existing build tree is reused), builds in a pinned Ubuntu 24.04 container
# with the POSIX-thread MinGW-w64 compilers and Rust 1.88.0 using the
# `mingw-cross` preset (static SDL2, LibArchive, zlib, xz, librashader and
# MinGW runtimes), audits the PE imports, and writes DIR/output/vaeg.exe,
# pe-imports.txt, third-party-notices.txt, the logs and BUILD-REPORT.md.
# --recording adds pinned minimal static FFmpeg archive recording. --shader-sdk
# DIR reuses only an audited dependency SDK root (librashader-git/windows-audit),
# mounted read-only and checked by the existing CMake archive/notices audit.
# It never reuses application objects. It prints vaeg.exe's path on success.

set -euo pipefail

image="ubuntu@sha256:008173c23f95b170204355c12626cb5a965d779a7e1283b09e9cffbb1bf33ca3"
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
revision="HEAD"
work_dir=""
recording="OFF"
shader_sdk=""

while [[ $# -gt 0 ]]; do
	case "$1" in
	--commit) revision="$2"; shift 2 ;;
	--work-dir) work_dir="$2"; shift 2 ;;
	--recording) recording="ON"; shift ;;
	--shader-sdk) shader_sdk="$(cd "$2" && pwd)"; shift 2 ;;
	*) echo "usage: $0 [--commit REV] [--work-dir DIR] [--recording] [--shader-sdk DIR]" >&2; exit 2 ;;
	esac
done

commit="$(git -C "${repo_root}" rev-parse --verify "${revision}^{commit}")"
if [[ -z "${work_dir}" ]]; then
	work_dir="$(dirname "${repo_root}")/vaeg-mingw-${commit:0:8}"
fi
if [[ -e "${work_dir}" ]]; then
	echo "work directory already exists: ${work_dir}" >&2
	exit 1
fi
mkdir -p "${work_dir}/output"
work_dir="$(cd "${work_dir}" && pwd)"

git clone --quiet --no-local --no-checkout "${repo_root}" "${work_dir}/source"
git -C "${work_dir}/source" checkout --quiet --detach "${commit}"
if [[ -n "$(git -C "${work_dir}/source" status --porcelain)" ]]; then
	echo "clean checkout is not clean" >&2
	exit 1
fi
branches="$(git -C "${repo_root}" branch -r --contains "${commit}" 2>/dev/null | tr -d ' ' | paste -sd, -)"

cat > "${work_dir}/inner.sh" <<'INNER'
#!/usr/bin/env bash
set -euo pipefail
trap 'chown -R "${HOST_UID}:${HOST_GID}" /work' EXIT
export DEBIAN_FRONTEND=noninteractive
apt-get update -qq
apt-get install -y -qq --no-install-recommends \
	ca-certificates curl git python3 cmake ninja-build pkg-config file gcc libc6-dev make xz-utils \
	mingw-w64 gcc-mingw-w64-x86-64-posix g++-mingw-w64-x86-64-posix \
	binutils-mingw-w64-x86-64 > /dev/null
update-alternatives --set x86_64-w64-mingw32-gcc /usr/bin/x86_64-w64-mingw32-gcc-posix
update-alternatives --set x86_64-w64-mingw32-g++ /usr/bin/x86_64-w64-mingw32-g++-posix
curl -sSf https://sh.rustup.rs | sh -s -- -y --profile minimal --default-toolchain 1.88.0 \
	--target x86_64-pc-windows-gnu > /dev/null
export PATH="${HOME}/.cargo/bin:${PATH}"
# CMake reads the commit and date with git under an empty git config, so
# git only trusts the clone if the container user owns it. The exit trap
# returns ownership to the host user.
chown -R 0:0 /work/source

cd /work/source
cmake_args=()
shader_audit=/work/source/build/mingw-static/windows-audit
if [[ -d /shader-sdk ]]; then
	# Application objects are never reused. CMake's existing source/archive/
	# notices audit remains mandatory for this read-only dependency SDK.
	shader_audit=/shader-sdk/windows-audit
	cmake_args+=(
		-DVAEG_LIBRASHADER_STATIC_LIBRARY=/shader-sdk/librashader-git/target/x86_64-pc-windows-gnu/release/liblibrashader_capi.a
		-DVAEG_LIBRASHADER_STATIC_NOTICES=/shader-sdk/windows-audit/third-party-notices.txt)
	cp "${shader_audit}/static-build.json" /work/output/reused-shader-sdk.json
else
	tools/release/build-mingw-static-librashader.sh > /work/output/librashader-build.log 2>&1
fi
if [[ "${RECORDING}" == ON ]]; then
	# Minimal LGPL-only archive recorder. No GPL/nonfree/version3, autodetected
	# external codecs, network, or implicit host-library dependencies.
	curl -fLsS https://ffmpeg.org/releases/ffmpeg-8.1.2.tar.xz -o /work/ffmpeg.tar.xz
	echo '464beb5e7bf0c311e68b45ae2f04e9cc2af88851abb4082231742a74d97b524c  /work/ffmpeg.tar.xz' | sha256sum -c -
	tar -xJf /work/ffmpeg.tar.xz -C /work
	(
		cd /work/ffmpeg-8.1.2
		./configure --prefix=/work/ffmpeg-stage --target-os=mingw32 --arch=x86_64 \
			--cross-prefix=x86_64-w64-mingw32- --enable-cross-compile \
			--pkg-config=pkg-config --disable-shared --enable-static \
			--disable-programs --disable-doc --disable-debug --disable-asm \
			--disable-autodetect --disable-network --disable-gpl --disable-nonfree \
			--disable-version3 --disable-everything \
			--enable-encoder=ffv1,pcm_s16le --enable-decoder=ffv1,pcm_s16le \
			--enable-muxer=matroska --enable-demuxer=matroska --enable-protocol=file
		make -j"$(nproc)"
		make install
	) > /work/output/ffmpeg-build.log 2>&1
	cp /work/ffmpeg-8.1.2/COPYING.LGPLv2.1 /work/output/ffmpeg-license.txt
	cp /work/ffmpeg-8.1.2/ffbuild/config.mak /work/output/ffmpeg-config.mak
	cmake_args+=(-DVAEG_MINGW_DEPENDENCY_PREFIX=/work/ffmpeg-stage)
fi
cmake --preset mingw-cross -DVAEG_ENABLE_RECORDING="${RECORDING}" \
	-DVAEG_PUBLIC_BUILD=ON -DVAEG_ENABLE_LOCAL_MP4_ENCODERS=OFF "${cmake_args[@]}" > /work/output/configure.log 2>&1
cmake --build --preset mingw-cross > /work/output/build.log 2>&1
python3 tools/release/check-static-windows-imports.py --binary build/mingw-cross/sdl2/vaeg.exe \
	> /work/output/import-audit.txt 2>&1
x86_64-w64-mingw32-objdump -p build/mingw-cross/sdl2/vaeg.exe | grep 'DLL Name' \
	> /work/output/pe-imports.txt
if ! grep -q "VAEG_BUILD_COMMIT \"$(git rev-parse HEAD)\"" build/mingw-cross/generated/vaeg_build_info.h ||
	grep -q 'VAEG_BUILD_DATE "unknown"' build/mingw-cross/generated/vaeg_build_info.h; then
	echo "build identity was not embedded" >&2
	exit 1
fi
cp build/mingw-cross/generated/vaeg_build_info.h /work/output/
cp build/mingw-cross/sdl2/vaeg.exe /work/output/vaeg.exe
cp "${shader_audit}/third-party-notices.txt" /work/output/
{
	echo "gcc=$(x86_64-w64-mingw32-gcc --version | head -1)"
	echo "gxx=$(x86_64-w64-mingw32-g++ --version | head -1)"
	echo "binutils=$(x86_64-w64-mingw32-ld --version | head -1)"
	echo "cmake=$(cmake --version | head -1)"
	echo "ninja=$(ninja --version)"
	echo "rustc=$(rustc --version)"
	echo "file=$(file -b /work/output/vaeg.exe)"
} > /work/output/toolchain.txt
INNER
chmod +x "${work_dir}/inner.sh"

echo "building ${commit} in ${work_dir}" >&2
docker_args=()
if [[ -n "${shader_sdk}" ]]; then
	docker_args+=(-v "${shader_sdk}:/shader-sdk:ro")
fi
docker run --rm ${docker_args[@]+"${docker_args[@]}"} -e HOST_UID="$(id -u)" -e HOST_GID="$(id -g)" -e RECORDING="${recording}" \
	-v "${work_dir}:/work" "${image}" /work/inner.sh

exe="${work_dir}/output/vaeg.exe"
digest="$(python3 -c 'import hashlib,sys; print(hashlib.sha256(open(sys.argv[1], "rb").read()).hexdigest())' "${exe}")"
size="$(python3 -c 'import os,sys; print(os.path.getsize(sys.argv[1]))' "${exe}")"
{
	echo "# VAEG MinGW-w64 fully static build"
	echo
	echo "- Source commit: \`${commit}\`"
	echo "- Remote branches containing it: ${branches:-none}"
	echo "- Clean clone in \`${work_dir}/source\`; no existing build tree was reused."
	echo "- Container image: \`${image}\` (Ubuntu 24.04)"
	echo "- Preset: \`mingw-cross\`, Release, tests disabled; static SDL2, LibArchive,"
	echo "  zlib, xz, librashader and MinGW gcc/libstdc++/winpthread runtimes."
	echo "- Shader SDK: ${shader_sdk:-fresh pinned source build}; application objects are fresh."
	echo "- Recording: ${recording}; when ON, pinned minimal LGPL-only FFmpeg 8.1.2 is static."
	echo "- Local-only H.264/AAC encoders: OFF (public handoff); archive encoder registrations audited before linking."
	echo "- Post-stop MP4 when recording ON: Windows Media Foundation OS APIs, no bundled H.264/AAC encoder implementation."
	echo "  Source tarball and full configure/build log are retained beside the clean clone."
	sed 's/^/- /' "${work_dir}/output/toolchain.txt"
	echo
	echo "## Artifact"
	echo
	echo "- Path: \`${exe}\`"
	echo "- Size: ${size} bytes"
	echo "- SHA-256: \`${digest}\`"
	echo "- Import audit (\`check-static-windows-imports.py\`):"
	sed 's/^/  /' "${work_dir}/output/import-audit.txt"
	echo "- Imported DLLs: see \`pe-imports.txt\`."
	echo
	echo "Compile and link only. Windows execution, self-tests and guest behaviour"
	echo "were not run by this script."
} > "${work_dir}/output/BUILD-REPORT.md"
echo "${exe}"
