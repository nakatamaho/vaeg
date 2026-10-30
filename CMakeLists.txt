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
cmake_minimum_required(VERSION 3.20)

project(vaeg LANGUAGES C CXX)

find_package(Threads REQUIRED)

option(VAEG_ENABLE_TESTS "Enable vaeg tests" OFF)
option(VAEG_ENABLE_RECORDING "Enable the optional recording facade" OFF)
option(VAEG_WERROR "Treat warnings as errors" OFF)
option(VAEG_WINDOWS_CONSOLE "Keep a console window for Windows SDL2 builds" OFF)
set(VAEG_DESKTOP_STATIC_DEFAULT OFF)
if(WIN32 OR APPLE)
    set(VAEG_DESKTOP_STATIC_DEFAULT ON)
endif()
option(VAEG_FETCH_SDL2 "Fetch pinned SDL2 instead of using system SDL2" ${VAEG_DESKTOP_STATIC_DEFAULT})
option(VAEG_STATIC_SDL2 "Link SDL2 statically" ${VAEG_DESKTOP_STATIC_DEFAULT})
option(VAEG_STATIC_FFMPEG "Link FFmpeg archives without shared-library fallback" ${VAEG_DESKTOP_STATIC_DEFAULT})
option(VAEG_ENABLE_LIBRASHADER "Enable optional native CRT presentation" ON)
option(VAEG_STATIC_LIBRASHADER "Link the pinned librashader C API statically" ${VAEG_DESKTOP_STATIC_DEFAULT})
set(VAEG_LIBRASHADER_STATIC_LIBRARY "" CACHE FILEPATH "Target-compatible librashader static archive")
set(VAEG_LIBRASHADER_NATIVE_LIBS "" CACHE STRING "Native link libraries reported by rustc")
set(VAEG_LIBRASHADER_STATIC_NOTICES "" CACHE FILEPATH "Generated static dependency notices")
option(VAEG_ENABLE_ARCHIVE_DROP
    "Enable archive disk-image drag and drop when LibArchive is available" ON)
option(VAEG_FETCH_LIBARCHIVE
    "Fetch pinned static archive dependencies instead of using system LibArchive" ${VAEG_DESKTOP_STATIC_DEFAULT})
option(VAEG_Z80_COMPAT_INTEGRATION_TRACE
    "Compile deterministic subsystem compatibility/FDD integration tracing" OFF)
option(VAEG_UPD9002_PERF_DIAGNOSTIC
    "Compile disabled-by-default uPD9002 runtime performance diagnostics" OFF)
set(VAEG_ZEX_ARTIFACT_DIR "" CACHE PATH
    "Offline directory containing the five hash-verified ZEX artifacts")
include(cmake/mp4_policy.cmake)

if(MINGW)
    if(NOT VAEG_FETCH_SDL2 OR NOT VAEG_STATIC_SDL2)
        message(FATAL_ERROR "MinGW builds require fetched static SDL2")
    endif()
    if(VAEG_ENABLE_ARCHIVE_DROP AND NOT VAEG_FETCH_LIBARCHIVE)
        message(FATAL_ERROR "MinGW archive drop requires the fetched static LibArchive stack")
    endif()
    if(VAEG_ENABLE_LIBRASHADER AND NOT VAEG_STATIC_LIBRASHADER)
        message(FATAL_ERROR "MinGW builds require statically linked librashader")
    endif()
endif()

if(APPLE AND (VAEG_ENABLE_LIBRASHADER OR VAEG_ENABLE_RECORDING))
    enable_language(OBJC)
    enable_language(OBJCXX)
endif()

set(VAEG_BUILD_DATE "unknown")
set(VAEG_BUILD_COMMIT "unknown")
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(VAEG_GIT_CONFIG "${CMAKE_CURRENT_BINARY_DIR}/generated/git-empty-config")
file(WRITE "${VAEG_GIT_CONFIG}" "")
find_package(Git QUIET)
if(Git_FOUND)
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env
            "GIT_CONFIG_GLOBAL=${VAEG_GIT_CONFIG}"
            "GIT_CONFIG_SYSTEM=${VAEG_GIT_CONFIG}"
            "${GIT_EXECUTABLE}" show -s --format=%H HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE VAEG_BUILD_COMMIT_RESULT
        OUTPUT_VARIABLE VAEG_BUILD_COMMIT_OUTPUT
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env
            "GIT_CONFIG_GLOBAL=${VAEG_GIT_CONFIG}"
            "GIT_CONFIG_SYSTEM=${VAEG_GIT_CONFIG}"
            "TZ=Asia/Tokyo"
            "${GIT_EXECUTABLE}" show -s
            --date=format-local:%Y%m%d --format=%cd HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE VAEG_BUILD_DATE_RESULT
        OUTPUT_VARIABLE VAEG_BUILD_DATE_OUTPUT
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(VAEG_BUILD_COMMIT_RESULT EQUAL 0
            AND VAEG_BUILD_COMMIT_OUTPUT MATCHES "^[0-9a-f]+$")
        set(VAEG_BUILD_COMMIT "${VAEG_BUILD_COMMIT_OUTPUT}")
    endif()
    if(VAEG_BUILD_DATE_RESULT EQUAL 0
            AND VAEG_BUILD_DATE_OUTPUT MATCHES "^[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9]$")
        set(VAEG_BUILD_DATE "${VAEG_BUILD_DATE_OUTPUT}")
    endif()
    # M120: a release tag on the commit names the release, so that a
    # follow-up release (rel-20261017.1) does not depend on the commit date.
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env
            "GIT_CONFIG_GLOBAL=${VAEG_GIT_CONFIG}"
            "GIT_CONFIG_SYSTEM=${VAEG_GIT_CONFIG}"
            "${GIT_EXECUTABLE}" tag --points-at HEAD --list "rel-*"
            --sort=-version:refname
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        RESULT_VARIABLE VAEG_BUILD_TAG_RESULT
        OUTPUT_VARIABLE VAEG_BUILD_TAG_OUTPUT
        OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()
set(VAEG_BUILD_RELEASE "${VAEG_BUILD_DATE}")
if(VAEG_BUILD_TAG_RESULT EQUAL 0 AND VAEG_BUILD_TAG_OUTPUT)
    string(REPLACE "\n" ";" VAEG_BUILD_TAGS "${VAEG_BUILD_TAG_OUTPUT}")
    foreach(VAEG_BUILD_TAG IN LISTS VAEG_BUILD_TAGS)
        if(VAEG_BUILD_TAG MATCHES
                "^rel-([0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9](\\.[1-9][0-9]*)?)$")
            set(VAEG_BUILD_RELEASE "${CMAKE_MATCH_1}")
            break()
        endif()
    endforeach()
endif()
include(cmake/release_identity.cmake)
vaeg_release_identity("${VAEG_BUILD_RELEASE}" "${VAEG_SOURCE_MILESTONE}" "${VAEG_BUILD_COMMIT}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/generated/release-identity.json"
    "{\n  \"release\": \"${VAEG_BUILD_RELEASE}\",\n"
    "  \"milestone\": \"${VAEG_BUILD_MILESTONE}\",\n"
    "  \"commit\": \"${VAEG_BUILD_COMMIT}\",\n"
    "  \"short_commit\": \"${VAEG_BUILD_SHORT_COMMIT}\",\n"
    "  \"display\": \"${VAEG_BUILD_DISPLAY}\",\n"
    "  \"package_stem\": \"${VAEG_PACKAGE_STEM}\"\n}\n")
message(STATUS "vaeg release name: ${VAEG_BUILD_DISPLAY}")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/vaeg_build_info.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/vaeg_build_info.h"
    @ONLY)

if(VAEG_ENABLE_TESTS AND NOT VAEG_Z80_COMPAT_INTEGRATION_TRACE)
    message(FATAL_ERROR
        "VAEG_ENABLE_TESTS requires VAEG_Z80_COMPAT_INTEGRATION_TRACE=ON; "
        "the trace-equivalence test cannot use an uninstrumented executable")
endif()

if(VAEG_FETCH_SDL2)
    include(FetchContent)
    if(VAEG_STATIC_SDL2)
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
        set(SDL_SHARED OFF CACHE BOOL "" FORCE)
        set(SDL_STATIC ON CACHE BOOL "" FORCE)
    else()
        set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
        set(SDL_SHARED ON CACHE BOOL "" FORCE)
        set(SDL_STATIC OFF CACHE BOOL "" FORCE)
    endif()
    set(SDL_TEST OFF CACHE BOOL "" FORCE)
    set(SDL_TESTS OFF CACHE BOOL "" FORCE)
    set(SDL2_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
    set(SDL_INSTALL_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(SDL2
        URL "https://github.com/libsdl-org/SDL/releases/download/release-2.32.10/SDL2-2.32.10.tar.gz"
        URL_HASH SHA256=5f5993c530f084535c65a6879e9b26ad441169b3e25d789d83287040a9ca5165
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(SDL2)
    if(VAEG_STATIC_SDL2)
        if(NOT TARGET SDL2::SDL2-static)
            message(FATAL_ERROR "Fetched SDL2 did not provide SDL2::SDL2-static")
        endif()
        set(VAEG_SDL2_LIBS SDL2::SDL2-static)
    else()
        set(VAEG_SDL2_LIBS SDL2::SDL2)
    endif()
else()
    find_package(SDL2 QUIET CONFIG)
    if(VAEG_STATIC_SDL2 AND TARGET SDL2::SDL2-static)
        set(VAEG_SDL2_LIBS SDL2::SDL2-static)
    elseif(VAEG_STATIC_SDL2)
        message(FATAL_ERROR
            "VAEG_STATIC_SDL2 requires SDL2::SDL2-static; use VAEG_FETCH_SDL2=ON")
    elseif(TARGET SDL2::SDL2)
        set(VAEG_SDL2_LIBS SDL2::SDL2)
    else()
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
        set(VAEG_SDL2_LIBS PkgConfig::SDL2)
    endif()
endif()

set(VAEG_FFMPEG_TARGETS)
if(VAEG_ENABLE_RECORDING)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(FFMPEG_AVUTIL REQUIRED IMPORTED_TARGET libavutil)
    pkg_check_modules(FFMPEG_AVCODEC REQUIRED IMPORTED_TARGET libavcodec)
    pkg_check_modules(FFMPEG_AVFORMAT REQUIRED IMPORTED_TARGET libavformat)
    pkg_check_modules(FFMPEG_SWRESAMPLE REQUIRED IMPORTED_TARGET libswresample)
    if(VAEG_STATIC_FFMPEG)
        include(cmake/ffmpeg_static.cmake)
        foreach(component AVUTIL AVCODEC AVFORMAT SWRESAMPLE)
            vaeg_ffmpeg_static_target(vaeg_ffmpeg_${component} FFMPEG_${component})
            list(APPEND VAEG_FFMPEG_TARGETS vaeg_ffmpeg_${component})
        endforeach()
    else()
        list(APPEND VAEG_FFMPEG_TARGETS
            PkgConfig::FFMPEG_AVUTIL
            PkgConfig::FFMPEG_AVCODEC
            PkgConfig::FFMPEG_AVFORMAT
            PkgConfig::FFMPEG_SWRESAMPLE)
    endif()
endif()

# Post-stop export is independent of emulator-core C interfaces and real-time capture.
add_library(vaeg_recording_mp4 STATIC sdl2/recording/recording_mp4.cpp)
vaeg_mp4_attach_build_guard(vaeg_recording_mp4)
target_compile_features(vaeg_recording_mp4 PRIVATE cxx_std_17)
set_target_properties(vaeg_recording_mp4 PROPERTIES CXX_EXTENSIONS OFF CXX_STANDARD_REQUIRED ON)
target_compile_definitions(vaeg_recording_mp4 PRIVATE
    VAEG_ENABLE_RECORDING=$<BOOL:${VAEG_ENABLE_RECORDING}>
    VAEG_ENABLE_LOCAL_MP4_ENCODERS=$<BOOL:${VAEG_ENABLE_LOCAL_MP4_ENCODERS}>)
target_compile_options(vaeg_recording_mp4 PRIVATE -Wall -Wextra -Wpedantic)
if(VAEG_WERROR)
    target_compile_options(vaeg_recording_mp4 PRIVATE -Werror)
endif()
target_link_libraries(vaeg_recording_mp4 PUBLIC Threads::Threads)
if(VAEG_ENABLE_RECORDING)
    target_link_libraries(vaeg_recording_mp4 PUBLIC ${VAEG_FFMPEG_TARGETS})
    if(APPLE)
        target_sources(vaeg_recording_mp4 PRIVATE sdl2/recording/recording_mp4_macos.mm)
        set_source_files_properties(sdl2/recording/recording_mp4_macos.mm
            PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
        target_link_libraries(vaeg_recording_mp4 PUBLIC
            "-framework Foundation" objc "-framework AVFoundation" "-framework AudioToolbox"
            "-framework CoreMedia" "-framework CoreVideo")
    elseif(WIN32)
        target_sources(vaeg_recording_mp4 PRIVATE sdl2/recording/recording_mp4_windows.cpp)
        target_link_libraries(vaeg_recording_mp4 PUBLIC mfuuid ole32)
    endif()
    if(VAEG_ENABLE_LOCAL_MP4_ENCODERS)
        target_sources(vaeg_recording_mp4 PRIVATE sdl2/recording/recording_mp4_local.cpp)
    endif()
endif()

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32 AND VAEG_ENABLE_LIBRASHADER)
    list(APPEND VAEG_SDL2_SOURCES
        sdl2/librashader/backends/d3d11_bridge.cpp
        sdl2/librashader/d3d11_presenter.cpp)
endif()

set(VAEG_INCLUDE_DIRS
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
    "${CMAKE_CURRENT_SOURCE_DIR}/sdl2"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/machine"
    "${CMAKE_CURRENT_SOURCE_DIR}/common"
    "${CMAKE_CURRENT_SOURCE_DIR}/cpu/upd9002"
    "${CMAKE_CURRENT_SOURCE_DIR}/cpu"
    "${CMAKE_CURRENT_SOURCE_DIR}/io"
    "${CMAKE_CURRENT_SOURCE_DIR}/cbus"
    "${CMAKE_CURRENT_SOURCE_DIR}/bios"
    "${CMAKE_CURRENT_SOURCE_DIR}/sound"
    "${CMAKE_CURRENT_SOURCE_DIR}/sound/vermouth"
    "${CMAKE_CURRENT_SOURCE_DIR}/sound/getsnd"
    "${CMAKE_CURRENT_SOURCE_DIR}/vram"
    "${CMAKE_CURRENT_SOURCE_DIR}/fdd"
    "${CMAKE_CURRENT_SOURCE_DIR}/font"
    "${CMAKE_CURRENT_SOURCE_DIR}/memoryva"
    "${CMAKE_CURRENT_SOURCE_DIR}/io"
    "${CMAKE_CURRENT_SOURCE_DIR}/generic")

set(VAEG_COMMON_SOURCES
    common/strres.c
    common/milstr.c
    common/_memory.c
    common/textfile.c
    common/profile.c
    common/rect.c
    common/lstarray.c
    common/bmpdata.c
    common/codecnv.c
    common/parts.c
    common/resize.c
    generic/cmjasts.c
    generic/cmver.c
    generic/np2info.c)

set(VAEG_CORE_SOURCES
    cpu/upd8087/upd8087.c
    cpu/upd9002/upd9002_core.c
	cpu/upd9002/upd9002_diagnostic.c
	diagnostics/upd9002_debug.c
    cpu/upd9002/upd9002_state.c
    cpu/upd9002/upd9002_mn.c
    cpu/upd9002/upd9002_ea.c
    cpu/upd9002/upd9002_8x.c
    cpu/upd9002/upd9002_sf.c
    cpu/upd9002/upd9002_f6.c
    cpu/upd9002/upd9002_fe.c
    cpu/upd9002/upd9002_rp.c
    cpu/upd9002/memory.c
    cpu/upd9002/dmap.c
    io/iocore.c
    io/bmsio.c
    io/dmac.c
    io/emsio.c
    io/fdc.c
    io/hostfat.c
    io/mouseif.c
    io/np2sysp.c
    io/pic.c
    io/pit.c
    io/cmt.c
    io/pcg88.c
    io/serial.c
    io/upd4990.c
    cbus/cbuscore.c
    cbus/sasiio.c
    cbus/scsiio.c
    cbus/scsicmd.c
    cbus/mpu98ii.c
    sound/vermouth/midiout.c
    sound/vermouth/midimod.c
    sound/vermouth/midinst.c
    sound/vermouth/midvoice.c
    sound/vermouth/midtable.c
    sound/getsnd/getsnd.c
    sound/getsnd/getwave.c
    sound/getsnd/getsmix.c
    bios/sxsibios.c
    sound/sound.c
    sound/sound_generation.c
    sound/sound_capture.c
    sound/sndcsec.c
    sound/fmboard.c
    sound/fmtimer.c
    sound/beepc.c
    sound/beepg.c
    sound/opngenc.c
    sound/opngeng.c
    sound/psggenc.c
    sound/psggeng.c
    sound/rhythmc.c
    sound/adpcmc.c
    sound/adpcmg.c
    sound/soundrom.c
    fdd/diskdrv.c
    fdd/newdisk.c
    fdd/fddfile.c
    fdd/fdd_xdf.c
    fdd/fdd_d88.c
    fdd/fdd_mtr.c
    fdd/sxsi.c
    font/font.c
    font/fontdata.c
    font/fontpc88.c
    font/fontpc98.c
    font/fontv98.c
    font/fontmake.c
    machine/pccore.c
    machine/nevent.c
    machine/calendar.c
    machine/timing.c
    machine/keystat.c
    machine/statsave.c
    machine/debugsub.c)

set(VAEG_Z80_COMPAT_PRODUCTION_SOURCES
    cpu/upd780/upd780_core_config.cpp
    cpu/upd780/upd780_disasm.cpp
    cpu/z80_compat_cpu.cpp
    cpu/z80_compat_state.cpp
    cpu/upd9002_upd70008_core_config.cpp
    cpu/upd9002_upd70008.cpp)
message(STATUS "Production uPD70008-compatible core: shared Z80 compatibility backend")

set(VAEG_VA_SOURCES
    bios/romva.c
    memoryva/gvramva.c
    memoryva/memoryva.c
    ${VAEG_Z80_COMPAT_PRODUCTION_SOURCES}
    io/bkupmemva.c
    io/boardsb2.c
    io/cgromva.c
    io/fdsubsys.c
    io/gactrlva.c
    io/i8255.c
    io/memctrlva.c
    io/mouseifva.c
    io/sgp.c
    io/subsystem.cpp
    io/subsystemif.c
    io/subsystemmx.c
    io/sysportva.c
    io/tsp.c
    io/upd9002_regs.c
    io/va91.c
    io/videova.c
    vram/makegrphva.c
    vram/makesprva.c
    vram/maketextva.c
    vram/scrndrawva.c
    vram/sdrawva.c)

set(VAEG_SDL2_SOURCES
    external/imgui/imgui.cpp
    external/imgui/imgui_draw.cpp
    external/imgui/imgui_tables.cpp
    external/imgui/imgui_widgets.cpp
    external/imgui/backends/imgui_impl_sdl2.cpp
    external/imgui/backends/imgui_impl_sdlrenderer2.cpp
    external/ymfm/src/ymfm_adpcm.cpp
    external/ymfm/src/ymfm_opn.cpp
    external/ymfm/src/ymfm_ssg.cpp
    sdl2/np2.c
    sdl2/headless_input.c
    sdl2/debug_harness.c
    sdl2/g75_screen.c
    sdl2/cliopts.c
    sdl2/screenshot.c
    sdl2/dosio.c
    sdl2/ini.c
    sdl2/taskmng.c
    sdl2/pacing.c
    sdl2/framedisp.c
    sdl2/viewport.c
    sdl2/frontpanel.c
    sdl2/machine_status.c
    sdl2/appicon.c
    sdl2/splash.c
    sdl2/sysmng.c
    sdl2/timemng.c
    sdl2/trace.c
    sdl2/scrnmng.c
    sdl2/librashader/frame_input.cpp
    sdl2/librashader/frame_conversion.cpp
    sdl2/librashader/frame_padding.cpp
    sdl2/librashader/native_presenter.cpp
    sdl2/librashader/native_presenter_controller.cpp
    sdl2/librashader/presenter_factory.cpp
    sdl2/librashader/shader_parameters.cpp
    sdl2/librashader/shader_preset.cpp
    sdl2/sdlkbd.c
    sdl2/kbdinject.c
    sdl2/kbdmap.c
    sdl2/kbdpaste.c
    sdl2/romankana.c
    sdl2/inputmng.c
    sdl2/dropmedia.cpp
    sdl2/hostfat_snapshot.cpp
    sdl2/hostfat_manager.cpp
    sdl2/extrarom.c
    sdl2/n80rom.c
    sdl2/romcheck.c
    sdl2/selftest.c
    sdl2/recording/recording.c
    sdl2/recording/recording_displayed_capture.c
    sdl2/recording/recording_native_composition.c
    sdl2/recording/recording_osd_snapshot.c
    sdl2/recording/recording_native_tap.c
    sdl2/recording/recording_native_session.cpp
    sdl2/recording/recording_runtime.c
    sdl2/recording/recording_contracts.cpp
    sdl2/recording/recording_clock.cpp
    sdl2/recording/recording_queue.cpp
    sdl2/recording/recording_realtime_clock.cpp
    sdl2/soundmng.c
    sdl2/soundopts.c
    sdl2/ymfmbridge.cpp
    sdl2/mousemng.c
    sdl2/mousestate.c
    sdl2/joymng.c
    sdl2/commng.c
    sdl2/fontmng.c
    sdl2/gui/gui.cpp)
if(VAEG_ENABLE_RECORDING)
    list(APPEND VAEG_SDL2_SOURCES
        sdl2/recording/recording_ffmpeg_capabilities.cpp
        sdl2/recording/recording_h264_encoder.cpp
        sdl2/recording/recording_ffmpeg_video_encoder.cpp
        sdl2/recording/recording_ffmpeg_pcm_encoder.cpp
        sdl2/recording/recording_ffmpeg_matroska.cpp)
endif()
if(APPLE AND VAEG_ENABLE_RECORDING)
    include(CheckSourceCompiles)
    set(CMAKE_REQUIRED_LIBRARIES "-framework Cocoa" "-weak_framework ScreenCaptureKit")
    check_source_compiles(OBJCXX "
        #import <ScreenCaptureKit/ScreenCaptureKit.h>
        int main() {
            if (@available(macOS 14.0, *)) {
                SCContentFilter *filter = nil;
                return filter.pointPixelScale > 0;
            }
            return 0;
        }" VAEG_HAVE_SCREEN_CAPTURE_KIT_GEOMETRY)
    unset(CMAKE_REQUIRED_LIBRARIES)
    if(VAEG_HAVE_SCREEN_CAPTURE_KIT_GEOMETRY)
        list(APPEND VAEG_SDL2_SOURCES sdl2/recording/recording_macos_window_capture.mm)
        set_source_files_properties(sdl2/recording/recording_macos_window_capture.mm
            PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
    else()
        message(STATUS "Recording window capture: SDK lacks ScreenCaptureKit geometry API")
    endif()
endif()
if(APPLE AND VAEG_ENABLE_LIBRASHADER)
    list(APPEND VAEG_SDL2_SOURCES
        sdl2/librashader/backends/metal_bridge.mm
        sdl2/librashader/metal_presenter.cpp)
endif()
if(WIN32 AND VAEG_ENABLE_LIBRASHADER)
    list(APPEND VAEG_SDL2_SOURCES
        sdl2/librashader/backends/d3d11_bridge.cpp
        sdl2/librashader/d3d11_presenter.cpp
        external/imgui/backends/imgui_impl_dx11.cpp)
endif()
if(UNIX AND NOT APPLE AND NOT WIN32 AND VAEG_ENABLE_LIBRASHADER)
    list(APPEND VAEG_SDL2_SOURCES
        sdl2/librashader/backends/gl_bridge.cpp
        sdl2/librashader/gl_presenter.cpp
        external/imgui/backends/imgui_impl_opengl3.cpp)
endif()
if(VAEG_ENABLE_TESTS)
    list(APPEND VAEG_SDL2_SOURCES
        tests/idp/m69_status_composition.c
        tests/upd780/subsystem_integration.cpp
        tests/upd9002/direct_harness.c
        tests/upd9002/brkem_upd70008.c
        tests/upd9002/main_cpu_roles.c
		tests/upd9002/dispatch_normalization.c
        tests/upd9002/flags_materialization.c
        tests/upd9002/iret_restoration.c
        tests/upd9002/m65a_ff7.c
        tests/upd9002/m65b_bound.c
        tests/upd9002/m65c_f72.c
        tests/upd9002/m65d_ff6.c
        tests/upd9002/m65e_tail10.c
        tests/upd9002/m96d_nop.c
        tests/upd9002/m106_near_jcc.c
        tests/upd9002/m68_segmented_memory.c
        tests/upd9002/m70_prefix_string.c
        tests/upd9002/m64_semantics.c
        tests/upd9002/mov_imm_register.c
        tests/upd9002/semantics_bundle.c
        tests/upd9002/fixtures.c
		tests/upd9002/rep0f_diagnostic_stop.c
        tests/upd9002/state_scenario.c
        tests/upd9002/statsave_boundary.c
        tests/upd9002/ssts_worker.c)
endif()

set(VAEG_SPLASH_DATA "${CMAKE_CURRENT_BINARY_DIR}/generated/vaeg_splash_data.c")
add_custom_command(
    OUTPUT "${VAEG_SPLASH_DATA}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory
        "${CMAKE_CURRENT_BINARY_DIR}/generated"
    COMMAND "${CMAKE_COMMAND}"
        "-DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/assets/vaeg.bmp"
        "-DOUTPUT=${VAEG_SPLASH_DATA}"
        "-DSYMBOL=vaeg_splash_bmp"
        "-DSOURCE_LABEL=assets/vaeg.bmp"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_binary.cmake"
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/vaeg.bmp"
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_binary.cmake"
    VERBATIM)
list(APPEND VAEG_SDL2_SOURCES "${VAEG_SPLASH_DATA}")

set(VAEG_GUI_FONT_DATA "${CMAKE_CURRENT_BINARY_DIR}/generated/vaeg_gui_font_data.c")
add_custom_command(
    OUTPUT "${VAEG_GUI_FONT_DATA}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory
        "${CMAKE_CURRENT_BINARY_DIR}/generated"
    COMMAND "${CMAKE_COMMAND}"
        "-DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/assets/NotoSansJP-Regular.ttf"
        "-DOUTPUT=${VAEG_GUI_FONT_DATA}"
        "-DSYMBOL=vaeg_gui_font_ttf"
        "-DSOURCE_LABEL=assets/NotoSansJP-Regular.ttf"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_binary.cmake"
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/NotoSansJP-Regular.ttf"
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_binary.cmake"
    VERBATIM)
list(APPEND VAEG_SDL2_SOURCES "${VAEG_GUI_FONT_DATA}")

set(VAEG_APP_ICON_DATA "${CMAKE_CURRENT_BINARY_DIR}/generated/vaeg_app_icon_data.c")
add_custom_command(
    OUTPUT "${VAEG_APP_ICON_DATA}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory
        "${CMAKE_CURRENT_BINARY_DIR}/generated"
    COMMAND "${CMAKE_COMMAND}"
        "-DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/assets/vaeg.ico"
        "-DOUTPUT=${VAEG_APP_ICON_DATA}"
        "-DSYMBOL=vaeg_app_icon_ico"
        "-DSOURCE_LABEL=assets/vaeg.ico"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_binary.cmake"
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/vaeg.ico"
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_binary.cmake"
    VERBATIM)
list(APPEND VAEG_SDL2_SOURCES "${VAEG_APP_ICON_DATA}")

find_program(VAEG_NASM_EXECUTABLE NAMES nasm)
set(VAEG_HOSTFAT_SYS "${CMAKE_CURRENT_BINARY_DIR}/guest/hostfat.sys")
set(VAEG_R2FPROBE_COM "${CMAKE_CURRENT_BINARY_DIR}/guest/r2fprobe.com")
set(VAEG_SGPSPRITE_COM "${CMAKE_CURRENT_BINARY_DIR}/guest/sgpdemo.com")
set(VAEG_SGPWIRE_COM "${CMAKE_CURRENT_BINARY_DIR}/guest/sgpwire.com")
set(VAEG_SGP256_COM "${CMAKE_CURRENT_BINARY_DIR}/guest/sgp256.com")
set(VAEG_V480PAT_COM "${CMAKE_CURRENT_BINARY_DIR}/guest/v480pat.com")
set(VAEG_TSPMODE_COM "${CMAKE_CURRENT_BINARY_DIR}/guest/tspmode.com")
if(VAEG_NASM_EXECUTABLE)
    add_custom_command(
        OUTPUT "${VAEG_HOSTFAT_SYS}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${CMAKE_CURRENT_BINARY_DIR}/guest"
        COMMAND "${VAEG_NASM_EXECUTABLE}" -f bin
            -o "${VAEG_HOSTFAT_SYS}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/pc88va/hostfat/hostfat.asm"
        DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/pc88va/hostfat/hostfat.asm"
        VERBATIM)
    add_custom_target(hostfat_sys ALL DEPENDS "${VAEG_HOSTFAT_SYS}")
    add_custom_command(
        OUTPUT "${VAEG_R2FPROBE_COM}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${CMAKE_CURRENT_BINARY_DIR}/guest"
        COMMAND "${VAEG_NASM_EXECUTABLE}" -f bin
            -o "${VAEG_R2FPROBE_COM}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/pc88va/hostfs/r2fprobe.asm"
        DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/pc88va/hostfs/r2fprobe.asm"
        VERBATIM)
    add_custom_target(r2fprobe_com ALL DEPENDS "${VAEG_R2FPROBE_COM}")
    add_custom_command(
        OUTPUT "${VAEG_SGPSPRITE_COM}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${CMAKE_CURRENT_BINARY_DIR}/guest"
        COMMAND "${VAEG_NASM_EXECUTABLE}" -f bin
            -o "${VAEG_SGPSPRITE_COM}"
            "${CMAKE_CURRENT_SOURCE_DIR}/demos/sgp-pseudo-sprite/sgp_sprite.asm"
        DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/demos/sgp-pseudo-sprite/sgp_sprite.asm"
        VERBATIM)
    add_custom_target(sgpsprite_com ALL DEPENDS "${VAEG_SGPSPRITE_COM}")
    add_custom_command(
        OUTPUT "${VAEG_SGPWIRE_COM}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${CMAKE_CURRENT_BINARY_DIR}/guest"
        COMMAND "${VAEG_NASM_EXECUTABLE}" -f bin
            -o "${VAEG_SGPWIRE_COM}"
            "${CMAKE_CURRENT_SOURCE_DIR}/demos/sgp-wireframe/sgp_wireframe.asm"
        DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/demos/sgp-wireframe/sgp_wireframe.asm"
        VERBATIM)
    add_custom_target(sgpwire_com ALL DEPENDS "${VAEG_SGPWIRE_COM}")
    add_custom_command(
        OUTPUT "${VAEG_SGP256_COM}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${CMAKE_CURRENT_BINARY_DIR}/guest"
        COMMAND "${VAEG_NASM_EXECUTABLE}" -f bin
            -o "${VAEG_SGP256_COM}"
            "${CMAKE_CURRENT_SOURCE_DIR}/demos/sgp-wireframe/sgp_wireframe256.asm"
        DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/demos/sgp-wireframe/sgp_wireframe256.asm"
        VERBATIM)
    add_custom_target(sgp256_com ALL DEPENDS "${VAEG_SGP256_COM}")
    add_custom_command(
        OUTPUT "${VAEG_V480PAT_COM}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${CMAKE_CURRENT_BINARY_DIR}/guest"
        COMMAND "${VAEG_NASM_EXECUTABLE}" -f bin
            -o "${VAEG_V480PAT_COM}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/pc88va/vtiming/v480pat.asm"
        DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/pc88va/vtiming/v480pat.asm"
        VERBATIM)
    add_custom_target(v480pat_com ALL DEPENDS "${VAEG_V480PAT_COM}")
    add_custom_command(
        OUTPUT "${VAEG_TSPMODE_COM}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${CMAKE_CURRENT_BINARY_DIR}/guest"
        COMMAND "${VAEG_NASM_EXECUTABLE}" -f bin
            -o "${VAEG_TSPMODE_COM}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/pc88va/vtiming/tspmode.asm"
        DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/pc88va/vtiming/tspmode.asm"
        VERBATIM)
    add_custom_target(tspmode_com ALL DEPENDS "${VAEG_TSPMODE_COM}")
    message(STATUS "HOSTFAT.SYS: enabled (${VAEG_NASM_EXECUTABLE})")
    message(STATUS "R2FPROBE.COM: enabled (${VAEG_NASM_EXECUTABLE})")
    message(STATUS "SGPDEMO.COM: enabled (${VAEG_NASM_EXECUTABLE})")
    message(STATUS "SGPWIRE.COM: enabled (${VAEG_NASM_EXECUTABLE})")
    message(STATUS "SGP256.COM: enabled (${VAEG_NASM_EXECUTABLE})")
    message(STATUS "V480PAT.COM: enabled (${VAEG_NASM_EXECUTABLE})")
    message(STATUS "TSPMODE.COM: enabled (${VAEG_NASM_EXECUTABLE})")
else()
    message(STATUS "HOSTFAT.SYS: NASM not found; guest-driver target disabled")
    message(STATUS "R2FPROBE.COM: NASM not found; guest-probe target disabled")
    message(STATUS "SGPDEMO.COM: NASM not found; guest-demo target disabled")
    message(STATUS "SGPWIRE.COM: NASM not found; guest-demo target disabled")
    message(STATUS "SGP256.COM: NASM not found; guest-demo target disabled")
    message(STATUS "V480PAT.COM: NASM not found; guest-test target disabled")
    message(STATUS "TSPMODE.COM: NASM not found; guest-test target disabled")
endif()

if(WIN32)
    enable_language(RC)
    list(APPEND VAEG_SDL2_SOURCES sdl2/vaeg.rc)
endif()

add_library(vaeg_common STATIC ${VAEG_COMMON_SOURCES})
target_include_directories(vaeg_common PUBLIC ${VAEG_INCLUDE_DIRS})
target_compile_options(vaeg_common PRIVATE -Wall -fsigned-char)
if(VAEG_WERROR)
    target_compile_options(vaeg_common PRIVATE -Werror)
endif()

# Berkeley SoftFloat Release 3e is vendored byte-for-byte under external/.
# Build the exact 8086-SSE specialization selected by the supplied upstream
# makefile; the emulator wrapper remains outside the vendored subtree.
include(cmake/softfloat_sources.cmake)
add_library(vaeg_softfloat STATIC
    ${VAEG_SOFTFLOAT_GENERIC_SOURCES}
    ${VAEG_SOFTFLOAT_SPECIALIZED_SOURCES})
target_include_directories(vaeg_softfloat PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/external/softfloat/SoftFloat-3e/source/8086-SSE"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/softfloat/SoftFloat-3e/source/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/softfloat/SoftFloat-3e/build/Linux-x86_64-GCC")
target_compile_definitions(vaeg_softfloat PUBLIC
    SOFTFLOAT_FAST_INT64
    SOFTFLOAT_ROUND_ODD
    INLINE_LEVEL=5
    SOFTFLOAT_FAST_DIV32TO16
    SOFTFLOAT_FAST_DIV64TO32)
target_compile_options(vaeg_softfloat PRIVATE -O2)

if(VAEG_Z80_COMPAT_INTEGRATION_TRACE)
    list(APPEND VAEG_CORE_SOURCES cpu/upd9002/upd9002_trace.c)
endif()
if(VAEG_UPD9002_PERF_DIAGNOSTIC)
    list(APPEND VAEG_CORE_SOURCES cpu/upd9002/upd9002_perf.c)
endif()

add_library(vaeg_core STATIC ${VAEG_CORE_SOURCES})
target_include_directories(vaeg_core PUBLIC ${VAEG_INCLUDE_DIRS})
target_link_libraries(vaeg_core PUBLIC vaeg_common vaeg_softfloat)
target_compile_definitions(vaeg_core PUBLIC
    SUPPORT_V30ORIGINAL)
if(VAEG_ENABLE_TESTS)
    target_compile_definitions(vaeg_core PRIVATE
        VAEG_UPD9002_M42_TESTING=1
		VAEG_UPD9002_M46_TESTING=1
        VAEG_UPD9002_SSTS_TESTING=1)
endif()
if(VAEG_Z80_COMPAT_INTEGRATION_TRACE)
    target_compile_definitions(vaeg_core PRIVATE
        VAEG_Z80_COMPAT_INTEGRATION_TRACE=1)
endif()
if(VAEG_UPD9002_PERF_DIAGNOSTIC)
    target_compile_definitions(vaeg_core PUBLIC
        VAEG_UPD9002_PERF_DIAGNOSTIC=1)
endif()
target_compile_options(vaeg_core PRIVATE -Wall -fsigned-char)
if(VAEG_WERROR)
    target_compile_options(vaeg_core PRIVATE -Werror)
endif()

add_executable(vaeg_8087_tests tools/8087/test_core.c)
target_include_directories(vaeg_8087_tests PRIVATE ${VAEG_INCLUDE_DIRS})
target_link_libraries(vaeg_8087_tests PRIVATE vaeg_core vaeg_softfloat)
target_compile_options(vaeg_8087_tests PRIVATE -Wall -fsigned-char)
if(VAEG_WERROR)
    target_compile_options(vaeg_8087_tests PRIVATE -Werror)
endif()

add_library(vaeg_va STATIC ${VAEG_VA_SOURCES})
target_include_directories(vaeg_va PUBLIC ${VAEG_INCLUDE_DIRS})
target_link_libraries(vaeg_va PUBLIC vaeg_core vaeg_common)
target_compile_options(vaeg_va PRIVATE -Wall -fsigned-char)
if(VAEG_ENABLE_TESTS)
    target_compile_definitions(vaeg_va PRIVATE VAEG_UPD9002_M76_TESTING=1)
endif()
target_include_directories(vaeg_va PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/external/suzukiplan-z80")
target_compile_definitions(vaeg_va PRIVATE
    Z80_DISABLE_BREAKPOINT
    Z80_DISABLE_DEBUG
    Z80_DISABLE_NESTCHECK
    Z80_NO_EXCEPTION
    Z80_UNSUPPORT_16BIT_PORT)
if(VAEG_Z80_COMPAT_INTEGRATION_TRACE)
    target_compile_definitions(vaeg_va PRIVATE
        VAEG_Z80_COMPAT_INTEGRATION_TRACE=1)
endif()
if(VAEG_ENABLE_TESTS)
    target_compile_definitions(vaeg_va PUBLIC
        VAEG_UPD780_INTEGRATION_TESTING=1)
endif()
if(VAEG_WERROR)
    target_compile_options(vaeg_va PRIVATE -Werror)
endif()

set(VAEG_SDL2_EXE_OPTIONS)
if(WIN32 AND NOT VAEG_WINDOWS_CONSOLE)
    list(APPEND VAEG_SDL2_EXE_OPTIONS WIN32)
endif()
add_executable(vaeg_sdl2 ${VAEG_SDL2_EXE_OPTIONS} ${VAEG_SDL2_SOURCES})
set_target_properties(vaeg_sdl2 PROPERTIES
    OUTPUT_NAME vaeg
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sdl2")
target_include_directories(vaeg_sdl2 PRIVATE
    ${VAEG_INCLUDE_DIRS}
    "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/ymfm/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/librashader/include")
if(VAEG_ENABLE_RECORDING)
    target_compile_definitions(vaeg_sdl2 PRIVATE VAEG_ENABLE_RECORDING=1)
else()
    target_compile_definitions(vaeg_sdl2 PRIVATE VAEG_ENABLE_RECORDING=0)
endif()
if(VAEG_ENABLE_LIBRASHADER)
    target_compile_definitions(vaeg_sdl2 PRIVATE VAEG_ENABLE_LIBRASHADER=1)
    if(VAEG_STATIC_LIBRASHADER)
        include(cmake/builtin_shaders.cmake)
        target_sources(vaeg_sdl2 PRIVATE sdl2/librashader/builtin_shaders.cpp)
        if(MINGW)
            target_sources(vaeg_sdl2 PRIVATE sdl2/librashader/optional_dxc.cpp)
        endif()
        if(NOT EXISTS "${VAEG_LIBRASHADER_STATIC_LIBRARY}")
            message(FATAL_ERROR "Static librashader requires VAEG_LIBRASHADER_STATIC_LIBRARY")
        endif()
        if(NOT EXISTS "${VAEG_LIBRASHADER_STATIC_NOTICES}")
            message(FATAL_ERROR "Static librashader requires generated dependency notices")
        endif()
        get_filename_component(libra_audit_dir "${VAEG_LIBRASHADER_STATIC_NOTICES}" DIRECTORY)
        file(READ "${libra_audit_dir}/static-build.json" libra_audit)
        string(JSON libra_pin GET "${libra_audit}" source_revision)
        string(JSON libra_expected GET "${libra_audit}" archive_sha256)
        string(JSON libra_notice_expected GET "${libra_audit}" notices_sha256)
        string(JSON libra_unresolved LENGTH "${libra_audit}" unresolved_notices)
        file(SHA256 "${VAEG_LIBRASHADER_STATIC_LIBRARY}" libra_actual)
        file(SHA256 "${VAEG_LIBRASHADER_STATIC_NOTICES}" libra_notice_actual)
        if(NOT libra_pin STREQUAL "87e8a97b50516d997defeaa168173dcd185d4022" OR
           NOT libra_expected STREQUAL libra_actual OR
           NOT libra_notice_expected STREQUAL libra_notice_actual OR libra_unresolved)
            message(FATAL_ERROR "Static librashader source/archive/license audit mismatch")
        endif()
        target_compile_definitions(vaeg_sdl2 PRIVATE VAEG_STATIC_LIBRASHADER=1)
        target_link_libraries(vaeg_sdl2 PRIVATE
            "${VAEG_LIBRASHADER_STATIC_LIBRARY}" ${VAEG_LIBRASHADER_NATIVE_LIBS})
    endif()
endif()
target_link_libraries(vaeg_sdl2 PRIVATE vaeg_core vaeg_va vaeg_common
    vaeg_core vaeg_va Threads::Threads ${VAEG_SDL2_LIBS} vaeg_recording_mp4)
if(VAEG_ENABLE_RECORDING)
    target_compile_definitions(vaeg_sdl2 PRIVATE VAEG_RECORDING_FFMPEG=1)
    target_link_libraries(vaeg_sdl2 PRIVATE ${VAEG_FFMPEG_TARGETS})
    if(APPLE AND VAEG_HAVE_SCREEN_CAPTURE_KIT_GEOMETRY)
        target_link_libraries(vaeg_sdl2 PRIVATE
            "-framework Cocoa" "-framework CoreMedia" "-framework CoreVideo"
            "-weak_framework ScreenCaptureKit")
    endif()
endif()
if(APPLE AND VAEG_ENABLE_LIBRASHADER)
    target_link_libraries(vaeg_sdl2 PRIVATE
        "-framework Foundation"
        "-framework Metal"
        "-framework QuartzCore")
endif()
if(WIN32 AND VAEG_ENABLE_LIBRASHADER)
    target_compile_definitions(vaeg_sdl2 PRIVATE SDL_VIDEO_DRIVER_WINDOWS=1)
    target_link_libraries(vaeg_sdl2 PRIVATE d3d11 dxgi d3dcompiler)
endif()
if(VAEG_ENABLE_TESTS)
    target_compile_definitions(vaeg_sdl2 PRIVATE
        VAEG_UPD9002_M42_TESTING=1
        VAEG_UPD9002_M76_TESTING=1
        VAEG_UPD9002_M44_TESTING=1
		VAEG_UPD9002_M46_TESTING=1
        VAEG_UPD9002_M48_TESTING=1
        VAEG_UPD9002_M60A_TESTING=1
        VAEG_UPD9002_M60E_TESTING=1
        VAEG_UPD9002_M61_TESTING=1
        VAEG_UPD9002_M62_TESTING=1
        VAEG_UPD9002_M64_TESTING=1
        VAEG_UPD9002_M65A_TESTING=1
        VAEG_UPD9002_M65B_TESTING=1
        VAEG_UPD9002_M65C_TESTING=1
        VAEG_UPD9002_M65D_TESTING=1
        VAEG_UPD9002_M65E_TESTING=1
        VAEG_UPD9002_M96D_TESTING=1
        VAEG_UPD9002_M106_TESTING=1
        VAEG_UPD9002_M68_TESTING=1
        VAEG_UPD9002_M70_TESTING=1
        VAEG_IDP_M69_TESTING=1
        VAEG_UPD9002_SSTS_TESTING=1
        "VAEG_UPD9002_HARNESS_MANIFEST_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/upd9002/harness_manifest.csv\""
        "VAEG_UPD9002_FIXTURE_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/upd9002/state_fixtures_m42.txt\"")
endif()
if(VAEG_Z80_COMPAT_INTEGRATION_TRACE)
    target_compile_definitions(vaeg_sdl2 PRIVATE
        VAEG_Z80_COMPAT_INTEGRATION_TRACE=1)
endif()
if(VAEG_ENABLE_ARCHIVE_DROP)
    if(MINGW OR VAEG_FETCH_LIBARCHIVE)
        include(FetchContent)
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

        set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(vaeg_zlib
            URL "https://github.com/madler/zlib/releases/download/v1.3.1/zlib-1.3.1.tar.xz"
            URL_HASH SHA256=38ef96b8dfe510d42707d9c781877914792541133e1870841463bfa73f883e32
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
        FetchContent_MakeAvailable(vaeg_zlib)
        configure_file("${vaeg_zlib_SOURCE_DIR}/zlib.h"
            "${vaeg_zlib_BINARY_DIR}/zlib.h" COPYONLY)

        set(XZ_NLS OFF CACHE BOOL "" FORCE)
        set(XZ_TOOL_XZ OFF CACHE BOOL "" FORCE)
        set(XZ_TOOL_XZDEC OFF CACHE BOOL "" FORCE)
        set(XZ_TOOL_LZMADEC OFF CACHE BOOL "" FORCE)
        set(XZ_TOOL_LZMAINFO OFF CACHE BOOL "" FORCE)
        set(XZ_DOC OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(vaeg_xz
            URL "https://github.com/tukaani-project/xz/releases/download/v5.8.3/xz-5.8.3.tar.xz"
            URL_HASH SHA256=fff1ffcf2b0da84d308a14de513a1aa23d4e9aa3464d17e64b9714bfdd0bbfb6
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
        FetchContent_MakeAvailable(vaeg_xz)

        set(ZLIB_USE_STATIC_LIBS ON CACHE BOOL "" FORCE)
        set(ZLIB_INCLUDE_DIR "${vaeg_zlib_BINARY_DIR}" CACHE PATH "" FORCE)
        if(WIN32)
            set(VAEG_ZLIB_STATIC "${vaeg_zlib_BINARY_DIR}/libzlibstatic.a")
        else()
            set(VAEG_ZLIB_STATIC "${vaeg_zlib_BINARY_DIR}/libz.a")
        endif()
        set(ZLIB_LIBRARY "${VAEG_ZLIB_STATIC}" CACHE FILEPATH "" FORCE)
        set(LIBLZMA_INCLUDE_DIR "${vaeg_xz_SOURCE_DIR}/src/liblzma/api"
            CACHE PATH "" FORCE)
        set(LIBLZMA_LIBRARY "${vaeg_xz_BINARY_DIR}/liblzma.a"
            CACHE FILEPATH "" FORCE)
        # FindLibLZMA cannot link-test a same-configure FetchContent target.
        # The pinned liblzma target exports these API functions once built.
        set(LIBLZMA_HAS_AUTO_DECODER TRUE CACHE BOOL "" FORCE)
        set(LIBLZMA_HAS_EASY_ENCODER TRUE CACHE BOOL "" FORCE)
        set(LIBLZMA_HAS_LZMA_PRESET TRUE CACHE BOOL "" FORCE)
        set(LZMA_API_STATIC TRUE CACHE BOOL "" FORCE)
        set(WITHOUT_LZMA_API_STATIC FALSE CACHE BOOL "" FORCE)

        set(ENABLE_WERROR OFF CACHE BOOL "" FORCE)
        set(ENABLE_TEST OFF CACHE BOOL "" FORCE)
        set(ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
        set(ENABLE_TAR OFF CACHE BOOL "" FORCE)
        set(ENABLE_CPIO OFF CACHE BOOL "" FORCE)
        set(ENABLE_CAT OFF CACHE BOOL "" FORCE)
        set(ENABLE_UNZIP OFF CACHE BOOL "" FORCE)
        set(ENABLE_OPENSSL OFF CACHE BOOL "" FORCE)
        set(ENABLE_MBEDTLS OFF CACHE BOOL "" FORCE)
        set(ENABLE_NETTLE OFF CACHE BOOL "" FORCE)
        set(ENABLE_LIBB2 OFF CACHE BOOL "" FORCE)
        set(ENABLE_LZ4 OFF CACHE BOOL "" FORCE)
        set(ENABLE_LZO OFF CACHE BOOL "" FORCE)
        set(ENABLE_ZSTD OFF CACHE BOOL "" FORCE)
        set(ENABLE_BZip2 OFF CACHE BOOL "" FORCE)
        set(ENABLE_LIBXML2 OFF CACHE BOOL "" FORCE)
        set(ENABLE_EXPAT OFF CACHE BOOL "" FORCE)
        set(ENABLE_WIN32_XMLLITE OFF CACHE BOOL "" FORCE)
        set(ENABLE_PCREPOSIX OFF CACHE BOOL "" FORCE)
        set(ENABLE_PCRE2POSIX OFF CACHE BOOL "" FORCE)
        set(ENABLE_XATTR OFF CACHE BOOL "" FORCE)
        set(ENABLE_ACL OFF CACHE BOOL "" FORCE)
        set(ENABLE_ICONV OFF CACHE BOOL "" FORCE)
        set(ENABLE_CNG OFF CACHE BOOL "" FORCE)
        set(ENABLE_LIBGCC OFF CACHE BOOL "" FORCE)
        set(POSIX_REGEX_LIB NONE CACHE STRING "" FORCE)
        set(ENABLE_ZLIB ON CACHE BOOL "" FORCE)
        set(ENABLE_LZMA ON CACHE BOOL "" FORCE)
        FetchContent_Declare(vaeg_libarchive
            URL "https://github.com/libarchive/libarchive/releases/download/v3.8.7/libarchive-3.8.7.tar.xz"
            URL_HASH SHA256=d3a8ba457ae25c27c84fd2830a2efdcc5b1d40bf585d4eb0d35f47e99e5d4774
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
        # LibArchive's Unix type probes expect the platform's C extensions.
        # Keep vaeg's own C targets on strict C99 outside this dependency.
        set(VAEG_SAVED_CMAKE_C_EXTENSIONS ${CMAKE_C_EXTENSIONS})
        set(CMAKE_C_EXTENSIONS ON)
        FetchContent_MakeAvailable(vaeg_libarchive)
        set(CMAKE_C_EXTENSIONS ${VAEG_SAVED_CMAKE_C_EXTENSIONS})
        add_dependencies(archive_static zlibstatic liblzma)
        target_compile_definitions(vaeg_sdl2 PRIVATE
            VAEG_HAVE_LIBARCHIVE LIBARCHIVE_STATIC)
        target_link_libraries(vaeg_sdl2 PRIVATE archive_static)
        message(STATUS
            "Archive disk-image drop: enabled (static fetched LibArchive 3.8.7)")
    else()
        find_package(LibArchive QUIET)
        if(LibArchive_FOUND)
            target_compile_definitions(vaeg_sdl2 PRIVATE VAEG_HAVE_LIBARCHIVE)
            if(TARGET LibArchive::LibArchive)
                target_link_libraries(vaeg_sdl2 PRIVATE LibArchive::LibArchive)
            else()
                target_include_directories(vaeg_sdl2 PRIVATE ${LibArchive_INCLUDE_DIRS})
                target_link_libraries(vaeg_sdl2 PRIVATE ${LibArchive_LIBRARIES})
            endif()
            message(STATUS "Archive disk-image drop: enabled (${LibArchive_VERSION})")
        else()
            message(STATUS "Archive disk-image drop: disabled (LibArchive not found)")
        endif()
    endif()
else()
    message(STATUS "Archive disk-image drop: disabled by VAEG_ENABLE_ARCHIVE_DROP")
endif()
include(cmake/third_party_licenses.cmake)

if(MINGW)
    target_link_options(vaeg_sdl2 PRIVATE -static -static-libgcc -static-libstdc++)
    target_link_libraries(vaeg_sdl2 PRIVATE winpthread)
endif()
if(UNIX)
    target_link_libraries(vaeg_sdl2 PRIVATE m)
endif()
if(WIN32 AND VAEG_FETCH_SDL2 AND NOT VAEG_STATIC_SDL2)
    add_custom_command(TARGET vaeg_sdl2 POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:SDL2::SDL2>"
            "$<TARGET_FILE_DIR:vaeg_sdl2>")
endif()
target_compile_options(vaeg_sdl2 PRIVATE -Wall -fsigned-char)
if(VAEG_WERROR)
    target_compile_options(vaeg_sdl2 PRIVATE -Werror)
endif()

if(VAEG_ENABLE_TESTS)
    enable_testing()
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_test(NAME vaeg_mingw_macports_sdk
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/frontend/test_mingw_macports.py")
    add_test(NAME vaeg_release_identity
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/frontend/test_release_identity.py")
    add_test(NAME vaeg_tvram_vb_builder
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/hardware/tvram_vb/test_build.py")
    add_test(NAME vaeg_runtime_state_contract
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/runtime_state_roundtrip.py" --selftest)
    add_executable(vaeg_recording_mp4_test tests/frontend/recording/test_recording_mp4.cpp)
    target_include_directories(vaeg_recording_mp4_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/sdl2")
    target_compile_features(vaeg_recording_mp4_test PRIVATE cxx_std_17)
    target_link_libraries(vaeg_recording_mp4_test PRIVATE vaeg_recording_mp4)
    if(WIN32)
        target_link_options(vaeg_recording_mp4_test PRIVATE -municode)
    endif()
    add_test(NAME vaeg_recording_mp4 COMMAND vaeg_recording_mp4_test)
    if(APPLE)
        add_executable(vaeg_recording_autorelease_pool_test
            tests/frontend/recording/test_recording_autorelease_pool.mm)
        target_include_directories(vaeg_recording_autorelease_pool_test PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/sdl2")
        target_compile_features(vaeg_recording_autorelease_pool_test PRIVATE cxx_std_17)
        target_compile_options(vaeg_recording_autorelease_pool_test PRIVATE -fobjc-arc)
        target_link_libraries(vaeg_recording_autorelease_pool_test PRIVATE "-framework Foundation")
        add_test(NAME vaeg_recording_autorelease_pool COMMAND vaeg_recording_autorelease_pool_test)
        set_tests_properties(vaeg_recording_autorelease_pool PROPERTIES
            LABELS "frontend;recording;romless;mp4")
    endif()
    add_test(NAME vaeg_mp4_build_policy
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/frontend/recording/test_mp4_policy.py")
    set_tests_properties(vaeg_recording_mp4 vaeg_mp4_build_policy PROPERTIES
        LABELS "frontend;recording;romless;mp4")

    add_test(NAME vaeg_ffmpeg_static_resolution
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/frontend/recording/test_static_ffmpeg.py")
    set_tests_properties(vaeg_ffmpeg_static_resolution PROPERTIES
        LABELS "frontend;portability;recording")
    add_test(NAME vaeg_release_recording_contract
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/frontend/recording/test_release_recording.py")
    set_tests_properties(vaeg_release_recording_contract PROPERTIES
        LABELS "frontend;portability;recording")
    foreach(compiler_header_language IN ITEMS c cpp)
        add_executable(vaeg_compiler_header_${compiler_header_language}_test
            tests/frontend/test_compiler_header.${compiler_header_language})
        target_include_directories(vaeg_compiler_header_${compiler_header_language}_test
            PRIVATE ${VAEG_INCLUDE_DIRS})
        add_test(NAME vaeg_compiler_header_${compiler_header_language}
            COMMAND vaeg_compiler_header_${compiler_header_language}_test)
        set_tests_properties(vaeg_compiler_header_${compiler_header_language} PROPERTIES
            LABELS "frontend;portability;recording")
    endforeach()
    add_executable(vaeg_recording_realtime_clock_test
        tests/frontend/recording/test_recording_realtime_clock.cpp
        sdl2/recording/recording_realtime_clock.cpp)
    target_include_directories(vaeg_recording_realtime_clock_test PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/sdl2")
    target_compile_features(vaeg_recording_realtime_clock_test PRIVATE cxx_std_17)
    target_link_libraries(vaeg_recording_realtime_clock_test PRIVATE Threads::Threads)
    target_compile_options(vaeg_recording_realtime_clock_test PRIVATE -Wall)
    if(VAEG_WERROR)
        target_compile_options(vaeg_recording_realtime_clock_test PRIVATE -Werror)
    endif()
    add_test(NAME vaeg_recording_realtime_clock COMMAND vaeg_recording_realtime_clock_test)
    set_tests_properties(vaeg_recording_realtime_clock PROPERTIES
        LABELS "romless;recording;realtime;clock")
    add_executable(vaeg_recording_c_contract_test
        tests/frontend/recording/test_recording_c.c
        sdl2/recording/recording.c)
    target_include_directories(vaeg_recording_c_contract_test PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/sdl2"
        "${CMAKE_CURRENT_SOURCE_DIR}")
    target_compile_definitions(vaeg_recording_c_contract_test PRIVATE
        VAEG_ENABLE_RECORDING=0)
    target_compile_options(vaeg_recording_c_contract_test PRIVATE
        -Wall -fsigned-char)
    if(VAEG_WERROR)
        target_compile_options(vaeg_recording_c_contract_test PRIVATE -Werror)
    endif()
    add_test(NAME vaeg_recording_c_contracts
        COMMAND vaeg_recording_c_contract_test)
    set_tests_properties(vaeg_recording_c_contracts PROPERTIES
        LABELS "romless;recording;contracts")

    add_executable(vaeg_recording_cli_test
        tests/frontend/recording/test_recording_cli.c
        sdl2/cliopts.c)
    target_include_directories(vaeg_recording_cli_test PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/cpu"
        "${CMAKE_CURRENT_SOURCE_DIR}/sdl2"
        "${CMAKE_CURRENT_SOURCE_DIR}/common"
        "${CMAKE_CURRENT_SOURCE_DIR}/io"
        "${CMAKE_CURRENT_SOURCE_DIR}/machine"
        "${CMAKE_CURRENT_SOURCE_DIR}")
    target_compile_options(vaeg_recording_cli_test PRIVATE
        -Wall -fsigned-char)
    if(VAEG_WERROR)
        target_compile_options(vaeg_recording_cli_test PRIVATE -Werror)
    endif()
    add_test(NAME vaeg_recording_cli
        COMMAND vaeg_recording_cli_test)
    set_tests_properties(vaeg_recording_cli PROPERTIES
        LABELS "romless;recording;cli")

    add_executable(vaeg_recording_osd_snapshot_test
        tests/frontend/recording/test_recording_osd_snapshot.c
        sdl2/recording/recording_osd_snapshot.c)
    target_include_directories(vaeg_recording_osd_snapshot_test PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/sdl2"
        "${CMAKE_CURRENT_SOURCE_DIR}")
    target_compile_options(vaeg_recording_osd_snapshot_test PRIVATE
        -Wall -fsigned-char)
    if(VAEG_WERROR)
        target_compile_options(vaeg_recording_osd_snapshot_test PRIVATE -Werror)
    endif()
    add_test(NAME vaeg_recording_osd_snapshot
        COMMAND vaeg_recording_osd_snapshot_test)
    set_tests_properties(vaeg_recording_osd_snapshot PROPERTIES
        LABELS "romless;recording;osd")

    add_executable(vaeg_recording_displayed_capture_test
        tests/frontend/recording/test_recording_displayed_capture.c
        sdl2/recording/recording_displayed_capture.c)
    target_include_directories(vaeg_recording_displayed_capture_test PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/sdl2"
        "${CMAKE_CURRENT_SOURCE_DIR}")
    target_compile_options(vaeg_recording_displayed_capture_test PRIVATE
        -Wall -fsigned-char)
    if(VAEG_WERROR)
        target_compile_options(vaeg_recording_displayed_capture_test PRIVATE -Werror)
    endif()
    add_test(NAME vaeg_recording_displayed_capture
        COMMAND vaeg_recording_displayed_capture_test)
    set_tests_properties(vaeg_recording_displayed_capture PROPERTIES
        LABELS "romless;recording;displayed;readback")

    add_executable(vaeg_recording_runtime_test
        tests/frontend/recording/test_recording_runtime.c
        sdl2/recording/recording_runtime.c)
    target_include_directories(vaeg_recording_runtime_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/sdl2")
    target_compile_features(vaeg_recording_runtime_test PRIVATE c_std_99)
    add_test(NAME vaeg_recording_runtime COMMAND vaeg_recording_runtime_test)
    set_tests_properties(vaeg_recording_runtime PROPERTIES LABELS "romless;recording;frontend")

    add_executable(vaeg_recording_native_composition_test
        tests/frontend/recording/test_recording_native_composition.c
        sdl2/recording/recording_native_composition.c
        sdl2/recording/recording_osd_snapshot.c)
    target_include_directories(vaeg_recording_native_composition_test PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/sdl2"
        "${CMAKE_CURRENT_SOURCE_DIR}")
    target_compile_options(vaeg_recording_native_composition_test PRIVATE
        -Wall -fsigned-char)
    if(VAEG_WERROR)
        target_compile_options(vaeg_recording_native_composition_test PRIVATE -Werror)
    endif()
    add_test(NAME vaeg_recording_native_composition
        COMMAND vaeg_recording_native_composition_test)
    set_tests_properties(vaeg_recording_native_composition PROPERTIES
        LABELS "romless;recording;native;osd")

    add_executable(vaeg_recording_native_tap_test
        tests/frontend/recording/test_recording_native_tap.c
        sdl2/recording/recording_native_tap.c)
    target_include_directories(vaeg_recording_native_tap_test PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/sdl2"
        "${CMAKE_CURRENT_SOURCE_DIR}")
    target_compile_options(vaeg_recording_native_tap_test PRIVATE
        -Wall -fsigned-char)
    if(VAEG_WERROR)
        target_compile_options(vaeg_recording_native_tap_test PRIVATE -Werror)
    endif()
    add_test(NAME vaeg_recording_native_tap
        COMMAND vaeg_recording_native_tap_test)
    set_tests_properties(vaeg_recording_native_tap PROPERTIES
        LABELS "romless;recording;native;video;timing")

    add_executable(vaeg_sound_generation_test
        tests/frontend/recording/test_sound_generation.c
        sound/sound_generation.c
        common/parts.c)
    target_include_directories(vaeg_sound_generation_test PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/sdl2"
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/common"
        "${CMAKE_CURRENT_SOURCE_DIR}/sound")
    target_compile_options(vaeg_sound_generation_test PRIVATE
        -Wall -fsigned-char)
    if(VAEG_WERROR)
        target_compile_options(vaeg_sound_generation_test PRIVATE -Werror)
    endif()
    add_test(NAME vaeg_sound_generation
        COMMAND vaeg_sound_generation_test)
    set_tests_properties(vaeg_sound_generation PROPERTIES
        LABELS "romless;recording;sound")

    add_executable(vaeg_sound_capture_test
        tests/frontend/recording/test_sound_capture.c
        sound/sound_capture.c)
    target_include_directories(vaeg_sound_capture_test PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/sdl2"
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/common"
        "${CMAKE_CURRENT_SOURCE_DIR}/sound")
    target_compile_options(vaeg_sound_capture_test PRIVATE
        -Wall -fsigned-char)
    if(VAEG_WERROR)
        target_compile_options(vaeg_sound_capture_test PRIVATE -Werror)
    endif()
    add_test(NAME vaeg_sound_capture
        COMMAND vaeg_sound_capture_test)
    set_tests_properties(vaeg_sound_capture PROPERTIES
        LABELS "romless;recording;sound;audio")

    add_executable(vaeg_recording_contract_test
        tests/frontend/recording/test_recording_contracts.cpp
        sdl2/recording/recording.c
        sdl2/recording/recording_contracts.cpp)
    target_include_directories(vaeg_recording_contract_test PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/sdl2"
        "${CMAKE_CURRENT_SOURCE_DIR}")
    target_compile_features(vaeg_recording_contract_test PRIVATE cxx_std_17)
    target_compile_definitions(vaeg_recording_contract_test PRIVATE
        VAEG_ENABLE_RECORDING=0)
    target_compile_options(vaeg_recording_contract_test PRIVATE
        -Wall -fsigned-char)
    if(VAEG_WERROR)
        target_compile_options(vaeg_recording_contract_test PRIVATE -Werror)
    endif()
    add_test(NAME vaeg_recording_contracts
        COMMAND vaeg_recording_contract_test)
    set_tests_properties(vaeg_recording_contracts PROPERTIES
        LABELS "romless;recording;contracts")

    add_executable(vaeg_recording_clock_test
        tests/frontend/recording/test_recording_clock.cpp
        sdl2/recording/recording_clock.cpp)
    target_include_directories(vaeg_recording_clock_test PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/sdl2"
        "${CMAKE_CURRENT_SOURCE_DIR}")
    target_compile_features(vaeg_recording_clock_test PRIVATE cxx_std_17)
    target_compile_options(vaeg_recording_clock_test PRIVATE -Wall -fsigned-char)
    if(VAEG_WERROR)
        target_compile_options(vaeg_recording_clock_test PRIVATE -Werror)
    endif()
    add_test(NAME vaeg_recording_clock
        COMMAND vaeg_recording_clock_test)
    set_tests_properties(vaeg_recording_clock PROPERTIES
        LABELS "romless;recording;clock")

    add_executable(vaeg_recording_queue_test
        tests/frontend/recording/test_recording_queue.cpp
        sdl2/recording/recording.c
        sdl2/recording/recording_queue.cpp)
    target_include_directories(vaeg_recording_queue_test PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/sdl2"
        "${CMAKE_CURRENT_SOURCE_DIR}")
    target_compile_features(vaeg_recording_queue_test PRIVATE cxx_std_17)
    target_compile_options(vaeg_recording_queue_test PRIVATE -Wall -fsigned-char)
    target_link_libraries(vaeg_recording_queue_test PRIVATE Threads::Threads)
    if(VAEG_WERROR)
        target_compile_options(vaeg_recording_queue_test PRIVATE -Werror)
    endif()
    add_test(NAME vaeg_recording_queue
        COMMAND vaeg_recording_queue_test)
    set_tests_properties(vaeg_recording_queue PROPERTIES
        LABELS "romless;recording;queue")

    if(VAEG_ENABLE_RECORDING)
        add_executable(vaeg_recording_native_session_test
            tests/frontend/recording/test_recording_native_session.cpp
            sdl2/recording/recording_displayed_capture.c
            sdl2/recording/recording_native_session.cpp
            sdl2/recording/recording_native_tap.c
            sdl2/recording/recording_queue.cpp
            sdl2/recording/recording.c
            sdl2/recording/recording_contracts.cpp
            sdl2/recording/recording_clock.cpp
            sdl2/recording/recording_ffmpeg_video_encoder.cpp
            sdl2/recording/recording_ffmpeg_pcm_encoder.cpp
            sdl2/recording/recording_ffmpeg_matroska.cpp
            common/parts.c)
        target_include_directories(vaeg_recording_native_session_test PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpu"
            "${CMAKE_CURRENT_BINARY_DIR}/generated"
            "${CMAKE_CURRENT_SOURCE_DIR}/sdl2"
            "${CMAKE_CURRENT_SOURCE_DIR}"
            "${CMAKE_CURRENT_SOURCE_DIR}/common"
            "${CMAKE_CURRENT_SOURCE_DIR}/sound")
        target_compile_features(vaeg_recording_native_session_test PRIVATE cxx_std_17)
        target_compile_definitions(vaeg_recording_native_session_test PRIVATE
            VAEG_ENABLE_RECORDING=1
            VAEG_RECORDING_FFMPEG=1)
        target_compile_options(vaeg_recording_native_session_test PRIVATE
            -Wall -fsigned-char)
        target_link_libraries(vaeg_recording_native_session_test PRIVATE
            Threads::Threads ${VAEG_FFMPEG_TARGETS})
        if(VAEG_WERROR)
            target_compile_options(vaeg_recording_native_session_test PRIVATE -Werror)
        endif()
        add_test(NAME vaeg_recording_native_session
            COMMAND vaeg_recording_native_session_test)
        set_tests_properties(vaeg_recording_native_session PROPERTIES
            LABELS "romless;recording;native;video;audio;integration")

        add_executable(vaeg_recording_runtime_integration_test
            tests/frontend/recording/test_recording_runtime_integration.cpp
            sdl2/recording/recording_displayed_capture.c
            sdl2/recording/recording_runtime.c
            sdl2/recording/recording_native_session.cpp
            sdl2/recording/recording_native_tap.c
            sdl2/recording/recording_queue.cpp
            sdl2/recording/recording.c
            sdl2/recording/recording_contracts.cpp
            sdl2/recording/recording_clock.cpp
            sdl2/recording/recording_ffmpeg_video_encoder.cpp
            sdl2/recording/recording_ffmpeg_pcm_encoder.cpp
            sdl2/recording/recording_ffmpeg_matroska.cpp
            common/parts.c)
        target_include_directories(vaeg_recording_runtime_integration_test PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/cpu" "${CMAKE_CURRENT_BINARY_DIR}/generated"
            "${CMAKE_CURRENT_SOURCE_DIR}/sdl2" "${CMAKE_CURRENT_SOURCE_DIR}"
            "${CMAKE_CURRENT_SOURCE_DIR}/common" "${CMAKE_CURRENT_SOURCE_DIR}/sound")
        target_compile_features(vaeg_recording_runtime_integration_test PRIVATE cxx_std_17)
        target_compile_definitions(vaeg_recording_runtime_integration_test PRIVATE
            VAEG_ENABLE_RECORDING=1 VAEG_RECORDING_FFMPEG=1)
        target_compile_options(vaeg_recording_runtime_integration_test PRIVATE -Wall -fsigned-char)
        target_link_libraries(vaeg_recording_runtime_integration_test PRIVATE
            Threads::Threads ${VAEG_FFMPEG_TARGETS})
        if(VAEG_WERROR)
            target_compile_options(vaeg_recording_runtime_integration_test PRIVATE -Werror)
        endif()
        add_test(NAME vaeg_recording_runtime_integration
            COMMAND vaeg_recording_runtime_integration_test)
        set_tests_properties(vaeg_recording_runtime_integration PROPERTIES
            LABELS "romless;recording;frontend;integration")
        if(APPLE AND VAEG_ENABLE_LIBRASHADER AND VAEG_STATIC_LIBRASHADER)
            get_target_property(m100s9_archive_sources vaeg_recording_runtime_integration_test SOURCES)
            add_executable(vaeg_recording_metal_archive_probe
                ${m100s9_archive_sources}
                sdl2/librashader/frame_input.cpp
                sdl2/librashader/frame_conversion.cpp
                sdl2/librashader/native_presenter.cpp
                sdl2/librashader/metal_presenter.cpp
                sdl2/librashader/shader_parameters.cpp
                sdl2/librashader/shader_preset.cpp
                sdl2/librashader/builtin_shaders.cpp
                sdl2/librashader/backends/metal_bridge.mm
                external/imgui/imgui.cpp
                external/imgui/imgui_draw.cpp
                external/imgui/imgui_tables.cpp
                external/imgui/imgui_widgets.cpp)
            target_include_directories(vaeg_recording_metal_archive_probe PRIVATE
                $<TARGET_PROPERTY:vaeg_sdl2,INCLUDE_DIRECTORIES>)
            target_compile_features(vaeg_recording_metal_archive_probe PRIVATE cxx_std_17)
            target_compile_definitions(vaeg_recording_metal_archive_probe PRIVATE
                VAEG_ENABLE_RECORDING=1 VAEG_RECORDING_FFMPEG=1
                VAEG_ENABLE_LIBRASHADER=1 VAEG_STATIC_LIBRASHADER=1
                VAEG_RECORDING_METAL_PROBE=1
                "VAEG_METAL_FRONTEND_BINARY=\"$<TARGET_FILE:vaeg_sdl2>\"")
            add_dependencies(vaeg_recording_metal_archive_probe vaeg_sdl2)
            target_compile_options(vaeg_recording_metal_archive_probe PRIVATE -Wall -fsigned-char)
            target_link_libraries(vaeg_recording_metal_archive_probe PRIVATE
                Threads::Threads ${VAEG_FFMPEG_TARGETS} ${VAEG_SDL2_LIBS}
                "${VAEG_LIBRASHADER_STATIC_LIBRARY}" ${VAEG_LIBRASHADER_NATIVE_LIBS}
                "-framework Cocoa" "-framework Metal" "-framework QuartzCore")
            add_test(NAME vaeg_recording_metal_archive_probe
                COMMAND vaeg_recording_metal_archive_probe)
            set_tests_properties(vaeg_recording_metal_archive_probe PROPERTIES
                SKIP_RETURN_CODE 77 TIMEOUT 90 LABELS "romless;recording;metal;hardware")
        endif()

        add_executable(vaeg_recording_ffmpeg_capabilities_test
            tests/frontend/recording/test_recording_ffmpeg_capabilities.cpp
            sdl2/recording/recording_ffmpeg_capabilities.cpp)
        target_include_directories(vaeg_recording_ffmpeg_capabilities_test PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/sdl2"
            "${CMAKE_CURRENT_SOURCE_DIR}")
        target_compile_features(vaeg_recording_ffmpeg_capabilities_test PRIVATE cxx_std_17)
        target_compile_definitions(vaeg_recording_ffmpeg_capabilities_test PRIVATE
            VAEG_ENABLE_RECORDING=1
            VAEG_RECORDING_FFMPEG=1)
        target_compile_options(vaeg_recording_ffmpeg_capabilities_test PRIVATE
            -Wall -fsigned-char)
        target_link_libraries(vaeg_recording_ffmpeg_capabilities_test PRIVATE
            Threads::Threads ${VAEG_FFMPEG_TARGETS})
        if(VAEG_WERROR)
            target_compile_options(vaeg_recording_ffmpeg_capabilities_test PRIVATE -Werror)
        endif()
        add_test(NAME vaeg_recording_ffmpeg_capabilities
            COMMAND vaeg_recording_ffmpeg_capabilities_test)
        set_tests_properties(vaeg_recording_ffmpeg_capabilities PROPERTIES
            LABELS "romless;recording;ffmpeg")

        if(APPLE AND VAEG_HAVE_SCREEN_CAPTURE_KIT_GEOMETRY)
            add_executable(vaeg_recording_macos_window_capture_test
                tests/frontend/recording/test_recording_macos_window_capture.mm
                sdl2/recording/recording_macos_window_capture.mm)
            target_include_directories(vaeg_recording_macos_window_capture_test PRIVATE
                "${CMAKE_CURRENT_SOURCE_DIR}/sdl2")
            set_target_properties(vaeg_recording_macos_window_capture_test PROPERTIES
                OBJCXX_STANDARD 17 OBJCXX_STANDARD_REQUIRED YES)
            set_source_files_properties(tests/frontend/recording/test_recording_macos_window_capture.mm
                PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
            target_compile_options(vaeg_recording_macos_window_capture_test PRIVATE -Wall)
            target_link_libraries(vaeg_recording_macos_window_capture_test PRIVATE
                "-framework Cocoa" "-framework CoreMedia" "-framework CoreVideo"
                "-weak_framework ScreenCaptureKit")
            if(VAEG_WERROR)
                target_compile_options(vaeg_recording_macos_window_capture_test PRIVATE -Werror)
            endif()
            add_test(NAME vaeg_recording_macos_capture_contracts
                COMMAND vaeg_recording_macos_window_capture_test --contracts)
            set_tests_properties(vaeg_recording_macos_capture_contracts PROPERTIES
                LABELS "romless;recording;window;contracts")
            # Interactive public-window runtime probe is opt-in, not part of
            # unattended CTest and never prompts for Screen Recording consent.
        endif()
        add_executable(vaeg_recording_h264_encoder_test
            tests/frontend/recording/test_recording_h264_encoder.cpp
            sdl2/recording/recording_h264_encoder.cpp
            sdl2/recording/recording_ffmpeg_matroska.cpp
            sdl2/recording/recording_ffmpeg_pcm_encoder.cpp
            common/parts.c)
        target_include_directories(vaeg_recording_h264_encoder_test PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/sdl2"
            "${CMAKE_CURRENT_SOURCE_DIR}/common"
            "${CMAKE_CURRENT_SOURCE_DIR}")
        target_compile_features(vaeg_recording_h264_encoder_test PRIVATE cxx_std_17)
        target_compile_options(vaeg_recording_h264_encoder_test PRIVATE -Wall)
        target_link_libraries(vaeg_recording_h264_encoder_test PRIVATE ${VAEG_FFMPEG_TARGETS})
        if(VAEG_WERROR)
            target_compile_options(vaeg_recording_h264_encoder_test PRIVATE -Werror)
        endif()
        add_test(NAME vaeg_recording_h264_contracts
            COMMAND vaeg_recording_h264_encoder_test --contracts)
        add_test(NAME vaeg_recording_h264_hardware_probe
            COMMAND vaeg_recording_h264_encoder_test)
        set_tests_properties(vaeg_recording_h264_contracts PROPERTIES
            LABELS "romless;recording;realtime;h264;contracts")
        set_tests_properties(vaeg_recording_h264_hardware_probe PROPERTIES
            LABELS "romless;recording;realtime;h264;hardware" SKIP_RETURN_CODE 77)
        add_executable(vaeg_recording_ffv1_test
            tests/frontend/recording/test_recording_ffmpeg_video_encoder.cpp
            sdl2/recording/recording.c
            sdl2/recording/recording_ffmpeg_video_encoder.cpp)
        target_include_directories(vaeg_recording_ffv1_test PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/sdl2"
            "${CMAKE_CURRENT_SOURCE_DIR}")
        target_compile_features(vaeg_recording_ffv1_test PRIVATE cxx_std_17)
        target_compile_definitions(vaeg_recording_ffv1_test PRIVATE
            VAEG_ENABLE_RECORDING=1
            VAEG_RECORDING_FFMPEG=1
            VAEG_RECORDING_FFMPEG_TESTING=1)
        target_compile_options(vaeg_recording_ffv1_test PRIVATE
            -Wall -fsigned-char)
        target_link_libraries(vaeg_recording_ffv1_test PRIVATE
            ${VAEG_FFMPEG_TARGETS})
        if(VAEG_WERROR)
            target_compile_options(vaeg_recording_ffv1_test PRIVATE -Werror)
        endif()
        add_test(NAME vaeg_recording_ffv1
            COMMAND vaeg_recording_ffv1_test)
        set_tests_properties(vaeg_recording_ffv1 PROPERTIES
            LABELS "romless;recording;ffmpeg;ffv1")

        add_executable(vaeg_recording_pcm_test
            tests/frontend/recording/test_recording_ffmpeg_pcm_encoder.cpp
            sdl2/recording/recording_ffmpeg_pcm_encoder.cpp
            common/parts.c)
        target_include_directories(vaeg_recording_pcm_test PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/sdl2"
            "${CMAKE_CURRENT_SOURCE_DIR}/common"
            "${CMAKE_CURRENT_SOURCE_DIR}")
        target_compile_features(vaeg_recording_pcm_test PRIVATE cxx_std_17)
        target_compile_definitions(vaeg_recording_pcm_test PRIVATE
            VAEG_ENABLE_RECORDING=1
            VAEG_RECORDING_FFMPEG=1
            VAEG_RECORDING_FFMPEG_TESTING=1)
        target_compile_options(vaeg_recording_pcm_test PRIVATE
            -Wall -fsigned-char)
        target_link_libraries(vaeg_recording_pcm_test PRIVATE
            ${VAEG_FFMPEG_TARGETS})
        if(VAEG_WERROR)
            target_compile_options(vaeg_recording_pcm_test PRIVATE -Werror)
        endif()
        add_test(NAME vaeg_recording_pcm
            COMMAND vaeg_recording_pcm_test)
        set_tests_properties(vaeg_recording_pcm PROPERTIES
            LABELS "romless;recording;ffmpeg;pcm")

        add_executable(vaeg_recording_matroska_test
            tests/frontend/recording/test_recording_ffmpeg_matroska.cpp
            sdl2/recording/recording_ffmpeg_matroska.cpp
            sdl2/recording/recording_ffmpeg_video_encoder.cpp
            sdl2/recording/recording_ffmpeg_pcm_encoder.cpp
            sdl2/recording/recording.c
            common/parts.c)
        target_include_directories(vaeg_recording_matroska_test PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/sdl2"
            "${CMAKE_CURRENT_SOURCE_DIR}/common"
            "${CMAKE_CURRENT_SOURCE_DIR}")
        target_compile_features(vaeg_recording_matroska_test PRIVATE cxx_std_17)
        target_compile_definitions(vaeg_recording_matroska_test PRIVATE
            VAEG_ENABLE_RECORDING=1
            VAEG_RECORDING_FFMPEG=1
            VAEG_RECORDING_FFMPEG_TESTING=1)
        target_compile_options(vaeg_recording_matroska_test PRIVATE
            -Wall -fsigned-char)
        target_link_libraries(vaeg_recording_matroska_test PRIVATE
            ${VAEG_FFMPEG_TARGETS})
        if(VAEG_WERROR)
            target_compile_options(vaeg_recording_matroska_test PRIVATE -Werror)
        endif()
        add_test(NAME vaeg_recording_matroska
            COMMAND vaeg_recording_matroska_test)
        set_tests_properties(vaeg_recording_matroska PROPERTIES
            LABELS "romless;recording;ffmpeg;matroska")
    endif()

    add_test(NAME vaeg_8087_core COMMAND vaeg_8087_tests)
    add_test(NAME vaeg_third_party_licenses
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/frontend/test_third_party_licenses.py")
    add_test(NAME vaeg_repo_check_eol
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/repo/test_check_eol.py")
    add_test(NAME vaeg_repo_check_md_comments
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/repo/test_check_md_comments.py")
    add_test(NAME vaeg_pcengine_disk_msdos
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/pc88va/test_pcengine_disk_msdos.py")
    add_test(NAME vaeg_pcengine_disk_capacity
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/pc88va/test_pcengine_disk_capacity.py")
    add_test(NAME vaeg_extract_vz_editor
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/pc88va/test_extract_vz_editor.py")
    add_test(NAME vaeg_sgptest_consistency
        COMMAND "${Python3_EXECUTABLE}" -I
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/pc88va/test_sgptest.py")
    add_test(NAME vaeg_all_demos_msdos4_disk
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/pc88va/test_all_demos_msdos4_disk.py")
    add_test(NAME vaeg_zex_archive_audit
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/z80_compat/test_check_zex_archive.py")
    add_test(NAME vaeg_cpmva_installer
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/cpmva/tests/test_install_cpmva.py")
    add_test(NAME vaeg_cpmva_zexbuild
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/cpmva/tests/test_zexbuild.py")
    add_test(NAME vaeg_cpmva_probe_results
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/cpmva/tests/test_probe_results.py")
    add_test(NAME vaeg_cpmva_extract_d88
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/cpmva/tests/test_extract_d88.py")
    add_test(NAME vaeg_cpmva_m101_evidence
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/cpmva/tests/test_m101_evidence.py")
    if(NOT CMAKE_CROSSCOMPILING AND NOT MSVC)
        add_test(NAME vaeg_headless_keys
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tests/frontend/test_headless_keys.py")
        set_tests_properties(vaeg_headless_keys PROPERTIES
            ENVIRONMENT "CC=${CMAKE_C_COMPILER};VAEG_TEST_SYSROOT=${CMAKE_OSX_SYSROOT}"
            LABELS "romless;frontend")
    endif()
    if(VAEG_STATIC_LIBRASHADER AND VAEG_ENABLE_LIBRASHADER)
        add_executable(vaeg_static_preset
            tests/frontend/librashader/test_static_preset.cpp
            sdl2/librashader/builtin_shaders.cpp)
        if(MINGW)
            target_sources(vaeg_static_preset PRIVATE sdl2/librashader/optional_dxc.cpp)
        endif()
        target_compile_definitions(vaeg_static_preset PRIVATE VAEG_STATIC_LIBRASHADER=1)
        target_include_directories(vaeg_static_preset PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/sdl2"
            "${CMAKE_CURRENT_SOURCE_DIR}/external/librashader/include"
            "${CMAKE_CURRENT_BINARY_DIR}/generated")
        target_link_libraries(vaeg_static_preset PRIVATE
            "${VAEG_LIBRASHADER_STATIC_LIBRARY}" ${VAEG_LIBRASHADER_NATIVE_LIBS})
        file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/static-preset-test")
        add_test(NAME vaeg_static_preset COMMAND vaeg_static_preset)
        if(NOT CMAKE_CROSSCOMPILING)
            add_test(NAME vaeg_static_cache
                COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/frontend/librashader/test_static_cache.py"
                    "$<TARGET_FILE:vaeg_static_preset>")
        endif()
        set_tests_properties(vaeg_static_preset PROPERTIES
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/static-preset-test")
        if(APPLE)
            add_executable(vaeg_static_metal_loader
                tests/frontend/librashader/test_static_metal_loader.mm)
            target_compile_definitions(vaeg_static_metal_loader PRIVATE
                VAEG_STATIC_LIBRASHADER=1)
            target_include_directories(vaeg_static_metal_loader PRIVATE
                "${CMAKE_CURRENT_SOURCE_DIR}/sdl2"
                "${CMAKE_CURRENT_SOURCE_DIR}/external/librashader/include")
            target_link_libraries(vaeg_static_metal_loader PRIVATE
                "${VAEG_LIBRASHADER_STATIC_LIBRARY}" ${VAEG_LIBRASHADER_NATIVE_LIBS})
            add_test(NAME vaeg_static_metal_loader COMMAND vaeg_static_metal_loader)
        endif()
    endif()
    set(VAEG_SSTS_V20_ROOT "" CACHE PATH
        "Verified external SingleStepTests V20 checkout for uPD9002 comparison")

    if(TARGET hostfat_sys)
        add_test(NAME vaeg_hostfat_driver
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/pc88va/hostfat/check_driver.py"
                --input "${VAEG_HOSTFAT_SYS}")
        set_tests_properties(vaeg_hostfat_driver PROPERTIES
            LABELS "romless;hostfat;pcengine")
    endif()

    if(TARGET r2fprobe_com)
        add_test(NAME vaeg_pcengine_redirector_probe
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/pc88va/hostfs/check_probe.py"
                --input "${VAEG_R2FPROBE_COM}")
        set_tests_properties(vaeg_pcengine_redirector_probe PROPERTIES
            LABELS "romless;hostfs;pcengine")
    endif()

    add_test(NAME vaeg_upd9002_dispatch_graph
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_rep0f_transition.py"
            --root "${CMAKE_CURRENT_SOURCE_DIR}")
    set_tests_properties(vaeg_upd9002_dispatch_graph PROPERTIES
        LABELS "romless;upd9002;dispatch")

    add_test(NAME vaeg_upd9002_dispatch_generator_selftest
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_rep0f_transition.py"
            --root "${CMAKE_CURRENT_SOURCE_DIR}" --selftest)
    set_tests_properties(vaeg_upd9002_dispatch_generator_selftest PROPERTIES
        LABELS "romless;upd9002;dispatch")

    add_test(NAME vaeg_upd9002_native_invariant
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_native_invariant.py"
            --root "${CMAKE_CURRENT_SOURCE_DIR}")
    set_tests_properties(vaeg_upd9002_native_invariant PROPERTIES
        LABELS "romless;upd9002;dispatch;state")

    add_test(NAME vaeg_upd9002_dispatch_normalization_static
		COMMAND "${Python3_EXECUTABLE}"
			"${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_dispatch_normalization.py"
			--root "${CMAKE_CURRENT_SOURCE_DIR}")
    set_tests_properties(vaeg_upd9002_dispatch_normalization_static PROPERTIES
		LABELS "romless;upd9002;dispatch;state")

    add_test(NAME vaeg_upd9002_dispatch_normalization
		COMMAND vaeg_sdl2 --upd9002-m46-dispatch-qa)
    set_tests_properties(vaeg_upd9002_dispatch_normalization PROPERTIES
		LABELS "romless;upd9002;dispatch;state")

    add_test(NAME vaeg_upd9002_rep0f_diagnostic_stop
		COMMAND vaeg_sdl2 --upd9002-m48-rep0f-diagnostic)
    set_tests_properties(vaeg_upd9002_rep0f_diagnostic_stop PROPERTIES
		LABELS "romless;upd9002;dispatch;state")

    add_test(NAME vaeg_upd9002_flags_materialization
		COMMAND vaeg_sdl2 --upd9002-m60a-flags-materialization)
    set_tests_properties(vaeg_upd9002_flags_materialization PROPERTIES
		LABELS "romless;upd9002;flags;semantics")

    add_test(NAME vaeg_upd9002_iret_restoration
		COMMAND vaeg_sdl2 --upd9002-m60e-iret-restoration)
    set_tests_properties(vaeg_upd9002_iret_restoration PROPERTIES
		LABELS "romless;upd9002;flags;iret;semantics")

    add_test(NAME vaeg_upd9002_brkem_upd70008
        COMMAND vaeg_sdl2 --upd9002-brkem-upd70008)
    set_tests_properties(vaeg_upd9002_brkem_upd70008 PROPERTIES
        LABELS "romless;upd9002;upd70008;compatibility;transitions")

    add_test(NAME vaeg_upd9002_mov_imm_register
		COMMAND vaeg_sdl2 --upd9002-m61-mov-imm-register)
    set_tests_properties(vaeg_upd9002_mov_imm_register PROPERTIES
		LABELS "romless;upd9002;mov;semantics")

    add_test(NAME vaeg_upd9002_semantics_bundle
		COMMAND vaeg_sdl2 --upd9002-m62-semantics-bundle)
    set_tests_properties(vaeg_upd9002_semantics_bundle PROPERTIES
		LABELS "romless;upd9002;m62;semantics")

    add_test(NAME vaeg_upd9002_m64_semantics
		COMMAND vaeg_sdl2 --upd9002-m64-semantics)
    set_tests_properties(vaeg_upd9002_m64_semantics PROPERTIES
		LABELS "romless;upd9002;m64;semantics")

    add_test(NAME vaeg_upd9002_m65a_ff7
		COMMAND vaeg_sdl2 --upd9002-m65a-ff7)
    set_tests_properties(vaeg_upd9002_m65a_ff7 PROPERTIES
		LABELS "romless;upd9002;m65a;semantics")

    add_test(NAME vaeg_upd9002_m65b_bound
		COMMAND vaeg_sdl2 --upd9002-m65b-bound)
    set_tests_properties(vaeg_upd9002_m65b_bound PROPERTIES
		LABELS "romless;upd9002;m65b;semantics")

    add_test(NAME vaeg_upd9002_m65c_f72
		COMMAND vaeg_sdl2 --upd9002-m65c-f72)
    set_tests_properties(vaeg_upd9002_m65c_f72 PROPERTIES
		LABELS "romless;upd9002;m65c;semantics")

    add_test(NAME vaeg_upd9002_m65d_ff6
		COMMAND vaeg_sdl2 --upd9002-m65d-ff6)
    set_tests_properties(vaeg_upd9002_m65d_ff6 PROPERTIES
		LABELS "romless;upd9002;m65d;semantics")
    add_test(NAME vaeg_upd9002_m65e_tail10
		COMMAND vaeg_sdl2 --upd9002-m65e-tail10)
    set_tests_properties(vaeg_upd9002_m65e_tail10 PROPERTIES
		LABELS "romless;upd9002;m65e;semantics")

    add_test(NAME vaeg_upd9002_m96d_nop
		COMMAND vaeg_sdl2 --upd9002-m96d-nop)
    set_tests_properties(vaeg_upd9002_m96d_nop PROPERTIES
		LABELS "romless;upd9002;m96d;semantics")

    add_test(NAME vaeg_upd9002_m106_near_jcc
		COMMAND vaeg_sdl2 --upd9002-m106-near-jcc)
    set_tests_properties(vaeg_upd9002_m106_near_jcc PROPERTIES
		LABELS "romless;upd9002;m106;semantics")

    add_test(NAME vaeg_upd9002_m68_segmented_memory
		COMMAND vaeg_sdl2 --upd9002-m68-segmented-memory)
    set_tests_properties(vaeg_upd9002_m68_segmented_memory PROPERTIES
		LABELS "romless;upd9002;m68;memory;semantics")

    add_test(NAME vaeg_idp_m69_status_composition
		COMMAND vaeg_sdl2 --idp-m69-status-composition)
    set_tests_properties(vaeg_idp_m69_status_composition PROPERTIES
		LABELS "romless;idp;tsp;m69")

    add_test(NAME vaeg_m75_scsi_controller
		COMMAND "${Python3_EXECUTABLE}"
			"${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/m75_scsi_controller.py"
			--root "${CMAKE_CURRENT_SOURCE_DIR}")
    set_tests_properties(vaeg_m75_scsi_controller PROPERTIES
		LABELS "romless;scsi;m75")

    add_test(NAME vaeg_m75_transfer_info
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/m75_transfer_info.py"
            --selftest)
    set_tests_properties(vaeg_m75_transfer_info PROPERTIES
        LABELS "romless;scsi;m75;transfer-info")

    add_test(NAME vaeg_m75_fat_inspection
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/test_inspect_vaeg_fat.py"
            -v)
    set_tests_properties(vaeg_m75_fat_inspection PROPERTIES
        LABELS "romless;scsi;m75;fat")

    add_test(NAME vaeg_m75_storage_regression_fixtures
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/m75_storage_regression.py"
            --selftest)
    set_tests_properties(vaeg_m75_storage_regression_fixtures PROPERTIES
        LABELS "romless;sasi;hostfat;m75;storage")

    add_test(NAME vaeg_upd9002_m70_prefix_string
		COMMAND vaeg_sdl2 --upd9002-m70-prefix-string)
    set_tests_properties(vaeg_upd9002_m70_prefix_string PROPERTIES
		LABELS "romless;upd9002;m70;semantics")

    add_test(NAME vaeg_upd9002_rep0f_transition_selftest
		COMMAND "${Python3_EXECUTABLE}"
			"${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_rep0f_transition.py"
			--root "${CMAKE_CURRENT_SOURCE_DIR}" --selftest)
    set_tests_properties(vaeg_upd9002_rep0f_transition_selftest PROPERTIES
		LABELS "romless;upd9002;dispatch;state;ssts")

    add_test(NAME vaeg_upd9002_rep0f_transition_static
		COMMAND "${Python3_EXECUTABLE}"
			"${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_rep0f_transition.py"
			--root "${CMAKE_CURRENT_SOURCE_DIR}")
    set_tests_properties(vaeg_upd9002_rep0f_transition_static PROPERTIES
		LABELS "romless;upd9002;dispatch;state;ssts")

    add_test(NAME vaeg_upd9002_protected_deletion
		COMMAND "${Python3_EXECUTABLE}"
			"${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_protected_deletion.py"
			--root "${CMAKE_CURRENT_SOURCE_DIR}")
    set_tests_properties(vaeg_upd9002_protected_deletion PROPERTIES
		LABELS "romless;upd9002;dispatch;state")

    add_test(NAME vaeg_upd9002_protected_deletion_selftest
		COMMAND "${Python3_EXECUTABLE}"
			"${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_protected_deletion.py"
			--root "${CMAKE_CURRENT_SOURCE_DIR}" --selftest)
    set_tests_properties(
		vaeg_upd9002_protected_deletion_selftest PROPERTIES
		LABELS "romless;upd9002;dispatch;state")

    add_test(NAME vaeg_upd9002_rename
		COMMAND "${Python3_EXECUTABLE}"
			"${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_rename.py"
			--root "${CMAKE_CURRENT_SOURCE_DIR}")
    set_tests_properties(vaeg_upd9002_rename PROPERTIES
		LABELS "romless;upd9002;repository")

    add_executable(vaeg_upd9002_abi tests/upd9002/abi.c)
    target_include_directories(vaeg_upd9002_abi PRIVATE ${VAEG_INCLUDE_DIRS})
    target_compile_definitions(vaeg_upd9002_abi PRIVATE
        SUPPORT_V30ORIGINAL)
    target_compile_options(vaeg_upd9002_abi PRIVATE -Wall -fsigned-char)
    add_test(NAME vaeg_upd9002_abi
        COMMAND vaeg_upd9002_abi
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/upd9002/abi_g41.txt")
    set_tests_properties(vaeg_upd9002_abi PROPERTIES
        LABELS "romless;upd9002;state")

    add_executable(vaeg_upd9002_state_boundary
        tests/upd9002/state_boundary.c
        cpu/upd9002/upd9002_state.c)
    target_include_directories(vaeg_upd9002_state_boundary PRIVATE
        ${VAEG_INCLUDE_DIRS})
    target_compile_definitions(vaeg_upd9002_state_boundary PRIVATE
        SUPPORT_V30ORIGINAL)
    target_compile_options(vaeg_upd9002_state_boundary PRIVATE
        -Wall -fsigned-char)
    add_test(NAME vaeg_upd9002_state_boundary
        COMMAND vaeg_upd9002_state_boundary)
    set_tests_properties(vaeg_upd9002_state_boundary PROPERTIES
        LABELS "romless;upd9002;state")

    add_executable(vaeg_upd9002_state_payload_probe
        tests/upd9002/state_payload_probe.c
        cpu/upd9002/upd9002_state.c)
    target_include_directories(vaeg_upd9002_state_payload_probe PRIVATE
        ${VAEG_INCLUDE_DIRS})
    target_compile_definitions(vaeg_upd9002_state_payload_probe PRIVATE
        SUPPORT_V30ORIGINAL)
    target_compile_options(vaeg_upd9002_state_payload_probe PRIVATE
        -Wall -fsigned-char)
    add_test(NAME vaeg_upd9002_state_payload_probe
        COMMAND vaeg_upd9002_state_payload_probe
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/upd9002/state_fixtures_m42.txt")
    set_tests_properties(vaeg_upd9002_state_payload_probe PROPERTIES
        LABELS "romless;upd9002;state")

    add_test(NAME vaeg_upd9002_m66a_state_verify
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_m66_state.py"
            --root "${CMAKE_CURRENT_SOURCE_DIR}" verify-m66a)
    set_tests_properties(vaeg_upd9002_m66a_state_verify PROPERTIES
        LABELS "romless;upd9002;state")

    add_test(NAME vaeg_upd9002_trace_equivalence
        COMMAND "${CMAKE_COMMAND}"
            "-DVAEG_EXECUTABLE=$<TARGET_FILE:vaeg_sdl2>"
            "-DTRACE_GOLDEN=${CMAKE_CURRENT_SOURCE_DIR}/tests/upd9002/trace_baseline.txt"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/upd9002/run_trace_equivalence.cmake")
    set_tests_properties(vaeg_upd9002_trace_equivalence PROPERTIES
        # This invokes the complete selftest three times.  The M55 64 MiB
        # snapshot identity checks are intentionally expensive under ASan.
        LABELS "romless;upd9002;trace;harness" TIMEOUT 420)

    if(VAEG_Z80_COMPAT_INTEGRATION_TRACE)
        add_test(NAME vaeg_m74_debug_harness
            COMMAND "${CMAKE_COMMAND}"
                "-DVAEG_EXECUTABLE=$<TARGET_FILE:vaeg_sdl2>"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/m74/run_debug_harness.cmake")
        set_tests_properties(vaeg_m74_debug_harness PROPERTIES
            LABELS "romless;upd9002;trace;harness;m74" TIMEOUT 60)
    endif()

    add_test(NAME vaeg_upd9002_ssts_selftest
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_ssts.py"
            selftest)
    set_tests_properties(vaeg_upd9002_ssts_selftest PROPERTIES
        LABELS "romless;upd9002;ssts")

    add_test(NAME vaeg_milestone_id_selftest
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/milestone_ids.py"
            --root "${CMAKE_CURRENT_SOURCE_DIR}"
            --selftest --discover --audit)
    set_tests_properties(vaeg_milestone_id_selftest PROPERTIES
        LABELS "romless;repository;milestone")

    add_test(NAME vaeg_upd9002_ssts_worker_selftest
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/upd9002_ssts.py"
            worker-selftest
            --worker "$<TARGET_FILE:vaeg_sdl2>")
    set_tests_properties(vaeg_upd9002_ssts_worker_selftest PROPERTIES
        LABELS "romless;upd9002;ssts;harness" TIMEOUT 60)

    # uPD9002 native-mode check against the SingleStepTests V20 corpus:
    # regressions and stale entries against tests/ssts/v20/baseline_ci.json.
    add_test(NAME vaeg_sst_v20_selftest
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/sst_v20.py" selftest)
    add_test(NAME vaeg_sst_v20_baseline
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/sst_v20.py" validate
            --baseline "${CMAKE_CURRENT_SOURCE_DIR}/tests/ssts/v20/baseline_ci.json")
    set_tests_properties(vaeg_sst_v20_selftest vaeg_sst_v20_baseline PROPERTIES
        LABELS "romless;upd9002;ssts;sst-v20")
    if(VAEG_SSTS_V20_ROOT)
        add_test(NAME vaeg_sst_v20_ci
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/sst_v20.py" ci
                --dataset-root "${VAEG_SSTS_V20_ROOT}"
                --manifest "${CMAKE_CURRENT_SOURCE_DIR}/tests/ssts/v20_dataset_manifest.json"
                --worker "$<TARGET_FILE:vaeg_sdl2>"
                --baseline "${CMAKE_CURRENT_SOURCE_DIR}/tests/ssts/v20/baseline_ci.json"
                --output "${CMAKE_CURRENT_BINARY_DIR}/sst-v20/ci_result.json")
        set_tests_properties(vaeg_sst_v20_ci PROPERTIES
            LABELS "romless;upd9002;ssts;sst-v20;external"
            TIMEOUT 1800)
    else()
        add_test(NAME vaeg_sst_v20_ci
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/sst_v20.py" skip
                --reason "VAEG_SSTS_V20_ROOT is not configured")
        set_tests_properties(vaeg_sst_v20_ci PROPERTIES
            LABELS "romless;upd9002;ssts;sst-v20;external"
            SKIP_RETURN_CODE 77)
    endif()

    function(vaeg_configure_z80_compat_standalone_target target)
        target_include_directories(${target} PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/external/suzukiplan-z80")
        target_compile_options(${target} PRIVATE -Wall -fsigned-char)
        if(VAEG_WERROR)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endfunction()

    set(VAEG_Z80_COMPAT_HEADER_COMPILE_SOURCE tests/z80_compat/header_compile.cpp)
    add_executable(vaeg_z80_compat_header_default
        ${VAEG_Z80_COMPAT_HEADER_COMPILE_SOURCE})
    vaeg_configure_z80_compat_standalone_target(vaeg_z80_compat_header_default)

    add_executable(vaeg_z80_compat_header_no_functional
        ${VAEG_Z80_COMPAT_HEADER_COMPILE_SOURCE})
    vaeg_configure_z80_compat_standalone_target(vaeg_z80_compat_header_no_functional)
    target_compile_definitions(vaeg_z80_compat_header_no_functional PRIVATE
        Z80_NO_FUNCTIONAL)

    add_executable(vaeg_z80_compat_header_debug_default
        ${VAEG_Z80_COMPAT_HEADER_COMPILE_SOURCE})
    vaeg_configure_z80_compat_standalone_target(vaeg_z80_compat_header_debug_default)
    target_compile_definitions(vaeg_z80_compat_header_debug_default PRIVATE
        VAEG_Z80_COMPAT_REQUIRE_DEBUG)

    add_executable(vaeg_z80_compat_header_release
        ${VAEG_Z80_COMPAT_HEADER_COMPILE_SOURCE})
    vaeg_configure_z80_compat_standalone_target(vaeg_z80_compat_header_release)
    target_compile_definitions(vaeg_z80_compat_header_release PRIVATE
        VAEG_Z80_COMPAT_REQUIRE_RELEASE
        Z80_DISABLE_DEBUG
        Z80_DISABLE_BREAKPOINT
        Z80_DISABLE_NESTCHECK
        Z80_NO_EXCEPTION)

    add_executable(vaeg_z80_compat_interrupt_default
        external/suzukiplan-z80/test/test-interrupt-extension.cpp)
    vaeg_configure_z80_compat_standalone_target(vaeg_z80_compat_interrupt_default)

    add_executable(vaeg_z80_compat_interrupt_no_functional
        external/suzukiplan-z80/test/test-interrupt-extension.cpp)
    vaeg_configure_z80_compat_standalone_target(
        vaeg_z80_compat_interrupt_no_functional)
    target_compile_definitions(vaeg_z80_compat_interrupt_no_functional PRIVATE
        Z80_NO_FUNCTIONAL)

    function(vaeg_add_z80_compat_wrapper_library target)
        add_library(${target} STATIC
            cpu/z80_compat_cpu.cpp
            cpu/z80_compat_state.cpp)
        target_include_directories(${target}
            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
            PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/external/suzukiplan-z80")
        target_compile_features(${target} PUBLIC cxx_std_17)
        target_compile_options(${target} PRIVATE -Wall -fsigned-char)
        target_compile_definitions(${target} PRIVATE
            Z80_DISABLE_BREAKPOINT
            Z80_DISABLE_DEBUG
            Z80_DISABLE_NESTCHECK
            Z80_NO_EXCEPTION
            Z80_UNSUPPORT_16BIT_PORT)
        if(VAEG_WERROR)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endfunction()

    vaeg_add_z80_compat_wrapper_library(vaeg_z80_compat_wrapper)
    vaeg_add_z80_compat_wrapper_library(vaeg_z80_compat_wrapper_no_functional)
    target_compile_definitions(vaeg_z80_compat_wrapper_no_functional PRIVATE
        Z80_NO_FUNCTIONAL)

    add_executable(vaeg_z80_compat_wrapper_default tests/z80_compat/wrapper.cpp)
    target_link_libraries(vaeg_z80_compat_wrapper_default PRIVATE vaeg_z80_compat_wrapper)
    target_compile_options(vaeg_z80_compat_wrapper_default PRIVATE
        -Wall -fsigned-char)

    add_executable(vaeg_z80_compat_wrapper_no_functional_test
        tests/z80_compat/wrapper.cpp)
    target_link_libraries(vaeg_z80_compat_wrapper_no_functional_test PRIVATE
        vaeg_z80_compat_wrapper_no_functional)
    target_compile_options(vaeg_z80_compat_wrapper_no_functional_test PRIVATE
        -Wall -fsigned-char)

    # Role ZEX runner: the shared backend with the production compile
    # definitions and each role's production core configuration.
    add_executable(vaeg_z80_compat_role_zex_runner
        tests/z80_compat/wrapper_zex_runner.cpp
        cpu/upd780/upd780_core_config.cpp
        cpu/upd9002_upd70008_core_config.cpp)
    target_link_libraries(vaeg_z80_compat_role_zex_runner PRIVATE
        vaeg_z80_compat_wrapper)
    target_compile_options(vaeg_z80_compat_role_zex_runner PRIVATE
        -Wall -fsigned-char)

    add_executable(vaeg_upd780_disasm
        cpu/upd780/upd780_disasm.cpp
        tests/upd780/disasm.cpp)
    target_include_directories(vaeg_upd780_disasm PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}")
    target_compile_features(vaeg_upd780_disasm PRIVATE cxx_std_17)
    target_compile_definitions(vaeg_upd780_disasm PRIVATE
        "VAEG_UPD780_DISASM_GOLDEN_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/tests/upd780/disasm_golden.txt\"")
    target_compile_options(vaeg_upd780_disasm PRIVATE -Wall -fsigned-char)
    if(VAEG_WERROR)
        target_compile_options(vaeg_upd780_disasm PRIVATE -Werror)
    endif()

    if(UNIX AND NOT APPLE AND NOT WIN32 AND VAEG_ENABLE_LIBRASHADER)
        add_executable(vaeg_gl_displayed_test
            tests/frontend/librashader/test_gl_displayed.cpp
            sdl2/librashader/backends/gl_bridge.cpp
            sdl2/librashader/frame_input.cpp
            sdl2/librashader/frame_conversion.cpp
            external/imgui/imgui.cpp
            external/imgui/imgui_draw.cpp
            external/imgui/imgui_tables.cpp
            external/imgui/imgui_widgets.cpp
            external/imgui/backends/imgui_impl_opengl3.cpp)
        target_include_directories(vaeg_gl_displayed_test PRIVATE
            sdl2 external/imgui external/librashader/include)
        target_compile_features(vaeg_gl_displayed_test PRIVATE cxx_std_17)
        target_link_libraries(vaeg_gl_displayed_test PRIVATE ${VAEG_SDL2_LIBS} ${CMAKE_DL_LIBS})
        add_test(NAME vaeg_gl_displayed COMMAND vaeg_gl_displayed_test)
        set_tests_properties(vaeg_gl_displayed PROPERTIES
            LABELS "romless;recording;displayed;gpu" DISABLED TRUE)
        # Opt-in: needs a real GL context, e.g. xvfb-run ./vaeg_gl_displayed_test.
    endif()

    add_executable(vaeg_librashader_frame_input_test
        sdl2/librashader/frame_input.cpp
        tests/frontend/librashader/test_frame_input.cpp)
    target_include_directories(vaeg_librashader_frame_input_test PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/sdl2")
    target_compile_features(vaeg_librashader_frame_input_test PRIVATE cxx_std_17)
    target_compile_options(vaeg_librashader_frame_input_test PRIVATE -Wall -fsigned-char)
    if(VAEG_WERROR)
        target_compile_options(vaeg_librashader_frame_input_test PRIVATE -Werror)
    endif()

    add_executable(vaeg_librashader_capture_boundary_test
        sdl2/librashader/frame_input.cpp
        tests/frontend/librashader/test_capture_boundary.cpp)
    target_include_directories(vaeg_librashader_capture_boundary_test PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/sdl2")
    target_compile_features(vaeg_librashader_capture_boundary_test PRIVATE cxx_std_17)
    target_compile_options(vaeg_librashader_capture_boundary_test PRIVATE -Wall -fsigned-char)
    if(VAEG_WERROR)
        target_compile_options(vaeg_librashader_capture_boundary_test PRIVATE -Werror)
    endif()

    add_executable(vaeg_librashader_presenter_state_test
        sdl2/librashader/native_presenter.cpp
        sdl2/librashader/presenter_factory.cpp
        sdl2/librashader/shader_parameters.cpp
        sdl2/librashader/shader_preset.cpp
        tests/frontend/librashader/test_presenter_state.cpp)
    target_include_directories(vaeg_librashader_presenter_state_test PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/sdl2"
        "${CMAKE_CURRENT_SOURCE_DIR}/external/librashader/include")
    target_compile_features(vaeg_librashader_presenter_state_test PRIVATE cxx_std_17)
    target_compile_options(vaeg_librashader_presenter_state_test PRIVATE -Wall -fsigned-char)
    if(VAEG_WERROR)
        target_compile_options(vaeg_librashader_presenter_state_test PRIVATE -Werror)
    endif()

    add_executable(vaeg_librashader_pass_through_test
        sdl2/librashader/frame_input.cpp
        sdl2/librashader/frame_conversion.cpp
        sdl2/librashader/native_presenter.cpp
        sdl2/librashader/presenter_factory.cpp
        sdl2/librashader/shader_parameters.cpp
        sdl2/librashader/shader_preset.cpp
        sdl2/viewport.c
        tests/frontend/librashader/test_pass_through.cpp)
    target_include_directories(vaeg_librashader_pass_through_test PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/sdl2"
        "${CMAKE_CURRENT_SOURCE_DIR}/common"
        "${CMAKE_CURRENT_SOURCE_DIR}/external/librashader/include")
    target_compile_features(vaeg_librashader_pass_through_test PRIVATE cxx_std_17)
    target_compile_options(vaeg_librashader_pass_through_test PRIVATE -Wall -fsigned-char)
    if(VAEG_WERROR)
        target_compile_options(vaeg_librashader_pass_through_test PRIVATE -Werror)
    endif()

    add_executable(vaeg_librashader_frame_padding_test
        sdl2/librashader/frame_input.cpp
        sdl2/librashader/frame_padding.cpp
        tests/frontend/librashader/test_frame_padding.cpp)
    target_include_directories(vaeg_librashader_frame_padding_test PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/sdl2")
    target_compile_features(vaeg_librashader_frame_padding_test PRIVATE cxx_std_17)
    add_test(NAME vaeg_librashader_frame_padding COMMAND vaeg_librashader_frame_padding_test)
    set_tests_properties(vaeg_librashader_frame_padding PROPERTIES
        LABELS "romless;frontend;librashader" TIMEOUT 30)

    add_executable(vaeg_librashader_scanline_aa_test
        tests/frontend/librashader/test_scanline_aa.cpp)
    target_include_directories(vaeg_librashader_scanline_aa_test PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/crt/shaders")
    target_compile_features(vaeg_librashader_scanline_aa_test PRIVATE cxx_std_17)
    add_test(NAME vaeg_librashader_scanline_aa COMMAND vaeg_librashader_scanline_aa_test)
    set_tests_properties(vaeg_librashader_scanline_aa PROPERTIES
        LABELS "romless;frontend;librashader" TIMEOUT 30)

    add_executable(vaeg_librashader_shader_parameters_test
        sdl2/librashader/shader_parameters.cpp
        tests/frontend/librashader/test_shader_parameters.cpp)
    target_include_directories(vaeg_librashader_shader_parameters_test PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/sdl2")
    target_compile_features(vaeg_librashader_shader_parameters_test PRIVATE cxx_std_17)
    target_compile_options(vaeg_librashader_shader_parameters_test PRIVATE -Wall -fsigned-char)
    if(VAEG_WERROR)
        target_compile_options(vaeg_librashader_shader_parameters_test PRIVATE -Werror)
    endif()

    if(WIN32)
        # Explicit optional smoke command: requires the pinned runtime DLL and WARP.
        # Not an unconditional CTest, since runtime-absent builds remain supported.
        add_executable(vaeg_librashader_d3d11_filter_test
            tests/frontend/librashader/test_d3d11_filter.cpp)
        target_include_directories(vaeg_librashader_d3d11_filter_test PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/sdl2"
            "${CMAKE_CURRENT_SOURCE_DIR}/external/librashader/include")
        target_compile_features(vaeg_librashader_d3d11_filter_test PRIVATE cxx_std_17)
        target_link_libraries(vaeg_librashader_d3d11_filter_test PRIVATE d3d11)
        add_executable(vaeg_librashader_d3d11_raster_test
            tests/frontend/librashader/test_d3d11_raster.cpp)
        target_include_directories(vaeg_librashader_d3d11_raster_test PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/sdl2")
        target_compile_features(vaeg_librashader_d3d11_raster_test PRIVATE cxx_std_17)
        target_link_libraries(vaeg_librashader_d3d11_raster_test PRIVATE d3d11 d3dcompiler)
        add_test(NAME vaeg_librashader_d3d11_raster COMMAND vaeg_librashader_d3d11_raster_test)
        set_tests_properties(vaeg_librashader_d3d11_raster PROPERTIES
            LABELS "romless;frontend;librashader" TIMEOUT 30)
    endif()

    add_executable(vaeg_librashader_controller_test
        sdl2/librashader/frame_input.cpp
        sdl2/librashader/frame_padding.cpp
        sdl2/librashader/native_presenter_controller.cpp
        sdl2/librashader/native_presenter.cpp
        sdl2/librashader/shader_parameters.cpp
        sdl2/librashader/shader_preset.cpp
        tests/frontend/librashader/test_controller.cpp)
    target_include_directories(vaeg_librashader_controller_test PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/sdl2"
        "${CMAKE_CURRENT_SOURCE_DIR}/external/librashader/include")
    target_compile_features(vaeg_librashader_controller_test PRIVATE cxx_std_17)
    target_link_libraries(vaeg_librashader_controller_test PRIVATE ${CMAKE_DL_LIBS})
    add_test(NAME vaeg_librashader_controller COMMAND vaeg_librashader_controller_test)
    set_tests_properties(vaeg_librashader_controller PROPERTIES
        LABELS "romless;frontend;librashader" TIMEOUT 30)

    add_executable(vaeg_librashader_d3d11_lifecycle_test
        sdl2/librashader/d3d11_presenter.cpp
        sdl2/librashader/native_presenter.cpp
        sdl2/librashader/shader_parameters.cpp
        sdl2/librashader/shader_preset.cpp
        tests/frontend/librashader/test_d3d11_lifecycle.cpp)
    target_include_directories(vaeg_librashader_d3d11_lifecycle_test PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/sdl2"
        "${CMAKE_CURRENT_SOURCE_DIR}/external/librashader/include")
    target_compile_features(vaeg_librashader_d3d11_lifecycle_test PRIVATE cxx_std_17)
    target_link_libraries(vaeg_librashader_d3d11_lifecycle_test PRIVATE ${CMAKE_DL_LIBS})
    add_test(NAME vaeg_librashader_d3d11_lifecycle COMMAND vaeg_librashader_d3d11_lifecycle_test)
    set_tests_properties(vaeg_librashader_d3d11_lifecycle PROPERTIES
        LABELS "romless;frontend;librashader" TIMEOUT 30)

    add_executable(vaeg_librashader_fallback_test
        sdl2/librashader/frame_padding.cpp
        sdl2/librashader/frame_input.cpp
        sdl2/librashader/native_presenter.cpp
        sdl2/librashader/native_presenter_controller.cpp
        sdl2/librashader/presenter_factory.cpp
        sdl2/librashader/shader_parameters.cpp
        sdl2/librashader/shader_preset.cpp
        tests/frontend/librashader/test_fallback.cpp)
    target_include_directories(vaeg_librashader_fallback_test PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/sdl2"
        "${CMAKE_CURRENT_SOURCE_DIR}/external/librashader/include")
    target_compile_features(vaeg_librashader_fallback_test PRIVATE cxx_std_17)
    target_compile_options(vaeg_librashader_fallback_test PRIVATE -Wall -fsigned-char)
    if(VAEG_WERROR)
        target_compile_options(vaeg_librashader_fallback_test PRIVATE -Werror)
    endif()

    add_executable(vaeg_z80_compat_zex_runner tests/z80_compat/zex_runner.cpp)
    vaeg_configure_z80_compat_standalone_target(vaeg_z80_compat_zex_runner)
    target_compile_definitions(vaeg_z80_compat_zex_runner PRIVATE
        Z80_CALLBACK_PER_INSTRUCTION
        Z80_DISABLE_DEBUG
        Z80_DISABLE_NESTCHECK
        Z80_NO_FUNCTIONAL
        Z80_UNSUPPORT_16BIT_PORT)

    add_executable(vaeg_z80_compat_flag_profile tests/z80_compat/flag_profile.cpp)
    vaeg_configure_z80_compat_standalone_target(vaeg_z80_compat_flag_profile)
    target_compile_definitions(vaeg_z80_compat_flag_profile PRIVATE
        Z80_DISABLE_DEBUG
        Z80_DISABLE_NESTCHECK
        Z80_NO_EXCEPTION
        Z80_NO_FUNCTIONAL
        "VAEG_M101_QA_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/docs/agents/reports/m101_zexall_qa/pc88va2/raw\""
        "VAEG_M102_QA_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/docs/agents/reports/m102_zexund_qa/run2_pc88va2/raw\"")

    # The shared backend with the production compile definitions and each
    # role's production core configuration, so that probe programs behave
    # as in vaeg.
    add_executable(vaeg_cpm_runner
        tests/z80_compat/cpm_runner.cpp
        cpu/upd780/upd780_core_config.cpp
        cpu/upd9002_upd70008_core_config.cpp)
    target_link_libraries(vaeg_cpm_runner PRIVATE vaeg_z80_compat_wrapper)
    target_compile_options(vaeg_cpm_runner PRIVATE -Wall -fsigned-char)
    if(VAEG_WERROR)
        target_compile_options(vaeg_cpm_runner PRIVATE -Werror)
    endif()

    # M140: freeze the bounded VA2 NEC/A-only flags contract, not Zilog Q.
    add_executable(vaeg_upd780_va2_flags
        tests/upd780/va2_flags.cpp
        cpu/upd780/upd780_core_config.cpp)
    target_link_libraries(vaeg_upd780_va2_flags PRIVATE vaeg_z80_compat_wrapper)
    target_compile_features(vaeg_upd780_va2_flags PRIVATE cxx_std_17)
    target_compile_options(vaeg_upd780_va2_flags PRIVATE -Wall -fsigned-char)
    if(VAEG_WERROR)
        target_compile_options(vaeg_upd780_va2_flags PRIVATE -Werror)
    endif()
    add_test(NAME vaeg_upd780_va2_flags COMMAND vaeg_upd780_va2_flags)
    set_tests_properties(vaeg_upd780_va2_flags PROPERTIES
        LABELS "romless;compatibility;upd780;host-only" TIMEOUT 60)

    # M140: mapped FDC RAM payload checks with a synthetic monitor transport.
    # This does not execute a private ROM or establish physical CPU timing.
    add_executable(vaeg_upd780_fdc_probe_runner
        tests/upd780/fdc_probe_runner.cpp
        cpu/upd780/upd780_core_config.cpp)
    target_link_libraries(vaeg_upd780_fdc_probe_runner PRIVATE vaeg_z80_compat_wrapper)
    target_compile_features(vaeg_upd780_fdc_probe_runner PRIVATE cxx_std_17)
    target_compile_options(vaeg_upd780_fdc_probe_runner PRIVATE -Wall -fsigned-char)
    if(VAEG_WERROR)
        target_compile_options(vaeg_upd780_fdc_probe_runner PRIVATE -Werror)
    endif()
    add_test(NAME vaeg_upd780_fdc_tools_selftest
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/hardware/upd780/test_fdc_tools.py")
    set_tests_properties(vaeg_upd780_fdc_tools_selftest PROPERTIES
        LABELS "romless;compatibility;upd780;host-only")

    add_test(NAME vaeg_upd780_fdc_seed_selftest
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/hardware/upd780/test_seed.py")
    set_tests_properties(vaeg_upd780_fdc_seed_selftest PROPERTIES
        LABELS "romless;compatibility;upd780;host-only")

    add_test(NAME vaeg_upd780_fdc_fixed_selftest
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/hardware/upd780/test_fixed.py")
    set_tests_properties(vaeg_upd780_fdc_fixed_selftest PROPERTIES
        LABELS "romless;compatibility;upd780;host-only")

    add_test(NAME vaeg_upd780_fdc_capture_selftest
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/hardware/upd780/test_capture.py")
    set_tests_properties(vaeg_upd780_fdc_capture_selftest PROPERTIES
        LABELS "romless;compatibility;upd780;host-only")

    add_test(NAME vaeg_upd780_fdc_context_selftest
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/hardware/upd780/test_context.py")
    set_tests_properties(vaeg_upd780_fdc_context_selftest PROPERTIES
        LABELS "romless;compatibility;upd780;host-only")

    add_test(NAME vaeg_upd780_fdc_xadr_selftest
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/hardware/upd780/test_xadr.py")
    set_tests_properties(vaeg_upd780_fdc_xadr_selftest PROPERTIES
        LABELS "romless;compatibility;upd780;host-only")

    add_test(NAME vaeg_upd780_fdc_pref_selftest
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/hardware/upd780/test_pref.py")
    set_tests_properties(vaeg_upd780_fdc_pref_selftest PROPERTIES
        LABELS "romless;compatibility;upd780;host-only")

    # M101/M102 probe programs per role: built with z80asm 1.8 and run by
    # vaeg_cpm_runner with each role's production core configuration.
    find_program(VAEG_Z80ASM_EXECUTABLE z80asm)
    if(VAEG_Z80ASM_EXECUTABLE)
        add_test(NAME vaeg_upd780_fdc_payloads
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/hardware/upd780/test_fdc_tools.py"
                --runner "$<TARGET_FILE:vaeg_upd780_fdc_probe_runner>"
                --assembler "${VAEG_Z80ASM_EXECUTABLE}")
        add_test(NAME vaeg_upd780_fdc_seed_payloads
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/hardware/upd780/test_seed.py"
                --runner "$<TARGET_FILE:vaeg_upd780_fdc_probe_runner>"
                --assembler "${VAEG_Z80ASM_EXECUTABLE}")
        add_test(NAME vaeg_upd780_fdc_fixed_payloads
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/hardware/upd780/test_fixed.py"
                --runner "$<TARGET_FILE:vaeg_upd780_fdc_probe_runner>"
                --assembler "${VAEG_Z80ASM_EXECUTABLE}")
        add_test(NAME vaeg_upd780_fdc_capture_payloads
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/hardware/upd780/test_capture.py"
                --runner "$<TARGET_FILE:vaeg_upd780_fdc_probe_runner>"
                --assembler "${VAEG_Z80ASM_EXECUTABLE}")
        add_test(NAME vaeg_upd780_fdc_xadr_payloads
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/hardware/upd780/test_xadr.py"
                --runner "$<TARGET_FILE:vaeg_upd780_fdc_probe_runner>"
                --assembler "${VAEG_Z80ASM_EXECUTABLE}")
        add_test(NAME vaeg_upd780_fdc_pref_payloads
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/hardware/upd780/test_pref.py"
                --runner "$<TARGET_FILE:vaeg_upd780_fdc_probe_runner>"
                --assembler "${VAEG_Z80ASM_EXECUTABLE}")
    else()
        add_test(NAME vaeg_upd780_fdc_payloads
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/z80_role_probes.py" skip
                --reason "z80asm 1.8 unavailable; FDC payload tests NOT_RUN")
        add_test(NAME vaeg_upd780_fdc_seed_payloads
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/z80_role_probes.py" skip
                --reason "z80asm 1.8 unavailable; seed payload tests NOT_RUN")
        add_test(NAME vaeg_upd780_fdc_fixed_payloads
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/z80_role_probes.py" skip
                --reason "z80asm 1.8 unavailable; fixed-address payload tests NOT_RUN")
        add_test(NAME vaeg_upd780_fdc_capture_payloads
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/z80_role_probes.py" skip
                --reason "z80asm 1.8 unavailable; capture/storage payload tests NOT_RUN")
        add_test(NAME vaeg_upd780_fdc_xadr_payloads
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/z80_role_probes.py" skip
                --reason "z80asm 1.8 unavailable; crossed-address payload tests NOT_RUN")
        add_test(NAME vaeg_upd780_fdc_pref_payloads
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/z80_role_probes.py" skip
                --reason "z80asm 1.8 unavailable; prefix/history payload tests NOT_RUN")
        set_tests_properties(vaeg_upd780_fdc_payloads vaeg_upd780_fdc_seed_payloads
            vaeg_upd780_fdc_fixed_payloads vaeg_upd780_fdc_capture_payloads
            vaeg_upd780_fdc_xadr_payloads vaeg_upd780_fdc_pref_payloads PROPERTIES SKIP_RETURN_CODE 77)
    endif()
    set_tests_properties(vaeg_upd780_fdc_payloads vaeg_upd780_fdc_seed_payloads
        vaeg_upd780_fdc_fixed_payloads vaeg_upd780_fdc_capture_payloads
        vaeg_upd780_fdc_xadr_payloads vaeg_upd780_fdc_pref_payloads PROPERTIES
        LABELS "romless;compatibility;upd780;host-only" TIMEOUT 180)
    add_test(NAME vaeg_z80_role_probes_selftest
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/z80_role_probes.py" selftest)
    set_tests_properties(vaeg_z80_role_probes_selftest PROPERTIES
        LABELS "romless;compatibility;role")
    foreach(role upd70008 upd780)
        if(VAEG_Z80ASM_EXECUTABLE)
            add_test(NAME vaeg_${role}_probes
                COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/z80_role_probes.py" run
                    --manifest "${CMAKE_CURRENT_SOURCE_DIR}/tests/z80_compat/${role}_probes.json"
                    --runner "$<TARGET_FILE:vaeg_cpm_runner>"
                    --assembler "${VAEG_Z80ASM_EXECUTABLE}")
            set_tests_properties(vaeg_${role}_probes PROPERTIES
                LABELS "romless;compatibility;role;${role}"
                TIMEOUT 1200)
        else()
            add_test(NAME vaeg_${role}_probes
                COMMAND "${Python3_EXECUTABLE}"
                    "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/z80_role_probes.py" skip
                    --reason "z80asm 1.8 was not found at configure time")
            set_tests_properties(vaeg_${role}_probes PROPERTIES
                LABELS "romless;compatibility;role;${role}"
                SKIP_RETURN_CODE 77)
        endif()
    endforeach()

    add_executable(vaeg_z80_compat_legacy_contract
        tests/z80_compat/legacy_contract.cpp)
    target_link_libraries(vaeg_z80_compat_legacy_contract PRIVATE
        vaeg_z80_compat_wrapper_no_functional)
    target_compile_options(vaeg_z80_compat_legacy_contract PRIVATE
        -Wall -fsigned-char)
    if(VAEG_WERROR)
        target_compile_options(vaeg_z80_compat_legacy_contract PRIVATE -Werror)
    endif()

    add_library(vaeg_z80_compat_trace_common STATIC
        tests/z80_compat/differential/trace_common.cpp)
    target_include_directories(vaeg_z80_compat_trace_common PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/z80_compat/differential")
    target_compile_features(vaeg_z80_compat_trace_common PUBLIC cxx_std_17)
    target_compile_options(vaeg_z80_compat_trace_common PRIVATE
        -Wall -fsigned-char)

    add_executable(vaeg_z80_compat_trace_reference
        tests/z80_compat/differential/reference_trace.cpp)
    target_link_libraries(vaeg_z80_compat_trace_reference PRIVATE
        vaeg_z80_compat_trace_common vaeg_z80_compat_wrapper_no_functional)
    target_compile_options(vaeg_z80_compat_trace_reference PRIVATE
        -Wall -fsigned-char)

    add_executable(vaeg_z80_compat_trace_repeat
        tests/z80_compat/differential/new_trace.cpp)
    target_link_libraries(vaeg_z80_compat_trace_repeat PRIVATE
        vaeg_z80_compat_trace_common vaeg_z80_compat_wrapper_no_functional)
    target_compile_options(vaeg_z80_compat_trace_repeat PRIVATE
        -Wall -fsigned-char)

    add_executable(vaeg_z80_compat_trace_compare
        tests/z80_compat/differential/trace_compare.cpp)
    target_include_directories(vaeg_z80_compat_trace_compare PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/z80_compat/differential")
    target_compile_features(vaeg_z80_compat_trace_compare PRIVATE cxx_std_17)
    target_compile_options(vaeg_z80_compat_trace_compare PRIVATE
        -Wall -fsigned-char)

    if(VAEG_WERROR)
        foreach(target vaeg_z80_compat_trace_common vaeg_z80_compat_trace_reference
                       vaeg_z80_compat_trace_repeat
                       vaeg_z80_compat_trace_compare)
            target_compile_options(${target} PRIVATE -Werror)
        endforeach()
    endif()

    add_test(NAME vaeg_z80_compat_legacy_contract
        COMMAND vaeg_z80_compat_legacy_contract)
    add_test(NAME vaeg_z80_compat_header_default
        COMMAND vaeg_z80_compat_header_default)
    add_test(NAME vaeg_z80_compat_header_no_functional
        COMMAND vaeg_z80_compat_header_no_functional)
    add_test(NAME vaeg_z80_compat_header_debug_default
        COMMAND vaeg_z80_compat_header_debug_default)
    add_test(NAME vaeg_z80_compat_header_release
        COMMAND vaeg_z80_compat_header_release)
    add_test(NAME vaeg_z80_compat_flag_profile
        COMMAND vaeg_z80_compat_flag_profile)
    add_test(NAME vaeg_z80_compat_interrupt_default
        COMMAND vaeg_z80_compat_interrupt_default)
    add_test(NAME vaeg_z80_compat_interrupt_no_functional
        COMMAND vaeg_z80_compat_interrupt_no_functional)
    add_test(NAME vaeg_z80_compat_wrapper_default
        COMMAND vaeg_z80_compat_wrapper_default)
    add_test(NAME vaeg_z80_compat_wrapper_no_functional
        COMMAND vaeg_z80_compat_wrapper_no_functional_test)
    add_test(NAME vaeg_upd780_disasm COMMAND vaeg_upd780_disasm)
    set_tests_properties(vaeg_upd780_disasm PROPERTIES
        LABELS "romless;upd780;disasm" TIMEOUT 120)
    add_test(NAME vaeg_librashader_frame_input
        COMMAND vaeg_librashader_frame_input_test)
    set_tests_properties(vaeg_librashader_frame_input PROPERTIES
        LABELS "romless;frontend;librashader" TIMEOUT 30)
    add_test(NAME vaeg_librashader_capture_boundary
        COMMAND vaeg_librashader_capture_boundary_test)
    set_tests_properties(vaeg_librashader_capture_boundary PROPERTIES
        LABELS "romless;frontend;librashader" TIMEOUT 30)
    add_test(NAME vaeg_librashader_presenter_state
        COMMAND vaeg_librashader_presenter_state_test)
    set_tests_properties(vaeg_librashader_presenter_state PROPERTIES
        LABELS "romless;frontend;librashader" TIMEOUT 30)
    add_test(NAME vaeg_librashader_pass_through
        COMMAND vaeg_librashader_pass_through_test)
    set_tests_properties(vaeg_librashader_pass_through PROPERTIES
        LABELS "romless;frontend;librashader" TIMEOUT 30)
    add_test(NAME vaeg_librashader_screen_size
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/frontend/librashader/test_screen_size.py")
    set_tests_properties(vaeg_librashader_screen_size PROPERTIES
        LABELS "romless;frontend;librashader" TIMEOUT 30)
    add_test(NAME vaeg_librashader_shader_parameters
        COMMAND vaeg_librashader_shader_parameters_test)
    set_tests_properties(vaeg_librashader_shader_parameters PROPERTIES
        LABELS "romless;frontend;librashader" TIMEOUT 30)
    add_test(NAME vaeg_librashader_fallback
        COMMAND vaeg_librashader_fallback_test)
    set_tests_properties(vaeg_librashader_fallback PROPERTIES
        LABELS "romless;frontend;librashader" TIMEOUT 30)
    foreach(regression_suite directed state generated convergence)
        add_test(NAME vaeg_z80_compat_regression_${regression_suite}
            COMMAND "${CMAKE_COMMAND}"
                "-DREFERENCE_RUNNER=$<TARGET_FILE:vaeg_z80_compat_trace_reference>"
                "-DREPEAT_RUNNER=$<TARGET_FILE:vaeg_z80_compat_trace_repeat>"
                "-DCOMPARATOR=$<TARGET_FILE:vaeg_z80_compat_trace_compare>"
                "-DSUITE=${regression_suite}"
                "-DCASES=128"
                "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/compat-regression"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/z80_compat/differential/run_differential.cmake")
        set_tests_properties(vaeg_z80_compat_regression_${regression_suite}
            PROPERTIES LABELS "romless;compatibility;regression" TIMEOUT 120)
    endforeach()
    if(VAEG_ZEX_ARTIFACT_DIR)
        get_filename_component(VAEG_ZEX_ARTIFACT_DIR_ABSOLUTE
            "${VAEG_ZEX_ARTIFACT_DIR}" ABSOLUTE)
        function(vaeg_verify_zex_artifact filename expected_sha256)
            set(artifact
                "${VAEG_ZEX_ARTIFACT_DIR_ABSOLUTE}/${filename}")
            if(NOT EXISTS "${artifact}")
                message(FATAL_ERROR "Required ZEX artifact is absent: ${artifact}")
            endif()
            file(SHA256 "${artifact}" actual_sha256)
            if(NOT actual_sha256 STREQUAL expected_sha256)
                message(FATAL_ERROR
                    "ZEX artifact hash mismatch for ${artifact}: "
                    "expected ${expected_sha256}, got ${actual_sha256}")
            endif()
        endfunction()
        vaeg_verify_zex_artifact(zexdoc.cim
            b3015112a99bb72273e0cacde7c7549eb9840ba996af76f7bf7992ef7d6e2f90)
        vaeg_verify_zex_artifact(zexall.cim
            fbb1bb5d46f61c33ea6841a71f2b23c49b9b62410ce6ed4e57b7d9b2e7b437e0)
        vaeg_verify_zex_artifact(zexdoc.src
            0e2e7d05e5dd27c932de64d4c3711351f53388ed02d2e99e2e706ef6216ca9b3)
        vaeg_verify_zex_artifact(zexall.src
            a263efc67ed6f890268c6f9e00f7911d9376a6bc6ddaec5ce04e33a5f483733c)
        vaeg_verify_zex_artifact(zex-license.txt
            e57f1c320b8cf8798a7d2ff83a6f9e06a33a03585f6e065fea97f1d86db84052)

        add_test(NAME vaeg_z80_compat_zexall
            COMMAND vaeg_z80_compat_zex_runner
                "${VAEG_ZEX_ARTIFACT_DIR_ABSOLUTE}/zexall.cim")
        # Role checks: uPD9002 uPD70008-compatible mode against the real
        # PC-88VA2 CRCs, FDC uPD780C against the stock Z80 CRCs. Each role
        # has its own expectation file so the roles can diverge. ZEXDOC is
        # not run: its test vectors equal ZEXALL's and its flag masks are a
        # subset of ZEXALL's all-ones masks, so a ZEXALL match implies it.
        foreach(suite zexall)
            add_test(NAME vaeg_upd70008_${suite}
                COMMAND vaeg_z80_compat_role_zex_runner
                    --role upd70008
                    --expect "${CMAKE_CURRENT_SOURCE_DIR}/tests/z80_compat/upd9002_${suite}_expected.txt"
                    "${VAEG_ZEX_ARTIFACT_DIR_ABSOLUTE}/${suite}.cim")
            set_tests_properties(vaeg_upd70008_${suite} PROPERTIES
                LABELS "romless;compatibility;zex;role;upd70008;upd9002"
                TIMEOUT 900)
            add_test(NAME vaeg_upd780_${suite}
                COMMAND vaeg_z80_compat_role_zex_runner
                    --role upd780
                    --expect "${CMAKE_CURRENT_SOURCE_DIR}/tests/z80_compat/upd780_${suite}_expected.txt"
                    "${VAEG_ZEX_ARTIFACT_DIR_ABSOLUTE}/${suite}.cim")
            set_tests_properties(vaeg_upd780_${suite} PROPERTIES
                LABELS "romless;compatibility;zex;role;upd780"
                TIMEOUT 900)
        endforeach()
        set_tests_properties(vaeg_z80_compat_zexall PROPERTIES
            LABELS "romless;compatibility;zex"
            TIMEOUT 900)
    endif()
    add_test(NAME vaeg_romless_tests COMMAND vaeg_sdl2 --selftest)
    set_tests_properties(vaeg_romless_tests PROPERTIES
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:vaeg_sdl2>"
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy"
        RESOURCE_LOCK vaeg_sxsi_image_selftest)
    add_test(NAME vaeg_sdl_startup_viewport
        COMMAND vaeg_sdl2 --smoke --no-cfg --no-bkupmem)
    set_tests_properties(vaeg_sdl_startup_viewport PROPERTIES
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:vaeg_sdl2>"
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy"
        LABELS "romless;frontend;viewport"
        TIMEOUT 30)
    # M132: synthetic-only local-QA checker tests; never use private captures in CI.
    add_test(NAME vaeg_local_basic_qa_selftest
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/compatible_basic_qa.py" --selftest)
    add_test(NAME vaeg_screen_compare_selftest
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/screen_compare.py" --selftest)
    add_test(NAME vaeg_x11_capture_selftest
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/x11_capture.py" --selftest)
    add_test(NAME vaeg_x11_matrix_selftest
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/x11_matrix.py" selftest)
    add_test(NAME vaeg_x11_gate_selftest
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/x11_gate.py" --selftest)
    add_test(NAME vaeg_guest_start_qa_selftest
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/guest_start_qa.py" --selftest)
    set_tests_properties(vaeg_local_basic_qa_selftest vaeg_screen_compare_selftest
        vaeg_x11_capture_selftest vaeg_x11_matrix_selftest vaeg_x11_gate_selftest
        vaeg_guest_start_qa_selftest PROPERTIES
        LABELS "romless;frontend;local-qa;m132")
    # M103b-M103d: V2 disk BASIC boots through automatic FDD selection,
    # answers PRINT 1+1, draws a painted circle and round-trips a program
    # through SAVE and LOAD. Needs maintainer-local VA2 ROMs and V2 BASIC
    # media.
    set(VAEG_V1V2_ROM_DIR "" CACHE PATH "Maintainer-local VA2 ROM directory for M103b")
    set(VAEG_V1V2_BASIC_MEDIA "" CACHE FILEPATH "Maintainer-local V2 BASIC D88 for M103b")
    add_test(NAME vaeg_m103b_basic_boot_selftest
        COMMAND "${Python3_EXECUTABLE}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/m103b_basic_boot.py" selftest)
    set_tests_properties(vaeg_m103b_basic_boot_selftest PROPERTIES
        LABELS "romless;v1v2;m103b")
    if(VAEG_V1V2_ROM_DIR AND VAEG_V1V2_BASIC_MEDIA)
        add_test(NAME vaeg_m103b_basic_boot
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/m103b_basic_boot.py" run
                --worker "$<TARGET_FILE:vaeg_sdl2>"
                --roms "${VAEG_V1V2_ROM_DIR}"
                --media "${VAEG_V1V2_BASIC_MEDIA}")
        set_tests_properties(vaeg_m103b_basic_boot PROPERTIES
            LABELS "romful;v1v2;m103b;external"
            TIMEOUT 900)
    else()
        add_test(NAME vaeg_m103b_basic_boot
            COMMAND "${Python3_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/qa/m103b_basic_boot.py" skip
                --reason "VAEG_V1V2_ROM_DIR and VAEG_V1V2_BASIC_MEDIA are not configured")
        set_tests_properties(vaeg_m103b_basic_boot PROPERTIES
            LABELS "v1v2;m103b;external"
            SKIP_RETURN_CODE 77)
    endif()
    add_test(NAME vaeg_m75_transfer_info_compiled COMMAND vaeg_sdl2 --selftest)
    set_tests_properties(vaeg_m75_transfer_info_compiled PROPERTIES
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:vaeg_sdl2>"
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy"
        LABELS "m75;scsi;compiled"
        RESOURCE_LOCK vaeg_sxsi_image_selftest)
    # Recording fixtures use assert both for checks and state-driving API calls.
    # Release -DNDEBUG must not turn them into false passes or infinite EOF loops.
    foreach(recording_test
            vaeg_recording_mp4_test vaeg_recording_realtime_clock_test
            vaeg_recording_c_contract_test vaeg_recording_cli_test
            vaeg_recording_osd_snapshot_test vaeg_recording_displayed_capture_test
            vaeg_recording_runtime_test vaeg_recording_native_composition_test
            vaeg_recording_native_tap_test vaeg_recording_contract_test
            vaeg_recording_clock_test vaeg_recording_queue_test
            vaeg_recording_native_session_test vaeg_recording_runtime_integration_test
            vaeg_recording_metal_archive_probe vaeg_recording_ffmpeg_capabilities_test
            vaeg_recording_macos_window_capture_test vaeg_recording_h264_encoder_test
            vaeg_recording_ffv1_test vaeg_recording_pcm_test vaeg_recording_matroska_test)
        if(TARGET ${recording_test})
            target_compile_options(${recording_test} PRIVATE -UNDEBUG)
        endif()
    endforeach()
    if(VAEG_ENABLE_RECORDING)
        set_tests_properties(vaeg_recording_ffv1 PROPERTIES TIMEOUT 30)
    endif()
endif()
