# Resume with exactly one dependency-ready milestone

Use after the previous required gate has actually passed. This does not instruct
Codex to implement the entire ladder in one session. A named `goals/recNN.md` is
preferable when you already know the next task.

```text
/goal Complete exactly one next eligible milestone from docs/recording-v1/milestones.md. Read root/scoped AGENTS and current conventions, then docs/recording-v1/spec.md, workflow.md, status.md, handoff.md and id-map.md. Verify actual dependency and human gates; select the first unfinished dependency-satisfied task executable on this host, explaining any unavailable platform-specific tasks. If no task is eligible, report the exact blocker without inventing approval. Implement only the selected task and its named tests. Both Native video and Displayed video must include enabled guest-area OSD; output remains MKV/FFV1/PCM only. Preserve canonical QA/screenshots, private data, C99 core boundaries and unrelated work. Record real commands/results and evaluated commits in its report, update status/handoff, and stop at its gate. Do not start a second milestone, drop platform requirements, rewrite history, or mark unavailable evidence PASS.
```
