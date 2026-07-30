# M78 - Audit VA BIOS reachability and remove 98-only BIOS handlers

M78 audits `bios/` and related VA BIOS hooks, then removes only BIOS handlers
that are proven 98-only and unreachable in the active VA product.

Predecessor: approved G77.

Branch: `topic/m78-va-bios-reachability-cleanup`

Commit prefix: `M78:`

Candidate gate: `G78`

Report: `docs/agents/reports/m78_va_bios_reachability_cleanup.md`

Do not start M79. Do not merge M78 to `main` before G78 approval. Do not
declare G78 passed.

## Scope

M78 must:

- audit BIOS entry points reached by PC-88VA boot, DOS, demo/game software,
  FDC, SASI/SCSI, HOSTFAT, display, sound, keyboard, mouse, and save/load;
- preserve VA-reachable BIOS hooks;
- remove only handlers proven 98-only and inactive;
- document any retained legacy-looking BIOS code with its active dependency.

## Non-goals

M78 must not change CPU instruction semantics, I/O dispatcher structure, or
FDC subsystem CPU organization.

## Validation

Run repository checks, normal builds, native tests, BIOS-focused smoke tests
where available, save/load checks, and manual VA boot/application gates.
