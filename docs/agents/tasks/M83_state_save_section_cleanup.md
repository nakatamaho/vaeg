# M83 - State-save section cleanup and compatibility report

M83 audits and cleans state-save sections after the VA-only source-tree
consolidation milestones have closed active dependencies.

Predecessor: approved G82.

Branch: `topic/m83-state-save-section-cleanup`

Commit prefix: `M83:`

Candidate gate: `G83`

Report: `docs/agents/reports/m83_state_save_section_cleanup.md`

Do not start M84. Do not merge M83 to `main` before G83 approval. Do not
declare G83 passed.

## Scope

M83 must:

- inventory every state-save section remaining from retired 98-only code;
- preserve required VA sections;
- define compatibility behavior for removed sections;
- preserve HOSTFAT identity checks and current VA save/load behavior;
- document any incompatible old-state rejection intentionally introduced by
  approved cleanup.

## Non-goals

M83 must not use state cleanup to hide behavior changes from earlier
milestones.

## Validation

Run save/load round-trip tests, incompatible-state negative tests where
required, repository checks, builds, native tests, and manual save/load gates.
