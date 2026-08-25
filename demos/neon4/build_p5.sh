#!/bin/sh
# Copyright (c) 2026 Nakata Maho
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS" AND ANY EXPRESS
# OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

set -eu

if [ "$#" -ne 1 ]; then
    printf 'usage: %s OUTPUT.COM\n' "$0" >&2
    exit 2
fi

assembler=${NASM:-nasm}
command -v "$assembler" >/dev/null 2>&1 || {
    printf 'error: NASM is unavailable: %s\n' "$assembler" >&2
    exit 127
}

scene=${NEON4_P5_SCENE:-0}
case "$scene" in
    0|1) ;;
    *) printf 'error: NEON4_P5_SCENE must be 0 or 1\n' >&2; exit 2 ;;
esac

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
output_dir=$(CDPATH= cd -- "$(dirname -- "$1")" && pwd)
output_path=$output_dir/$(basename -- "$1")
raw_path=$output_dir/.$(basename -- "$1").raw

"$assembler" -f bin -O2 \
    -dNEON4_STAGE=8 \
    -dNEON4_P5_SCENE="$scene" \
    -I "$script_dir/src/" \
    "$script_dir/src/neon4_p3.asm" -o "$raw_path"

"$assembler" -f bin -O2 \
    -dNEON_PAYLOAD_FILE="\"$raw_path\"" \
    -I "$script_dir/../neon3/src/" \
    "$script_dir/../neon3/src/neon_payload_loader.asm" -o "$output_path"

rm -f "$raw_path"
printf '%s\n' "$output_path"
