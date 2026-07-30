# M79 - Audit VA BIOS reachability and remove 98-only BIOS handlers

M79 audits `bios/` and related VA BIOS hooks, then removes only BIOS handlers
that are proven 98-only and unreachable in the active VA product.

Predecessor: approved G78.

Branch: `topic/m79-va-bios-reachability-cleanup`

Commit prefix: `M79:`

Candidate gate: `G79`

Report: `docs/agents/reports/m79_va_bios_reachability_cleanup.md`

Do not start M80. Do not merge M79 to `main` before G79 approval. Do not
declare G79 passed.

## Scope

M79 must:

- audit BIOS entry points reached by PC-88VA boot, DOS, demo/game software,
  FDC, SASI/SCSI, HOSTFAT, display, sound, keyboard, mouse, and save/load;
- preserve VA-reachable BIOS hooks;
- remove only handlers proven 98-only and inactive;
- document any retained legacy-looking BIOS code with its active dependency.

## Non-goals

M79 must not change CPU instruction semantics, I/O dispatcher structure, or
FDC subsystem CPU organization.

## Validation

Run repository checks, normal builds, native tests, BIOS-focused smoke tests
where available, save/load checks, and manual VA boot/application gates.
