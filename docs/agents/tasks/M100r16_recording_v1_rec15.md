# M100r16 - REC15 recording-v1 Displayed SDL readback seam

Status: IMPLEMENTED_UNVERIFIED
Feature: REC15
Scope: define the final-client Displayed capture contract and add the SDL
renderer-owner readback seam

This wrapper assigns REC15 of recording-v1 to legal repository ID M100r16.
Read `docs/recording-v1/milestones/rec15-displayed-contract.md` and its named
references together with the applicable root instructions before acting.

The implementation reads the actual SDL renderer output immediately before
`SDL_RenderPresent`, after the guest, enabled guest-area OSD, status and
in-client GUI composition. It uses a fixed drawable descriptor, a guest
`drawcount` token and a bounded reusable RGBA32 buffer. Dummy and unsupported
contexts fail explicitly; resize and renderer-generation changes stop the
capture. REC16 still owns the recording-session/MKV connection.

The machine report records the deterministic render-target fixture, ON/OFF,
Werror and sanitizer checks, and the unavailable real macOS UI-service run.
It does not claim Displayed recording, cross-platform presenter support, human
acceptance or release validation.

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
