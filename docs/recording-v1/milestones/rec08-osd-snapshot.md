# REC08 — OSD layer inventory and immutable snapshot extraction

Dependencies: **REC01**, all at their required passed gates.
Execution platform: Host supporting the existing frontend tests.
Gate: **MACHINE**. Exactly one milestone per session.
Repository M ID: resolve from `../id-map.md`; never invent or reuse a reserved ID.

## Read first

Read root/scoped AGENTS, current repository conventions, `../spec.md`,
`../workflow.md`, `../status.md`, and `../handoff.md`. The following paths are
relative to `docs/recording-v1/`:

- `display-osd.md`
- `spec.md`
- `audit.md`

## Result at this checkpoint

A complete true-OSD inventory and shared immutable semantic snapshot, without changing live rendering.

## Bounded implementation instructions

1. Use REC00 layer mapping to enumerate every enabled guest-area OSD path. Explicitly separate outside status/menu strips and native OS dialogs.

2. Extract snapshot acquisition from existing draw functions with minimal refactoring. Own copied strings/values and associate the snapshot with a guest frame token.

3. Keep existing live overlay drawing and visual order unchanged while consuming the snapshot or equivalent shared semantic model.

4. Add deterministic snapshots covering each existing OSD class, enabled/disabled states, text updates, transparency, anchors and clipping.

5. Document any live host-clock statistics as sampled values; do not mark them deterministic across runs. Reuse existing font assets without moving or replacing them.

## Acceptance

- N02: live rendering/canonical guest image and existing screenshots do not change from extraction.
- Every existing OSD class has an inventory row and a controlled snapshot test; none omitted silently.
- Snapshot survives source string/GUI object destruction and contains no borrowed transient pointers.

Run the concrete current-checkout commands captured in `audit.md`, plus the
focused tests introduced by this task and affected existing tests. Add the
relevant CTest targets/labels when new tests are introduced. Record exact commands,
exit status, evaluated source/binary identity and public-safe evidence; do not
claim the illustrative test names in the specification existed before this task.

## Forbidden shortcuts

No new OSD checkbox/filter, forced diagnostics, full ImGui rasterizer, private font-ROM use, or redefining all host UI as OSD.

Do not weaken the public source/OSD/timing contract to fit the implementation.
Do not change unrelated guest hardware, history, private assets or dependencies.

## Report and stop

Write `docs/agents/reports/recording-v1/rec08-report.md` using
`../report-template.md`. Update `../status.md` and `../handoff.md`. Use the mapped
legal M prefix for commits and the assigned topic branch under repository policy.
A machine gate requires actual successful tests. A human gate stays
`WAITING_HUMAN` until the maintainer explicitly accepts it. Unavailable required
platform/runtime evidence is `IMPLEMENTED_UNVERIFIED` or `BLOCKED`, not PASS.
Stop here. Do not start another REC task in this session.
