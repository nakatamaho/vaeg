# Complete file placement

## Delivered files: copy these into the assigned VAEG worktree

All paths below are repository-relative. These are new documentation files, not
replacements for the root AGENTS.md, project roadmap or an older M99 task. The
package installer copies only docs/recording-v1/ and refuses differing existing
files. The package root readme.md, install.py and manifest.sha256 stay outside the
repository unless separately approved.

```text
docs/recording-v1/acceptance.md
docs/recording-v1/architecture.md
docs/recording-v1/audit.md
docs/recording-v1/codex-prompt.md
docs/recording-v1/decisions.md
docs/recording-v1/display-osd.md
docs/recording-v1/ffmpeg-backend.md
docs/recording-v1/file-layout.md
docs/recording-v1/goal-next.md
docs/recording-v1/goal-start.md
docs/recording-v1/goals/rec00.md
docs/recording-v1/goals/rec01.md
docs/recording-v1/goals/rec02.md
docs/recording-v1/goals/rec03.md
docs/recording-v1/goals/rec04.md
docs/recording-v1/goals/rec05.md
docs/recording-v1/goals/rec06.md
docs/recording-v1/goals/rec07.md
docs/recording-v1/goals/rec08.md
docs/recording-v1/goals/rec09.md
docs/recording-v1/goals/rec10.md
docs/recording-v1/goals/rec11.md
docs/recording-v1/goals/rec12.md
docs/recording-v1/goals/rec13.md
docs/recording-v1/goals/rec14.md
docs/recording-v1/goals/rec15.md
docs/recording-v1/goals/rec16.md
docs/recording-v1/goals/rec17.md
docs/recording-v1/goals/rec18.md
docs/recording-v1/goals/rec19.md
docs/recording-v1/goals/rec20.md
docs/recording-v1/goals/rec21.md
docs/recording-v1/goals/rec22.md
docs/recording-v1/goals/rec23.md
docs/recording-v1/goals/rec24.md
docs/recording-v1/goals/rec25.md
docs/recording-v1/goals/rec26.md
docs/recording-v1/goals/rec27.md
docs/recording-v1/handoff.md
docs/recording-v1/id-map.md
docs/recording-v1/milestones.md
docs/recording-v1/milestones/rec00-audit.md
docs/recording-v1/milestones/rec01-contracts.md
docs/recording-v1/milestones/rec02-clock.md
docs/recording-v1/milestones/rec03-queue.md
docs/recording-v1/milestones/rec04-ffmpeg-capabilities.md
docs/recording-v1/milestones/rec05-ffv1.md
docs/recording-v1/milestones/rec06-pcm.md
docs/recording-v1/milestones/rec07-matroska.md
docs/recording-v1/milestones/rec08-osd-snapshot.md
docs/recording-v1/milestones/rec09-native-composition.md
docs/recording-v1/milestones/rec10-audio-seam.md
docs/recording-v1/milestones/rec11-audio-tap.md
docs/recording-v1/milestones/rec12-native-video-tap.md
docs/recording-v1/milestones/rec13-native-session.md
docs/recording-v1/milestones/rec14-native-ui.md
docs/recording-v1/milestones/rec15-displayed-contract.md
docs/recording-v1/milestones/rec16-displayed-sdl.md
docs/recording-v1/milestones/rec17-d3d11-readback.md
docs/recording-v1/milestones/rec18-displayed-d3d11.md
docs/recording-v1/milestones/rec19-opengl-readback.md
docs/recording-v1/milestones/rec20-displayed-opengl.md
docs/recording-v1/milestones/rec21-metal-readback.md
docs/recording-v1/milestones/rec22-displayed-metal.md
docs/recording-v1/milestones/rec23-lifecycle.md
docs/recording-v1/milestones/rec24-final-ui.md
docs/recording-v1/milestones/rec25-faults-performance.md
docs/recording-v1/milestones/rec26-packaging.md
docs/recording-v1/milestones/rec27-release-validation.md
docs/recording-v1/platform-matrix.md
docs/recording-v1/readme.md
docs/recording-v1/report-template.md
docs/recording-v1/sources.md
docs/recording-v1/spec.md
docs/recording-v1/status.md
docs/recording-v1/timing-audio.md
docs/recording-v1/workflow.md
```

## Files created by the implementation workflow, not fabricated by this pack

REC00 creates or updates:

```text
docs/agents/reports/recording-v1/rec00-report.md
```

Each later task creates its corresponding report:

```text
docs/agents/reports/recording-v1/rec01-report.md
docs/agents/reports/recording-v1/rec02-report.md
docs/agents/reports/recording-v1/rec03-report.md
docs/agents/reports/recording-v1/rec04-report.md
docs/agents/reports/recording-v1/rec05-report.md
docs/agents/reports/recording-v1/rec06-report.md
docs/agents/reports/recording-v1/rec07-report.md
docs/agents/reports/recording-v1/rec08-report.md
docs/agents/reports/recording-v1/rec09-report.md
docs/agents/reports/recording-v1/rec10-report.md
docs/agents/reports/recording-v1/rec11-report.md
docs/agents/reports/recording-v1/rec12-report.md
docs/agents/reports/recording-v1/rec13-report.md
docs/agents/reports/recording-v1/rec14-report.md
docs/agents/reports/recording-v1/rec15-report.md
docs/agents/reports/recording-v1/rec16-report.md
docs/agents/reports/recording-v1/rec17-report.md
docs/agents/reports/recording-v1/rec18-report.md
docs/agents/reports/recording-v1/rec19-report.md
docs/agents/reports/recording-v1/rec20-report.md
docs/agents/reports/recording-v1/rec21-report.md
docs/agents/reports/recording-v1/rec22-report.md
docs/agents/reports/recording-v1/rec23-report.md
docs/agents/reports/recording-v1/rec24-report.md
docs/agents/reports/recording-v1/rec25-report.md
docs/agents/reports/recording-v1/rec26-report.md
docs/agents/reports/recording-v1/rec27-report.md
```

Repository M-prefixed task wrappers and roadmap registration follow the actual
root conventions. REC00 records every exact mapped ID/path in id-map.md; none is
preallocated here. A wrapper points at the corresponding REC task and includes
its machine/human acceptance classification. Do not create obsolete task IDs or
use RECNN as an invalid repository commit prefix.

## REC00 actual placement

REC00 created or updated only these workflow documents:

    docs/agents/reports/recording-v1/rec00-report.md
    docs/agents/tasks/M100r1_recording_v1_rec00.md
    docs/recording-v1/audit.md
    docs/recording-v1/id-map.md
    docs/recording-v1/platform-matrix.md
    docs/recording-v1/status.md
    docs/recording-v1/handoff.md

The supplied docs/recording-v1 task package is documentation-only input and is
kept in its documented location. Future M100r2-M100r28 wrappers, reports,
recording sources, tests and tools were not created. No production source or
binary payload was changed by REC00.

## Planned implementation responsibilities

The following is a proposed small module layout, **not existing implemented
files**. REC00 adapts exact names to the real checkout and scoped conventions,
then records the approved mapping. Do not create empty placeholders for all files
or duplicate an existing utility. Keep the C core C99; implementation in this
frontend subtree may use the repository's C++17 rules.

```text
sdl2/recording/
    recording.h              # Narrow C-compatible frontend bridge
    recording.cpp            # Session controller and lifecycle
    types.h                  # Owned frame/audio/interval descriptors; no FFmpeg
    clock.h
    clock.cpp                # Exact guest/sample accounting
    queue.h
    queue.cpp                # Bounded complete-interval ownership
    native_frame.h
    native_frame.cpp         # Canonical copy plus OSD composition
    osd_snapshot.h
    osd_snapshot.cpp         # Immutable semantic OSD snapshot
    displayed_capture.h
    displayed_capture.cpp    # Shared final-client capture contract
    backend.h                # Private recorder sink boundary
    ffmpeg_backend.cpp       # FFV1, PCM, muxing, encoder draining
    local_file.h
    local_file.cpp           # Exclusive seekable partial-file ownership

tests/frontend/recording/
    clock_test.cpp
    queue_test.cpp
    ffv1_test.cpp
    pcm_test.cpp
    matroska_test.cpp
    osd_test.cpp
    native_capture_test.cpp
    displayed_capture_test.cpp
    lifecycle_test.cpp
    recording_fixture.cpp

tools/recording/
    verify_movie.py          # Independent decoded RGB/PCM/timeline checks
```

Split only when responsibility or test isolation warrants it. Synthetic fixture
source/sink helpers can live in existing test support. Actual CMake target and
CTest names must be discovered; these paths do not assume an existing test target.
All new source/scripts/comments are English and follow existing header/licensing
rules. Existing assets and private media remain untouched.

## Existing integration points to inspect, then patch minimally

The public-tree observations below are navigation aids, not a claim about the
assigned checkout. Verify all symbols and paths during REC00; see sources.md.

| Existing area | Intended narrow responsibility |
|---|---|
| CMakeLists.txt | Optional VAEG_ENABLE_RECORDING switch, scoped dependencies and tests |
| sdl2/np2.c; sdl2/pacing.c; sdl2/framedisp.c | Guest frame boundary, recording lifecycle and no skipped capture intervals |
| sdl2/scrnmng.c; sdl2/screenshot.c | Read canonical source without changing QA/screenshot semantics; SDL final-client capture |
| sdl2/viewport.c | Drawable geometry and safe-stop notification; no Native scaling |
| sdl2/soundmng.c; sound/sound.c | Minimal C99 audio ownership seam, recording producer and device-only callback |
| sdl2/cliopts.c; sdl2/gui/gui.cpp | Exact source labels, recording CLI, Start/Stop/status/errors |
| sdl2/librashader/native_presenter.cpp; sdl2/librashader/native_presenter_controller.cpp | Existing native GPU presenter lifetime, final-composition hook and shared capture contract |
| sdl2/librashader/backends/d3d11_presenter.cpp; sdl2/librashader/backends/d3d11_bridge.cpp | D3D11 final target readback on its owning thread |
| sdl2/librashader/backends/gl_presenter.cpp; sdl2/librashader/backends/gl_bridge.cpp | OpenGL final target readback, pixel-store/context restoration |
| sdl2/librashader/backends/metal_presenter.cpp; sdl2/librashader/backends/metal_bridge.mm | Metal stored final target, blit/readback and completion ownership |
| Existing CI/build/packaging definitions, paths found in REC00 | Separate OFF/ON matrix and explicitly reviewed optional distribution |

The GPU implementations belong next to the actual owning presenters, not in the
C core and not in a second shader engine. Never patch a source file merely because
it is listed here. Demonstrate why the actual local call site is needed.
