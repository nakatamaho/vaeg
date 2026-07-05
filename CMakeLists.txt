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

find_package(SDL2 QUIET CONFIG)
if(TARGET SDL2::SDL2)
    set(VAEG_SDL2_LIBS SDL2::SDL2)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
    set(VAEG_SDL2_LIBS PkgConfig::SDL2)
endif()

option(VAEG_ENABLE_TESTS "Enable vaeg tests" OFF)
option(VAEG_WERROR "Treat warnings as errors" OFF)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(VAEG_INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}/sdl2"
    "${CMAKE_CURRENT_SOURCE_DIR}/sdl"
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
    generic/cmver.c)

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
    sdl2/np2.c
    sdl2/dosio.c
    sdl2/ini.c
    sdl2/taskmng.c
    sdl2/sysmng.c
    sdl2/timemng.c
    sdl2/trace.c
    sdl2/scrnmng.c
    sdl2/sdlkbd.c
    sdl2/inputmng.c
    sdl2/soundmng.c
    sdl2/mousemng.c
    sdl2/joymng.c
    sdl2/commng.c
    sdl2/fontmng.c
    sdl2/gui/gui.cpp)

add_library(vaeg_common STATIC ${VAEG_COMMON_SOURCES})
target_include_directories(vaeg_common PUBLIC ${VAEG_INCLUDE_DIRS})
target_compile_definitions(vaeg_common PUBLIC SUPPORT_PC88VA)
target_compile_options(vaeg_common PRIVATE -Wall -fsigned-char)
if(VAEG_WERROR)
    target_compile_options(vaeg_common PRIVATE -Werror)
endif()

add_library(vaeg_core STATIC ${VAEG_CORE_SOURCES})
target_include_directories(vaeg_core PUBLIC ${VAEG_INCLUDE_DIRS})
target_link_libraries(vaeg_core PUBLIC vaeg_common)
target_compile_definitions(vaeg_core PUBLIC
    USE_I286C
    SUPPORT_PC88VA
    SUPPORT_BMS
    SUPPORT_V30ORIGINAL)
set_source_files_properties(i286c/i286c.c pccore.c
    PROPERTIES COMPILE_DEFINITIONS VAEG_FIX)
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

add_executable(vaeg_sdl2 ${VAEG_SDL2_SOURCES})
set_target_properties(vaeg_sdl2 PROPERTIES
    OUTPUT_NAME vaeg
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/sdl2")
target_include_directories(vaeg_sdl2 PRIVATE
    ${VAEG_INCLUDE_DIRS}
    "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui"
    "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends")
target_link_libraries(vaeg_sdl2 PRIVATE vaeg_core vaeg_va vaeg_common
    vaeg_core vaeg_va ${VAEG_SDL2_LIBS})
if(UNIX)
    target_link_libraries(vaeg_sdl2 PRIVATE m)
endif()
target_compile_options(vaeg_sdl2 PRIVATE -Wall -fsigned-char)
if(VAEG_WERROR)
    target_compile_options(vaeg_sdl2 PRIVATE -Werror)
endif()
