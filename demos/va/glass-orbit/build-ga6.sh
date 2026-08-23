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
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
# WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

set -eu

if [ "$#" -ne 1 ]; then
    printf 'usage: %s OUTPUT.COM\n' "$0" >&2
    exit 2
fi

output=$1
output_dir=$(CDPATH= cd -- "$(dirname -- "$output")" && pwd)
output_name=$(basename -- "$output")
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
source_dir=$script_dir/src
assembler=${NASM:-nasm}

command -v "$assembler" >/dev/null 2>&1 || {
    printf 'error: NASM is unavailable: %s\n' "$assembler" >&2
    exit 1
}

build_payload() {
    name=$1
    definition=$2
    "$assembler" -f bin -O2 -I "$source_dir/" $definition \
        "$source_dir/glass_orbit_ga6.asm" -o "$output_dir/$name"
    payload_size=$(wc -c < "$output_dir/$name" | tr -d ' ')
    if [ "$payload_size" -gt 61184 ]; then
        printf 'error: GA-6 raw payload exceeds fixed stack boundary: %s bytes\n' \
            "$payload_size" >&2
        exit 1
    fi
}

build_loader() {
    payload=$1
    loader=$2
    (
        cd "$output_dir"
        "$assembler" -f bin -O2 -I "$source_dir/" \
            -dGLASS_GA6_PAYLOAD_FILE=\"$payload\" \
            "$source_dir/glass_orbit_ga6_loader.asm" -o "$loader"
    )
    com_size=$(wc -c < "$output_dir/$loader" | tr -d ' ')
    if [ "$com_size" -gt 65280 ]; then
        printf 'error: GA-6 loader exceeds the DOS COM size limit: %s bytes\n' \
            "$com_size" >&2
        exit 1
    fi
}

build_payload GLASSG6.BIN ''
build_payload GLASSG6A.BIN '-dGLASS_GA6_CAPTURE_PAGE=0'
build_payload GLASSG6B.BIN '-dGLASS_GA6_CAPTURE_PAGE=1'
build_loader GLASSG6.BIN "$output_name"
build_loader GLASSG6A.BIN GLASSP6A.COM
build_loader GLASSG6B.BIN GLASSP6B.COM

printf 'Built GLASS ORBIT GA-6 interactive loader: %s\n' "$output"
printf 'Built GLASS ORBIT GA-6 page-A capture loader: %s\n' "$output_dir/GLASSP6A.COM"
printf 'Built GLASS ORBIT GA-6 page-B capture loader: %s\n' "$output_dir/GLASSP6B.COM"
