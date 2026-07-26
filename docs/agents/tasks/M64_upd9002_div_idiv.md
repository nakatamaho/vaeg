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
# M64 — Correct DIV/IDIV and divide-error semantics

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

Prerequisite: G62 explicitly approved and current ranking confirms F6.6/F6.7/F7.6/F7.7.

Branch: `topic/m64-upd9002-div-idiv`

Commit prefix: `M64:`

Gate: `G64`

## Goal

Correct unsigned/signed division and divide-error behavior after FLAGS/frame/IRET foundations are
approved.

## Required work

Derive and implement quotient/remainder, zero divisor, overflow boundaries, most-negative quotient,
defined successful FLAGS, failure-path register preservation, pushed IP/CS/FLAGS, termination, and
applicable REP-prefix behavior. Use transactional execution so a fault does not leak partial state
unless exact SST evidence requires it.

Do not alter F7 `/2`, other group subforms, target classifications, cycles, or prefetch. Choose saved
IP from observed SST frame bytes, not Intel terminology.

## Gate G64

All applicable DIV/IDIV failures clear, no unrelated F6/F7 regression occurs, and a complete fresh
failure inventory is published for M65.

Write `docs/agents/reports/m64_upd9002_div_idiv.md` and stop.
