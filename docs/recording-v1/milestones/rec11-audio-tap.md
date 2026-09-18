# REC11 — Guest-time recording audio producer and playback consumer

Dependencies: **REC10, REC03**, all at their required passed gates.
Execution platform: Host supporting audio integration and synthetic tests.
Gate: **MACHINE**. Exactly one milestone per session.
Repository M ID: resolve from `../id-map.md`; never invent or reuse a reserved ID.

## Read first

Read root/scoped AGENTS, current repository conventions, `../spec.md`,
`../workflow.md`, `../status.md`, and `../handoff.md`. The following paths are
relative to `docs/recording-v1/`:

- `timing-audio.md`
- `architecture.md`
- `acceptance.md`

## Result at this checkpoint

One guest-time producer supplies complete recording audio while the device callback only consumes playback data.

## Bounded implementation instructions

1. Implement recording-mode arming/handover using the REC10 cursor accounting, without replaying/skipping look-ahead or resetting sound hardware state.

2. Drive shared synthesis once from authoritative guest time using REC02 exact sample debt. Deliver owned final-format blocks with sample indices and the shared epoch.

3. Fan out to a distinct bounded playback FIFO; while recording the SDL callback only reads it or returns playback-only underrun silence. Device scheduling cannot change recorded counts.

4. Handle mute as counted output silence while synthesis advances. Separate mixer initialization from playback-device success if required by the audit; no-device recording must still work.

5. Transfer ownership safely back to legacy scheduling after Stop. Exercise repeated sessions and distinguish monitoring underrun/drop counters from forbidden recording drops.

6. Route recording backpressure to the coordinator safe point, not a sound lock or callback. Account for large guest-time advancement in bounded chunks.

## Acceptance

- A02-A04, T02, T04, T05: no-device/delayed callback/mute/faster production preserve expected sample count and bytes.
- Register/cursor traces show no double synthesis or entry/exit phase reset; legacy OFF regression passes.
- Callback has no encoder/file operations or wait for recording queue; lock-order stress does not deadlock.

Run the concrete current-checkout commands captured in `audit.md`, plus the
focused tests introduced by this task and affected existing tests. Add the
relevant CTest targets/labels when new tests are introduced. Record exact commands,
exit status, evaluated source/binary identity and public-safe evidence; do not
claim the illustrative test names in the specification existed before this task.

## Forbidden shortcuts

No device-clock timestamps, filling lost guest audio with zeros, second mixer/chip instance, hidden resampling, unbounded staging or guest-state change.

Do not weaken the public source/OSD/timing contract to fit the implementation.
Do not change unrelated guest hardware, history, private assets or dependencies.

## Report and stop

Write `docs/agents/reports/recording-v1/rec11-report.md` using
`../report-template.md`. Update `../status.md` and `../handoff.md`. Use the mapped
legal M prefix for commits and the assigned topic branch under repository policy.
A machine gate requires actual successful tests. A human gate stays
`WAITING_HUMAN` until the maintainer explicitly accepts it. Unavailable required
platform/runtime evidence is `IMPLEMENTED_UNVERIFIED` or `BLOCKED`, not PASS.
Stop here. Do not start another REC task in this session.
