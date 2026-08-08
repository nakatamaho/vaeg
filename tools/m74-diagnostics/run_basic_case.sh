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
model=${VAEG_M74_MODEL:-va}
trace_limit=${VAEG_M74_CPU_TRACE_LIMIT:-1}
trace_command=${VAEG_M74_CPU_TRACE_COMMAND:-3}
reachability=${VAEG_M74_REACHABILITY:-1}
free_boundary=${VAEG_M74_FREE_BOUNDARY:-0}
allocation_capture=${VAEG_M74_ALLOCATION_CAPTURE:-0}
installer_capture=${VAEG_M74_INSTALLER_CAPTURE:-0}
vector_watch=${VAEG_M74_VECTOR_WATCH:-0}
reset_arm=${VAEG_M74_RESET_ARM:-0}
prompt_timeout=${VAEG_HEADLESS_PROMPT_TIMEOUT_FRAMES:-300}
runner_path=$0
repo_root=$(CDPATH= cd -- "$(dirname "$runner_path")/../.." && pwd)
mkdir -p "$(dirname "$script_path")" "$(dirname "$output_path")"
printf '%s\n' BASIC @prompt "$command" @prompt @exit >"$script_path"
repo_sha=$(GIT_CONFIG_GLOBAL=/dev/null GIT_CONFIG_SYSTEM=/dev/null git -C "$repo_root" rev-parse HEAD)
{
    printf 'repository_source_sha=%s\n' "$repo_sha"
    sha256sum "$worker"
    sha256sum "$runner_path"
    sha256sum "$disk"
    if [ -f "$rom_root/varom00.rom" ]; then
        sha256sum "$rom_root/varom00.rom"
    fi
    printf 'worker=%s\n' "$worker"
    printf 'runner=%s\n' "$runner_path"
    printf 'rom_root=%s\n' "$rom_root"
    printf 'disk=%s\n' "$disk"
    printf 'model=%s\n' "$model"
    printf 'command=%s\n' "$command"
    printf 'guest_frame_bound=%s\n' "$bound"
    printf 'trace_limit=%s\n' "$trace_limit"
    printf 'trace_command=%s\n' "$trace_command"
    printf 'reachability=%s\n' "$reachability"
    printf 'free_boundary=%s\n' "$free_boundary"
    printf 'allocation_capture=%s\n' "$allocation_capture"
    printf 'installer_capture=%s\n' "$installer_capture"
    printf 'vector_watch=%s\n' "$vector_watch"
    printf 'reset_arm=%s\n' "$reset_arm"
    printf 'prompt_timeout_frames=%s\n' "$prompt_timeout"
    printf 'working_directory=%s\n' "$(pwd)"
} >"$output_path.identity"
set +e
VAEG_M74_CPU_TRACE_LIMIT="$trace_limit" \
VAEG_M74_CPU_TRACE_COMMAND="$trace_command" \
VAEG_M74_REACHABILITY="$reachability" \
VAEG_M74_FREE_BOUNDARY="$free_boundary" \
VAEG_M74_ALLOCATION_CAPTURE="$allocation_capture" \
VAEG_M74_INSTALLER_CAPTURE="$installer_capture" \
VAEG_M74_VECTOR_WATCH="$vector_watch" \
VAEG_M74_RESET_ARM="$reset_arm" \
VAEG_HEADLESS_MAX_FRAMES="$bound" \
VAEG_HEADLESS_PROMPT_TIMEOUT_FRAMES="$prompt_timeout" \
SDL_VIDEODRIVER=dummy \
SDL_AUDIODRIVER=dummy \
"$worker" --model "$model" --roms "$rom_root" --fdd1 "$disk" \
    --nowait --headless-input-script "$script_path" >"$output_path" 2>&1
status=$?
set -e
printf 'emulator_exit_status=%s\n' "$status" >>"$output_path.identity"
exit "$status"
