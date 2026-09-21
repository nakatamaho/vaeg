# Feature-to-repository milestone map

Allocation state: REC10 COMPLETE; REC00 accepted on 2026-09-19. REC00 uses the
first legal, collision-free lettered namespace after the current M99 sequence.
The current roadmap, task files, reports and repository references contain no
M100 or M101 assignment. The milestone validator accepts M100r1 through
M100r28.

| Feature ID | Legal repository M ID | Assigned task-wrapper path |
|---|---|---|
| REC00 | M100r1 | docs/agents/tasks/M100r1_recording_v1_rec00.md |
| REC01 | M100r2 | docs/agents/tasks/M100r2_recording_v1_rec01.md |
| REC02 | M100r3 | docs/agents/tasks/M100r3_recording_v1_rec02.md |
| REC03 | M100r4 | docs/agents/tasks/M100r4_recording_v1_rec03.md |
| REC04 | M100r5 | docs/agents/tasks/M100r5_recording_v1_rec04.md |
| REC05 | M100r6 | docs/agents/tasks/M100r6_recording_v1_rec05.md |
| REC06 | M100r7 | docs/agents/tasks/M100r7_recording_v1_rec06.md |
| REC07 | M100r8 | docs/agents/tasks/M100r8_recording_v1_rec07.md |
| REC08 | M100r9 | docs/agents/tasks/M100r9_recording_v1_rec08.md |
| REC09 | M100r10 | docs/agents/tasks/M100r10_recording_v1_rec09.md |
| REC10 | M100r11 | docs/agents/tasks/M100r11_recording_v1_rec10.md |
| REC11 | M100r12 | Reserved; wrapper not created |
| REC12 | M100r13 | Reserved; wrapper not created |
| REC13 | M100r14 | Reserved; wrapper not created |
| REC14 | M100r15 | Reserved; wrapper not created |
| REC15 | M100r16 | Reserved; wrapper not created |
| REC16 | M100r17 | Reserved; wrapper not created |
| REC17 | M100r18 | Reserved; wrapper not created |
| REC18 | M100r19 | Reserved; wrapper not created |
| REC19 | M100r20 | Reserved; wrapper not created |
| REC20 | M100r21 | Reserved; wrapper not created |
| REC21 | M100r22 | Reserved; wrapper not created |
| REC22 | M100r23 | Reserved; wrapper not created |
| REC23 | M100r24 | Reserved; wrapper not created |
| REC24 | M100r25 | Reserved; wrapper not created |
| REC25 | M100r26 | Reserved; wrapper not created |
| REC26 | M100r27 | Reserved; wrapper not created |
| REC27 | M100r28 | Reserved; wrapper not created |

The reservation is documentation-only for unfinished milestones and does not
register future roadmap milestones or authorize future implementation. REC00
was explicitly accepted on 2026-09-19. REC01 uses M100r2, REC02 uses M100r3 and
REC03 uses M100r4; all three are complete at their machine gates. Later
milestones remain reserved
until their dependencies and gates are satisfied under workflow.md. REC04 uses
M100r5 and is complete at its machine gate. REC05 uses M100r6 and is complete
at its machine gate. REC06 uses M100r7 and is complete at its machine gate.
REC07 uses M100r8 and is complete at its machine gate.
REC08 uses M100r9 and is complete at its machine gate. REC09 uses M100r10 and
is complete at its machine gate. REC10 uses M100r11 and is complete at its
machine gate.

Allocation evidence:

* python3 tools/qa/milestone_ids.py --selftest --audit --discover passed.
* The discovery/audit run found no conflicting M100/M101 task, report,
  roadmap assignment or reference.
* The legal parser form permits the lowercase letter and nonzero decimal
  suffix used above.

Maintainer acceptance of REC00: explicitly accepted on 2026-09-19 with
authorization to proceed to REC01. REC02 and REC03 machine evidence is
recorded in their reports; no human acceptance is claimed.
