# REC03 — Session state, complete-interval queue, and fake worker

Dependencies: **REC02**, all at their required passed gates.
Execution platform: Any development host.
Gate: **MACHINE**. Exactly one milestone per session.
Repository M ID: resolve from `../id-map.md`; never invent or reuse a reserved ID.

## Read first

Read root/scoped AGENTS, current repository conventions, `../spec.md`,
`../workflow.md`, `../status.md`, and `../handoff.md`. The following paths are
relative to `docs/recording-v1/`:

- `architecture.md`
- `timing-audio.md`
- `acceptance.md`

## Result at this checkpoint

A tested recorder coordinator and bounded worker queue operating entirely on synthetic owned slices.

## Bounded implementation instructions

1. Implement lifecycle transitions and complete CaptureSlice admission with an explicit byte/count budget. Include the active interval and readback staging reservation in capacity calculations.

2. Use a fake writer boundary and a worker that owns its sink. Implement producer close, drain, failure notification and cancellation wakeups. No FFmpeg or live rendering yet.

3. Expose nonblocking admission/WouldBlock so live emulation can wait only at a safe scheduling point later. Do not lock the audio callback into the queue.

4. Validate sequence IDs, consistent descriptors, exact interval endpoints and contiguous audio ranges at admission.

5. Exercise Stop during Preparing, Armed, queue-full and Finalizing; repeated Stop; worker failure; and object teardown. Add bounded-pool ownership tests or equivalent sanitizer checks.

## Acceptance

- C02, C05, C06: mutate original producer buffers after submission and confirm queued data stays intact.
- A deliberately slow/failing sink produces no silent drop, unbounded memory or deadlock; cancellation wakes all waits.
- Invalid sequence/descriptor/sample-gap tests assert distinct intended error codes.

Run the concrete current-checkout commands captured in `audit.md`, plus the
focused tests introduced by this task and affected existing tests. Add the
relevant CTest targets/labels when new tests are introduced. Record exact commands,
exit status, evaluated source/binary identity and public-safe evidence; do not
claim the illustrative test names in the specification existed before this task.

## Forbidden shortcuts

No unbounded queue, detached worker, audio-thread waits, file I/O, UI polling loop, or fabricated success after failure.

Do not weaken the public source/OSD/timing contract to fit the implementation.
Do not change unrelated guest hardware, history, private assets or dependencies.

## Report and stop

Write `docs/agents/reports/recording-v1/rec03-report.md` using
`../report-template.md`. Update `../status.md` and `../handoff.md`. Use the mapped
legal M prefix for commits and the assigned topic branch under repository policy.
A machine gate requires actual successful tests. A human gate stays
`WAITING_HUMAN` until the maintainer explicitly accepts it. Unavailable required
platform/runtime evidence is `IMPLEMENTED_UNVERIFIED` or `BLOCKED`, not PASS.
Stop here. Do not start another REC task in this session.
