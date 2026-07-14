<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# VAEG Fork Bug-Fix Ledger

This is the permanent index of demonstrated correctness fixes in the
maintained VAEG fork. It complements release-oriented `CHANGES*.md` files and
milestone-oriented `docs/agents/tasks/M*.md` records. It is not a feature
list.

The initial historical entries below cover the major active-tree fixes whose
cause and correction can be recovered from the milestone records and commit
history. Smaller build-only and presentation changes remain in their task
documents and git history. New correctness fixes must be added here when they
land.

## Maintenance Rules

For every new entry, record:

- observed symptom and affected model/platform;
- demonstrated root cause, clearly separated from rejected hypotheses;
- correction and compatibility boundary;
- automated and human verification actually performed;
- milestone/task document and fixing commit;
- status: `fixed`, `accepted parity correction`, `open`, or `reverted`.

Do not mark a defect fixed solely because a plausible code difference was
found. If the human reproduction still fails, retain the finding as a
separate parity correction or move it to Open Defects.

## Fixed Defects

### Portable V30/uPD9002 execution did not match the VA CPU path

- **Status:** fixed in M9.
- **Symptom:** the initial portable C core could not reliably execute the VA
  firmware and software paths handled by the frozen V30 core.
- **Root cause:** the portable opcode tables and reset handoff were missing or
  incorrectly mapping V30 behavior, including REPC carry semantics, REPC
  dispatch, POP SP, V30 IRET/trap handling, and preservation of V30 CPU type
  during reset.
- **Correction:** ported the required V30 handlers and table wiring, matched
  uPD9002 POP SP behavior, and restored the legacy V30 reset/mode handoff.
- **Evidence:** [V30/uPD9002 map](../agents/reports/m9_v30_map.md) and
  [M9 boot comparison](../agents/reports/m9_boot_debug.md).
- **Commits:** [b9b0da1](https://github.com/nakatamaho/vaeg/commit/b9b0da147d501e67e14eaba0a90d91f7a05eaf3b),
  [27b1712](https://github.com/nakatamaho/vaeg/commit/27b1712d15c9b1cd1af84bc92e625fc26bfea92e),
  [7765038](https://github.com/nakatamaho/vaeg/commit/77650387f3e62f602ca9fe4410fc22777202f48c),
  [dcb6939](https://github.com/nakatamaho/vaeg/commit/dcb6939492e3fd896a17ad2052484b7bb3b77ccd), and
  [ee1e9e9](https://github.com/nakatamaho/vaeg/commit/ee1e9e91e8fab4a2c4177a09a13def44c3603d03).

### Direct-mode FDD DMA stopped before a sector was transferred

- **Status:** fixed in M9.
- **Symptom:** after switching the VA FDD interface to direct mode, a
  1024-byte Read Data operation transferred only 35 bytes in the portable
  build, preventing normal disk boot.
- **Root cause:** the portable V30 loop used the plain V30 DMA pump instead of
  the VA-aware i286 cadence used by the frozen implementation.
- **Correction:** routed the active V30 execution and step loops through the
  VA-aware DMA pump and honored extended DRQ state.
- **Verification:** differential FDC/DMAC traces showed the same channel,
  range, and bank with the transfer reaching 1024 bytes.
- **Evidence:** [M9 boot comparison](../agents/reports/m9_boot_debug.md).
- **Commit:** [cc7a154](https://github.com/nakatamaho/vaeg/commit/cc7a154d2c4839a1a59d67cbfccb865a18f8f695).

### SDL2 pacing starved host-time work and diverged from legacy frame skip

- **Status:** fixed in M9.
- **Symptom:** FDD motor/seek sound events could remain dead or stuck, and
  automatic frame skipping did not follow the established VAEG cadence.
- **Root cause:** a simplified pending-frame loop stopped pumping the host
  tick while waiting and did not preserve the legacy NOWAIT/fixed/auto-skip
  state machine.
- **Correction:** serviced task and host ticks on every outer loop iteration
  and restored one-frame-at-a-time legacy auto-skip pacing with a bounded
  catch-up limit.
- **Verification:** `--pacelog`, FDD host-event testing, and the G9 timing
  comparison.
- **Evidence:** [M9 boot and pacing analysis](../agents/reports/m9_boot_debug.md).
- **Commits:** [0909547](https://github.com/nakatamaho/vaeg/commit/090954705ac235c1b47692bf9376e87c21b257d0) and
  [733b4fa](https://github.com/nakatamaho/vaeg/commit/733b4faf1804953967b23bce84c533db1e7c9926).

### Runtime handles and pointers were truncated on 64-bit Windows

- **Status:** fixed in M11.
- **Symptom:** MinGW LLP64 builds could truncate `FILE *`, handle, callback,
  and runtime pointer values stored through 32-bit `long`, risking crashes or
  invalid file/state access.
- **Root cause:** ILP32 assumptions inherited from the Win32 tree were used in
  active 64-bit runtime structures and conversions.
- **Correction:** stored runtime file handles as `FILEH`, introduced
  pointer-sized runtime conversions, and removed unsafe pointer-to-long
  comparisons while preserving serialized formats.
- **Verification:** the LLP64 audit classified each conversion and the MinGW
  build completed after the corrections.
- **Evidence:** [M11 LLP64 audit](../agents/reports/m11_llp64_audit.md).
- **Commits:** [3f0f4ce](https://github.com/nakatamaho/vaeg/commit/3f0f4ce8356503b5fd43713e5893aace84354afd),
  [08723d5](https://github.com/nakatamaho/vaeg/commit/08723d561930d88267d5935e6192d891e010467a), and
  [7faae73](https://github.com/nakatamaho/vaeg/commit/7faae736d711faa04e36100c5644dd70be42c1fc).

### VA backup memory was not consistently loaded and saved by SDL2

- **Status:** fixed in M11, with portable lookup aligned again in M19.
- **Symptom:** VA backup state could be missed or not persisted across active
  SDL2 sessions, depending on frontend and state-path selection.
- **Root cause:** the portable main lifecycle did not explicitly load/save VA
  backup memory, and path selection differed from the intended portable
  executable-local/user-state policy.
- **Correction:** connected backup-memory load/save to SDL2 startup/shutdown
  and unified its path priority with active configuration lookup.
- **Verification:** state-path tests and human persistence checks.
- **Evidence:** [M11 portability task](../agents/tasks/M11_mingw_macos.md) and
  [M19 portable runtime task](../agents/tasks/M19_portable_runtime.md).
- **Commits:** [06aaa90](https://github.com/nakatamaho/vaeg/commit/06aaa90a95952932d0f9aaebd2624d28f0863bfd) and
  [4d4f8a0](https://github.com/nakatamaho/vaeg/commit/4d4f8a01d4033f09898305d0d0353aedcb65bb10).

### JIS Kana and punctuation scancodes produced incorrect guest keys

- **Status:** fixed in M14.
- **Symptom:** JIS Yen/pipe and related punctuation could be missing or mapped
  to US physical positions, and the Right Alt Kana-lock path did not behave as
  the selected JIS/Roman mode required.
- **Root cause:** several SDL scancodes needed explicit JIS physical actions;
  the US keytop translation and JIS physical preset could not share all
  punctuation assumptions.
- **Correction:** separated JIS physical mappings from US keytop actions and
  corrected Kana and punctuation bindings without injecting text directly.
- **Verification:** ROM-less mapping tests and the M14 human JIS/US keyboard
  gate.
- **Evidence:** [M14 keyboard mapping task](../agents/tasks/M14_keyboard_mapping.md)
  and [keyboard mapping reference](keyboard-mapping.md).
- **Commits:** [57be6f1](https://github.com/nakatamaho/vaeg/commit/57be6f1a658620a182f0efecacf2cf51aa7c0576) and
  [8bb09b4](https://github.com/nakatamaho/vaeg/commit/8bb09b40854a8e24ac59465d4c7a134f07c134d1).

### VA2/VA3 could silently use the wrong model ROM set

- **Status:** fixed in M18.
- **Symptom:** the flat historical ROM layout allowed VA and VA2/VA3 ROMs with
  overlapping names to be confused, producing model-dependent startup
  failures that were difficult to diagnose.
- **Root cause:** one unsuffixed lookup namespace was used for distinct model
  ROM contents.
- **Correction:** VA keeps unsuffixed files while VA2/VA3 requires MAME-style
  `*_va2.rom` names with no VA fallback; size, CRC32, and SHA-1 diagnostics
  identify mismatches.
- **Verification:** G18 model boot checks and checksum diagnostics.
- **Evidence:** [M18 ROM layout task](../agents/tasks/M18_rom_layout.md).
- **Commit:** [f59c106](https://github.com/nakatamaho/vaeg/commit/f59c106e4789217326cb53153908de49873a9e7b)
  and its linked M18 topic history.

### SDL2 dropped the rightmost VA guest pixel

- **Status:** fixed in M21.
- **Symptom:** the right edge of the 640-pixel VA display was one pixel short.
- **Root cause:** the legacy converter produces a 641-pixel row containing a
  left guard followed by 640 guest pixels, but SDL2 uploaded the first 640
  pixels and therefore displayed the guard while dropping guest pixel 639.
- **Correction:** retained the converter contract but uploaded from the first
  guest pixel and made uniform-frame checks use the same visible span.
- **Verification:** ROM-less checks cover the guard, guest pixels 0 and 639,
  and the 641-pixel backing row; the maintainer confirmed the edge display.
- **Evidence:** [M21 SDL2 display task](../agents/tasks/M21_sdl2_display_effects.md).
- **Commit:** [caaf97c](https://github.com/nakatamaho/vaeg/commit/caaf97c56dd39e861c12537a38b6b31b43bd8722).

### V30 LOOP timing made firmware BEEP delays too short

- **Status:** fixed in M21.
- **Symptom:** BASIC `BEEP` duration was shorter than original VAEG/hardware
  behavior in VA and VA2/VA3 modes.
- **Root cause:** opcode `E2` inherited i286 LOOP timing instead of the V30
  timing used by ROM delay loops.
- **Correction:** added a V30-specific LOOP handler using 17 clocks when taken
  and 5 clocks on termination, leaving i286, PIT, audio, and frozen code
  unchanged.
- **Verification:** ROM-less timing tests and maintainer VA/VA2 BASIC BEEP
  comparison.
- **Evidence:** [M21 SDL2 display task](../agents/tasks/M21_sdl2_display_effects.md).
- **Commit:** [a06ab6e](https://github.com/nakatamaho/vaeg/commit/a06ab6ec24f03116152492fa3a0e3c69d87830ad).

### Restored archive mounts disappeared from the FDD menu

- **Status:** fixed in M22.
- **Symptom:** an archive-extracted FDD image remained mounted after restart
  but its filename was no longer shown in the FDD menu.
- **Root cause:** the menu used transient archive/frontend state instead of
  the live FDD mount path restored from configuration.
- **Correction:** render FDD1/FDD2 labels from live disk state, including the
  delayed insertion path, and preserve full-path hover text.
- **Verification:** M22 persistence gate across restart.
- **Evidence:** [M22 disk-image drop task](../agents/tasks/M22_disk_image_drop.md).
- **Commit:** [afeee5b](https://github.com/nakatamaho/vaeg/commit/afeee5b20e23a27b4f5a1e75024fe1dbb0afe5f).

### Archive loading was unavailable or failed on POSIX Japanese paths

- **Status:** fixed in M22.
- **Symptom:** Linux release builds could report archive support unavailable;
  archives containing UTF-16LE/Japanese entry names could fail with a locale
  conversion error.
- **Root cause:** release dependency availability varied by build host, while
  LibArchive pathname conversion ran under the startup C locale.
- **Correction:** pinned the release archive dependency stack and perform
  POSIX entry-name decoding under a thread-local UTF-8 locale without changing
  global process locale.
- **Verification:** Linux release selftest/smoke and a Japanese 7z basename
  regression case.
- **Evidence:** [M22 disk-image drop task](../agents/tasks/M22_disk_image_drop.md).
- **Commits:** [bd905a2](https://github.com/nakatamaho/vaeg/commit/bd905a2b78fd950b9a8cc76c44427f25150f6445) and
  [c05411e](https://github.com/nakatamaho/vaeg/commit/c05411e5760d57d9530949a18fc4b19550ba0c2c).

### FDD Open exposed managed extraction storage after an archive drop

- **Status:** corrected again in M32 after the first macOS arm64 G32 check
  failed; G32 recheck pending.
- **Symptom:** after dragging and dropping a ZIP, 7z, or LZH archive, opening
  FDD1/FDD2 Open started the browser in the managed user-state extraction
  directory instead of the directory containing the source archive.
- **Root cause:** the FDD browser always derived its initial directory from the
  live mounted image path. Archive mounts necessarily replace that path with
  an extracted image path, while the archive loader retained no association
  with the source archive directory.
- **Correction:** retain the source directory per mounted drive and extracted
  image, use it only when the live mount still matches, and store the
  association beside each managed image so it survives application restart
  and is removed by the existing prune lifecycle. The follow-up also detects
  managed extraction paths without metadata, falls back to the persisted FDD
  browser directory, updates that fallback on each archive drop, and does not
  display the internal extracted path in the FDD Open filename field.
- **Verification:** ROM-less dropmedia tests cover ZIP and 7z source capture,
  metadata reload, drive/path matching, unrelated-path rejection, managed-path
  classification, and the no-metadata browser fallback. The first macOS arm64
  GUI check still showed the extraction directory; after the follow-up, the
  macOS MacPorts and MinGW cross targets build successfully and the full
  ROM-less selftest passes. GUI recheck remains in G32.
- **Evidence:** [M32 command-line startup task and G32 follow-up](../agents/tasks/M32_cli_startup_overrides.md#g32-archive-browser-follow-up).
- **Commits:** initial correction
  [ce26003782cec9b93639cc34b2e33c5de3e63d8a](https://github.com/nakatamaho/vaeg/commit/ce26003782cec9b93639cc34b2e33c5de3e63d8a)
  and macOS arm64 follow-up
  [214cab3bc14b26ea2aa044aa505c0e1151787bb7](https://github.com/nakatamaho/vaeg/commit/214cab3bc14b26ea2aa044aa505c0e1151787bb7).

### VA1 PC-Engine 1.00 selected a stack in banked TVRAM

- **Status:** fixed in M29; focused human boot gate passed.
- **Symptom:** a clean PC-Engine 1.00 system disk booted in VA2/VA3 but failed
  to complete startup in VA mode.
- **Root cause:** system-memory bank 1 exposed the full legacy 256KB
  `A0000H-DFFFFH` text backing to the CPU although VA TVRAM is only 64KB at
  `A0000H-AFFFFH`. The memory probe therefore selected `D000:FFFx` for its
  stack. A VA ROM switch from bank 1 to backup-memory bank 9 hid a pushed map
  value; the subsequent pop restored `FFFFH`, selected bank F, and corrupted
  stack/interrupt state.
- **Correction:** limited VA1 CPU-visible bank-1 TVRAM to 64KB, returned
  open-bus ones, and ignored writes in `B0000H-DFFFFH`, including
  boundary-crossing word access.
- **Verification:** ROM-less aperture tests and successful maintainer VA1
  PC-Engine 1.00 boot.
- **Evidence:** [M29 VA1 TVRAM aperture task](../agents/tasks/M29_va1_tvram_aperture.md).
- **Commit:** [c17d64a](https://github.com/nakatamaho/vaeg/commit/c17d64a71f6f32cf9ce6cd070da7ae3e68899af6).

### The M29 TVRAM clamp regressed VA2 V3 BASIC

- **Status:** fixed during M31 verification.
- **Symptom:** M28 could enter and use VA2 V3 BASIC, while its direct child
  M29 froze after entering BASIC. VA1 PC-Engine 1.00 still required the M29
  aperture correction.
- **Root cause:** the M29 `A0000H-AFFFFH` limit was implemented in shared
  `tvram_*()` handlers without a model check, so it changed the VA2/VA3
  memory path as well as VA1. NEC specifications identify 64KB of TVRAM in
  VA1 and 256KB in VA2/VA3.
- **Correction:** apply the 64KB/open-bus behavior only to `PCMODEL_VA1` and
  preserve the 256KB bank-1 backing behavior for `PCMODEL_VA2`. This matches
  the documented physical TVRAM capacities; the VA2 V3 BASIC regression test
  also exercises the active `A0000H-DFFFFH` mapping.
- **Verification:** ROM-less tests cover both model-specific mappings. Human
  testing confirmed VA2 V3 BASIC and VA1 PC-Engine 1.00; the separate inherited
  VA1 V3 BASIC command failure remains open.
- **Evidence:** [M29 VA1 TVRAM aperture task](../agents/tasks/M29_va1_tvram_aperture.md),
  [M31 CLI boot-model task](../agents/tasks/M31_cli_boot_model.md),
  [NEC PC-88VA specification](https://support.nec-lavie.jp/support/product/data/spec/cpu/b047-1.html),
  [NEC PC-88VA2 specification](https://support.nec-lavie.jp/support/product/data/spec/cpu/b048-1.html),
  [NEC PC-88VA3 specification](https://support.nec-lavie.jp/support/product/data/spec/cpu/b049-1.html),
  and the [PC-88VA hardware comparison](http://www.pc88.gr.jp/~va/va-hard.html#mem).
- **Commit:** [c580222](https://github.com/nakatamaho/vaeg/commit/c5802228f1d8f7cf91b41d1182aaad4ebd30ccea).

### The disabled VA BMS window incorrectly exposed ordinary RAM

- **Status:** accepted parity correction in M30.
- **Symptom:** with BMS disabled, `80000H-9FFFFH` retained writes as ordinary
  memory instead of behaving as an absent bank-memory aperture.
- **Root cause:** the M9 C port could not call the assembly-internal BMS
  handlers and temporarily routed the window through normal i286 memory.
- **Correction:** implemented local byte/word BMS handlers: absent storage is
  open bus with ignored writes, while enabled access uses the selected 128KB
  bank with allocation bounds checks.
- **Verification:** ROM-less absent-window, bank-selection, and bank-isolation
  tests. This did not fix the separate VA1 BASIC failure and is not claimed to
  do so.
- **Evidence:** [M30 VA BMS window task](../agents/tasks/M30_va_bms_window.md).
- **Commit:** [11da283](https://github.com/nakatamaho/vaeg/commit/11da283a0ffa47fc4b645423e4324550d1438bcf).

## Open Defects

### VA1 N88 BASIC V3.0 commands can enter an apparent hang

- **Status:** open.
- **Symptom:** in the inherited VA1 execution path, commands including
  `FILES`, `LIST`, and `BEEP` can leave the guest executing a repeated path;
  sound-enabled `BEEP` may also expose text-screen corruption.
- **Known exclusions:** it reproduced in original VAEG; Sound Off, suppressing
  BEEP PCM registration, suppressing the BEEP event, correct ROM checksums,
  `clk_mult=2`, M29, and M30 did not establish a fix for this command path.
- **Current evidence:** the CPU remains active and repeated FDC Sense Interrupt
  Status polling has been observed, but the exact guest wait condition is not
  yet demonstrated.
- **Next step:** capture a bounded post-command CPU/register trace and compare
  the decisive VA1 and VA2/VA3 control flow before changing FDC or memory code.
- **Evidence:** [M21 diagnostic record](../agents/tasks/M21_sdl2_display_effects.md),
  [build and runtime notes](BUILD.md), and
  [M30 BMS investigation result](../agents/tasks/M30_va_bms_window.md).

### 2D floppy compatibility is not established

- **Status:** open; failed workaround reverted from the exposed feature.
- **Symptom:** generated 2D images produced sector-not-found errors in the VA
  FDD path although older VAEG reportedly read 2D media.
- **Current decision:** blank-image creation exposes only tested 2HD and 2DD
  formats. A double-step adjustment did not pass the human gate and is not
  treated as a completed fix.
- **Evidence:** [M23 formatted FDD task](../agents/tasks/M23_formatted_fdd_images.md).
