# M80 - Audit VA BIOS reachability and remove 98-only BIOS handlers

M80 audits `bios/` and related VA BIOS hooks, then removes only BIOS handlers
that are proven 98-only and unreachable in the active VA product.

Predecessor: approved G79.

Branch: `topic/m80-va-bios-reachability-cleanup`

Commit prefix: `M80:`

Candidate gate: `G80`

Report: `docs/agents/reports/m80_va_bios_reachability_cleanup.md`

Do not start M81. Do not merge M80 to `main` before G80 approval. Do not
declare G80 passed.

## Scope

M80 must:

- audit BIOS entry points reached by PC-88VA boot, DOS, demo/game software,
  FDC, SASI/SCSI, HOSTFAT, display, sound, keyboard, mouse, and save/load;
- preserve VA-reachable BIOS hooks;
- remove only handlers proven 98-only and inactive;
- document any retained legacy-looking BIOS code with its active dependency.

## Non-goals

M80 must not change CPU instruction semantics, I/O dispatcher structure, or
FDC subsystem CPU organization.

## Validation

Run repository checks, normal builds, native tests, BIOS-focused smoke tests
where available, save/load checks, and manual VA boot/application gates.
