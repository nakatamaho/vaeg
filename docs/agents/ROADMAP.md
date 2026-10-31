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

M13 closed phase 2 by removing retired paths and documenting the tier split
that existed at that gate:

- Active tree: CMake/C/SDL2/Dear ImGui; main CPU in `cpu/upd9002/`; VA memory
  in `cpucva/memoryva.c`; Z80 side in the suzukiplan-backed
  `cpucva/z80_compat_cpu.cpp` wrapper with `cpucva/upd780_disasm.cpp`.
- Then-frozen reference tier: `win9x/`, `i286x/`,
  `cpuxva/memoryva.x86`, and `hlp/`. The v141 build was decisive in the G9
  defect chain: differential FDC traces, the V30 DMA pump comparison, and
  same-tree A/B isolated the portable defect.
- Removed in M13: retired `sdl/` SDL1 frontend and leftover accessories
  Visual Studio project metadata.

M57 later removed that reference tier from the current tree after preserving
its legal evidence. Its exact G56 snapshot is protected by annotated tag
`archive/frozen-win9x-i286x-g56` and source history, not by a current CI or
compile guarantee.

## Milestone table

Task files whose gates have passed are historical records, not runnable
instructions. See [`tasks/README.md`](tasks/README.md), including the explicit
M36–M41 archive status.

| ID  | Task file                  | Deliverable | Gate |
|-----|----------------------------|-------------|------|
| M0  | tasks/M0_inventory.md      | Inventory report (no repo mutation) | **Review passed** |
| M1  | tasks/M1_vs2008_baseline.md | VS2008 Win32 Release builds as-is | **G1 passed** (human) |
| M2  | tasks/M2_vs2017_v141.md    | v141 build of unmodified code | **G2 passed** (human) |
| M3  | tasks/M3_prune.md          | Unreferenced files deleted from approved list | **G3 passed** (human) |
| M4  | tasks/M4_lowercase.md      | All tracked paths lowercase | **G4 passed** (human) |
| M5  | tasks/M5_eol_lf.md         | LF everywhere except declared CRLF exceptions; `.gitattributes` | **G5 passed** (human) |
| M6  | tasks/M6_utf8.md           | UTF-8 without BOM sources; charset flags decided | **G6 passed** (human) |
| M7  | tasks/M7_cmake_core.md     | CMake skeleton; NP2 core libs compile with gcc+clang on Linux; portable `sdl2/compiler.h` | **G7 passed** (machine + review) |
| M8  | tasks/M8_sdl2_frontend.md  | `sdl2/` SDL2 frontend (video/audio/input/timer/main loop) runs the PC-98 core on Linux | **G8 passed** (human) |
| M9  | tasks/M9_va_portable.md    | `cpucva/memoryva.c`; VA machine builds and runs on i286c; V3 boot + VA demo on Linux | **G9 passed** (human, standard VA gate) |
| M10 | tasks/M10_imgui.md         | Dear ImGui GUI: mount/reset/state/display/sound/exit; GUI-PARITY.md | **G10 passed** (human) |
| M11 | tasks/M11_mingw_macos.md   | MinGW + macOS builds via CMake presets; UTF-8 path boundary on Windows | **G11 passed** (human per OS) |
| M12 | tasks/M12_ci.md            | GitHub Actions 3-OS matrix; ROM-less tests; repo invariant checks | **G12 passed** (machine) |
| M13 | tasks/M13_retire_legacy.md | Delete retired `sdl/`; keep frozen `win9x/`, `i286x/`, `cpuxva/memoryva.x86`, `hlp/`; docs | **G13 passed** (human sign-off) |
| M14 | tasks/M14_keyboard_mapping.md | PC-88VA/PC-8801-style SDL2 keyboard mapping; JIS physical, US keytop, and custom presets; Kana/Roman-Kana input; tenkeyless overlay; GUI binding table | **G14 passed** |
| M15 | tasks/M15_support_pc88va_constant_fold.md | Fold the always-enabled `SUPPORT_PC88VA` compile-time flag in the active tree while retaining runtime model selection | **G15 passed** |
| M16 | tasks/M16_sasi_hdd_gui.md | Reactivate SASI in active CMake; expose SASI HDI creation and SASI-1/SASI-2 Open/Remove in the SDL2 ImGui HardDisk menu | **G16 passed** |
| M17 | tasks/M17_opn_backend.md | Keep NP2 OPN/OPNA FM selectable; add BSD-3-Clause ymfm YM2203/YM2608 as the default backend with GUI/config selection | **G17 passed** |
| M18 | tasks/M18_rom_layout.md | Use executable-relative MAME ROM names/checksums, with VA2 `*_va2.rom` names and GUI VA/VA2 selection | **G18 passed** |
| M19 | tasks/M19_portable_runtime.md | Embed frontend assets, consolidate portable state under `vaeg.cfg`, align backup-memory lookup, and model VA OPN/OPNA hardware explicitly | **G19 passed** |
| M20 | tasks/M20_cpu_sgp_speed_pacing.md | Separate V30 and SGP execution capacity from fixed machine/peripheral time; add Configure, No Wait, frame skip, and hold-F11 fast-forward | **G20 passed** |
| M21 | tasks/M21_sdl2_display_effects.md | SDL2-only display effects, resizable common viewport, simplified fullscreen, and embedded historical application icon | **G21 passed** |
| M22 | tasks/M22_disk_image_drop.md | Direct and ZIP/7z/LZH disk-image drag and drop plus FDD-picker archive open, with sorted assignment and bounded safe extraction | **G22 passed** |
| M23 | tasks/M23_formatted_fdd_images.md | Create formatted blank FAT12 D88 images as Japanese MS-DOS 2HD (1.232 MB) or 2DD (640 KB), with optional persisted FDD1/FDD2 mounting | **G23 passed** |
| M24 | tasks/M24_host_clipboard_paste.md | Paste host clipboard printable ASCII and line breaks through a paced guest keyboard make/break queue | **G24 passed** |
| M25 | tasks/M25_fdd_raw_images.md | Create formatted FAT12 FDD images as D88 or mtools-compatible IMG raw containers | **G25 passed** |
| M26 | tasks/M26_mouse_input.md | Port original relative mouse capture to SDL2 and expose the VA joystick/mouse controller-port choice | **G26 passed** |
| M27 | tasks/M27_frame_display.md | Restore the original measured guest-draw FPS display in the native window title | **G27 passed** |
| M28 | tasks/M28_sound_output_settings.md | Select common output sampling rate and sound buffer plus ymfm FM fidelity from the SDL2 Sound menu | **G28 passed** |
| M29 | tasks/M29_va1_tvram_aperture.md | Enforce the VA1 bank-1 64KB TVRAM aperture and restore PC-Engine 1.00 boot compatibility | **G29 focused human passed; VA2 regression corrected in M31** |
| M30 | tasks/M30_va_bms_window.md | Restore the VA `80000H-9FFFFH` BMS window semantics lost in the portable C memory port | **G30 passed** |
| M31 | tasks/M31_cli_boot_model.md | Select the VA or VA2/VA3 boot model with a session-only command-line override | **G31 passed** |
| M32 | tasks/M32_cli_startup_overrides.md | Add session-only CLI overrides for sound, media, execution, display, and input; remove positional FDD syntax | **G32 passed** |
| M34 | tasks/M34_z80_migration_contract.md | Verify the legacy Z80 contract, select the migration design, and retain revision-1 fixtures | **G34 passed** |
| M35 | tasks/M35_suzukiplan_irq_extension.md | Add the approved interrupt-acknowledge, level-IRQ, and raw-IM0 extension upstream or in a minimal fork | **G35 passed** |
| M36 | tasks/M36_z80_vendor_conformance.md | Vendor the approved Z80 revision and add standalone conformance and ZEX CI | **G36 passed** |
| M37 | tasks/M37_z80_wrapper.md | Add independently authored vaeg interfaces, revision-1 codec, and wrapper unit tests without integration | **G37 passed** |
| M38 | tasks/M38_z80_differential.md | Compare normalized externally observable legacy and replacement traces | **G38 passed** |
| M39 | tasks/M39_z80_integration.md | Integrate an opt-in replacement Z80 path and run private-system regressions | **G39 passed** |
| M40 | tasks/M40_z80_disassembler.md | Replace active legacy disassembly consumers and close the dual-core evidence period | **G40 passed** |
| M41 | tasks/M41_z80_cutover.md | Select the replacement exclusively, delete the seven approved files, and audit releases | **G41 passed** |
| M42 | tasks/M42_upd9002_adr_inventory_harness.md | Record uPD9002 ownership, dispatch/state inventory, behavior-neutral trace/harness infrastructure, and reproducible baselines | **G42 passed** |
| M43 | tasks/M43_upd9002_singlestep_v20_baseline.md | Pin and classify the external V20 corpus and freeze deterministic comparison baselines without changing CPU behavior | **G43 passed** |
| M44 | tasks/M44_upd9002_state_boundary.md | Separate runtime and serialized CPU state while preserving G41 payload compatibility and adding atomic validation | **G44 passed** |
| M45 | tasks/M45_upd9002_native_dispatch_fold.md | Make V30-compatible execution unconditional and remove the per-instruction 286/V30 selector and `i286c_step()` | **G45 passed** |
| M46 | tasks/M46_upd9002_dispatch_normalization.md | Normalize the one-time V30 dispatch constructor, prove post-construction immutability, and remove the obsolete block executors | **G46 passed** |
| M47 | tasks/M47_upd9002_rep0f_correctness.md | Determine correct uPD9002/V52 REP-prefixed 0x0F semantics and protected-state policy from pinned documents, V20 corpus evidence, and a safe PC-88VA probe without changing behavior | **G47 passed** |
| M48 | tasks/M48_upd9002_rep0f_implementation.md | Implement only the exact REP+0F semantic rule, state policy, and baseline transition explicitly approved at G47 | **G48 passed** |
| M49 | tasks/M49_upd9002_isolate_np2_286_protected_mode.md | Inventory the remaining NP2 286 protected-mode dependency closure after the approved correctness transition | **G49 passed** |
| M50 | tasks/M50_remove_np2_286_protected_mode.md | Remove only dependency-closed protected-mode groups explicitly approved at G49 | **G50 passed** |
| M51 | tasks/M51_upd9002_rename.md | Perform pure uPD9002 moves, public API renames, and final repository guards | **G51 passed** |
| M52 | tasks/M52_io_bank_memory.md | Restore portable I/O Bank Memory configuration and correct bank-zero main-RAM pass-through | **G52 passed** |
| M53 | tasks/M53_host_pacing.md | Add configurable non-blocking host pacing that slows guest execution without slowing the UI | **G53 passed** |
| M54 | tasks/M54_hostfat_readonly_prototype.md | Add a session-only read-only HOSTFAT block-device prototype backed by a fixed FAT snapshot | **G54 passed** |
| M55 | tasks/M55_hostfat_integration.md | Add PC-Engine-compatible FAT12-max HOSTFAT geometry, GUI/configuration, save-state identity, refresh policy, and hardened host-path handling | **G55 passed** |
| M56 | tasks/M56_hostfs_readonly_redirector.md | Probe the PC-Engine DOS redirector bridge before a clean-room read-only HOSTFS implementation | **G56 passed** (administratively closed at `b72e641733ddea6f0e8faef2507093f7c3aee5a4`: prerequisite absent; no HOSTFS implementation) |
| M57 | tasks/M57_remove_frozen_reference_tier.md | Preserve legal provenance, archive the exact G56 tier, then remove `win9x/`, `i286x/`, `hlp/`, and `cpuxva/memoryva.x86` without changing behavior | **G57 passed at `72322d5c9b8e40e4a988312aebe163a8190e2aa5`** |
| M58 | tasks/M58_upd9002_ssts_ratchet.md | Add immutable hash-level SST epochs, separate architectural/fingerprint profiles, strict classification governance, and lettered-milestone tooling support without changing CPU semantics | **G58 passed at `bc8a55c6da1082b85b794068e0d933e31fe46b13`** |
| M59 | tasks/M59_upd9002_semantics_evidence_pack.md | Produce deterministic expected/actual evidence for the first uPD9002 semantic milestones without changing CPU behavior | **G59 passed at `e7f2325bc81310532091a8ca82914030fdb8b6ba`** |
| M60a | tasks/M60a_upd9002_flags_materialization.md | Correct SST-observed guest-visible FLAGS materialization and load rules without changing interrupt-frame placement or IRET | **G60a passed at `ba2b7d3f5c76646b30d63fd8951f4a1964817b15`** |
| M60b | tasks/M60b_upd9002_rom_authority_epoch.md | Bind monitor-ROM and debugger authority to a content-addressed target-policy epoch and correct only the authorized 6C–6F and exact 0F gap classifications | **G60b passed at `4e5d74d0d9f675df2342353b8bfdbb2e5cded768`** |
| M60c | tasks/M60c_upd9002_fpo2_main_dispatch_audit.md | Audit the main dispatch and FPO2/66/67 target authority without changing instruction semantics | **G60c passed at `e425e55fc17117000ba5178a796de4444d897234`** |
| M60d | tasks/M60d_upd9002_interrupt_frame.md | Resolve only an independently proven synchronous interrupt-frame residual, or close with evidence if none remains | **G60d passed at `8736f8afe6d8eeb58e58c7afdaf5951e2306cb63`** |
| M60e | tasks/M60e_upd9002_iret.md | Correct only evidence-supported IRET semantics after interrupt-frame governance is closed | **G60e passed at `a3915e2bf77bb735bc45a21b05e1f66dc4eb6a5b`** |
| M61 | tasks/M61_upd9002_mov_immediate_register.md | Correct the C6/C7 register-form MOV-immediate family while leaving F7 `/2` for residue planning | **G61 passed at `829f314bb0d363ec5b6e9aa738e948b1a3adb365`** |
| M62 | tasks/M62_upd9002_semantics_bundle.md | One-time maintainer-approved bundle: correct AAM, ROR4, ROL4 activation, BCD/ASCII adjust, and shifts through independently reviewable phase commits | **G62 passed at `70b8e94e96aef4cb79eed72c7813c4148c5c0dd8`** |
| M64 | tasks/M64_upd9002_div_idiv.md | Correct DIV/IDIV and requested SST-covered monitor-authorized 0F families; bind BRKEM's accepted zero-case SST status in a separate authority checkpoint | **G64 passed at `9b151923f9468555043152ffe8651c97b9ecac5b`** |
| M65 | tasks/M65_upd9002_residue_replan.md | Serial residue campaign M65j then M65a–M65m; terminal-only formal approval at terminal campaign gate | **G65 passed; terminal campaign approved** |
| M66a | tasks/M66a_upd9002_drop_cpu286_state_compat.md | Remove obsolete CPU286 save-state compatibility as the first internal checkpoint in the combined M66 bundle | **G66a passed** (internal checkpoint in the combined M66 bundle) |
| M66b | tasks/M66b_upd9002_remove_i286_identity.md | Remove the remaining active I286/i286c identity and close the combined M66 bundle | **G66b passed at `97f760e8da573888edf089c2875c623895a3c2c9`** |
| M67 | tasks/M67_upd9002_divergence_consolidation.md | Consolidate approved divergences, hardware questions, and the final target-policy evidence into `tests/ssts/divergence/g67/registry.json` plus generated compatibility views | **G67 passed at `f8f350e1aadec4b6c79c20192d14c50bd39934be`** |
| M68 | tasks/M68_upd9002_segmented_word_mapped_dispatch.md | Restore canonical mapped-memory dispatch ownership for uPD9002 segmented word access while preserving M65e segment wrapping | **G68 passed at `d1e0225c4edb716893fe5579283fbf0915db72b9`** |
| M69 | tasks/M69_upd9002_idp_0142_status_composition.md | Correct only the IDP/TSP `0142H` status-bit Boolean composition, preserving stored flags while adding dynamic VB | **G69 passed at `680308a603b24341c5b9649657f01791b79002f7`** |
| M70 | tasks/M70_upd9002_prefix_string_closure.md | Implement the maintainer-approved `64H`/`65H` REPNC/REPC prefix plus string-instruction closure for the exact 19-group, 5,908-hash population, with negative protection for prefixed `6C`-`6F` | **G70 passed at `53d47ed500baef247a1be5f3ccc18bdb0c00c0cc`** |
| M71 | tasks/M71_upd9002_core_dispatch_fold.md | Fold the obsolete standalone uPD9002 dispatch translation unit into `upd9002_core.c` and remove current `v30` dispatch/core naming without changing behavior | **G71 passed at `24950894eca79e308afae8d574d43c8f393bb483`** |
| M72 | tasks/M72_misc_compile_flag_cleanup.md | Audit inactive VAEG-irrelevant code, remove only proven-safe inactive cleanup targets, remove About/More 98x1 UI details, fold always-enabled `VAEG_FIX`, keep required SCSI/HOSTFAT paths, remove legacy HOSTDRV, and audit inactive PC-9821/EPSON/`CPUCORE_IA32`/IDE/PC-9861K/`DISABLE_SOUND`/`VAEG_EXT`/font/embed boundaries without changing active behavior outside the explicitly approved cleanup | **G72 passed at `643d9f7289d817c67f343bf01be368b546bc1438`** |
| M73 | tasks/M73_upd9002_post_m49_performance_regression.md | Isolate and, if evidence permits, correct the runtime performance regression observed between the approved M49 and M50 checkpoints before later guest-behavior and source-tree restructuring work | **G73 human gate passed; M73 closed at `d7f1fd4b642ffa1bf71e855502e00341e9f37152`** |
| M74 | tasks/M74_debug_harness.md | Build a reusable, deterministic, default-off emulator debug harness with bounded captures and private-input isolation, without changing guest-visible behavior | **G74 human gate passed; M74 closed at `3785cc115155c52928817b8c95d38b40268a7bde`** |
| M75 | tasks/M75_scsi_support.md | Clean up, validate, and document active PC-9801-55-compatible VA SCSI support with the driver-installed support disk while preserving SASI and HOSTFAT | **G75 passed at `4ddba36f28dbfbe35a52117964b99b5685fdaa3d`** |
| M76 | tasks/M76_upd9002_upd780_emulation_mode_authority.md | Audit uPD9002 main-CPU uPD780 emulation-mode authority and decide whether a later production implementation is safe without repaired-hardware evidence | **G76 passed at `2ef9716d9628ce8eefdf61a1feedca0be5921077`** |
| M77 | tasks/M77_iova_to_io_rename.md | Move `iova/*` into `io/` with rename-only semantics and no behavior change | **G77 passed at `630e8f27fc4f2d574daf7cdc630836964a4247dc`; merged to `main`** |
| M78 | tasks/M78_iova_to_io_reference_fixups.md | Normalize include paths, CMake source lists, and current documentation after the `iova` to `io` move | **G78 passed at `23e9f4673e2e122835a5ad2fb256e6961f860866`** |
| M79 | tasks/M79_va_io_dispatcher_consolidation.md | Make the VA I/O dispatcher canonical and remove the `iocore` / `iocoreva` split where behavior-neutral | **G79 passed at `1e19c4c539fd99dcc7dcd4a92770a51aef93aad1`** |
| M80 | tasks/M80_98_only_io_cleanup.md | Audit and remove proven 98-only `io/` implementations while retaining C-bus boards and deferring FDD320 until 5-inch 2D evidence is resolved | **G80 passed at `c44569bd8c47c87c19c6e59bfb735ce7431102bd`** |
| M81 | tasks/M81_va_bios_reachability_cleanup.md | Audit VA BIOS reachability and remove only proven 98-only BIOS handlers | **G81 human gate passed; M81 closed at `027cd761df98ce00fa1c24501d6233d7faaa0110`** |
| M82 | tasks/M82_upd780_subsystem_cpu_audit.md | Audit the FDC subsystem uPD780-compatible CPU boundary currently implemented through the suzukiplan-backed wrapper | **G82 human gate passed; M82 closed at `788cd90aa07bf1619c47b2f130a2183d4fd7111c`** |
| M83 | tasks/M83_move_upd780_subsystem_cpu.md | Create `cpu/upd780/` and move the FDC subsystem uPD780-compatible CPU wrapper/backend there | **G83 human gate passed; M83 closed at `d90c8721d6120af9994cedb63685e8a60546513e`** |
| M84 | tasks/M84_cpucva_boundary_cleanup.md | M84a: retire the approved non-VA C-bus sound-board dependency closure (`amd98`, `board26k`, `board86`, `board118`, `pcm86io`, and `cs4231io`); M84b: clean up the remaining `cpucva/` boundary while keeping uPD9002 instruction execution and VA memory ownership separate | **G84 human gate passed; M84 closed at `9aeb6512e59da7e794ffede50b7a184f601d137e`** |
| M85 | tasks/M85_state_save_section_cleanup.md | Audit retired state-save sections, remove only approved obsolete sections, and document compatibility behavior | **G85 human gate passed; M85 closed at `0b6633041e2fb8bae8de7efa1a1768dc6c3e5cba`** |
| M86 | tasks/M86_machine_core_relocation.md | Move active root machine-core sources such as `pccore`, `nevent`, `timing`, `calendar`, `keystat`, `statsave`, `debugsub`, and `clockscale` under `machine/` without behavior change | **G86 human gate passed; M86 closed at `74a5eac8bc0fa145fc0c4bf5ed66e3ff5368c0ae`** |
| M87 | tasks/M87_legacy_tool_rom_regeneration_audit.md | Audit remaining legacy tools, ROM/resource regeneration flows, and `lio/` BIOS/LIO compatibility hooks before the final VA-only source-tree audit | **G87 human gate passed; M87 closed at `d2d1a13167ccd094d0fae180c775ad5e1d7eb78e`; merged to `main` at `f876dbbfe4e69f0a2ad2021b289962d15754812d`** |
| M88 | tasks/M88_final_va_only_source_tree_audit.md | Final VA-only active source-tree audit after performance, BASIC, SCSI, uPD9002 emulation-mode authority, I/O, BIOS, uPD780, `cpucva`, state-save, machine-core relocation, legacy tool cleanup, and `lio/` disposition | **G88 human gate passed; M88 closed at [98d7343](https://github.com/nakatamaho/vaeg/commit/98d7343df9c763354e0775bd04a7b6d8d9c6a291); merged to `main` at [b142bc3](https://github.com/nakatamaho/vaeg/commit/b142bc37c4fe0cc50381727eac5766a5b3843e71)** |
| M89 | tasks/M89_merge_va_source_directories.md | Consolidate the active VA BIOS, VRAM, and CPU adapter/backend directories into `bios/`, `vram/`, and `cpu/` without behavior change | **G89 human gate passed; M89 closed at [665877a](https://github.com/nakatamaho/vaeg/commit/665877ab7e0961907a255796b30e7438115c6e51); Hosted CI [31577266904](https://github.com/nakatamaho/vaeg/actions/runs/31577266904) passed; merged to `main` at [5b4a22b](https://github.com/nakatamaho/vaeg/commit/5b4a22ba4e8a4fc7ef44f3d2dfcfe4c1001cde97)** |
| M90 | tasks/M90_va_ems_board.md | Enable the retained VA EMS page-frame board with 1MB capacity units, GUI/configuration support, an EMMVA/SQEMM98/RDEMS bootable supplemental-disk workflow, and redistributable HOSTFAT/SQEMM98 release assets | **G90 human gate passed; M90 closed at [f16d6af](https://github.com/nakatamaho/vaeg/commit/f16d6af14039359e5b617d757f906a47f45b1ad9); final 10-job Hosted CI [31679653128](https://github.com/nakatamaho/vaeg/actions/runs/31679653128) passed** |
| M91 | tasks/M91_va_single_path.md | Remove retired PC-98 machine-mode routing and use one native V3 VA I/O, memory, CGROM, and initialization path | **G91 human gate passed; M91 closed at [66fbf20](https://github.com/nakatamaho/vaeg/commit/66fbf20f6f5a8fbe107884ca1223acc53d352cbd)** |
| M92 | tasks/M92_clang_format.md | Establish a pinned clang-format 22 policy and mechanically normalize the active first-party C/C++ source set | **G92 human gate passed; M92 closed at [caa1f40](https://github.com/nakatamaho/vaeg/commit/caa1f403cd0c1f6ce7673d6f839de7d3932c5316)** |
| M96 | tasks/M96_va_only_structural_cleanup.md | Audit and simplify the VA-only source tree in staged, evidence-backed cleanup stages while preserving live C-bus and VA hardware boundaries | **G96 human gate passed; M96 closed at `74c859111951f22e86b2bd6453a5f07b040fd6da`** |
| M97 | tasks/M97_sgp_tekumani_commands.md | Complete manual-derived SGP descriptor, LINE, SCAN, ROP, and transparency semantics and add an isolated LINE visual test without timing or real-hardware claims | **G97 human gate passed on 2026-08-24; M97 closed after candidate `7ab4fabfff8ace9f7ae8648cee4004ad6d507b94`** |
| M98 | tasks/M98_zundamon_orbit_master_plan.md | Build an isolated 320x200 billboard-orbit demo around a generic local indexed-image input, a shared 30-scale single-bank BMS atlas, and deterministic 1-16 instance SGP composition | **M98 reassigned to Zundamon orbit; G98a, G98e, G98j, G98k, G98l, G98o, G98p, G98q, G98r, G98s, G98t, G98v, G98w, G98x, G98y, G98z, and G98aa human gates and G98b-G98d, G98f-G98i, and G98u machine gates passed; M98aa closed (IDA 1-64 animation), M98m/M98n remain absorbed reservations** |
| M100s2 | tasks/M100s2_recording_h264_boundary.md | Native FFV1/PCM/MKV lossless recording, one-click controls, rotation/geometry/reset file splits and static desktop dependencies | **G100s2 native archive human gate passed by explicit maintainer acceptance; [audio correction942a831a](https://github.com/nakatamaho/vaeg/commit/942a831ab68452d4148da604a1a7b2c1b7f878ea); [main merge daec0356](https://github.com/nakatamaho/vaeg/commit/daec03567a1842ba3b00aee1e25001e65dccd578). Metal deferred; real-time H.264/provider and distribution gates remain separate.** |
| M100s9 | tasks/M100s9_recording_presenter_readback.md | Actual final-client readback and guest-time Displayed FFV1 integration | **G100s9 human gate passed by explicit maintainer acceptance of a4c0b6a0/03faab3c; [gate record 1325121e](https://github.com/nakatamaho/vaeg/commit/1325121eb0ffa107a9eacfc19615292788673d7a). Merged to main at [4b487028](https://github.com/nakatamaho/vaeg/commit/4b4870281afed01e723834e5f36075f68635340f). IMPLEMENTED at [73948eaa](https://github.com/nakatamaho/vaeg/commit/73948eaa511d3a97896155a1d05af864e818b535): GUI/CLI source selection, lifecycle, exact actual Metal GUI/RGB/PCM parity, ON/OFF/strict/sanitizer and static macOS/clean MinGW compile/import checks. Latest-main integration [bc0198b9](https://github.com/nakatamaho/vaeg/commit/bc0198b9dbdd919d1046bd94081812affd18194c) imports f25cda38 and repeats builds/runtime checks with new macOS/MinGW artifacts. [a4c0b6a0](https://github.com/nakatamaho/vaeg/commit/a4c0b6a02fdd616071044c39d49c1fe6c1ccb162) persists the GUI recording source, with config/restart/CLI isolation checks and refreshed macOS/clean MinGW artifacts. Unavailable local media/PSD/DOSBox regeneration checks remain NOT_RUN/BLOCKED, not inferred PASS. H.264/Windows standard-API, other platform/device and distribution/legal gates remain separate. Accepted handoffs, evidence and expensive SDK results are retained before scoped cleanup.** |
| M100s11 | tasks/M100s11_recording_post_mp4.md | Optional background post-recording H.264/AAC MP4; macOS/Windows OS APIs, Linux local-only static encoders | **G100s11 PASSED by explicit maintainer "passed!" after the final fef92dea checklist / clean static Windows 16ed03ee handoff; persistent compression notification, native 60 Hz timing/retry, cancellation cleanup and screenshot rotation complete; macOS/Linux export/decode and full six-component demo/media source closure verified, utility FDD and both SASI model boots/DIR plus CP/M generation/boot recorded. Windows playback/seek and V3/demo/OS acceptance are maintainer-reported; physical Linux GPU/Wayland, missing-MF and independent Windows decode/quality/probe measurements remain unverified. Merged with `--no-ff` at [9920f86d](https://github.com/nakatamaho/vaeg/commit/9920f86d9f776b205c5e3bd42a40a63725e8af0a); integration macOS ON/OFF CTest 140/131 listed, zero failed (nine skips each). Original CI 39723588276 retains eight successful jobs and two fixture/archive failures; corrections are verified by [scoped CI 39794085772](https://github.com/nakatamaho/vaeg/actions/runs/39794085772) on [81f4314a](https://github.com/nakatamaho/vaeg/commit/81f4314a1d47ec87bb6d32db0221f126d8b9789e): both jobs pass, native Windows policy/static 21/8 and source content 17 plus actual archive; exact fixture-only scope preserves production/SDK inputs. CI follow-up complete, not a newly repeated full pipeline. Prior artifacts/SDKs/evidence retained; no legal/distribution approval or next milestone.** |
| M100s10 | tasks/M100s10_recording_displayed_windows_linux.md | Windows/MinGW and Linux final-client Displayed FFV1/PCM archive recording | **G100s10 PASSED by explicit maintainer acceptance of the ce62cacd/37338696 handoff; merged to main at [25bbca7d](https://github.com/nakatamaho/vaeg/commit/25bbca7daa096610c59c8152cdd037feb65d6ae3); accepted artifacts/SDK/evidence preserved before scoped cleanup; client implementation and clean static recording-ON MinGW build complete at [ce62cacd](https://github.com/nakatamaho/vaeg/commit/ce62cacd32885f5980c98d264c6e857b8c9c2795). Linux SDL/native GL recording/decode and ON/OFF/strict/sanitizer checks pass; native Windows runtime, physical Linux GPU/Wayland and private-media gates remain unverified.** |
| M101 | tasks/M101_upd9002_flag_profile.md | Add the µPD9002 Z80-emulation-mode flag profile (R1–R7) beside the Zilog reference profile, import ZEX for user-built CP/MVA disks, and build real-machine flag probes | **G101 human gate passed on 2026-09-30; M101 complete (gate build [e3f63d7](https://github.com/nakatamaho/vaeg/commit/e3f63d7d4c4a86558f9292a938a7a62fbb85853b))** |
| M102 | tasks/M102_upd9002_undocumented_opcodes.md | Close O3 (INC/DEC IYH/IYL), fix the null undefined-opcode handlers of the vendored Z80 core, and test undocumented DDCB/FDCB, NEG, prefix and undefined ED opcodes of the µPD9002 Z80 emulation mode on a real PC-88VA2 | **G102 human gate passed on 2026-10-02 (gate build [077718b](https://github.com/nakatamaho/vaeg/commit/077718baab2b0e73c0db9a01f982e053bb7cb10c)); M102 complete; merged to `main` at [659a01e](https://github.com/nakatamaho/vaeg/commit/659a01e16f5e0c7b79e314b5cbdc0a3e12cddcca)** |
| M103a | tasks/M103a_alternate_register_storage.md | **V1/V2 series (ADR-0016), shared prerequisite:** give the µPD9002 alternate register set (`AF'`/`BC'`/`DE'`/`HL'`) machine-owned storage in the native state image with a defined lifetime, measure its persistence across a CALLN round trip on the real PC-88VA2 (`ALTPRB`), and fix the `StateLoad` native-vector-reader omission | **G103a human gate passed on 2026-10-03 (gate build [bb21713](https://github.com/nakatamaho/vaeg/commit/bb217138bb039fdec3f57d5bd48af69f0cd58f07)); M103a complete; candidate [719f839](https://github.com/nakatamaho/vaeg/commit/719f83950a5d3ce723f175dd3ac4babc9e24e759); merged to `main` at [23dd911](https://github.com/nakatamaho/vaeg/commit/23dd911e7ca1b14d757444ae4db0be37b273de30)** |
| M103b | tasks/M103b_v1v2_mode_plumbing.md | V1/V2 series: `BRKEM2`, boot inputs (PC key and SW7), `153H` bit 6 and the 88-mode window with the hardware banking ports, I/O trap hardware; ROM-derived window map; trace-verified automatic FDD selection and handoff into N88-BASIC without a mode override | **G103b human gate passed on 2026-10-03; M103b complete: disk BASIC reaches its key-input wait (trace-verified, romful test); merged to `main` at [a572079](https://github.com/nakatamaho/vaeg/commit/a572079c970a7776c2970844d9d9d29e197b2641)** |
| M103c | tasks/M103c_v1v2_interrupts_and_text.md | V1/V2 series: 8214-mode interrupt controller with µPD780/V30 vector selection, 600 Hz general timer 2, keyboard reaching BASIC, 88-mode TVRAM and TSP 3301-compatible text display | **G103c human gate passed on 2026-10-03 (gate build [411e6c8](https://github.com/nakatamaho/vaeg/commit/411e6c8802c298d01a6b344208238ac0c20eee9a)); M103c complete: V2 disk BASIC prompt, typing, MON; merged to `main` at [91e3140](https://github.com/nakatamaho/vaeg/commit/91e3140535f14c5b2bab889d1dbfd4b7948327c2)** |
| M103d | tasks/M103d_v1v2_graphics_and_disk.md | V1/V2 series: extended GVRAM access (ALU, ports 32h/34h/35h), 88-mode graphics display (port 31h, palette mode, colour and monochrome), disk BASIC `FILES`/`SAVE`/`LOAD` → N88-DISK BASIC V2 usable with graphics statements | **G103d human gate passed on 2026-10-04; M103d complete: V2 BASIC colour and monochrome graphics, disk commands; merged to `main` at [98a2333](https://github.com/nakatamaho/vaeg/commit/98a233335fc66c9e03849b51fc42adf0e3d7170b)** |
| M103e | tasks/M103e_v1v2_demo_compatibility.md | V1/V2 series: compatibility against PC-8801 demonstration software (kanji ROM ports, extended RAM, dictionary ROM window, D88 track-table fix; SR-DEMO, MA2-DEMO and GAME-A run) | **G103e human gate passed on 2026-10-04; M103e complete: SR-DEMO, MA2-DEMO and GAME-A run; merged to `main` at [1520c42](https://github.com/nakatamaho/vaeg/commit/1520c42994ce43f81b3d96d58af6aaf6b151d7e7)** |
| M103f | tasks/M103f_v1v2_text_and_media.md | V1/V2 series: TSP byte-mode start addresses (text after a 3301 RESET), selecting an image of a multi-image D88 (`--fdd1-image`/`--fdd2-image`) | **G103f human gate passed on 2026-10-04; M103f complete: byte-mode text addressing, multi-image D88; merged to `main` at [8b446b1](https://github.com/nakatamaho/vaeg/commit/8b446b131413d6d71fa9a50ce69bd75f3523f942)** |
| M103g | tasks/M103g_v1v2_semigraphics_and_sound.md | V1/V2 series: µPD3301 semigraphics, 40-column text, SR-DEMO sound timing | **G103g human gate passed on 2026-10-04; M103g complete: semigraphics titles, 40 columns, winter-scene music; merged to `main` at [375d96b](https://github.com/nakatamaho/vaeg/commit/375d96ba41819e13c504f429e9d575f6f16c1ba8)** |
| M103h | tasks/M103h_v1v2_nbasic.md | V1/V2 series: Z80 mode menu (V1/V2, H/S), N-BASIC from a user-supplied ROM via a start-up option and `NEW ON 1` | **G103h human gate passed on 2026-10-05; M103h complete: Z80 mode menu, N mode boot of N-BASIC (known dumps by SHA-1), original-VA cursor (TSP CURS), V1/V2 text copy; merged to `main` at [0fecd39](https://github.com/nakatamaho/vaeg/commit/0fecd39f83973117cbec9466f9329f679bb151bc)** |
| M103i | tasks/M103i_v1_mode.md | V1/V2 series: V1 mode with V1 media, compared with the real VA | **G103i human gate passed on 2026-10-05; M103i complete: V1 media checked against the real VA, monitor switches (15 kHz, digital RGB), port 40h buzzer and port sound, fine buzzer volume; merged to `main` at [a727a45](https://github.com/nakatamaho/vaeg/commit/a727a458a112d90c721d78f8db9d4580953c18b1)** |
| M103j | tasks/M103j_cassette_tape.md | V1/V2 series, closing milestone: cassette tape images for V1/V2 BASIC (vaeg extension; the VA has no cassette interface) | **G103j human gate passed on 2026-10-05; M103j complete: cassette tape images in V1/V2 BASIC with tape sound (vaeg extension), unformatted 2DD 720 KB / 2D 320 KB / 2D 360 KB new FDD images; merged to `main` at [b1888e0](https://github.com/nakatamaho/vaeg/commit/b1888e04bc6a886b056dc2c9b467a5e66b77d402)** |
| M104 | tasks/M104_v3_display_timing.md | V3 series: display timing beyond the documented modes (480-line frames, 15 kHz 240 lines, 24.8 kHz line doubling) measured on real hardware and implemented | **G104 human gate passed on 2026-10-07; M104 complete: 480-line and 15 kHz 240-line frames measured on a PC-88VA2 and shown in vaeg (480-row canvas, 15.98 kHz graphics start fixed to sync, 24.8 kHz doubling stop after line 200, SYNC stops the TSP text), undocumented TSP modes probed (not usable), N-mode disk boot fixed; merged to `main` at [2bec316](https://github.com/nakatamaho/vaeg/commit/2bec31628adb1b9cfcfd0b9bce623a5f2fb6fc00)** |
| M105 | tasks/M105_scanline_fill_tape_leader.md | 15 kHz scanline fill setting (CRT gaps or LCD fill), the cassette leader sound (low then high before `Found`), frames near 60 Hz measured, an emulation speed slider, a CPU speed slider, and a PC-8801-style V1/V2 display option | **G105 human gate passed on 2026-10-07; M105 complete: 15 kHz scanline fill setting, tape leader sound (C11), frames near 60 Hz measured on a PC-88VA2, sync shorter than 4 lines counted as written, emulation speed and CPU speed sliders, PC-8801-style V1/V2 display (C12); merged to `main` at [9b4f8ef](https://github.com/nakatamaho/vaeg/commit/9b4f8ef8b08084fec69a31315af20e28f4c77d9b)** |
| M105a | tasks/M105a_v480pat_utility_media.md | Source-built V480PAT, source and instructions on utility FDD and both SASI HDD variants | **G105a human gate passed (maintainer); M105a complete: source-built V480PAT, source and instructions packaged on the utility media; merged to `main` by [PR #23](https://github.com/nakatamaho/vaeg/pull/23) at [7b6d1ff](https://github.com/nakatamaho/vaeg/commit/7b6d1ff283aa4448986dbc6a4d18a0340e3d5fd7). V480PAT was later kept off the utility FDD by M110 ([dd7a661](https://github.com/nakatamaho/vaeg/commit/dd7a661e23164385b3995091db35b827d41bd3cc)) so that the FDD builds again** |
| M106 | tasks/M106_bios_int80_documentation.md | Publish static INT 20H/21H DOS and INT 90H/91H V1/V2 references, INT 80H–83H details and chapter 6.5–6.20 interrupt references with conventional-memory work notes and a complete `docs/bios/index.md`; establish canonical placement in `AGENTS.md` | **G106 passed** (documentation-only machine gate; runtime verification remains open) |
| M106 | tasks/M106_sgp_bootable_demos.md | Self-booting SGP demo disk built from source (IPL, menu loader, minimal INT 21h), for real-hardware runs without PC-Engine | **G106 human gate passed on 2026-10-14; M106 complete: self-booting SGP demo disk (used for hardware QA), demos assembled for the 80186 (`0F 8x` near jumps removed; vaeg halts on them), SGP LINE directions as measured, NEON3 rebuilt from its public sources, MS-DOS 4.0 all-demos disk, fixed-disk boot following the attached HDD, tape timing and archives, utility media with VZ Editor, front-panel lamp bar; merged to `main` at [2c2ec50](https://github.com/nakatamaho/vaeg/commit/2c2ec5030e4ff7f8bf5b4bfeb25402e72714d0aa)** |
| M106a | tasks/M106a_sgp_line_demofix.md | Backport hardware-measured SGP LINE directions and rebuild affected demo distributions from source | **G106a passed on 2026-10-15 (maintainer); M106a complete: SGP LINE directions as measured and the rebuilt LINE demos, merged to `main` by [PR #20](https://github.com/nakatamaho/vaeg/pull/20) and confirmed on hardware within the M106 human gate** |
| M107 | tasks/M107_visual_audio_voyage.md | Original V3 Visual Audio Voyage short prototype: undulating perspective sea, camera bank/heave, SGP rasterization, FM/SSG event synchronization and safe local guest workflow | **G107 passed** (maintainer prototype approval; OPNA follow-up authorized; no new real-hardware/regression claims; no emulator changes) |
| M107a | tasks/M107a_voyage_opna_arrangement.md | Original six-FM/three-SSG arrangement and source-built ADPCM kick/snare/sweep preparation; VOYAGE guest sources unchanged | **G107a passed** (machine preparation checkpoint; OPNA guest integration/audition remain separate) |
| M107b | tasks/M107b_voyage_opna_playback.md | Separate VOYOPNA guest integration: six FM voices, three SSG tones and original ADPCM-B percussion; bounded upload, shared cues and shutdown | **G107b human gate passed on 2026-10-16; M107b complete: VOYOPNA plays six FM voices, three SSG tones and ADPCM percussion on the VA2 (slight visible lag noted); the legacy VOYAGE now uses the measured SGP LINE direction bits, merged to `main` at [e6076a5](https://github.com/nakatamaho/vaeg/commit/e6076a52ca169c98f8ddc8077935c2b0498bf347)** |
| M108 | tasks/M108_cpu_conformance_checks.md | Per-role CPU conformance checks: uPD9002 native mode against a SingleStepTests V20 failing-record baseline, uPD70008-compatible mode against real PC-88VA2 ZEX CRCs, FDC uPD780C against stock Z80 ZEX CRCs; role core configuration shared by production and tests | **G108 machine-verifiable gate passed on 2026-10-15 (maintainer); M108 complete, no guest-visible behaviour change; merged to `main` by [PR #28](https://github.com/nakatamaho/vaeg/pull/28) and [PR #29](https://github.com/nakatamaho/vaeg/pull/29)** |
| M109 | tasks/M109_demo_display_fixes.md | Demo fixes from the hardware runs (ZUNDAORB without bank memory, HUD spacing, up to 1024 balls), 200-line modes without scanlines at 24 kHz, kana and Caps Lock indicators | **G109 human gate passed on 2026-10-14; M109 complete: ZUNDAORB without bank memory (512 KiB) with the PSD-built Zundamon art, up to 1024 balls in all pseudo-sprite demos, 200-line demo modes line-doubled at 24 kHz, KANA and CAPS lamps, V480PAT/TSPMODE/TSPFILL on the distributable self-booting disk, digital RGB monitor threshold from hardware photographs, tape fast load off by default; merged to `main` at [e9e8e2c](https://github.com/nakatamaho/vaeg/commit/e9e8e2c9cce92391fb9d870e321686c8c9debff0)** |
| M109a | tasks/M109a_sgp_fbw_and_sgptest.md | SGP pitch (FBW) read as on the VA2 also on the original VA (striped sprites in VA mode); SGPTEST, an SGP regression test for vaeg and hardware, on the self-booting disk | **G109a human gate passed on 2026-10-16; M109a complete: SGP FBW decoded alike on both models (no striped reversed sprites on the VA), SGPTEST and SGPDUMP on the self-booting SGP disk (hardware: all tests equal except two FBW values whose results vary), the menu cursor turned off; merged to `main` at [823996f](https://github.com/nakatamaho/vaeg/commit/823996f8db3a9d4bf5046989675285eb4c6aa18b)** |
| M110 | tasks/M110_menu_reorganization.md | Menu reorganization: 模擬 and nine further top menus, unused and always-greyed items removed, Pause as an F12 assignment, one speed control, 8087 on by default, cursor-key setting; plan for machine ▶ boot mode, extra ROMs, PC-8801/8001 machines and PCG (M111+) | **G110 human gate passed on 2026-10-15; M110 complete: 模擬 and nine further top menus, Pause as an F12 assignment, dialogs that fit the window, per-software state slots with thumbnails, master and per-source volume, CPU clock presets, histories and recent boots, 8087 on by default; merged to `main` at [6f15dc5](https://github.com/nakatamaho/vaeg/commit/6f15dc5fcc9c16668366ce54ba9dd578e36f134f)** |
| M111 | tasks/M111_boot_modes.md | Boot modes: 起動 as machine ▶ mode with 自動, V1/V2 H/S started even with a V3 disk or no disk (SW7 OFF until compatible code runs), and N; `BootMode` replaces `V1V2_NMode` | **G111 human gate passed on 2026-10-15; M111 complete: 起動 as machine and mode with 自動, V1/V2 H/S started even from a V3 disk or no disk (SW7 off until compatible code runs), and N; `BootMode` replaces `V1V2_NMode`; merged to `main` at [f5fced4](https://github.com/nakatamaho/vaeg/commit/f5fced4268514ba46936a980be1fc996bb403828)** |
| M112 | tasks/M112_extra_roms.md | Extra ROMs: `extra/<machine>/<role>.rom` next to the executable (or a combined `PC88.ROM`), any revision accepted, named from MAME's published labels or its banner, missing roles shown per model, revisions selectable under 起動, `--list-extra-roms` | **G112 human gate passed on 2026-10-15; M112 complete: extra/<model>/<role>.rom or a combined `PC88.ROM`, any revision accepted, named from MAME's published identities or the ROM banner, revisions selectable under 起動, `--list-extra-roms`; merged to `main` at [4aa0f5b](https://github.com/nakatamaho/vaeg/commit/4aa0f5b6bb18dc862afdf154a6d36415b6a058c2)** |
| M113 | tasks/M113_pc8801_machines.md | PC-8801 machines from the M112 extra ROMs, PC-8801MA first: V2 BASIC prompt and FDD boot, the PC-8801 without an FDD unit, a set derived from the VA ROMs, kanji level-2 order check | **G113 human gate passed on 2026-10-15; M113 complete: PC-8801 models under extra/ started in the VA's V1/V2 mode with their own ROMs (missing roles allowed except n88), no FDD unit without disk.rom, a set from the VA ROMs, level-2 kanji order fix, shorter 起動 and 画面 menus, one unformatted new FDD image per media type; merged to `main` at [3ad13c2](https://github.com/nakatamaho/vaeg/commit/3ad13c2e1f454fe38647bfc77e1d82ed6195bbd4)** |
| M114 | tasks/M114_pc8001_machines.md | PC-8001, PC-8001mkII and mkIISR from the M112 extra ROMs, in the VA's N mode with their own N-BASIC, at 15 kHz, port 31h with its mkII meaning | **G114 human gate passed on 2026-10-16; M114 complete: PC-8001, PC-8001mkII and mkIISR in the VA's N mode with their own N-BASIC, at 15 kHz, port 31h with its mkII meaning; merged to `main` at [6f33314](https://github.com/nakatamaho/vaeg/commit/6f33314049c7638dd8136eb5a3e2e7d877682847)** |
| M115 | tasks/M115_pcg.md | PCG-8100/8800 ported from X88000 (public domain): 256 user-definable 8x8 characters and the 8253 three-voice tone generator in V1/V2 mode, as a configuration option | **G115 human gate passed on 2026-10-16; M115 complete: PCG-8100/8800 ported from X88000 (characters and three tones in V1/V2 mode, an option), the Configure dialog sized to its contents, paste paced by guest frames; merged to `main` at [5e25777](https://github.com/nakatamaho/vaeg/commit/5e25777ecd9065cb941c243a18dbcf61a18331bd)** |
| M115a | tasks/M115a_pcg_quasi88.md | PCG behaviour checked against QUASI88 (store on port 02h only, 8253 at 3.9936 MHz with counter modes), band-limited tones, X88000 and QUASI88 credited under About | **G115a human gate passed on 2026-10-16; M115a complete: PCG RAM stored on port 02h only, 8253 at 3.9936 MHz with counter modes and band-limited tones (issue #41), X88000 and QUASI88 credited under About; merged to `main` at [e492658](https://github.com/nakatamaho/vaeg/commit/e492658c044d257a054c5667b938c3c24036b306)** |
| M116 | tasks/M116_recent_boots.md | 最近の起動: archive disks recorded as their archive, missing files shown, no silent reset into the previous disk; host pacing label in Japanese | **G116 human gate passed on 2026-10-16; M116 complete: archive disks recorded as their archive, missing files shown on hover, no silent reset into the previous disk, host pacing label in Japanese; merged to `main` at [5f6b033](https://github.com/nakatamaho/vaeg/commit/5f6b0331abf37b2106c6f700568a214e1bc02b35)** |
| M115b | tasks/M115b_pcg_sample.md | PCG-8100/8800 sample BASIC program in `demos/pcg-sample` | **G115b human gate passed on 2026-10-16; M115b complete: PCG-8100/8800 sample program in `demos/pcg-sample`; merged to `main` at [70d506b](https://github.com/nakatamaho/vaeg/commit/70d506bf40b0b1e03674472cb68da5eeaeefe50e)** |
| M117 | tasks/M117_tsp_attribute_blink.md | TSP attribute modes 4-7 decoded, including blink; screen turned 90 degrees right | **G117 human gate passed on 2026-10-16; M117 complete: TSP attribute modes 2-7 decoded (blink, double-width halves, record stream), the cursor hidden without DSPON, the screen turned 90 degrees right; merged to `main` at [2c95625](https://github.com/nakatamaho/vaeg/commit/2c95625ae5010704e867941ab27f690ed01bfc50)** |
| M118 | tasks/M118_release_notes_20261017.md | Rel.20261017 release notes and README features and News | **G118 human gate passed on 2026-10-16; M118 complete: `CHANGES.20261017.md` with screenshots, README features for the V1/V2 and N modes and a Rel.20261017 News entry; Rel.20261017 published (tag `rel-20261017`); merged to `main` at [c5d518f](https://github.com/nakatamaho/vaeg/commit/c5d518f53603f15ac9c5b899103344668c788adf)** |
| M119 | tasks/M119_markdown_comment_check.md | Repository check for Markdown HTML comments that hide content (unclosed or hiding a heading) | **G119 machine-verifiable gate passed on 2026-10-17; M119 complete: `tools/repo/check_md_comments.py` reports Markdown HTML block comments that never close or hide a heading, run in CTest and the CI lint job; merged to `main` at [ad4a606](https://github.com/nakatamaho/vaeg/commit/ad4a606e6b690c918f2482bcbf9acadc900350d4)** |
| M120 | tasks/M120_d88_multi_image.md | Multi-image D88 disks chosen from the FDD menus and kept in states, `vaeg.cfg` and recent boots; release Rel.20261017.1 named from its tag | **G120 human gate passed on 2026-10-17; M120 complete: multi-image D88 disks (disk B into FDD2 on opening, FDD1/FDD2 ディスク, kept over resets, restarts, states and recent boots), release name from a `rel-*` tag, Rel.20261017.1 (later merged into the Rel.20261017 re-release); merged to `main` at [bc5d913](https://github.com/nakatamaho/vaeg/commit/bc5d9134a9c271ecdb24c23a59dd3880334ffb25)** |
| M122 | tasks/M122_recent_folders.md | Separate recent folders for the FDD, HDD and tape file windows (borrowing in the order FDD, HDD, tape); `GUI_fdddir`/`GUI_hdddir` dropped | **G122 human gate passed on 2026-10-17; M122 complete: separate recent folders for the FDD, HDD and tape windows (borrowing FDD, HDD, tape), `GUI_fdddir`/`GUI_hdddir` dropped, released in the consolidated Rel.20261017 re-release; merged to `main` at [0e923b4](https://github.com/nakatamaho/vaeg/commit/0e923b428a88dd02bb306b82261a240be73226a3)** |
| M123 | tasks/M123_8087_host_x87.md | 8087 FPTAN/FPATAN/F2XM1/FYL2X/FYL2XP1 on the host x87 (inline assembly) on x86 hosts, `NDP8087HostFPU` | **G123 human gate passed on 2026-10-18; M123 complete: 8087 transcendentals on the host x87 on x86 hosts (`NDP8087HostFPU`), the precision-control mapping fixed (64-bit default had rounded to 53 bits), accurate SoftFloat FPATAN/F2XM1 series, `demos/fpu-test`; merged to `main` at [a50f1db](https://github.com/nakatamaho/vaeg/commit/a50f1db7e378621121cae79db33ee79775e3a427)** |
| M124 | tasks/M124_recent_boot_display.md | Monitor (24/15 kHz), and the window size, renderer, scaling and rotation a session ends with, kept in 最近の起動; two-disk archives restored as A and B | **G124 human gate passed on 2026-10-18; M124 complete: monitor and the session-end window size, renderer, scaling and rotation kept in 最近の起動, two-disk archives restored as A and B; merged to `main` at [a892699](https://github.com/nakatamaho/vaeg/commit/a8926994230297f57e7740aa5e3e4601d09ae84c)** |
| M125 | tasks/M125_quick_save.md | Quick save to five rotating states on F12, quick load on Shift+F12 | **G125 human gate passed on 2026-10-19; merged to main at [75bd10f6](https://github.com/nakatamaho/vaeg/commit/75bd10f69b4751d81ab1c0aeb54cbe7b6ab83bde)** |
| M126 | tasks/M126_utility_softlib.md | Requested Softlib packages and FPUTEST/MANDEL BASIC samples on both utility HDDs; M125 explicitly on hold | **G126 machine packaging gate passed; merged to main at [9d3b927c](https://github.com/nakatamaho/vaeg/commit/9d3b927cdebcc4788efb1c44f09bb68b30171fb2)** |
| M127 | tasks/M127_letter_keypad.md | Independent YUI/HJK/NM,. letter keypad below Key / かな入力; requested FDD XZ filter and partially missing recent-boot fixes | **G127 human gate passed on 2026-10-19; merged to main at [c7a14dd0](https://github.com/nakatamaho/vaeg/commit/c7a14dd03b071c7cc3e67fb2ed576e69f6f6e5c1)** |
| M128 | tasks/M128_sgp_negative_fbw_timing.md | SGP: negative pitches FFF6h/FFF2h/FFF0h undefined (garbled on VA and VA2); setup, BITBLT and CLS costs calibrated per model to SGPBENCH 2 | **G128 human gate passed on 2026-10-19; M128 complete: negative SGP pitches with low bits 10b and FFF0h garble after the first line as on hardware, setup/BITBLT/CLS costs calibrated per model to SGPBENCH; merged to main at [5767d04](https://github.com/nakatamaho/vaeg/commit/5767d049921aa82c5a275cb53dcc090d79b455ce)** |
| M133 | tasks/M133_hidden_text_graphics_colour.md | PR #56: preserve attribute colours for text-merged monochrome graphics while characters are hidden | **G133 human gate PASSED** for candidate [bcfb11aa](https://github.com/nakatamaho/vaeg/commit/bcfb11aa74b7856c10e9ce8868897673cde7dfc3), using the clean static recording/OS-MP4-enabled Windows handoff; explicit maintainer merge request. Candidate CI/build passed; new raster regression tests, latest-main runtime and task-specific demo/media regeneration NOT_RUN, not inferred PASS. Merged with `--no-ff` at [af995613](https://github.com/nakatamaho/vaeg/commit/af99561345dc408ecb502ab40a7f2c0ae16c75c0). Accepted handoff retained. See task record. |
| M132 | tasks/M132_hs_clock_status.md | PR #55-based H/S clock and IN6Eh correction; formal machine/mode status, short clocks/tooltips and independent default-ON information bar | **Implementation started from the accepted M131 baseline; original PR #55 source/history integrated and agreed reset/timing/status corrections implemented. Local unit, bundled-demo/OS headless checks, full PSD/demo/media source closure and current recording/OS-MP4-enabled clean static MinGW handoff complete; machine-owned virtual-X11 capture/reference automation completes 64 sessions with 32/32 exact matches against explicitly approved references (X11 suite PASS). Maintainer confirms the current Windows download/startup/recording/post-stop MP4/playback workflow. Requested machine QA now passes actual state/input/reset/clock operations, refreshed eight-case M131 probe and guest-start regression, and software-X11 tooltip/FPS/FRAME/menu/restart actions; local full tests execute 138 PASS with nine SKIPs and zero failures. Final composed CI coverage passes: eight successful original jobs retained, corrected ASan/native-Windows follow-up SUCCESS; original FAIL preserved. A fresh recording-enabled, bundled-local-encoders-OFF static MinGW handoff of the corrected pushed commit is complete. Native CRT/Cocoa/physical HiDPI automation remains BLOCKED, not PASS. Design in docs/modernization/v1v2-hs-clock-status-plan.md. **G132 PASSED** by explicit maintainer `G132 passed` acceptance; [gate record 02de073d](https://github.com/nakatamaho/vaeg/commit/02de073dfcad622d4c87d4c51f160a55448cf257), merged with `--no-ff` at [d94188e8](https://github.com/nakatamaho/vaeg/commit/d94188e85c09a48700d5f90942e8f8e1e14f3bf1) ([PR #63](https://github.com/nakatamaho/vaeg/pull/63)). Updated-main cache policy preserved and focused integration tests pass; emulator/build inputs unchanged. Private evidence/received inputs and all historical handoffs are archived outside Git; final executable/report retained in the work-tree-bounded release handoff. Post-gate cleanup completed after the pushed merge/roadmap record: contained local branch, temporary merge worktree, private scratch and four static build directories removed; main worktree, remote branch, archived evidence/received inputs, final handoff and reusable shader SDK retained. #56 excluded, #58/#59 uncertainty and documented unverified automation remain unchanged; no next milestone started.** |
| M131 | tasks/M131_guest_start_display_corruption.md | Correct severe display corruption after Enter at Start in private fixture guest-start-01 | **Hardware-driven correction [de47a4c0](https://github.com/nakatamaho/vaeg/commit/de47a4c0af20f457c7bc114951e44ce6222e3f68) keeps distinct CPU text banks and A/A display in all four modes and preserves IN31 F9h/79h even in S, matching the eight hardware probe cases. Controlled red/green selftests and CTest pass (140 listed, zero failed, nine skipped); 24 isolated runtime cases pass, including coherent gameplay in V1S, BASIC PRINT and all eight probes. H/S timing and IN6Eh remain separate issues #58/#59. G131 requested after accepted M130 integration at [3326349a](https://github.com/nakatamaho/vaeg/commit/3326349ae4b9f6ac352f5aabc07891b0009a407a): renewed CTest/24-case matrix, nine-distribution payload closure, utility FDD and both model-specific SASI boots/DIR, 34 all-demos drawing cases, BASIC Mandelbrot and self-booting SGP verified. Current clean static Windows handoff available. Full formatting retains baseline drift; all 89 M131 edited lines pass. **G131 PASSED** by explicit maintainer `gate passed` acceptance of the 3326349a clean static Windows handoff and 5888d899 preparation record. M131 closed; [gate record 00f22844](https://github.com/nakatamaho/vaeg/commit/00f22844570cc84a44718b1a19dbe8cf53a4c01a), merged to main with `--no-ff` at [93355b69](https://github.com/nakatamaho/vaeg/commit/93355b695b2976a5943432a8f91b7a9472c4a203). Documented format debt/skips remain unchanged; #58/#59 remain open.** |
| M129 | tasks/M129_tsp_sync_global_reverse.md | TSP SYNC global reverse (RV) in both text renderers (PR #31); PC-8801 models ignore it with monochrome graphics | **G129 human gate passed on 2026-10-21; M129 complete: TSP SYNC.RV in both text renderers (PR #31, shitamo); VA/VA2 apply it in every graphics mode, PC-8801 models ignore it with monochrome graphics, the PC-8001 family always applies it; merged to `main` at [b8b4cbf](https://github.com/nakatamaho/vaeg/commit/b8b4cbf98dda4dfdee8561e0cf56b9261d313459)** |
| M130 | tasks/M130_v30_timing.md | Main-CPU timing of the uPD70116 (V30): manual clock counts, prefixes, odd-address cycles, prefetch queue, WCU programmed waits (VA extended RAM 1), GVRAM waits; #53; CPU/SGP contention as a time share (#38), demo sgp-contention | **G130 human gate passed on 2026-10-23; M130 complete: V30 clock counts, prefetch queue, WCU and GVRAM waits within 0.95-1.09 of the VA and VA2 for CPU loops (SPRBENCH 1.10-1.13 left in #47); CPU/SGP contention as a per-model time share (0.31 VA, 0.55 VA2) with demos/sgp-contention; merged to main at [45f5a181](https://github.com/nakatamaho/vaeg/commit/45f5a18168e8f02fc966215e297c336fc9a2c443)** |
| M134 | tasks/M134_v1v2_hidden_text.md | V1/V2 hidden text with monochrome graphics: colours kept (PR #56), VA cell backgrounds kept as blank blocks (issue #50), PC-8801 models as a PC-8801 | **G134 human gate passed on 2026-10-25; M134 complete: V1/V2 hidden text keeps the multiplane graphics colours (PR #56, shitamo), VA/VA2 boots keep cell backgrounds as blank blocks in every graphics mode (issue #50), PC-8801/8001 models as a PC-8801; media builders find cached packages whatever their case; merged to `main` at [3bdb49bb](https://github.com/nakatamaho/vaeg/commit/3bdb49bb9eb481886aca7388a3453c5d8930a4f8)** |
| M135 | tasks/M135_compat_mode_timing.md | Compatible (uPD70008) mode timing calibrated to the PC-88VA and PC-88VA2 (issue #62): Z80TIME 2 per-instruction measurements, a cost model in the uPD70008 adapter | **G135 human gate passed on 2026-10-25; M135 complete: compatible-mode instructions charged their measured cost (Z80TIME 2 on the VA and VA2, rows within 0.95-1.03), memory waits word-wise, BRKEM/CALLN 47 and RETEM 37 clocks; merged to main at [5a69978b](https://github.com/nakatamaho/vaeg/commit/5a69978ba5bba56b319b3652489f973d298559e9)** |
| M138 | tasks/M138_release_20261025.md | Rel.20261025 notes and four approved screenshots; public recording ON with audited minimal static FFmpeg and source/relinking materials | **G138: Rel.20261025 published under the maintainer's release authorization after M137.** Immutable tag `rel-20261025` → `4fb3c44052fcccab7e47830ca1945d1498469bb2`; notes through M137 and four approved screenshots published, PR #57 still excluded. Public recording/static FFmpeg ON, bundled local encoders OFF; three packages, source/relinking materials, encoder/import/archive audits and clean static MinGW handoff verified. Successful selected CTest profiles: Linux 125 PASS/5 SKIP, macOS 141/11, Windows 139/5, zero final failures; earlier failures and scoped recovery are preserved in the task. Final publish run `40202401194` succeeds, reusing digest-bound macOS and original Linux outputs. No new physical GPU/display/HiDPI/private-input coverage or cleanup is inferred. |
| M136 | tasks/M136_controller_port_none.md | Controller port empty (なし) by default; ControllerPort key; cursor-key joystick/mouse only with that device on the port | **G136 human gate passed on 2026-10-25; M136 complete: controller port empty (なし) by default with the ControllerPort key, cursor-key joystick/mouse only with that device on the port; merged to `main` at [e377981e](https://github.com/nakatamaho/vaeg/commit/e377981e8b21364ee204425afcb933090048d789)** |
| M137 | tasks/M137_va_nmode_option.md | N mode on VA/VA2 boots only when enabled in 詳細設定 (off by default); greyed 起動 items with a tooltip | **G137 human gate passed on 2026-10-25; M137 complete: N mode for VA/VA2 boots behind 詳細設定 (off by default), greyed 起動 items with a tooltip; merged to `main` at [21b16211](https://github.com/nakatamaho/vaeg/commit/21b16211caf4b88352df3066b00e64e69e8bb253)** |
| M138a | tasks/M138a_msys2_native_build.md | Windows-native MSYS2 build: UTF-8 in build-static-librashader.py; MSYS2 rustup documented | **G138a human gate passed on 2026-10-26; M138a complete (renumbered from M138, which the Rel.20261025 release also used): UTF-8 in build-static-librashader.py and MSYS2 rustup documented; the reporter's MSYS2 build of the branch succeeded; merged to `main` at [65a6b5f7](https://github.com/nakatamaho/vaeg/commit/65a6b5f79fcbd73190ff4d72c6b67e04dd699165)** |
| M139 | tasks/M139_rv_all_machines.md | TSP SYNC.RV on every machine: M129's PC-8801 monochrome exception removed (a real SR reverses too) | **G139 human gate passed on 2026-10-26; M139 complete: SYNC.RV on every machine and graphics mode, M129's PC-8801 exception removed; merged to `main` at [0310a8f1](https://github.com/nakatamaho/vaeg/commit/0310a8f101b09c1f35888bd74d7286ff007c4a7f)** |
| M140 | tasks/M140_fdc_upd780_conformance.md | FDC-uPD780 probes, preserved physical observations, VA2 instruction/flag contract and upstream selector proposal | **G140 SCOPED_ACCEPTED**, confirmed VA2 uPD780C-1 instruction/flag contract only; [acceptance/deferred obligations](reports/m140_g140_va2_acceptance.md). Retained2896 VA2 AF/history rows and67 mapped ZEX CRCs validate;393216 software cases freeze the existing A-only behavior without a vendor or instruction change. Original-VA discrepancies, independent silicon controls, absolute timing/H/S and broader boundaries remain OPEN/DEFERRED. Upstream candidate is pushed but PR NOT_SUBMITTED. The inherited CPU-role ancestry is now separately accepted as described below; integration does not enlarge either scope. |
| M141 | tasks/M141_tsp_sprite_timing.md | TSP sprites and TVRAM as on the PC-88VA and PC-88VA2 (issue #67, demos/tsp-sprites) | **G141 human gate passed on 2026-10-29; M141 complete: sprites only after DSPON, the line's limit of about 300 image bytes, SPROV, IBF held after each byte (SPRON 3.4k VA / 3.7k VA2 clocks), the VA's VB of 23 rasters, TVRAM word waits; TSPSPR screen 5 within 0.92-1.09 of both machines except VA2 row Q (1.45, #75); state thumbnails turned, save times with seconds, debug script load-state; merged to `main` at [f6977770](https://github.com/nakatamaho/vaeg/commit/f6977770). Open: #74 (VRTC interrupt and VB out of phase), #75 (VA2 row Q), #76 (states not portable between Windows and Linux)** |
| M141 | tasks/M141_tsp_sprite_timing.md | TSP sprites and TVRAM as on the PC-88VA and PC-88VA2 (issue #67, TSPSPR): sprites only after DSPON, a line's image limit, SC/SPROV, IBF hold, the VA's VB length, TVRAM word waits | **G141 human gate pending** |
| M142 | tasks/M142_runtime_portability.md | FPS, clean memory defaults, requested PCG/N-mode defaults, numeric clocks, VA2 TVRAM investigation (#75), portable states (#76), explicit release identity; MacPorts MinGW handoff without Colima | **IN_PROGRESS; complete static MacPorts Windows build PASS; actual native state/expansion-size/restart checks PASS; local CTest aggregate 150 PASS/16 SKIP/0 FAIL with disclosed focused recovery/reuse; #75 investigation COMPLETE by explicit maintainer scope revision, cause identification/correction DEFERRED and issue OPEN (not a remaining M142 fix prerequisite); fresh nine-demo-media closure, 34 drawing cases, MANDEL progress and self-boot menu PASS ([media follow-up](reports/m142_demo_media_regression.md)); checked container-free DOS Open Watcom backend and RDBMS load-policy fix; fresh utility FDD/both SASI generation, payload readback and four default-off live boot contracts PASS ([utility follow-up](reports/m142_utility_media_regression.md)); extra redirected LHA query remains FAIL/unclassified; historical eight-worker save audit retained; authorized BMS/device-RAM, content-bound all-slot disk restore, checked callers and safe-save fixes implemented with intentionally incompatible new SAV format; corrected native source 0f3cbe20: fresh 155 PASS/16 SKIP/0 FAIL CTest and 18 identity-bound state/config/prior-file workers with exact expected rejections; fresh corrected-source static MacPorts MinGW handoff and 62-entry compiled table comparison PASS; actual Windows/genuine cross-OS execution remains NOT_RUN ([correction/evidence](reports/m142_save_portability_audit.md#authorized-restoration-correction-and-verification)); requested PCG/VA-N-mode startup-default follow-up implemented with explicit OFF preserved ([task](tasks/M142_runtime_portability.md#pcg-and-va-n-mode-startup-defaults)); GUI/interoperability checks pending ([gate preparation](reports/m142_gate_preparation.md)); G142 NOT_READY.** |
| M99999 | tasks/M99999_readme_attribution.md | Record the maintained-fork attribution and original project link in the top-level README | **G99999 passed** (documentation check) |

Historical parallel ID assignment: the [main-CPU role task](tasks/M139_upd780_or_upd9002_check.md)
also used M139/G139 on its separate branch. The maintainer explicitly accepts
its resolved MA identification scope as **G139 SCOPED_ACCEPTED**; see the
[acceptance supplement](reports/m139_cpu_role_acceptance.md). This is additional
to, not a replacement for, the unchanged whole-screen-reverse M139 row above.
The CPU candidate's later MA red-screen stop remains OPEN outside that scope.
The [local QA/handoff](reports/m139_demo_media_handoff.md) covers nine distribution
payload maps,34 bounded demo draws, BASIC computation/progress, four native boot
contracts and a clean static MinGW audit under their original identities and
limits. Both accepted histories are integrated with preserved M141 code by
[non-fast-forward merge a9b1d6c0](https://github.com/nakatamaho/vaeg/commit/a9b1d6c02fdbcec5781565a626a0bffa939bf01c).
The merged native profile passes154 CTests/8 SKIP/0 FAIL,34 new demo draw
contracts, BASIC computation/progress, four boot contracts and MA H=1/result16.
TSPSPR rebuilds byte-identically; unchanged earlier media-generation inputs
retain their digest-bound results. No stale branch replaces current main,
no new milestone is silently assigned, and no full-demo, physical-timing or
broader Windows coverage is inferred. See the acceptance supplement for the
new clean static MinGW handoff and retained evidence.

Planned series (ADR-0016; rows are added when each task file exists;
hardware model and milestone table in
[`docs/modernization/v1v2-mode-plan.md`](../modernization/v1v2-mode-plan.md)):
the V1/V2 series reached its ADR-0016 completion criteria with `M103d`
(N88-DISK BASIC V2 operates) and `M103e` (the PC-8801mkIISR DEMO runs),
followed by `M103f`, `M103g` and `M103h` (N-BASIC from a user-supplied
ROM, entered by the N mode and by `NEW ON 1`); `M103i` (V1 mode) followed; the series closed with `M103j` (cassette
tape, a vaeg extension) on 2026-10-05. N80SR and the N80 mode are not pursued. The V3 series starts at `M104` (display timing beyond the documented
modes, maintainer decision 2026-10-06); SGP follows in a later milestone.

Phase 2 dependencies: M7 → M8 → {M9, M10 parallel} → M11 → M12 → M13.
Post-phase dependency: M13 → M14 → M15 → M16 → M17 → M18 → M19 → M20 → M21 → M22 → M23 → M24 → M25 → M26 → M27 → M28 → M29 → M30 → M31 → M32. The required Z80 migration sequence M34 → M35 → M36 → M37 → M38 → M39 → M40 → M41 is complete. The separately authorized uPD9002 preparation sequence passed G42 through G51. M52–M56 were consumed by unrelated work and retain their historical meanings. The renumbered semantics campaign passed G57 at exactly `72322d5c9b8e40e4a988312aebe163a8190e2aa5`, G58 at exactly `bc8a55c6da1082b85b794068e0d933e31fe46b13`, G59 at exactly `e7f2325bc81310532091a8ca82914030fdb8b6ba`, G60a at exactly `ba2b7d3f5c76646b30d63fd8951f4a1964817b15`, G60b at exactly `4e5d74d0d9f675df2342353b8bfdbb2e5cded768`, G61 at exactly `829f314bb0d363ec5b6e9aa738e948b1a3adb365`, G62 at exactly `70b8e94e96aef4cb79eed72c7813c4148c5c0dd8`, G64 at exactly `9b151923f9468555043152ffe8651c97b9ecac5b`, terminal G65m at exactly `81887aae14f718d7d4d0f2a7bd3fe05d5ea80630`, G66b at exactly `97f760e8da573888edf089c2875c623895a3c2c9`, G67 at exactly `f8f350e1aadec4b6c79c20192d14c50bd39934be`, G68 at exactly `d1e0225c4edb716893fe5579283fbf0915db72b9`, G69 at exactly `680308a603b24341c5b9649657f01791b79002f7`, G70 at exactly `53d47ed500baef247a1be5f3ccc18bdb0c00c0cc`, G71 at exactly `24950894eca79e308afae8d574d43c8f393bb483`, and G72 at exactly `643d9f7289d817c67f343bf01be368b546bc1438`. M73 starts from the approved and main-integrated G72 candidate and owns only the post-M49 runtime performance regression. The broader IDP timing and buffer semantics remain deferred. See [`UPD9002_SEMANTICS_MIGRATION.md`](UPD9002_SEMANTICS_MIGRATION.md).
M9 must pass before M11 (all three OSes must ship the VA machine, not
the PC-98 scaffold).

The current approved gate ledger is:

- G73 passed at exactly
  `d7f1fd4b642ffa1bf71e855502e00341e9f37152`. This closes the M73
  post-M49 performance-regression investigation after the diagnostic pending
  check was made inline without changing the fail-closed CPU behavior.
- G74 passed at exactly
  `3785cc115155c52928817b8c95d38b40268a7bde`. This closes the deterministic
  debug-harness milestone.
- G75 passed at exactly
  `4ddba36f28dbfbe35a52117964b99b5685fdaa3d`. This is the M75 integration
  and release-documentation checkpoint for the PC-9801-55-compatible SCSI
  path, including the validated SCSI/SASI/HOSTFAT storage workflows.
- G76 passed at exactly
  `2ef9716d9628ce8eefdf61a1feedca0be5921077`. This is the M76 Stage 1
  uPD70008-compatible Z80 emulation-mode checkpoint, including the CP/MVA
  validation evidence. BRKEM2, full Z80 compatibility, and FDC/SCSI changes
  remain outside that approval.
- G77 passed at exactly
  `630e8f27fc4f2d574daf7cdc630836964a4247dc`. This is the M77 final
  `iova/` to `io/` tree move plus the required build, include, CMake, QA, and
  current-documentation reference updates. The commit is merged to `main`.
- G78 passed at exactly
  `23e9f4673e2e122835a5ad2fb256e6961f860866`. This closes the M78
  `iova/` to `io/` reference, CMake, and current-documentation normalization.
- G79 passed at exactly
  `1e19c4c539fd99dcc7dcd4a92770a51aef93aad1`. This closes the VA I/O
  dispatcher consolidation.
- G80 passed at exactly
  `c44569bd8c47c87c19c6e59bfb735ce7431102bd`. This closes the 98-only I/O
  cleanup while retaining VA-supported C-bus and storage paths.
- G81 passed at exactly
  `027cd761df98ce00fa1c24501d6233d7faaa0110`. This closes the VA BIOS
  reachability cleanup.
- G82 passed at exactly
  `788cd90aa07bf1619c47b2f130a2183d4fd7111c`. This closes the FDC
  subsystem uPD780-compatible CPU boundary audit.
- G83 passed at exactly
  `d90c8721d6120af9994cedb63685e8a60546513e`. This closes the move of the
  FDC subsystem uPD780-compatible CPU under `cpu/upd780/`.
- G84 passed at exactly
  `9aeb6512e59da7e794ffede50b7a184f601d137e`. This closes the approved
  M84a non-VA board retirement and M84b `cpucva/` boundary cleanup.
- G85 passed at exactly
  `0b6633041e2fb8bae8de7efa1a1768dc6c3e5cba`. This closes the state-save
  section cleanup and compatibility audit.
- G86 human gate passed on 2026-08-12 after the clean-checkout V3/demo/OS/
  simple-operation validation. M86 is closed at the implementation merge
  [74a5eac8](https://github.com/nakatamaho/vaeg/commit/74a5eac8bc0fa145fc0c4bf5ed66e3ff5368c0ae);
  rename-only, reference-fixup, and machine-validation details are recorded in
  the M86 report.
- G87 human gate passed on 2026-08-12 for the M87 candidate
  [d2d1a13](https://github.com/nakatamaho/vaeg/commit/d2d1a13167ccd094d0fae180c775ad5e1d7eb78e).
  This closes the legacy-tool and ROM/resource-regeneration audit; the
  deletion, deferred-boundary, and machine-validation details are recorded in
  the M87 report. The resulting M87 implementation and hotfix chain was
  merged to `main` at
  [f876dbb](https://github.com/nakatamaho/vaeg/commit/f876dbbfe4e69f0a2ad2021b289962d15754812d).
- M88 source cleanup is recorded in
  [2fe49c9](https://github.com/nakatamaho/vaeg/commit/2fe49c944797ca8508c3cfc53ed39ffdef5014b0).
  It removes the retired VM/VX, GDC/CRTC, generic non-VA renderer, and
  FM7/X1/X68K font surfaces while retaining VA/VA2 display, MPU98II,
  SASI/SCSI, FDD, HOSTFAT, and the shared CPU-memory compatibility layer.
  The detailed disposition and machine validation are in
  [`m88_final_va_only_source_tree_audit.md`](reports/m88_final_va_only_source_tree_audit.md);
  hosted run [31573711804](https://github.com/nakatamaho/vaeg/actions/runs/31573711804)
  passed all jobs except the Windows MinGW compatibility Configure step;
  G88 human validation passed against candidate `98d7343df9c763354e0775bd04a7b6d8d9c6a291`; M88 was merged to `main` at
  [b142bc3](https://github.com/nakatamaho/vaeg/commit/b142bc37c4fe0cc50381727eac5766a5b3843e71).

-
  M89 source-directory consolidation completed at candidate
  [665877a](https://github.com/nakatamaho/vaeg/commit/665877ab7e0961907a255796b30e7438115c6e51).
  Hosted run [31577266904](https://github.com/nakatamaho/vaeg/actions/runs/31577266904)
  passed all nine jobs against that exact candidate. The maintainer passed
  G89 human validation on 2026-08-12. The approved history was fast-forwarded
  to `main` at [5b4a22b](https://github.com/nakatamaho/vaeg/commit/5b4a22ba4e8a4fc7ef44f3d2dfcfe4c1001cde97);
  M89 is closed.

- G90 human gate passed on 2026-08-13 against the final M90 history at
  [f16d6af](https://github.com/nakatamaho/vaeg/commit/f16d6af14039359e5b617d757f906a47f45b1ad9).
  This closes the VA EMS board, bootable EMMVA/SQEMM98/RDEMS workflow,
  guest-driver distribution, and EMS-enabled development-disk work.

- G91 human gate passed on 2026-08-15 against candidate
  [66fbf20](https://github.com/nakatamaho/vaeg/commit/66fbf20f6f5a8fbe107884ca1223acc53d352cbd).
  This closes the native V3 VA single-path consolidation after the required
  clean-checkout boot, demo, OS, and simple-operation validation.

- G97 human gate passed on 2026-08-24 against the extended M97 candidate
  `7ab4fabfff8ace9f7ae8648cee4004ad6d507b94`. This closes the documented
  SGP descriptor/LINE/SCAN work, the staged visual-test family, and the
  GLASS P4 regression evidence. G97 remains a VAEG visual-regression gate;
  it does not establish real-PC-88VA equivalence or timing behavior.

M73 is closed after the post-M49 performance-regression investigation and
its approved runtime correction. M74 is a separate diagnostic-infrastructure
milestone for a deterministic debug harness and is closed at
`3785cc115155c52928817b8c95d38b40268a7bde`. M75 through M77 remain completed
with their approved gate SHAs above. M78 through M85 are now also completed
with the approved gate SHAs recorded above. M86 is now closed after its
implementation merge and G86 human gate. M87 is now closed after G87 human
validation and its merge to `main`; M88 is closed after G88 human validation
and is merged to `main` at `b142bc37c4fe0cc50381727eac5766a5b3843e71`.
M89 is closed after G89 human validation. The approved source-directory
consolidation history was fast-forwarded to `main` at
[5b4a22b](https://github.com/nakatamaho/vaeg/commit/5b4a22ba4e8a4fc7ef44f3d2dfcfe4c1001cde97).
M90 is closed after G90 human validation against the final M90 commit
[f16d6af](https://github.com/nakatamaho/vaeg/commit/f16d6af14039359e5b617d757f906a47f45b1ad9).
M91 is closed after G91 human validation against the native V3 single-path
candidate [66fbf20](https://github.com/nakatamaho/vaeg/commit/66fbf20f6f5a8fbe107884ca1223acc53d352cbd).

M90 starts from the G89-integrated `main` tree. It restores the retained EMS
page-frame implementation as a VA-configurable expansion board, using the
existing `ExMemory` and save-state identities with disabled or 1-13MB capacity
in 1MB units. It installs the redistributable EMMVA adapter, SQEMM98 manager,
and dependent RDEMS utility in the generated supplemental-disk workflow. It
also builds a hash-pinned HOSTFAT/SQEMM98 guest-driver bundle in CI and includes
the same drivers, licenses, instructions, and checksums in normal artifacts and
tagged release packages. M90 does not change I/O Bank Memory semantics, bundle
commercial software, or track generated media or driver binaries.

M90 implementation is complete at
[498b283](https://github.com/nakatamaho/vaeg/commit/498b283b67bc0e68dcd6f507260e190974c07f9f),
with local validation recorded in
[`m90_va_ems_board.md`](reports/m90_va_ems_board.md) and hosted CI
[31675702844](https://github.com/nakatamaho/vaeg/actions/runs/31675702844)
passing all nine jobs for the preceding implementation candidate. The final
13MB/832-page behavior follow-up hosted run
[31678186077](https://github.com/nakatamaho/vaeg/actions/runs/31678186077)
also passed all nine jobs. The guest-driver distribution candidate then passed
all ten jobs in hosted run
[31679653128](https://github.com/nakatamaho/vaeg/actions/runs/31679653128),
including its dedicated clean-build distribution job. The maintainer passed
G90 on 2026-08-13 against the final M90 history at
[f16d6af](https://github.com/nakatamaho/vaeg/commit/f16d6af14039359e5b617d757f906a47f45b1ad9),
and M90 is closed.

M72 closed the inactive compile-flag cleanup while intentionally leaving
`SUPPORT_WAVEREC`, `SUPPORT_OPRECORD`, and FDD320 for later focused audits.
M87-M89 define the final VA-only source-tree consolidation sequence: after
the completed dispatcher, 98-only I/O, BIOS, uPD780, `cpu/`, state-save,
and machine-core work, M87 closed the legacy tool and ROM/resource
regeneration audit while retaining deferred provenance boundaries. M88
closed the final VA-only source-tree audit and M89 consolidates the active
VA BIOS, VRAM, and CPU adapter/backend directories into `bios/`, `vram/`,
and `cpu/`. `cbus/` is not treated as 98-only; VA-supported expansion
boards remain in scope for retention.

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
references them. The FDD1/FDD2 Open browser also accepts those archive formats
through the same extraction path. The implementation and G22 checklist are in
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

M27 is complete and G27 passed. It restores the original Frame Disp semantics
in the SDL2 frontend. It measures the core guest-draw counter over an
approximately two-second window and appends `N.NFPS` to the native window
title. Frame display defaults to enabled when no saved `DspClock` setting
exists, while an explicitly saved off setting is preserved. It does not report
ImGui present rate or change frame skip, VBlank, CPU/SGP speed, or host
pacing. The implementation scope and G27 checklist are in
`tasks/M27_frame_display.md`.

M28 adds common audio-output controls to the Sound menu. Sampling rate and
requested sound-buffer length apply to both NP2 and ymfm through the existing
sound rebuild path. ymfm also exposes Minimum, Medium, and Maximum native OPN
fidelity, with Minimum retained as the compatibility default. The scope,
backend boundary, automated checks, and G28 checklist are in
`tasks/M28_sound_output_settings.md`.

M29 corrects the VA1 system-memory bank-1 aperture. TVRAM remains backed by the
legacy `textmem` object, but VA1 CPU access is limited to the documented 64KB
`A0000H-AFFFFH` range; the unused `B0000H-DFFFFH` range reads as open bus and
ignores writes. This prevents PC-Engine 1.00 from misdetecting banked system
memory as main RAM and placing its VA1 stack where a ROM bank switch hides it.
M31 testing found that applying the same clamp to VA2/VA3 regressed V3 BASIC,
so that model retains its 256KB bank-1 behavior. NEC's VA, VA2, and VA3 product
specifications confirm the model split: 64KB of TVRAM in VA1 and 256KB in
VA2/VA3. The root-cause trace, rejected workarounds, automated boundary tests,
regression record, and human results are in
`tasks/M29_va1_tvram_aperture.md`.

M30 restores the frozen implementation's BMS behavior in the portable VA
memory layer. The `80000H-9FFFFH` aperture now reads as open bus and ignores
writes when BMS is disabled; when enabled it accesses the selected 128KB bank.
The M9 C port had temporarily routed the aperture to ordinary main RAM because
the assembly-only BMS handlers were not callable from `i286c`. That made a
nonexistent 128KB region pass software memory probes. The code evidence,
ROM-less bank tests, and G30 acceptance are in
`tasks/M30_va_bms_window.md`. Human testing showed that this correction does
not resolve the separate VA1 N88 BASIC V3.0 `FILES`/`BEEP` failure; that
investigation is deferred and is not part of M30.

M31 adds `--model va` and `--model va2` to the active SDL2 command line. The
override is applied after loading `vaeg.cfg`, then uses the same canonical
model, ROM-set, and sound-hardware transition policy as the GUI. It is restored
before configuration save, so command-line boot selection does not rewrite
the user's persistent `pc_model` or `SNDboard`. G31 also corrected the M29
VA2/VA3 TVRAM regression while preserving the VA1 PC-Engine 1.00 fix. The
implementation and gate are in `tasks/M31_cli_boot_model.md`.

M32 extends the session-only command line across the existing sound, FDD/SASI,
CPU/SGP pacing, display, controller, and keyboard settings. It validates the
complete effective machine configuration before video, audio, and machine
initialization, including the VA2/VA3 OPNA requirement and actual SASI image
classification. Named
`--fdd1`/`--fdd2` options replace the retired positional FDD syntax. CLI-owned
values are restored before configuration save while GUI changes made during
the run remain persistable. The implementation and G32 checklist are in
`tasks/M32_cli_startup_overrides.md`. G32 passed after maintainer verification
of the deployed MinGW build.

M52 restores the active SDL2 configuration path for the optional PC-88VA I/O
Bank Memory device and corrects the M30 bank-zero interpretation using RDBMS
1.21 source and guest evidence. Port value zero restores ordinary main RAM at
`80000H-9FFFFH`; values 1 through N select the N allocated 128KB banks. Native
PC-88VA port `01D0H` is the default; `00ECH` remains the PC-9801 mode. RDBMS's
compiled `00ECH` default must be overridden with `-P1D0` in PC-88VA mode.
The implementation scope and G52 checklist are in
`tasks/M52_io_bank_memory.md`.

A post-G90 follow-up preserves M52's bank-zero/pass-through policy while
retaining the clean capacity defaults. The current model exposes 640KB of
conventional RAM through `9FFFFH`: selector zero restores ordinary main RAM at
`80000H-9FFFFH`, selectors 1 through N map the N independent 128KB BMS banks,
and invalid nonzero selectors remain open bus. That follow-up enabled
128 banks (16MB) at `01D0H` and 13MB of EMS for clean configurations.
M142 changes fresh defaults to both boards absent to avoid unnecessary state
payloads; explicit persisted capacities/enables remain user choices. The correction is
[c52bd8d](https://github.com/nakatamaho/vaeg/commit/c52bd8dbc62dfabc5d7bbbc50b4fbfe7bd6deef4).

M53 adds an optional `PacingMs` host delay in Configure. The implementation
does not alter emulated clock accounting. It schedules guest frames at the
chosen interval while continuing to process and render the host UI, so large
values such as 64ms leave menus and input responsive. The implementation and
G53 checklist are in `tasks/M53_host_pacing.md`.

M54 adds the first read-only HOSTFAT path for PC-Engine. A session-only command
line option converts a deliberately constrained host directory into one fixed
FAT12 snapshot before machine startup. A small clean-room CONFIG.SYS
block driver reads that snapshot through the versioned emulator-private
07EDH/07EFH channel. It is a virtual block disk, not an INT 2FH redirector;
host changes are not visible until a new emulator session, and all guest write
commands return write-protect. The exact prototype boundary and G54 checklist
are in `tasks/M54_hostfat_readonly_prototype.md`. G54 passed for the original
driver at `19626dc`, after which its source provenance was found insufficient
for two-clause BSD redistribution. The independently authored replacement,
factual contract, and attestation require supplemental human revalidation
before M55; no history rewrite is part of that correction. The maintainer
accepted the clean-room replacement and its supplemental gate at
`e0bafbaa3cc0b12f945e18c231c843fc17ff0392`.

M55 adds persistent GUI selection, snapshot refresh and identity policy,
save-state handling, broader deterministic 8.3 mapping, and the final
host-path containment checks. It also expands the fixed image to the practical
PC-Engine CONFIG.SYS-driver FAT12 limit: 1024-byte logical sectors, 16 KiB
clusters, 4084 DOS-visible data clusters, and at most 63.71875 MiB of
allocatable cluster payload. The read-only block-device contract established
by M54 remains unchanged. The G55 run rejected the original 2048-byte/
32 KiB proposal: a 96 KiB file was truncated to 6144 bytes. The corrected
16 KiB geometry copied that file byte-identically and also read a marker
allocated beyond 60 MiB. Historical SASI HDD and SCSI MO support use their
dedicated storage paths and do not prove larger clusters for this independent
CONFIG.SYS driver path.

M56 first tested the prerequisite for an independently authored read-only DOS
network redirector. The non-resident probe reports PC-Engine's DOS interface
as 2.00; `INT 21H/AX=5F02H` and `5F03H` both return `AX=0001, CF=1` without a
single call to the temporary `INT 2FH/AH=11H` hook. The conventional redirector
design therefore cannot expose transparent `DIR`, `TYPE`, or program loading
in the accepted environment. M56 stopped at this fail-closed evidence gate.
HOSTFAT remains unchanged and visible. Any PC-Engine file-service patch or
non-transparent utility protocol requires a separately approved design. See
`tasks/M56_hostfs_readonly_redirector.md`,
`research/m56_pcengine_redirector_probe.md`, and
`reports/m56_hostfs_readonly_redirector.md`. On 2026-07-23 the maintainer
administratively closed G56 at approved SHA
`b72e641733ddea6f0e8faef2507093f7c3aee5a4` because the required DOS
redirector bridge is absent. This closure accepts the prerequisite-stop
disposition only; it is not evidence of a successful HOSTFS implementation.

M57 preserves the former Win9x/i286x tier's legal and lineage evidence, then
removes exactly `win9x/`, `i286x/`, `hlp/`, and
`cpuxva/memoryva.x86` from the current tree. The byte-identical legacy notice
is retained at `LICENSES/legacy-vaeg.txt`, with its relationship to the
archived source recorded in `docs/legal/legacy-source-provenance.md`. The
complete pre-deletion tree remains at annotated tag
`archive/frozen-win9x-i286x-g56`. The maintainer passed G57 at exactly
`72322d5c9b8e40e4a988312aebe163a8190e2aa5`.

M58 adds immutable references to the M43 summaries and failure sidecars,
separate blocking architectural and diagnostic all-FLAGS fingerprint
profiles, a hash-level predecessor ratchet, gap taxonomy, and strict
lettered-milestone parsing. It changes no uPD9002 instruction semantics and
stops at the G58 human gate.

## Gate protocol

Agent side (pasted into PR): CMake build logs, `tools/repo/` check
output, and for M8+ a headless smoke run
(`SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/sdl2/vaeg
--smoke` or documented equivalent). The archived reference tier has no
current-tree build target.

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
- **Archive and removal of the reference tier (M57).** The exact G56 tree is
  retained at annotated tag `archive/frozen-win9x-i286x-g56`; legal evidence
  is preserved in the current tree before the four frozen paths are removed.
- **SDL2 keyboard policy (M14).** ADR-0008 keeps guest input scancode
  based, distinguishes JIS physical from US keytop behavior, routes all
  synthetic input through normal guest make/break handling, stores custom
  bindings by scancode name, and forbids text or guest-memory injection.
- **PC-88VA active-tree invariant (M15, updated by M72).** The active
  CMake/SDL2 build always includes VA support, so `SUPPORT_PC88VA` is folded
  true. M72 folds the always-enabled `VAEG_FIX` behavior, removes inactive
  PC-9821 and EPSON guarded/model code plus stale 98x1 About/More UI details,
  audits inactive IA32 CPU-core, IDE I/O, PC-9861K expansion serial,
  no-sound, and other VAEG-irrelevant legacy code, keeps required SCSI and
  HOSTFAT paths, removes the separate legacy HOSTDRV path, and treats
  `VAEG_EXT` as obsolete active-tree cleanup rather than a feature to enable
  blindly when audit evidence proves the target is inactive.
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
