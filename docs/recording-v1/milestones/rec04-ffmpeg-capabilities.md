# REC04 — Optional FFmpeg discovery and runtime capability checks

Dependencies: **REC01**, all at their required passed gates.
Execution platform: Any host with a target-compatible FFmpeg development environment.
Gate: **MACHINE**. Exactly one milestone per session.
Repository M ID: resolve from `../id-map.md`; never invent or reuse a reserved ID.

## Read first

Read root/scoped AGENTS, current repository conventions, `../spec.md`,
`../workflow.md`, `../status.md`, and `../handoff.md`. The following paths are
relative to `docs/recording-v1/`:

- `ffmpeg-backend.md`
- `audit.md`

## Result at this checkpoint

A target-scoped optional backend dependency with explicit capability and version diagnostics.

## Bounded implementation instructions

1. Implement CMake discovery of libavformat/libavcodec/libavutil using the current toolchain conventions. Preserve cross-compilation roots and avoid host libraries in target builds.

2. Freeze a tested API/version baseline, initially considering FFmpeg 6.1 or newer, and record exact tested versions. Keep capability-query/version adaptation private.

3. Implement a small backend capability probe for FFV1, PCM S16LE, Matroska, compatible RGB format and required FFV1 options. Return stable structured failures.

4. Test the OFF build without headers/libraries, ON with complete libraries, and ON with deliberately missing requirements. A missing dependency should stop configuration with a useful message.

5. Record build configuration/license identity for the development libraries without claiming release-package compliance.

## Acceptance

- C01, C04, V04: enabled probe succeeds only with required capabilities; disabled executable has no FFmpeg dependency.
- Missing muxer/codec/pixel format and incompatible target-library tests fail for their own specific reasons.
- No optional runtime ffmpeg executable is discovered or launched by production code.

Run the concrete current-checkout commands captured in `audit.md`, plus the
focused tests introduced by this task and affected existing tests. Add the
relevant CTest targets/labels when new tests are introduced. Record exact commands,
exit status, evaluated source/binary identity and public-safe evidence; do not
claim the illustrative test names in the specification existed before this task.

## Forbidden shortcuts

No vendoring/binary download, GPL/nonfree release configuration, global link flags, or adding swscale/resampler just to make defaults work.

Do not weaken the public source/OSD/timing contract to fit the implementation.
Do not change unrelated guest hardware, history, private assets or dependencies.

## Report and stop

Write `docs/agents/reports/recording-v1/rec04-report.md` using
`../report-template.md`. Update `../status.md` and `../handoff.md`. Use the mapped
legal M prefix for commits and the assigned topic branch under repository policy.
A machine gate requires actual successful tests. A human gate stays
`WAITING_HUMAN` until the maintainer explicitly accepts it. Unavailable required
platform/runtime evidence is `IMPLEMENTED_UNVERIFIED` or `BLOCKED`, not PASS.
Stop here. Do not start another REC task in this session.
