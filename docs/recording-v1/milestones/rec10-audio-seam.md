# REC10 — Audio production seam with legacy behavior preserved

Dependencies: **REC01, REC02**, all at their required passed gates.
Execution platform: Host supporting existing audio/core tests.
Gate: **MACHINE**. Exactly one milestone per session.
Repository M ID: resolve from `../id-map.md`; never invent or reuse a reserved ID.

## Read first

Read root/scoped AGENTS, current repository conventions, `../spec.md`,
`../workflow.md`, `../status.md`, and `../handoff.md`. The following paths are
relative to `docs/recording-v1/`:

- `timing-audio.md`
- `audit.md`
- `architecture.md`

## Result at this checkpoint

A minimal tested sound-generation/packing seam, with the recording-off legacy behavior unchanged.

## Bounded implementation instructions

1. Trace audited generation call sites again at current HEAD. Add sample-cursor/accounting observations needed for trustworthy handover, without changing scheduling yet.

2. Extract the smallest reusable C99 generation/packing operation from existing paths while preserving invocation order, sample lengths, saturation, callback behavior and chip state.

3. Add a controllable test harness for generator calls, reserved buffers, callback look-ahead, overflow/truncation boundaries and mute/device behavior.

4. Compare before/after legacy samples and sound state for the same scripted calls. Keep any known preexisting limitation documented rather than correcting unrelated synthesis in this refactor.

5. Prepare a concrete handover procedure for REC11 covering already-generated data and locks. If the cursor cannot be established, report the missing accounting and fix that seam before proceeding.

## Acceptance

- A04, L02: recording-OFF waveform, generator call sequence/state, saturation and buffer behavior match the baseline.
- The C-facing seam remains C99 and cannot throw; existing sound tests pass.
- A report describes cursor/lock ownership and entry/exit reconciliation with no unproven reset-to-zero shortcut.

Run the concrete current-checkout commands captured in `audit.md`, plus the
focused tests introduced by this task and affected existing tests. Add the
relevant CTest targets/labels when new tests are introduced. Record exact commands,
exit status, evaluated source/binary identity and public-safe evidence; do not
claim the illustrative test names in the specification existed before this task.

## Forbidden shortcuts

No simultaneous recording integration, new sample-rate algorithm, ymfm internals edit, host callback tap, or chip reset to make accounting easier.

Do not weaken the public source/OSD/timing contract to fit the implementation.
Do not change unrelated guest hardware, history, private assets or dependencies.

## Report and stop

Write `docs/agents/reports/recording-v1/rec10-report.md` using
`../report-template.md`. Update `../status.md` and `../handoff.md`. Use the mapped
legal M prefix for commits and the assigned topic branch under repository policy.
A machine gate requires actual successful tests. A human gate stays
`WAITING_HUMAN` until the maintainer explicitly accepts it. Unavailable required
platform/runtime evidence is `IMPLEMENTED_UNVERIFIED` or `BLOCKED`, not PASS.
Stop here. Do not start another REC task in this session.
