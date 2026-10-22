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

# Keep pkg-config's ordered private dependency closure, but replace every
# FFmpeg -l flag with an exact archive. Merely using --static is not sufficient:
# a normal -lavcodec can still select a shared library beside the archive.
function(vaeg_ffmpeg_static_target target prefix)
    set(CMAKE_FIND_LIBRARY_SUFFIXES "${CMAKE_STATIC_LIBRARY_SUFFIX}")
    set(libraries)
    set(found_ffmpeg OFF)
    set(framework_flag "")
    foreach(flag IN LISTS ${prefix}_STATIC_LDFLAGS)
        if(framework_flag)
            list(APPEND libraries "${framework_flag} ${flag}")
            set(framework_flag "")
            continue()
        elseif(flag STREQUAL "-framework" OR flag STREQUAL "-weak_framework")
            set(framework_flag "${flag}")
            continue()
        endif()
        if(flag STREQUAL "-lx264" AND NOT VAEG_ENABLE_LOCAL_MP4_ENCODERS)
            message(FATAL_ERROR "MP4_LOCAL_LINK_FORBIDDEN: x264 in ordinary static dependencies")
        endif()
        if(flag MATCHES "^-l(avutil|avcodec|avformat|swresample|x264)$")
            set(found_ffmpeg ON)
            set(name "${CMAKE_MATCH_1}")
            unset(archive)
            unset(archive CACHE)
            find_library(archive NAMES "${name}"
                HINTS ${${prefix}_STATIC_LIBRARY_DIRS} NO_DEFAULT_PATH)
            if(NOT archive)
                message(FATAL_ERROR
                    "VAEG_FFMPEG_STATIC_ARCHIVE_MISSING: ${name}; "
                    "install target-compatible static FFmpeg in the pkg-config prefix")
            endif()
            if(name STREQUAL "avcodec" AND VAEG_ENABLE_RECORDING)
                vaeg_mp4_audit_archive("${archive}")
                if(NOT TARGET vaeg_mp4_static_audit)
                    set(mp4_audit_args)
                    if(VAEG_ENABLE_LOCAL_MP4_ENCODERS)
                        list(APPEND mp4_audit_args --local-only)
                    endif()
                    add_custom_target(vaeg_mp4_static_audit
                        COMMAND "${VAEG_MP4_AUDIT_PYTHON}"
                            "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../tools/repo/check_mp4_codec_archive.py"
                            --nm "${CMAKE_NM}" --archive "${archive}" ${mp4_audit_args}
                        VERBATIM)
                    vaeg_mp4_attach_build_guard(vaeg_mp4_static_audit)
                endif()
            endif()
            list(APPEND libraries "${archive}")
        else()
            list(APPEND libraries "${flag}")
        endif()
    endforeach()
    if(NOT found_ffmpeg OR framework_flag)
        message(FATAL_ERROR "VAEG_FFMPEG_STATIC_METADATA_MISSING: ${prefix}")
    endif()
    add_library(${target} INTERFACE)
    if(COMMAND vaeg_mp4_attach_build_guard)
        vaeg_mp4_attach_build_guard(${target})
    endif()
    target_include_directories(${target} SYSTEM INTERFACE ${${prefix}_STATIC_INCLUDE_DIRS})
    target_compile_options(${target} INTERFACE ${${prefix}_STATIC_CFLAGS_OTHER})
    target_link_libraries(${target} INTERFACE ${libraries})
    if(TARGET vaeg_mp4_static_audit)
        add_dependencies(${target} vaeg_mp4_static_audit)
    endif()
endfunction()
