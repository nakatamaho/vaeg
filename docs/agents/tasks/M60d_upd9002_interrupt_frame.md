# M60d — Verify or correct residual synchronous interrupt-frame semantics

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

Prerequisite: G60c explicitly approved and a fresh target-correct scoreboard reviewed.

Branch: `topic/m60d-upd9002-interrupt-frame`

Commit prefix: `M60d:`

Gate: `G60d`

## Conditional status

M60a already owns guest-visible FLAGS materialization, including any saved-FLAGS change made by its
live approved scope. M59 proved frame placement matched for its observed interrupt population.
Therefore first determine whether an independent synchronous-frame residual remains after G60c.

If CC/CD/taken-CE and the dependent BOUND frame-only population are green with no unexplained frame
signature, make no semantic edit. Produce an evidence-only closure report and stop at G60d.

## Goal if residuals remain

Correct only evidence-proven residuals in INT3, INT imm8, and taken INTO frame delivery:

- stack addresses/wrapping;
- saved CS/IP values;
- final SP;
- vector fetch/final CS:IP;
- TF/IF post-entry handling;
- event classification.

Do not revisit a green saved-FLAGS image by analogy.

## Scope restrictions

- No IRET change.
- No DIV/IDIV arithmetic.
- No BOUND range-decision change.
- No `6C-6F`, FPO2, decoder, timing, or prefetch work.
- Do not broaden scope merely to create a semantic commit.

## Gate

Run architectural CI/full and fingerprint profiles against exact G60c, enumerate all changed
hashes/signatures, and require no new failure. Verify CC/CD/CE and the M59 BOUND frame-only set
separately from BOUND range residuals.

Write `docs/agents/reports/m60d_upd9002_interrupt_frame.md` and stop.
