# M73 - SCSI support cleanup and validation

M73 is reserved for SCSI support cleanup, validation, and VA integration.

Predecessor: approved G72.

Branch: `topic/m73-scsi-support`

Commit prefix: `M73:`

Candidate gate: `G73`

Report: `docs/agents/reports/m73_scsi_support.md`

Do not start M74. Do not merge M73 to `main` before G73 approval. Do not
declare G73 passed.

## Scope

M73 owns the active SCSI support path after M72 folds `SUPPORT_SCSI` to the
enabled side.

M73 must:

- audit `cbus/scsiio.c`, `cbus/scsicmd.c`, BIOS SxSI helpers, SDL2 media UI,
  configuration, save-state entries, and ROM-less tests;
- confirm which SCSI behavior is active VA-supported behavior;
- preserve SASI, HOSTFAT, and existing disk-image behavior;
- add or update focused validation for the active SCSI path where practical;
- document remaining hardware or guest-OS gaps.

## Non-goals

M73 must not:

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
results, and a G73 human-review checklist.
