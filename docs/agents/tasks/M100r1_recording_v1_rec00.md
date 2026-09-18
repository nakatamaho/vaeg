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
# M100r1 - REC00 recording-v1 checkout audit

Status: WAITING_HUMAN
Feature: REC00
Scope: audit and documentation only

This wrapper assigns exactly REC00 of recording-v1 to legal repository ID
M100r1. Read docs/recording-v1/spec.md, workflow.md, milestones.md, status.md,
handoff.md and milestones/rec00-audit.md together with the applicable root
instructions before acting.

REC00 must inspect the assigned checkout, record actual video/audio clocks and
ownership, enumerate all OSD/status layers, run the applicable baseline checks,
and update the recording-v1 audit, ID map, platform matrix, status, handoff and
file placement records. It must create the REC00 report. It must not implement
REC01 or any recording production code, change existing screenshot/QA behavior,
rewrite the roadmap, or publish private media evidence.

The REC00 gate is a maintainer human gate. Stop after the audit is complete and
leave the status WAITING_HUMAN until the maintainer explicitly accepts it.
