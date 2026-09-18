# Codex execution workflow

## W1. Authority and source inspection

Follow root/scoped AGENTS and repository conventions. This pack does not replace
them. The current user requirement overrides older recording discussions: OSD is
included in both sources. Read `spec.md` before accepting any old Native-without-
OSD proposal. Read current source before inventing symbols or a project layout.

REC00 is documentation/audit only. Inspect the assigned checkout, current branch,
dirty files, active build/presets/tests, existing roadmap and reserved IDs. Public
web source observations in `sources.md` are navigation aids, not the evaluated
commit. Record the real base SHA locally in the report and map actual integration
points. Do not search unrelated user directories or private assets to complete an
audit. Read `docs/88va/` only when present and relevant; hardware timing facts still
need a trace to the implemented scheduler, not a guessed refresh rate.

## W2. Integrate with existing M task discipline

The public repository requires legal M-prefixed commit IDs and one milestone per
session. REC labels are a feature-local ladder, not permission to break that rule.
REC00 allocates an unused legal M namespace according to the checked-out roadmap
and reservations. One possible mapping is an unused M base with legal `r1` through
`r28` suffixes, but use it only if local validators permit it and the namespace is
not already reserved. Store all concrete mappings in `id-map.md`.

Create lightweight assigned-task wrappers in the repository's established task
location/name convention only if its workflow requires them. Each points to the
single corresponding REC task and its gate; do not copy/diverge the whole spec.
Do not overwrite unrelated tasks or rewrite the whole roadmap. REC00's ID map and
integration decision are a human gate before implementation. Use the allocated
legal M ID in commits, with the REC label in the subject/body for navigation.

## W3. One milestone per session

Start from `status.md` and the requested `milestones/recNN-*.md`. Check dependencies
and available execution platform. Summarize the narrow scope, then implement only
that milestone. Do not start the next one after finishing, even if it looks easy.
Stop at its declared gate with evidence and a handoff. Do not execute old M99 tasks,
8087 work, ROM changes, unrelated cleanup, or repository-history reconstruction.

For a generic next-goal, choose the first dependency-satisfied unfinished task that
can actually run on the current host. Explain skips of platform-specific blocked
tasks. GPU branches depend on the shared contract, not on one another, so a Linux
session may make OpenGL progress while D3D11/Metal runtime evidence is pending.
Never turn unavailable tests into PASS or start a dependent task whose required
gate has not passed.

## W4. Milestone loop

1. Read current task and its named documents; inspect real call sites.
2. Add focused positive and negative tests for the contract being changed.
3. Implement the smallest coherent change; preserve unrelated formatting/headers.
4. Run the task's checks, affected existing tests, and current repository validators.
5. Repair failures within this scope. Split a demonstrably oversized milestone
   into named revision substeps in its report instead of adding unrelated scope.
6. Self-review lifetime, synchronization, arithmetic, source semantics, disabled
   behavior, error handling, licensing and private-data leakage.
7. Record evaluated commit, exact commands/exit codes, artifact identities, limits,
   then update `status.md` and `handoff.md` before the stopping gate.

Code, comments, commit subjects and new docs are English. Keep C99 core and C++17
frontend boundaries. New source headers follow repository policy. Do not weaken
checks, edit vendored dependencies, or change golden baselines merely to pass.

## W5. Honest status

`NOT_STARTED`, `IN_PROGRESS`, `FAIL`, `BLOCKED`, `IMPLEMENTED_UNVERIFIED`,
`WAITING_HUMAN`, and `PASS` are distinct. Machine gates can PASS only after actual
required checks succeed. Human gates remain WAITING_HUMAN until the maintainer
explicitly accepts the required demonstration. A report drafted by the agent is
not human approval. Record the actual approval message/date without inventing it.

Missing compiler/GPU/runtime/media access is a concrete limitation. Continue only
with independent authorized work in a later session; do not substitute synthetic
or other-backend evidence. REC27 is not complete until every required platform/
source gate is truly satisfied, or the user explicitly revises release scope.

## W6. Evidence and Git hygiene

Work only in the assigned checkout/worktree. Do not reset, clean, stash, overwrite,
force-push, rewrite main, delete history, or touch other worktrees. Preserve user
changes. Stop before editing an overlapping dirty file; record the conflict. Small
additive edits in unrelated files may proceed when repository policy permits.

Use one concern per commit, obey the existing branch/push rules, and push only the
assigned non-main topic branch when required and available. Never treat an unavailable
remote or rejected push as success. Run local checks first; do not use hosted CI as
an iterative debugger. No automatic merge, release publication, or history rewrite.

Reports live under `docs/agents/reports/recording-v1/` with lowercase names. Public
synthetic artifacts go under an approved untracked build/test directory; private
integration evidence stays in a maintainer-provided, Git-ignored location. Never
track private screenshots, traces, ROM/media names, absolute paths or their hashes.
Use neutral case IDs. Ensure public report references cannot expose private assets.

A source evaluation commit and an evidence-only commit may be separate. State both
accurately; do not manufacture self-referential commit hashes. Reuse expensive
evidence only when the source/binary/test-contract identities still match under
repository policy. Fixing unrelated old defects requires a separately scoped task.

## W7. End-of-session response

Report the REC ID and mapped M ID, changed files, exact evaluated SHA, check results,
artifact locations/digests allowed for publication, capability/platform limitations,
gate state, and the next eligible task. Say explicitly whether the result is
PASS, WAITING_HUMAN, IMPLEMENTED_UNVERIFIED, or BLOCKED. Never describe the entire
recording feature as complete because an encoder fixture or one OS works.
