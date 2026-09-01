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

if [ "$#" -ne 3 ]; then
    printf 'usage: %s SOURCE_BOOTABLE_2HD.d88 ZUNDORB.BIN OUTPUT.d88\n' "$0" >&2
    exit 2
fi

source_image=$1
atlas_image=$2
output_image=$3
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_root=$(CDPATH= cd -- "$script_dir/../.." && pwd)
output_parent=$(dirname -- "$output_image")

[ -f "$source_image" ] || {
    printf 'error: source D88 does not exist: %s\n' "$source_image" >&2
    exit 1
}
[ -f "$atlas_image" ] || {
    printf 'error: atlas does not exist: %s\n' "$atlas_image" >&2
    exit 1
}
[ ! -e "$output_image" ] || {
    printf 'error: refusing to overwrite existing output: %s\n' "$output_image" >&2
    exit 1
}
[ -d "$output_parent" ] || {
    printf 'error: output directory does not exist: %s\n' "$output_parent" >&2
    exit 1
}
command -v python3 >/dev/null 2>&1 || {
    printf 'error: required host command is missing: python3\n' >&2
    exit 127
}

work_dir=$(mktemp -d "${TMPDIR:-/tmp}/zundamon-orbit-m98v.XXXXXX")
cleanup() {
    rm -rf "$work_dir"
}
trap cleanup EXIT HUP INT TERM

mkdir -p "$work_dir/payload/root"
NASM=${NASM:-nasm} \
    "$script_dir/256/build.sh" "$work_dir/payload/root/ZUNDORB.COM" \
    "$work_dir/ZUNDORB.LST"
cp "$atlas_image" "$work_dir/payload/root/ZUNDORB.BIN"

python3 "$script_dir/tools/build_zundamon_orbit_boot_disk.py" \
    --source "$source_image" \
    --payload "$work_dir/payload" \
    --output "$output_image"
python3 "$repo_root/tools/pc88va/pcengine_disk.py" list \
    --image "$output_image"

printf 'Created local bootable M98v disk: %s\n' "$output_image"
printf '  ZUNDORB.COM runs the full-page multi-ZUNDAMON ellipse with G0 HUD.\n'
printf '  LEFT/RIGHT select cadence, SPACE pauses, and ESC restores and exits.\n'
printf '  The source template is unchanged; this output remains local-only.\n'
