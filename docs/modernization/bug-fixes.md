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

### uPD9002 FF /7 executed a POP-like operation instead of the observed stack push

- **Status:** fixed in the M65 residue campaign; formal approval deferred to
  terminal G65m.
- **Symptom:** all 5,000 applicable G65 `FF /7` SST cases terminated normally
  but failed final architectural comparison. The expected state decremented
  `SP` by two and wrote the selected `r/m16` value to the stack; the actual
  state followed the inherited POP-like dispatch.
- **Root cause:** the active `FF` ModR/M group table routed `/7` to the
  obsolete `_pop_ea16` helper. The reconstructed M65a evidence proves the
  observable `FF /7` contract is a stack push, including the `r/m = SP` alias
  writing the decremented `SP`.
- **Correction:** `FF /7` now uses an M65a-owned push helper. It preserves the
  existing `FF /6` helper for the later M65d-owned SP-alias residue.
- **Verification:** the focused `vaeg_upd9002_m65a_ff7` test covers register,
  memory, and SP-alias forms. The selective M65a replay ran the exact 5,000
  owned hashes as `5,000 pass / 0 fail`, with zero timeout/crash and an M65d
  guard preserving the exact 144 `FF /6` G65 failures.
- **Evidence:** [M65a report](../agents/reports/m65a_upd9002_ff7.md) and
  [M65 expected/actual reconstruction report](../agents/reports/m65_campaign_expected_actual_reconstruction.md).
- **Commit:** [15f2ac8e](https://github.com/nakatamaho/vaeg/commit/15f2ac8e861c3cfedbc12acc9ef470925d00716c).

### uPD9002 BOUND used unsigned range comparison

- **Status:** fixed in the M65 residue campaign; formal approval deferred to
  terminal G65m.
- **Symptom:** 1,244 applicable G65 `62` BOUND SST cases failed final
  architectural comparison. Some cases expected a type-5 event but completed
  normally; others expected normal completion but entered type 5.
- **Root cause:** the active BOUND implementation compared the register
  operand and memory bounds as unsigned 16-bit values. The reconstructed M65b
  evidence proves the observable contract uses signed 16-bit lower and upper
  bounds with inclusive boundaries.
- **Correction:** BOUND now converts the selected register, lower bound, and
  upper bound to `SINT16` before applying the inclusive range decision. It
  continues to use the existing effective-address and synchronous type-5
  event-entry paths.
- **Verification:** the focused `vaeg_upd9002_m65b_bound` test covers
  signed lower/upper inclusivity, negative, positive, and cross-zero ranges,
  segment override, offset wrapping, physical wrapping, and type-5 frame
  preservation. The selective M65b replay ran the exact 1,244 owned hashes as
  `1,244 pass / 0 fail`, with zero timeout/crash, M65a `FF /7` protection,
  M65d `FF /6` guard preservation, and 3,565 former BOUND frame-only hashes
  still passing.
- **Evidence:** [M65b report](../agents/reports/m65b_upd9002_bound.md) and
  [M65 expected/actual reconstruction report](../agents/reports/m65_campaign_expected_actual_reconstruction.md).
- **Commit:** [d0e01694](https://github.com/nakatamaho/vaeg/commit/d0e01694a9b82b4cd16500743d77e45459c74be1).

### uPD9002 F7 /2 word NOT updated only the low memory byte

- **Status:** fixed in the M65 residue campaign; formal approval deferred to
  terminal G65m.
- **Symptom:** 1,113 applicable G65 `F7 /2` word NOT memory SST cases failed
  final architectural comparison. The expected RAM contained both complemented
  operand bytes, while the actual RAM complemented only the low byte.
- **Root cause:** the `_not_ea16` memory fast path applied `^= 0xffff` through
  the byte pointer `mem + madr`, so only the low byte was modified. The
  inhibited word path already used the 16-bit memory read/write helpers.
- **Correction:** `_not_ea16` now loads the little-endian word with
  `LOADINTELWORD`, complements all 16 bits, and stores the complete word with
  `STOREINTELWORD` on the direct memory path.
- **Verification:** the focused `vaeg_upd9002_m65c_f72` test covers register
  protection, low-memory word writes, odd-address word-path protection,
  segment override, indexed displacement, offset boundary behavior, high
  memory path preservation, FLAGS, IP, and neighbor preservation. The M65c
  replay ran the exact 1,113 owned hashes as `1,113 pass / 0 fail` and the
  complete selected `F7 /2` population as `5,000 pass / 0 fail`, with zero
  timeout/crash and M65a, M65b, M65d, and M65e guards preserved.
- **Evidence:** [M65c report](../agents/reports/m65c_upd9002_f72.md) and
  [M65 expected/actual reconstruction report](../agents/reports/m65_campaign_expected_actual_reconstruction.md).
- **Commit:** [8d338a52](https://github.com/nakatamaho/vaeg/commit/8d338a528a7c3b4a18636f2f3a4678ece6dbcd4f).

### uPD9002 FF /6 pushed the old SP value for the SP register operand

- **Status:** fixed in the M65 residue campaign; formal approval deferred to
  terminal G65m.
- **Symptom:** 144 applicable G65 `FF /6` SST cases failed final RAM
  comparison. All owned cases were register-form `r/m = SP` rows. The
  predecessor decremented `SP` but pushed the pre-decrement SP value.
- **Root cause:** `_push_ea16` read the register operand before invoking the
  push macro. For the SP alias case, the observable pushed value must be the
  decremented SP value produced by the push.
- **Correction:** `_push_ea16` now captures `SP - 2` for the register
  `r/m = SP` case before using the existing push path. Other register and
  memory-source forms remain unchanged.
- **Verification:** the focused `vaeg_upd9002_m65d_ff6` test covers SP alias,
  segment-prefixed SP alias, non-SP register, memory operand, and stack-wrap
  cases. The M65d replay ran the exact 144 owned hashes as
  `144 pass / 0 fail` and the complete selected `FF /6` population as
  `5,000 pass / 0 fail`, with zero timeout/crash and M65a, M65b, BOUND frame,
  M65c, and M65e guards preserved.
- **Evidence:** [M65d report](../agents/reports/m65d_upd9002_ff6.md) and
  [M65 expected/actual reconstruction report](../agents/reports/m65_campaign_expected_actual_reconstruction.md).
- **Commit:** [5cfc3540](https://github.com/nakatamaho/vaeg/commit/5cfc3540b5f1d78a7aace699d51729d272529552).

### uPD9002 wrapped segment-offset word accesses used contiguous linear bytes

- **Status:** fixed in the M65 residue campaign; formal approval deferred to
  terminal G65m.
- **Symptom:** the exact ten-case M65e tail failed final architectural
  comparison across `61`, `81 /6`, `FF /5`, `A5`, `9C`, `D1 /6`, `C8`, and
  `C4` forms. The mismatches involved registers, FLAGS, or represented RAM
  when a word operand crossed offset `0xffff`.
- **Root cause:** several inherited word paths treated the second byte as the
  next contiguous physical byte after `segment_base + 0xffff` rather than the
  byte at the same segment base with offset `0x0000`.
- **Correction:** M65e adds segment-offset word helpers and applies them only
  to the proven tail paths: `POPA`, V30 `PUSHF`, wrapped memory word ALU and
  shift operations, `MOVSW`, `LES`/`LDS`, far pointer fetches, and `ENTER`
  frame-copy reads and writes. Generic stack macros remain protected.
- **Verification:** the focused `vaeg_upd9002_m65e_tail10` test covers all
  eight structural tail forms. The M65e replay ran the exact ten owned hashes
  as `10 pass / 0 fail`; the original 7,511-hash G65 architectural residue
  replayed as `7,511 pass / 0 fail`, with zero timeout/crash and all M65a
  through M65d protected populations preserved.
- **Evidence:** [M65e report](../agents/reports/m65e_upd9002_tail10.md) and
  [M65 expected/actual reconstruction report](../agents/reports/m65_campaign_expected_actual_reconstruction.md).
- **Commit:** [c7bb5ee2](https://github.com/nakatamaho/vaeg/commit/c7bb5ee274441d608096e4a33e2eca5a2d5af3a4).

### uPD9002 segmented word helpers bypassed mapped-memory dispatch

- **Status:** fixed in M68; G68 approved at
  `d1e0225c4edb716893fe5579283fbf0915db72b9`.
- **Symptom:** PC-Engine/MS-DOS text output stopped scrolling normally after
  reaching the bottom row. New output repeatedly overwrote the last text row
  instead of moving existing lines upward.
- **Root cause:** the M65e A5/MOVSW segmented-word path delegated through a
  helper that correctly calculated 16-bit segment wrapping but then
  independently selected a flat `mem[]` fast path. For VA TVRAM and BMS
  mapped regions this bypassed the canonical mapped-memory dispatcher,
  callbacks, dirty/display side effects, and the active backing store such as
  `textmem[]`.
- **Correction:** segmented word helpers now own only segment-offset address
  formation and `0xffff`-to-`0x0000` wrapping. Contiguous words delegate to the
  canonical generic word API, and only the noncontiguous segment-wrap case
  splits into canonical byte accesses. No A5-, TVRAM-, BMS-, or `A0000h`-
  specific special case was added.
- **Verification:** focused M68 mapped-memory probes fail on the predecessor
  for the flat-`mem[]` bypass and pass after the fix for TVRAM, BMS, normal
  RAM, segmented word reads/writes, REP and non-REP MOVSW, DF=0/1, `FFFEh`,
  and `FFFFh -> 0000h` wrapping. The full architectural CI/full and
  fingerprint-full SST profiles preserve the approved G67 counts and digests,
  and the maintainer reported the PC-Engine/MS-DOS manual gate passed.
- **Evidence:** [M68 report](../agents/reports/m68_upd9002_segmented_word_mapped_dispatch.md).
- **Commit:** [90258f26](https://github.com/nakatamaho/vaeg/commit/90258f26207b7ce7dc3473a5df2811da4bb0c19c).

### IDP/TSP 0142H status reads erased stored status flags

- **Status:** fixed in M69; G69 remains pending human review.
- **Symptom:** reading the IDP/TSP `0142H` status port could return only
  `00H` or `40H`. Stored flags such as BUSY at bit 2 were erased, and any
  nonzero stored status falsely produced the VB bit.
- **Root cause:** `tsp_i142()` used the expression
  `tsp.status | (tsp.vsync) ? STATUS_VB : 0`. C parses that expression as
  `(tsp.status | tsp.vsync) ? STATUS_VB : 0`, so the stored status was first
  converted to a Boolean condition and then replaced by `STATUS_VB` or zero.
- **Correction:** the `0142H` input path now starts from `tsp.status` and
  orthogonally ORs `STATUS_VB` when `tsp.vsync` is active. No command timing,
  parameter-port behavior, rendering, state format, or broader IDP/TSP status
  semantics were changed.
- **Verification:** M69 adds a registered-I/O-path regression test covering
  the minimum BUSY/VB truth table, every non-VB single bit, representative
  combinations, VB idempotence, word `IN 0142H`, observable BUSY during
  `CMD_SYNC`, and exhaustive `256 x 2` stored-status/VB composition. The test
  fails on the predecessor with 508 exhaustive row failures and passes after
  the correction. Native non-external CTest, ASan/UBSan focused tests, MinGW
  build, repository invariants, and maintainer PC-Engine/MS-DOS runtime
  validation passed.

### WD33C93 PIO register access advanced the DATA/COMMAND windows

- **Status:** open; M75b2 fixes the demonstrated register-boundary defect,
  while the post-SELECT phase engine remains in progress.
- **Symptom:** the PCPLUS/SCHD low-level `07h Select without ATN` path
  stopped after CSR `11h`/COMMAND request.  The controller model treated the
  selected register as an ordinary auto-incremented byte file, cleared
  Auxiliary Status on `0CC0h` reads, and had no explicit DBR/CIP/BSY
  composition.  This could not reproduce the WD33C93 PIO host contract for
  the fixed DATA window or distinguish device CSR consumption from an 8259
  EOI.
- **Root cause:** the active C-Bus implementation conflated the WD33C93
  register address counter with the fixed COMMAND/DATA windows and used the
  emulated PIC path as the only observable interrupt state.  The complete
  M75a trace separately proves PIO mode (`Control=08h`, `0CC4h=02h`) and a
  missing post-`8Ah` transfer sequence; no DMA behavior is implicated.
- **Correction:** M75b2 adds fixed AR `18h`/`19h` windows, composes Auxiliary
  Status from DBR/CIP/BSY/PE plus the depth-one CSR latch, consumes the device
  CSR only through AR `17h`, and records unsupported 0CC4h DMA strobes and
  NEC AR `32h`/`34h`/`35h` accesses as hardware-pending.  DMA and 0CC6h
  hardware claims remain outside this checkpoint.
- **Verification:** `tools/qa/m75_scsi_controller.py`, the Linux debug
  build, and `vaeg --selftest` pass after the correction.  The guest trace
  still stops before AR `12h`-`14h`, AR `18h=20h`, and AR `19h` CDB transfer;
  the remaining phase-engine gap is tracked by M75c.
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md) and
  [M75 task](../agents/tasks/M75_scsi_support.md).
- **Commit:** [7b5672b](https://github.com/nakatamaho/vaeg/commit/7b5672b9f6823d92f86b17592878f928c133e76b).
- **Evidence:** [M69 report](../agents/reports/m69_upd9002_idp_0142_status_composition.md).
- **Commit:** [6ef4f98e](https://github.com/nakatamaho/vaeg/commit/6ef4f98ec1be20054db2aeb9c4a44c6a3d2e36bf).

### PCPLUS STATUS requests were exposed before target phase readiness

- **Status:** corrected in M75d1; full G75 acceptance remains pending.
- **Symptom:** the PCPLUS/SCHD low-level `07h Select without ATN` path received
  `CSR=8Bh`, but its phase handoff was consumed by the main event pump before
  the foreground transfer setup returned. The guest then did not issue the
  STATUS `AR=18h <- 20h` request.
- **Root cause:** the controller released a pending target phase as soon as
  the preceding CSR was consumed by `AR=17h`. CSR-latch consumption and target
  readiness are separate gates. The raw `8Bh` was normalized correctly; the
  event was simply visible while the previous completion path was still active.
- **Correction:** pending phase requests now hold DBR low until a general
  controller processing event exposes the target REQ. Same-phase PIO
  continuation does not incur a phase-transition event. No guest address,
  CDB, filename, or transfer-count special case was added. REQUEST SENSE is
  implemented with a fixed no-sense DATA IN response because it is observed in
  the PCPLUS probe before INQUIRY.
- **Verification:** the real PCPLUS trace now shows `CSR=8Bh` at the guest
  `1B67h` wait path, `CS:[047Eh]=3Bh` after consumption, `1BA1h -> 1C14h ->
  1C32h`, STATUS data `00h` with `CSR=1Bh`, MESSAGE data `00h` with `CSR=1Fh`,
  and REQUEST SENSE DATA IN beginning with `70h`. Static M75 validation,
  focused CTest, Linux SDL2 build, and `--selftest` pass. The bounded trace
  did not yet reach the later 36-byte INQUIRY golden, so this entry remains
  separate from terminal G75 approval.
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md) and
  [M75 task](../agents/tasks/M75_scsi_support.md).
- **Commits:** [1cd3edb](https://github.com/nakatamaho/vaeg/commit/1cd3edb2b3bcde572f00b8a1131bd05f66ee3bff),
  [e56a16e](https://github.com/nakatamaho/vaeg/commit/e56a16edf18985f95d78868fed73031a77514e88),
  [9e6919f](https://github.com/nakatamaho/vaeg/commit/9e6919fdb5aa04b2a8d100ace89747c73a5059fc),
  [07c1074](https://github.com/nakatamaho/vaeg/commit/07c1074a2c664e4b88bf4c8731cbcf4c5c4ad666).

- **Follow-up:** [4ab457b](https://github.com/nakatamaho/vaeg/commit/4ab457bd8361bed27fd8e09eaa25bbbe97644ed0)
  preserves DATA IN cursor state across repeated one-byte PIO requests, and
  [dafeae0](https://github.com/nakatamaho/vaeg/commit/dafeae0da3b4657371a6181b72c40874db30905c) resets that
  cursor only at actual phase boundaries.  Compact diagnostic controls are
  provided by [dca6090](https://github.com/nakatamaho/vaeg/commit/dca6090a01076f244697be157f2aa4d80eec3d20) and
  [52f9788](https://github.com/nakatamaho/vaeg/commit/52f9788a5c7d2c86beadddea3e4a135e7536046c).

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

### The M48 diagnostic-stop poll slowed active uPD9002 runtime execution

- **Status:** fixed in M73; G73 human review pending.
- **Symptom:** maintainer runtime testing found that older CI binaries through
  approved G43 remained fast, while the approved G48 binary was slow on the
  same notebook workload. The slowdown was host-runtime visible and did not
  require an SST semantic mismatch.
- **Root cause:** M48 correctly added fail-closed REP-prefixed `0F`
  diagnostics, but the scheduler hot loop tested the diagnostic latch through
  an out-of-line `upd9002_diagnostic_pending()` function after every
  `v30c_step()`. For ordinary execution the result is almost always false, so
  this became a high-frequency call overhead on every uPD9002 instruction.
- **Correction:** the diagnostic state remains a single shared latch, but the
  pending test is now an inline macro reading the latch reason directly. The
  REP+0F fail-closed behavior, message, state atomicity, and diagnostic getter
  remain unchanged.
- **Verification:** the focused M48 diagnostic, M68 mapped-memory, M69 status,
  M70 prefix/string, full ROM-less CTest suite, normal macOS build, and MinGW
  cross build passed. The MinGW executable
  `/tmp/vaeg-m73-inline-diagnostic.exe` had SHA-256
  `cd3cd52fc5b83b9831acebfd7a2b1178b4ad7c18e657f2bd0bbd3e47cc547221`; the
  maintainer reported it was fast.
- **Evidence:** [M73 task](../agents/tasks/M73_upd9002_post_m49_performance_regression.md)
  and [M73 report](../agents/reports/m73_upd9002_post_m49_performance_regression.md).
- **Commit:** [e7ac7e93](https://github.com/nakatamaho/vaeg/commit/e7ac7e930c685e565bff131a42fe48f08c799990).

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

### uPD9002 guest-visible FLAGS images lost or loaded reserved bits

- **Status:** fixed in the M60a implementation; G60a candidate review pending.
- **Symptom:** the saved FLAGS word in every executing `INT3`, `INT imm8`, and
  interrupting `INTO` frame lost bits 12 through 15. `POPF` and `SAHF` also
  loaded reserved bits 3 and 5 instead of forcing them to zero.
- **Affected scope:** guest-visible FLAGS materialization for software
  interrupt frames and `PUSHF`, plus the SST-observed loading rules for
  `POPF` and `SAHF`. Interrupt eligibility, vectoring, frame placement,
  stack addressing, `IRET`, and final FLAGS comparison contracts are
  unchanged.
- **Demonstrated root cause:** the software-interrupt path saved
  `REAL_FLAGREG`, which masks the internal FLAGS word to 12 bits. The V30
  `POPF` path copied the popped low FLAGS bits directly, and `SAHF` copied AH
  directly, so neither path applied the target-observed zero rule for bits 3
  and 5.
- **Correction:** construct the interrupt and `PUSHF` images explicitly from
  all 16 stored FLAGS bits plus the split overflow bit, and apply explicit,
  instruction-specific masks when loading `POPF` and `SAHF`. `LAHF` remains
  unchanged.
- **Verification:** focused deterministic tests cover ordinary, 64-KiB
  segment-wrap, and 20-bit physical-wrap frames and all affected bit rules.
  The complete architectural SST population improved from 84,329 to 60,582
  failures: all 19,968 directly targeted records passed, as did 3,565 BOUND
  and 214 divide-fault records that use the corrected saved-FLAGS primitive.
  No hash became newly failing, and all required profiles completed with zero
  timeout or crash. Hosted Linux GCC/Clang/ASan, macOS, Windows-MinGW, Z80
  conformance, repository-invariant, and architectural-ratchet jobs passed.
- **Evidence:** [M60a task](../agents/tasks/M60a_upd9002_flags_materialization.md)
  and [M60a report](../agents/reports/m60a_upd9002_flags_materialization.md).
- **Commit:** [aab78b78](https://github.com/nakatamaho/vaeg/commit/aab78b78a2473ce35b1e28a9af7420e46e72a1c4).

### uPD9002 IRET loaded reserved FLAGS bits 3 and 5

- **Status:** fixed in the M60e implementation; G60e candidate review pending.
- **Symptom:** 3,769 of the 5,000 applicable `CF IRET` SST records failed
  because the restored FLAGS value retained reserved bits 3 and 5 from the
  stack.
- **Affected scope:** real-mode uPD9002 `IRET` FLAGS restoration only. Stack
  word order, logical and physical stack addresses, restored IP and CS, final
  SP, termination, interrupt entry, and other FLAGS instructions are
  unchanged.
- **Demonstrated root cause:** the V30 `IRET` path masked the popped FLAGS word
  with `0x0fff`, which made bits 3 and 5 loadable. Complete pre-fix replay
  showed that the SST-observed rule forces both bits to zero while every other
  observed IRET rule already matched.
- **Correction:** use the IRET-specific `0x0fd7` stack mask, clearing only
  bits 3 and 5. The underdetermined bit 8 and the existing internal high-FLAGS
  representation are deliberately preserved.
- **Verification:** deterministic focused tests cover ordinary, 16-bit
  segment-wrap, and 20-bit physical-wrap stack reads and explicit FLAGS bit
  rules. The complete CF population improved from 1,231 pass / 3,769 fail to
  5,000 pass / 0 fail. Architectural full failures fell from 59,941 to
  56,172, with no newly failing hash, timeout, crash, or protected-form
  regression.
- **Evidence:** [M60e IRET task](../agents/tasks/M60e_upd9002_iret.md) and
  [M60e report](../agents/reports/m60e_upd9002_iret.md).
- **Commit:** [7f815acb](https://github.com/nakatamaho/vaeg/commit/7f815acb26f1be546bbcfd5de12972235dfd175c).

### uPD9002 C6/C7 register forms wrote the ModR/M extension register

- **Status:** fixed in the M61 implementation; G61 candidate review pending.
- **Symptom:** 1,088 C6 and 1,120 C7 applicable SST records failed. The
  encoded destination did not receive the immediate, and a different register
  could change instead.
- **Affected scope:** register-destination forms of `C6 /0 MOV r/m8, imm8` and
  `C7 /0 MOV r/m16, imm16`. Their memory forms and all other instruction
  families are unchanged.
- **Demonstrated root cause:** both register-form paths selected ModR/M bits
  5:3 through `REG8_B53` or `REG16_B53`. Complete pre-fix replay and direct
  code inspection show that the executed SST population selects the
  destination through r/m bits 2:0. The 161 C6 and 154 C7 pre-fix
  register-form passes are exactly the records where those two fields happen
  to name the same register; they are not value-coincidence passes.
- **Correction:** select the byte or word register through `REG8_B20` or
  `REG16_B20`. Immediate fetch, instruction length, FLAGS, termination, and
  the memory paths are unchanged.
- **Verification:** focused tests cover all eight byte-register encodings, all
  eight word-register encodings, immediate edge values, paired-byte and
  unrelated-register preservation, and representative memory displacement,
  segment-override, 16-bit offset-wrap, and 20-bit physical-wrap cases.
  Complete G61 SST results are recorded in the milestone report.
- **Evidence:** [M61 task](../agents/tasks/M61_upd9002_mov_immediate_register.md)
  and [M61 report](../agents/reports/m61_upd9002_mov_imm_register.md).
- **Commit:** [90fa7dec](https://github.com/nakatamaho/vaeg/commit/90fa7dec5d46708a807851f61ae0792ee39e9b8f).

### uPD9002 AAM ignored its encoded radix

- **Status:** fixed in the M62 implementation; G62 candidate review pending.
- **Symptom:** 4,803 of the 5,000 applicable `D4 AAM` SST records failed.
- **Affected scope:** real-mode uPD9002 `AAM imm8` result and FLAGS
  materialization. `D5 AAD` is unchanged.
- **Demonstrated root cause:** the active handler skipped the encoded
  immediate and always divided AL by 10. It also derived SZP from AX rather
  than the final AL and retained unrelated FLAGS state.
- **Correction:** consume the immediate as the SST-observed radix, place the
  quotient and remainder in AH and AL, apply the observed immediate-zero
  normal-result rule, and materialize the exact result FLAGS.
- **Verification:** the complete D4 population is 5,000 pass / 0 fail,
  including radix values 0, 1, 2, 9, 10, 11, 16, and 255; D5 remains 5,000
  pass / 0 fail.
- **Evidence:** [M62 task](../agents/tasks/M62_upd9002_semantics_bundle.md)
  and [M62 report](../agents/reports/m62_upd9002_semantics_bundle.md).
- **Commit:** [c55e5730](https://github.com/nakatamaho/vaeg/commit/c55e57305052b2670f0edf4f1e9bda6041cb0c80).

### uPD9002 ROR4 retained the old AL high nibble

- **Status:** fixed in the M62 implementation; G62 candidate review pending.
- **Symptom:** 4,692 of the 5,000 applicable `0F 2A ROR4` SST records failed.
- **Affected scope:** V30 packed-BCD `ROR4` register and memory operands only.
- **Demonstrated root cause:** the handler merged the source low nibble into
  the old AL high nibble instead of transferring the complete source byte to
  AL. The destination calculation was otherwise structurally correct.
- **Correction:** write the rotated destination first, preserving the AL
  alias case, then transfer the original complete source byte to AL.
- **Verification:** all 5,000 register and memory ROR4 records pass, including
  displacement, prefix, 16-bit offset-wrap, and physical-wrap partitions.
- **Evidence:** [M62 task](../agents/tasks/M62_upd9002_semantics_bundle.md)
  and [M62 report](../agents/reports/m62_upd9002_semantics_bundle.md).
- **Commit:** [e74d814f](https://github.com/nakatamaho/vaeg/commit/e74d814f4397a5d832e7fbef675a93df4160bb2f).

### uPD9002 decimal and ASCII adjust used inherited 286 behavior

- **Status:** fixed in the M62 implementation; G62 candidate review pending.
- **Symptom:** the complete G61 populations contained 34 DAA, 64 DAS, 124
  AAA, and 4,716 AAS architectural failures.
- **Affected scope:** `27 DAA`, `2F DAS`, `37 AAA`, and `3F AAS` only.
  `D4 AAM` and `D5 AAD` are independently governed.
- **Demonstrated root cause:** the inherited handlers did not implement the
  SST-observed V30 high-adjust branch and FLAGS rules. AAA/AAS additionally
  adjusted AX as a word, allowing a carry or borrow across AL/AH where the
  observed behavior adjusts the two bytes independently.
- **Correction:** add V30-specific low/high adjustment decisions, result
  materialization, AF/CF/SZP/OF rules, and byte-local AAA/AAS AH changes.
- **Verification:** each of DAA, DAS, AAA, and AAS is 5,000 pass / 0 fail;
  their exact pre-fix union contains 4,938 hashes.
- **Evidence:** [M62 task](../agents/tasks/M62_upd9002_semantics_bundle.md)
  and [M62 report](../agents/reports/m62_upd9002_semantics_bundle.md).
- **Commits:** [33bec007](https://github.com/nakatamaho/vaeg/commit/33bec0078328fdaf6612188b6341c6e938f6dcb6)
  and [bfd9710b](https://github.com/nakatamaho/vaeg/commit/bfd9710bdac52ec5092871a2f5595a34212df1f2).

### uPD9002 shifts used the inherited normalized-count paths

- **Status:** fixed in the M62 implementation; G62 candidate review pending.
- **Symptom:** 19,139 applicable shift records failed across the C0, C1, D2,
  and D3 `/4` through `/7` subforms.
- **Affected scope:** 8-bit and 16-bit SHL/SAL/SHR/SAR register and memory
  forms. Rotate subforms `/0` through `/3` are unchanged.
- **Demonstrated root cause:** the inherited group paths normalized counts
  and materialized destination and FLAGS with rules that differ from the
  complete V30 SST population, especially at zero, width, and beyond-width
  counts. Subform `/6` also required the evidence-proven SHL behavior.
- **Correction:** route only shift subforms through width-specific raw-count
  helpers with exact destination, CF, OF, AF, SZP, and count-zero
  preservation rules; retain the existing rotate paths.
- **Verification:** all 40,000 shift hashes pass, and all 40,000 protected
  rotate hashes remain architecturally green.
- **Evidence:** [M62 task](../agents/tasks/M62_upd9002_semantics_bundle.md)
  and [M62 report](../agents/reports/m62_upd9002_semantics_bundle.md).
- **Commit:** [2cdaed95](https://github.com/nakatamaho/vaeg/commit/2cdaed95072d74bbf7187ae854fb31d3886c995d).

### uPD9002 DIV and IDIV used inherited result and exception rules

- **Status:** fixed in the M64 implementation; G64 candidate review pending.
- **Symptom:** 12,486 applicable records failed across `F6 /6`, `F6 /7`,
  `F7 /6`, and `F7 /7`.
- **Affected scope:** byte and word unsigned and signed division arithmetic,
  result placement, divide-error decisions, and pre-event FLAGS only.
- **Demonstrated root cause:** the inherited paths used result, overflow, and
  FLAGS rules that differ from the complete V20 SST contract. The word signed
  path also exposed the host-language minimum-signed-value divided by `-1`
  hazard.
- **Correction:** use widened arithmetic with explicit zero and overflow
  checks, materialize the observed quotient/remainder and pre-event FLAGS,
  and retain the G60d-approved type-0 entry machinery unchanged.
- **Verification:** all four 5,000-case populations pass; the exact 214-case
  saved-FLAGS dependency set remains green.
- **Evidence:** [M64 task](../agents/tasks/M64_upd9002_div_idiv.md) and
  [M64 report](../agents/reports/m64_upd9002_div_idiv.md).
- **Commit:** [63f12b4e](https://github.com/nakatamaho/vaeg/commit/63f12b4e2bc38999efec66a43042673111e242fe).

### uPD9002 packed-BCD string operations used incomplete inherited behavior

- **Status:** fixed in the M64 implementation; G64 candidate review pending.
- **Symptom:** 91 `ADD4S` and 304 `SUB4S` architectural records failed, while
  `CMP4S` was monitor-authorized but remained an implementation gap.
- **Affected scope:** `0F20 ADD4S`, `0F22 SUB4S`, and `0F26 CMP4S`.
- **Demonstrated root cause:** the inherited ADD4S/SUB4S paths used
  incompatible decimal-adjust and address-wrap behavior, and no CMP4S
  handler existed.
- **Correction:** apply the independently observed packed-decimal carry,
  borrow, comparison, register-update, and logical/physical wrapping rules;
  implement CMP4S without a destination write.
- **Verification:** all three 1,000-case full populations pass; ROL4 and ROR4
  remain 5,000 pass / 0 fail.
- **Evidence:** [M64 task](../agents/tasks/M64_upd9002_div_idiv.md) and
  [M64 report](../agents/reports/m64_upd9002_div_idiv.md).
- **Commit:** [60385167](https://github.com/nakatamaho/vaeg/commit/60385167cede30a3c06e97373a92646e19021523).

### uPD9002 monitor-authorized bit-operation forms were incomplete

- **Status:** fixed in the M64 implementation; G64 candidate review pending.
- **Symptom:** six exact monitor-authorized forms remained classified as
  implementation gaps even though their byte/word and CL/immediate sibling
  forms were present.
- **Affected scope:** the expanded `TEST1`, `CLR1`, `SET1`, and `NOT1`
  opcodes `0F10` through `0F1F`.
- **Demonstrated root cause:** dispatch and handlers were missing for
  `0F13`, `0F15`, `0F16`, `0F17`, `0F1E`, and `0F1F`.
- **Correction:** add only the evidence-derived word/CL and NOT1 forms,
  including exact bit-index, register/memory, FLAGS, and instruction-length
  behavior, then activate their complete pre-approved structural sets.
- **Verification:** every one of the sixteen expanded 5,000-case populations
  passes. `0FFF BRKEM` is not counted: v20 has metadata but no SST shard, so
  selected and executed coverage is exactly zero.
- **Evidence:** [M64 task](../agents/tasks/M64_upd9002_div_idiv.md) and
  [M64 report](../agents/reports/m64_upd9002_div_idiv.md).
- **Commit:** [99c6388d](https://github.com/nakatamaho/vaeg/commit/99c6388df903dfc69432730cc9fa908a83946774).


### SCSI DATA IN ended without reporting an early phase change

- **Status:** corrected in M75d1; terminal G75 acceptance remains pending.
- **Symptom:** when a host allocation exceeded the target response, the AR19
  pump continued until the host count reached zero and never exposed the
  target's STATUS phase with the residual transfer count.
- **Demonstrated root cause:** DATA IN completion was tested only against the
  host-programmed remaining count; `cmdpos` exhaustion was not converted into
  the WD33C93 unexpected-information-phase status.
- **Correction:** [ca29efb](https://github.com/nakatamaho/vaeg/commit/ca29efb)
  preserves residual TC and emits `0x48 | (phase & 7)` (4Bh for STATUS) when
  response data ends first.  When TC ends first, the unrequested suffix is
  discarded and normal completion advances to STATUS.
- **Verification:** focused M75 QA, Linux SDL2 build, and
  `vaeg_m75_scsi_controller` CTest pass.  Normal-speed guest evidence for
  the short INQUIRY path is still outstanding.
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md) and
  [M75 task](../agents/tasks/M75_scsi_support.md).

### Transfer Info completion encoded the wrong phase and consumed post-count requests

- **Status:** corrected in M75d1; G75 manual acceptance remains pending.
- **Symptom:** TUR completion was logged as `1Ah` while the next request was STATUS (`8Bh`), and the following STATUS/MESSAGE-IN request could be consumed or regenerated instead of remaining available for the next Transfer Info.  Message-In completion and Negate ACK also produced the wrong lifecycle.
- **Demonstrated root cause:** successful completion used `0x10 | MCI` instead of the WD33C93A `1MCI` encoding; REQ and ACK were coupled; the post-count REQ was not an independent retained bus state; Message-In was treated like a normal post-count phase; and command admission rejected Level-I Negate ACK solely because a Level-II state was active.
- **Correction:** derive completion from the next service status (`0x18 | (service_status & 7)`), retain the post-count REQ with its sequence/phase/direction across CSR read, split target REQ and initiator ACK transitions, return `20h` after the Message-In byte while holding ACK, and make Negate ACK clear ACK without directly generating bus-free status.  Transfer-count writes are traced and SBT semantics are explicit.
- **Verification:** compiled controller-path selftest (21 named cases), focused CTest, controller QA, Linux SDL2 selftest/build, MinGW cross-build, and semantic-limit real-ROM trace.  The real trace now shows `1Bh` for TUR STATUS, retained request IDs across `1Fh`, `20h`, and later `85h`, INQUIRY DATA IN with 32 reads and `4Bh` residual completion, and no unexplained `010000h`.
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md) and [M75 task](../agents/tasks/M75_scsi_support.md).
- **Commit:** [4e17c6f](https://github.com/nakatamaho/vaeg/commit/4e17c6f3fee67642ca69329147808cd18c71c9a7).

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
- **Next step:** M74 is reserved to capture a bounded post-command
  CPU/register/I/O trace, compare the decisive VA1 and VA2/VA3 control flow,
  and correct the defect if the root cause is proven within the milestone
  scope.
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


### PCPLUS bus-free completion was left pending after MESSAGE IN

- **Status:** corrected in M75d1; full G75 acceptance remains pending.
- **Symptom:** after PCPLUS consumed `CSR=1Fh` and the MESSAGE IN byte, the
  controller did not deliver the configured ending-disconnect `CSR=85h`.
  The guest could continue without a visible bus-free completion, and the
  terminal controller sequence was incomplete.
- **Demonstrated root cause:** the target-phase helper assumed every phase
  transition would be followed by a host `TRANSFER INFO`.  Bus free has no
  such request, so the pending `85h` status remained not-ready and was never
  scheduled after the `1Fh` CSR was consumed.
- **Correction:** mark only `85h`/`80h` bus-free results target-ready at the
  MESSAGE IN boundary.  The existing depth-one CSR latch, AR17 consumption,
  DBR gating, and processing event for data-bearing transitions are unchanged.
- **Verification:** the normal-speed full trace now shows MESSAGE `00h`,
  `CSR=1Fh`, then IRQ6 `CSR=85h`, `AR17=85h`, and `AR16=00h`.  The focused
  M75 validator and Linux SDL2 build pass.
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md) and
  [M75 task](../agents/tasks/M75_scsi_support.md).
- **Commit:** [bc29d9e7](https://github.com/nakatamaho/vaeg/commit/bc29d9e7cecc426c4da22cbc628ab95f8c7efe8f).

### INQUIRY response payload has not been guest-validated

- **Status:** open; no payload-field change is authorized from static
  inspection alone.
- **Symptom:** the current direct-access response table is 32 bytes while the
  observed INQUIRY allocation is 36 bytes.  This requires the corrected short
  transfer contract, not forced padding.
- **Demonstrated state:** byte4 is now `1Bh`, equal to the 32-byte table length
  minus five, enforced by [4eeacda](https://github.com/nakatamaho/vaeg/commit/4eeacda).
- **Next step:** observe the normal-speed guest branch after the short `4Bh`
  response, then decide whether ANSI level, vendor/product text, or response
  length needs a separately evidenced change.
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md) and
  [M75 task](../agents/tasks/M75_scsi_support.md).

### CPU-multiplier SCSI timing mismatch

- **Status:** reopened in M75d1; the prior `f406b86` closure is superseded.
- **Symptom:** the normal-speed run (the intended CPU/device ratio) reaches
  TUR completion but does not reach the later MODE SENSE DATA IN within the
  180-second bounded run, while the diagnostic `--cpumult 8` run reaches
  INQUIRY, READ CAPACITY, and MODE SENSE but exhibits partial DATA IN.
- **Demonstrated state:** the source audit proves that phase readiness and
  interrupt delivery use emulated-clock events and that AR19 byte access is
  synchronous.  It does not prove that the chosen target-processing quantum
  is long enough for the normal-speed guest to return to its intended wait
  consumer.  Progress under an artificial multiplier therefore cannot be
  classified as expected behavior.
- **Correction:** none yet.  Compare the normal-speed and `--cpumult 8`
  consumer paths for the second SELECT/COMMAND event (`1CCDh` wait path
  versus `1742h`/`1747h` main-pump path), then derive any processing delay from
  the emulated device contract rather than tuning to PCPLUS.
- **Verification:** controller QA, builds, CTest, and selftest pass, but the
  timing defect and normal-speed INQUIRY/MODE SENSE evidence remain open.
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md),
  [0c80c447](https://github.com/nakatamaho/vaeg/commit/0c80c447b6b655b81b3d08e5b67c8a1457d5be91),
  and [df2981e](https://github.com/nakatamaho/vaeg/commit/df2981e).


### MODE SENSE page-04 geometry was omitted from the SCSI target

- **Status:** corrected in M75d1; normal-speed SCHD registration remains open.
- **Symptom:** PCPLUS/SCHD issued `1A 00 04 00 24 00` but the target ignored
  the page code and returned only a 12-byte mode header and block descriptor,
  omitting the rigid-disk cylinder/head geometry.
- **Demonstrated root cause:** `scsicmd_datain()` treated every MODE SENSE(6)
  request as the descriptor-only case and capped the response at 12 bytes.
- **Correction:** decode page `04h` and `3Fh`, emit the allocation-bounded
  page-04 geometry with mounted `SXSIDEV` values, validate the geometry
  product, and return CHECK CONDITION/ILLEGAL REQUEST for unsupported pages or
  inconsistent geometry.  REQUEST SENSE exposes and then clears the sense
  data.
- **Verification:** M75 controller QA, Linux SDL2 build, focused CTest, and
  SDL selftest pass; the accelerated real-ROM trace reaches the MODE SENSE
  DATA IN phase.  Normal-speed SCHD registration is not yet demonstrated.
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md) and
  [M75 task](../agents/tasks/M75_scsi_support.md).
- **Commit:** [03d4cd765](https://github.com/nakatamaho/vaeg/commit/03d4cd76541a3058cf32b0c239b499e0c0431627).

### WD33C93 transfer count byte-order hypothesis (superseded)

- **Status:** superseded in M75d1; no low/middle/high production correction is retained.
- **Observed symptom:** the intermediate MinGW run with the low/middle/high experiment produced `TC=060000` for a six-byte CDB, `TC=010000` for a one-byte transfer, and `TC=0a0000` for a ten-byte CDB.
- **Demonstrated correction:** the WSLg `scsitrace` proves PCPLUS/WD33C93 uses AR12/AR13/AR14 as high, middle, low: the expected counts are 6, 1, and 10, while the experiment multiplied them by 65536. The original high/middle/low decode and decrement order is restored.
- **Follow-up commit:** [c959453](https://github.com/nakatamaho/vaeg/commit/c959453a0a482994ac25ab6db0b33e425306a0e9).
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md) and [M75 task](../agents/tasks/M75_scsi_support.md).
### MODE SENSE block length was written at the reserved-byte offset

- **Status:** corrected in M75d1; SCHD/SCFORM manual confirmation remains pending.
- **Symptom:** after INQUIRY completed and SCHD reported direct-access fixed-media mode, the driver halted during MODE SENSE geometry processing.
- **Demonstrated root cause:** the six-byte MODE SENSE block descriptor places the three-byte block length at response bytes 9--11.  `scsicmd_datain()` wrote it at byte 8, overwriting the reserved byte and shifting the value read by SCHD.
- **Correction:** write the mounted `SXSIDEV` block size at `scsiio.data + 9`; no device-specific or guest-address workaround was added.
- **Verification:** M75 QA, Linux SDL2 build, focused CTest, and SDL selftest pass.  The corrected MinGW/manual SCFORM run is still required to confirm SCHD registration and format completion.
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md) and [M75 task](../agents/tasks/M75_scsi_support.md).


### CSR pending queue admitted target events ahead of the host consumer

- **Status:** corrected in M75d1; later normal-speed guest progress remains open.
- **Symptom:** a target transfer-completion CSR could be requested while the
  previous SELECT/command CSR was still active, filling the runtime pending
  slot and later causing overrun/drop and wrong event ordering.
- **Demonstrated root cause:** the old implementation let target state advance
  on scheduled time while the WD33C93 CSR latch was unread, so the runtime
  state contained an active event, a visible latch, and a successor pending
  event.  The pre-correction WSLg trace records the active `CSR=11h`, a
  concurrent `CSR=1Ah`, `pending=1`, and subsequent `csr-overrun` records.
- **Correction:** remove the CSR pending slot.  Target-origin events are
  pulled from persistent target state only after AR17 consumes the visible
  CSR; host-synchronous transfer-completion events remain serialized by the
  host I/O boundary.  Overlap is fail-closed and trace-visible.  Add a
  deterministic watchdog for unread CSR, stalled target delay, and missing
  DATA IN request handoff.
- **Verification:** fixed 4000/40000-clock stress and five seeded jitter runs
  produced zero `csr-overrun`, `csr-drop`, and `invariant` records.  QA,
  focused CTest, and SDL selftest pass.  A normal-speed bounded run still
  reports an unread later `CSR=8Ah`, so SCHD registration and G75 acceptance
  remain open.
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md) and
  [M75 task](../agents/tasks/M75_scsi_support.md).
- **Commit:** [ccb0666](https://github.com/nakatamaho/vaeg/commit/ccb066695907456314783cc3bb9a28dfad279c55).

### DATA IN TC=1 was incorrectly treated as a phase end

- **Status:** corrected in M75d1; fresh SCFORM/SCHD acceptance remains pending.
- **Symptom:** the WSLg SCFORM/SCHD trace completed TUR and selected ID0, but an INQUIRY DATA IN request programmed `TC=1` after an initial `TC=36`.  The emulator moved to STATUS after the first byte, so SCHD saw an incomplete INQUIRY response and reported that no device was connected.
- **Demonstrated root cause:** `scsiio_data_read()` changed `SCSIPH_DATAIN` to STATUS whenever the host count reached zero, even while `rddatpos < cmdpos`.  PCPLUS uses repeated one-byte TRANSFER INFO requests within the same DATA IN phase.
- **Correction:** keep DATA IN active and call the phase-aware transfer path for the next byte; transition to STATUS only when the target response cursor is exhausted.  No guest address, CDB-order, image-name, or fixed-length workaround was added.
- **Verification:** M75 controller QA, focused CTest, SDL selftest, and MinGW cross-build pass.  The corrected binary requires a new manual SCFORM/SCHD run.
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md), [M75 task](../agents/tasks/M75_scsi_support.md), and [84bc2ef](https://github.com/nakatamaho/vaeg/commit/84bc2efe1de9e5661fd28d31ba087a304f1a82ac).

### Transfer Info was not represented as an active Level-II lifecycle

- **Status:** corrected in M75d1; PCPLUS/SCHD integration remains open.
- **Symptom:** after `AR18=20h`, the emulator could treat a REQ-waiting
  Transfer Info as an abandoned legacy phase transfer, causing the guest to
  fall back to single-byte recovery and fail to complete device discovery.
- **Demonstrated root cause:** the Transfer Info path had no single explicit
  lifecycle distinguishing command acceptance, REQ wait, byte pending, and
  post-count REQ.  Service Required could therefore be considered while the
  command was still active, and completion could be emitted without a
  distinct post-count REQ.
- **Correction:** add the explicit lifecycle in `cbus/scsiio.c`; reject
  commands while INT/CSR is pending, preserve active state while waiting for
  REQ, enforce REQ/DBR/ACK byte accounting, emit `4MCI` for early phase
  changes, and emit successful MCI only after the post-count REQ.  An already
  asserted REQ starts transfer immediately.  No guest-address or payload
  workaround was added.
- **Verification:** M75 Transfer Info selftest (10 tests), controller QA,
  focused CTest, Linux SDL2 build, and SDL selftest pass.  The bounded real-ROM
  run still times out before INQUIRY DATA IN, so manual SCHD/SCFORM acceptance
  remains open.
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md),
  [M75 task](../agents/tasks/M75_scsi_support.md),
  [f0b14d7](https://github.com/nakatamaho/vaeg/commit/f0b14d71a2015b9469c92ea51abe2b9ebf964b43),
  [9827d09](https://github.com/nakatamaho/vaeg/commit/9827d09756779943d46b0973436f26f32142dced).


### SCSI target LUN isolation and malformed INQUIRY response

- **Status:** corrected in M75d1; SCHD/SCFORM registration remains open.
- **Symptom:** one configured SCSI image could be interpreted through multiple
  logical-unit discovery paths, and the 32-byte INQUIRY response left SCHD
  reading stale bytes for Product Revision.
- **Demonstrated root cause:** the backend did not centrally require both the
  WD Target LUN register and CDB LUN to be zero, and `hdd_inquiry` advertised
  `1Bh` additional length in a 32-byte table with revision bytes in the wrong
  fixed-width offsets.
- **Correction:** centralize LUN0 validation for PIO and Select-and-Transfer;
  return GOOD/36-byte byte-`7Fh` INQUIRY for unsupported LUNs and
  CHECK CONDITION `05/25/00` for other unsupported-LUN commands; replace the
  normal response with the exact 36-byte `NEC`/`NP2-HDD`/`1.00` layout.
- **Verification:** compiled production selftests, M75 QA, focused CTest,
  Linux SDL2, MinGW cross-build, and bounded target/LUN trace pass.  SCHD
  still issues unsupported READ(10), so SCFORM and manual registration are
  not claimed.
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md) and
  [M75 task](../agents/tasks/M75_scsi_support.md).
- **Commit:** [103d59e](https://github.com/nakatamaho/vaeg/commit/103d59e).


### SCSI block commands were missing from the target backend

- **Status:** corrected in M75d2; guest enumeration and persistent filesystem gates remain open.
- **Symptom:** SCHD reached READ(10) during discovery, received CHECK CONDITION `05/20/00`, and later reported that the registered C: drive had no sectors.  The earlier multi-device report is not classified because it lacked a target/LUN/registration matrix.
- **Demonstrated root cause:** `cbus/scsicmd.c` had no common READ/WRITE(6/10) command path, so no SXSIDEV media operation occurred for block access.
- **Correction:** add shared 6-/10-byte LBA/count decoding, overflow-safe range validation, SXSIDEV-backed chunked PIO DATA IN/OUT, complete-write commit accounting, and fixed sense mappings for range, write-protect, and backend errors.  The implementation reuses `sxsi_read()`/`sxsi_write()` and does not special-case SCHD, SCFORM, target IDs, or guest addresses.
- **Verification:** compiled production selftests cover zero-count semantics, 6-/10-byte decoding, range and sense handling, read/write persistence, incomplete writes, chunk boundaries, and LUN isolation.  Normal-speed real-ROM trace now shows READ(10) LBA 0 with 256 DATA IN bytes, one backend block, zero residual, and GOOD status, followed by STATUS/MESSAGE IN.
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md), [M75 task](../agents/tasks/M75_scsi_support.md), and [a4d21e9](https://github.com/nakatamaho/vaeg/commit/a4d21e9a5e0a3b31818cc1dfcd8b281b3b62a67d).
- **Commit:** [a4d21e9](https://github.com/nakatamaho/vaeg/commit/a4d21e9a5e0a3b31818cc1dfcd8b281b3b62a67d).


### Legacy SCSI DATA OUT bypassed the backend commit and erased CHECK CONDITION

- **Status:** corrected in M75; fresh SCFORM/FAT persistence remains open.
- **Symptom:** a legacy SCSI DATA OUT request could fill the staging buffer,
  switch directly to STATUS, and report completion without proving that the
  mounted image had been updated.  A backend write failure could also be
  replaced by GOOD during STATUS transfer.
- **Demonstrated root cause:** `cbus/scsiio.c`'s 0CC6h handler wrote the
  buffer directly and emitted `8Bh` without calling the common
  `scsicmd_transinfo()`/`scsicmd_block_dataout_complete()` lifecycle.  The
  STATUS branch in `cbus/scsicmd.c` unconditionally assigned `00h`.
- **Correction:** AR19 and 0CC6h now share DATA OUT payload accounting;
  complete chunks call the common command layer, which alone invokes
  `sxsi_write()`, advances chunks, and selects status.  STATUS preserves the
  command-layer result.  No SCFORM, FAT, target-ID, or guest-address special
  case was added.
- **Verification:** SDL production selftest covers successful 0CC6h commit,
  no direct STATUS completion, failed backend write, and CHECK CONDITION
  persistence; Linux build, SDL selftest, focused CTest, and M75 QA pass.
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md) and
  [d284468](https://github.com/nakatamaho/vaeg/commit/d284468fd256598489e07307fda58fbd1a0aa302).


### Added VHD/FAT16 forensic inspection for the G75 free-space failure

- **Status:** diagnostic tooling added; guest FAT acceptance remains open.
- **Symptom:** freshly formatted guest volumes report no free disk space, but
  the first incorrect on-disk byte was not available in the current sandbox.
- **Correction:** add `tools/inspect_vaeg_fat.py`, which matches the VAEG
  VHD header, respects physical/header offsets, assembles four-block logical
  sectors, validates FAT16 BPB geometry, compares FAT copies, counts free
  clusters, inspects root-directory entries, and reports changed physical-LBA
  ranges.  Generated fixtures reject duplicated/reordered four-block metadata
  and zero-free or mismatched FAT tables.
- **Verification:** the initial seven generated unittest cases and focused CTest
  pass.  At that time the original user images were inaccessible; the later
  read-only copies and their truncation evidence are recorded in the
  superseding forensic entry below.
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md).


### Supplied SCFORM images were truncated before FAT validation

- **Status:** evidence limitation recorded in M75; G75 remains open.
- **Symptom:** the supplied `scsi40_formatted.hdd` was expected to explain the
  guest's zero-free-space report, but no valid FAT16 BPB could be decoded.
- **Demonstrated finding:** both supplied VHD1.00 headers report a 40 MiB
  image with 163,840 256-byte blocks, while the actual files contain only 3
  and 651 complete data blocks (plus 220-byte tails).  The formatted artifact
  has early IPL/formatter data and partial writes, not a complete filesystem
  image.
- **Correction:** the inspector now reports file size, complete block count,
  truncation, and changed partial tails; it never fabricates missing blocks.
  Original files were opened read-only.
- **Verification:** nine generated inspector tests pass.  No FAT free-cluster
  count or post-fix SCFORM result is claimed from these truncated artifacts.
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md).

### SCSI VHD creator left declared capacity truncated

- **Status:** corrected in M75; guest SCFORM and filesystem acceptance remain open.
- **Symptom:** VHD1.00 headers declared 163840 256-byte blocks, but newly created backing files ended after the header/initial IPL and SXSIDEV could read past the actual EOF.
- **Demonstrated root cause:** the image creator wrote the header and boot bytes without setting the logical file length; the production `VHDHDR` is 220 bytes, so validation must use `sizeof(VHDHDR)` rather than an inferred 256-byte header.
- **Correction:** `newdisk_vhd_create()` performs checked geometry arithmetic, writes a complete 220-byte-header image through a temporary path, sets the exact sparse logical length, flushes and atomically renames it.  SCSI open/read/write paths now reject incomplete or overlong stores and propagate short I/O/flush failures.
- **Verification:** production selftests cover exact size, sparse zero reads, first/middle/last block boundaries, persistence after reopen, out-of-range rejection, truncated-image rejection, and no-overwrite behavior.  The generated 40 MiB image is recorded in the M75 report.
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md).


### FAT padding was counted as usable free space and 64KiB PIO positions wrapped

- **Status:** corrected in M75; guest CHKDSK and file lifecycle remain open.
- **Symptom:** FAT inspection reported free entries beyond the valid data-cluster
  range, while exact 64KiB READ(6) transfers could address a repeated data
  window at the 64KiB boundary.
- **Demonstrated root cause:** the inspector counted the physical FAT entry
  capacity rather than `FAT[2]..FAT[ClusterCount+1]`; AR19 and 0CC6h DATA IN
  indexing masked the 32-bit data position with 16-bit and 15-bit masks.
- **Correction:** derive valid cluster and padding ranges from the BPB, and use
  checked unmasked positions for both PIO data paths.  Transfer traces now
  compare backend, staging and delivered bytes, and record TC `010000h` as a
  valid 65,536-byte count.
- **Verification:** compiled SDL selftests cover 65,535/65,536/65,537-byte
  boundaries, READ(6) 256-block semantics and READ(10) exact 64KiB/chunked
  reads; ten FAT-inspector tests pass; the bounded formatted-image run shows
  equal backend/staging/AR19 digests for its first three block reads.
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md) and
  [a7d244d](https://github.com/nakatamaho/vaeg/commit/a7d244d61d93eedaf8498185ec55f8e8ac743926).


### Direct Select-and-Transfer WRITE committed before guest DATA OUT

- **Status:** fixed; G75 remains open for the independent SASI, HOSTFAT, and
  non-SCSI regression gates.
- **Symptom:** PC-Engine issued a successful WRITE(10), but the guest's
  subsequent AR19 DATA OUT bytes arrived after the controller had already
  committed the stale staging buffer. The first write could therefore update
  the image with the preceding command's data and reject the actual DATA OUT
  phase.
- **Root cause:** the direct Select-and-Transfer WRITE path called the
  backend write at command acceptance, before the guest drained the DATA OUT
  window. This was demonstrated by the first WRITE trace: 1024 bytes were
  reported as backend-written before the first guest DATA OUT byte, followed
  by phase-direction-mismatch warnings.
- **Correction:** direct WRITE commands now remain active in DATA OUT, accept
  bytes through AR19, and call the backend write only after the programmed
  byte count is complete. The trace digest equality predicate covers both READ
  and WRITE data paths.
- **Verification:** the isolated guest creation run completed a WRITE(10) at
  LBA 572 for four 256-byte blocks with TC `000400`, AR15h `00h`, AR19 DATA
  OUT, GOOD status, residual zero, commit_count one, and equal backend,
  staging, and delivered digests. The guest read/reopen/delete run printed
  `G75 READ-REOPEN-DELETE OK`; a second boot printed `G75 DELETE PERSISTED`.
  The focused Python tests, `M75_SCSI_CONTROLLER_OK`, Linux build, and
  `git diff --check` passed.
- **Evidence:** `docs/agents/reports/m75_scsi_support.md`, G75b screen and
  trace artifacts retained outside the repository.
- **Commit:** [13c978b](https://github.com/nakatamaho/vaeg/commit/13c978b)

### HOSTFAT configuration could make startup unrecoverable

- **Status:** fixed in M75; HOSTFAT remains read-only and its guest
  filesystem contract is unchanged.
- **Symptom:** changing `HOSTFATDIR` and closing vaeg before the asynchronous
  rebuild/reset completed could leave the configuration pointing at a new
  path while the old snapshot remained mounted. A later startup could then
  fail before the emulator window appeared; deleting the configuration was
  required to recover.
- **Root cause:** the GUI persisted `HOSTFAT`/`HOSTFATDIR` before the worker
  had built and mounted the replacement image, and startup treated a failed
  configured HOSTFAT rebuild as fatal.
- **Correction:** retain pending GUI values until successful mount, commit the
  path only at the mount event, and disable invalid configured HOSTFAT on
  startup while retaining the path so the emulator can boot and the setting
  can be corrected later.
- **Verification:** Linux SDL selftest, HOSTFAT manager failed-rebuild
  retention selftest, invalid configured-directory startup recovery probe,
  `M75_SCSI_CONTROLLER_OK`, all required local builds, and `git diff --check`
  passed.
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md).
- **Commit:** [bc51051](https://github.com/nakatamaho/vaeg/commit/bc510511326b9fdb3f61018d751dfc598159512a)


### HOSTFAT rejected a Windows Dropbox root path

- **Status:** fixed in M75; links that escape the selected root, special files,
  and containment checks remain rejected.
- **Symptom:** a Windows HOSTFAT directory selected under a Dropbox tree could
  be treated as unavailable when the selected root was exposed as a junction or
  directory reparse point. Paths copied with surrounding quotes were also
  passed to filesystem validation literally.
- **Root cause:** the GUI and snapshot builder did not normalize user-entered
  HOSTFAT paths, and the builder rejected a Windows root reparse point before
  canonicalizing it.
- **Correction:** normalize whitespace/quotes, use `USERPROFILE` for the
  Windows browser start directory, canonicalize the selected Windows root and
  contained links/reparse points, and reject links that escape the snapshot
  tree.
- **Verification:** HOSTFAT snapshot selftest now covers a quoted path and,
  on Windows when permitted, both a root reparse point and a contained
  reparse-point directory. Linux debug and MinGW cross builds pass.
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md).
- **Commit:** [1ec024b](https://github.com/nakatamaho/vaeg/commit/1ec024b)
- **Follow-up:** [7e6ede7](https://github.com/nakatamaho/vaeg/commit/7e6ede7)


### HOSTFAT GUI hid rebuild failures and overstated capacity

- **Status:** fixed in M75.
- **Symptom:** Configure displayed `127.44 MiB usable` although the current
  PC-Engine FAT12 geometry provides about 63.72 MiB, and asynchronous rebuild
  errors were not visually distinguished from normal status text.
- **Root cause:** the GUI retained a stale capacity label and rendered the
  manager error message with the normal text style.
- **Correction:** display the actual 63.72 MiB limit, render
  `HOSTFAT_MANAGER_ERROR` messages in red with their detailed reason, and
  reopen Configure automatically after an asynchronous failure.
- **Verification:** Linux selftest, Linux/macOS/MinGW builds, M75 QA, and
  repository encoding/EOL/case checks passed.
- **Evidence:** [M75 report](../agents/reports/m75_scsi_support.md).
- **Commit:** [55800c6](https://github.com/nakatamaho/vaeg/commit/55800c6)
- **Follow-up:** [2515598](https://github.com/nakatamaho/vaeg/commit/2515598)
