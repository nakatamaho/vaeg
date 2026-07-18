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
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.

if(NOT DEFINED VAEG_EXECUTABLE OR NOT DEFINED TRACE_GOLDEN)
    message(FATAL_ERROR "VAEG_EXECUTABLE and TRACE_GOLDEN are required")
endif()

function(run_trace output_variable)
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E env
            SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy
            "${VAEG_EXECUTABLE}" --selftest --trace-cpu 8
        RESULT_VARIABLE result
        OUTPUT_VARIABLE stdout_text
        ERROR_VARIABLE stderr_text)
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "trace-enabled selftest failed (${result}):\n${stderr_text}")
    endif()
    string(REGEX MATCHALL
        "(upd9002-trace-v1|begin step=[^\n]*|event step=[^\n]*|end step=[^\n]*)"
        trace_lines "${stderr_text}")
    if(NOT trace_lines)
        message(FATAL_ERROR "trace-enabled run emitted no canonical trace")
    endif()
    list(JOIN trace_lines "\n" trace_text)
    set(${output_variable} "${trace_text}\n" PARENT_SCOPE)
    string(REGEX REPLACE
        "upd9002-trace-v1\n|begin step=[^\n]*\n|event step=[^\n]*\n|end step=[^\n]*\n"
        "" checkpoint_text "${stderr_text}")
    set(${output_variable}_checkpoints "${checkpoint_text}" PARENT_SCOPE)
endfunction()

run_trace(first)
run_trace(second)
if(NOT first STREQUAL second)
    message(FATAL_ERROR "two identical trace-enabled runs differ")
endif()
if(NOT first_checkpoints STREQUAL second_checkpoints)
    message(FATAL_ERROR "two trace-enabled checkpoint streams differ")
endif()
foreach(origin cpu dma device)
    if(NOT first MATCHES "origin=${origin}")
        message(FATAL_ERROR "canonical trace lacks ${origin} origin")
    endif()
endforeach()
file(READ "${TRACE_GOLDEN}" golden)
if(NOT first STREQUAL golden)
    message(FATAL_ERROR "canonical trace differs from M42 golden")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env
        SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy
        "${VAEG_EXECUTABLE}" --selftest
    RESULT_VARIABLE untraced_result
    OUTPUT_QUIET ERROR_VARIABLE untraced_error)
if(NOT untraced_result EQUAL 0)
    message(FATAL_ERROR "trace-disabled equivalence selftest failed: ${untraced_error}")
endif()
if(NOT first_checkpoints STREQUAL untraced_error)
    message(FATAL_ERROR
        "trace-enabled and trace-disabled final checkpoint streams differ")
endif()

message(STATUS "uPD9002 trace determinism, origin schema, golden, and complete on/off checkpoint equivalence passed")
