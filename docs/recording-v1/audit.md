# REC00 audit record

Status: WAITING_HUMAN. REC00 is complete as an audit/documentation change and
stops at the declared human gate. No recording production code was changed.

## Checkout and workflow

The audit was performed in the assigned checkout on branch
topic/rec00-recording-audit, from evaluated source commit
1544a1b8e19cc324c203a94c4e055361abb57d8b. Existing unrelated untracked work
was preserved and is not included in the REC00 commit. The applicable
repository instructions are the root AGENTS.md, docs/agents/ROADMAP.md, and
docs/agents/CONVENTIONS.md.

The current roadmap and discovered tasks contain no M100 or M101 assignment.
The repository milestone validator accepts the legal lettered form M100r1
through M100r28. REC00 reserves that range for this feature family; the
complete mapping is recorded in id-map.md. The roadmap was not rewritten to
register a future milestone.

The recording-v1 documentation package supplied under docs/recording-v1/ was
inspected as task input. REC00 adds only the audit/report/wrapper and updates
the package's audit state. No REC01 wrapper or implementation file was
created.

### Baseline environment and commands

The following baseline was run before the documentation-only changes:

    cmake --preset macos-macports
    cmake --build --preset macos-macports -j2
    SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/macos-macports/sdl2/vaeg --selftest --no-cfg --no-bkupmem
    SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/macos-macports/sdl2/vaeg --smoke --no-cfg --no-bkupmem
    python3 tools/qa/milestone_ids.py --selftest --audit --discover
    python3 tools/repo/check_case.py
    python3 tools/repo/check_encoding.py --expect utf8
    python3 tools/repo/check_eol.py --enforce

All commands exited successfully. The self-test reported
selftest: all tests passed; the smoke run completed with its expected
ROM-less missing-ROM warning. The milestone validator reported 49 strict
self-tests, tasks=141 reports=160 roadmap_rows=100, and no audit error.
Case, encoding and EOL validators reported zero findings/violations.

The available host was macOS arm64 with AppleClang 21, CMake 3.31.12,
MacPorts SDL2 2.32.10 and the macos-macports build preset. FFmpeg command
line tools were present for inspection, but no libav recording integration is
currently discovered in CMake. VLC was not available. Windows and Linux
runtime presenters were not available in this audit session. These facts are
environment evidence only; they are not recording acceptance.

## Video source map

### Canonical guest raster

The canonical guest surface is owned by sdl2/scrnmng.c:

* SCRNMNG_CANVAS_WIDTH and SCRNMNG_CANVAS_HEIGHT are 640 by 400.
* scrnmng_create allocates the shadow surface with a one-pixel left guard and
  a 16-bit pitch based on (width + guard) * 2.
* scrnmng_surflock exposes the shadow pointer, pitch, 16-bit format and active
  dimensions. The active pixel base used by upload/readback paths is the
  shadow pointer after the guard pixel.
* scrnmng_makepal16 and sdrawva.c establish the existing RGB565 palette
  expansion. The guest raster is composed by scrndrawva.c and driven by
  machine/pccore.c:drawscreenva.

The guest graphics layers (text, sprites, G0 and G1) are guest content and
belong in the canonical raster. They are not separate recorder OSD layers.
Native recording must copy this raster before CRT processing, window scaling,
letterboxing or host GUI composition. It must not reuse the existing
guest-only screenshot output in a way that changes screenshot/QA behavior.

### Frame and presenter boundaries

machine/pccore.c:drawscreenva is the completed guest drawing boundary. It
updates TSP timing, composes/rasterizes the guest image and increments
drawcount. sdl2/np2.c:run_guest_frame calls the guest execution and then enters
GUI/presenter work. scrnmng_framedisp_tick is host pacing, not a guest-time
source.

The SDL path is assembled in sdl2/scrnmng.c: upload to the SDL texture,
viewport/effect processing, overlays and SDL presentation. The native
presenter is owned by sdl2/librashader/native_presenter.h and its backend
bridges. D3D11 uses a staging readback with row-pitch handling in
d3d11_bridge.cpp; Metal uses a completed blit/readback path in
metal_bridge.mm; OpenGL owns context/readback state in gl_bridge.cpp. These
are Displayed-video integration points, not proof that a readback is currently
available for recording.

The existing display screenshot path crops the menu strip and has separate
readback behavior. It remains unchanged. The existing G75 rendered capture is
QA-only and is not a recording implementation.

### Required source classification

| Source | Recording image boundary | Required contents | Explicit exclusions |
|---|---|---|---|
| Native video | Complete 640x400 canonical guest raster, copied before CRT/window scaling | Guest content plus every enabled guest-area emulator OSD, composed on a recording-owned copy | CRT, scaling, letterbox, title/OS decorations, host menu/status/dialog UI |
| Displayed video | Final completed main-window client composition before Present/swap | Active CRT/effect, viewport/letterbox, guest-area OSD, visible in-client GUI, menu/dialog/status regions | OS window decorations, desktop and external dialogs |

The two choices are exactly the user-facing Native video and Displayed video.
There is no clean third source and no recording OSD checkbox.

## OSD and status map

Classification is by the actual draw path, not by the word “status”.

| Actual layer | Local owner / control | Guest-area or host UI | Native | Displayed |
|---|---|---|---|---|
| Video information overlay | sdl2/scrnmng.c:842-907, VAEG_DISPINFO_VIDEO | Guest-area emulator OSD | Include when enabled on recording-owned copy | Include when enabled |
| Framebuffer information overlay | sdl2/scrnmng.c:909-987, VAEG_DISPINFO_FRAMEBUFFER | Guest-area emulator OSD | Include when enabled on recording-owned copy | Include when enabled |
| Native overlay primitive entry | sdl2/scrnmng.c:989-995 and native presenter overlay plumbing | Guest-area overlay path | Include semantic enabled guest-area content, not live GUI draw data | Include in final client composition |
| FDD/FPS/CPU/SGP/frame title text | sdl2/scrnmng.c:543-582 | Host title/OS decoration | Exclude | Exclude with OS decorations |
| ImGui main menu bar | sdl2/gui/gui.cpp:4120-4134 | In-client GUI | Exclude | Include when visible |
| Fullscreen hint | sdl2/gui/gui.cpp:4136-4148 | In-client GUI | Exclude | Include when visible |
| ImGui dialogs | sdl2/gui/gui.cpp:4150-4164 | In-client GUI | Exclude | Include when visible |
| GUI/status state | sdl2/gui/gui.cpp:199-303, screen/info menus | In-client GUI/status | Exclude | Include when visible |
| Screenshot-only graphics analysis | scrnmng_draw_graphics_analysis and guest screenshot path | Screenshot annotation, not live OSD | Exclude | Exclude unless actually rendered in final client |
| Startup splash | sdl2/scrnmng.c startup path | Pre-guest presentation | Exclude | Exclude from a guest recording interval |

The Native implementation must snapshot enabled guest-area OSD semantically and
render it on an owned copy. It must not implement a live ImGui software
renderer or insert host menu/status strips. Displayed acquisition belongs after
the final in-client overlay/GUI pass and before Present/swap.

## Video clocks and ownership

The machine master clock is selected by machine/pccore.c from the model base
clocks in machine/pccore.h; for the standard VA2 configuration the base clock
is 3,993,600 Hz with multiplier 2, yielding a 7,987,200 Hz pccore.realclock.
The CPU event scheduler in machine/nevent.c owns CPU_CLOCK, CPU_BASECLOCK,
CPU_REMCLOCK and event decrementing. The 32-bit CPU counter wraps, so a
recorder must extend it before global arithmetic.

Guest display timing is generated by the TSP in io/tsp.c, not by the
frontend's nominal 60 Hz value. The TSP selects mode-dependent pixel clocks,
line counts, horizontal clocks and total frame timing, then calls
timing_setrate and schedules display/vsync events. drawscreenva and
screenvsyncva are therefore the authoritative guest video-boundary integration
points. Model, clock, mode, interlace and reset/state-load changes must be
treated as recording descriptor changes.

The frontend's SDL tick, frame skip, nowait and callback cadence are host
presentation policy. They must not determine recorded time or cause a complete
guest interval to be omitted.

## Audio ownership and timing

The current audio path is:

1. machine/pccore.c accepts 11025, 22050 and 44100 Hz and initializes the sound
   board.
2. sound/sound.c:sound_create owns an interleaved signed 32-bit stereo stream
   and a reserve sized to 100 ms.
3. sound/sound.c:sound_changeclock derives the sound clock from
   pccore.realclock / 25 and the output sample grid from rate / 25.
4. sound/sound.c:sound_sync converts elapsed guest clock into generated samples
   and calls streamprepare.
5. sdl2/soundmng.c:sound_play_cb calls sound_pcmlock, converts/clips to
   AUDIO_S16SYS, and supplies stereo samples to SDL. The SDL request is stereo
   S16 at the configured rate, but the current SDL_AudioSpec have value is not
   retained, so the actual obtained device format is not yet an explicit
   recording descriptor.

The ymfm bridge is in sdl2/ymfmbridge.cpp. The local YM2203 clock is 3,993,552
Hz and the YM2608 clock is 7,987,104 Hz. In the vendored
external/ymfm/src/ymfm_opn.h, both paths report these exact source rates under
the current fidelity policy:

| Chip | Minimum fidelity | Medium fidelity | Maximum fidelity |
|---|---:|---:|---:|
| YM2203 | 166,398 Hz | 332,796 Hz | 998,388 Hz |
| YM2608 | 166,398 Hz | 332,796 Hz | 998,388 Hz |

The current default is minimum fidelity. The ymfm bridge generates/averages
toward the configured output rate; its source-rate reporting and the existing
sound stream's sample accounting must be reconciled before recording production
is added. The oscillator/native chip rates must not be mistaken for the public
recording sample grid.

### Look-ahead finding

sound_pcmlock can call streamprepare(remain - reserve) from the audio callback
when the remaining stream is beyond the reserve, then updates lastclock from
the guest clock. sound_sync also generates based on guest elapsed clock. REC00
does not claim these paths are already a recording-safe single owner. REC10 and
REC11 must inventory generated samples, reserve contents, generator cursor and
callback ownership, then establish one guest-time producer without duplicate or
silently discarded samples. Device absence and mute must not change the
recording timeline.

The required v1 common epoch is a guest-clock/sample-grid relation. Audio
samples are selected by exact rational grid positions for each completed guest
interval; callback arrival is not a timestamp source.

## Validation, risks and human gate

The baseline build and ROM-less self-tests above establish that the audit
checkout builds and that the pre-existing QA entry points still pass. They do
not establish FFV1 encoding, MKV seekability, audio/video alignment, native or
displayed readback, or any Windows/Linux/real-GPU behavior. No movie was
generated because REC00 forbids implementation.

Open implementation risks are:

* the audio callback look-ahead must be reconciled before any recording tap;
* the obtained SDL device format and conversion state need an explicit owner;
* each native presenter needs a final-client capture proof, including Metal
  completion/storage and D3D11 row-pitch/format handling;
* geometry, clock, audio-format and presenter changes need a safe-stop boundary;
* bounded backpressure must be cancellable without taking audio, emulator-core
  or GPU locks;
* native and displayed OSD must remain distinct without changing existing
  screenshots or QA captures.

REC00 machine evidence is complete for the documentation/audit scope. The
maintainer must explicitly accept this report before REC01 is eligible.
