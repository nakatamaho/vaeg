# REC18 — D3D11 Displayed recording end to end

Dependencies: **REC17, REC13**, all at their required passed gates.
Execution platform: Windows with native VAEG D3D11 presenter and usable CRT runtime.
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

Native D3D11 CRT/OSD/client composition records through the common FFV1/PCM session.

## Bounded implementation instructions

1. Attach readback after the final native D3D11 GUI/OSD/status composition and before swapchain presentation.

2. Reuse one rendered CRT result and preserve frame-token association; do not create a separate recorder shader chain.

3. Enable Displayed capability for this presenter only after successful preflight and geometry/format validation.

4. Run real CRT-on and CRT-off captures with audio and enabled OSD; prove complete-client regions, Native independence and safe stop before presenter replacement.

5. Exercise supported GPU runtime and playback in actual Windows VLC. Store only public synthetic or neutral private-case reports.

## Acceptance

- D01-D05, N04, T03, R03: native D3D11 production integration and playback verified.
- No color swap/flip, missing GUI layer, temporal double-step or silent SDL/Native substitution.
- Human actual-platform gate required; WARP primitive success alone is insufficient for final runtime coverage.

Run the concrete current-checkout commands captured in `audit.md`, plus the
focused tests introduced by this task and affected existing tests. Add the
relevant CTest targets/labels when new tests are introduced. Record exact commands,
exit status, evaluated source/binary identity and public-safe evidence; do not
claim the illustrative test names in the specification existed before this task.

## Forbidden shortcuts

No renderer reconstruction/M99 redo, separate capture renderer, missing-runtime PASS, or private image/hash publication.

Do not weaken the public source/OSD/timing contract to fit the implementation.
Do not change unrelated guest hardware, history, private assets or dependencies.

## Report and stop

Write `docs/agents/reports/recording-v1/rec18-report.md` using
`../report-template.md`. Update `../status.md` and `../handoff.md`. Use the mapped
legal M prefix for commits and the assigned topic branch under repository policy.
A machine gate requires actual successful tests. A human gate stays
`WAITING_HUMAN` until the maintainer explicitly accepts it. Unavailable required
platform/runtime evidence is `IMPLEMENTED_UNVERIFIED` or `BLOCKED`, not PASS.
Stop here. Do not start another REC task in this session.
