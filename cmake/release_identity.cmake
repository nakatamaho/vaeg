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
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# Explicit source identity, NOT inferred from commit subjects, task filenames,
# the highest milestone number or the last merged documentation branch.
# Remove -dev only in an explicitly accepted release-preparation commit.
set(VAEG_SOURCE_MILESTONE "M142-dev")

function(vaeg_release_identity release milestone commit)
    if(NOT milestone MATCHES "^M(0|[1-9][0-9]*([a-z]([1-9][0-9]*)?)?)(-dev)?$")
        message(FATAL_ERROR "RELEASE_ID_MILESTONE: invalid canonical milestone")
    endif()
    if(NOT release MATCHES "^(unknown|[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9](\\.[1-9][0-9]*)?)$")
        message(FATAL_ERROR "RELEASE_ID_DATE: invalid release date")
    endif()
    string(LENGTH "${commit}" commit_length)
    if(NOT commit STREQUAL "unknown" AND (NOT commit MATCHES "^[0-9a-f]+$" OR NOT commit_length EQUAL 40))
        message(FATAL_ERROR "RELEASE_ID_COMMIT: invalid source commit")
    endif()
    string(SUBSTRING "${commit}" 0 8 short_commit)
    string(TOLOWER "${milestone}" package_milestone)
    set(display "Rel.${release} (${milestone})")
    set(stem "vaeg-${release}-${package_milestone}")
    if(milestone MATCHES "-dev$")
        set(display "Rel.${release} (${milestone}, ${short_commit})")
        string(APPEND stem "-g${short_commit}")
    endif()
    set(VAEG_BUILD_MILESTONE "${milestone}" PARENT_SCOPE)
    set(VAEG_BUILD_SHORT_COMMIT "${short_commit}" PARENT_SCOPE)
    set(VAEG_BUILD_DISPLAY "${display}" PARENT_SCOPE)
    set(VAEG_PACKAGE_STEM "${stem}" PARENT_SCOPE)
endfunction()

# Direct script invocation is the release workflow's cheap preflight.
# Inclusion by the normal development configure and fixture tests is not.
if(CMAKE_SCRIPT_MODE_FILE STREQUAL CMAKE_CURRENT_LIST_FILE AND
        VAEG_SOURCE_MILESTONE MATCHES "-dev$")
    message(FATAL_ERROR "RELEASE_ID_DEVELOPMENT: milestone gate is not accepted")
endif()
