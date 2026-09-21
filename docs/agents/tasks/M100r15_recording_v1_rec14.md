# M100r15 - REC14 recording-v1 Native recording controls

Status: IMPLEMENTED_UNVERIFIED
Feature: REC14
Scope: add the public Native recording CLI and GUI controls, validation and safe stop feedback

This wrapper assigns REC14 of recording-v1 to legal repository ID M100r15.
Read `docs/recording-v1/milestones/rec14-native-ui.md` and its named
references together with the applicable root instructions before acting.

The implementation exposes exactly the Native video and Displayed video
choices, with Native as the default. Displayed remains visibly unavailable
until a final-client presenter path exists. Both CLI and GUI use the same
Native session controller, preserve enabled guest-area OSD, reject invalid or
existing destinations before creating output, support finite frame counts and
provide a stop/finalization/error status. Existing screenshots, QA captures,
recording-OFF behavior, C99 core boundaries and private-data restrictions are
preserved.

Machine evidence and the pending real guest demonstration are recorded in
`docs/agents/reports/recording-v1/rec14-report.md`. This wrapper does not claim
Displayed presenter support, VLC acceptance, cross-platform runtime coverage,
human acceptance or release validation.

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
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->
