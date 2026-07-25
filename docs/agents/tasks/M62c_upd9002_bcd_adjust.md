# M62c — Correct the BCD/ASCII adjust family

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

Prerequisite: G62b2 explicitly approved and fresh ranking confirms the family.

Branch: `topic/m62c-upd9002-bcd-adjust`

Commit prefix: `M62c:`

Gate: `G62c`

## Goal

Correct AAS and only evidence-confirmed residual AAA/DAA/DAS behavior.

Before treating any form as green, verify classification, selected count, executed count, and exact
pass set. Implement result adjustment, AF/CF, defined/materialized flags, and branch conditions from
aggregate SST evidence. If root causes are independent, stop and split before semantic editing.

Do not modify D5 by analogy and do not combine packed-BCD rotate or mode-transition work.

## Gate G62c

Every included adjust family is green; excluded green/non-applicable forms retain exact results.
Run all target-correct profiles and ratchet gates.

Write `docs/agents/reports/m62c_upd9002_bcd_adjust.md` and stop.
