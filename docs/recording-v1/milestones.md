# Milestone ladder

There are 28 deliberately bounded tasks. REC00 is an audit, not implementation.
REC07 proves the recorder backend using synthetic inputs; REC13 proves actual
Native integration; REC16/18/20/22 prove the different Displayed paths. No earlier
checkpoint is a claim that the whole feature works.

The native GPU branches are independent after REC15. A host without D3D11 can
work on an eligible OpenGL task in a later session; missing D3D11 evidence still
blocks final release. Human gates require actual maintainer acceptance.

| Feature ID | Task | Dependencies | Gate |
|---|---|---|---|
| REC00 | [Checkout audit, source map, and legal milestone allocation](milestones/rec00-audit.md) | None | HUMAN |
| REC01 | [Recording contracts and compiled-OFF skeleton](milestones/rec01-contracts.md) | REC00 | MACHINE |
| REC02 | [Exact guest-time and audio sample arithmetic](milestones/rec02-clock.md) | REC01 | MACHINE |
| REC03 | [Session state, complete-interval queue, and fake worker](milestones/rec03-queue.md) | REC02 | MACHINE |
| REC04 | [Optional FFmpeg discovery and runtime capability checks](milestones/rec04-ffmpeg-capabilities.md) | REC01 | MACHINE |
| REC05 | [FFV1 RGB encoder and exact pixel round trip](milestones/rec05-ffv1.md) | REC04, REC02 | MACHINE |
| REC06 | [PCM encoder and exact sample round trip](milestones/rec06-pcm.md) | REC04, REC02 | MACHINE |
| REC07 | [Seekable Matroska writer and combined A/V fixture](milestones/rec07-matroska.md) | REC03, REC05, REC06 | MACHINE |
| REC08 | [OSD layer inventory and immutable snapshot extraction](milestones/rec08-osd-snapshot.md) | REC01 | MACHINE |
| REC09 | [Native frame copy and OSD compositor](milestones/rec09-native-composition.md) | REC08, REC02 | MACHINE |
| REC10 | [Audio production seam with legacy behavior preserved](milestones/rec10-audio-seam.md) | REC01, REC02 | MACHINE |
| REC11 | [Guest-time recording audio producer and playback consumer](milestones/rec11-audio-tap.md) | REC10, REC03 | MACHINE |
| REC12 | [Guest-boundary Native video tap](milestones/rec12-native-video-tap.md) | REC09, REC02 | MACHINE |
| REC13 | [Integrated Native A/V session and safe stop](milestones/rec13-native-session.md) | REC07, REC11, REC12 | HUMAN |
| REC14 | [Minimal recording CLI and Native GUI](milestones/rec14-native-ui.md) | REC13 | HUMAN |
| REC15 | [Final-client capture contract and SDL readback seam](milestones/rec15-displayed-contract.md) | REC14 | MACHINE |
| REC16 | [SDL Displayed recording end to end](milestones/rec16-displayed-sdl.md) | REC15 | HUMAN |
| REC17 | [D3D11 final-target readback](milestones/rec17-d3d11-readback.md) | REC15 | MACHINE |
| REC18 | [D3D11 Displayed recording end to end](milestones/rec18-displayed-d3d11.md) | REC17, REC13 | HUMAN |
| REC19 | [OpenGL final-framebuffer readback](milestones/rec19-opengl-readback.md) | REC15 | MACHINE |
| REC20 | [OpenGL Displayed recording end to end](milestones/rec20-displayed-opengl.md) | REC19, REC13 | HUMAN |
| REC21 | [Metal stored-target readback](milestones/rec21-metal-readback.md) | REC15 | MACHINE |
| REC22 | [Metal Displayed recording end to end](milestones/rec22-displayed-metal.md) | REC21, REC13 | HUMAN |
| REC23 | [Lifecycle, geometry, and state-operation hardening](milestones/rec23-lifecycle.md) | REC14, REC16 | MACHINE |
| REC24 | [Final source-selection UI and diagnostics](milestones/rec24-final-ui.md) | REC16, REC23 | HUMAN |
| REC25 | [Fault injection, boundedness, and measured performance](milestones/rec25-faults-performance.md) | REC07, REC13, REC23 | MACHINE |
| REC26 | [Enabled build flavor, dependency packaging, and notices](milestones/rec26-packaging.md) | REC04, REC24, REC25 | HUMAN |
| REC27 | [Final cross-platform validation and release handoff](milestones/rec27-release-validation.md) | REC14, REC16, REC18, REC20, REC22, REC23, REC24, REC25, REC26 | HUMAN |

## Progress groups

- REC00-REC07: audit, contracts, exact clocks, bounded queue, optional FFmpeg and a real synthetic A/V movie.
- REC08-REC14: OSD preservation, Native pixels, audio ownership separation, actual Native recording and minimal UI.
- REC15-REC22: final-client contract and separate SDL/D3D11/OpenGL/Metal acquisition/integration proofs.
- REC23-REC27: lifecycle, final interface, faults/performance, explicit packaging and full release evidence.

A task may add only the small prerequisites genuinely necessary within its stated
scope. If a local architectural surprise makes it substantially larger, record a
revision subplan inside that task and preserve its original completion gate. Do
not silently merge multiple REC tasks or skip their tests.
