# M100r13 - REC12 recording-v1 native guest-boundary video tap

Status: MACHINE_GATE_PASS
Feature: REC12
Scope: complete guest-boundary Native RGB frame and OSD tap

This wrapper assigns REC12 of recording-v1 to legal repository ID M100r13.
Read `docs/recording-v1/milestones/rec12-native-video-tap.md` and its named
references together with the applicable root instructions before acting.

The implementation forces canonical guest composition whenever the Native tap
is active, even when normal host frame skipping would suppress a draw. It emits
one owned Native RGB24 frame and its matching immutable OSD snapshot at each
complete guest display boundary. The tap carries a monotonic guest-clock
interval, sequence, and descriptor generation through a C-compatible callback.
Existing screenshot, QA, redraw, and non-recording paths remain unchanged.

The mapped machine checks and evidence are recorded in
`docs/agents/reports/recording-v1/rec12-report.md`. This is not the integrated
Native A/V session, public recording UI, live MKV, presenter runtime, human
demonstration, or release validation.

<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->
