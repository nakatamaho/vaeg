# ROADMAP — vaeg modernization

## Phase 1 (COMPLETE, tag `phase1-complete`)

Toolchain + repo hygiene: VS2008 baseline (M1, tag `baseline-vs2008`,
frozen at `vs2008-final`), VS2017/v141 (M2, tag `baseline-v141`),
prune (M3), lowercase (M4), LF (M5), UTF-8 without BOM (M6, Option A
charset flags). Task files `tasks/M0..M6` are historical record; do not
re-run them.

## Phase 2 — cross-platform (macOS / Linux / Windows-MinGW)

Goal: SDL2 frontend + Dear ImGui GUI + CMake, C-only cores, sustainable
tree. The v141 LEGACY build stays green as the behavioral reference
until M13.

Central technical fact driving the ordering: the VA subsystem currently
requires x86 assembly (`i286x/` core + `cpuxva/memoryva.x86`, 1,256
lines of NASM, included by 10+ files in `iova/` and `vramva/`). There is
NO C implementation of the VA memory layer. The portable C path
(`i286c/` + `sound/opngenc.c` + `cpucva/z80c.cpp`) covers everything
EXCEPT memoryva. Therefore:

- M7/M8 bring up build system + SDL2 frontend on the plain PC-98 core
  (zero assembly, exactly what `sdl/` proved possible on SDL1). This
  de-risks the frontend independently of the core-porting risk.
- M9 is the high-risk milestone: port memoryva to C and wire the VA
  machine onto i286c. It gets its own gate and its own branch.
- M10 (ImGui) depends only on M8 and may run in parallel with M9 on a
  separate branch/session.

| ID  | Task file                  | Deliverable | Gate |
|-----|----------------------------|-------------|------|
| M7  | tasks/M7_cmake_core.md     | CMake skeleton; NP2 core libs compile with gcc+clang on Linux; portable `sdl2/compiler.h` | **G7** machine + review |
| M8  | tasks/M8_sdl2_frontend.md  | `sdl2/` SDL2 frontend (video/audio/input/timer/main loop) runs the PC-98 core on Linux | **G8** human |
| M9  | tasks/M9_va_portable.md    | `cpucva/memoryva.c`; VA machine builds and runs on i286c; V3 boot + VA demo on Linux | **G9** human (standard VA gate) |
| M10 | tasks/M10_imgui.md         | Dear ImGui GUI: mount/reset/state/display/sound/exit; GUI-PARITY.md | **G10** human |
| M11 | tasks/M11_mingw_macos.md   | MinGW + macOS builds via CMake presets; UTF-8 path boundary on Windows | **G11** human per OS |
| M12 | tasks/M12_ci.md            | GitHub Actions 3-OS matrix; ROM-less tests; repo invariant checks | **G12** machine |
| M13 | tasks/M13_retire_legacy.md | Retire `sdl/`; decide fate of `win9x/`, `i286x/`, `memoryva.x86`; docs | **G13** human sign-off |

Dependencies: M7 → M8 → {M9, M10 parallel} → M11 → M12 → M13.
M9 must pass before M11 (all three OSes must ship the VA machine, not
the PC-98 scaffold).

## Gate protocol

Agent side (pasted into PR): build logs (cmake + the LEGACY v141 status
statement), `tools/repo/` check output, and for M8+ a headless smoke run
(`SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/sdl2/vaeg
--smoke` or documented equivalent).

User side (manual, per `gates/GATE_CHECKLIST_PHASE2.md`): clean-checkout
build, V3-mode boot, bundled VA demo, OS boot + simple operations
(DIR, launch a program). G7/G12 are machine gates; G8 is the frontend
gate on the PC-98 scaffold; G9 onward use the full VA checklist.

A gate passes only when the user says so. Pushed tags are immutable.
Tag `portable-pc98` after G8, `portable-va` after G9,
`phase2-complete` after G13.

## Known decision points (do not resolve silently)

- **memoryva porting strategy (M9).** Faithful transliteration of the
  NASM into C versus re-derivation from `memoryva.h` + hardware docs.
  Default is faithful transliteration (behavior reference exists in the
  LEGACY build); re-derivation requires an ADR.
- **ImGui rendering backend (M10).** SDL_Renderer backend vs OpenGL3.
  Propose with trade-offs, record as ADR before implementing.
- **Fate of `win9x/` (M13).** Keep as frozen reference vs delete after
  ImGui parity. User decides at G13; nothing is deleted before that.
- **Japanese font for ImGui (M10).** Must not read guest font ROM.
  Bundled font vs system font lookup; ADR.
