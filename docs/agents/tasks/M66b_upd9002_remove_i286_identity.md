# M66b — Remove the remaining active I286/i286c implementation identity

## Mandatory preparation

Before doing any work:

1. Read `AGENTS.md`.
2. Read `docs/agents/ROADMAP.md` and `docs/agents/CONVENTIONS.md`.
3. Read `docs/agents/UPD9002_SEMANTICS_MIGRATION.md`.
4. Read this task and all reports from prerequisite gates.
5. Run `git status --short`; the tracked worktree must be clean.
6. Record the exact starting branch and SHA.
7. Resolve and verify the exact approved predecessor gate SHA. Do not infer it from the current
   worktree.
8. Work on this milestone only and stop at its gate.

All newly authored source, comments, identifiers, commit messages, test names, and repository
Documentation must be in English.

## Scheduling

Prerequisite: G66a explicitly approved and every production semantic family green.
If the roadmap renumbers this task, use the approved identifier consistently.

Branch: `topic/m66b-upd9002-remove-i286-identity`

Commit prefix: `M66b:`

Gate: `G66b`

## Goal

Rename and delete the final active production 286-derived identities without changing behavior.

## Required work

For production uPD9002 build sources only:

- rename remaining source basenames;
- rename declarations, definitions, macros, and dispatch targets;
- delete now-unreachable helpers and dead compatibility tables;
- update build lists and current documentation;
- use rename-only commits followed by reference-fix commits where repository conventions require it.

## Active-scope zero gate

No active production declaration, definition, dispatch target, source basename, or macro used by the
uPD9002 build may use `I286` or `i286c` identity.

The check must exclude historical/evidence paths, at minimum:

- `docs/agents/reports/**`
- `docs/agents/tasks/archive/**`
- `tools/qa/golden/**`
- historical legal/provenance documents

Do not edit excluded evidence merely to satisfy grep.

## Gate G66b

- Active-scope identity count is zero.
- Production dispatch graph and all architectural SST hashes are unchanged.
- Full build and system regression gates are green.
- Deleted helpers have proven zero reachability.

Write `docs/agents/reports/m66b_upd9002_remove_i286_identity.md` and stop.
