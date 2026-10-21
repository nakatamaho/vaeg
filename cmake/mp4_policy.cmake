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
# POSSIBILITY OF SUCH DAMAGE.

option(VAEG_ENABLE_LOCAL_MP4_ENCODERS "LOCAL ONLY: statically link libx264/AAC; NEVER distribute" OFF)
option(VAEG_PUBLIC_BUILD "CI/release/clean handoff: forbid local-only encoder implementations" OFF)
set(VAEG_LOCAL_MP4_ACKNOWLEDGEMENT "" CACHE STRING "Required local-only license acknowledgement")
# This is independently testable without Git, an SDK, or a future evidence commit.
function(vaeg_mp4_validate_policy)
    if(NOT VAEG_ENABLE_LOCAL_MP4_ENCODERS)
        return()
    endif()
    foreach(name CI GITHUB_ACTIONS GITLAB_CI TF_BUILD BUILD_NUMBER JENKINS_URL
            TEAMCITY_VERSION BUILDKITE CIRCLECI TRAVIS APPVEYOR)
        if(DEFINED ENV{${name}} AND NOT "$ENV{${name}}" STREQUAL "")
            message(FATAL_ERROR "MP4_LOCAL_CI_FORBIDDEN: ${name}")
        endif()
    endforeach()
    if(VAEG_PUBLIC_BUILD)
        message(FATAL_ERROR "MP4_LOCAL_PUBLIC_FORBIDDEN")
    endif()
    # A -D override must not turn a public preset into a private build.
    if(DEFINED ENV{VAEG_PUBLIC_PRESET} AND NOT "$ENV{VAEG_PUBLIC_PRESET}" STREQUAL "")
        message(FATAL_ERROR "MP4_LOCAL_PRESET_FORBIDDEN")
    endif()
    if(NOT VAEG_LOCAL_MP4_ACKNOWLEDGEMENT STREQUAL "LOCAL_ONLY_NO_DISTRIBUTION")
        message(FATAL_ERROR "MP4_LOCAL_ACK_REQUIRED")
    endif()
    if(NOT VAEG_ENABLE_RECORDING OR NOT VAEG_STATIC_FFMPEG)
        message(FATAL_ERROR "MP4_LOCAL_STATIC_RECORDING_REQUIRED")
    endif()
    if(APPLE OR WIN32)
        message(FATAL_ERROR "MP4_LOCAL_LINUX_ONLY: use the native OS backend")
    endif()
endfunction()
vaeg_mp4_validate_policy()

function(vaeg_mp4_audit_archive archive)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    set(VAEG_MP4_AUDIT_PYTHON "${Python3_EXECUTABLE}" PARENT_SCOPE)
    set(args)
    if(VAEG_ENABLE_LOCAL_MP4_ENCODERS)
        list(APPEND args --local-only)
    endif()
    execute_process(COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../tools/repo/check_mp4_codec_archive.py"
        --nm "${CMAKE_NM}" --archive "${archive}" ${args}
        RESULT_VARIABLE result OUTPUT_VARIABLE out ERROR_VARIABLE err)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "${out}${err}")
    endif()
    message(STATUS "${out}")
endfunction()
