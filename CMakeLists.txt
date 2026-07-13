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

option(VAEG_ENABLE_TESTS "Enable vaeg tests" OFF)
option(VAEG_WERROR "Treat warnings as errors" OFF)
option(VAEG_WINDOWS_CONSOLE "Keep a console window for Windows SDL2 builds" OFF)
option(VAEG_FETCH_SDL2 "Fetch pinned SDL2 instead of using system SDL2" OFF)
option(VAEG_STATIC_SDL2 "Link SDL2 statically" OFF)
option(VAEG_ENABLE_ARCHIVE_DROP
    "Enable archive disk-image drag and drop when LibArchive is available" ON)
option(VAEG_FETCH_LIBARCHIVE
    "Fetch pinned static archive dependencies instead of using system LibArchive" OFF)

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

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(VAEG_INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}/sdl2"
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/common"
    "${CMAKE_CURRENT_SOURCE_DIR}/i286c"
    "${CMAKE_CURRENT_SOURCE_DIR}/io"
    "${CMAKE_CURRENT_SOURCE_DIR}/cbus"
    "${CMAKE_CURRENT_SOURCE_DIR}/bios"
    "${CMAKE_CURRENT_SOURCE_DIR}/biosva"
    "${CMAKE_CURRENT_SOURCE_DIR}/sound"
    "${CMAKE_CURRENT_SOURCE_DIR}/sound/vermouth"
    "${CMAKE_CURRENT_SOURCE_DIR}/sound/getsnd"
    "${CMAKE_CURRENT_SOURCE_DIR}/vram"
    "${CMAKE_CURRENT_SOURCE_DIR}/vramva"
    "${CMAKE_CURRENT_SOURCE_DIR}/fdd"
    "${CMAKE_CURRENT_SOURCE_DIR}/lio"
    "${CMAKE_CURRENT_SOURCE_DIR}/font"
    "${CMAKE_CURRENT_SOURCE_DIR}/cpucva"
    "${CMAKE_CURRENT_SOURCE_DIR}/cpuxva"
    "${CMAKE_CURRENT_SOURCE_DIR}/iova"
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
    i286c/i286c.c
    i286c/i286c_mn.c
    i286c/i286c_ea.c
    i286c/i286c_0f.c
    i286c/i286c_8x.c
    i286c/i286c_sf.c
    i286c/i286c_f6.c
    i286c/i286c_fe.c
    i286c/i286c_rp.c
    i286c/v30patch.c
    i286c/memory.c
    i286c/egcmem.c
    i286c/dmap.c
    io/iocore.c
    io/artic.c
    io/bmsio.c
    io/cgrom.c
    io/cpuio.c
    io/crtc.c
    io/dipsw.c
    io/dmac.c
    io/egc.c
    io/epsonio.c
    io/emsio.c
    io/fdc.c
    io/fdd320.c
    io/gdc.c
    io/gdc_sub.c
    io/gdc_pset.c
    io/mouseif.c
    io/necio.c
    io/nmiio.c
    io/np2sysp.c
    io/np2vasup.c
    io/pic.c
    io/pit.c
    io/printif.c
    io/serial.c
    io/sysport.c
    io/upd4990.c
    cbus/cbuscore.c
    cbus/sasiio.c
    cbus/scsiio.c
    cbus/scsicmd.c
    cbus/pc9861k.c
    cbus/mpu98ii.c
    cbus/board14.c
    cbus/board26k.c
    cbus/board86.c
    cbus/boardx2.c
    cbus/board118.c
    cbus/boardspb.c
    cbus/amd98.c
    cbus/pcm86io.c
    cbus/cs4231io.c
    sound/vermouth/midiout.c
    sound/vermouth/midimod.c
    sound/vermouth/midinst.c
    sound/vermouth/midvoice.c
    sound/vermouth/midtable.c
    sound/getsnd/getsnd.c
    sound/getsnd/getwave.c
    sound/getsnd/getsmix.c
    bios/bios.c
    bios/bios09.c
    bios/bios0c.c
    bios/bios12.c
    bios/bios13.c
    bios/bios18.c
    bios/bios19.c
    bios/bios1a.c
    bios/bios1b.c
    bios/bios1c.c
    bios/bios1f.c
    bios/sxsibios.c
    sound/sound.c
    sound/sndcsec.c
    sound/fmboard.c
    sound/fmtimer.c
    sound/beepc.c
    sound/beepg.c
    sound/tms3631c.c
    sound/tms3631g.c
    sound/opngenc.c
    sound/opngeng.c
    sound/psggenc.c
    sound/psggeng.c
    sound/rhythmc.c
    sound/adpcmc.c
    sound/adpcmg.c
    sound/pcm86c.c
    sound/pcm86g.c
    sound/cs4231c.c
    sound/cs4231g.c
    sound/soundrom.c
    sound/s98.c
    vram/vram.c
    vram/scrndraw.c
    vram/sdraw.c
    vram/sdrawq16.c
    vram/dispsync.c
    vram/palettes.c
    vram/maketext.c
    vram/maketgrp.c
    vram/makegrph.c
    vram/scrnbmp.c
    fdd/diskdrv.c
    fdd/newdisk.c
    fdd/fddfile.c
    fdd/fdd_xdf.c
    fdd/fdd_d88.c
    fdd/fdd_mtr.c
    fdd/sxsi.c
    lio/lio.c
    lio/gscreen.c
    lio/gpset.c
    lio/gline.c
    lio/gcircle.c
    lio/gput1.c
    font/font.c
    font/fontdata.c
    font/fontpc88.c
    font/fontpc98.c
    font/fontv98.c
    font/fontfm7.c
    font/fontx1.c
    font/fontx68k.c
    font/fontmake.c
    pccore.c
    nevent.c
    calendar.c
    timing.c
    keystat.c
    statsave.c
    debugsub.c)

set(VAEG_VA_SOURCES
    biosva/biosva.c
    cpucva/gvramva.c
    cpucva/memoryva.c
    cpucva/z80c.cpp
    cpucva/z80diag.cpp
    iova/bkupmemva.c
    iova/boardsb2.c
    iova/cgromva.c
    iova/fdsubsys.c
    iova/gactrlva.c
    iova/i8255.c
    iova/iocoreva.c
    iova/memctrlva.c
    iova/mouseifva.c
    iova/sgp.c
    iova/subsystem.cpp
    iova/subsystemif.c
    iova/subsystemmx.c
    iova/sysportva.c
    iova/tsp.c
    iova/upd9002.c
    iova/va91.c
    iova/videova.c
    vramva/makegrphva.c
    vramva/makesprva.c
    vramva/maketextva.c
    vramva/palettesva.c
    vramva/scrndrawva.c
    vramva/sdrawva.c)

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
    sdl2/dosio.c
    sdl2/ini.c
    sdl2/taskmng.c
    sdl2/pacing.c
    sdl2/framedisp.c
    sdl2/viewport.c
    sdl2/appicon.c
    sdl2/splash.c
    sdl2/sysmng.c
    sdl2/timemng.c
    sdl2/trace.c
    sdl2/scrnmng.c
    sdl2/sdlkbd.c
    sdl2/kbdinject.c
    sdl2/kbdmap.c
    sdl2/kbdpaste.c
    sdl2/romankana.c
    sdl2/inputmng.c
    sdl2/dropmedia.cpp
    sdl2/romcheck.c
    sdl2/selftest.c
    sdl2/soundmng.c
    sdl2/ymfmbridge.cpp
    sdl2/mousemng.c
    sdl2/mousestate.c
    sdl2/joymng.c
    sdl2/commng.c
    sdl2/fontmng.c
    sdl2/gui/gui.cpp)

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

add_library(vaeg_core STATIC ${VAEG_CORE_SOURCES})
target_include_directories(vaeg_core PUBLIC ${VAEG_INCLUDE_DIRS})
target_link_libraries(vaeg_core PUBLIC vaeg_common)
target_compile_definitions(vaeg_core PUBLIC
    USE_I286C
    SUPPORT_BMS
    SUPPORT_SASI
    SUPPORT_V30ORIGINAL
    VAEG_FIX)
target_compile_options(vaeg_core PRIVATE -Wall -fsigned-char)
if(VAEG_WERROR)
    target_compile_options(vaeg_core PRIVATE -Werror)
endif()

add_library(vaeg_va STATIC ${VAEG_VA_SOURCES})
target_include_directories(vaeg_va PUBLIC ${VAEG_INCLUDE_DIRS})
target_link_libraries(vaeg_va PUBLIC vaeg_core vaeg_common)
target_compile_options(vaeg_va PRIVATE -Wall -fsigned-char)
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
    "${CMAKE_CURRENT_SOURCE_DIR}/external/ymfm/src")
target_link_libraries(vaeg_sdl2 PRIVATE vaeg_core vaeg_va vaeg_common
    vaeg_core vaeg_va ${VAEG_SDL2_LIBS})
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
    add_test(NAME vaeg_romless_tests COMMAND vaeg_sdl2 --selftest)
    set_tests_properties(vaeg_romless_tests PROPERTIES
        WORKING_DIRECTORY "$<TARGET_FILE_DIR:vaeg_sdl2>"
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
endif()
