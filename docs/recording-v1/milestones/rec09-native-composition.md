# REC09 — Native frame copy and OSD compositor

Dependencies: **REC08, REC02**, all at their required passed gates.
Execution platform: Any host with the ROM-free frontend test harness.
Gate: **MACHINE**. Exactly one milestone per session.
Repository M ID: resolve from `../id-map.md`; never invent or reuse a reserved ID.

## Read first

Read root/scoped AGENTS, current repository conventions, `../spec.md`,
`../workflow.md`, `../status.md`, and `../handoff.md`. The following paths are
relative to `docs/recording-v1/`:

- `display-osd.md`
- `spec.md`
- `acceptance.md`

## Result at this checkpoint

A native-resolution recording frame containing the real canonical raster plus enabled OSD.

## Bounded implementation instructions

1. Implement owned canonical-frame copying with validated geometry, guard exclusion and established RGB conversion; do not change the producer buffer.

2. Implement or adapt the actual OSD primitive subset to a CPU/software destination at native resolution. Preserve semantic content/order/visibility and use native coordinate anchors.

3. Exclude outside host status/menu strips, GUI dialogs and CRT effects. Do not resize the guest raster merely to fit overlays.

4. Test every audited OSD class, edge clipping, alpha, unchanged guest image with changing OSD, no enabled OSD, and source row guards.

5. Expose this as a frontend helper, not a new screenshot mode; integrate live recording only after the video clock task.

## Acceptance

- N01, N02, V02, V03: expected post-OSD pixels correct; canonical source and existing QA snapshots byte-identical.
- No live GPU/window is required for this helper; ROM-free synthetic OSD tests pass.
- Native coded dimensions/SAR and exclusion of presentation effects are explicit in tests.

Run the concrete current-checkout commands captured in `audit.md`, plus the
focused tests introduced by this task and affected existing tests. Add the
relevant CTest targets/labels when new tests are introduced. Record exact commands,
exit status, evaluated source/binary identity and public-safe evidence; do not
claim the illustrative test names in the specification existed before this task.

## Forbidden shortcuts

No screenshot-downscale trick, canonical-buffer OSD writeback, hidden OSD omission, host-strip append, or new clean third source.

Do not weaken the public source/OSD/timing contract to fit the implementation.
Do not change unrelated guest hardware, history, private assets or dependencies.

## Report and stop

Write `docs/agents/reports/recording-v1/rec09-report.md` using
`../report-template.md`. Update `../status.md` and `../handoff.md`. Use the mapped
legal M prefix for commits and the assigned topic branch under repository policy.
A machine gate requires actual successful tests. A human gate stays
`WAITING_HUMAN` until the maintainer explicitly accepts it. Unavailable required
platform/runtime evidence is `IMPLEMENTED_UNVERIFIED` or `BLOCKED`, not PASS.
Stop here. Do not start another REC task in this session.
