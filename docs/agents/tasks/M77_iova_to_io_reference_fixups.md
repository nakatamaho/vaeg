# M77 - Normalize references after the VA I/O move

M77 normalizes include paths, CMake entries, documentation references, and
repository-local path assumptions after M76 moves `iova/*` into `io/`.

Predecessor: approved G76.

Branch: `topic/m77-iova-io-reference-fixups`

Commit prefix: `M77:`

Candidate gate: `G77`

Report: `docs/agents/reports/m77_iova_io_reference_fixups.md`

Do not start M78. Do not merge M77 to `main` before G77 approval. Do not
declare G77 passed.

## Scope

M77 owns only reference normalization after the move:

- update include paths and build lists to the new `io/` locations;
- update current task/report references that describe active paths;
- leave approved historical reports intact except where repository policy
  requires a generated current-path view;
- preserve state-save section names and runtime behavior.

## Non-goals

M77 must not integrate the dispatcher, delete legacy devices, or rename
production symbols beyond path-derived include guards when required.

## Validation

Run repository invariant checks, normal builds, native tests, and VA boot
smoke/manual validation.
