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
cache_dir=${VAEG_PC88VA_DEVDISK_CACHE:-${XDG_CACHE_HOME:-${HOME}/.cache}/vaeg/pc88va-development-disk}
work_dir=
output_tmp=
download_tmp=

usage() {
	printf '%s\n' \
		"Usage: $program_name --source SOURCE.d88 --output OUTPUT.d88 [--cache DIR]" \
		'' \
		'First create a vanilla PC-Engine 1.1 system disk, then add PCEPAT,' \
		'BMS, MSE 3.52b, PCPLUS, network and disk tools, and K-Launcher.' \
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
	if [[ -n ${work_dir} && ${work_dir} == "${TMPDIR:-/tmp}"/vaeg-pc88va-devdisk.* && -d ${work_dir} ]]; then
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

for required_command in curl dosbox lha python3 sha256sum tar; do
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
	mv -- "$download_tmp" "$destination"
	download_tmp=
}

fetch_package pcepat.com \
	59296bcb77b158ce072a7f62bdbdca420305fb43004f69845345efc73c276945 \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=330&fname=PCEPAT.COM'
fetch_package pcp108.lzh \
	4561df318cdfb08bdf8276741058ca2b0d4a4eeaf084a8c5b587f520f6c9e3f0 \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=378&fname=PCP108.LZH'
fetch_package pcp108p.lzh \
	25f1d9432247c88667b880f4153966725a757b4dcd1a062c88497d32b0c8eef7 \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=451&fname=PCP108P.LZH'
fetch_package bdiff128.lzh \
	0ba491ee4829a6f292cfbcad25371a98c2161c1a92d028b0d2fd5dd9d9011153 \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=328&fname=BDIFF128.LZH'
fetch_package mse352a.lzh \
	bdbe863b4eb451692d5450b9ae754260fec8b3481a4936f1c38ffa850b4a9cbd \
	'https://web.archive.org/web/20060220170035id_/http://hp.vector.co.jp:80/authors/VA015636/mse352a.lzh'
fetch_package mse352bf.lzh \
	46007ecf062fc32c3be04f3a6715ca38e3f086f570f817b11a5183aef9254b9a \
	'https://web.archive.org/web/20060517104746id_/http://hp.vector.co.jp:80/authors/VA015636/mse352bf.lzh'
fetch_package wsp150.lzh \
	e2c9ebfcf2aea495baab186cab7a1ac790027f7ea93e41650f1744c5ccb594b3 \
	'https://ftp.vector.co.jp/00/08/531/wsp150.lzh'
fetch_package lha213.exe \
	7ff44c3c971e453c1db784c731471307b5d04248150ea246e52afdff1b378b6d \
	'https://web.archive.org/web/20021020200442id_/http://archiver.wakusei.ne.jp:80/docs/lha213.exe'
fetch_package kl130.lzh \
	8b8e2b23d3da27cf4089e283f49d923e884611a11e213532afa77b5fb4246dfb \
	'https://toroidj.github.io/dos/KL130.LZH'
fetch_package bms15020.tgz \
	b0ee1dc6679ecad155ed9aabc2aa66f253c9d5f1c0190cf2110cc68e59f7b405 \
	'https://ftp.vector.co.jp/09/04/385/bms15020.tgz'
fetch_package teen030p.lzh \
	9b6bdd4b2dbc4908d5a749994cdb87c63e99b13cb294c108608ce4f04248c71e \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=470&fname=TEEN030P.LZH'
fetch_package vbuff102.lzh \
	c51d2f9bd04efeda77760a2c8e476777c07edb8775a7120793dd98bc0a8ff01f \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=452&fname=VBUFF102.LZH'
fetch_package fatmap11.lzh \
	9e25c73df9d589306ae24c3908fb3b8e4ee2b1c6f306a1b8eb07155a60e2e701 \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=430&fname=FATMAP11.LZH'
fetch_package forg203.lzh \
	1315141e7e6c37d010ef9a725a927fa2ba71e4e086d630d2aa942a704a7ae5c4 \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=431&fname=FORG203.LZH'
fetch_package ramdisk.com \
	e0cf4510f4f54ee2825c866ee3a2b07fb2e5f60b7e8d10bfa34401a29e7e4b51 \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=398&fname=RAMDISK.COM'
fetch_package ramdisk.doc \
	4f5e549bdbc75db6cf95ebd13dc722500891fa22265ed793b22e81585e94e461 \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=398&fname=RAMDISK.DOC'

work_dir=$(mktemp -d "${TMPDIR:-/tmp}/vaeg-pc88va-devdisk.XXXXXX")

extract_archive() {
	local archive=$1
	local destination=$2

	mkdir -p -- "$destination"
	lha xfw="$destination" "$archive" >/dev/null
}

extract_archive "$cache_dir/pcepat.com" "$work_dir/pcepat"
extract_archive "$cache_dir/pcp108.lzh" "$work_dir/pcp108"
extract_archive "$cache_dir/pcp108p.lzh" "$work_dir/pcp108p"
extract_archive "$cache_dir/bdiff128.lzh" "$work_dir/bdiff"
extract_archive "$cache_dir/mse352a.lzh" "$work_dir/mse352a"
extract_archive "$cache_dir/mse352bf.lzh" "$work_dir/mse352bf"
extract_archive "$cache_dir/wsp150.lzh" "$work_dir/wsp"
extract_archive "$cache_dir/lha213.exe" "$work_dir/lha"
extract_archive "$cache_dir/kl130.lzh" "$work_dir/kl"
extract_archive "$cache_dir/teen030p.lzh" "$work_dir/teen"
extract_archive "$cache_dir/vbuff102.lzh" "$work_dir/vbuff"
extract_archive "$cache_dir/fatmap11.lzh" "$work_dir/fatmap"
extract_archive "$cache_dir/forg203.lzh" "$work_dir/forg"
mkdir -p -- "$work_dir/bms"
tar -xzf "$cache_dir/bms15020.tgz" -C "$work_dir/bms"

stage_dir=$work_dir/stage
mkdir -p -- "$stage_dir"
cp -p -- "$work_dir/wsp/WSP.COM" "$stage_dir/"
cp -p -- "$work_dir/mse352a/ALIAS.COM" "$stage_dir/"
cp -p -- "$work_dir/mse352a/MSE350.DEF" "$stage_dir/"
cp -p -- "$work_dir/mse352a/MSE352A.COM" "$stage_dir/"
cp -p -- "$work_dir/mse352a/MSE352A.DOC" "$stage_dir/"
cp -p -- "$work_dir/mse352a/MSE352A.HIS" "$stage_dir/"
cp -p -- "$work_dir/mse352a/MSECUST.COM" "$stage_dir/"
cp -p -- "$work_dir/mse352a/MSET.COM" "$stage_dir/"
cp -p -- "$work_dir/mse352a/README.DOC" "$stage_dir/"
cp -p -- "$work_dir/mse352bf/MSE352BF.WUP" "$stage_dir/"
cp -p -- "$work_dir/bdiff/BUPDATE.EXE" "$stage_dir/"
cp -p -- "$work_dir/pcp108/PCP108/PCPLUS.SYS" "$stage_dir/"
cp -p -- "$work_dir/pcp108p/PCPLUS.BDF" "$stage_dir/"
cp -p -- "$work_dir/kl/KL.COM" "$stage_dir/"
cp -p -- "$work_dir/kl/KLV.EXE" "$stage_dir/"
cp -p -- "$work_dir/kl/KLCUST.EXE" "$stage_dir/"
cp -p -- "$work_dir/kl/KLVA.COM" "$stage_dir/"

dosbox_conf=$work_dir/dosbox.conf
printf '%s\n' \
	'[midi]' \
	'mpu401=none' \
	'mididevice=none' \
	'[sblaster]' \
	'sbtype=none' \
	'[gus]' \
	'gus=false' \
	'[speaker]' \
	'pcspeaker=false' \
	'tandy=off' \
	'disney=false' >"$dosbox_conf"

printf '%s\n' 'Applying MSE, PCPLUS, and K-Launcher patches'
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy dosbox -conf "$dosbox_conf" -exit \
	-c "mount c $stage_dir" \
	-c 'c:' \
	-c 'wsp -t -b mse352bf.wup > mse.log' \
	-c 'bupdate -x -i -o pcplus.bdf > pcp.log' \
	-c 'wsp -t -b klva.com > kl.log' \
	-c 'exit' >/dev/null 2>&1

verify_generated() {
	local name=$1
	local expected=$2

	[[ -f $stage_dir/$name ]] || die "DOS patch did not create $name"
	verify_sha256 "$stage_dir/$name" "$expected" ||
		die "DOS patch created unexpected contents: $name"
}

verify_generated ALIAS.COM de06d39557440dc1a296b5d5ef80fda9fb7b57f63c67a2d339bc51db9bc12ed7
verify_generated MSE352B.COM 794375496c62bf8f508ccbf57c8ceeb2ab439606d31638eea397eaac6bd3e68a
verify_generated PCPLUS.SYS f86d03201a2fa6c0dab13345df55f3bb929f41ec3c7c6d03efb4dbd7935f1b06
verify_generated KLL.COM 752600dfb9809432310046047f6142b8edce47c25e25a14c1baa2d91bda87910
verify_generated KLVA.EXE c6ad097435111398f1c1ebc90e9f35cd15caded6b5d6bed49d92c354ab7f3c43
verify_generated KLCUST.EXE 72376b967fe51d4f40759f5d875762fa3b2b09a353afb1a9ea3c957f5a9c87bf

payload_dir=$work_dir/payload
mkdir -p -- "$payload_dir/root" "$payload_dir/bin" "$payload_dir/doc" \
	"$payload_dir/tmp"

copy_payload() {
	cp -- "$1" "$payload_dir/$2"
}

copy_payload "$work_dir/pcepat/PCEPAT.SYS" root/PCEPAT.SYS
copy_payload "$work_dir/bms/bmsdrva.com" root/BMSDRVA.COM
copy_payload "$work_dir/bms/bmsaddva.com" root/BMSADDVA.COM
copy_payload "$stage_dir/MSE352B.COM" root/MSE352B.COM
copy_payload "$stage_dir/PCPLUS.SYS" root/PCPLUS.SYS

copy_payload "$work_dir/lha/LHA.EXE" bin/LHA.EXE
copy_payload "$work_dir/bdiff/BUPDATE.EXE" bin/BUPDATE.EXE
copy_payload "$work_dir/wsp/WSP.COM" bin/WSP.COM
copy_payload "$stage_dir/MSET.COM" bin/MSET.COM
copy_payload "$stage_dir/ALIAS.COM" bin/ALIAS.COM
copy_payload "$stage_dir/MSECUST.COM" bin/MSECUST.COM
copy_payload "$stage_dir/MSE350.DEF" bin/MSE350.DEF
copy_payload "$stage_dir/KLL.COM" bin/KLL.COM
copy_payload "$stage_dir/KLVA.EXE" bin/KLVA.EXE
copy_payload "$stage_dir/KLCUST.EXE" bin/KLCUST.EXE
copy_payload "$work_dir/kl/KL.CFG" bin/KL.CFG
copy_payload "$work_dir/kl/KLJPN.HLP" bin/KLJPN.HLP
copy_payload "$work_dir/teen/TEEN.COM" bin/TEEN.COM
copy_payload "$work_dir/teen/TEENM.COM" bin/TEENM.COM
copy_payload "$work_dir/teen/TEEN.DEF" bin/TEEN.DEF
copy_payload "$work_dir/teen/TOPEN.EXE" bin/TOPEN.EXE
copy_payload "$work_dir/teen/TCLOSE.EXE" bin/TCLOSE.EXE
copy_payload "$work_dir/teen/TLOG.COM" bin/TLOG.COM
copy_payload "$work_dir/teen/TLOGBMS.COM" bin/TLOGBMS.COM
copy_payload "$work_dir/vbuff/VBUFF.COM" bin/VBUFF.COM
copy_payload "$work_dir/fatmap/FATMAP.EXE" bin/FATMAP.EXE
copy_payload "$work_dir/fatmap/FATMAP_E.COM" bin/FATMAP_E.COM
copy_payload "$work_dir/forg/FORG.EXE" bin/FORG.EXE
copy_payload "$work_dir/forg/FORG.DAT" bin/FORG.DAT
copy_payload "$cache_dir/ramdisk.com" bin/RAMDISK.COM

copy_payload "$work_dir/teen/TEEN.DOC" doc/TEEN.DOC
copy_payload "$work_dir/teen/TEENUPDT.DOC" doc/TEENUPDT.DOC
copy_payload "$work_dir/teen/README.DOC" doc/TEENREAD.DOC
copy_payload "$work_dir/teen/TLOG.DOC" doc/TLOG.DOC
copy_payload "$work_dir/vbuff/VBUFF.DOC" doc/VBUFF.DOC
copy_payload "$work_dir/vbuff/VBUFF.LOG" doc/VBUFF.LOG
copy_payload "$work_dir/fatmap/FATMAP.MAN" doc/FATMAP.MAN
copy_payload "$work_dir/fatmap/README.DOC" doc/FATMREAD.DOC
copy_payload "$work_dir/forg/FORG.DOC" doc/FORG.DOC
copy_payload "$work_dir/forg/README.DOC" doc/FORGREAD.DOC
copy_payload "$cache_dir/ramdisk.doc" doc/RAMDISK.DOC

printf '%s\r\n' \
	'FILES = 20' \
	'BUFFERS = 30' \
	'DEVICE = A:\PCEPAT.SYS' \
	'DEVICE = A:\MSE352B.COM' \
	'DEVICE = A:\PCPLUS.SYS' >"$payload_dir/root/CONFIG.SYS"

printf '%s\r\n' \
	'PATH A:\BIN' \
	'SET TEEN=A:\BIN\TEEN.DEF' \
	'SET TMP=A:\TMP' \
	'SET COMSPEC=A:\PCENGINE.COM' >"$payload_dir/root/AUTOEXEC.BAT"

vanilla_d88=$work_dir/vanilla.d88
"$script_dir/create-vanilla-system-disk.sh" \
	--source "$source_d88" \
	--output "$vanilla_d88"

output_tmp=$(mktemp "$output_d88.tmp.XXXXXX")
cp -- "$vanilla_d88" "$output_tmp"
python3 "$script_dir/pcengine_disk.py" install \
	--image "$output_tmp" \
	--payload "$payload_dir"
chmod 0644 "$output_tmp"
output_moved=false
for move_attempt in 1 2 3; do
	if mv -- "$output_tmp" "$output_d88"; then
		output_moved=true
		break
	fi
	sleep 1
done
[[ ${output_moved} == true ]] || die 'could not finalize output after three attempts'
output_tmp=

printf 'Created bootable development disk: %s\n' "$output_d88"
