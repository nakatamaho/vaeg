#!/usr/bin/env bash
#
# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
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

set -euo pipefail

program_name=${0##*/}
script_dir=$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
source_d88=
output_d88=
cache_dir=${VAEG_PC88VA_SOFTLIB_CACHE:-${XDG_CACHE_HOME:-${HOME}/.cache}/vaeg/pc88va-softlib-archive-disk}
work_dir=
output_tmp=
download_tmp=

usage() {
	printf '%s\n' \
		"Usage: $program_name --source SOURCE.d88 --output OUTPUT.d88 [--cache DIR]" \
		'' \
		'Create a non-system PC-Engine data disk containing pinned PC-88VA' \
		'Softlib downloads in A:\ARCHIVE. The downloaded files are not extracted.' \
		'The source and generated D88 images are never added to the repository.'
}

die() {
	printf 'error: %s\n' "$*" >&2
	exit 1
}

cleanup() {
	if [[ -n ${download_tmp} && -f ${download_tmp} ]]; then
		rm -f -- "$download_tmp"
	fi
	if [[ -n ${output_tmp} && -f ${output_tmp} ]]; then
		rm -f -- "$output_tmp"
	fi
	if [[ -n ${work_dir} && ${work_dir} == "${TMPDIR:-/tmp}"/vaeg-pc88va-softlib.* && -d ${work_dir} ]]; then
		rm -rf -- "$work_dir"
	fi
}

trap cleanup EXIT HUP INT TERM

while (($#)); do
	case $1 in
	--source)
		(($# >= 2)) || die '--source requires a path'
		source_d88=$2
		shift 2
		;;
	--output)
		(($# >= 2)) || die '--output requires a path'
		output_d88=$2
		shift 2
		;;
	--cache)
		(($# >= 2)) || die '--cache requires a directory'
		cache_dir=$2
		shift 2
		;;
	-h | --help)
		usage
		exit 0
		;;
	*)
		die "unknown argument: $1"
		;;
	esac
done

[[ -n ${source_d88} ]] || die '--source is required'
[[ -n ${output_d88} ]] || die '--output is required'
[[ -f ${source_d88} && -r ${source_d88} ]] || die 'source D88 is not a readable file'
[[ ! -e ${output_d88} ]] || die 'output already exists; refusing to overwrite it'
[[ -d ${output_d88%/*} || ${output_d88} != */* ]] || die 'output directory does not exist'

for required_command in curl python3 sha256sum; do
	command -v "$required_command" >/dev/null 2>&1 ||
		die "required host command is missing: $required_command"
done

mkdir -p -- "$cache_dir"

verify_sha256() {
	local path=$1
	local expected=$2
	local actual

	actual=$(sha256sum -- "$path")
	actual=${actual%% *}
	[[ ${actual} == "${expected}" ]]
}

move_with_retry() {
	local source=$1
	local destination=$2
	local attempt

	for attempt in 1 2 3; do
		if mv -- "$source" "$destination"; then
			return 0
		fi
		sleep 1
	done
	return 1
}

fetch_package() {
	local name=$1
	local expected=$2
	local url=$3
	local destination=$cache_dir/$name

	if [[ -f ${destination} ]]; then
		verify_sha256 "$destination" "$expected" ||
			die "cached package has the wrong SHA-256: $name"
		printf 'Using cached %s\n' "$name"
		return
	fi

	download_tmp=$(mktemp "$cache_dir/$name.part.XXXXXX")
	printf 'Fetching %s\n' "$name"
	if ! curl --fail --location --silent --show-error --retry 3 --connect-timeout 20 \
		--output "$download_tmp" "$url"; then
		die "download failed: $name"
	fi
	verify_sha256 "$download_tmp" "$expected" ||
		die "downloaded package has the wrong SHA-256: $name"
	move_with_retry "$download_tmp" "$destination" ||
		die "could not finalize cached package after three attempts: $name"
	download_tmp=
}

fetch_softlib_package() {
	local name=$1
	local expected=$2
	local gnum=$3
	local url="http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=$gnum&fname=$name"

	fetch_package "$name" "$expected" "$url"
}

fetch_softlib_package VBUFF102.LZH \
	c51d2f9bd04efeda77760a2c8e476777c07edb8775a7120793dd98bc0a8ff01f 452
fetch_softlib_package ALGO_VA.DOC \
	47af2b89f09b3123fdc874ac5569d9c151baf1e791f440690715d85d159913a4 390
fetch_softlib_package ALGO_VA.LZH \
	7fab7bd72912560969da0a829faff4d7d9c4aabc7dfaf5be63e6e2abecb86965 390
fetch_softlib_package 2HCDRSRC.LZH \
	69a380af1ee74ee9d4e2fc6d536d4aa87aba0280c52808863e6cc3f41be2331e 400
fetch_softlib_package EMACSVA.LZH \
	64d496d67668f7d5bd071ff304ed33a689b0795fa793afac9e631346070c8a8a 435
fetch_softlib_package EMACSVA.DOC \
	e77458e3f9a10d82c4dec6aad86b5e635b930f669012ce73fbbeff6e7f21d73a 435
fetch_softlib_package CPMVA.LZH \
	c5188efa73c80609e2184890d5a1ee5f0b274f8d29a3d73ae370fe7526d9dccd 424
fetch_softlib_package FDFRMSRC.LZH \
	d81358cbcfc1d6175359059d9c01fb75e5585993c3bc3d3e1fc988d7aa7c3e5a 401
fetch_softlib_package RDPCM001.LZH \
	a823296e0fc56927f9cf332cf3da6d1670469fada79675ba968fda1fa351891b 396
fetch_softlib_package RDPCM001.DOC \
	7321b1818176afb08c1492809b26f774518b2c730b69fc2c1340ff7ee67ce524 396
fetch_softlib_package 2HCDRV.ZIP \
	1da4d799b1aaf3a2fc94f8872eb3fd2cf6eb788fb907e8ba8c85cc79b0487e39 306
fetch_softlib_package EMMVA15A.LZH \
	1ec9bb379291f1402475afc1f6e2784b5aeda922a8310bde230e29d18c2c493c 351
fetch_softlib_package JFPPAT.ZIP \
	900e2ee9b7a3562ff1f8f9f0a4bbbd82bbd248a35f49768d5dc34607b9c194b0 307
fetch_softlib_package RDEMS152.LZH \
	0ba023a9f82defca085dc13d7103fe5b2a788ef9217d660686f2a20d8b0e70f9 270
fetch_softlib_package TDC10.LZH \
	c6c31cf6a604b07220c88a010bcd2e40cdb009dd4601e7d8a0ad903a4f2df23e 201
fetch_softlib_package BENCH003.DOC \
	a2034c794d7d3e974b77091341a5a883240a72560eb5d6cb505e1fcfcd18e9b7 389
fetch_softlib_package BENCH003.LZH \
	40f5fbf391d416a79d843c13e11797abfd8ff49ea45a7d6f8a627cb389d9c79c 389
fetch_package LSIC330C.LZH \
	c8c4c49aed600fb2413cf5707ef01b2f4057de69196c3478d5226bf1b224081b \
	'https://ftp.vector.co.jp/00/11/980/lsic330c.lzh'

work_dir=$(mktemp -d "${TMPDIR:-/tmp}/vaeg-pc88va-softlib.XXXXXX")
payload_dir=$work_dir/payload
mkdir -p -- "$payload_dir/archive"

for package in \
	VBUFF102.LZH ALGO_VA.DOC ALGO_VA.LZH 2HCDRSRC.LZH \
	EMACSVA.LZH EMACSVA.DOC CPMVA.LZH FDFRMSRC.LZH \
	RDPCM001.LZH RDPCM001.DOC 2HCDRV.ZIP EMMVA15A.LZH \
	JFPPAT.ZIP RDEMS152.LZH TDC10.LZH BENCH003.DOC BENCH003.LZH \
	LSIC330C.LZH; do
	cp -- "$cache_dir/$package" "$payload_dir/archive/$package"
done

data_d88=$work_dir/data.d88
python3 "$script_dir/pcengine_disk.py" data \
	--source "$source_d88" \
	--output "$data_d88"

output_tmp=$(mktemp "$output_d88.tmp.XXXXXX")
cp -- "$data_d88" "$output_tmp"
python3 "$script_dir/pcengine_disk.py" install \
	--image "$output_tmp" \
	--payload "$payload_dir"
chmod 0644 "$output_tmp"
move_with_retry "$output_tmp" "$output_d88" ||
	die 'could not finalize output after three attempts'
output_tmp=

printf 'Created PC-88VA Softlib archive disk: %s\n' "$output_d88"
