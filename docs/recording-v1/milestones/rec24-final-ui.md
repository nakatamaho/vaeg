# REC24 — Final source-selection UI and diagnostics

Dependencies: **REC16, REC23**, all at their required passed gates.
Execution platform: Host with GUI and at least SDL Displayed integration.
Gate: **HUMAN**. Exactly one milestone per session.
Repository M ID: resolve from `../id-map.md`; never invent or reuse a reserved ID.

## Read first

Read root/scoped AGENTS, current repository conventions, `../spec.md`,
`../workflow.md`, `../status.md`, and `../handoff.md`. The following paths are
relative to `docs/recording-v1/`:

- `spec.md`
- `acceptance.md`
- `platform-matrix.md`

## Result at this checkpoint

The final two-source interface exposes only truthful capabilities and safe controls.

## Bounded implementation instructions

1. Complete the exact Recording source / Native video / Displayed video UI, retaining native default and no OSD filter.

2. Bind availability to the currently active presenter, compiled backend and known format/geometry constraints. Each implemented native GPU path participates without a fallback disguise.

3. Show arming/recording/finalizing/error states, source, guest elapsed time, output path locally and useful failure reasons; avoid private data in tracked reports.

4. Ensure Start, Stop, cancellation, path selection and disabled geometry/presenter controls are coherent with lifecycle states. Do not lose stop access when menus pause emulation.

5. Update user-facing help for guest-time pause/fast-forward, OSD burn-in, fixed dimensions, normal MKV playback and optional dependency. Run Native/Displayed UI demonstrations.

## Acceptance

- U01-U03, L01, R03: correct labels, no hidden source/filter, truthful errors and working controls.
- Source changes during an active session are rejected or gated by Stop; no implicit recording replacement.
- Human interface review required; later GPU integrations must rerun relevant source-capability tests.

Run the concrete current-checkout commands captured in `audit.md`, plus the
focused tests introduced by this task and affected existing tests. Add the
relevant CTest targets/labels when new tests are introduced. Record exact commands,
exit status, evaluated source/binary identity and public-safe evidence; do not
claim the illustrative test names in the specification existed before this task.

## Forbidden shortcuts

No codec/bitrate/FPS panels, MP4 mode, new overlay watermark, offscreen export source, or claiming unavailable backends are supported.

Do not weaken the public source/OSD/timing contract to fit the implementation.
Do not change unrelated guest hardware, history, private assets or dependencies.

## Report and stop

Write `docs/agents/reports/recording-v1/rec24-report.md` using
`../report-template.md`. Update `../status.md` and `../handoff.md`. Use the mapped
legal M prefix for commits and the assigned topic branch under repository policy.
A machine gate requires actual successful tests. A human gate stays
`WAITING_HUMAN` until the maintainer explicitly accepts it. Unavailable required
platform/runtime evidence is `IMPLEMENTED_UNVERIFIED` or `BLOCKED`, not PASS.
Stop here. Do not start another REC task in this session.
