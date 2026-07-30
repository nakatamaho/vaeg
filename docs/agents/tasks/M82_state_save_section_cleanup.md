# M82 - State-save section cleanup and compatibility report

M82 audits and cleans state-save sections after the VA-only source-tree
consolidation milestones have closed active dependencies.

Predecessor: approved G81.

Branch: `topic/m82-state-save-section-cleanup`

Commit prefix: `M82:`

Candidate gate: `G82`

Report: `docs/agents/reports/m82_state_save_section_cleanup.md`

Do not start M83. Do not merge M82 to `main` before G82 approval. Do not
declare G82 passed.

## Scope

M82 must:

- inventory every state-save section remaining from retired 98-only code;
- preserve required VA sections;
- define compatibility behavior for removed sections;
- preserve HOSTFAT identity checks and current VA save/load behavior;
- document any incompatible old-state rejection intentionally introduced by
  approved cleanup.

## Non-goals

M82 must not use state cleanup to hide behavior changes from earlier
milestones.

## Validation

Run save/load round-trip tests, incompatible-state negative tests where
required, repository checks, builds, native tests, and manual save/load gates.
