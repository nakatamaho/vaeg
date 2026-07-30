# retired VA1 diagnostic investigation - SCSI support cleanup and validation

retired VA1 diagnostic investigation is reserved for SCSI support cleanup, validation, and VA integration.

Predecessor: approved G73.

Branch: `topic/retired-va1-diagnostic-investigation`

Commit prefix: `retired VA1 diagnostic investigation:`

Candidate gate: `G74`

Report: `docs/agents/reports/m74_scsi_support.md`

Do not start M75. Do not merge retired VA1 diagnostic investigation to `main` before G74 approval. Do not
declare G74 passed.

## Scope

retired VA1 diagnostic investigation owns the active SCSI support path after M72 folds `SUPPORT_SCSI` to the
enabled side.

retired VA1 diagnostic investigation must:

- audit `cbus/scsiio.c`, `cbus/scsicmd.c`, BIOS SxSI helpers, SDL2 media UI,
  configuration, save-state entries, and ROM-less tests;
- confirm which PC-9801-55-compatible SCSI board behavior is active
  VA-supported behavior;
- validate against the documented PC-88VA SCSI support disk flow in
  `docs/modernization/scsi-support.md`, including a driver-installed boot disk
  when available;
- preserve SASI, HOSTFAT, and existing disk-image behavior;
- add or update focused validation for the active SCSI path where practical;
- document remaining hardware or guest-OS gaps.

## Non-goals

retired VA1 diagnostic investigation must not:

- move `iova/` sources;
- delete 98-only `io/` devices;
- redesign state-save format outside the SCSI evidence needed by this task;
- remove SASI or HOSTFAT;
- start VA-only source tree consolidation.

## Validation

Run the repository invariant checks, the normal CMake build, available native
tests, and focused SCSI/SASI/HOSTFAT smoke checks. Record unavailable platform
checks with exact blocker details.

## Closure

The final report must include the audited SCSI dependency graph, retained and
removed code paths if any, validation commands and exit statuses, manual gate
results, and a G74 human-review checklist.
