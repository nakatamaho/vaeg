# REC23 — Lifecycle, geometry, and state-operation hardening

Dependencies: **REC14, REC16**, all at their required passed gates.
Execution platform: Any host with Native and at least one implemented Displayed path.
Gate: **MACHINE**. Exactly one milestone per session.
Repository M ID: resolve from `../id-map.md`; never invent or reuse a reserved ID.

## Read first

Read root/scoped AGENTS, current repository conventions, `../spec.md`,
`../workflow.md`, `../status.md`, and `../handoff.md`. The following paths are
relative to `docs/recording-v1/`:

- `spec.md`
- `architecture.md`
- `acceptance.md`

## Result at this checkpoint

Common controller policies are stress-tested across all available sources without weakening unavailable-platform gates.

## Bounded implementation instructions

1. Expand the already-present safe Start/Stop protections into a lifecycle matrix: immediate cancel, rapid repeated sessions, paused stop, queue pressure, exit and worker failure.

2. Route reset/state-load/rewind/emulated clock/sample-rate/presenter changes through safe recording finalization before the requested operation.

3. Test Native resize independence versus actual canonical descriptor changes. Test Displayed drawable/DPI/fullscreen/zero-size/minimize behavior and restoration of prior resizability.

4. Ensure state saves do not serialize recorder internals or private paths. Ordinary media changes with stable descriptors may continue.

5. Exercise concurrent error/Stop events and teardown ordering under sanitizers or the closest supported tooling. Apply generic fake-presenter tests plus actual available backend tests.

## Acceptance

- L01, L02, C05, C06, D04, M04: common cuts, no tail loss on normal Stop, no deadlocks/use-after-free.
- No successful file mixes dimensions/audio formats/presenters; source is immutable for the session.
- Unavailable GPU cells remain unverified; common fake-presenter coverage is labelled accurately.

Run the concrete current-checkout commands captured in `audit.md`, plus the
focused tests introduced by this task and affected existing tests. Add the
relevant CTest targets/labels when new tests are introduced. Record exact commands,
exit status, evaluated source/binary identity and public-safe evidence; do not
claim the illustrative test names in the specification existed before this task.

## Forbidden shortcuts

No auto-segmentation, resize resampling, recorder state in save files, silent restart, or postponing critical failures as cosmetic UI work.

Do not weaken the public source/OSD/timing contract to fit the implementation.
Do not change unrelated guest hardware, history, private assets or dependencies.

## Report and stop

Write `docs/agents/reports/recording-v1/rec23-report.md` using
`../report-template.md`. Update `../status.md` and `../handoff.md`. Use the mapped
legal M prefix for commits and the assigned topic branch under repository policy.
A machine gate requires actual successful tests. A human gate stays
`WAITING_HUMAN` until the maintainer explicitly accepts it. Unavailable required
platform/runtime evidence is `IMPLEMENTED_UNVERIFIED` or `BLOCKED`, not PASS.
Stop here. Do not start another REC task in this session.
