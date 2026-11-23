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

# Build SGPBENCH.COM, CPUBENCH.COM and its CPU4.DAT, CPU8.DAT and
# CPU16.DAT (README.md) into OUTDIR (default .). With --msdos4 IMAGE
# (msdos4-pc88va-2hd.d88 of FreeDOS-88VA msdos4-va.2), also
# OUTDIR/sprite-bench-msdos4.d88: a vanilla copy with the programs.
# PREVIEW=path also writes the characters as a PPM.

set -eu

nasm_command=${NASM:-nasm}
python_command=${PYTHON:-python3}
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
msdos4=
if [ "${1:-}" = "--msdos4" ]; then
	msdos4=$2
	shift 2
fi
outdir=${1:-.}
mkdir -p "$outdir"
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

"$python_command" -I "$script_dir/gen_data.py" "$work/data.inc" --cpu "$outdir" \
	${PREVIEW:+--preview "$PREVIEW"}
for program in sgpbench cpubench; do
	upper=$(printf '%s' "$program" | tr a-z A-Z)
	"$nasm_command" -f bin -I"$work/" -I"$script_dir/" "$script_dir/$program.asm" \
		-o "$outdir/$upper.COM"
done
if [ -n "$msdos4" ]; then
	tool="$script_dir/../../tools/pc88va/pcengine_disk.py"
	mkdir -p "$work/payload/root"
	for f in SGPBENCH.COM CPUBENCH.COM CPU4.DAT CPU8.DAT CPU16.DAT; do
		cp "$outdir/$f" "$work/payload/root/"
	done
	rm -f "$outdir/sprite-bench-msdos4.d88"
	"$python_command" -I "$tool" vanilla --source "$msdos4" --output "$outdir/sprite-bench-msdos4.d88"
	"$python_command" -I "$tool" install --image "$outdir/sprite-bench-msdos4.d88" --payload "$work/payload"
fi
