# REC01 — Recording contracts and compiled-OFF skeleton

Dependencies: **REC00**, all at their required passed gates.
Execution platform: Any development host.
Gate: **MACHINE**. Exactly one milestone per session.
Repository M ID: resolve from `../id-map.md`; never invent or reuse a reserved ID.

## Read first

Read root/scoped AGENTS, current repository conventions, `../spec.md`,
`../workflow.md`, `../status.md`, and `../handoff.md`. The following paths are
relative to `docs/recording-v1/`:

- `architecture.md`
- `ffmpeg-backend.md`
- `audit.md`

## Result at this checkpoint

A small compile-tested C/C++ boundary and recording-OFF implementation with no FFmpeg dependency.

## Bounded implementation instructions

1. Add the private recording directory and conceptual descriptors/state interfaces only where the audit mapped them. Reuse equivalent existing types rather than multiplying frame abstractions.

2. Add VAEG_ENABLE_RECORDING default OFF and target-scoped build wiring. At this milestone ON may report that the backend is not yet installed; OFF must remain a complete supported build.

3. Implement an inert capability/status facade and owned descriptor validation helpers. Do not add live taps, worker execution, encoder headers, or UI commands yet.

4. Add a C99 compile test for the C-facing bridge and C++17 compile tests for private frontend interfaces. Define stable error enums without exposing C++ exceptions across C functions.

5. Check checked-size arithmetic and ownership assumptions before any implementation uses raw pointers.

## Acceptance

- C01, C03, C04: OFF configures/builds without FFmpeg; existing applicable tests still pass.
- Invalid dimensions/stride, zero rates, overflow and invalid source enum are rejected deterministically.
- No link references or included headers from FFmpeg when OFF.

Run the concrete current-checkout commands captured in `audit.md`, plus the
focused tests introduced by this task and affected existing tests. Add the
relevant CTest targets/labels when new tests are introduced. Record exact commands,
exit status, evaluated source/binary identity and public-safe evidence; do not
claim the illustrative test names in the specification existed before this task.

## Forbidden shortcuts

No new global compiler flags/standards, generic plugin framework, silent ON-to-OFF fallback, or dependency fetch.

Do not weaken the public source/OSD/timing contract to fit the implementation.
Do not change unrelated guest hardware, history, private assets or dependencies.

## Report and stop

Write `docs/agents/reports/recording-v1/rec01-report.md` using
`../report-template.md`. Update `../status.md` and `../handoff.md`. Use the mapped
legal M prefix for commits and the assigned topic branch under repository policy.
A machine gate requires actual successful tests. A human gate stays
`WAITING_HUMAN` until the maintainer explicitly accepts it. Unavailable required
platform/runtime evidence is `IMPLEMENTED_UNVERIFIED` or `BLOCKED`, not PASS.
Stop here. Do not start another REC task in this session.
