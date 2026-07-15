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

foreach(required LEGACY_RUNNER NEW_RUNNER COMPARATOR SUITE OUTPUT_DIR)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "${required} is required")
    endif()
endforeach()

if(NOT DEFINED CASES)
    set(CASES 128)
endif()

file(MAKE_DIRECTORY "${OUTPUT_DIR}")
set(legacy_trace "${OUTPUT_DIR}/${SUITE}-legacy.trace")
set(new_trace "${OUTPUT_DIR}/${SUITE}-new.trace")
set(cycle_report "${OUTPUT_DIR}/${SUITE}-cycle-deltas.md")
if(DEFINED CYCLE_REPORT AND NOT CYCLE_REPORT STREQUAL "")
    set(cycle_report "${CYCLE_REPORT}")
endif()

execute_process(
    COMMAND "${LEGACY_RUNNER}" --suite "${SUITE}" --cases "${CASES}"
        --output "${legacy_trace}"
    RESULT_VARIABLE legacy_result
    OUTPUT_VARIABLE legacy_output
    ERROR_VARIABLE legacy_error)
if(NOT legacy_result EQUAL 0)
    message(FATAL_ERROR
        "legacy trace failed (${legacy_result})\n${legacy_output}${legacy_error}")
endif()
message(STATUS "${legacy_output}")

execute_process(
    COMMAND "${NEW_RUNNER}" --suite "${SUITE}" --cases "${CASES}"
        --output "${new_trace}"
    RESULT_VARIABLE new_result
    OUTPUT_VARIABLE new_output
    ERROR_VARIABLE new_error)
if(NOT new_result EQUAL 0)
    message(FATAL_ERROR
        "new trace failed (${new_result})\n${new_output}${new_error}")
endif()
message(STATUS "${new_output}")

execute_process(
    COMMAND "${COMPARATOR}" "${legacy_trace}" "${new_trace}"
        --cycle-report "${cycle_report}"
    RESULT_VARIABLE compare_result
    OUTPUT_VARIABLE compare_output
    ERROR_VARIABLE compare_error)
if(NOT compare_result EQUAL 0)
    message(FATAL_ERROR
        "trace comparison failed (${compare_result})\n"
        "${compare_output}${compare_error}")
endif()
message(STATUS "${compare_output}")
