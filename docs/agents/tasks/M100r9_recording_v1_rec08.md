# M100r9 - REC08 recording-v1 OSD snapshot

Status: IN_PROGRESS
Feature: REC08
Scope: true guest-area OSD inventory and immutable semantic snapshot extraction

This wrapper assigns REC08 of recording-v1 to legal repository ID M100r9.
Read `docs/recording-v1/milestones/rec08-osd-snapshot.md` and its named
references together with the applicable root instructions before acting.

The implementation is limited to the two existing guest-area diagnostic OSD
classes, their owned frame-associated snapshot and focused deterministic tests.
It preserves the existing live drawing order and screenshot/QA paths. Host title
text, the in-client ImGui UI, native OS decorations and screenshot-only analysis
are not guest-area OSD and are not included in this snapshot. Native and
Displayed remain the only recording sources, both with enabled guest-area OSD.
The current authorized recording-v1 run may continue after machine evidence is
recorded; this does not waive later platform, human or release validation.
