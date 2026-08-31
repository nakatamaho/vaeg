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

if [ "$#" -ne 4 ]; then
    printf 'usage: VAEG_ZUNDAMON_MODEL=va2 %s SOURCE_BOOTABLE_2HD.d88 VAEG ROM_DIRECTORY OUTPUT_DIRECTORY\n' "$0" >&2
    exit 2
fi

source_image=$1
vaeg=$2
rom_directory=$3
output_directory=$4
model=${VAEG_ZUNDAMON_MODEL:-va2}
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
pristine_disk_image=$output_directory/zundamon-orbit-m98o-pristine.d88
disk_image=$output_directory/zundamon-orbit-m98o.d88
guest_image=$output_directory/ZUNDORB.COM
guest_listing=$output_directory/ZUNDORB.LST
atlas_directory=$output_directory/public-atlas
atlas_image=$output_directory/ZUNDORB.BIN
trace_log=$output_directory/sgp-trace.log

[ -f "$source_image" ] || { printf 'error: source D88 does not exist\n' >&2; exit 1; }
[ -f "$vaeg" ] && [ -x "$vaeg" ] || { printf 'error: VAEG executable is unavailable\n' >&2; exit 1; }
[ -d "$rom_directory" ] || { printf 'error: ROM directory does not exist\n' >&2; exit 1; }
[ ! -e "$output_directory" ] || { printf 'error: refusing to overwrite output directory\n' >&2; exit 1; }
case "$model" in
    va|va2) ;;
    *) printf 'error: VAEG_ZUNDAMON_MODEL must be va or va2\n' >&2; exit 2 ;;
esac

mkdir -p "$output_directory"
python3 "$script_dir/tools/build_zundamon_orbit_pipeline.py" \
    --fixture-output "$atlas_directory"
cp "$atlas_directory/zundorb.bin" "$atlas_image"
NASM=${NASM:-nasm} "$script_dir/256/build.sh" "$guest_image" "$guest_listing"
NASM=${NASM:-nasm} "$script_dir/build-local-d88.sh" \
    "$source_image" "$atlas_image" "$pristine_disk_image"
cp "$pristine_disk_image" "$disk_image"
cmp "$pristine_disk_image" "$disk_image"

SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy VAEG_SGP_SCAN_TRACE=1 \
    "$vaeg" \
        --model "$model" \
        --roms "$rom_directory" \
        --fdd1 "$disk_image" \
        --no-cfg \
        --no-bkupmem \
        --nowait \
        --mute \
        --debug-script "$script_dir/zundamon_orbit_m98o.debug" \
        --debug-output-dir "$output_directory" \
        >"$output_directory/vaeg.stdout.log" 2>"$trace_log"

python3 "$script_dir/tools/verify_zundamon_orbit_guest.py" \
    --atlas "$atlas_image" \
    --trace "$trace_log" \
    --report "$output_directory/m98o-oracle.json" \
    "$output_directory"
printf 'M98O_VAEG_CAPTURE_PASS output=%s\n' "$output_directory"
