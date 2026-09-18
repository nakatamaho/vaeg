# Feature-to-repository milestone map

Allocation state: REC01 COMPLETE; REC00 accepted on 2026-09-19. REC00 uses the
first legal, collision-free lettered namespace after the current M99 sequence.
The current roadmap, task files, reports and repository references contain no
M100 or M101 assignment. The milestone validator accepts M100r1 through
M100r28.

| Feature ID | Legal repository M ID | Assigned task-wrapper path |
|---|---|---|
| REC00 | M100r1 | docs/agents/tasks/M100r1_recording_v1_rec00.md |
| REC01 | M100r2 | docs/agents/tasks/M100r2_recording_v1_rec01.md |
| REC02 | M100r3 | Reserved; wrapper not created |
| REC03 | M100r4 | Reserved; wrapper not created |
| REC04 | M100r5 | Reserved; wrapper not created |
| REC05 | M100r6 | Reserved; wrapper not created |
| REC06 | M100r7 | Reserved; wrapper not created |
| REC07 | M100r8 | Reserved; wrapper not created |
| REC08 | M100r9 | Reserved; wrapper not created |
| REC09 | M100r10 | Reserved; wrapper not created |
| REC10 | M100r11 | Reserved; wrapper not created |
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
was explicitly accepted on 2026-09-19. REC01 uses M100r2 and is complete at its
machine gate; later milestones remain reserved until their dependencies and
gates are satisfied under workflow.md.

Allocation evidence:

* python3 tools/qa/milestone_ids.py --selftest --audit --discover passed.
* The discovery/audit run found no conflicting M100/M101 task, report,
  roadmap assignment or reference.
* The legal parser form permits the lowercase letter and nonzero decimal
  suffix used above.

Maintainer acceptance of REC00: explicitly accepted on 2026-09-19 with
authorization to proceed to REC01.
