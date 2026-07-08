# ROADMAP — vaeg modernization

## Phase 1 (COMPLETE, tag `phase1-complete`)

Toolchain + repo hygiene: VS2008 baseline (M1, tag `baseline-vs2008`,
frozen at `vs2008-final`), VS2017/v141 (M2, tag `baseline-v141`),
prune (M3), lowercase (M4), LF (M5), UTF-8 without BOM (M6, Option A
charset flags). Task files `tasks/M0..M6` are historical record; do not
re-run them.

## Phase 2 — cross-platform (macOS / Linux / Windows-MinGW)

Goal: SDL2 frontend + Dear ImGui GUI + CMake, C-only cores, sustainable
tree. M7-M12 achieved the portable build, VA support on `i286c/`,
three-platform CMake coverage, and CI.

M13 closes phase 2 by removing retired paths and documenting the final
tier split:

- Active tree: CMake/C/SDL2/Dear ImGui; CPU in `i286c/`; VA memory in
  `cpucva/memoryva.c`; Z80 side in `cpucva/z80c.cpp`.
- Frozen reference tier: `win9x/`, `i286x/`, `cpuxva/memoryva.x86`, and
  `hlp/`. These remain reference-only because the v141 build was
  decisive in the G9 defect chain: differential FDC traces, the V30 DMA
  pump comparison, and same-tree A/B isolated the portable defect.
- Removed in M13: retired `sdl/` SDL1 frontend and leftover accessories
  Visual Studio project metadata.

The frozen reference tier is protected by immutable tags and source
history, not by a current CI or compile guarantee.

## Milestone table

| ID  | Task file                  | Deliverable | Gate |
|-----|----------------------------|-------------|------|
| M0  | tasks/M0_inventory.md      | Inventory report (no repo mutation) | review only |
| M1  | tasks/M1_vs2008_baseline.md | VS2008 Win32 Release builds as-is | **G1** human |
| M2  | tasks/M2_vs2017_v141.md    | v141 build of unmodified code | **G2** human |
| M3  | tasks/M3_prune.md          | Unreferenced files deleted from approved list | **G3** human |
| M4  | tasks/M4_lowercase.md      | All tracked paths lowercase | **G4** human |
| M5  | tasks/M5_eol_lf.md         | LF everywhere except declared CRLF exceptions; `.gitattributes` | **G5** human |
| M6  | tasks/M6_utf8.md           | UTF-8 without BOM sources; charset flags decided | **G6** human |
| M7  | tasks/M7_cmake_core.md     | CMake skeleton; NP2 core libs compile with gcc+clang on Linux; portable `sdl2/compiler.h` | **G7** machine + review |
| M8  | tasks/M8_sdl2_frontend.md  | `sdl2/` SDL2 frontend (video/audio/input/timer/main loop) runs the PC-98 core on Linux | **G8** human |
| M9  | tasks/M9_va_portable.md    | `cpucva/memoryva.c`; VA machine builds and runs on i286c; V3 boot + VA demo on Linux | **G9** human (standard VA gate) |
| M10 | tasks/M10_imgui.md         | Dear ImGui GUI: mount/reset/state/display/sound/exit; GUI-PARITY.md | **G10** human |
| M11 | tasks/M11_mingw_macos.md   | MinGW + macOS builds via CMake presets; UTF-8 path boundary on Windows | **G11** human per OS |
| M12 | tasks/M12_ci.md            | GitHub Actions 3-OS matrix; ROM-less tests; repo invariant checks | **G12** machine |
| M13 | tasks/M13_retire_legacy.md | Delete retired `sdl/`; keep frozen `win9x/`, `i286x/`, `cpuxva/memoryva.x86`, `hlp/`; docs | **G13** human sign-off |

Phase 2 dependencies: M7 → M8 → {M9, M10 parallel} → M11 → M12 → M13.
M9 must pass before M11 (all three OSes must ship the VA machine, not
the PC-98 scaffold).

## Gate protocol

Agent side (pasted into PR): CMake build logs, `tools/repo/` check
output, and for M8+ a headless smoke run
(`SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/sdl2/vaeg
--smoke` or documented equivalent). The frozen reference tier has no
current CI build.

User side (manual, per `gates/GATE_CHECKLIST_PHASE2.md`): clean-checkout
build, V3-mode boot, bundled VA demo, OS boot + simple operations
(DIR, launch a program). G7/G12 are machine gates; G8 is the frontend
gate on the PC-98 scaffold; G9 onward use the full VA checklist.

A gate passes only when the user says so. Pushed tags are immutable.
Tag `portable-pc98` after G8, `portable-va` after G9,
`phase2-complete` after G13.

## Resolved decision points

- **memoryva porting strategy (M9).** Faithful transliteration of
  `cpuxva/memoryva.x86` into `cpucva/memoryva.c`.
- **ImGui rendering backend (M10).** ADR-0002 selected
  `imgui_impl_sdl2` + `imgui_impl_sdlrenderer2`.
- **Japanese font for ImGui (M10).** ADR-0003 selected bundled
  `assets/NotoSansJP-Regular.ttf`.
- **Fate of `win9x/` and assembly references (M13).** ADR-0007 keeps
  `win9x/`, `i286x/`, `cpuxva/memoryva.x86`, and `hlp/` frozen as
  references; deletes retired `sdl/` and leftover accessories project
  metadata.
