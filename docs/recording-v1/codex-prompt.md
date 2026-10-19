# Initial Codex implementation prompt

Paste the block below as the initial message in the assigned VAEG checkout. It
starts with REC00 only. Use the existing selected 5.6 luna / xhigh setting; this
text does not assume a particular undocumented model CLI identifier.

```text
Implement VAEG recording v1 according to docs/recording-v1/, starting with REC00 only.

Read the repository AGENTS.md, applicable scoped instructions, current roadmap and
conventions first. Then read docs/recording-v1/spec.md, workflow.md, milestones.md,
status.md, handoff.md, and milestones/rec00-audit.md. Inspect the actual assigned
checkout; do not assume that public main or an earlier conversation matches it.

The frozen user-facing scope is:
- Recording source has exactly two choices: Native video and Displayed video.
- Both sources burn in all enabled guest-area emulator OSD. Do not add an OSD
  recording checkbox, hidden OSD filter, or clean third source.
- Native preserves the complete canonical guest raster before CRT/window scaling,
  composing OSD only on a recording-owned copy. Existing guest-only screenshots
  and QA captures must not change.
- Displayed captures the actual final main-window client composition, including
  active CRT, OSD, status and in-client GUI, but not OS decorations/desktop/dialogs.
- Output is a normal seekable local MKV with FFV1 v3 8-bit RGB and PCM only.
- Use an optional private libavformat/libavcodec/libavutil backend; no runtime
  ffmpeg subprocess and no FFmpeg types in C core interfaces.
- Use common guest time and exact sample accounting. The audio callback must not
  determine captured time or synthesize captured samples. Audit and reconcile
  existing audio look-ahead before implementing the recording producer.
- Lossless success permits host slowdown, not dropped frames/samples. Use bounded
  owned buffers and cancellable backpressure outside audio/core/GPU locks.
- V1 dimensions are fixed per file. Safely stop on incompatible geometry, clock,
  audio-format or presenter changes; do not add auto-segmentation or silent scaling.

For REC00, make only audit/documentation changes. Record baseline commands and
actual source integration points, enumerate every OSD/status layer, identify exact
video/audio clocks and ownership, and allocate collision-free legal repository M
IDs for feature labels REC00-REC27 in id-map.md. Preserve the repository's naming,
C99 core/C++17 frontend, task, commit and human-gate rules. Do not rewrite root
AGENTS.md, replay old M99 work, change other milestones, or rewrite Git history.

Write docs/agents/reports/recording-v1/rec00-report.md. Update audit.md, id-map.md,
platform-matrix.md, status.md and handoff.md with actual evidence and unresolved
issues. Do not publish private ROM/media names, paths, hashes, screenshots or logs.
Stop at the REC00 human gate. Do not implement REC01 in this session.

After REC00 is explicitly accepted, later sessions implement exactly one eligible
milestone using its goals/recNN.md prompt. Run the real specified checks and stop
at each declared gate. Report missing runtime/platform evidence as BLOCKED or
IMPLEMENTED_UNVERIFIED; never invent PASS or claim one backend proves another.
```
