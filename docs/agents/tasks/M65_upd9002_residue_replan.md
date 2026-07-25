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
# M65 — Re-plan the target-correct structural and long-tail residue

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


## Status and identifiers

This is planning/evidence only. Prerequisite: G64 explicitly approved.

Branch: `topic/m65-upd9002-residue-plan`

Commit prefix: `M65:`

Gate: `G65`

## Goal

Replace the residue bucket with actual independently gated tasks from the exact G64 target-correct
scoreboard and all remaining `implementation_missing` entries.

## Mandatory populations

At minimum classify and plan:

- F7 `/2` low-memory word RMW, kept separate from C6/C7;
- BOUND range-decision residuals, separate from frame-only history;
- FF `/7` normal-completion state behavior;
- active `6C-6F` V20 handlers: acquire reserved-opcode behavior evidence, then remove or route them so
  they are not reachable/advertised as uPD9002 string-I/O instructions; never return them to the
  blocking denominator;
- BRKFEM `0FFE` vector handling, destination mode, and return mechanism;
- BRKEM/BRKFEM relationship and Z80-side RETEM/CALLN questions;
- the M60c 66/67/FPO2 conclusion, including a separate implementation/profile-policy task if target
  support is proven, or exact pending evidence if unresolved;
- remaining NEC `0F` implementation-missing forms;
- undefined/reserved opcode policy;
- REPC/REPNC and multi-prefix restart;
- remaining long-tail failures.

Do not interpret missing FPO generic strings as absence. Do not plan INS/EXT or `6C-6F` V20
semantics as implementation work after M60b target authority.

## Required output

For every proposed task provide exact selectors/hashes/digests, mismatch classes, evidence status,
blast radius, prerequisite order, task/report/branch/gate names, and classification transitions.
Create the actual future task files only after the decomposition is supported by evidence.

## Gate G65

No production semantic change. Every remaining applicable failure and implementation-missing entry
has one non-overlapping owner or an explicit evidence task. Maintainer approval is required before
any generated semantic task begins.

Write `docs/agents/reports/m65_upd9002_residue_replan.md` and stop.
