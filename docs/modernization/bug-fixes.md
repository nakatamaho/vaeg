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

### State-load rejection feedback disappeared with the State menu

- **Status:** fixed; corrected G55 human gate passed on 2026-07-22.
- **Symptom:** after rebuilding a changed HOSTFAT snapshot, selecting an older
  state correctly refused the load but appeared to do nothing. Reopening the
  State menu was the only way to find the rejection text.
- **Root cause:** the SDL2 frontend stored the preflight error in
  `state_status`, but rendered that string only inside the State menu. Choosing
  a load slot closes that menu before the next frame, so no rejection feedback
  remained visible.
- **Correction:** every rejected state load now opens a root-scope modal and
  blocks guest input until it is dismissed. When a valid state's only blocking
  preflight condition is its HOSTFAT identity (apart from the already accepted
  disk-change warning), the modal offers an explicit `Force load`. That path
  retains the current HOSTFAT mount state and read-only snapshot and warns that
  guest-cached FAT, directory, open-file, or file data may differ.
- **Verification:** Linux and Wine selftests proved strict rejection leaves CPU
  IP and guest memory unchanged. They also proved the explicit override
  restores the saved CPU/memory state without changing the currently mounted
  HOSTFAT digest. A PC-Engine GUI run displayed the mismatch modal, returned to
  the live guest on cancel, and restored the earlier guest state on explicit
  force; the maintainer accepted the focused interaction and explicitly
  declared the corrected G55 human gate passed.
- **Evidence:** [M55 task](../agents/tasks/M55_hostfat_integration.md) and
  [M55 report](../agents/reports/m55_hostfat_integration.md).
- **Commit:** [40b96aca](https://github.com/nakatamaho/vaeg/commit/40b96acaea8b925873d50c33f6fd3fc52dd71eb1).

### HOSTFAT 32 KiB clusters truncated files under PC-Engine

- **Status:** fixed in the M55 human-gate correction; corrected G55 retest
  pending.
- **Symptom:** the proposed 128 MiB HOSTFAT mounted and listed files, but
  PC-Engine reported only about 8 MiB free. More importantly, copying a
  generated 96 KiB file produced only 6144 bytes on writable guest media.
- **Root cause:** M55 used 2048-byte sectors and 16 sectors per cluster, making
  each FAT entry represent 32 KiB. The PC-Engine CONFIG.SYS block-device path
  does not accept that cluster size: the three-entry source chain advanced as
  only 2 KiB per entry. The separate PC-88VA 40 MB SASI layout uses 16 KiB
  clusters and does not establish 32 KiB support for this driver path.
- **Correction:** HOSTFAT now uses 1024-byte sectors and 16 sectors per
  cluster. Its 65,362 visible sectors still yield exactly 4084 FAT12 data
  clusters; reserved cluster identifiers `0FF0H`--`0FF5H` remain unavailable.
  The readable payload limit is 63.71875 MiB.
- **Verification:** a nonzero 96 KiB source copied byte-identically through
  PC-Engine. A separate 4 KiB marker allocated after a 60 MiB filler also
  copied byte-identically, proving that PC-Engine's approximately 8 MiB free-
  space display is not the readable-capacity limit. The snapshot selftest now
  asserts both allocation boundaries, and the generated-driver checker
  requires the corrected BPB.
- **Evidence:** [M55 task](../agents/tasks/M55_hostfat_integration.md) and
  [M55 report](../agents/reports/m55_hostfat_integration.md).
- **Commit:** [14157f7d](https://github.com/nakatamaho/vaeg/commit/14157f7d5888bbc6d1e9243f382506a8ced863a8).

### HOSTFAT Browse opened its popup under a different ImGui ID scope

- **Status:** fixed in M55; corrected G55 retest pending.
- **Symptom:** Configure -> HOSTFAT -> Browse appeared to do nothing on the
  SDL2 frontend.
- **Root cause:** the button called `ImGui::OpenPopup` inside the HOSTFAT child
  region, while `BeginPopupModal` ran later in its parent. ImGui popup IDs are
  relative to the current ID stack, so those two calls addressed different
  popup IDs.
- **Correction:** the child records a one-shot browser request; the parent
  consumes it and calls `OpenPopup` from the same ID scope as
  `BeginPopupModal`.
- **Verification:** GCC, Linux release, and MinGW builds completed, and an
  Xvfb-driven Configure interaction visibly opened the directory selector
  with navigation and Select/Cancel controls.
- **Evidence:** [M55 task](../agents/tasks/M55_hostfat_integration.md) and
  [M55 report](../agents/reports/m55_hostfat_integration.md).
- **Commit:** [5e83dfc9](https://github.com/nakatamaho/vaeg/commit/5e83dfc9a7ab47166c7be46a53c9bcf253307676).

### HOSTFAT discarded host modification timestamps

- **Status:** fixed in the M54 supplemental human-gate correction; maintainer
  timestamp display recheck pending.
- **Symptom:** every HOSTFAT file and directory appeared in PC-Engine with the
  timestamp `80-01-01 00:00` regardless of its host last-write time.
- **Root cause:** the snapshot scanner captured regular-file modification time
  only to detect source mutation, while the directory-entry writer always
  emitted FAT time `0000H` and date `0021H`.
- **Correction:** files, directories, the volume label, `.` and `..` now use
  host local last-write time at FAT's two-second resolution. Values clamp to
  the FAT 1980--2107 range. Directory type and time are checked before and
  after construction so the metadata addition remains transactional.
- **Verification:** GCC, Clang and ASan/UBSan CTest passed with exact FAT-field
  and range-clamp assertions; MinGW compiled the Windows `FILETIME` path and
  its Wine selftest passed. Final hosted and human results are recorded in the
  clean-room report.
- **Evidence:** [M54 clean-room report](../agents/reports/m54_hostfat_cleanroom_reimplementation.md)
  and [M54 task](../agents/tasks/M54_hostfat_readonly_prototype.md).
- **Commit:** [9c707a93](https://github.com/nakatamaho/vaeg/commit/9c707a93bc64ded691c756e205a2b7a0ef42c899).

### HOSTFAT clean-room dispatch emitted unsupported 80386 branches

- **Status:** fixed in the M54 clean-room provenance correction; supplemental
  maintainer gate pending.
- **Symptom:** the independently authored replacement driver initialized far
  enough for PC-Engine to reach `Ready`, but the first HOSTFAT DIR printed the
  initialization message and failed to list the snapshot.
- **Root cause:** NASM's unspecified CPU level relaxed long conditional jumps
  to the 80386 `0F 84H` encoding. NEC V30 does not decode `0F 84H` as that
  conditional branch, so the sector-read command did not reach its handler.
  A same-disk comparison excluded snapshot geometry and host transport; an
  8086-safe build passed with either tested resident-end ordering.
- **Correction:** the source fixes NASM at the 8086 CPU level and implements
  dispatch with short `JNE` plus 8086 `JMP`. The checker decodes all nine
  command edges and rejects all `0F 80H`--`0F 8FH` encodings.
- **Verification:** the final 528-byte driver completed root DIR, TYPE, and
  COPY in a private PC-Engine boot. The 6,780-byte copied file was re-extracted
  from the temporary D88 and matched its source byte-for-byte and by SHA-256.
  Clean GCC, Clang, ASan/UBSan, MinGW/Wine, and hosted results are recorded in
  the clean-room report.
- **Evidence:** [M54 clean-room report](../agents/reports/m54_hostfat_cleanroom_reimplementation.md)
  and [clean-room contract](../agents/research/m54_hostfat_cleanroom_spec.md).
- **Commit:** [bdcbeae8](https://github.com/nakatamaho/vaeg/commit/bdcbeae89b254dd02b8916104baac81c94f94a4d).

### HOSTFAT COPY rejected valid lifecycle requests and misclassified its FAT

- **Status:** fixed in the M54 human-gate correction; remaining G54 media and
  reset checks pending.
- **Symptom:** root-directory listing worked, but PC-Engine COPY first reported
  that the drive's driver could not execute the command. After that rejection
  was isolated, file copying failed during source reads.
- **Root cause:** the driver returned unknown-command status `8103H` for the
  valid `0DH` device-open and `0EH` device-close notifications that bracket
  COPY. Independently, its 8192-sector BPB described 4087 data clusters,
  crossing the 4085-cluster FAT12/FAT16 boundary. PC-Engine consequently read
  the packed FAT12 table as FAT16: cluster 2's successor became `0040H`, and a
  later packed pair became the invalid source LBA `AE18H`.
- **Correction:** open and close are explicit successful no-ops; write and
  write-with-verify remain write-protected. The backing image remains 8192
  sectors, but the BPB and host service expose only 8186 sectors, yielding
  exactly 4084 data clusters. The inaccessible final six sectors are rejected
  by the same pre-transfer range check as every other out-of-range request.
- **Verification:** the generated-driver checker requires both lifecycle
  comparisons, decodes the BPB, and fails if its data-cluster count reaches
  the FAT16 boundary. The ROM-less transport test rejects the first hidden
  sector without modifying guest memory. In a private live PC-Engine boot,
  COPY of neutral `TEST.TXT` returned to `Ready`, and destination DIR reported
  the exact 3958-byte length.
- **Evidence:** [M54 task](../agents/tasks/M54_hostfat_readonly_prototype.md)
  and [M54 report](../agents/reports/m54_hostfat_readonly_prototype.md).
- **Commits:** [bf6896d8](https://github.com/nakatamaho/vaeg/commit/bf6896d801c2d021f44cec43b7070531030c780a),
  [5faa8ca0](https://github.com/nakatamaho/vaeg/commit/5faa8ca0b04aac954a1da3d08c882c32651a0033).

### HOSTFAT used an IBM-sized request layout on PC-Engine

- **Status:** fixed in the M54 human-gate correction; G54 PC-Engine retest
  pending.
- **Symptom:** PC-Engine printed `HOSTFAT read-only drive ready` while loading
  `HOSTFAT.SYS`, then hung before completing CONFIG.SYS processing.
- **Root cause:** both the guest driver and emulator service treated the
  non-IBM block request as an 18-byte packet. PC-Engine uses a 13-byte common
  header, including eight reserved bytes at offsets 5--12, followed by the
  media/unit byte at `0DH`, transfer pointer at `0EH`, count or BPB pointer at
  `12H`, and starting sector at `14H`. The wrong initialization offsets made
  PC-Engine reclaim most of the resident driver and later execute overwritten
  code.
- **Correction:** the driver and host service now use the complete 22-byte
  layout. The ROM-less transport test fills the reserved header bytes with a
  nonzero pattern, and the generated-driver checker verifies the exact field
  displacements in emitted machine code so the former layout fails closed.
- **Verification:** clean GCC, Clang, and ASan/UBSan suites pass all 36 tests
  apart from the configured external-corpus skip; the former generated SYS is
  rejected by the strengthened checker and two independent corrected NASM
  outputs are byte-identical. A private live PC-Engine boot printed the ready
  message and reached the command prompt instead of hanging. G54 retains live
  DIR/TYPE/copy/write-protect and reset checks.
- **Evidence:** [M54 task](../agents/tasks/M54_hostfat_readonly_prototype.md)
  and [M54 report](../agents/reports/m54_hostfat_readonly_prototype.md).
- **Commit:** [a07a8c4a](https://github.com/nakatamaho/vaeg/commit/a07a8c4a764a2b5d8560bdbaea8f5ebc5c0edae4).

### VA mode did not expose the emulator-private value/string channels

- **Status:** fixed in M54; G54 PC-Engine integration review pending.
- **Symptom:** a PC-88VA guest driver using vaeg's established emulator-private
  interface could not exchange the scalar values required by a request-packet
  protocol. The generic and VA I/O paths also disagreed about whether ports
  `07EDH` and `07EFH` were present.
- **Root cause:** `np2sysp_bind()` attached both scalar and string callbacks to
  `07EFH`, so the later string attachment replaced the scalar callback. It
  attached only the generic I/O table even though active VA execution uses the
  separate VA table.
- **Correction:** `07EDH` now carries four-byte values and `07EFH` carries
  command/response strings in both the generic and VA tables. These remain
  emulator-private channels; no physical PC-88VA port was reassigned.
- **Verification:** the M54 ROM-less test sends a version probe through both
  tables, performs a sector transfer through the VA table, and verifies that
  malformed packet, count, LBA, destination, and unmounted-image failures do
  not modify the guest destination. G54 retains a real PC-Engine driver gate.
- **Evidence:** [M54 task](../agents/tasks/M54_hostfat_readonly_prototype.md)
  and [M54 report](../agents/reports/m54_hostfat_readonly_prototype.md).
- **Commit:** [f79b677c](https://github.com/nakatamaho/vaeg/commit/f79b677c1e48071779349a4ac3b404ed291f821a).

### REP-prefixed 0F could enter unverified 80286 protected-mode behavior

- **Status:** fixed by the G47-approved M48 fail-closed policy; G48 human
  review pending.
- **Symptom:** F2/F3-prefixed 0F could run inherited NP2 80286 system handlers;
  in particular, F2/F3 0F 01 F0 could set `MSW.PE`. A saved state with
  `MSW.PE` set could then activate legacy selector processing after import.
- **Root cause:** `v30op_repne[0x0f]` and `v30op_repe[0x0f]` inherited
  `i286c_cts`, while CPU286 state validation accepted `MSW.PE`. M47's source,
  runtime, and state audit demonstrated both active paths. No primary source
  or pinned V20 record establishes that behavior as correct uPD9002/V52
  semantics.
- **Correction:** both dispatch slots now latch an emulator diagnostic and
  restore the complete pre-instruction runtime state before DMA or VA-device
  scheduling. State preflight rejects `MSW.PE` transactionally; dormant
  descriptor residue with PE clear remains opaque and compatible. This is a
  safety policy, not an architectural instruction claim.
- **Verification:** a 522-case regression covers all F2/F3+0F second bytes,
  segment-prefix entry, PE-set runtime state, full CPU-state equality, memory
  equality, and persistent stop. Direct and full-file state tests prove the
  new rejection leaves CPU runtime, compatibility image, PCCORE, UPD9002, and
  memory unchanged. The M48 transition manifest accounts for every graph,
  provenance, and support row while preserving all M42/M43 historical files.
- **Evidence:** [ADR-0013](../agents/DECISIONS/ADR-0013-upd9002-rep0f-correctness.md),
  [M48 task](../agents/tasks/M48_upd9002_rep0f_implementation.md), and
  [M48 report](../agents/reports/m48_upd9002_rep0f_implementation.md).
- **Commits:** [bc00b370](https://github.com/nakatamaho/vaeg/commit/bc00b370480283dbf7f7529fc6345def87a7dc75)
  and [9924b85c](https://github.com/nakatamaho/vaeg/commit/9924b85ca13a87610571392968ca63bd74e85321).

### Invalid CPU286 state payloads could partially alter the machine

- **Status:** fixed in M44 implementation; G44 human review pending.
- **Symptom:** a malformed CPU286 section, or a legacy payload selecting a CPU
  type other than V30, could be discovered only while loading raw live state,
  after unrelated machine sections had begun to change.
- **Root cause:** the CPU286 section used the generic raw binary statsave path,
  which had no CPU-specific size/type validation and no complete preflight
  before live-section application.
- **Correction:** introduced a dedicated CPU286 serialization adapter and
  statsave handler. It validates the complete temporary payload, requires
  `CPUTYPE_V30`, constructs temporary runtime state, and commits the runtime and
  opaque compatibility image together only after full-file preflight.
- **Verification:** invalid CPU type, malformed declared size, and truncation
  tests preserve CPU runtime, compatibility bytes, PCCORE, UPD9002 registers,
  and memory. Raw-G41/current bidirectional tests preserve every valid CPU286
  and UPD9002 payload byte, including reset and CPU_SHUT behavior.
- **Evidence:** [M44 state-boundary report](../agents/reports/m44_upd9002_state_boundary.md)
  and [M44 task](../agents/tasks/M44_upd9002_state_boundary.md).
- **Commits:** [2895c113](https://github.com/nakatamaho/vaeg/commit/2895c11354c73b1758b6c06fad4b5c5ec8e68570)
  and [8e709db3](https://github.com/nakatamaho/vaeg/commit/8e709db3431a3a7c64f7040c4cf719e5102de559).

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

- **Status:** fixed in M32; macOS arm64 archive-browser check passed.
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
  and is removed by the existing prune lifecycle.
- **Verification:** ROM-less dropmedia tests cover ZIP and 7z source capture,
  metadata reload, drive/path matching, and unrelated-path rejection. Two
  apparent macOS arm64 failures used a stale executable copied to the wrong
  destination and were not valid results. With the `ce26003` build installed
  at the actual test location, the maintainer confirmed that FDD Open starts
  in the source ZIP directory and declared the defect fixed. The macOS
  MacPorts and MinGW cross targets build successfully and the full ROM-less
  selftest passes.
- **Evidence:** [M32 command-line startup task and G32 follow-up](../agents/tasks/M32_cli_startup_overrides.md#g32-archive-browser-follow-up).
- **Commit:** [ce26003782cec9b93639cc34b2e33c5de3e63d8a](https://github.com/nakatamaho/vaeg/commit/ce26003782cec9b93639cc34b2e33c5de3e63d8a).

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

### VA BMS bank zero hid 128KB of conventional memory

- **Status:** M30's open-bus interpretation was disproved and corrected in
  M52; G52 passed.
- **Symptom:** enabling I/O Bank Memory with 640KB main memory prevented
  CONFIG.SYS RAM-disk and MSE registration. Reducing main memory to 512KB
  avoided the failure by leaving `80000H-9FFFFH` unused.
- **Affected scope:** CPU byte/word and SGP word access to the 128KB BMS
  aperture, both while BMS was disabled and after the driver reset the bank
  selector to zero.
- **Demonstrated root cause:** the M30 portable handlers treated selector zero
  as expansion bank zero and permanently overlaid `80000H-9FFFFH`. RDBMS 1.21
  instead selects a nonzero bank for each transfer and writes zero in its
  `ResetBank` macro to restore conventional memory.
- **Correction:** selector zero now passes through ordinary main RAM; selectors
  1 through N map one-to-one onto N allocated 128KB banks. Invalid nonzero
  selectors remain open bus. CPU and SGP paths use the same rule.
- **Verification:** the updated ROM-less lifecycle test covers disabled and
  enabled selector-zero pass-through, bank-one isolation, ordinary-reset
  retention, and disable-time restoration. Maintainer guest testing with
  640KB main memory is the G52 gate.
- **Evidence:** [M30 historical task](../agents/tasks/M30_va_bms_window.md) and
  [M52 corrected I/O Bank Memory task](../agents/tasks/M52_io_bank_memory.md).
- **Commit:** [5eb04ae9](https://github.com/nakatamaho/vaeg/commit/5eb04ae91a9900833096bb43b3b599d358c099c5).

### VA bank memory defaulted to the PC-9801 compatibility port

- **Status:** fixed in the M52 implementation; G52 passed.
- **Symptom:** a clean VAEG configuration selected `00ECH`, so a PC-88VA bank
  memory driver configured for the machine-native `01D0H` control port could
  not select the emulated banks without a matching manual configuration
  change.
- **Affected scope:** clean configurations and invalid persisted BMS port
  values. An explicitly saved valid `00ECH` selection remains supported and
  is not migrated.
- **Demonstrated root cause:** the restored portable dialog inherited the
  first generic BMS choice from the frozen frontend. The bundled historical
  specification help identifies `00ECH` as the PC-9801 choice and `01D0H` as
  the PC-88VA-01/02 choice, but the active default still used `00ECH`.
- **Correction:** made `01D0H` the active clean-config default, invalid-value
  fallback, and first GUI choice while retaining `00ECH` as an explicit
  compatibility option. BMS remains disabled by default.
- **Verification:** the ROM-less selftest checks the exact port constants and
  copied runtime default; Linux and MinGW clean release builds pass their
  selftests, including BMS configuration/window lifecycle coverage. A
  black-box run loading `BMS_Port=1234` logs fallback to `01d0` before machine
  startup.
- **Evidence:** [M52 I-O Bank Memory task](../agents/tasks/M52_io_bank_memory.md).
- **Commit:** [e9ad63e3](https://github.com/nakatamaho/vaeg/commit/e9ad63e3d720e8dad14d5a63289f3d3443b54422).

### Z80 state-codec rejection was ignored by the state coordinator

- **Status:** fixed in M39.
- **Symptom:** a save whose embedded Z80 status revision was unsupported could
  continue through `statsave_load()` as though the subsystem CPU had restored
  successfully, leaving a partially loaded machine rather than reporting the
  incompatible state.
- **Affected scope:** revision-1 subsystem state loading under both production
  Z80 selections; valid revision-1 images are unchanged.
- **Demonstrated root cause:** the subsystem C bridge returned `void` and
  discarded `Z80C::LoadStatus()`'s Boolean result; `flagload_subsystemcpu()`
  therefore had no failure to propagate.
- **Correction:** return success/failure from the subsystem save/load bridge
  and convert a codec rejection into `STATFLAG_FAILURE` in the existing state
  coordinator. No scheduler or status-image layout changed.
- **Verification:** the ROM-less state test saves a valid complete state,
  copies it, changes only revision-1 byte 59 to unsupported revision 2, and
  requires top-level `statsave_load()` failure under both `legacy` and
  `suzukiplan`; the original image then loads and re-saves successfully.
- **Evidence:** [M39 integration task](../agents/tasks/M39_z80_integration.md)
  and [M39 integration contract](z80-integration.md#state-boundary-and-error-handling).
- **Commit:** [23b7071](https://github.com/nakatamaho/vaeg/commit/23b70711b84deb027a1c8dbf11e6284b65d0d4fe).

## Open Defects

### Legacy Z80 reset leaves saved undocumented flag bits uninitialized

- **Status:** open; demonstrated during M34 contract capture, with no behavior
  change authorized in that milestone.
- **Symptom:** a revision-1 Z80 state saved immediately after reset can depend
  on an indeterminate `xf` byte, and architectural F bits 3 and 5 can inherit
  that value when flags are materialized.
- **Demonstrated root cause:** `Z80C::Reset()` zeroes the register structure and
  lazy-flag mask but does not initialize member `xf`; `GetAF()` merges `xf`
  into F, and `SaveStatus()` serializes it.
- **Current containment:** the M34 ROM-less legacy fixtures execute `XOR A`
  before capture so their bytes are deterministic. This avoids the defect in
  evidence generation but does not correct production reset behavior.
- **Next step:** decide in a separately authorized correctness milestone
  whether to initialize `xf` in the legacy path or correct it only at the M41
  replacement cutover, then add a reset/save regression test and human gate.
- **Evidence:** [M34 legacy Z80 contract](z80-legacy-contract.md#verified-legacy-execution-behavior)
  and [ADR-0011](../agents/DECISIONS/ADR-0011-z80-migration.md#consequences-and-unresolved-risks).

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
