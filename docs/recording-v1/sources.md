# Primary sources and verification boundaries

Checked publicly on 2026-09-09. These sources guide the design; they do not prove
which commit or dependency version exists in the user's assigned worktree.
REC00 records the actual evaluated checkout and REC04 the tested FFmpeg versions.
No research-paper or book claim is needed here; these are source code, official
API documentation, project policy, and product documentation. Do not invent DOIs
or ISBNs for API pages. No third-party code is supplied by this pack.

| ID | Primary source and limited use |
|---|---|
| P1 | VAEG `AGENTS.md`: C/C++ boundary, lowercase new-path rules, M-prefixed task/commit and human-gate discipline. |
| P2 | VAEG `CMakeLists.txt`: current frontend and SDL/native presenter source locations. |
| P3 | VAEG `sdl2/scrnmng.c`: canonical canvas/shadow, overlay/presentation/capture navigation. |
| P4 | VAEG `sdl2/soundmng.c`: S16 stereo SDL output and callback calling `sound_pcmlock`. |
| P5 | VAEG `sound/sound.c`: existing producer/buffer/clock and callback-related generation paths. |
| P6 | FFmpeg send/receive API: encoder draining, EAGAIN and ownership protocol. |
| P7 | SDL2 RenderReadPixels: render-target readback, pitch, pre-Present placement and cost warning. |
| P8 | Microsoft D3D11 CopyResource: resource-copy constraints and asynchronous GPU execution. |
| P9 | Khronos glReadPixels reference source: pixel-pack semantics and bottom-up row order. |
| P10 | Apple Metal command-buffer and attachment docs: completion and stored render output requirements. |
| P11 | FFmpeg muxing API: header, packet writing, interleaving and trailer lifecycle. |
| P12 | FFmpeg 8.0 Matroska encoder source: observed 1/1000 stream time base; re-query actual linked version. |
| P13 | FFmpeg official legal page: configuration-dependent licensing and distribution checklist. |
| P14 | VideoLAN features: Matroska/LPCM general playback; not proof of every FFV1/platform combination. |
| P15 | OpenAI Goals documentation: verifiable objective, constraints and stopping conditions. |
| P16 | FFmpeg encoder/pixel-format source: FFV1 RGB packed formats and v3 options; runtime capability checks still required. |

## Exact source locations

```text
P1 https://raw.githubusercontent.com/nakatamaho/vaeg/main/AGENTS.md
P2 https://raw.githubusercontent.com/nakatamaho/vaeg/main/CMakeLists.txt
P3 https://raw.githubusercontent.com/nakatamaho/vaeg/main/sdl2/scrnmng.c
P4 https://raw.githubusercontent.com/nakatamaho/vaeg/main/sdl2/soundmng.c
P5 https://raw.githubusercontent.com/nakatamaho/vaeg/main/sound/sound.c
P6 https://ffmpeg.org/doxygen/8.0/group__lavc__encdec.html
P7 https://wiki.libsdl.org/SDL2/SDL_RenderReadPixels
P8 https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11devicecontext-copyresource
P9 https://raw.githubusercontent.com/KhronosGroup/OpenGL-Refpages/main/gl4/glReadPixels.xml
P10 https://developer.apple.com/documentation/metal/mtlcommandbuffer/waituntilcompleted()
P10 https://developer.apple.com/documentation/metal/mtlrenderpassattachmentdescriptor/storeaction
P11 https://ffmpeg.org/doxygen/8.0/group__lavf__encoding.html
P12 https://ffmpeg.org/doxygen/8.0/matroskaenc_8c_source.html
P13 https://ffmpeg.org/legal.html
P14 https://images.videolan.org/vlc/features.html
P15 https://developers.openai.com/codex/use-cases/follow-goals/
P15 https://developers.openai.com/blog/run-long-horizon-tasks-with-codex
P16 https://raw.githubusercontent.com/FFmpeg/FFmpeg/n8.0/libavcodec/ffv1enc.c
P16 https://ffmpeg.org/doxygen/6.1/pixfmt_8h_source.html
```

The design does not assert universal VLC support from the generic features page.
Actual v1 files must pass the platform-specific playback gates. An exact codec
round-trip and a visually correct player image are different forms of evidence.
