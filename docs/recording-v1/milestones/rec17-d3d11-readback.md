# REC17 — D3D11 final-target readback

Dependencies: **REC15**, all at their required passed gates.
Execution platform: Windows with actual D3D11 context; WARP may prove this primitive only.
Gate: **MACHINE**. Exactly one milestone per session.
Repository M ID: resolve from `../id-map.md`; never invent or reuse a reserved ID.

## Read first

Read root/scoped AGENTS, current repository conventions, `../spec.md`,
`../workflow.md`, `../status.md`, and `../handoff.md`. The following paths are
relative to `docs/recording-v1/`:

- `display-osd.md`
- `audit.md`

## Result at this checkpoint

A correct owned-pixel readback primitive for VAEG native D3D11 presentation.

## Bounded implementation instructions

1. Inspect final target/backbuffer format, sample count and GUI composition ownership in the current D3D11 presenter.

2. Add staging allocation/copy/map/unmap on the owner thread; honor RowPitch and resolve multisampling if the supported real source requires it.

3. Normalize orientation/channel order without gamma or YUV conversion. Use source-generation identity to reject stale resized resources.

4. Handle allocation/map/copy/device-loss errors and release all resources. Keep capture-off path unchanged.

5. Add a Windows-target synthetic readback test with odd pitch, distinct corners and a separate GUI/status area. Keep the acquisition API independent of movie encoding.

## Acceptance

- D02, C02, D04: actual target bytes and failure/lifetime handling are correct.
- Record whether a real GPU or WARP supplied evidence; cross compilation alone is not PASS.
- Resource-generation/resize and device-loss negative tests fail explicitly without stale pixels.

Run the concrete current-checkout commands captured in `audit.md`, plus the
focused tests introduced by this task and affected existing tests. Add the
relevant CTest targets/labels when new tests are introduced. Record exact commands,
exit status, evaluated source/binary identity and public-safe evidence; do not
claim the illustrative test names in the specification existed before this task.

## Forbidden shortcuts

No Wine/SDL-only test claimed as native D3D11 proof, unowned mapped-pointer queue, or changing the renderer backend.

Do not weaken the public source/OSD/timing contract to fit the implementation.
Do not change unrelated guest hardware, history, private assets or dependencies.

## Report and stop

Write `docs/agents/reports/recording-v1/rec17-report.md` using
`../report-template.md`. Update `../status.md` and `../handoff.md`. Use the mapped
legal M prefix for commits and the assigned topic branch under repository policy.
A machine gate requires actual successful tests. A human gate stays
`WAITING_HUMAN` until the maintainer explicitly accepts it. Unavailable required
platform/runtime evidence is `IMPLEMENTED_UNVERIFIED` or `BLOCKED`, not PASS.
Stop here. Do not start another REC task in this session.
