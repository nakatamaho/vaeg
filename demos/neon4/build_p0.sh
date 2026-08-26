#!/bin/sh
# ---------------------------------------------------------------------------
# Copyright (c) 2026 Nakata Maho
#
# This file is licensed under the BSD 2-Clause License.  See the repository
# license for the complete terms.  Ported By Maho Nakata.
# ---------------------------------------------------------------------------
set -eu

root_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
out_file=${1:-"$root_dir/neon4-p0.com"}

nasm -f bin -O2 -I "$root_dir/src" \
    "$root_dir/src/neon4_p0.asm" -o "$out_file"

printf 'built %s (%s bytes)\n' "$out_file" "$(wc -c < "$out_file" | tr -d ' ')"
