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
# M62a — Correct AAM (D4) semantics

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

Prerequisite: G61 explicitly approved and fresh ranking confirms D4.

Branch: `topic/m62a-upd9002-aam`

Commit prefix: `M62a:`

Gate: `G62a`

## Goal

Correct D4 using the complete M59 immediate-stratified evidence. D5 remains protected.

## Required work

- Revalidate D4 expected/actual behavior for all immediate values represented by the corpus.
- Cover immediate 0, 1, 2, 9, 10, 11, 16, and 255 explicitly.
- Preserve M59's proven normal termination for the D4 immediate-zero population unless newer
  target evidence contradicts it.
- Implement quotient/remainder, defined FLAGS, and side effects exactly as observed.

Do not assume a base-10 rule from tradition; state the evidence-derived formula. Do not modify D5:
M59 proved all 5,000 D5 architectural records executed and passed.

## Gate G62a

All applicable D4 failures are cleared with no D5 or unrelated BCD regression. Run target-correct
architectural CI/full, fingerprint, ratchet, and full repository gates.

Write `docs/agents/reports/m62a_upd9002_aam.md` and stop.
