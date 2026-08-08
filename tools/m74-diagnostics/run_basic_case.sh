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
# WARRANTIES ARE DISCLAIMED.
set -eu
worker=${1:?worker executable}
rom_root=${2:?ROM root}
disk=${3:?boot disk}
command=${4:?BASIC command}
bound=${5:?guest frame bound}
script_path=${6:?headless script path}
output_path=${7:?output path}
mkdir -p "$(dirname "$script_path")" "$(dirname "$output_path")"
printf '%s\n' BASIC @prompt "$command" @prompt @exit >"$script_path"
sha256sum "$worker" "$disk" >"$output_path.identity"
printf 'worker=%s\n' "$worker" >>"$output_path.identity"
printf 'rom_root=%s\ndisk=%s\ncommand=%s\nbound=%s\n' "$rom_root" "$disk" "$command" "$bound" >>"$output_path.identity"
VAEG_M74_CPU_TRACE_LIMIT=${VAEG_M74_CPU_TRACE_LIMIT:-1} VAEG_M74_CPU_TRACE_COMMAND=3 VAEG_M74_REACHABILITY=1 VAEG_HEADLESS_MAX_FRAMES="$bound" VAEG_HEADLESS_PROMPT_TIMEOUT_FRAMES=300 SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy "$worker" --model va --roms "$rom_root" --fdd1 "$disk" --headless-input-script "$script_path" >"$output_path" 2>&1
