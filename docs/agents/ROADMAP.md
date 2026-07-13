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
| M14 | tasks/M14_keyboard_mapping.md | PC-88VA/PC-8801-style SDL2 keyboard mapping; JIS physical, US keytop, and custom presets; Kana/Roman-Kana input; tenkeyless overlay; GUI binding table | **G14 passed** |
| M15 | tasks/M15_support_pc88va_constant_fold.md | Fold the always-enabled `SUPPORT_PC88VA` compile-time flag in the active tree while retaining runtime model selection | **G15 passed** |
| M16 | tasks/M16_sasi_hdd_gui.md | Reactivate SASI in active CMake; expose SASI HDI creation and SASI-1/SASI-2 Open/Remove in the SDL2 ImGui HardDisk menu | **G16 passed** |
| M17 | tasks/M17_opn_backend.md | Keep NP2 OPN/OPNA FM selectable; add BSD-3-Clause ymfm YM2203/YM2608 as the default backend with GUI/config selection | **G17 passed** |
| M18 | tasks/M18_rom_layout.md | Use executable-relative MAME ROM names/checksums, with VA2 `*_va2.rom` names and GUI VA/VA2 selection | **G18 passed** |
| M19 | tasks/M19_portable_runtime.md | Embed frontend assets, consolidate portable state under `vaeg.cfg`, align backup-memory lookup, and model VA OPN/OPNA hardware explicitly | **G19 passed** |
| M20 | tasks/M20_cpu_sgp_speed_pacing.md | Separate V30 and SGP execution capacity from fixed machine/peripheral time; add Configure, No Wait, frame skip, and hold-F11 fast-forward | **G20 passed** |
| M21 | tasks/M21_sdl2_display_effects.md | SDL2-only display effects, resizable common viewport, simplified fullscreen, and embedded historical application icon | **G21 passed** |
| M22 | tasks/M22_disk_image_drop.md | Direct and ZIP/7z/LZH disk-image drag and drop with sorted FDD1/FDD2 assignment and bounded safe extraction | **G22 human** |
| M23 | tasks/M23_formatted_fdd_images.md | Create formatted blank FAT12 D88 images as Japanese MS-DOS 2HD (1.232 MB) or 2DD (640 KB), with optional persisted FDD1/FDD2 mounting | **G23 passed** |
| M24 | tasks/M24_host_clipboard_paste.md | Paste host clipboard printable ASCII and line breaks through a paced guest keyboard make/break queue | **G24 passed** |
| M25 | tasks/M25_fdd_raw_images.md | Create formatted FAT12 FDD images as D88 or mtools-compatible IMG raw containers | **G25 passed** |
| M26 | tasks/M26_mouse_input.md | Port original relative mouse capture to SDL2 and expose the VA joystick/mouse controller-port choice | **G26 human** |

Phase 2 dependencies: M7 → M8 → {M9, M10 parallel} → M11 → M12 → M13.
Post-phase dependency: M13 → M14 → M15 → M16 → M17 → M18 → M19 → M20 → M21 → M22 → M23 → M24 → M25 → M26.
M9 must pass before M11 (all three OSes must ship the VA machine, not
the PC-98 scaffold).

M14 is complete. The SDL2 frontend now has a named VA key inventory,
normal guest make/break injection for physical and synthetic input,
JIS physical and US keytop modes, custom scancode-name bindings,
guest-visible Kana lock, Roman-Kana input, and a tenkeyless game overlay.
The implementation and human-gate record are in
`tasks/M14_keyboard_mapping.md`; detailed mapping evidence remains in
`../modernization/keyboard-mapping.md`.

M15 is complete. `SUPPORT_PC88VA` is no longer an active-tree feature
flag or CMake definition. The runtime `pccore.model_va` checks remain
because VA1/VA2 and non-VA guest behavior are runtime state, not build
configuration. The implementation scope, release-integration adjustment,
verification commands, and G15 record are in
`tasks/M15_support_pc88va_constant_fold.md`.

M19 is complete. Active
executables embed the GUI font and startup splash, the portable frontend uses
only `vaeg.cfg`, executable-local configuration and existing backup memory
override user-state copies, and VA sound hardware distinguishes built-in OPN
from OPNA in Sound Board II and VA2/VA3. The implementation record and passed
G19 checklist are in `tasks/M19_portable_runtime.md`.

M20 is complete and G20 passed. V30 instruction execution
and SGP command execution now have independent scaling while the existing
standard-x2 machine/peripheral timeline remains fixed. The SDL2 frontend adds
transactional CPU/SGP configuration, persisted No Wait and frame skip, and a
non-persistent hold-F11 fast-forward shortcut. The clock-domain audit,
automated results, remaining hardware uncertainty, and human checklist are in
`tasks/M20_cpu_sgp_speed_pacing.md`.

The V30/uPD9002 model default remains 7.9872 MHz for VA, VA2, and VA3. SGP
Model default follows the documented model distinction: 3.9936 MHz for VA and
7.9872 MHz for VA2/VA3.

M21 is implemented and G21 passed as an SDL_Renderer-only display milestone.
It adds a shared viewport, resizable windows, immediate
Windowed/current-desktop Exclusive switching, and procedural lightweight
effects without bgfx, custom shaders, MAME renderer code, or new graphics
dependencies. Its packaging follow-up embeds the unchanged historical VAEG
ICO for SDL runtime window icons on all platforms and as a native Windows PE
resource. Borderless and detailed monitor/mode selection are not exposed in
the simplified current GUI. The scope, icon provenance, verification, and G21
checklist are in
`tasks/M21_sdl2_display_effects.md`.

M22 adds SDL2 disk-image drag and drop. Direct images and archive contents are
sorted by basename and assigned to FDD1/FDD2. ZIP, 7z, and LZH extraction uses
bounded LibArchive streaming with traversal and link rejection; MinGW builds
the pinned archive stack statically, as do macOS release builds. Extracted
images are persistent managed user state and are pruned only when neither FDD
references them. The implementation and G22 checklist are in
`tasks/M22_disk_image_drop.md`.

M23 is complete and G23 passed. It adds FDD-menu creation of empty, formatted
FAT12 data disks in D88 containers. It covers the Japanese MS-DOS 1.232 MB
2HD geometry and standard 640 KB 2DD geometry, refuses overwrite, and can
mount the new image through the existing persistent FDD path. The
implementation and G23 checklist are in `tasks/M23_formatted_fdd_images.md`.

M24 is complete and G24 passed. It adds host-to-guest ASCII clipboard paste
through the existing keyboard injection path. The Edit menu and platform
paste shortcut enqueue printable ASCII and Return actions at a conservative
rate. GUI text capture pauses the queue; reset, state load, focus loss, and
shutdown cancel it safely.
Japanese/IME paste and guest-to-host copy remain later work. The implementation
and G24 checklist are in `tasks/M24_host_clipboard_paste.md`.

M25 is complete and G25 passed. It extends M23 creation with a D88/IMG
container choice. IMG is a contiguous FAT12 sector image suitable for mtools
and normal vaeg mounting. Both the 1.232 MB 2HD and 640 KB 2DD raw geometries
are recognized. The implementation and G25
checklist are in `tasks/M25_fdd_raw_images.md`.

M26 is complete and G26 passed. The guest-side generic and PC-88VA mouse
I/O paths remain unchanged; the active SDL2 frontend now supplies relative
motion and active-low buttons through their existing `mousemng_getstat()`
seam. Capture uses SDL relative mode with focus/ImGui safety, original
F12/middle-button controls, and persisted VA joystick/mouse port selection.
The implementation record and G26 checklist are in
`tasks/M26_mouse_input.md`.

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
`phase2-complete` after G13. M14-M19 currently have no tag
assignment.

## Resolved decision points

- **memoryva porting strategy (M9).** Faithful transliteration of
  `cpuxva/memoryva.x86` into `cpucva/memoryva.c`.
- **ImGui rendering backend (M10).** ADR-0002 selected
  `imgui_impl_sdl2` + `imgui_impl_sdlrenderer2`.
- **Japanese font for ImGui (M10).** ADR-0003 selected
  `assets/NotoSansJP-Regular.ttf`, now embedded in active executables.
- **Fate of `win9x/` and assembly references (M13).** ADR-0007 keeps
  `win9x/`, `i286x/`, `cpuxva/memoryva.x86`, and `hlp/` frozen as
  references; deletes retired `sdl/` and leftover accessories project
  metadata.
- **SDL2 keyboard policy (M14).** ADR-0008 keeps guest input scancode
  based, distinguishes JIS physical from US keytop behavior, routes all
  synthetic input through normal guest make/break handling, stores custom
  bindings by scancode name, and forbids text or guest-memory injection.
- **PC-88VA active-tree invariant (M15).** The active CMake/SDL2 build
  always includes VA support, so `SUPPORT_PC88VA` is folded true. Runtime
  model checks, `VAEG_FIX`, and `VAEG_EXT` remain independent controls.
- **Selectable OPN/OPNA synthesis (M17).** ADR-0009 keeps NP2 as a
  compatibility option and selects the BSD-3-Clause ymfm YM2203/YM2608
  implementation as the default FM-operator backend. NP2 retains timer/IRQ,
  SSG, ADPCM, rhythm, board-I/O, and mixer ownership in this milestone.
- **Model-specific ROM names (M18).** The active SDL2 frontend reads ROMs
  beside the executable. VA uses unsuffixed names; VA2 and VA3 use MAME's
  `pc88va2` `*_va2.rom` names without fallback to VA files. Executable-relative
  lookup is primary; cwd is a development fallback. Size, CRC32, and SHA-1
  are checked against MAME, including the extra `vasubsys.rom`, with warning-
  only mismatch handling. The frozen reference layout is unchanged.
- **Portable runtime identity and VA sound hardware (M19).** The active
  frontend has one configuration name, `vaeg.cfg`, selected executable-local
  first and user-state second. Existing executable-local `vabkupmem.dat`
  similarly selects portable backup state; otherwise user-state storage is
  used. Frontend assets are embedded, while `SNDboard` independently models
  VA built-in YM2203/OPN or YM2608/OPNA hardware; NP2/ymfm remains a separate
  synthesis-backend choice.
