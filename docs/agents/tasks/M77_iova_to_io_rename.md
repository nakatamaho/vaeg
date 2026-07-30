# M76 - Move VA I/O sources into io

M76 moves the VA I/O source files from `iova/` to `io/` with rename-only
semantics wherever possible.

Predecessor: approved G75.

Branch: `topic/m76-iova-to-io-rename`

Commit prefix: `M76:`

Candidate gate: `G76`

Report: `docs/agents/reports/m76_iova_to_io_rename.md`

Do not start M77. Do not merge M76 to `main` before G76 approval. Do not
declare G76 passed.

## Scope

M76 owns the path move only:

- move active `iova/*` sources and headers into `io/`;
- preserve file contents except for the minimum path comments needed to keep
  generated documentation coherent;
- do not rename device symbols or change public state-save section names;
- do not fold `iocoreva` into `iocore`;
- do not delete old 98-only `io/*` implementations.

## Rules

Use rename-only commits for file moves. Reference fixups, CMake source-list
updates, and include-path updates must be in separate commits.

## Validation

Run repository invariant checks, `git diff --check`, normal CMake configure
and build, native tests, M68/M69/M70/M71/M72 protected checks where available,
and a manual VA boot gate.

## Closure

The report must prove that the tree shape changed while behavior did not.
