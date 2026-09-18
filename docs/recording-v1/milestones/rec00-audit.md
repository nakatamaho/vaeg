# REC00 — Checkout audit, source map, and legal milestone allocation

Dependencies: **None**, all at their required passed gates.
Execution platform: Any development host.
Gate: **HUMAN**. Exactly one milestone per session.
Repository M ID: resolve from `../id-map.md`; never invent or reuse a reserved ID.

## Read first

Read root/scoped AGENTS, current repository conventions, `../spec.md`,
`../workflow.md`, `../status.md`, and `../handoff.md`. The following paths are
relative to `docs/recording-v1/`:

- `spec.md`
- `workflow.md`
- `sources.md`
- `file-layout.md`

## Result at this checkpoint

A reviewed, source-backed integration map and an allocated legal M-ID mapping; no production behavior changes.

## Bounded implementation instructions

1. Read root/scoped AGENTS, current roadmap/conventions and the actual assigned branch. Record the base SHA, relevant dirty files and baseline build/test commands. Never discard or stash user changes.

2. Fill audit.md with exact local file/symbol references for canonical guest composition, screenshot/QA capture, RGB conversion, every OSD class, status/menu layers, all active presenters, and actual drawable-size handling.

3. Trace audio from chip generation to mixer, saturation, sound_sync/sound_pcmlock equivalents and SDL callback. Record ownership, locks, look-ahead, buffer clipping, mute and no-device behavior. Identify the true final PCM rate/format and whether the S16 stereo contract matches.

4. Trace the guest video boundary and clock counters; distinguish fixed canvas geometry from guest display modes and physical monitor refresh. Document counter wraps and reset/state-load behavior. Do not use nominal frontend FPS.

5. Allocate all REC-to-M mappings using the current roadmap and actual naming validators. Record the reservation in id-map.md and add only the minimal task-wrapper/roadmap references required by current policy. Preserve existing unrelated IDs.

6. Fill platform-matrix.md with supported backend families and actual available test hosts. Mark unrun tests NOT_RUN. Fill file-layout mappings and concrete commands. Submit the audio-entry reconciliation and OSD-layer map for human review.

## Acceptance

- Existing relevant baseline build/tests and repository validators run, or each unavailable prerequisite is explicitly reported.
- No production code, shader, audio, framebuffer, saved-state, external dependency or history changes.
- The report identifies every unresolved integration issue, particularly audio look-ahead and any format mismatch; no invented symbols or PASS results.

Run the concrete current-checkout commands captured in `audit.md`, plus the
focused tests introduced by this task and affected existing tests. Add the
relevant CTest targets/labels when new tests are introduced. Record exact commands,
exit status, evaluated source/binary identity and public-safe evidence; do not
claim the illustrative test names in the specification existed before this task.

## Forbidden shortcuts

No implementation, FFmpeg installation, private-asset inventory, root AGENTS replacement, M99 replay, or M-ID guess committed as fact.

Do not weaken the public source/OSD/timing contract to fit the implementation.
Do not change unrelated guest hardware, history, private assets or dependencies.

## Report and stop

Write `docs/agents/reports/recording-v1/rec00-report.md` using
`../report-template.md`. Update `../status.md` and `../handoff.md`. Use the mapped
legal M prefix for commits and the assigned topic branch under repository policy.
A machine gate requires actual successful tests. A human gate stays
`WAITING_HUMAN` until the maintainer explicitly accepts it. Unavailable required
platform/runtime evidence is `IMPLEMENTED_UNVERIFIED` or `BLOCKED`, not PASS.
Stop here. Do not start another REC task in this session.
