# M76 - Normalize references after the VA I/O move

M76 normalizes include paths, CMake entries, documentation references, and
repository-local path assumptions after M75 moves `iova/*` into `io/`.

Predecessor: approved G75.

Branch: `topic/m76-iova-io-reference-fixups`

Commit prefix: `M76:`

Candidate gate: `G76`

Report: `docs/agents/reports/m76_iova_io_reference_fixups.md`

Do not start M77. Do not merge M76 to `main` before G76 approval. Do not
declare G76 passed.

## Scope

M76 owns only reference normalization after the move:

- update include paths and build lists to the new `io/` locations;
- update current task/report references that describe active paths;
- leave approved historical reports intact except where repository policy
  requires a generated current-path view;
- preserve state-save section names and runtime behavior.

## Non-goals

M76 must not integrate the dispatcher, delete legacy devices, or rename
production symbols beyond path-derived include guards when required.

## Validation

Run repository invariant checks, normal builds, native tests, and VA boot
smoke/manual validation.
