#!/bin/sh
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
#
# Build the PC-9801B3-E02 probes (M148, README.md) and a bootable MS-DOS
# 4.0 disk with them in its root:
#   build.sh [--msdos4 msdos4-pc88va-2hd.d88] OUTDIR
# writes OUTDIR/B3E0.COM ... and OUTDIR/b3e02-msdos4.d88. The MS-DOS 4.0
# image defaults to the shared cache
# ($VAEG_CACHE_DIR/msdos4-va.2/dl/msdos4-pc88va-2hd.d88) and must be the
# msdos4-va.2 release image. The build id is the first 8 hex digits of the
# checkout's HEAD (BUILD_ID overrides). Everything stays out of Git.

set -eu

nasm_command=${NASM:-nasm}
python_command=${PYTHON:-python3}
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_root=$(CDPATH= cd -- "$script_dir/../.." && pwd)
cache=${VAEG_CACHE_DIR:-${HOME}/work/pc88va/pc88va-private-docs/pc88va-utilities-cache}
msdos4=$cache/msdos4-va.2/dl/msdos4-pc88va-2hd.d88
if [ "${1:-}" = "--msdos4" ]; then
	msdos4=$2
	shift 2
fi
if [ "$#" -ne 1 ]; then
	printf 'usage: %s [--msdos4 IMAGE] OUTDIR\n' "$0" >&2
	exit 2
fi
outdir=$1
image=$outdir/b3e02-msdos4.d88
mkdir -p "$outdir"
[ ! -e "$image" ] || { printf 'error: %s exists\n' "$image" >&2; exit 1; }
[ -f "$msdos4" ] || { printf 'error: MS-DOS 4.0 image not found: %s\n' "$msdos4" >&2; exit 1; }
echo '7c4b141d31e0034120e0b07eb93b9bea808e1c54822e48e15189cc7398385db0  '"$msdos4" |
	sha256sum -c - >/dev/null || { printf 'error: not the msdos4-va.2 release image\n' >&2; exit 1; }

build_id=${BUILD_ID:-$(git -C "$script_dir" rev-parse HEAD 2>/dev/null | cut -c1-8 || true)}
[ -n "$build_id" ] || build_id=nogit000

work=$(mktemp -d "${TMPDIR:-/tmp}/vaeg-b3e02.XXXXXX")
trap 'rm -rf "$work"' EXIT
root=$work/payload/root
mkdir -p "$root"
for name in B3E0 B3E1 B3E2 B3E3 B3E4 B3E5; do
	lower=$(printf '%s' "$name" | tr 'A-Z' 'a-z')
	"$nasm_command" -f bin -I"$script_dir/" "-DBUILD_ID='$build_id'" \
		"$script_dir/tests/$lower.asm" -o "$outdir/$name.COM"
	cp "$outdir/$name.COM" "$root/"
done
# CR LF copy of the operating instructions.
sed 's/$/\r/' "$script_dir/b3e02.txt" >"$root/B3E02.TXT"
"$python_command" -I "$repo_root/tools/pc88va/pcengine_disk.py" vanilla --source "$msdos4" --output "$image"
"$python_command" -I "$repo_root/tools/pc88va/pcengine_disk.py" install --image "$image" --payload "$work/payload"
printf 'build id %s\n' "$build_id"
(cd "$outdir" && sha256sum B3E0.COM B3E1.COM B3E2.COM B3E3.COM B3E4.COM B3E5.COM b3e02-msdos4.d88)
