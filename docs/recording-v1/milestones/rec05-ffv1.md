# REC05 — FFV1 RGB encoder and exact pixel round trip

Dependencies: **REC04, REC02**, all at their required passed gates.
Execution platform: Host with enabled FFmpeg backend.
Gate: **MACHINE**. Exactly one milestone per session.
Repository M ID: resolve from `../id-map.md`; never invent or reuse a reserved ID.

## Read first

Read root/scoped AGENTS, current repository conventions, `../spec.md`,
`../workflow.md`, `../status.md`, and `../handoff.md`. The following paths are
relative to `docs/recording-v1/`:

- `ffmpeg-backend.md`
- `acceptance.md`

## Result at this checkpoint

A production FFV1 encoder component tested on synthetic owned RGB frames; no emulator tap yet.

## Bounded implementation instructions

1. Implement RAII/private cleanup for video codec context, frame and packet objects. Select verified packed RGB, v3, keyframe-every-frame and slice CRC settings.

2. Convert supported input layout by reversible byte repacking and audited RGB565 expansion only. Explicitly initialize ignored alpha/padding and respect source/destination stride.

3. Implement send/receive handling, EAGAIN draining and EOF flushing without dropping packets. Preserve frame sequence and time ledger associations.

4. Create deterministic nonblack fixtures: all channel corners, fine color edges, gradients, seeded noise, padded rows and odd/small dimensions.

5. Decode produced packets in a test harness or a minimal test container and compare normalized RGB24 against independent expected bytes. Test failures during allocation/open/send/receive/flush.

## Acceptance

- V01, V02, V03, V04: exact pixel equality and intended codec settings confirmed.
- FFV1 handles the fixture dimensions without invalid hardcoded slice count; no format fallback to YUV.
- Sanitizer or equivalent ownership checks cover normal and failed encoder cleanup.

Run the concrete current-checkout commands captured in `audit.md`, plus the
focused tests introduced by this task and affected existing tests. Add the
relevant CTest targets/labels when new tests are introduced. Record exact commands,
exit status, evaluated source/binary identity and public-safe evidence; do not
claim the illustrative test names in the specification existed before this task.

## Forbidden shortcuts

No H.264, RGB-to-YUV conversion, arbitrary quantization/resize, public encoder-option panel, or FFmpeg encode command as implementation.

Do not weaken the public source/OSD/timing contract to fit the implementation.
Do not change unrelated guest hardware, history, private assets or dependencies.

## Report and stop

Write `docs/agents/reports/recording-v1/rec05-report.md` using
`../report-template.md`. Update `../status.md` and `../handoff.md`. Use the mapped
legal M prefix for commits and the assigned topic branch under repository policy.
A machine gate requires actual successful tests. A human gate stays
`WAITING_HUMAN` until the maintainer explicitly accepts it. Unavailable required
platform/runtime evidence is `IMPLEMENTED_UNVERIFIED` or `BLOCKED`, not PASS.
Stop here. Do not start another REC task in this session.
