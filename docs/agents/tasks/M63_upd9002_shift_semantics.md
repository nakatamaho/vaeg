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
# M63 — Correct SHL/SAL/SHR/SAR semantics

## Mandatory preparation

Before doing any work:

1. Read `AGENTS.md`.
2. Read `docs/agents/ROADMAP.md` and `docs/agents/CONVENTIONS.md`.
3. Read `docs/agents/UPD9002_SEMANTICS_MIGRATION.md`.
4. Read this task and every report from prerequisite gates.
5. Run `git status --short`; use a clean dedicated worktree at the exact approved predecessor SHA.
6. Record the starting branch, SHA, remote, tool versions, and verified SST corpus identity.
7. Resolve the approved predecessor from the maintainer-approved report; never infer it from HEAD,
   a branch tip, a mutable tag, or milestone numbering.
8. Execute this milestone only and stop at its candidate gate.

All newly authored source, comments, identifiers, commit messages, test names, schemas, and
repository documentation must be in English.


## Predecessor and identifiers

Prerequisite: G62c explicitly approved and current ranking confirms shifts.

Branch: `topic/m63-upd9002-shifts`

Commit prefix: `M63:`

Gate: `G63`

## Goal

Correct C0/C1/D2/D3 subforms `.4-.7` from the M59 stratified evidence.

## Evidence discipline

M59 showed that raw-count, `count & 0x1f`, and neither-model observations coexist. Do not begin by
installing one unconditional count rule. Re-stratify by width, register/memory, subform, count source,
count 0/1/>1/at-width/beyond-width, sign, and initial CF.

If count/destination semantics and FLAGS semantics have independent causes that cannot be safely
reviewed as one family, stop before production edits and obtain separately gated M63 submilestones.

Implement only evidenced destination, CF, OF, AF, ZF, SF, PF, count-zero, alias, and memory rules.
Preserve the exact 40,000 applicable/green rotate hashes confirmed by M59/current epoch.

## Gate G63

All included applicable shift failures clear, no rotate regression occurs, and all target-policy
identities/classifications remain unchanged.

Write `docs/agents/reports/m63_upd9002_shift_semantics.md` and stop.
