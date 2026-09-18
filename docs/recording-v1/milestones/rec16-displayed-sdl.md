# REC16 — SDL Displayed recording end to end

Dependencies: **REC15**, all at their required passed gates.
Execution platform: Host with real SDL fallback presentation.
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

The actual SDL fallback client, including OSD/status/GUI, records with audio into a playable MKV.

## Bounded implementation instructions

1. Connect the SDL final-client acquisition to the existing session coordinator/writer using the same guest frame tokens and audio epoch.

2. Enable Displayed selection only when the active SDL path can fulfill the contract. Keep capability tied to current active renderer and recording state.

3. Capture all in-client regions once, including popups when drawn. Confirm Native still excludes host strips while preserving true OSD.

4. Exercise visible-client size changes, pause and Stop. Apply the v1 descriptor-change policy and restore temporarily disabled UI state.

5. Run a real VAEG SDL demonstration with motion/audio/OSD plus a synthetic exact-byte acquisition test and playback/seek check.

## Acceptance

- D01-D05, T03, M01-M04, U01: actual complete client and synchronized audio in the file.
- Native and Displayed differ exactly in intended presentation/UI scope, not accidental missing OSD.
- Human review accepts the real SDL source; this does not grant PASS to native GPU backends.

Run the concrete current-checkout commands captured in `audit.md`, plus the
focused tests introduced by this task and affected existing tests. Add the
relevant CTest targets/labels when new tests are introduced. Record exact commands,
exit status, evaluated source/binary identity and public-safe evidence; do not
claim the illustrative test names in the specification existed before this task.

## Forbidden shortcuts

No SDL dummy evidence as real output, forced Native fallback, cropping out GUI/status, or source change during recording.

Do not weaken the public source/OSD/timing contract to fit the implementation.
Do not change unrelated guest hardware, history, private assets or dependencies.

## Report and stop

Write `docs/agents/reports/recording-v1/rec16-report.md` using
`../report-template.md`. Update `../status.md` and `../handoff.md`. Use the mapped
legal M prefix for commits and the assigned topic branch under repository policy.
A machine gate requires actual successful tests. A human gate stays
`WAITING_HUMAN` until the maintainer explicitly accepts it. Unavailable required
platform/runtime evidence is `IMPLEMENTED_UNVERIFIED` or `BLOCKED`, not PASS.
Stop here. Do not start another REC task in this session.
