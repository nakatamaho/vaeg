# REC26 — Enabled build flavor, dependency packaging, and notices

Dependencies: **REC04, REC24, REC25**, all at their required passed gates.
Execution platform: Available release-build hosts/toolchains; target runtime checks still required.
Gate: **HUMAN**. Exactly one milestone per session.
Repository M ID: resolve from `../id-map.md`; never invent or reuse a reserved ID.

## Read first

Read root/scoped AGENTS, current repository conventions, `../spec.md`,
`../workflow.md`, `../status.md`, and `../handoff.md`. The following paths are
relative to `docs/recording-v1/`:

- `ffmpeg-backend.md`
- `sources.md`
- `platform-matrix.md`

## Result at this checkpoint

Explicit recording-enabled artifacts and unchanged recording-OFF artifacts with reviewed dependency provenance.

## Bounded implementation instructions

1. Preserve current OFF release packaging. Add an explicit enabled flavor using minimal target-compatible FFmpeg libraries and existing repository build/release conventions.

2. Pin/record actual library build versions, configure flags, transitive dependencies, corresponding source and notices. Exclude GPL/nonfree components from the proposed released minimal configuration.

3. Use the default dynamic-link proposal only with the required distribution review; do not assume it preserves a standalone single executable. Any static-link alternative needs separate approval.

4. Inspect target DLL/dylib/so resolution and install/package paths. Run enabled artifacts without relying on a ffmpeg executable, and OFF artifacts without FFmpeg libraries.

5. Update build/user docs and platform matrix with actual target results. Cross-build evidence must stay separate from runtime GPU/player evidence. Do not publish a release or change project licensing.

## Acceptance

- C01, R01, R02: OFF preserved, ON works with intended libraries and accurate notices/source/configuration.
- Missing runtime library consequences are documented truthfully; no untested universal portability claim.
- Maintainer reviews distribution/license/artifact changes before the human gate can PASS.

Run the concrete current-checkout commands captured in `audit.md`, plus the
focused tests introduced by this task and affected existing tests. Add the
relevant CTest targets/labels when new tests are introduced. Record exact commands,
exit status, evaluated source/binary identity and public-safe evidence; do not
claim the illustrative test names in the specification existed before this task.

## Forbidden shortcuts

No ffmpeg binary vendoring, default distro GPL library silently shipped, automatic release/upload, license rewrite or breaking old packages without acknowledgement.

Do not weaken the public source/OSD/timing contract to fit the implementation.
Do not change unrelated guest hardware, history, private assets or dependencies.

## Report and stop

Write `docs/agents/reports/recording-v1/rec26-report.md` using
`../report-template.md`. Update `../status.md` and `../handoff.md`. Use the mapped
legal M prefix for commits and the assigned topic branch under repository policy.
A machine gate requires actual successful tests. A human gate stays
`WAITING_HUMAN` until the maintainer explicitly accepts it. Unavailable required
platform/runtime evidence is `IMPLEMENTED_UNVERIFIED` or `BLOCKED`, not PASS.
Stop here. Do not start another REC task in this session.
