# REC14 — Minimal recording CLI and Native GUI

Dependencies: **REC13**, all at their required passed gates.
Execution platform: Host with a working GUI and Native recorder.
Gate: **HUMAN**. Exactly one milestone per session.
Repository M ID: resolve from `../id-map.md`; never invent or reuse a reserved ID.

## Read first

Read root/scoped AGENTS, current repository conventions, `../spec.md`,
`../workflow.md`, `../status.md`, and `../handoff.md`. The following paths are
relative to `docs/recording-v1/`:

- `spec.md`
- `acceptance.md`
- `workflow.md`

## Result at this checkpoint

A normal user can select a destination, start Native recording, stop, and play the result.

## Bounded implementation instructions

1. Add --record, --record-source and positive --record-frames parsing without reinterpreting existing CLI options. Native is the default; invalid/unavailable requests fail before creating output.

2. Add the source/save dialog with exact Native video / Displayed video labels. Displayed remains visibly unavailable until its active backend supports capture, with an explanatory reason.

3. Add Stop Recording and elapsed guest-time/finalizing/error feedback. Keep Stop reachable during pause and backpressure; do not expose an OSD checkbox.

4. Preserve screenshot/F12/menu/input behavior and existing UI conventions. Do not add a conflicting global shortcut without checking the current key map.

5. Test cancel, double start, file exists, invalid extension, Unicode path, startup recording, finite recording-frames and no-FFmpeg UI state.

## Acceptance

- U01-U03, C01, M03: exact choices/default/error flow and no overwrite.
- GUI and CLI use the same recording session implementation; finite CLI output has the specified completed interval count.
- Human starts/stops a real Native movie containing visible OSD and sound; gate remains pending until accepted.

Run the concrete current-checkout commands captured in `audit.md`, plus the
focused tests introduced by this task and affected existing tests. Add the
relevant CTest targets/labels when new tests are introduced. Record exact commands,
exit status, evaluated source/binary identity and public-safe evidence; do not
claim the illustrative test names in the specification existed before this task.

## Forbidden shortcuts

No MP4/codec options, third source, OSD filter, automatic output upload, or enabling unsupported Displayed by silently capturing Native.

Do not weaken the public source/OSD/timing contract to fit the implementation.
Do not change unrelated guest hardware, history, private assets or dependencies.

## Report and stop

Write `docs/agents/reports/recording-v1/rec14-report.md` using
`../report-template.md`. Update `../status.md` and `../handoff.md`. Use the mapped
legal M prefix for commits and the assigned topic branch under repository policy.
A machine gate requires actual successful tests. A human gate stays
`WAITING_HUMAN` until the maintainer explicitly accepts it. Unavailable required
platform/runtime evidence is `IMPLEMENTED_UNVERIFIED` or `BLOCKED`, not PASS.
Stop here. Do not start another REC task in this session.
