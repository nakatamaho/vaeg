# REC02 — Exact guest-time and audio sample arithmetic

Dependencies: **REC01**, all at their required passed gates.
Execution platform: Any development host.
Gate: **MACHINE**. Exactly one milestone per session.
Repository M ID: resolve from `../id-map.md`; never invent or reuse a reserved ID.

## Read first

Read root/scoped AGENTS, current repository conventions, `../spec.md`,
`../workflow.md`, `../status.md`, and `../handoff.md`. The following paths are
relative to `docs/recording-v1/`:

- `timing-audio.md`
- `acceptance.md`
- `audit.md`

## Result at this checkpoint

Independently tested rational timeline helpers, sample-grid slicing, and frame-interval bookkeeping.

## Bounded implementation instructions

1. Implement exact guest-time domain descriptors and checked conversion helpers using existing portable integer support. Add wrap-extension logic only for the actual audited clock representation.

2. Implement sample-grid ceil calculations, sub-sample origin offsets, cumulative frame/sample counts and interval partitioning. Keep host clocks out of the API.

3. Implement codec-time versus muxer-time distinction and quantization helpers using explicit absolute endpoints. Do not link FFmpeg just to test rational core arithmetic.

4. Add synthetic rational frame schedules, changing input block sizes, irregular arrival times and virtual multi-hour runs. Independently calculate expected counts with a different test method where practical.

5. Document the actual VA clock mapping separately from the synthetic fixture rate; do not infer actual hardware timing from test constants.

## Acceptance

- C03, T02, T05, A02: partition-invariant exact counts and bounded origin offset.
- No cumulative drift over virtual multi-hour schedules; overflow/wrap/zero-denominator cases fail predictably.
- Pause/faster producer invocation cannot change virtual timestamps.

Run the concrete current-checkout commands captured in `audit.md`, plus the
focused tests introduced by this task and affected existing tests. Add the
relevant CTest targets/labels when new tests are introduced. Record exact commands,
exit status, evaluated source/binary identity and public-safe evidence; do not
claim the illustrative test names in the specification existed before this task.

## Forbidden shortcuts

No floating-point accumulator, per-frame rounded audio lengths, guessed 60 Hz, or guest timing changes.

Do not weaken the public source/OSD/timing contract to fit the implementation.
Do not change unrelated guest hardware, history, private assets or dependencies.

## Report and stop

Write `docs/agents/reports/recording-v1/rec02-report.md` using
`../report-template.md`. Update `../status.md` and `../handoff.md`. Use the mapped
legal M prefix for commits and the assigned topic branch under repository policy.
A machine gate requires actual successful tests. A human gate stays
`WAITING_HUMAN` until the maintainer explicitly accepts it. Unavailable required
platform/runtime evidence is `IMPLEMENTED_UNVERIFIED` or `BLOCKED`, not PASS.
Stop here. Do not start another REC task in this session.
