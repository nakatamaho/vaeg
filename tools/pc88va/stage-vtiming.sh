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
# USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

set -euo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
if [[ $# != 2 || $1 != --output ]]; then
	printf 'Usage: %s --output DIR\n' "${0##*/}" >&2
	exit 1
fi
output_dir=$2
[[ ! -e $output_dir ]] || { printf 'error: output already exists\n' >&2; exit 1; }
command -v nasm >/dev/null
mkdir -p -- "$output_dir/BIN" "$output_dir/SRC/VTIMING" "$output_dir/DOC"
nasm -f bin -o "$output_dir/BIN/V480PAT.COM" "$script_dir/vtiming/v480pat.asm"
cp -- "$script_dir/vtiming/v480pat.asm" "$output_dir/SRC/VTIMING/V480PAT.ASM"
cp -- "$script_dir/vtiming/vtiming.txt" "$output_dir/DOC/VTIMING.TXT"
printf 'Staged V480PAT executable, source and instructions\n'
