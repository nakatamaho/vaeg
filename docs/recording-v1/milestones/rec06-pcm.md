# REC06 — PCM encoder and exact sample round trip

Dependencies: **REC04, REC02**, all at their required passed gates.
Execution platform: Host with enabled FFmpeg backend.
Gate: **MACHINE**. Exactly one milestone per session.
Repository M ID: resolve from `../id-map.md`; never invent or reuse a reserved ID.

## Read first

Read root/scoped AGENTS, current repository conventions, `../spec.md`,
`../workflow.md`, `../status.md`, and `../handoff.md`. The following paths are
relative to `docs/recording-v1/`:

- `ffmpeg-backend.md`
- `timing-audio.md`
- `acceptance.md`

## Result at this checkpoint

A production PCM encoder component preserving audited sample representation, channel order, count and PTS.

## Bounded implementation instructions

1. Implement PCM S16LE stereo at the audited effective output rate. If REC00 recorded an explicit format-contract adjustment, implement only that resolved canonical representation.

2. Accept variable-sized sample blocks with cumulative sample indices and declared origin offset. Do not derive sample counts from wall-clock arrival or video-frame count.

3. Reuse the existing final saturation/packing semantics through a tested helper where needed; do not use 32-bit internal accumulator bytes as if already S16LE.

4. Test independent channel ramps/impulses, exact silence, clipping boundaries, irregular block splits and final short blocks.

5. Exercise encoder failure and EOF cleanup and retain exact sample counts throughout. Do not integrate into the live sound callback yet.

## Acceptance

- A01, A02, T05: normalized decoded sample bytes/counts exactly match the independent input.
- Changing block partition or producer speed cannot alter content or cumulative PTS.
- Stereo order, byte order and final short-block duration are correct.

Run the concrete current-checkout commands captured in `audit.md`, plus the
focused tests introduced by this task and affected existing tests. Add the
relevant CTest targets/labels when new tests are introduced. Record exact commands,
exit status, evaluated source/binary identity and public-safe evidence; do not
claim the illustrative test names in the specification existed before this task.

## Forbidden shortcuts

No new audio resampler, chip clock/rate change, audio normalization, callback tap, fake silence to conceal missing samples, or extra audio format menu.

Do not weaken the public source/OSD/timing contract to fit the implementation.
Do not change unrelated guest hardware, history, private assets or dependencies.

## Report and stop

Write `docs/agents/reports/recording-v1/rec06-report.md` using
`../report-template.md`. Update `../status.md` and `../handoff.md`. Use the mapped
legal M prefix for commits and the assigned topic branch under repository policy.
A machine gate requires actual successful tests. A human gate stays
`WAITING_HUMAN` until the maintainer explicitly accepts it. Unavailable required
platform/runtime evidence is `IMPLEMENTED_UNVERIFIED` or `BLOCKED`, not PASS.
Stop here. Do not start another REC task in this session.
