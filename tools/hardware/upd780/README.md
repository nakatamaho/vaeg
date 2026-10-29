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
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->

# M140 physical-FDC diagnostic preparation

The maintainer separated this work from M139 into M140. Existing v1 `M139`
banners, stable error codes and JSON schema IDs are retained as legacy protocol
identifiers so tested binaries/evidence are not changed merely to rename a task.
ZEX adaptations are versioned in [external patches](../../../external/zex/patches/README.md),
not hidden string substitutions or hand-edited upstream source.

**Implemented hardware-facing loader and RAM programs; full physical acceptance
remains BLOCKED.** Returned physical VA logs contain all 67 ZEX groups (matching
identical-image mapped host CRCs), 256 AF pairs and 67 timing rows. SCF/CCF X/Y
differences need history controls; absolute timing and physical H/S remain open.
See [observations](../../../docs/agents/reports/m140_fdc_va_observations.md).
`results.disk_files()` accepts validated 2D double-step capture tables using
an in-memory header-only normalization; the media builder remains strict. Main-CPU CP/M is only the
controller. Running the original disk's `ZEXALL.COM` still tests its **main
CPU**, not the FDC. Use the new names below.

The supported interface is the VA/VA2 FDC monitor family described in
[the canonical monitor analysis](../../../docs/bios/fdc_monitor.md). Do not use
this destructive temporary RAM takeover on an unverified firmware revision.
No ROM is replaced, no drive-write command occurs during takeover, and the
private input disk is never modified by the builder. This is not an installer
for arbitrary CP/M layouts, FDC boards or PC-8801 subsystem ROMs.

## Build a private working disk

Requirements: Python 3, a C compiler for the pinned z80asm 1.8 source build
(or an explicitly supplied z80asm 1.8), and the supported 320-KiB CP/M input
D88. Its geometry must be 80 tracks × 16 sectors × 256 bytes, with the
EXM=1, 2-KiB-block, 128-entry directory at raw offset 4000h.

```sh
export TMPDIR="$PWD/../tmp"
python3 tools/hardware/upd780/build.py \
  --source-disk "$INPUT" --output-dir "$OUT" --cache-dir "$ARCHIVE_CACHE"
# Optional: --assembler /absolute/path/to/z80asm
```

Keep `$OUT` under task scratch or hardware handoff, **outside the private input
and archive-cache trees**. The cache receives downloaded locked archives only;
assembler compilation/executables are placed below `$OUT`. An existing output
D88 is refused. The tool preserves source sectors/metadata except newly
allocated directory entries and free data blocks, validates allocation and
round-trips the image. Original file names are not overwritten. Existing files
in the second half of the directory and multi-extent files are supported.

Outputs beside the private D88 include source ASM, exact subsystem BINs,
controller COMs, controller symbol JSON and `build-report.json` with source,
program, payload, input and output digests. Keep these identities with records;
do not upload the private image, input identity, logs or captures to Git/CI.
The disk also gets `FDCHELP.TXT` and `ZEXLIC.TXT`. ZEX source notices are
preserved; this build selects the GPLv3 option of its GPL-2.0-or-later grant,
using the pinned repository's verbatim license text (DOS text line endings).
Independent controller/tool code is BSD-2-Clause.

## Run on the supported machine

Boot a **writable copy**, with its normal console and otherwise idle disk
service. Keep it inserted. Run one program at a time:

| Command | Purpose | Result file |
| --- | --- | --- |
| `FDCONE` | First smoke test: NEG group only, not full coverage | `FDCONE.TXT` |
| `FDCPRB` | AF matrix and separate speed kernels | `FDCPRB.TXT` |
| `FDCZEX` | All 67 relocated ZEXALL groups | `FDCZEX.TXT` |

Smoke may take about a minute; speed probes take several minutes and full ZEX
can take **hours**, depending on the actual board. These are planning bounds,
not measured physical speed claims. Computation need not print continuously.
A long-computation handshake selects a larger but finite receive budget.

Wait for `M139_FDC_RAM_RESTORED` and the CP/M prompt, then `DIR`/`TYPE` normally.
The controller backs up 4000h–7EFFh to main RAM, uploads the payload and zeros
the remaining diagnostic RAM to match the host initial state, uses bounded subsystem RAM
and SP=7EFFh, receives results into a 12-KiB main-RAM buffer, restores the
saved range and verifies every byte. **Only then** does it create its log.
It owns a separate, image-backed 512-byte main stack, restores the original
main SP and default DMA=0080h on ordinary return. It does not promise to
restore all FDC CPU registers or every live 7Fxx firmware variable.

An existing output filename is refused before takeover. To retain a log for
another run, e.g. `REN PRBS.TXT=FDCPRB.TXT`, or use another working copy.
Leave enough free space for logs; a disk-full failure after verified recovery
closes any created partial file and restores DMA but does not erase it.
For `M139_FDC_TIMEOUT`, `M139_FDC_RECOVERY_FAILED`, `M139_TIMER_WRAP` or
`M139_LOG_CAPACITY`, **reset the machine**. The controller deliberately halts
rather than issue unsafe disk I/O in an unknown state. Ctrl-C is not a safe
asynchronous FDC abort. Never remove the disk or start concurrent disk work
while the subsystem program is active.

### AF observations

There are 16 A values × 16 requested F values. Each `F` row has six
concatenated four-hex-digit AF words: requested, POP-AF readback, `BIT 0,A`,
`AND 55h`, `SCF`, `CCF`. Each instruction receives a fresh input AF. The
readback distinguishes requested flags from physically retained flags.
No undocumented bits are masked. Differences from the independent Zilog
formula are reported as observations, not automatically classified as bugs.

### Speed observations

There are 11 bodies: branch control, NOP, eight NOPs, register transfer,
RAM read, RAM write, immediate ALU, PUSH/POP pair, indexed BIT, SCF and CCF.
Each is repeated twice at N=1/4/16, each N unit being **60000 iterations**;
there is also one empty notification-path measurement: 67 timing rows total.
Loops contain no transfer, console, CRC or disk operations. The last inner
branch is untaken; this is not a complete taken/untaken instruction matrix.

`E=hhhh` counts **both VRTC transitions** observed at main port 40h bit 5.
It is not an FDC T-state, host time or MHz. Fixed protocol/observer latency
is outside the inner loop; endpoint slopes remove fixed intercepts, and the
branch body's slope removes shared loop/outer-loop control. Repetition and
middle-count residual checks reject inactive/nonlinear/noisy observations.
The two VRTC edges need not be uniformly spaced: a window has ±2-edge phase
error; interpolating N=4 between N=1/16 permits a four-edge residual, while
repeat slopes differ by at most 8/15 edges per N. These analytic bounds are
fixture-tested, not widened after an unexplained hardware mismatch.
NOP8 improves quantization; PUSH/POP is only a pair measurement, not separate
PUSH and POP costs. These are board/RAM execution measurements, not ROM-wait
or intrinsic-silicon-cycle measurements.

```sh
python3 tools/hardware/upd780/results.py "$FDCPRB_TXT" \
  --kind probe --output "$REPORT"
# ONLY after independent physical reference approval:
python3 tools/hardware/upd780/results.py "$FDCPRB_TXT" \
  --kind probe --output "$CALIBRATED_REPORT" \
  --vrtc-hz "$APPROVED_HZ" --reference-id "$APPROVED_REFERENCE_ID"
```

No nominal reference frequency is guessed. The optional conversion reports
baseline-subtracted seconds per body sequence and per-instruction average,
with a conservative two-edge-per-endpoint quantization allowance. Reference
frequency uncertainty is **additional**, explicitly not included. Machine,
firmware/board, main H/S setting, payload/build identity and reference identity
must accompany physical records. Repeat on main H and S to expose observer
coupling; do not infer FDC clock from the main CPU's displayed clock.

### Relocated ZEX observations

The original GPL ZEX source remains untouched in `external/zex/`. The
reproducible translation uses ORG=4000h, bounded RAM/stack, monitor output,
`JP 002Ah` completion and DI around the instruction-under-test wrapper.
It retains all 67 groups, flag masks, opcode vectors and increment/shift
vectors. The `LD r,n` descriptor's **base** HL changes 0339h → 7339h because
the original address would be FDC ROM. This is a declared porting change,
not an NEC flag fix. Other absolute/address-derived state changes with
relocation, so actual CRCs must not be compared blindly against stock CRCs.

Every row prints `STOCK=xxxxxxxx CRC=xxxxxxxx`, followed by `Tests complete`.
No failed group is hidden or converted into a fabricated stock PASS. Compare
physical records with the **identical mapped payload** on the host, then
minimize deltas with address/relocation controls and an independent Zilog
control before proposing per-instance NEC backend changes.

```sh
python3 tools/hardware/upd780/results.py "$FDCZEX_TXT" \
  --kind zex --output "$REPORT"
# --kind smoke is deliberately distinct and requires the NEG-only banner.
```

The result status is `COMPLETE_OBSERVATIONS`, not physical conformance PASS.
A complete host run is not an independent silicon oracle. Unrun groups,
unsupported firmware, missing recovery, incomplete records and timing failures
must remain FAIL/BLOCKED/NOT_RUN as appropriate.

## SCF/CCF instruction-history follow-up (history suite)

The new disk uses `--suite history`; default `--suite legacy` retains the
byte-identical original FDCPRB/FDCONE/FDCZEX disk. The history disk contains
**FDCHIST.COM**, **FDCFLAG.COM**, HISTHELP.TXT and the GPL license, alongside
unchanged original CP/M files. It intentionally does not add the three legacy
FDC COMs: the first all-tools prototype exhausted storage while saving the
second history log. That failure is retained. The builder now checks both
free allocation blocks and directory entries against planned maximum logs
before producing an image. The history suite reserves 24 KiB for its two logs.

Run **FDCHIST**, wait for `M139_FDC_RAM_RESTORED` and `A>`, then run **FDCFLAG**.
Each produces 512 records in its own same-base `.TXT` file. Use a fresh writable
disk copy for each machine/setting; existing logs are refused, not overwritten.
Do not interrupt the current VA2 full-ZEX run or change H/S mid-run. Full ZEX
need not be repeated unchanged just for these new probes. Record model, V1/V2,
H/S and run order. Computation is short; physical transfer/display/save duration
has not been measured. On unknown-state errors, reset without further disk I/O.

Each history uses an explicit `DI; XOR A; PUSH BC; POP AF` seed, then one of:
no extra instruction, NOP, LD A,A, OR A, AND A, CP A, SCF or CCF. SCF/CCF
immediately follows that predecessor: no CALL, dispatch, monitor I/O or AF
snapshot is inserted between them. A separate identical-prefix control span
captures pre-AF; all three spans finish before any record output. This controls
instruction adjacency, **not an independently verified internal Q value**.

FDCHIST covers all X/Y/carry combinations across four A patterns. FDCFLAG
covers broader A and S/Z/PV/N/H flag patterns, including prior discrepant cases.
A record is `Q history operation requested-AF before-AF after-AF` (S=SCF,
C=CCF). Content validation checks exact coverage, duplicate/missing records,
control preconditions, repeated pre-AF and completion/recovery. Hardware
post-AF differences are observations, not forced to match the current model.

```sh
python3 tools/hardware/upd780/build.py --suite history \
  --source-disk "$PRIVATE_INPUT" --output-dir "$OUT" --cache-dir "$ARCHIVE_CACHE"
python3 tools/hardware/upd780/results.py "$FDCHIST_TXT" --kind history \
  --profile FDCHIST --output "$HISTORY_REPORT"
python3 tools/hardware/upd780/results.py "$FDCFLAG_TXT" --kind history \
  --profile FDCFLAG --output "$FLAGS_REPORT"
```

Both programs pass the production mapped worker and actual ROM-backed software
upload/launch/recovery/persistence/DIR checks. The final reduced disk also passes
the same-disk sequence, preserving the first log while creating the second.
These are not physical acceptance, an independent Zilog oracle, or grounds
to change the FDC/non-VA-main CPU profile yet.

## Seed and raw-readback follow-up (FDCSEED)

This is a new bounded probe, not a request to repeat the completed four-setting
FDCHIST/FDCFLAG matrix. `seed.py build` creates a separate private disk copy with
`FDCSEED.COM` and `SEEDHELP.TXT`, preserving all original input files/system data.
It requires an explicitly supplied, verified z80asm 1.8 and reserves 4096 bytes
for `FDCSEED.DAT`. The old builder, loader source and all five earlier programs
are unchanged; the new loader is derived by exact-context insertion only.

```sh
python3 tools/hardware/upd780/seed.py build --source-disk "$PRIVATE_INPUT" \
  --output-dir "$NEW_SEED_OUTPUT" --assembler "$VERIFIED_Z80ASM_1_8"
python3 tools/hardware/upd780/test_seed.py
python3 tools/hardware/upd780/test_seed.py --runner "$MAPPED_WORKER" \
  --assembler "$VERIFIED_Z80ASM_1_8"
```

There are **80 operation rows**: A=00h/55h, F=08h/0Fh/28h/29h, five seed
recipes, SCF/CCF. Seed 0 preserves the previous `DI; XOR A; PUSH BC; POP AF`
anchor; seed 1 substitutes `LD A,0; OR A` (same arithmetic flags, different
opcode history). Seeds 2–4 use `LD A,08h/20h/28h; OR A` before loading the
same target AF. A separate same-prefix control captures pre-AF; no snapshot,
CALL or output interrupts seed/POP AF/IUT adjacency. These are controlled
instruction sequences, **not known physical Q values or a proven Q reset**.

All snapshots are stored before result formatting. The FDC prints ASCII from
the captured table, then returns to the monitor. Before restoring RAM, the main
controller reads the exact 652-byte table at 7000h–728Bh using command 0Bh and
appends those bytes directly to its log buffer, without hexadecimal conversion.
The new read is capacity-checked and uses the original bounded handshake. The
original full 4000h–7EFFh backup/restore/readback verification still precedes any
result-file creation. Unknown-state errors still halt without disk I/O.

The resulting **FDCSEED.DAT is mixed text/binary; do not TYPE it**. Run FDCSEED,
wait for `M139_FDC_RAM_RESTORED` and `A>`, then DIR and return the entire image.
Use a fresh writable copy per run; existing DAT is refused before takeover.
Record machine/V2/H-or-S explicitly. Initial useful targets are original VA and
VA2 in the same V2 setting; repeating all four H/S conditions is not required
for this new seed/transport question.

```sh
python3 tools/hardware/upd780/seed.py check "$FDCSEED_DAT" --output "$PRIVATE_REPORT"
python3 tools/hardware/upd780/integration.py --worker "$WORKER" --roms "$STAGED_ROMS" \
  --image "$SEED_DISK" --build-report "$SEED_BUILD_REPORT" \
  --output "$PRIVATE_CASE" --case seed --standard true
# Also --standard false; --case seed-existing takes a completed output disk.
```

Validation requires closed descriptor coverage, exact control AF, A/documented
flags, ASCII/raw agreement, binary framing, restored marker and CP/M padding.
X/Y differences and seed dependence remain observations; `--host-model` additionally
requires the present worker's A-only baseline, not physical conformance. The two
paths share captured RAM and the device/link: agreement checks formatting and
readback consistency, **not two independent silicon captures**, a physical Q read,
firmware identity or absolute timing. The mapped worker's optional second argument
saves diagnostic RAM only after successful execution; no guest instruction is
added. Native/ROM integration and this new RAM-dump worker are not a clean
application gate or independent Zilog silicon reference.

## Fixed-address repeated follow-up (FDCFIX)

FDCFIX is a distinct experiment for the remaining seed-versus-code-address
confound, not an unchanged FDCSEED/history/full-ZEX rerun. It uses **one sampler
and one IUT address** for both SCF and CCF, eight passes over target AF=0028h/
0029h and five seeds (160 rows). Passes alternate forward/reversed case order.
Seed and IUT bytes are patched and readback-checked **before** calling the shared
sampler. Control capture substitutes NOP at the IUT; the measured span is exactly
`DI; LD A,n; XOR A/OR A; PUSH BC; POP AF; SCF/CCF; PUSH AF; POP DE; RET`.
Every measured call also has the same call site. No dispatch/capture/output is
inserted between seed, POP AF and the actual IUT.

Seed 0 is now `LD A,0; XOR A`, not the earlier XOR-only byte sequence. Seed 1 is
`LD A,0; OR A`; both have the same instruction count and nominal arithmetic flags.
Seeds 2–4 use OR A after loading 08h/20h/28h. This controls positions and adds
repeat/order observations, **not all prior context, physical Q or self-modifying
fetch independently**. Readback checks memory bytes; they are not a silicon
instruction-fetch trace. The common address itself is not swept across pages.

```sh
python3 tools/hardware/upd780/fixed.py build --source-disk "$PRIVATE_INPUT" \
  --output-dir "$NEW_FIXED_OUTPUT" --assembler "$VERIFIED_Z80ASM_1_8"
python3 tools/hardware/upd780/test_fixed.py
python3 tools/hardware/upd780/test_fixed.py --runner "$MAPPED_WORKER" \
  --assembler "$ABSOLUTE_Z80ASM_1_8"
```

The new private disk contains FDCFIX.COM/FIXHELP.TXT and reserves 8192 bytes for
**FDCFIX.DAT**, a mixed text/binary file. Use fresh writable copies; run FDCFIX,
wait for `M139_FDC_RAM_RESTORED` and `A>`, then DIR. **Do not TYPE the DAT.**
Return the whole disk and explicitly identify actual machine/V2/H-or-S. Initial
VA and VA2 runs in the same V2/H setting suffice for this question; a new four-
setting matrix is not requested. Existing DAT is refused before takeover.

Raw/text recovery uses the seed transport: after all snapshots/formatting, the
main controller reads 1452 bytes at 7000h–75ABh using monitor 0Bh, then restores
and readback-verifies all saved RAM before any result-file creation. Control
byte 03 is specific to this diagnostic stream, not a new firmware opcode: a
patch-readback failure makes the new controller print `M140_FIXED_PATCH_VERIFY`
and take the existing unknown-state fatal halt, with no disk operation. Timeout,
patch or recovery failure requires RESET. Legacy/seed loaders stay unchanged.

```sh
python3 tools/hardware/upd780/fixed.py check "$FDCFIX_DAT" --output "$PRIVATE_REPORT"
python3 tools/hardware/upd780/integration.py --worker "$WORKER" --roms "$STAGED_ROMS" \
  --image "$FIXED_DISK" --build-report "$FIXED_RECIPE" \
  --output "$PRIVATE_PASSING_CASE" --case fixed --standard true
# Also --standard false; --case fixed-existing uses a completed output disk.
python3 tools/hardware/upd780/test_fixed.py --rom-worker "$WORKER" \
  --roms "$STAGED_ROMS" --source-image "$FIXED_DISK" --build-report "$FIXED_RECIPE" \
  --passing-integration "$PASSING_RUN_JSON" --output "$PRIVATE_FAULT_CASE"
```

The last command reuses a passing, digest-bound ROM integration fixture rather
than rerunning it. Exactly one COM opcode is mutated (`CP (HL)` to `OR (HL)` in
patch verification). It requires the dedicated guest error and a byte-identical
post-run disk; a zero worker exit alone is not PASS. Outputs are private, out of
tree; this is SDL-dummy software fault-safety evidence, not physical halt-state
readout. Unit negatives isolate content from media/worker identity checks.

Content validation requires all repeat/seed/operation/target descriptors,
exact control AF, documented flags, raw/text agreement, framing, recovery marker
and CP/M padding. Repeat-varying groups and differing per-seed result sets are
reported as observations, not rejected or fitted into a NEC rule. `--host-model`
adds the present worker's A-only baseline requirement. No Q value, silicon cause,
physical repeatability or timing is established by software success.

## Capture/stack/storage follow-up (FDCCAP)

The VA fixed-address results strongly support old-F/A OR **in that instruction
sequence** (159/160), with one outlier. FDCCAP tests capture/storage alternatives;
it is not a claim that no flag rule is visible, nor an unchanged FDCFIX replay.
There are **96 rows**: target AF=0028h/0029h, SCF/CCF, seed IDs 0/1/3 and eight
forward/reversed passes. Seed 3 retains the earlier outlier's OR-A/20h recipe.
A common sampler/IUT is used, at **new** addresses 4800h/4806h. Other code/context
changed too; comparison with the previous executable is not address-only. A is
zero in these targets, so this is not a full A/F truth table or a way to uniquely
distinguish OR from every other rule agreeing on those selected inputs.

After each IUT, three consecutive PUSH AF instructions use different adjacent
stack cells. POP HL/BC/DE exports the three words before any comparisons change
flags. A NOP substitutes for the control IUT. The three words are compared in
registers, then copied through code-RAM mailboxes into the result row. Primary
post-AF also gets duplicate/inverted copies from the mailbox, not from reading
back the primary row. Snapshot/mirror word stores have immediate comparisons;
a final scan checks the duplicate/inverse relations before formatting.

Before captures, 7000h–7857h (including the previous outlier's 74D7h cell) is
filled then read with 00h/FFh/55h/AAh/29h/21h and low-address XOR high-address
patterns. Pretest counts/first address, immediate-store/triple status and delayed
scan counts are retained. This is a bounded result-RAM check, **not exhaustive
RAM qualification**. It does not test stack/code RAM in the same way. Mailboxes,
CPU, repeated PUSH-AF export and output/readback remain shared paths. Agreement
is not an independent original-register/Q readout or proof that old RAM was bad.

```sh
python3 tools/hardware/upd780/capture.py build --source-disk "$PRIVATE_INPUT" \
  --output-dir "$NEW_CAPTURE_OUTPUT" --assembler "$ABSOLUTE_Z80ASM_1_8"
python3 tools/hardware/upd780/test_capture.py
python3 tools/hardware/upd780/test_capture.py --runner "$MAPPED_WORKER" \
  --assembler "$ABSOLUTE_Z80ASM_1_8"
```

The new private disk contains FDCCAP.COM/CAPHELP.TXT and reserves 12288 bytes
for **FDCCAP.DAT**. Use a fresh writable copy, run FDCCAP, wait for
`M139_FDC_RAM_RESTORED` and `A>`, then DIR and return the whole image. **Do not
TYPE the mixed text/binary DAT.** Initially use VA/V2/H and VA2/V2/H, explicitly
reporting actual settings. Timeout, patch-verify or recovery failure requires
RESET without disk operations. Existing DAT is refused before takeover.

The controller reads the 2136-byte table through monitor 0Bh, then restores and
readback-verifies all saved RAM before file creation. The new loader alone uses
`M140_CAPTURE_PATCH_VERIFY`; old programs/loaders stay unchanged.

```sh
python3 tools/hardware/upd780/capture.py check "$FDCCAP_DAT" --output "$PRIVATE_REPORT"
python3 tools/hardware/upd780/integration.py --worker "$WORKER" --roms "$STAGED_ROMS" \
  --image "$CAPTURE_DISK" --build-report "$CAPTURE_RECIPE" \
  --output "$PRIVATE_CASE" --case capture --standard true
# Also --standard false; --case capture-existing takes a completed output image.
python3 tools/hardware/upd780/test_capture.py --rom-worker "$WORKER" \
  --roms "$STAGED_ROMS" --source-image "$CAPTURE_DISK" --build-report "$CAPTURE_RECIPE" \
  --passing-integration "$PASSING_CAPTURE_RUN_JSON" --output "$PRIVATE_FAULT_CASE"
```

Container/descriptor/transport/framing/recovery validation is separate from
**diagnostic findings**. A coherent triple, control, documented-flag, mirror,
pretest or delayed-summary problem is retained in `record_findings` or
`header_findings`, not silently fitted into a CPU rule. `COMPLETE_OBSERVATIONS`
and exit 0 mean complete diagnostic data, **not integrity PASS**: inspect
`integrity_status`. `--host-model` rejects findings, then requires the current
A-only baseline. Primary host/OR counts are exploratory if integrity findings
exist. Even `NO_FINDINGS_IN_TESTED_CONTRACT` is not a physical conformance gate.

Pure content negatives have passing synthetic fixtures and exact stable codes.
Mapped single-byte mutants additionally exercise the actual RAM-error counter,
register-triple comparison, immediate store verification and delayed mirror scan.
The ROM fault test reuses a digest-bound passing fixture, verifies a dedicated
error and unchanged disk, and does not rerun an unchanged positive case. No
hosted CI/private-input upload, physical timing or CPU/profile fix is implied.

## Offline binary context audit

`context.py` audits the existing seed/fixed/capture binaries without regenerating
or executing them. It checks entry/SP setup, measured/control opcode spans,
CALL destinations, snapshot push/pop order and stores; capture additionally
requires both BC reloads, mailbox word order and immediate save calls. It is
**not a full control-flow verifier or physical instruction/SP trace**. A nominal
stack diagram assumes balanced calls without interrupts. Seed's XOR-only seed 0,
inline/no-CALL capture and omitted (not substituted) control IUT differ from
later programs and are reported explicitly.

```sh
python3 tools/hardware/upd780/context.py --kind capture \
  --payload "$FDCCAP_BIN" --symbols "$FDCCAP_SYMBOLS_JSON" --program "$FDCCAP_COM" \
  --log "$RETURNED_FDCCAP_DAT" --output "$NEW_OUT_OF_TREE_AUDIT_JSON"
python3 tools/hardware/upd780/test_context.py
```

`--log` is optional for static-only auditing. Output must be new and outside
the source tree. The complete payload must occur exactly once in the supplied
COM; deployment identity still comes from the intake/master recipe. Existing
content validators run before observations are decoded. Capture integrity
findings block this hypothesis layer (`M140_CONTEXT_INTEGRITY`) rather than
being silently interpreted as original CPU flags. No Git history, private input
or assembler is needed for the synthetic unit fixtures.

Literal F3=IUT-address-bit3, F3=CALL-return-bit3 and F3=current-PUSH-address-bit3
are **closed falsification probes**, not fitted NEC models. Counts have explicit
denominators; an inline program's missing return address is `NOT_APPLICABLE`,
not a zero-error PASS. An uncontradicted constant-bit coincidence is only
`NOT_FALSIFIED_IN_DATA`. Repeated results at the same target-AF/seed/op/static
addresses can falsify determinism over those inputs alone, but do not exclude
hidden history, phase or address-dependent effects jointly with other state.
Source/input digests, commands and counterexample witnesses go in the private
JSON. The current audit does not establish an OR/Q rule or authorize a new CPU
profile, relocation fix or unchanged hardware replay.

## Offline SCF/CCF model reduction

`reduction.py` validates completed physical logs and compares A-only,
unconditional old-F/A OR, and an explicitly labeled software-history candidate.
It also checks the A/F/Q slice of the public SingleStepTests/z80 software corpus,
commit `ebe1875d48f374bcfd4b505d8eb8ee751568b5f7`, files `v1/37.json` and
`v1/3f.json`. Obtain those pinned files out of tree; the CLI verifies their
SHA-256 identities and requires 1000 records each. It does not download anything.

```sh
python3 tools/hardware/upd780/reduction.py \
  --scf-reference "$SST_37_JSON" --ccf-reference "$SST_3F_JSON" \
  --capture hw-case-01 FDCHIST "$FDCHIST_TXT" \
  --capture hw-case-01 FDCFLAG "$FDCFLAG_TXT" \
  --output "$PRIVATE_ANALYSIS"
```

Repeat `--capture CASE_ID PROFILE LOG` for additional retained settings. Keep
inputs, output and exact command/identity records outside Git. Output includes
private log digests. `COMPLETE_ANALYSIS_NOT_CONFORMANCE` means analysis completed,
not silicon or milestone acceptance. These Ares-derived software tests are not
physical Zilog observations; no full-core/cycle SST run is performed. A log's
separate control AF is not a direct read of hidden Q at the instruction under
test. Content functions and reference identity checks are independently callable
for focused tests and do not depend on Git history or private assets. See the
[derived reduction and its limits](../../../docs/agents/reports/m140_fdc_va_observations.md#offline-reduction-and-separate-software-q-controls).
No unchanged physical history/full-ZEX rerun is required.

## Local tests and evidence separation

```sh
python3 tools/hardware/upd780/test_fdc_tools.py
cmake --build "$BUILD" --target vaeg_upd780_fdc_probe_runner
ctest --test-dir "$BUILD" -R '^vaeg_upd780_fdc_' --output-on-failure
"$BUILD/vaeg_upd780_fdc_probe_runner" "$OUT/fdcprb.bin" > "$HOST_TXT"
python3 tools/hardware/upd780/results.py "$HOST_TXT" --kind probe \
  --unit MODEL_T --host-model --output "$HOST_REPORT"
```

The C++ worker links the production uPD780 role, restricts data/opcode RAM,
and supplies only a tiny synthetic monitor transport at RST18h/JP2Ah. Its
`MODEL_T` is synthetic cycle accounting, not physical timing or firmware
integration. Content/allocator negatives start from passing fixtures and
assert the specific stable error code after one mutation. Optional assembled
payload tests include AF/T-state contracts, NEG smoke and a single-address
unmapped-write negative. Full 67-group mapped runs are bounded expensive
local evidence, not repeatedly rerun during documentation changes.

Private ROM/CP/M integration additionally requires actual protocol upload,
launch, completion, byte-verified recovery, persisted log validation,
unchanged pre-existing file contents and a healthy post-return DIR/prompt.
A zero worker exit or screenshot alone does not satisfy those checks.
The explicit local runner is:

```sh
python3 tools/hardware/upd780/integration.py \
  --worker "$WORKER" --roms "$STAGED_ROMS" --image "$OUT/$INPUT_BASENAME" \
  --build-report "$OUT/build-report.json" --output "$PRIVATE_CASE" \
  --case probe --standard true
# Also run --standard false; --case smoke uses NEG-only; --case existing
# takes a prior probe's output image and verifies safe existing-log refusal.
```

Its CP/M text contract is F300h/120-byte rows/80 characters, not N88-BASIC's
F3C8h layout. Outputs contain private identities and must remain out of Git.
Dummy-video integration does not establish physical FDC timing, independent
Zilog conformance, presenter inspection or any physical display acceptance.
See [M140](../../../docs/agents/tasks/M140_fdc_upd780_conformance.md) for
current evidence and the still-blocked broader milestone gate.
