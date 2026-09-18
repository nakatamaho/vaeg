# Recording v1 handoff

Current state: WAITING_HUMAN. Completed task: REC00, mapped to legal repository
ID M100r1. Assigned branch: topic/rec00-recording-audit.

## REC00 completion

* Source audit and integration map are recorded in audit.md.
* The complete REC00 report is
  docs/agents/reports/recording-v1/rec00-report.md.
* The legal REC00-REC27 mapping is recorded in id-map.md as M100r1-M100r28.
* platform-matrix.md records the actual macOS baseline and keeps unavailable
  recording/backend evidence NOT_RUN.
* file-layout.md records the files created by this audit.
* No production source, C core interface, audio path, presenter, screenshot
  path, or existing QA path was changed.
* Existing unrelated untracked work was preserved and excluded.

## Checks and evidence boundary

The macos-macports configure/build, ROM-less self-test and smoke command,
milestone ID audit, case check, UTF-8 check and EOL check all completed
successfully. This is documentation/build baseline evidence only. It does not
prove FFV1/MKV output, PCM accounting, A/V sync, Native or Displayed readback,
VLC seekability, or Windows/Linux/native GPU behavior.

The unresolved REC00 risks are the existing audio callback look-ahead and
device-format ownership, presenter-specific final-client readback, safe-stop
descriptor changes, and bounded cancellation/backpressure. They are recorded
in audit.md and must be resolved by the corresponding later milestones.

## Human gate

REC00 remains pending the maintainer's explicit acceptance. Do not start REC01
or create its wrapper until that acceptance is recorded. The next eligible
task is REC01, mapped to M100r2, and it requires the exact goals/rec01.md
prompt plus a new milestone report and gate evidence.

At every later boundary retain exact source/evaluated/evidence commit identity,
commands and outcomes, public-safe evidence locations, private neutral case
IDs, unresolved risks and the actual human acceptance statement.
