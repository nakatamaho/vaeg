# M100r14 - REC13 recording-v1 integrated Native A/V session

Status: IMPLEMENTED_UNVERIFIED
Feature: REC13
Scope: connect the Native guest-boundary video tap, guest-time audio tap,
bounded complete-slice queue and private MKV writer

This wrapper assigns REC13 of recording-v1 to legal repository ID M100r14.
Read `docs/recording-v1/milestones/rec13-native-session.md` and its named
references together with the applicable root instructions before acting.

The implementation owns the Native session lifecycle, uses the common guest
clock and indexed audio sample grid, copies complete Native/OSD slices before
queue submission, and writes FFV1 v3 BGR0 video plus PCM S16LE stereo into a
seekable MKV. Reset, state-load, descriptor and audio-epoch changes stop the
session fail-closed. The private environment controller is temporary until the
public recording control milestone; existing screenshots, QA captures,
Displayed video and recording-OFF behavior remain unchanged.

The machine evidence and unresolved guest/human validation are recorded in
`docs/agents/reports/recording-v1/rec13-report.md`. This wrapper does not claim
Displayed presenter support, public UI, VLC acceptance, cross-platform runtime
coverage or release validation.

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
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->
