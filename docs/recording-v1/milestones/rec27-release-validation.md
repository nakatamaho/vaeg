# REC27 — Final cross-platform validation and release handoff

Dependencies: **REC14, REC16, REC18, REC20, REC22, REC23, REC24, REC25, REC26**, all at their required passed gates.
Execution platform: All supported release platforms/presenter runtimes.
Gate: **HUMAN**. Exactly one milestone per session.
Repository M ID: resolve from `../id-map.md`; never invent or reuse a reserved ID.

## Read first

Read root/scoped AGENTS, current repository conventions, `../spec.md`,
`../workflow.md`, `../status.md`, and `../handoff.md`. The following paths are
relative to `docs/recording-v1/`:

- `spec.md`
- `acceptance.md`
- `platform-matrix.md`
- `workflow.md`

## Result at this checkpoint

A truthful release handoff proving Native and every supported Displayed backend, without pretending unavailable tests ran.

## Bounded implementation instructions

1. Review every specification section against implementation and all milestone reports. Close the platform/source/OSD/audio/lifecycle matrix with actual evidence.

2. Run the final public synthetic fixture suite using the release candidate: decoded RGB/PCM equality, frame/sample counts, common epoch, quantized timestamps, seek and failure paths.

3. Execute actual VAEG Native and Displayed demonstrations on Windows D3D11/SDL, Linux OpenGL/SDL and macOS Metal/SDL where each is supported. Include motion/audio/OSD and CRT for native GPU paths.

4. Open the produced files in actual target-platform VLC; record version and video/audio/seek results. Do not substitute mobile VLC or another OS for required desktop coverage.

5. Run affected existing QA/screenshot/input/audio/state/repository checks. Validate exact evaluated and evidence commits and private-data exclusion.

6. Write the final release-readiness report and user guide. Any required missing runtime/human evidence leaves the milestone BLOCKED or IMPLEMENTED_UNVERIFIED with exact remaining actions, not a partial PASS.

## Acceptance

- All applicable C/V/A/M/T/N/D/L/U/P/R checks pass with real evaluated artifacts.
- No task marked PASS solely by compilation, a fake source, a code review, or another backend test.
- Maintainer accepts final human gate. Release publication/merging remains outside this prompt unless separately authorized.

Run the concrete current-checkout commands captured in `audit.md`, plus the
focused tests introduced by this task and affected existing tests. Add the
relevant CTest targets/labels when new tests are introduced. Record exact commands,
exit status, evaluated source/binary identity and public-safe evidence; do not
claim the illustrative test names in the specification existed before this task.

## Forbidden shortcuts

No dropping a platform from scope, invented test output, hidden OSD exclusion, synthetic-only release claim, automatic main merge or release publication.

Do not weaken the public source/OSD/timing contract to fit the implementation.
Do not change unrelated guest hardware, history, private assets or dependencies.

## Report and stop

Write `docs/agents/reports/recording-v1/rec27-report.md` using
`../report-template.md`. Update `../status.md` and `../handoff.md`. Use the mapped
legal M prefix for commits and the assigned topic branch under repository policy.
A machine gate requires actual successful tests. A human gate stays
`WAITING_HUMAN` until the maintainer explicitly accepts it. Unavailable required
platform/runtime evidence is `IMPLEMENTED_UNVERIFIED` or `BLOCKED`, not PASS.
Stop here. Do not start another REC task in this session.
