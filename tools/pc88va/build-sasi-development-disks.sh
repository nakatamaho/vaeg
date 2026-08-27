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
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
# USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

set -euo pipefail

program_name=${0##*/}
script_dir=$(CDPATH= cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
repo_root=$(CDPATH= cd -- "$script_dir/../.." && pwd)
builder=$script_dir/build-sasi-development-disk.py
source_va=$repo_root/docs/disks/'PC-Engine 1.05.d88'
source_va2=$repo_root/docs/disks/'PC-Engine 1.1.d88'
payload_d88=$repo_root/docs/disks/pc88va-development.d88
output_dir=/private/tmp

usage() {
	cat <<EOF
Usage: $program_name [--source-va PATH] [--source-va2 PATH]
       [--payload-d88 PATH] [--output-dir DIR]

Build two 40 MB PC-88VA SASI HDI development disks:
  pc88va-sasi-40mb-va.hdi   PC-Engine 1.05
  pc88va-sasi-40mb-va2.hdi  PC-Engine 1.1

The source D88 files default to docs/disks under the repository.  Use the
source options when the preserved system disks are outside the repository.
The payload D88 defaults to the complete development disk at
docs/disks/pc88va-development.d88.  Its BIN, SYS, DOC, ARCHIVE, and TMP
directories are transplanted into both SASI images.  COM/EXE utilities from
each matching PC-Engine source D88 are also installed in that image's BIN;
the boot PCENGINE.COM remains at the root.  CONFIG.SYS and AUTOEXEC.BAT are
regenerated from the documented VA load order.
Generated images are never overwritten.
EOF
}

die() {
	printf 'error: %s\n' "$*" >&2
	exit 1
}

while (($#)); do
	case $1 in
	--source-va)
		(($# >= 2)) || die '--source-va requires a path'
		source_va=$2
		shift 2
		;;
	--source-va2)
		(($# >= 2)) || die '--source-va2 requires a path'
		source_va2=$2
		shift 2
		;;
	--payload-d88)
		(($# >= 2)) || die '--payload-d88 requires a path'
		payload_d88=$2
		shift 2
		;;
	--output-dir)
		(($# >= 2)) || die '--output-dir requires a directory'
		output_dir=$2
		shift 2
		;;
	-h|--help)
		usage
		exit 0
		;;
	*)
		die "unknown argument: $1"
		;;
	esac
done

[[ -f $source_va && -r $source_va ]] || die "VA source D88 is not readable: $source_va"
[[ -f $source_va2 && -r $source_va2 ]] || die "VA2 source D88 is not readable: $source_va2"
[[ -f $payload_d88 && -r $payload_d88 ]] ||
	die "development payload D88 is not readable: $payload_d88"
[[ -f $builder && -r $builder ]] || die "builder is not readable: $builder"
mkdir -p -- "$output_dir"

python3 "$builder" --variant va --source "$source_va" \
	--payload-d88 "$payload_d88" \
	--output "$output_dir/pc88va-sasi-40mb-va.hdi"
python3 "$builder" --variant va2 --source "$source_va2" \
	--payload-d88 "$payload_d88" \
	--output "$output_dir/pc88va-sasi-40mb-va2.hdi"
