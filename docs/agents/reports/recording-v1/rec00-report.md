<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# REC00 recording-v1 checkout audit

Status: WAITING_HUMAN

Feature ID and mapped legal M ID: REC00 / M100r1
Assigned branch: topic/rec00-recording-audit
Base SHA: 1544a1b8e19cc324c203a94c4e055361abb57d8b
Evaluated source SHA: 1544a1b8e19cc324c203a94c4e055361abb57d8b
Evaluated binary/toolchain identity: macOS arm64, AppleClang 21,
CMake 3.31.12, MacPorts SDL2 2.32.10, macos-macports preset
Evidence-only SHA: Not applicable
Task/spec version: docs/recording-v1 as supplied; REC00 audit task

## Scope and changes

REC00 made documentation and audit changes only. It did not add a recorder,
change the C core, change an SDL/presenter path, alter audio generation, change
screenshots or QA captures, or add a libav dependency.

Updated or added:

* docs/recording-v1/audit.md - actual source, timing, OSD and risk audit.
* docs/recording-v1/id-map.md - collision-free REC00-REC27 reservation.
* docs/recording-v1/platform-matrix.md - actual host inventory and evidence
  boundaries.
* docs/recording-v1/status.md - REC00 WAITING_HUMAN state.
* docs/recording-v1/handoff.md - gate handoff and next-task conditions.
* docs/recording-v1/file-layout.md - actual REC00 placement.
* docs/agents/tasks/M100r1_recording_v1_rec00.md - legal REC00 wrapper.
* docs/agents/reports/recording-v1/rec00-report.md - this report.

The supplied recording-v1 documentation package was inspected and retained in
its documented location. No future REC01 wrapper or production implementation
file was created. Existing unrelated untracked work was preserved and is not
part of this report.

The frozen source contract remains exactly two choices: Native video and
Displayed video. Native is the canonical guest raster before CRT/window
scaling, with enabled guest-area emulator OSD composed onto a recording-owned
copy. Displayed is the completed main-window client composition, including
active CRT, OSD, in-client GUI and status, but excluding OS decorations,
desktop and external dialogs. There is no recording OSD checkbox or clean
third source.

## Implementation findings and decisions

### Video

The canonical surface is in sdl2/scrnmng.c and is 640 by 400, backed by a
16-bit RGB565 shadow surface with a one-pixel left guard. scrnmng_surflock
exposes the pitch and active dimensions; the upload/readback paths use the
pixel base after the guard. scrndrawva.c, sdrawva.c and
machine/pccore.c:drawscreenva own guest composition and completed raster
production. sdl2/np2.c:run_guest_frame then enters GUI/presenter work.

The guest display clock is TSP/machine driven. machine/pccore.c selects the
model base clock and multiplier; the standard VA2 configuration is
3,993,600 Hz times 2 = 7,987,200 Hz. io/tsp.c derives mode-dependent pixel,
line and frame timing and schedules display/vsync events. CPU_CLOCK and its
remaining-clock/event state are owned by machine/nevent.c and require safe
wrap extension. The frontend SDL tick, nominal 60 Hz value, frame skip and
nowait policy are host pacing and are not recording clocks.

The SDL final composition is assembled in sdl2/scrnmng.c. Native presentation
is owned by the native presenter and backend bridges under
sdl2/librashader/. D3D11, OpenGL and Metal readback requirements were traced,
but no recording readback was implemented or accepted in REC00.

### OSD/status

The enabled guest-area video-information overlay
(VAEG_DISPINFO_VIDEO, sdl2/scrnmng.c:842-907) and framebuffer-information
overlay (VAEG_DISPINFO_FRAMEBUFFER, sdl2/scrnmng.c:909-987) are included by
both sources. Native must render them from a semantic snapshot onto its
owned copy; Displayed gets them from the final composition.

The title text for FDD/FPS/CPU/SGP/frame is an OS/title decoration and is
excluded from both sources. ImGui menu, fullscreen hint, dialogs and visible
in-client status are excluded from Native and included by Displayed. The
screenshot-only graphics-analysis annotation is not live recording OSD.
Startup splash is not a guest recording interval. This inventory is recorded
in audit.md with owners and source locations.

### Audio and common time

The current sound path is sound/sound.c guest-clock production plus
sdl2/soundmng.c SDL callback consumption. The internal stream is interleaved
signed 32-bit stereo; the SDL request is S16 stereo at the configured rate,
but the obtained SDL_AudioSpec is not retained as an explicit recording
descriptor. Valid configured output rates are 11025, 22050 and 44100 Hz.

The local ymfm clocks are 3,993,552 Hz for YM2203 and 7,987,104 Hz for
YM2608. The vendored ymfm revision reports 166,398 Hz minimum, 332,796 Hz
medium and 998,388 Hz maximum for both chip paths. The current default is
minimum fidelity. These are source/generation rates and must not be confused
with the recording PCM grid.

The important unresolved seam is sound_pcmlock: when the stream exceeds its
reserve, it can generate look-ahead from the audio callback and update
lastclock, while sound_sync also produces from guest elapsed time. REC00
does not treat this as recording-safe. REC10/REC11 must reconcile already
generated samples, reserve contents, generator cursor and callback ownership
before adding a recording tap. Recording time must use one extended guest
clock/sample-grid epoch, exact sample accounting and a guest-time producer;
callback arrival cannot timestamp or synthesize captured samples.

## Checks

| Check ID | Exact command/environment | Exit code / outcome | Evidence |
|---|---|---|---|
| REC00-BUILD | cmake --preset macos-macports; cmake --build --preset macos-macports -j2 | 0 / PASS | macOS arm64 baseline |
| REC00-SELFTEST | SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/macos-macports/sdl2/vaeg --selftest --no-cfg --no-bkupmem | 0 / PASS | selftest: all tests passed |
| REC00-SMOKE | SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/macos-macports/sdl2/vaeg --smoke --no-cfg --no-bkupmem | 0 / PASS | ROM-less smoke completed; expected missing-ROM warning |
| REC00-ID | python3 tools/qa/milestone_ids.py --selftest --audit --discover | 0 / PASS | 49 strict checks; discovery and audit passed |
| REC00-CASE | python3 tools/repo/check_case.py | 0 / PASS | 0 findings |
| REC00-ENCODING | python3 tools/repo/check_encoding.py --expect utf8 | 0 / PASS | 0 violations |
| REC00-EOL | python3 tools/repo/check_eol.py --enforce | 0 / PASS | 0 violations |

No recording unit test, FFV1 encoder test, MKV seek/parse test, decoded
frame/sample-count test, VLC test, native GPU readback test, Windows test or
Linux test applies to this documentation-only milestone. No movie or private
integration artifact was created.

## Integrity results

Not applicable in REC00. No frame, sample, packet, PTS, queue, or movie was
generated. The exact-count and A/V alignment checks are deferred to the
milestones that implement the relevant seams.

## Review and limitations

The source/ownership review identified the audio callback look-ahead, missing
obtained-device-format ownership, presenter-specific final-target readback and
geometry/clock safe-stop requirements. These are unresolved implementation
risks, not claims of failure. The audit does not prove any recording backend.

The current roadmap has no conflicting M100/M101 assignment. M100r1-M100r28
are reserved in id-map.md without rewriting the roadmap or creating future
wrappers. The final commit SHA cannot be self-referenced in this report; it
will be reported after the documentation commit is created.

## Gate and next task

Machine gate: PASS for REC00 audit completeness, baseline build/self-test and
repository validators. No feature runtime gate was applicable.

Human gate: PENDING. The maintainer must explicitly accept REC00. Until then,
REC01 is not started.

Push/remote state: to be reported after the focused REC00 commit and topic
branch push.

Next dependency-ready task: REC01 / M100r2, only after explicit REC00
acceptance, with a host capable of running its specified contract checks.

Stop here; do not implement REC01 in this report session.
