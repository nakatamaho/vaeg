# REC21 — Metal stored-target readback

Dependencies: **REC15**, all at their required passed gates.
Execution platform: macOS with an actual Metal device/context.
Gate: **MACHINE**. Exactly one milestone per session.
Repository M ID: resolve from `../id-map.md`; never invent or reuse a reserved ID.

## Read first

Read root/scoped AGENTS, current repository conventions, `../spec.md`,
`../workflow.md`, `../status.md`, and `../handoff.md`. The following paths are
relative to `docs/recording-v1/`:

- `display-osd.md`
- `audit.md`

## Result at this checkpoint

A correct Metal completed-target readback primitive with safe resource/completion lifetime.

## Bounded implementation instructions

1. Inspect drawable/framebuffer-only constraints, final GUI composition, attachment store actions and supported target formats/storage modes.

2. Add a final-target readback path; where necessary render once into a shared final texture that is copied identically to presentation and capture, with parity tests.

3. Blit to suitable CPU-visible storage, using valid row alignment, explicit completion and required managed-resource synchronization on applicable targets.

4. Never wait on an uncommitted command buffer or read a private texture as ordinary CPU memory. Retain source/staging resources through completion and handle command failure.

5. Test synthetic corners, GUI/status region, geometry-generation change, nil drawable and failed completion. Keep .mm code within the existing frontend boundary.

## Acceptance

- D02, C02, D04: actual Metal target pixels and ownership/completion behavior verified.
- Store/discard and framebuffer-only constraints are handled deliberately, not assumed.
- Record Apple/Intel hardware used; untested hardware remains a declared coverage limit.

Run the concrete current-checkout commands captured in `audit.md`, plus the
focused tests introduced by this task and affected existing tests. Add the
relevant CTest targets/labels when new tests are introduced. Record exact commands,
exit status, evaluated source/binary identity and public-safe evidence; do not
claim the illustrative test names in the specification existed before this task.

## Forbidden shortcuts

No unchecked getBytes on private texture, CPU read before completion, double CRT render, global Metal rewrite, or macOS compile-only PASS.

Do not weaken the public source/OSD/timing contract to fit the implementation.
Do not change unrelated guest hardware, history, private assets or dependencies.

## Report and stop

Write `docs/agents/reports/recording-v1/rec21-report.md` using
`../report-template.md`. Update `../status.md` and `../handoff.md`. Use the mapped
legal M prefix for commits and the assigned topic branch under repository policy.
A machine gate requires actual successful tests. A human gate stays
`WAITING_HUMAN` until the maintainer explicitly accepts it. Unavailable required
platform/runtime evidence is `IMPLEMENTED_UNVERIFIED` or `BLOCKED`, not PASS.
Stop here. Do not start another REC task in this session.
