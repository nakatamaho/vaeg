# REC25 — Fault injection, boundedness, and measured performance

Dependencies: **REC07, REC13, REC23**, all at their required passed gates.
Execution platform: Host with enabled recorder and available readback paths.
Gate: **MACHINE**. Exactly one milestone per session.
Repository M ID: resolve from `../id-map.md`; never invent or reuse a reserved ID.

## Read first

Read root/scoped AGENTS, current repository conventions, `../spec.md`,
`../workflow.md`, `../status.md`, and `../handoff.md`. The following paths are
relative to `docs/recording-v1/`:

- `architecture.md`
- `ffmpeg-backend.md`
- `acceptance.md`

## Result at this checkpoint

Data loss, deadlock, unsafe file handling and unbounded memory are actively tested, with honest performance measurements.

## Bounded implementation instructions

1. Add deterministic failure injection at allocation, encode/open/send/receive/flush, output create/write/seek/trailer/close/publication, and capture failure boundaries.

2. Run slow-writer, full-queue, concurrent Stop/quit, no-device and high-entropy RGB stress fixtures. Verify accepted versus written counts and error disposition.

3. Test exclusive-create/race handling and Unicode paths on available OSes; keep remaining target-specific checks in the matrix.

4. Measure capture conversion/readback, encoding, queue high-water mark and host throughput on native-sized and larger client fixtures. Report hardware and limitations, not a universal FPS promise.

5. Confirm recording-OFF runtime incurs no capture allocations, GPU reads, worker or audio-owner changes. Optimize only a measured issue without changing semantics; asynchronous rings remain optional follow-up work.

## Acceptance

- C05, C06, M02-M04, P01, P02: successful recordings lose no intervals/samples; failed ones never claim success.
- Every negative test begins from a passing fixture and asserts the intended stable error, not any arbitrary failure.
- Memory/queue bounds hold under slow encoding; cancellation/error wakes all waiting components.

Run the concrete current-checkout commands captured in `audit.md`, plus the
focused tests introduced by this task and affected existing tests. Add the
relevant CTest targets/labels when new tests are introduced. Record exact commands,
exit status, evaluated source/binary identity and public-safe evidence; do not
claim the illustrative test names in the specification existed before this task.

## Forbidden shortcuts

No queue growth, forced thread kill, silent frame drops, weakened checks, invalid black/silent fixtures, or major GPU optimization mixed into fault fixes.

Do not weaken the public source/OSD/timing contract to fit the implementation.
Do not change unrelated guest hardware, history, private assets or dependencies.

## Report and stop

Write `docs/agents/reports/recording-v1/rec25-report.md` using
`../report-template.md`. Update `../status.md` and `../handoff.md`. Use the mapped
legal M prefix for commits and the assigned topic branch under repository policy.
A machine gate requires actual successful tests. A human gate stays
`WAITING_HUMAN` until the maintainer explicitly accepts it. Unavailable required
platform/runtime evidence is `IMPLEMENTED_UNVERIFIED` or `BLOCKED`, not PASS.
Stop here. Do not start another REC task in this session.
