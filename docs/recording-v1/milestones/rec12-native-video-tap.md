# REC12 — Guest-boundary Native video tap

Dependencies: **REC09, REC02**, all at their required passed gates.
Execution platform: Host supporting frontend/core integration tests.
Gate: **MACHINE**. Exactly one milestone per session.
Repository M ID: resolve from `../id-map.md`; never invent or reuse a reserved ID.

## Read first

Read root/scoped AGENTS, current repository conventions, `../spec.md`,
`../workflow.md`, `../status.md`, and `../handoff.md`. The following paths are
relative to `docs/recording-v1/`:

- `timing-audio.md`
- `display-osd.md`
- `audit.md`

## Result at this checkpoint

Native images and OSD snapshots are emitted once per actual complete guest frame with correct guest timestamps.

## Bounded implementation instructions

1. Add a narrow completed-frame hook at the audited boundary. Snapshot/copy the canonical image only after all guest planes for that boundary are finalized.

2. Associate guest frame sequence/time and the matching OSD snapshot; preserve existing screenshot/debug APIs and guest buffer lifetime.

3. While recording, request every needed guest composition even when normal host frame skipping or dirty optimization would omit it. Do not force extra emulation instructions or use SDL Present as the clock.

4. Emit unchanged/blanked frames with their legitimate duration and updated OSD; respect existing field/interlace composition.

5. Test mode changes that preserve the descriptor and explicitly signal incompatible descriptor changes. Keep the disabled path cheap and allocation-free.

## Acceptance

- T01, N01-N04: correct boundary/sequence/OSD and no intermediate guest frame loss.
- Frame-skipping, paused/fast production and unchanged pixels do not corrupt timing or QA output.
- Source copy excludes guards/padding and is independent of the selected presentation backend.

Run the concrete current-checkout commands captured in `audit.md`, plus the
focused tests introduced by this task and affected existing tests. Add the
relevant CTest targets/labels when new tests are introduced. Record exact commands,
exit status, evaluated source/binary identity and public-safe evidence; do not
claim the illustrative test names in the specification existed before this task.

## Forbidden shortcuts

No guessed FPS/SDL ticks, forced window-size Native image, plane-only capture, QA semantics change, or rebuilding the VA rasterizer.

Do not weaken the public source/OSD/timing contract to fit the implementation.
Do not change unrelated guest hardware, history, private assets or dependencies.

## Report and stop

Write `docs/agents/reports/recording-v1/rec12-report.md` using
`../report-template.md`. Update `../status.md` and `../handoff.md`. Use the mapped
legal M prefix for commits and the assigned topic branch under repository policy.
A machine gate requires actual successful tests. A human gate stays
`WAITING_HUMAN` until the maintainer explicitly accepts it. Unavailable required
platform/runtime evidence is `IMPLEMENTED_UNVERIFIED` or `BLOCKED`, not PASS.
Stop here. Do not start another REC task in this session.
