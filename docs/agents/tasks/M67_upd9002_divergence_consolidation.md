# M67 — Consolidate divergences and hardware/authority questions

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

Prerequisite: G66b explicitly approved and the target-correct architectural full profile has zero
applicable failures.

Branch: `topic/m67-upd9002-divergence-consolidation`

Commit prefix: `M67:`

Gate: `G67`

## Goal

Publish the final minimal, content-addressed target inventory, approved divergences, and unresolved
hardware questions without production semantic change.

## Required review

- Verify every `expected_target_divergence` against exact primary target/hardware evidence.
- Verify every remaining `target_support_unverified` hash has exact hardware-pending coverage.
- Verify all `6C-6F` and `0F31/33/39/3B` records are exact documented-silicon-absent gaps and are
  never reported as passes.
- Verify `0F28` is applicable and passing.
- Reconcile active/reserved behavior for `6C-6F` with the final handler reachability decision.
- Consolidate BRKFEM/BRKEM, RETEM/CALLN, MD/Z80 mode, and FPO2 questions.
- State explicitly that generic FPO string absence is non-evidence.
- Preserve the historical G43 1,204 OUTS gain as V20 evidence only.

## Gate G67

- Zero target-correct applicable failures.
- No implementation-missing or unclassified record.
- Every target-absent and divergence entry is exact/evidence-backed.
- Unresolved FPO2/BRK/mode questions are minimal and explicit.
- No active I286/i286c identity remains and historical artifacts are unchanged.
- Report does not claim complete silicon validation.

Write `docs/agents/reports/m67_upd9002_divergence_consolidation.md` and stop.
