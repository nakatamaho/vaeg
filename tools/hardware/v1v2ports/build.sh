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
# Build V1V2PORT.COM and a bootable MS-DOS 4.0 test disk (M144):
#   build.sh [--msdos4 msdos4-pc88va-2hd.d88] OUTPUT.d88
# The MS-DOS 4.0 image defaults to the shared cache
# ($VAEG_CACHE_DIR/msdos4-va.2/dl/msdos4-pc88va-2hd.d88).  The disk stays
# outside the repository.

set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_root=$(CDPATH= cd -- "$script_dir/../../.." && pwd)
cache=${VAEG_CACHE_DIR:-${HOME}/work/pc88va/pc88va-private-docs/pc88va-utilities-cache}
msdos4=$cache/msdos4-va.2/dl/msdos4-pc88va-2hd.d88
if [ "${1:-}" = "--msdos4" ]; then
	msdos4=$2
	shift 2
fi
if [ "$#" -ne 1 ]; then
	printf 'usage: %s [--msdos4 IMAGE] OUTPUT.d88\n' "$0" >&2
	exit 2
fi
output=$1
[ ! -e "$output" ] || { printf 'error: %s exists\n' "$output" >&2; exit 1; }
[ -f "$msdos4" ] || { printf 'error: MS-DOS 4.0 image not found: %s\n' "$msdos4" >&2; exit 1; }
echo '7c4b141d31e0034120e0b07eb93b9bea808e1c54822e48e15189cc7398385db0  '"$msdos4" |
	sha256sum -c - >/dev/null || { printf 'error: not the msdos4-va.2 release image\n' >&2; exit 1; }

work=$(mktemp -d "${TMPDIR:-/tmp}/vaeg-v1v2port.XXXXXX")
trap 'rm -rf "$work"' EXIT
mkdir -p "$work/payload/root"
nasm -f bin -o "$work/payload/root/V1V2PORT.COM" "$script_dir/v1v2port.asm"
python3 -I "$repo_root/tools/pc88va/pcengine_disk.py" vanilla --source "$msdos4" --output "$output"
python3 -I "$repo_root/tools/pc88va/pcengine_disk.py" install --image "$output" --payload "$work/payload"
sha256sum "$work/payload/root/V1V2PORT.COM" "$output"
