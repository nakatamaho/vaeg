# REC22 — Metal Displayed recording end to end

Dependencies: **REC21, REC13**, all at their required passed gates.
Execution platform: macOS with native VAEG Metal presenter and usable CRT runtime.
Gate: **HUMAN**. Exactly one milestone per session.
Repository M ID: resolve from `../id-map.md`; never invent or reuse a reserved ID.

## Read first

Read root/scoped AGENTS, current repository conventions, `../spec.md`,
`../workflow.md`, `../status.md`, and `../handoff.md`. The following paths are
relative to `docs/recording-v1/`:

- `display-osd.md`
- `spec.md`
- `acceptance.md`

## Result at this checkpoint

Actual macOS Metal CRT/client output records with OSD and synchronized PCM.

## Bounded implementation instructions

1. Attach the proven readback path to the final native Metal GUI/OSD/status composition and the common session.

2. Use actual drawable pixel dimensions on Retina displays; avoid logical-point captures or a recorder-only image scale.

3. Confirm shared final-target parity where used and that temporal effects advance only through the actual presentation path.

4. Run real CRT-on/off captures, Native independence, pause/stop, monitor/DPI change and drawable-loss policy checks.

5. Validate playback/audio/seek using actual macOS VLC and the supported packaged runtime. Keep untested Intel/Apple configurations explicitly distinguished.

## Acceptance

- D01-D05, N04, T03, R03: real Metal integration verified.
- No missing menu/status area, stale readback, incomplete command-buffer data or retina-size error.
- Human platform gate required; missing Metal hardware/runtime remains IMPLEMENTED_UNVERIFIED.

Run the concrete current-checkout commands captured in `audit.md`, plus the
focused tests introduced by this task and affected existing tests. Add the
relevant CTest targets/labels when new tests are introduced. Record exact commands,
exit status, evaluated source/binary identity and public-safe evidence; do not
claim the illustrative test names in the specification existed before this task.

## Forbidden shortcuts

No ScreenCaptureKit/desktop permission, silent SDL fallback, new high-bit-depth downgrade, or release claim based on one unsupported configuration.

Do not weaken the public source/OSD/timing contract to fit the implementation.
Do not change unrelated guest hardware, history, private assets or dependencies.

## Report and stop

Write `docs/agents/reports/recording-v1/rec22-report.md` using
`../report-template.md`. Update `../status.md` and `../handoff.md`. Use the mapped
legal M prefix for commits and the assigned topic branch under repository policy.
A machine gate requires actual successful tests. A human gate stays
`WAITING_HUMAN` until the maintainer explicitly accepts it. Unavailable required
platform/runtime evidence is `IMPLEMENTED_UNVERIFIED` or `BLOCKED`, not PASS.
Stop here. Do not start another REC task in this session.
