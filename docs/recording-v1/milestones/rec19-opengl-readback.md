# REC19 — OpenGL final-framebuffer readback

Dependencies: **REC15**, all at their required passed gates.
Execution platform: Linux with an actual current OpenGL context.
Gate: **MACHINE**. Exactly one milestone per session.
Repository M ID: resolve from `../id-map.md`; never invent or reuse a reserved ID.

## Read first

Read root/scoped AGENTS, current repository conventions, `../spec.md`,
`../workflow.md`, `../status.md`, and `../handoff.md`. The following paths are
relative to `docs/recording-v1/`:

- `display-osd.md`
- `audit.md`

## Result at this checkpoint

A state-safe, orientation-correct OpenGL final-framebuffer readback primitive.

## Bounded implementation instructions

1. Identify the final framebuffer/read buffer and owner context used by the native OpenGL presenter.

2. Implement synchronous readback before swap, saving/restoring modified read-FBO/read-buffer and pixel-pack state, including pack-buffer binding.

3. Handle bottom-up row order, pack alignment, stride and explicit pixel format without double gamma conversion.

4. Check context/capability/readback errors and reject stale geometry generations. Avoid changing global GL state seen by subsequent frames.

5. Add synthetic target tests with nondefault pack state, distinct corners, GUI/status area and error paths. No PBO ring is required initially.

## Acceptance

- D02, C02, D04: pixel equality, orientation/pitch and state restoration demonstrated on a real GL context.
- A bound pixel-pack buffer cannot turn a CPU pointer into an unintended offset.
- Off path and following normal render pass are unaffected by capture.

Run the concrete current-checkout commands captured in `audit.md`, plus the
focused tests introduced by this task and affected existing tests. Add the
relevant CTest targets/labels when new tests are introduced. Record exact commands,
exit status, evaluated source/binary identity and public-safe evidence; do not
claim the illustrative test names in the specification existed before this task.

## Forbidden shortcuts

No image flipping based on visual guessing, resetting global GL state indiscriminately, CPU fallback labelled native GL, or required PBO optimization.

Do not weaken the public source/OSD/timing contract to fit the implementation.
Do not change unrelated guest hardware, history, private assets or dependencies.

## Report and stop

Write `docs/agents/reports/recording-v1/rec19-report.md` using
`../report-template.md`. Update `../status.md` and `../handoff.md`. Use the mapped
legal M prefix for commits and the assigned topic branch under repository policy.
A machine gate requires actual successful tests. A human gate stays
`WAITING_HUMAN` until the maintainer explicitly accepts it. Unavailable required
platform/runtime evidence is `IMPLEMENTED_UNVERIFIED` or `BLOCKED`, not PASS.
Stop here. Do not start another REC task in this session.
