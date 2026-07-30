# M84 - State-save section cleanup and compatibility report

M84 audits and cleans state-save sections after the VA-only source-tree
consolidation milestones have closed active dependencies.

Predecessor: approved G83.

Branch: `topic/m84-state-save-section-cleanup`

Commit prefix: `M84:`

Candidate gate: `G84`

Report: `docs/agents/reports/m84_state_save_section_cleanup.md`

Do not start M85. Do not merge M84 to `main` before G84 approval. Do not
declare G84 passed.

## Scope

M84 must:

- inventory every state-save section remaining from retired 98-only code;
- preserve required VA sections;
- define compatibility behavior for removed sections;
- preserve HOSTFAT identity checks and current VA save/load behavior;
- document any incompatible old-state rejection intentionally introduced by
  approved cleanup.

## Non-goals

M84 must not use state cleanup to hide behavior changes from earlier
milestones.

## Validation

Run save/load round-trip tests, incompatible-state negative tests where
required, repository checks, builds, native tests, and manual save/load gates.
