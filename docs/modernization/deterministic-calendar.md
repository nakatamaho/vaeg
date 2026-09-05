<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# Deterministic calendar initialization

`--calendar-start YYYY-MM-DDTHH:MM:SS` explicitly selects the initial civil
calendar time for a new process. It is optional and disabled by default.
The format has no timezone suffix: it is a guest civil time, not a host Unix
timestamp. No host timezone conversion is performed. Years must be within
1980 through 2079, matching the existing calendar century window. Invalid
dates, extra characters, leap seconds and duplicate options fail closed.

Without this option, existing host-time initialization and calendar-mode
selection are unchanged. With it, initialization uses the supplied seed and
calendar reads use the existing virtual calendar, including its normal
emulated-time progression and guest calendar writes. The persistent calendar
configuration is not rewritten. This is an explicit runtime policy, not a
trace filter, a frozen clock, or normalization of differing trace records.
It does not depend on whether tracing or test memory is enabled.

## Why an explicit seed is needed

`machine/calendar.c:calendar_initialize` obtains its initial value from
`sdl2/timemng.c:timemng_gettime`. Previously, even virtual-calendar mode was
initially seeded through host `time`/`localtime`. Consequently two otherwise
identical processes could expose different clock input to the guest.
Selecting virtual progression alone did not define a reproducible reset
contract. The new option supplies the missing input without replacing the
calendar device or adding observation reads.

## ROM-free validation

The SDL selftest verifies default-off behavior, strict parsing, Gregorian
date validation, weekday calculation, invalid-input nonmutation, CLI duplicate
rejection, virtual progression, initialization repeatability and restoration
of the default policy. These tests use project-authored dates only.
The existing production-memory trace matrix runs the selftest in P0/P1/T0/T1;
the P1 build still disables tests and must contain no flat test-memory backend.

The save/load/save selftest separately pauses asynchronous SDL sound synthesis
throughout its stopped-guest comparison. State loading normally resumes audio,
which could otherwise advance the FM backend between the two snapshots without
guest CPU execution. This only fixes the test's stopped-state assumption;
production audio and state-load behavior remain unchanged, and FM state bytes
are still compared without masking.

This option does not establish firmware correctness, disk boot success, or
hardware validation. Private execution inputs and results are not part of this
document or public test fixtures.
