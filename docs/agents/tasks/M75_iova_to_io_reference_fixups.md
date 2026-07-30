# M75 - Normalize references after the VA I/O move

M75 normalizes include paths, CMake entries, documentation references, and
repository-local path assumptions after retired VA1 diagnostic investigation moves `iova/*` into `io/`.

Predecessor: approved G74.

Branch: `topic/m75-iova-io-reference-fixups`

Commit prefix: `M75:`

Candidate gate: `G75`

Report: `docs/agents/reports/m75_iova_io_reference_fixups.md`

Do not start M76. Do not merge M75 to `main` before G75 approval. Do not
declare G75 passed.

## Scope

M75 owns only reference normalization after the move:

- update include paths and build lists to the new `io/` locations;
- update current task/report references that describe active paths;
- leave approved historical reports intact except where repository policy
  requires a generated current-path view;
- preserve state-save section names and runtime behavior.

## Non-goals

M75 must not integrate the dispatcher, delete legacy devices, or rename
production symbols beyond path-derived include guards when required.

## Validation

Run repository invariant checks, normal builds, native tests, and VA boot
smoke/manual validation.
