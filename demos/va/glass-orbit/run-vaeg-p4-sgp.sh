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
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
# EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

set -eu

if [ "$#" -ne 4 ]; then
    printf 'usage: VAEG_P4_MODEL=va|va2 %s SOURCE_BOOTABLE_2HD.d88 VAEG ROM_DIRECTORY OUTPUT_DIRECTORY\n' "$0" >&2
    exit 2
fi

source_image=$1
vaeg=$2
rom_directory=$3
output_directory=$4
model=${VAEG_P4_MODEL:-va}
# The functional capture defaults to the documented CLI's accelerated SGP
# model so that the bounded harness can finish the full fixed-frame list.
# It is not a real-hardware timing mode or a performance measurement.
sgp_speed=${VAEG_P4_SGP_SPEED:-16}
cpu_multiple=${VAEG_P4_CPU_MULT:-2}
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
disk_image=$output_directory/glass-orbit-p4-sgp-bootable.d88

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
[ "$model" = va ] || [ "$model" = va2 ] || {
    printf 'error: VAEG_P4_MODEL must be va or va2: %s\n' "$model" >&2
    exit 2
}
[ "$sgp_speed" = model ] || [ "$sgp_speed" = follow-cpu ] || {
    case "$sgp_speed" in
        1|2|3|4|5|6|7|8|9|10|11|12|13|14|15|16) ;;
        *)
            printf 'error: VAEG_P4_SGP_SPEED must be model, follow-cpu, or 1 through 16: %s\n' "$sgp_speed" >&2
            exit 2
            ;;
    esac
}
case "$cpu_multiple" in
    1|2|3|4|5|6|7|8|9|10|11|12|13|14|15|16|17|18|19|20|21|22|23|24|25|26|27|28|29|30|31|32) ;;
    *)
        printf 'error: VAEG_P4_CPU_MULT must be 1 through 32: %s\n' "$cpu_multiple" >&2
        exit 2
        ;;
esac

mkdir -p "$output_directory"
"$script_dir/build-p4-sgp-bootable-d88.sh" "$source_image" "$disk_image"
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
    "$vaeg" \
        --model "$model" \
        --roms "$rom_directory" \
        --fdd1 "$disk_image" \
        --no-cfg \
        --no-bkupmem \
        --nowait \
        --mute \
        --cpumult "$cpu_multiple" \
        --sgp "$sgp_speed" \
        --debug-script "$script_dir/glass_orbit_p4_sgp.debug" \
        --debug-output-dir "$output_directory"
printf 'GLASS ORBIT P4-2 SGP VAEG capture directory: %s\n' "$output_directory"
