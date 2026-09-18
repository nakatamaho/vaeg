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
# M100r2 - REC01 recording-v1 contracts

Status: MACHINE_GATE_PASS
Feature: REC01
Scope: recording contracts and compiled-OFF skeleton

This wrapper assigns exactly REC01 of recording-v1 to legal repository ID
M100r2. Read docs/recording-v1/milestones/rec01-contracts.md and its named
references together with the applicable root instructions before acting.

REC01 adds only the narrow C99/C++17 contract boundary, deterministic descriptor
validation, owned frame storage, inert capability/status behavior, the
VAEG_ENABLE_RECORDING target option and focused contract tests. It must not add
FFmpeg/libav types or dependencies, live taps, workers, encoders, UI commands,
or changes to screenshots and QA.

The REC01 gate is machine-verifiable. Stop after the required checks pass and
do not begin REC02 in this session.
