# REC20 — OpenGL Displayed recording end to end

Dependencies: **REC19, REC13**, all at their required passed gates.
Execution platform: Linux with native VAEG OpenGL presenter and usable CRT runtime.
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

Actual Linux OpenGL CRT/client output records with OSD and synchronized PCM.

## Bounded implementation instructions

1. Connect the final OpenGL client target to the common recording coordinator after all GUI/status layers.

2. Keep one CRT evaluation per intended rendered frame and correct guest-token association while recording disables guest frame omission.

3. Validate native Displayed capability versus SDL fallback explicitly; do not silently change the selected renderer to pass tests.

4. Run real CRT-on/off Native/Displayed comparisons with sound, OSD, resize/minimize and presenter-stop handling.

5. Perform actual Linux VLC video/audio/seek checks and record backend/driver/runtime identity for synthetic evidence.

## Acceptance

- D01-D05, N04, T03, R03: real GL capture and playback verified.
- No upside-down frames, channel swap, altered following GL draws, missing UI or temporal double rendering.
- Human backend gate required; a compilation or isolated target test cannot close it.

Run the concrete current-checkout commands captured in `audit.md`, plus the
focused tests introduced by this task and affected existing tests. Add the
relevant CTest targets/labels when new tests are introduced. Record exact commands,
exit status, evaluated source/binary identity and public-safe evidence; do not
claim the illustrative test names in the specification existed before this task.

## Forbidden shortcuts

No defaulting to Native under Wayland/X11 readback trouble, OS portal capture, M99 redesign, or unsupported-platform PASS.

Do not weaken the public source/OSD/timing contract to fit the implementation.
Do not change unrelated guest hardware, history, private assets or dependencies.

## Report and stop

Write `docs/agents/reports/recording-v1/rec20-report.md` using
`../report-template.md`. Update `../status.md` and `../handoff.md`. Use the mapped
legal M prefix for commits and the assigned topic branch under repository policy.
A machine gate requires actual successful tests. A human gate stays
`WAITING_HUMAN` until the maintainer explicitly accepts it. Unavailable required
platform/runtime evidence is `IMPLEMENTED_UNVERIFIED` or `BLOCKED`, not PASS.
Stop here. Do not start another REC task in this session.
