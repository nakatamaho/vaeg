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
		'Build a bootable PC-Engine 1.1 development floppy with PCEPAT,' \
		'MSE 3.52b, PCPLUS, DOS patch/archive tools, and K-Launcher.' \
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

for required_command in curl dosbox lha python3 sha256sum; do
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
mkdir -p -- "$payload_dir"

copy_payload() {
	cp -- "$1" "$payload_dir/$2"
}

copy_payload "$work_dir/pcepat/PCEPAT.SYS" PCEPAT.SYS
copy_payload "$work_dir/pcepat/PCEPAT.DOC" PCEPAT.DOC
copy_payload "$stage_dir/MSE352B.COM" MSE352B.COM
copy_payload "$stage_dir/MSE352B.DOC" MSE352B.DOC
copy_payload "$stage_dir/MSE352B.HIS" MSE352B.HIS
copy_payload "$stage_dir/ALIAS.COM" ALIAS.COM
copy_payload "$stage_dir/MSE350.DEF" MSE350.DEF
copy_payload "$stage_dir/MSECUST.COM" MSECUST.COM
copy_payload "$stage_dir/MSET.COM" MSET.COM
copy_payload "$stage_dir/README.DOC" MSEREAD.DOC
copy_payload "$stage_dir/PCPLUS.SYS" PCPLUS.SYS
copy_payload "$work_dir/pcp108/PCP108/PCPLUS.DOC" PCPLUS.DOC
copy_payload "$work_dir/pcp108/PCP108/PCPLUS.TXT" PCPLUS.TXT
copy_payload "$work_dir/pcp108/PCP108/BIN/SMSTAT.COM" SMSTAT.COM
copy_payload "$work_dir/pcp108/PCP108/BIN/SETDMA.COM" SETDMA.COM
copy_payload "$work_dir/lha/LHA.EXE" LHA.EXE
copy_payload "$work_dir/lha/LHA.DOC" LHA.DOC
copy_payload "$work_dir/lha/HISTORY.DOC" LHAHIST.DOC
copy_payload "$work_dir/bdiff/BDIFF.EXE" BDIFF.EXE
copy_payload "$work_dir/bdiff/BUPDATE.EXE" BUPDATE.EXE
copy_payload "$work_dir/bdiff/MODTIME.EXE" MODTIME.EXE
copy_payload "$work_dir/bdiff/BDIFF.DOC" BDIFF.DOC
copy_payload "$work_dir/wsp/WSP.COM" WSP.COM
copy_payload "$work_dir/wsp/WSP.DOC" WSP.DOC
copy_payload "$stage_dir/KLL.COM" KLL.COM
copy_payload "$stage_dir/KLVA.EXE" KLVA.EXE
copy_payload "$stage_dir/KLCUST.EXE" KLCUST.EXE
copy_payload "$work_dir/kl/KL.CFG" KL.CFG
copy_payload "$work_dir/kl/KLJPN.HLP" KLJPN.HLP
copy_payload "$work_dir/kl/KL.DOC" KL.DOC
copy_payload "$work_dir/kl/KL1ST.DOC" KL1ST.DOC

printf '%s\r\n' \
	'FILES = 20' \
	'BUFFERS = 30' \
	'DEVICE = A:\PCEPAT.SYS' \
	'DEVICE = A:\MSE352B.COM' \
	'DEVICE = A:\PCPLUS.SYS' >"$payload_dir/CONFIG.SYS"

printf '%s\r\n' \
	'@ECHO OFF' \
	'PATH A:\' \
	'SET TMP=A:\' \
	'SET COMSPEC=A:\PCENGINE.COM' \
	'PROMPT $P$G' >"$payload_dir/AUTOEXEC.BAT"

output_tmp=$(mktemp "$output_d88.tmp.XXXXXX")
cp -- "$source_d88" "$output_tmp"

python3 - "$output_tmp" "$payload_dir" <<'PY'
import re
import struct
import sys
from pathlib import Path


SECTOR_SIZE = 1024
DATA_START_LBA = 11
DATA_CLUSTER_COUNT = 1269
LAST_DATA_CLUSTER = DATA_CLUSTER_COUNT + 1
ROOT_START_LBA = 5
ROOT_SECTORS = 6


def fail(message):
    raise SystemExit(f"error: {message}")


image_path = Path(sys.argv[1])
payload_path = Path(sys.argv[2])
image = bytearray(image_path.read_bytes())

if len(image) < 0x2B0:
    fail("source is too small to be a D88 image")
if image[0x1B] != 0x20:
    fail("source is not a 2HD D88 image")
if struct.unpack_from("<I", image, 0x1C)[0] != len(image):
    fail("D88 header size does not match the file size")
if image[0x1A] != 0:
    fail("source D88 is write protected")

track_offsets = struct.unpack_from("<164I", image, 0x20)
sectors = {}
for track_index, track_offset in enumerate(track_offsets):
    if track_offset == 0:
        continue
    if track_offset + 16 > len(image):
        fail(f"invalid D88 track offset at index {track_index}")
    sector_count = struct.unpack_from("<H", image, track_offset + 4)[0]
    position = track_offset
    for _ in range(sector_count):
        if position + 16 > len(image):
            fail(f"truncated D88 sector header at track {track_index}")
        cylinder, head, record, size_code = struct.unpack_from("<BBBB", image, position)
        status = image[position + 8]
        stored_size = struct.unpack_from("<H", image, position + 14)[0]
        data_offset = position + 16
        if data_offset + stored_size > len(image):
            fail(f"truncated D88 sector data at track {track_index}")
        key = (cylinder, head, record)
        if key in sectors:
            fail(f"duplicate D88 sector C={cylinder} H={head} R={record}")
        sectors[key] = (data_offset, stored_size, size_code, status)
        position = data_offset + stored_size

normal_sectors = []
for cylinder in range(80):
    for head in range(2):
        for record in range(1, 9):
            key = (cylinder, head, record)
            if key not in sectors:
                fail(f"missing normal-area sector C={cylinder} H={head} R={record}")
            sector = sectors[key]
            if sector[1:] != (SECTOR_SIZE, 3, 0):
                fail(f"unexpected normal-area sector form at C={cylinder} H={head} R={record}")
            normal_sectors.append(sector)


def read_lbas(start, count):
    result = bytearray()
    for lba in range(start, start + count):
        data_offset = normal_sectors[lba][0]
        result.extend(image[data_offset:data_offset + SECTOR_SIZE])
    return result


def write_lbas(start, data):
    if len(data) % SECTOR_SIZE:
        fail("internal non-sector-aligned write")
    for index in range(len(data) // SECTOR_SIZE):
        data_offset = normal_sectors[start + index][0]
        begin = index * SECTOR_SIZE
        image[data_offset:data_offset + SECTOR_SIZE] = data[begin:begin + SECTOR_SIZE]


boot_sector = bytes(read_lbas(0, 1))
fat = read_lbas(1, 2)
fat_copy = read_lbas(3, 2)
root = read_lbas(ROOT_START_LBA, ROOT_SECTORS)

if fat != fat_copy:
    fail("the two source FAT12 copies differ")
if fat[:3] != b"\xfe\xff\xff":
    fail("source does not have the expected PC-Engine FAT12 media header")


def fat_get(cluster):
    offset = cluster + cluster // 2
    pair = fat[offset] | (fat[offset + 1] << 8)
    if cluster & 1:
        pair >>= 4
    return pair & 0xFFF


def fat_set(cluster, value):
    offset = cluster + cluster // 2
    value &= 0xFFF
    if cluster & 1:
        fat[offset] = (fat[offset] & 0x0F) | ((value & 0x0F) << 4)
        fat[offset + 1] = (value >> 4) & 0xFF
    else:
        fat[offset] = value & 0xFF
        fat[offset + 1] = (fat[offset + 1] & 0xF0) | ((value >> 8) & 0x0F)


def short_name(name):
    upper = name.upper()
    if upper.count(".") > 1:
        fail(f"payload name is not 8.3: {name}")
    base, separator, extension = upper.partition(".")
    if not base or len(base) > 8 or len(extension) > 3:
        fail(f"payload name is not 8.3: {name}")
    valid = re.compile(r"^[A-Z0-9_$~!#%&'()@^`{}-]+$")
    if not valid.fullmatch(base) or (extension and not valid.fullmatch(extension)):
        fail(f"payload name uses unsupported FAT characters: {name}")
    return base.ljust(8).encode("ascii") + extension.ljust(3).encode("ascii")


def find_root_entry(raw_name):
    first_free = None
    for offset in range(0, len(root), 32):
        first = root[offset]
        if first in (0x00, 0xE5) and first_free is None:
            first_free = offset
        if first == 0x00:
            break
        if first != 0xE5 and bytes(root[offset:offset + 11]) == raw_name:
            return offset, True
    if first_free is None:
        fail("root directory is full")
    return first_free, False


def release_chain(first_cluster):
    cluster = first_cluster
    visited = set()
    while 2 <= cluster <= LAST_DATA_CLUSTER:
        if cluster in visited:
            fail("loop in an existing FAT12 chain")
        visited.add(cluster)
        following = fat_get(cluster)
        fat_set(cluster, 0)
        if following >= 0xFF8:
            return
        if following == 0:
            fail("unterminated existing FAT12 chain")
        cluster = following
    fail("existing FAT12 chain points outside the data area")


expected_source_files = {
    "ENGINEIO.SYS": (2, 4096),
    "PCENGINE.SYS": (6, 62347),
    "ADVGBIOS.SYS": (67, 16364),
    "PCENGINE.COM": (83, 5),
    "HDFORM.COM": (84, 6706),
}
for source_name, (expected_cluster, expected_size) in expected_source_files.items():
    source_offset, source_exists = find_root_entry(short_name(source_name))
    if not source_exists:
        fail(f"source is not the expected PC-Engine 1.1 disk: missing {source_name}")
    source_cluster = struct.unpack_from("<H", root, source_offset + 26)[0]
    source_size = struct.unpack_from("<I", root, source_offset + 28)[0]
    if (source_cluster, source_size) != (expected_cluster, expected_size):
        fail(f"source is not the expected PC-Engine 1.1 layout: {source_name}")


fixed_date = ((2026 - 1980) << 9) | (1 << 5) | 1
payloads = sorted((path for path in payload_path.iterdir() if path.is_file()),
                  key=lambda path: path.name)
added_bytes = 0

for payload in payloads:
    raw_name = short_name(payload.name)
    root_offset, existed = find_root_entry(raw_name)
    if existed:
        attributes = root[root_offset + 11]
        if attributes & 0x18:
            fail(f"refusing to replace a directory or volume label: {payload.name}")
        old_cluster = struct.unpack_from("<H", root, root_offset + 26)[0]
        if old_cluster:
            release_chain(old_cluster)

    contents = payload.read_bytes()
    cluster_count = (len(contents) + SECTOR_SIZE - 1) // SECTOR_SIZE
    free_clusters = [cluster for cluster in range(2, LAST_DATA_CLUSTER + 1)
                     if fat_get(cluster) == 0][:cluster_count]
    if len(free_clusters) != cluster_count:
        fail(f"not enough free space for {payload.name}")

    for index, cluster in enumerate(free_clusters):
        following = 0xFFF if index + 1 == cluster_count else free_clusters[index + 1]
        fat_set(cluster, following)
        lba = DATA_START_LBA + cluster - 2
        data_offset = normal_sectors[lba][0]
        begin = index * SECTOR_SIZE
        chunk = contents[begin:begin + SECTOR_SIZE]
        image[data_offset:data_offset + SECTOR_SIZE] = chunk.ljust(SECTOR_SIZE, b"\x00")

    entry = bytearray(32)
    entry[:11] = raw_name
    entry[11] = 0x20
    struct.pack_into("<H", entry, 14, 0)
    struct.pack_into("<H", entry, 16, fixed_date)
    struct.pack_into("<H", entry, 18, fixed_date)
    struct.pack_into("<H", entry, 22, 0)
    struct.pack_into("<H", entry, 24, fixed_date)
    struct.pack_into("<H", entry, 26, free_clusters[0] if free_clusters else 0)
    struct.pack_into("<I", entry, 28, len(contents))
    root[root_offset:root_offset + 32] = entry
    added_bytes += len(contents)

write_lbas(1, fat)
write_lbas(3, fat)
write_lbas(ROOT_START_LBA, root)

if bytes(read_lbas(0, 1)) != boot_sector:
    fail("internal error: boot sector changed")

image_path.write_bytes(image)
free_clusters = sum(fat_get(cluster) == 0
                    for cluster in range(2, LAST_DATA_CLUSTER + 1))
print(f"Installed {len(payloads)} files ({added_bytes} bytes)")
print(f"Remaining FAT12 space: {free_clusters * SECTOR_SIZE} bytes")
PY

chmod 0644 "$output_tmp"
mv -- "$output_tmp" "$output_d88"
output_tmp=

printf 'Created bootable development disk: %s\n' "$output_d88"
