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


# M109: build the zundamon-orbit distribution disk from the free
# "ずんだもん立ち絵素材2.3.psd" (its stored first standing pose).
#   build-from-psd.sh [--python PYTHON] PSD SOURCE_2HD_TEMPLATE.d88 OUTPUT.d88
# The atlas is made with ImageMagick convert when present, otherwise with
# psd-tools (PYTHON must import psd_tools and PIL). The tracked IDA profile
# tables under 256/ida/ are regenerated from the atlas and must match.

set -eu

python_command=python3
if [ "${1:-}" = "--python" ]; then
    python_command=$2
    shift 2
fi
if [ "$#" -ne 3 ]; then
    printf 'usage: %s [--python PYTHON] PSD SOURCE_2HD_TEMPLATE.d88 OUTPUT.d88\n' "$0" >&2
    exit 2
fi
psd=$1
source_image=$2
output_image=$3
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
profile_dir=$script_dir/256/ida

work_dir=$(mktemp -d "${TMPDIR:-/tmp}/zundamon-orbit-psd.XXXXXX")
trap 'rm -rf "$work_dir"' EXIT HUP INT TERM

"$python_command" "$script_dir/tools/build_zundamon_orbit_psd_atlas.py" \
    "$psd" "$work_dir/ZUNDAMON.BIN"
python3 "$script_dir/tools/generate_zundamon_orbit_depth_table.py" \
    --atlas "$work_dir/ZUNDAMON.BIN" --radius 96 16 \
    --output "$work_dir/zundamon_depth_table.inc" >/dev/null
python3 "$script_dir/tools/generate_zundamon_orbit_hud.py" \
    --output "$work_dir/zundamon_hud_table.inc" \
    --status-output "$work_dir/zundamon_status_table.inc" \
    --subject IDA --count-max 64 --speed-profile ida64 >/dev/null
for table in zundamon_depth_table.inc zundamon_hud_table.inc zundamon_status_table.inc; do
    cmp "$work_dir/$table" "$profile_dir/$table" || {
        printf 'error: %s does not match the tracked profile\n' "$table" >&2
        exit 1
    }
done
M98Y_PRIVATE_PROFILE_DIR=$profile_dir \
    "$script_dir/build-d88.sh" "$source_image" "$work_dir/ZUNDAMON.BIN" "$output_image"
