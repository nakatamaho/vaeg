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

if [ "$#" -gt 2 ]; then
    printf 'usage: %s [OUTPUT.COM [OUTPUT.LST]]\n' "$0" >&2
    exit 2
fi

nasm_command=${NASM:-nasm}
bounded_qa=${M98R_BOUNDED_QA:-0}
qa_cycles=${M98R_QA_CYCLES:-1}
qa_scenario=${M98R_QA_SCENARIO:-0}
initial_visible_page=${M98R_INITIAL_VISIBLE_PAGE:-0}
clear_mode=${M98R_CLEAR_MODE:-1}
output=${1:-ZUNDORB.COM}
listing=${2:-${output%.*}.LST}
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
output_parent=$(dirname -- "$output")

[ -d "$output_parent" ] || {
    printf 'error: output directory does not exist: %s\n' "$output_parent" >&2
    exit 1
}
command -v "$nasm_command" >/dev/null 2>&1 || {
    printf 'error: NASM command is unavailable\n' >&2
    exit 127
}
case "$bounded_qa" in
    0|1) ;;
    *) printf 'error: M98R_BOUNDED_QA must be 0 or 1\n' >&2; exit 2 ;;
esac
case "$qa_cycles" in
    1|2) ;;
    *) printf 'error: M98R_QA_CYCLES must be 1 or 2\n' >&2; exit 2 ;;
esac
case "$qa_scenario" in
    0|1|2|3) ;;
    *) printf 'error: M98R_QA_SCENARIO must be 0, 1, 2, or 3\n' >&2; exit 2 ;;
esac
case "$initial_visible_page" in
    0|1) ;;
    *) printf 'error: M98R_INITIAL_VISIBLE_PAGE must be 0 or 1\n' >&2; exit 2 ;;
esac
case "$clear_mode" in
    0|1) ;;
    *) printf 'error: M98R_CLEAR_MODE must be 0 or 1\n' >&2; exit 2 ;;
esac

"$nasm_command" -f bin \
    -dM98R_BOUNDED_QA="$bounded_qa" \
    -dM98R_QA_CYCLES="$qa_cycles" \
    -dM98R_QA_SCENARIO="$qa_scenario" \
    -dM98Q_INITIAL_VISIBLE_PAGE="$initial_visible_page" \
    -dM98Q_CLEAR_MODE="$clear_mode" \
    -l "$listing" \
    "$script_dir/zundamon_orbit_256.asm" -o "$output"

size=$(wc -c < "$output" | tr -d ' ')
[ "$size" -lt 65280 ] || {
    printf 'error: generated COM exceeds the 64-KiB DOS payload limit\n' >&2
    exit 1
}

printf 'M98R_GUEST_BUILD_PASS size=%s bounded_qa=%s cycles=%s scenario=%s initial_page=%s clear_mode=%s listing=%s\n' \
    "$size" "$bounded_qa" "$qa_cycles" "$qa_scenario" "$initial_visible_page" "$clear_mode" "$listing"
