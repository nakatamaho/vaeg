# VAEG recording v1: implementation pack

Version: 1.0-osd-included. Prepared: 2026-09-09.

This is an implementation specification, not a claim that VAEG already records
movies. The actual checkout must be audited in REC00. The user's latest explicit
requirement is decisive: **visible emulator OSD is burned into both Native video
and Displayed video**. Earlier proposals excluding OSD from Native are superseded.

## The deliverable

Exactly two recording sources, one movie format, and one active recording:

| UI label | CLI value | Captured image |
|---|---|---|
| Native video | `native` | Completed guest image, before presentation effects, plus enabled guest-area OSD |
| Displayed video | `displayed` | Actual completed main-window client composition, including CRT, OSD, status, and in-client GUI |

Both produce a normal seekable `.mkv`: FFV1 v3, lossless 8-bit RGB, and PCM audio.
The movie is intended for VLC playback. Actual supported-platform VLC playback
is a release gate, not something inferred from codec names.

Native is not a rawvideo byte dump, and it is not a clean archival framebuffer:
OSD can cover guest pixels permanently. Existing guest-only screenshots and QA
capture retain their original contracts and remain available separately.

## Read and execute

Read the root `AGENTS.md`, applicable scoped instructions, and the repository's
current conventions first. Then read `spec.md`, `workflow.md`, `status.md`, and the
assigned file in `milestones/`. Read its additional reference files, not every
historical task. Start with `goals/rec00.md` or `codex-prompt.md`.

Run exactly one REC milestone per session. Each has a dependency list, bounded
changes, tests, forbidden shortcuts, and a stopping gate. REC IDs are feature-local
labels; REC00 maps them to the repository's legal M-prefixed IDs before commits.
Do not revive, revert, or execute old M99 work or any unrelated milestone.

The intended implementation setting is the user's selected 5.6 luna / xhigh.
This pack does not invent a model identifier or change model configuration.

## Important v1 limits

The timeline is emulator time, not desktop wall-clock time. A paused emulator
adds no movie duration. Fast-forward changes production speed, not movie speed.
Resolution is fixed within a file; an incompatible geometry change stops and
finalizes at the last valid boundary. There is no automatic segmentation, live
streaming, desktop capture, hardware video encoder, MP4, or optional OSD filter.
Synchronous Displayed readback is allowed initially; optimization follows proof.

All implementation instructions and code are English. New repository paths in
this pack are lowercase. Do not replace root agent or project specification files.
