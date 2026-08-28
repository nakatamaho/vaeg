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
lsic_archive=${VAEG_LSIC_ARCHIVE:-${VAEG_PC88VA_SOFTLIB_CACHE:-${XDG_CACHE_HOME:-${HOME}/.cache}/vaeg/pc88va-softlib-archive-disk}/LSIC330C.LZH}
cpm_archive=${VAEG_CPM_EXECUTOR_ARCHIVE:-${VAEG_PC88VA_SOFTLIB_CACHE:-${XDG_CACHE_HOME:-${HOME}/.cache}/vaeg/cpm08}/cpm08.zip}
mo_cache=${VAEG_PC88VA_MO_CACHE:-${XDG_CACHE_HOME:-${HOME}/.cache}/vaeg/pc88va-development-disk}
mo_schd_archive=${VAEG_MO_SCHD_ARCHIVE:-$mo_cache/schd155t.lzh}
mo_va128mo_archive=${VAEG_MO_VA128MO_ARCHIVE:-$mo_cache/va128mo.lzh}
mo_stest_archive=${VAEG_MO_STEST_ARCHIVE:-$mo_cache/stest115.lzh}
jwasm_archive=${VAEG_JWASM_ARCHIVE:-${XDG_CACHE_HOME:-${HOME}/.cache}/vaeg/jwasm/JWasm_v220_dos.zip}
cpm_tools_d88=${VAEG_CPM_TOOLS_D88:-${HOME}/88VA/images/cpm/cpmva-tools.d88}
cpm_source_d88=${VAEG_CPM_SOURCE_D88:-${HOME}/88VA/images/cpm/cpmva-source.d88}
cpm_dev_d88=${VAEG_CPM_DEV_D88:-${HOME}/88VA/images/cpm/cpmva-dev.d88}
work_dir=
jwasm_tmp=

cleanup() {
	if [[ -n ${jwasm_tmp} && -f ${jwasm_tmp} ]]; then
		rm -f -- "$jwasm_tmp"
	fi
	if [[ -n ${work_dir} && ${work_dir} == "${TMPDIR:-/tmp}"/vaeg-pc88va-sasi.* && -d ${work_dir} ]]; then
		rm -rf -- "$work_dir"
	fi
}

trap cleanup EXIT HUP INT TERM

usage() {
	cat <<EOF
Usage: $program_name [--source-va PATH] [--source-va2 PATH]
       [--payload-d88 PATH] [--lsic-archive PATH] [--cpm-archive PATH]
       [--jwasm-archive PATH]
       [--mo-schd-archive PATH] [--mo-va128mo-archive PATH]
       [--mo-stest-archive PATH]
       [--cpm-tools-d88 PATH] [--cpm-source-d88 PATH] [--cpm-dev-d88 PATH]
       [--output-dir DIR]

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
regenerated from the documented VA load order.  LSIC330C.LZH is verified,
retained in A:\\ARCHIVE, and extracted below A:\\LSIC86 for use through MSE.
The archive defaults to the verified softlib cache; use --lsic-archive to
select another copy.  The CP/M emulator defaults to the verified cpm08.zip
cache, and the three preserved CP/M data disks default to ~/88VA/images/cpm;
use the CP/M options to select other copies.
The SCHD155T, VA128MO, and STEST115 archives default to the verified
pc88va-development-disk cache; use the --mo-* options or VAEG_MO_*_ARCHIVE
variables to select explicit copies.  The archives are retained under
A:\\ARCHIVE and their documented files are installed under BIN, DOC, and SYS.
JWasm_v220_dos.zip defaults to the pinned free JWasm release cache (or
VAEG_JWASM_ARCHIVE); JWASMR.EXE, its readme/license, and the original archive
are installed under BIN, DOC, and ARCHIVE.
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
	--lsic-archive)
		(($# >= 2)) || die '--lsic-archive requires a path'
		lsic_archive=$2
		shift 2
		;;
	--jwasm-archive)
		(($# >= 2)) || die '--jwasm-archive requires a path'
		jwasm_archive=$2
		shift 2
		;;
	--mo-schd-archive)
		(($# >= 2)) || die '--mo-schd-archive requires a path'
		mo_schd_archive=$2
		shift 2
		;;
	--mo-va128mo-archive)
		(($# >= 2)) || die '--mo-va128mo-archive requires a path'
		mo_va128mo_archive=$2
		shift 2
		;;
	--mo-stest-archive)
		(($# >= 2)) || die '--mo-stest-archive requires a path'
		mo_stest_archive=$2
		shift 2
		;;
	--cpm-archive)
		(($# >= 2)) || die '--cpm-archive requires a path'
		cpm_archive=$2
		shift 2
		;;
	--cpm-tools-d88)
		(($# >= 2)) || die '--cpm-tools-d88 requires a path'
		cpm_tools_d88=$2
		shift 2
		;;
	--cpm-source-d88)
		(($# >= 2)) || die '--cpm-source-d88 requires a path'
		cpm_source_d88=$2
		shift 2
		;;
	--cpm-dev-d88)
		(($# >= 2)) || die '--cpm-dev-d88 requires a path'
		cpm_dev_d88=$2
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

ensure_jwasm_archive() {
	local parent
	local actual
	parent=$(dirname -- "$jwasm_archive")
	mkdir -p -- "$parent"
	if [[ -f $jwasm_archive ]]; then
		actual=$(sha256sum -- "$jwasm_archive")
		actual=${actual%% *}
		[[ $actual == e4cab76e0cdc038e4bc284be136cbd0e5116b02a0a2a76fc4a12cad326224723 ]] ||
			die "JWasm archive has the wrong SHA-256: $jwasm_archive"
		return
	fi
	command -v curl >/dev/null 2>&1 || die 'required host command is missing: curl'
	command -v sha256sum >/dev/null 2>&1 || die 'required host command is missing: sha256sum'
	jwasm_tmp=$(mktemp "$parent/JWasm_v220_dos.zip.part.XXXXXX")
	if ! curl --fail --location --silent --show-error --retry 3 \
		--connect-timeout 20 --output "$jwasm_tmp" \
		https://github.com/Baron-von-Riedesel/JWasm/releases/download/v2.20/JWasm_v220_dos.zip; then
		die 'download failed: JWasm_v220_dos.zip'
	fi
	actual=$(sha256sum -- "$jwasm_tmp")
	actual=${actual%% *}
	[[ $actual == e4cab76e0cdc038e4bc284be136cbd0e5116b02a0a2a76fc4a12cad326224723 ]] ||
		die "downloaded JWasm archive has the wrong SHA-256: $actual"
	mv -- "$jwasm_tmp" "$jwasm_archive"
	jwasm_tmp=
}

[[ -f $source_va && -r $source_va ]] || die "VA source D88 is not readable: $source_va"
[[ -f $source_va2 && -r $source_va2 ]] || die "VA2 source D88 is not readable: $source_va2"
[[ -f $payload_d88 && -r $payload_d88 ]] ||
	die "development payload D88 is not readable: $payload_d88"
[[ -f $lsic_archive && -r $lsic_archive ]] ||
	die "LSI-C archive is not readable: $lsic_archive (use --lsic-archive)"
[[ -f $cpm_archive && -r $cpm_archive ]] ||
	die "CP/M emulator archive is not readable: $cpm_archive (use --cpm-archive)"
[[ -f $mo_schd_archive && -r $mo_schd_archive ]] ||
	die "SCHD155T archive is not readable: $mo_schd_archive (use --mo-schd-archive)"
[[ -f $mo_va128mo_archive && -r $mo_va128mo_archive ]] ||
	die "VA128MO archive is not readable: $mo_va128mo_archive (use --mo-va128mo-archive)"
[[ -f $mo_stest_archive && -r $mo_stest_archive ]] ||
	die "STEST115 archive is not readable: $mo_stest_archive (use --mo-stest-archive)"
[[ -f $cpm_tools_d88 && -r $cpm_tools_d88 ]] ||
	die "CP/M tools D88 is not readable: $cpm_tools_d88 (use --cpm-tools-d88)"
[[ -f $cpm_source_d88 && -r $cpm_source_d88 ]] ||
	die "CP/M source D88 is not readable: $cpm_source_d88 (use --cpm-source-d88)"
[[ -f $cpm_dev_d88 && -r $cpm_dev_d88 ]] ||
	die "CP/M development D88 is not readable: $cpm_dev_d88 (use --cpm-dev-d88)"
[[ -f $builder && -r $builder ]] || die "builder is not readable: $builder"
ensure_jwasm_archive
mkdir -p -- "$output_dir"

work_dir=$(mktemp -d "${TMPDIR:-/tmp}/vaeg-pc88va-sasi.XXXXXX")
lsic_tree=$work_dir/lsic
mkdir -p -- "$lsic_tree"
lha xfw="$lsic_tree" "$lsic_archive" >/dev/null
mo_schd_tree=$work_dir/mo-schd
mo_va128mo_tree=$work_dir/mo-va128mo
mo_stest_tree=$work_dir/mo-stest
mkdir -p -- "$mo_schd_tree" "$mo_va128mo_tree" "$mo_stest_tree"
lha xfw="$mo_schd_tree" "$mo_schd_archive" >/dev/null
lha xfw="$mo_va128mo_tree" "$mo_va128mo_archive" >/dev/null
lha xfw="$mo_stest_tree" "$mo_stest_archive" >/dev/null

python3 "$builder" --variant va --source "$source_va" \
	--payload-d88 "$payload_d88" \
	--jwasm-archive "$jwasm_archive" \
	--lsic-archive "$lsic_archive" --lsic-tree "$lsic_tree" \
	--mo-schd-archive "$mo_schd_archive" --mo-schd-tree "$mo_schd_tree" \
	--mo-va128mo-archive "$mo_va128mo_archive" --mo-va128mo-tree "$mo_va128mo_tree" \
	--mo-stest-archive "$mo_stest_archive" --mo-stest-tree "$mo_stest_tree" \
	--cpm-archive "$cpm_archive" --cpm-tools-d88 "$cpm_tools_d88" \
	--cpm-source-d88 "$cpm_source_d88" --cpm-dev-d88 "$cpm_dev_d88" \
	--output "$output_dir/pc88va-sasi-40mb-va.hdi"
python3 "$builder" --variant va2 --source "$source_va2" \
	--payload-d88 "$payload_d88" \
	--jwasm-archive "$jwasm_archive" \
	--lsic-archive "$lsic_archive" --lsic-tree "$lsic_tree" \
	--mo-schd-archive "$mo_schd_archive" --mo-schd-tree "$mo_schd_tree" \
	--mo-va128mo-archive "$mo_va128mo_archive" --mo-va128mo-tree "$mo_va128mo_tree" \
	--mo-stest-archive "$mo_stest_archive" --mo-stest-tree "$mo_stest_tree" \
	--cpm-archive "$cpm_archive" --cpm-tools-d88 "$cpm_tools_d88" \
	--cpm-source-d88 "$cpm_source_d88" --cpm-dev-d88 "$cpm_dev_d88" \
	--output "$output_dir/pc88va-sasi-40mb-va2.hdi"

cleanup
work_dir=
