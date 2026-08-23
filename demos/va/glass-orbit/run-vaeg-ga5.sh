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
    printf 'usage: %s SOURCE_BOOTABLE_2HD.d88 VAEG ROM_DIRECTORY OUTPUT_DIRECTORY\n' "$0" >&2
    exit 2
fi

source_image=$1
vaeg=$2
rom_directory=$3
output_directory=$4
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
disk_image=$output_directory/glass-orbit-ga5-bootable.d88
cpu_debug_output=$output_directory/cpu-reference
debug_output=$output_directory/sgp-debug

[ -f "$vaeg" ] || {
    printf 'error: VAEG executable does not exist: %s\n' "$vaeg" >&2
    exit 1
}
[ -d "$rom_directory" ] || {
    printf 'error: ROM directory does not exist: %s\n' "$rom_directory" >&2
    exit 1
}
[ ! -e "$output_directory" ] || {
    printf 'error: refusing to overwrite output directory: %s\n' "$output_directory" >&2
    exit 1
}

mkdir -p "$output_directory" "$cpu_debug_output" "$debug_output"
"$script_dir/build-ga5-bootable-d88.sh" "$source_image" "$disk_image"

SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
    "$vaeg" \
        --model va \
        --roms "$rom_directory" \
        --fdd1 "$disk_image" \
        --no-cfg \
        --no-bkupmem \
        --nowait \
        --mute \
        --debug-script "$script_dir/glass_orbit_ga5_cpu.debug" \
        --debug-output-dir "$cpu_debug_output"

SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
    "$vaeg" \
        --model va \
        --roms "$rom_directory" \
        --fdd1 "$disk_image" \
        --no-cfg \
        --no-bkupmem \
        --nowait \
        --mute \
        --debug-script "$script_dir/glass_orbit_ga5.debug" \
        --debug-output-dir "$debug_output"

python3 "$script_dir/tools/verify-ga5-capture.py" \
    "$cpu_debug_output" "$debug_output"
printf 'GLASS ORBIT GA-5 VAEG capture directory: %s\n' "$output_directory"
