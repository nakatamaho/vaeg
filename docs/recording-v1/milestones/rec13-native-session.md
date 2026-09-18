# REC13 — Integrated Native A/V session and safe stop

Dependencies: **REC07, REC11, REC12**, all at their required passed gates.
Execution platform: Host with enabled backend and VAEG integration environment.
Gate: **HUMAN**. Exactly one milestone per session.
Repository M ID: resolve from `../id-map.md`; never invent or reuse a reserved ID.

## Read first

Read root/scoped AGENTS, current repository conventions, `../spec.md`,
`../workflow.md`, `../status.md`, and `../handoff.md`. The following paths are
relative to `docs/recording-v1/`:

- `spec.md`
- `timing-audio.md`
- `architecture.md`
- `acceptance.md`

## Result at this checkpoint

The actual VAEG Native path records guest video with OSD and synchronized audio into a complete MKV.

## Bounded implementation instructions

1. Connect the Native frame/audio taps, common-epoch coordinator, complete-slice queue and production writer.

2. Implement initial lifecycle protections now: safe Start/Stop/quit, descriptor invalidation, reset/state-load/clock/rate change stops. Later stress tasks expand coverage; do not postpone fundamental safety.

3. Ensure Stop during pause uses an already-complete boundary and never advances the guest. Drain accepted work and retain failure status/partial path.

4. Use a private test controller or established harness to start/stop sessions before the public UI is added. Verify no test-only path bypasses the production taps.

5. Generate a public ROM-free integration fixture where feasible, then execute a maintainer-approved local guest demonstration with actual motion, sound and enabled OSD. Keep private evidence private.

## Acceptance

- V01, A01, T03-T05, N01-N04, M01-M04, L01: both real paths and common cut are verified.
- Actual movie decodes with expected counts and opens with audio/seek in available VLC; synthetic fixture alone is insufficient.
- Required repository human demonstration and Native source semantics are accepted explicitly; otherwise WAITING_HUMAN.

Run the concrete current-checkout commands captured in `audit.md`, plus the
focused tests introduced by this task and affected existing tests. Add the
relevant CTest targets/labels when new tests are introduced. Record exact commands,
exit status, evaluated source/binary identity and public-safe evidence; do not
claim the illustrative test names in the specification existed before this task.

## Forbidden shortcuts

No loader-only/synthetic-only claim of full emulator recording, dummy audio, OSD omission, implicit success before trailer, or bypass of human gate.

Do not weaken the public source/OSD/timing contract to fit the implementation.
Do not change unrelated guest hardware, history, private assets or dependencies.

## Report and stop

Write `docs/agents/reports/recording-v1/rec13-report.md` using
`../report-template.md`. Update `../status.md` and `../handoff.md`. Use the mapped
legal M prefix for commits and the assigned topic branch under repository policy.
A machine gate requires actual successful tests. A human gate stays
`WAITING_HUMAN` until the maintainer explicitly accepts it. Unavailable required
platform/runtime evidence is `IMPLEMENTED_UNVERIFIED` or `BLOCKED`, not PASS.
Stop here. Do not start another REC task in this session.
