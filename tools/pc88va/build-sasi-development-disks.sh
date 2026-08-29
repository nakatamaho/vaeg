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
softlib_cache=${VAEG_PC88VA_SOFTLIB_CACHE:-${XDG_CACHE_HOME:-${HOME}/.cache}/vaeg/pc88va-softlib-archive-disk}
lsic_archive=${VAEG_LSIC_ARCHIVE:-$softlib_cache/LSIC330C.LZH}
ish_archive=${VAEG_ISH_ARCHIVE:-$softlib_cache/ISHARC.COM}
ish_doc=${VAEG_ISH_DOC:-$softlib_cache/ISHARC.DOC}
infozip_unzip_archive=${VAEG_INFOZIP_UNZIP_ARCHIVE:-$softlib_cache/UNZ532X3.EXE}
infozip_zip_archive=${VAEG_INFOZIP_ZIP_ARCHIVE:-$softlib_cache/ZIP22X.ZIP}
emacs_archive=${VAEG_EMACSVA_ARCHIVE:-$softlib_cache/EMACSVA.LZH}
cpmva_archive=${VAEG_CPMVA_ARCHIVE:-$softlib_cache/CPMVA.LZH}
tdc_archive=${VAEG_TDC_ARCHIVE:-$softlib_cache/TDC10.LZH}
bench_archive=${VAEG_BENCH_ARCHIVE:-$softlib_cache/BENCH003.LZH}
unix_tools_archive=${VAEG_UNIX_TOOLS_ARCHIVE:-$softlib_cache/UXTL412H.TGZ}
two_hc_source_archive=${VAEG_2HCDR_SOURCE_ARCHIVE:-$softlib_cache/2HCDRSRC.LZH}
two_hc_driver_archive=${VAEG_2HCDR_DRIVER_ARCHIVE:-$softlib_cache/2HCDRV.ZIP}
pcepat_source_archive=${VAEG_PCEPAT_SOURCE_ARCHIVE:-$softlib_cache/PCPATSRC.ZIP}
tsclv_source_archive=${VAEG_TSCLV_SOURCE_ARCHIVE:-$softlib_cache/TSCLVSRC.LZH}
s88valsi_archive=${VAEG_S88VALSI_ARCHIVE:-$softlib_cache/S88VALSI.LZH}
s88valsi_doc=${VAEG_S88VALSI_DOC:-$softlib_cache/S88VALSI.DOC}
s88va250_archive=${VAEG_S88VA250_ARCHIVE:-$softlib_cache/S88VA250.LZH}
s88va250_doc=${VAEG_S88VA250_DOC:-$softlib_cache/S88VA250.DOC}
stest_source_archive=${VAEG_STEST_SOURCE_ARCHIVE:-$softlib_cache/ST115SRC.LZH}
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
download_tmp=

cleanup() {
	if [[ -n ${jwasm_tmp} && -f ${jwasm_tmp} ]]; then
		rm -f -- "$jwasm_tmp"
	fi
	if [[ -n ${download_tmp} && -f ${download_tmp} ]]; then
		rm -f -- "$download_tmp"
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
       [--ish-archive PATH] [--ish-doc PATH]
       [--infozip-unzip-archive PATH] [--infozip-zip-archive PATH]
       [--emacs-archive PATH] [--cpmva-archive PATH]
	       [--tdc-archive PATH] [--bench-archive PATH]
	       [--unix-tools-archive PATH]
	       [--2hc-source-archive PATH] [--2hc-driver-archive PATH]
	       [--pcepat-source-archive PATH] [--tsclv-source-archive PATH]
	       [--s88valsi-archive PATH] [--s88valsi-doc PATH]
	       [--s88va250-archive PATH] [--s88va250-doc PATH]
	       [--stest-source-archive PATH]
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
The verified ISHARC package installs ISHVA, PKPAK, and PKUNPAK.  The 16-bit
Info-ZIP packages install ZIP and UNZIP with their manuals.  EMACSVA, CPMVA,
	TDC, and BENCH are also expanded into BIN/DOC; their source archives are kept
	under ARCHIVE.  These packages default to the verified softlib cache and may
	be overridden with the corresponding options or VAEG_*_ARCHIVE variables.
The SASI-only 2HCDRSRC/2HCDRV, PCPATSRC, TSCLVSRC, S88VALSI, and S88VA250
source/library packages are retained verbatim under A:\\ARCHIVE without
expansion.  The runnable 2HCDRV.COM and FDFORM.COM tools are installed in
A:\\BIN, but 2HCDRV.COM is not loaded by the generated CONFIG.SYS because its
resident startup path can reset VAEG; these source packages are not added to
FDD.
The Vector UNIX-like tools 4.12h package is staged below A:\\UNIX\\BIN with
its manuals and source documentation below A:\\UNIX\\MAN and A:\\UNIX\\DOC;
the original TGZ is retained under A:\\ARCHIVE.  Its path is added after
A:\\BIN so existing VA utilities keep their historical command names.
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
	--ish-archive)
		(($# >= 2)) || die '--ish-archive requires a path'
		ish_archive=$2
		shift 2
		;;
	--ish-doc)
		(($# >= 2)) || die '--ish-doc requires a path'
		ish_doc=$2
		shift 2
		;;
	--infozip-unzip-archive)
		(($# >= 2)) || die '--infozip-unzip-archive requires a path'
		infozip_unzip_archive=$2
		shift 2
		;;
	--infozip-zip-archive)
		(($# >= 2)) || die '--infozip-zip-archive requires a path'
		infozip_zip_archive=$2
		shift 2
		;;
	--emacs-archive)
		(($# >= 2)) || die '--emacs-archive requires a path'
		emacs_archive=$2
		shift 2
		;;
	--cpmva-archive)
		(($# >= 2)) || die '--cpmva-archive requires a path'
		cpmva_archive=$2
		shift 2
		;;
	--tdc-archive)
		(($# >= 2)) || die '--tdc-archive requires a path'
		tdc_archive=$2
		shift 2
		;;
	--bench-archive)
		(($# >= 2)) || die '--bench-archive requires a path'
		bench_archive=$2
		shift 2
		;;
	--unix-tools-archive)
		(($# >= 2)) || die '--unix-tools-archive requires a path'
		unix_tools_archive=$2
		shift 2
		;;
	--2hc-source-archive)
		(($# >= 2)) || die '--2hc-source-archive requires a path'
		two_hc_source_archive=$2
		shift 2
		;;
	--2hc-driver-archive)
		(($# >= 2)) || die '--2hc-driver-archive requires a path'
		two_hc_driver_archive=$2
		shift 2
		;;
	--pcepat-source-archive)
		(($# >= 2)) || die '--pcepat-source-archive requires a path'
		pcepat_source_archive=$2
		shift 2
		;;
	--tsclv-source-archive)
		(($# >= 2)) || die '--tsclv-source-archive requires a path'
		tsclv_source_archive=$2
		shift 2
		;;
	--s88valsi-archive)
		(($# >= 2)) || die '--s88valsi-archive requires a path'
		s88valsi_archive=$2
		shift 2
		;;
	--s88valsi-doc)
		(($# >= 2)) || die '--s88valsi-doc requires a path'
		s88valsi_doc=$2
		shift 2
		;;
	--s88va250-archive)
		(($# >= 2)) || die '--s88va250-archive requires a path'
		s88va250_archive=$2
		shift 2
		;;
	--s88va250-doc)
		(($# >= 2)) || die '--s88va250-doc requires a path'
		s88va250_doc=$2
		shift 2
		;;
	--stest-source-archive)
		(($# >= 2)) || die '--stest-source-archive requires a path'
		stest_source_archive=$2
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

ensure_cached_package() {
	local path=$1
	local expected=$2
	local url=$3
	local parent
	local name
	local actual

	parent=$(dirname -- "$path")
	name=$(basename -- "$path")
	mkdir -p -- "$parent"
	if [[ -f $path ]]; then
		actual=$(sha256sum -- "$path")
		actual=${actual%% *}
		[[ $actual == "$expected" ]] ||
			die "cached package has the wrong SHA-256: $path"
		return
	fi
	command -v curl >/dev/null 2>&1 || die 'required host command is missing: curl'
	command -v sha256sum >/dev/null 2>&1 || die 'required host command is missing: sha256sum'
	download_tmp=$(mktemp "$parent/.${name}.part.XXXXXX")
	if ! curl --fail --location --silent --show-error --retry 3 \
		--connect-timeout 20 --output "$download_tmp" "$url"; then
		die "download failed: $name"
	fi
	actual=$(sha256sum -- "$download_tmp")
	actual=${actual%% *}
	[[ $actual == "$expected" ]] ||
		die "downloaded package has the wrong SHA-256: $name ($actual)"
	mv -- "$download_tmp" "$path"
	download_tmp=
}

[[ -f $source_va && -r $source_va ]] || die "VA source D88 is not readable: $source_va"
[[ -f $source_va2 && -r $source_va2 ]] || die "VA2 source D88 is not readable: $source_va2"
[[ -f $payload_d88 && -r $payload_d88 ]] ||
	die "development payload D88 is not readable: $payload_d88"
ensure_cached_package "$lsic_archive" \
	c8c4c49aed600fb2413cf5707ef01b2f4057de69196c3478d5226bf1b224081b \
	'https://ftp.vector.co.jp/00/11/980/lsic330c.lzh'
ensure_cached_package "$ish_archive" \
	c51ffc66551f55872532b0ebde186ac7e9f76ef9c8287c731190f7e433bba61f \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=69&fname=ISHARC.COM'
ensure_cached_package "$ish_doc" \
	d3ec746ac0ce93bd9b4995a0fd1cb53de3289b914a7019c47e7b01d440657fc3 \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=69&fname=ISHARC.DOC'
ensure_cached_package "$infozip_unzip_archive" \
	cb55dee22473caf143353938da76e61d5574c5edead7b321c14b8900c0b493ce \
	'https://www.ibiblio.org/pub/micro/pc-stuff/freedos/mirrors/gnuish/dos_only/unz532x3.exe'
ensure_cached_package "$infozip_zip_archive" \
	f0048e0003d0a115624c086e9570355b8689cdd1376b4f6fc7dc4f55cb6eb9a5 \
	'https://www.ibiblio.org/pub/micro/pc-stuff/freedos/mirrors/gnuish/dos_only/zip22x.zip'
ensure_cached_package "$emacs_archive" \
	64d496d67668f7d5bd071ff304ed33a689b0795fa793afac9e631346070c8a8a \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=435&fname=EMACSVA.LZH'
ensure_cached_package "$cpmva_archive" \
	c5188efa73c80609e2184890d5a1ee5f0b274f8d29a3d73ae370fe7526d9dccd \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=424&fname=CPMVA.LZH'
ensure_cached_package "$tdc_archive" \
	c6c31cf6a604b07220c88a010bcd2e40cdb009dd4601e7d8a0ad903a4f2df23e \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=201&fname=TDC10.LZH'
ensure_cached_package "$bench_archive" \
	40f5fbf391d416a79d843c13e11797abfd8ff49ea45a7d6f8a627cb389d9c79c \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=389&fname=BENCH003.LZH'
ensure_cached_package "$unix_tools_archive" \
	dfc2b671cdbf7287cd845cf8166317ad0ecb13e720e50f8da858d3292316396f \
	'https://ftp.vector.co.jp/50/03/2177/uxtl412h.tgz'
ensure_cached_package "$two_hc_source_archive" \
	69a380af1ee74ee9d4e2fc6d536d4aa87aba0280c52808863e6cc3f41be2331e \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=400&fname=2HCDRSRC.LZH'
ensure_cached_package "$two_hc_driver_archive" \
	1da4d799b1aaf3a2fc94f8872eb3fd2cf6eb788fb907e8ba8c85cc79b0487e39 \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=306&fname=2HCDRV.ZIP'
ensure_cached_package "$pcepat_source_archive" \
	089cd52e5bc936e7feb30b53a39d2cc6e1e18b1ced64ec08b61a0e8b736b8815 \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=331&fname=PCPATSRC.ZIP'
ensure_cached_package "$tsclv_source_archive" \
	a571a4a885a1dc71351f5b8d52405cbfaf72c1db2bfc745885f1cb5b305bf76e \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=403&fname=TSCLVSRC.LZH'
ensure_cached_package "$s88valsi_archive" \
	5c12dd438f99c20f0f45235d5156adea93f3c9bdb26405c2f7211cc163e7559b \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=436&fname=S88VALSI.LZH'
ensure_cached_package "$s88valsi_doc" \
	92d493bbc617ed1aca0055be13a2cdc35b80bff3d16b44aa23ebe228b3346946 \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=436&fname=S88VALSI.DOC'
ensure_cached_package "$s88va250_archive" \
	3ba8e94c8263a0e90826f4f751e80aacfb8e81e952c2bce14722ba4a74cda9eb \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=382&fname=S88VA250.LZH'
ensure_cached_package "$s88va250_doc" \
	66727a56813997f8484d08c0c40ba211efe2b8cc881350a5f58f63d78eb9e355 \
	'http://www.pc88.gr.jp/softlib/index.php?action=download&anum=2&gnum=382&fname=S88VA250.DOC'
ensure_cached_package "$stest_source_archive" \
	1192d3a38a4d9444a9b8b021fcd550e61e7b860bc39b67a01868c58e62bc2e51 \
	'https://www2u.biglobe.ne.jp/~pumpkin/hlabo/osl/driver/ST115SRC.LZH'
ensure_cached_package "$mo_stest_archive" \
	6ae981b0010df20a510f85165567add33032241854b147ed47937a59953010bc \
	'https://www2u.biglobe.ne.jp/~pumpkin/hlabo/osl/driver/STEST115.LZH'
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

# Older payload D88 files predate the complete development-tool set.  Stage
# the verified packages through the common stager so the FDD and SASI
# injectors use one extraction and destination manifest.
supplemental_tree=$work_dir/supplemental
supplemental_manifest=$work_dir/supplemental.manifest.tsv
"$script_dir/stage-development-tools.sh" --output "$supplemental_tree" \
	--manifest "$supplemental_manifest" \
	--ish-archive "$ish_archive" --ish-doc "$ish_doc" \
	--infozip-unzip-archive "$infozip_unzip_archive" \
	--infozip-zip-archive "$infozip_zip_archive" \
	--emacs-archive "$emacs_archive" --cpmva-archive "$cpmva_archive" \
	--tdc-archive "$tdc_archive" --bench-archive "$bench_archive" \
	--unix-tools-archive "$unix_tools_archive" \
	--2hc-source-archive "$two_hc_source_archive" \
	--2hc-driver-archive "$two_hc_driver_archive" \
	--pcepat-source-archive "$pcepat_source_archive" \
	--tsclv-source-archive "$tsclv_source_archive" \
	--s88valsi-archive "$s88valsi_archive" --s88valsi-doc "$s88valsi_doc" \
		--s88va250-archive "$s88va250_archive" --s88va250-doc "$s88va250_doc" \
		--stest-source-archive "$stest_source_archive"

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
	--supplemental-tree "$supplemental_tree" \
	--supplemental-manifest "$supplemental_manifest" \
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
	--supplemental-tree "$supplemental_tree" \
	--supplemental-manifest "$supplemental_manifest" \
	--output "$output_dir/pc88va-sasi-40mb-va2.hdi"

cleanup
work_dir=
