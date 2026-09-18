# REC15 — Final-client capture contract and SDL readback seam

Dependencies: **REC14**, all at their required passed gates.
Execution platform: Host with real SDL renderer output.
Gate: **MACHINE**. Exactly one milestone per session.
Repository M ID: resolve from `../id-map.md`; never invent or reuse a reserved ID.

## Read first

Read root/scoped AGENTS, current repository conventions, `../spec.md`,
`../workflow.md`, `../status.md`, and `../handoff.md`. The following paths are
relative to `docs/recording-v1/`:

- `display-osd.md`
- `architecture.md`
- `spec.md`

## Result at this checkpoint

A common Displayed capability/readback boundary and a correct final-client SDL acquisition primitive.

## Bounded implementation instructions

1. Add a small frontend capability/readback interface mapped to existing presenters. Identify actual final composition after GUI/status/OSD, not merely the guest texture.

2. Implement SDL readback on the renderer owner, before Present, with drawable-size, pitch, channel order and explicit failure handling.

3. Implement fixed-descriptor admission and basic stop signaling for size/generation change. Unsupported and dummy-only contexts must not masquerade as a real Displayed source.

4. Associate frame tokens with final rendering. Ensure capture does not call rendering twice or introduce timed movie frames for paused UI-only redraws.

5. Create a real-render-target synthetic fixture with distinct guest/OSD/menu/status regions; compare normalized acquisition bytes to independently populated target expectations.

## Acceptance

- D01, D02, D05, C02: correct composition order, target size, ownership and frame token.
- SDL readback test fails clearly on unsupported contexts; a dummy reconstruction is labelled as a fixture, not platform evidence.
- Capture inactive means no extra readback/allocation or change to rendering output.

Run the concrete current-checkout commands captured in `audit.md`, plus the
focused tests introduced by this task and affected existing tests. Add the
relevant CTest targets/labels when new tests are introduced. Record exact commands,
exit status, evaluated source/binary identity and public-safe evidence; do not
claim the illustrative test names in the specification existed before this task.

## Forbidden shortcuts

No operating-system capture API, cropped guest texture labelled Displayed, separate CRT pipeline, or async-performance claim without measurement.

Do not weaken the public source/OSD/timing contract to fit the implementation.
Do not change unrelated guest hardware, history, private assets or dependencies.

## Report and stop

Write `docs/agents/reports/recording-v1/rec15-report.md` using
`../report-template.md`. Update `../status.md` and `../handoff.md`. Use the mapped
legal M prefix for commits and the assigned topic branch under repository policy.
A machine gate requires actual successful tests. A human gate stays
`WAITING_HUMAN` until the maintainer explicitly accepts it. Unavailable required
platform/runtime evidence is `IMPLEMENTED_UNVERIFIED` or `BLOCKED`, not PASS.
Stop here. Do not start another REC task in this session.
