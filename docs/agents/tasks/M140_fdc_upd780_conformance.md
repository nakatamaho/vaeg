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
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# M140 — physical FDC uPD780 conformance and speed probes

Status: maintainer-authorized separation from M139 because the milestones
would otherwise overlap. Harness and mapped images implemented; inherited
software checks pass within their stated scope. Returned VA media now contains
complete full ZEX/AF/timing observations: 67/67 mapped-host CRCs match, while
35 AF input pairs expose SCF/CCF undocumented-bit discrepancies. All original
files/programs and boot/system sectors are preserved in that VA case. VA2/V2/S
follow-ups now match all 67 legacy CRCs and both 512-row controlled-history
sets, with zero legacy AF differences. Original VA/V2/S now has both controlled
logs: FDCHIST has 84 X/Y post-AF differences, while FDCFLAG matches. The VA/V2/H
controlled logs match VA/S byte-for-byte; newly received VA2/H also matches
VA2/S byte-for-byte. All four controlled settings are collected. The new seed
probe is also returned under VA/V2/H and VA2/V2/H labels: 80 rows each, raw/text
agreement throughout, 30 versus zero host differences; one VA CCF group varies
across seed recipes, with address/context still confounded. Fixed-address
follow-ups now have 160 rows each: VA has 160 host differences and one repeat
outlier against OR; VA2 has zero host differences and no repeat variation.
FDCCAP returns add 96 rows each: both have no findings in bounded RAM/triple/
mirror checks. VA has 96 host differences, OR fits 56, and three CCF groups vary
coherently across all three captures; VA2 matches host throughout. Common CPU/
export/context effects remain unisolated. **G140 BLOCKED**:
history-controlled discrepancy reduction, independent Zilog controls, approved
timing reference, physical H/S and remaining gate obligations are open.
Branch: `topic/m140-fdc-upd780-conformance`.
Base: `d483636a70c427953bfa13cce0b27ca27216829e`.

## Authorization and ownership

M140 owns FDC-side RAM upload/launch/result/recovery, relocated full ZEXALL,
AF/boundary/undocumented probes, separate speed kernels, physical evidence and
any justified suzukiplan upstream work. The available physical uPD780 is in
the FDC: main-CPU CP/MVA and host Zilog-model runs are not its measurements.
[M139](M139_upd780_or_upd9002_check.md) retains main-CPU role selection and
the later private guest stop. Neither gate is accepted by this separation.
The maintainer explicitly authorizes this separate task while G139 remains
open. This branch inherits the pending M139 implementation; that ancestry is
not authorization to merge M139 or relabel its application as a clean candidate.

No past commit, release tag, evidence identity or private handoff is rewritten.
The earlier source and software results are retained in the
[verbatim pre-split record](../reports/m140_fdc_pre_split_record.md).
Those evaluated commits and their original FAIL/revalidation reports remain
historical facts, not newly executed M140 tests.

## Implementation and patch ownership

- [Tools and run instructions](../../../tools/hardware/upd780/README.md):
  FDCONE (NEG-only), FDCPRB (256 AF pairs, 67 timing rows), FDCZEX (67 groups).
- [Canonical firmware ABI analysis](../../bios/fdc_monitor.md): bounded takeover,
  saved/restored/readback-verified RAM, deterministic zeroed diagnostic tail,
  motors off during computation, no disk writes before recovery; unknown-state
  errors halt and require reset. Verify the supported monitor family first.
- ZEX transformations belong under `external/zex/patches/`, with exact pinned
  original/translated source, patch and output hashes. Apply reproducible diffs
  out of tree; never hand-edit the original `zexall.src`, `zexdoc.src` or license.
  Preserve original GPL notices, all full-run groups/opcode/scan vectors/masks,
  explicitly describe relocation and 0339h→7339h scratch adaptation, and label
  the separate NEG-only patch as reduced smoke coverage.
- Preserve the existing v1 `M139` banner/error/schema tokens as **legacy
  transport/evidence identifiers**, not current milestone ownership. Renumbering
  them would change already-tested COM/BIN bytes for no CPU-behavior reason.
  New patch-validation errors use M140. No physical result is inferred from
  either prefix.
- Do not silently apply measured FDC silicon quirks to every non-VA main CPU.
  Reduce a demonstrated mismatch, use independent Zilog/relocation controls,
  preserve VA/uPD9002 and Zilog profiles, and follow pinned upstream/patch
  provenance rather than editing `external/suzukiplan-z80/` by hand.

## Checks and evidence reuse

Run encoding/LF/case/Markdown checks, strict milestone selftest/audit, focused
`vaeg_upd780_fdc_*` CTest and tool tests. Validate patch integrity/context and
passing-fixture single-mutation negatives independently of Git history. Prove
that extracted patches reproduce the previous full/smoke ASM and every
COM/BIN byte. Keep manifests/evidence generation out of tree before placement.

Reuse the inherited completed 67-group mapped run, stock baseline and private
CP/M upload/restore/persistence/DIR cases only when worker, program/corpus and
target-policy identities match. A metadata/patch-storage change is not grounds
to rerun unchanged expensive emulator behavior. Record a new comparison-contract
identity when a validator changes; never overwrite an original failed report.
Private media, ROM identities, logs/captures and generated images stay outside
Git and hosted CI. Existing expensive evidence/handoffs and shared cache are
not moved or deleted as part of the milestone separation.

## Reassignment and external-patch validation

The patches are generated out of tree, exact-context applied and checked before
repository placement. Full and NEG-only generated ASM are byte-identical to
the pre-extraction versions. Rebuilding the private image with the extracted
series produces identical bytes for all three COMs/BINs, both added text files
and the complete D88. Its private input/image hashes remain outside Git.
The original two upstream sources and GPL license still match their pinned hashes.

Local checks: **19/19 content/allocator tests PASS** (including three new
passing-fixture single-mutation patch input/digest/output negatives), and
**2/2 focused CTest PASS**, 2.78 s. The mapped production worker remains
SHA-256 `4f8a14266448b76d82490d87dd48cc4733f42e8b46ff77b135647f9f49541fce`.
No emulator execution source, reader comparison functions or timing bodies
change; the original full mapped/stock runs and private ROM-backed S/H,
NEG/recovery/persistence/DIR and refusal captures are retained, not rerun or
promoted to physical results. A fresh M140 handoff is generated with matching
program/image identities, without moving or deleting the earlier handoff.

Run commands:

```sh
python3 tools/hardware/upd780/test_fdc_tools.py
cmake --build "$BUILD" --target vaeg_upd780_fdc_probe_runner
ctest --test-dir "$BUILD" -R '^vaeg_upd780_fdc_' --output-on-failure
python3 tools/hardware/upd780/build.py --source-disk "$PRIVATE_INPUT" \
  --output-dir "$M140_HANDOFF" --cache-dir "$ARCHIVE_CACHE" \
  --assembler "$VERIFIED_Z80ASM_1_8"
```

These exit 0. Encoding/LF/case/Markdown checks, 49 strict milestone checks,
milestone audit and staged scope/diff checks also exit 0. Exact unified-diff
context tabs/blank lines remain intact, with closed-path whitespace attributes
for the two patch files only; the patches remain text-diffable.
No hosted CI or unchanged expensive CPU/media run is requested for this
administrative separation and byte-identical patch-storage refactor.

## Initial physical VA smoke observation — hw-fdc-va-01

The maintainer identifies the target as a physical VA. An operator-supplied
photo shows `FDCONE` producing:

```text
M139 FDC NEG SMOKE ONLY
neg... STOCK=d638dd6a CRC=d638dd6a
Tests complete
M139_FDC_RAM_RESTORED
```

The photo also shows a subsequent successful `TYPE FDCONE.TXT` reading the
same records and returning to `A>`. Thus **NEG-only execution/CRC observation
and post-return persisted log readback PASS within this photographed case**.
The CRC agrees with the displayed stock expectation; it is not a new NEC
instruction difference. Recovery is reported by the controller, whose intended
implementation verifies RAM readback; this is not an independent physical
RAM dump comparison. The image additionally shows an earlier unsuccessful
TYPE of another filename, followed by the successful correct-name readback;
that is not evidence of a failed persisted `FDCONE.TXT`.

Raw photo/derived display copy and transcription stay outside Git under a
neutral observation ID. The intended handoff source is
[8931ea75ea2e5e0a110ebc27bd37595ceb624789](https://github.com/nakatamaho/vaeg/commit/8931ea75ea2e5e0a110ebc27bd37595ceb624789),
but physical-media program hashes, firmware revision and machine configuration
are not independently established from this photo. At this initial photo-only
stage there was no raw off-device log; the subsequent returned-media case below
supplies complete records and matching returned-program identities. No claims are made for unchanged original files, a post-run DIR,
VA2, full 67-group conformance, the AF matrix, H/S timing or physical MHz.
The next staged collection is FDCPRB and its persisted log; the full FDCZEX
run remains separate. No G140 acceptance or emulator correction follows.

## Returned physical VA media — hw-fdc-va-02

See the [analysis and verification report](../reports/m140_fdc_va_observations.md).
The maintainer subsequently confirms the setting as **VA / V2 / H**, not VA2.
This annotates the existing capture; it is not a second run or an independently
measured FDC clock. The S-setting comparison remains unrun.
All 67 full-ZEX CRCs match the preserved identical-image mapped host. The 29
stock-address mismatches also occur in that host and are not NEC differences.
All 256 AF pairs and 67 timing records validate structurally; 35 AF pairs differ
only in SCF/CCF X/Y bits (34 SCF, 35 CCF). Hidden Q/history is uncontrolled,
so a minimal controlled probe and independent Zilog control are needed before
any backend/profile change. No absolute speed conversion or physical H/S
comparison is justified yet; no unchanged full run needs repeating.

The received D88 uses a double-step track table. The original strict-intake
failure is preserved separately from header-only normalization and production
reader validation; sector headers/payloads and the supplied original are not
modified. The [readback correction](https://github.com/nakatamaho/vaeg/commit/26108cb2c3701b8511ba8a76064cb1db187bf74d)
keeps writable-media geometry strict. Tests: 24/24 unit, 2/2 focused CTest PASS;
encoding/LF/case/Markdown/milestone/diff checks all exit 0. Private
snapshot/log/contract/output identities and exact commands remain outside Git.

## History-controlled follow-up disk (r2)

The maintainer requests additional probes and a D88 while the physical VA2
measurement is running. Do not interrupt that run. New **FDCHIST** and
**FDCFLAG** programs/log names preserve the existing full-ZEX/FDCPRB results.
Use a fresh writable copy per machine/setting; run the two new commands in
that order, waiting for recovery and `A>` after each, then return both logs.
At preparation time physical VA2/new-probe results were pending. Subsequent
VA2/V2/S captures now validate all legacy records and both 512-row history
sets; every history control/result row matches the same-image mapped host.
A temporary H-looking filename was corrected to S before intake; that earlier
case remains S. A separate newly supplied VA2/V2/H result now matches VA2/S
byte-for-byte. VA/V2/H also matches original-VA/V2/S byte-for-byte. See the
[updated physical matrix](../reports/m140_fdc_va_observations.md#va2--v2--s-follow-ups).
The VA/H legacy AF discrepancy is still unresolved, not a fixed NEC rule.
A further original-VA/V2/S capture (`hw-fdc-va-03`) completes both history logs:
FDCHIST has 84 post-AF X/Y differences (42 SCF, 42 CCF), plus 56 distinct-row
pre-AF differences in shared controls; FDCFLAG fully matches mapped host and
VA2/S. This now compares the same V2/S labels across machines, so H/S mismatch
alone cannot explain the observations. Both exact COMs, original CP/M input
files and system region are preserved; two supplementary texts do not match
the complete package. Hidden Q, CPU variant/firmware and a universal flag rule
remain unestablished. The subsequent VA/V2/H capture (`hw-fdc-va-04`) has the
same 84 post-AF/56 pre-AF FDCHIST row differences and zero FDCFLAG differences;
both persisted logs are byte-identical to VA/S. This isolates equal observed
controlled flags across H/S on the reported VA, not equal timings or clocks.
The final VA2/V2/H case (`hw-fdc-va2-03`) completes both 512-row logs with zero
mapped-host control/result differences; both logs are byte-identical to VA2/S.
The four-setting matrix now contains 4096 operation rows. Both machines' H/S
flags agree within that reported machine; the discrepancy tracks machine in
these samples, not H/S. Original files/programs/system regions are preserved;
the two supplementary text mismatches in these follow-ups are not silently
classified as full-package identity. Configuration remains operator metadata.
Do not request unchanged repeat histories/full ZEX: next work is discrepancy
reduction and independent controls, with timing calibration separate. See the
report; no backend change or G140 acceptance is made.

The [history-suite instructions](../../../tools/hardware/upd780/README.md#scfccf-instruction-history-follow-up-history-suite)
describe both 512-record matrices. Each span seeds `DI; XOR A; PUSH BC; POP AF`,
then one of eight predecessors immediately followed by SCF/CCF. A separate
identical-prefix control captures pre-AF; snapshots/CALL/dispatch/monitor I/O
never interrupt predecessor/IUT adjacency. This controls instruction history,
not a known internal Q value or an independent Zilog oracle. All three spans
complete before output. Both generated ASM and actual instruction bytes are
checked, including DI, seed, predecessor, IUT and capture order.

Verification: **35/35 unit tests PASS**, **2/2 focused CTest PASS**, 2.88 s.
Each mapped production-worker run emits 512 records (11319 bytes before the
loader recovery line); host arithmetic comparisons and bounded-map execution
pass. Program bytes are unchanged between prototype r1 and final r2. The
old three programs and default legacy D88 also remain byte-identical to the
previous handoff; no unchanged full mapped/stock run is repeated.

Private-input software integration with the existing worker and a ROM set
matching MAME's `pc88va2` passes upload/launch/verified recovery, 512 persisted
records per program, original-file preservation and post-return DIR/prompt.
Both individual r1 runs pass. The first all-tools same-disk sequence then
fails while saving the second log: the guest reports `M139_LOG_WRITE_FAILED`,
leaves a closed partial file and returns to CP/M; content validation correctly
fails `M139_RECOVERY_RECORD`. This is demonstrated allocation exhaustion, not
an FDC instruction failure. The original failure, disk and captures are kept.

Final `--suite history` omits the three extra legacy FDC COMs/FDCHELP from the
new disk (but preserves every original input file and the GPL license). It
has **55296 bytes free**, reserving **24576 bytes** plus two directory entries
for both maximum-size logs. A new content-only headroom guard rejects an
under-budget build before writing the D88. Passing-fixture single-mutation
block/directory/budget negatives assert stable codes. Default `--suite legacy`
retains the original disk without adding these new tools.

The final r2 **same-boot FDCHIST then FDCFLAG PASS**: both complete 512-record
logs persist and guest DIR/prompt succeeds. A separate restarted-worker,
same-disk sequence also passes, preserving the first log and all original files. This remains SDL-dummy software integration,
not a physical display or new physical-silicon PASS. Exact worker/corpus/recipe/
contract/command/output identities and initial failed evidence stay out of Git.
No CPU profile or shared backend change follows from these software controls.
Implementation and capacity correction:
[1560faac6d1f2689c86614d3b6c26f76ea1388b0](https://github.com/nakatamaho/vaeg/commit/1560faac6d1f2689c86614d3b6c26f76ea1388b0).
Repository encoding/LF/case/Markdown, 49 strict milestone checks, milestone audit
and staged scope/diff checks exit 0. Hosted CI/private uploads are not requested.

## Offline discrepancy reduction and software-reference control

The [derived reduction](../reports/m140_fdc_va_observations.md#offline-reduction-and-separate-software-q-controls)
now compares all 4096 retained history operation rows without new hardware runs.
VA's FDCHIST has six post cases outside both A-only and old-F/A OR, reducing to
AF `0029`, three predecessors and SCF/CCF. VA2 has none. A MAME-source-informed
software history candidate leaves VA FDCHIST six post/four shared-control row
mismatches, but also leaves FDCFLAG 24 post/16 control row mismatches on both
machines. No candidate is a demonstrated NEC rule, and physical Q is not read.

New `tools/hardware/upd780/reduction.py` checks pinned SingleStepTests/z80
SCF/CCF inputs (1000 each): all agree with the corpus Q formula, while the
current A-only expression differs on 229/219. This is a separate Ares-derived
software A/F/Q control, **not Zilog silicon, full-core SST or bus/cycle coverage**.
No external code/corpus is vendored and no emulator/profile/program bytes change.
The physical root cause and independent silicon control remain open.

Commands, exit status and generated output identities are retained out of tree:

```sh
python3 tools/hardware/upd780/test_fdc_tools.py
ctest --test-dir "$BUILD" -R '^vaeg_upd780_fdc_' --output-on-failure
python3 tools/hardware/upd780/reduction.py \
  --scf-reference "$SST_37_JSON" --ccf-reference "$SST_3F_JSON" \
  --capture hw-case-01 FDCHIST "$FDCHIST_TXT" \
  --capture hw-case-01 FDCFLAG "$FDCFLAG_TXT" --output "$PRIVATE_ANALYSIS"
```

All exit 0: **46 direct unit tests and 2 focused CTests PASS**; production
analysis includes all eight logs and the pinned 2000 reference records. Focused
single-mutation negatives assert exact codes without Git or corpus dependencies;
combined reference identity/content plus complete-history analysis is also tested.
An exploratory F-only/complete-AF comparison mistake and initial download 404s
remain preserved and explicitly superseded. Corrected counts preserve A and
include a nonzero-A regression. Unchanged expensive worker/full-ZEX evidence is
reused; raw private identities and generated analysis remain untracked. Repository
encoding/EOL/case/Markdown checks, 49 strict milestone selftests and the milestone
audit exit 0. No new hardware request, merge, release, cleanup or G140 acceptance
follows.

## Bounded seed / raw-readback probe (FDCSEED)

The [new probe and usage](../../../tools/hardware/upd780/README.md#seed-and-raw-readback-follow-up-fdcseed)
implements the next distinct control without changing the completed history
matrix or prescribing a NEC rule. There are 80 rows: two A values, four F
values (including the reduced `0029` case), five seed recipes and SCF/CCF.
The original XOR-A anchor is retained alongside zero/nonzero OR-A seeds.
A separate control span captures pre-AF without perturbing IUT adjacency;
source and all 160 assembled control/IUT spans are checked. Physical Q is not
read or assigned by the seed descriptions.

All results are captured in RAM before formatting. FDC ASCII and a subsequent
monitor-0Bh binary read of the same 652-byte table must agree. The main controller
appends binary bytes directly before full RAM restoration/readback verification;
only after that may BDOS create `FDCSEED.DAT`. This mixed text/binary file is not
for TYPE. The new loader retains bounded handshake, capacity checks, stack/DMA
cleanup, existing-log refusal and unknown-state halt behavior. Shared captured
RAM means two transport paths, not independent silicon observations.

Local validation:

- **13/13 new content/source tests PASS**, with passing fixtures, single-mutation
  negatives and exact stable codes; **46/46 existing direct tests PASS**.
- **4/4 focused FDC CTests PASS**, including source/machine-code adjacency and
  mapped execution with post-execution diagnostic-RAM dump. All 80 raw/text AF
  rows match the production host baseline. This is not physical conformance.
- ROM-backed **H and S integration PASS** with a ROM set matching MAME's
  `pc88va2`: 80 persisted rows, raw/text equality, reported full RAM restoration
  with byte-verified readback, source/original-file/system-area preservation,
  2560-byte DAT and post-return DIR/A>. SDL dummy, not display/silicon coverage.
- A completed H disk **existing-DAT refusal PASS**: complete file set unchanged,
  refusal visible and healthy DIR/A>. No earlier physical capture is regraded.
- Fresh legacy and history disk rebuilds are **whole-image byte-identical** to
  their previous masters. No original payload, patch, loader or CPU core changes.
  The separate seed disk has 94208 free bytes and reserves 4096 for its result.

Repository encoding/EOL/case/Markdown checks, 49 strict milestone selftests,
milestone audit, changed C++ formatting and staged diff checks all exit 0.

The old mapped worker is retained at its previous digest for unchanged expensive
full/stock evidence. The new worker adds only an optional post-success RAM dump;
its new digest is recorded separately. Production application worker remains
unchanged. Exact commands, worker/recipe/corpus/contracts, produced image hashes
and generated captures/results stay out of Git in task-specific directories.
No private inputs or generated media are uploaded to CI. No full-ZEX rerun is
needed for this distinct probe. Physical FDCSEED execution was NOT_RUN at
preparation; the separate returned cases below do not change those software-run
identities. G140, independent Zilog silicon, absolute timing and broader gates
remain open.

## Returned seed/readback cases

See the [new observations](../reports/m140_fdc_va_observations.md#returned-fdcseed-seedreadback-controls):
`hw-fdc-va-05` and `hw-fdc-va2-04`, returned with VA/V2/H and VA2/V2/H labels.
Configuration labels follow the returned filenames/requested targets, not an
independently encoded hardware identity. Both 80-row DAT files pass the unchanged
production content CLI (exit 0), including control AF, documented flags, raw/text
agreement, framing and loader restoration marker. All complete master files,
exact generated COM and boot/system region are preserved. No separate physical
full-RAM dump or final DIR/prompt capture is provided by these images alone.

VA has 30 X/Y-only post differences (15 SCF/15 CCF) from current host; VA2 has
zero. VA CCF at AF `0029` gives `0030` after XOR-A and `0038` after zero OR-A,
with `0030` for the other three seed recipes. This one observed group is not
a deterministic rule: the unrolled IUTs occupy different addresses/contexts.
Both machines' eight overlapping XOR-anchor rows match their earlier H history
captures. Agreement of raw RAM and ASCII argues against a formatter-only cause,
not against a shared capture/RAM issue or every possible internal-Q mechanism.

Snapshots, CLI commands/exit status, input/program/contracts and output digests
are retained outside Git. Unchanged unit/mapped/ROM software evidence from
[21e7dcb513ab2ccef20fdada3c6eee25151fe67e](https://github.com/nakatamaho/vaeg/commit/21e7dcb513ab2ccef20fdada3c6eee25151fe67e)
is reused; no code, program, worker or content contract changes in this intake.
Encoding/EOL/case/Markdown, 49 strict milestone checks, audit and staged scope
checks exit 0. Next work is address-controlled repeated minimal-probe design,
not an unchanged physical rerun. No CPU fix, merge or gate acceptance follows.

## Fixed-address repeated control prepared (FDCFIX)

The [new bounded control](../../../tools/hardware/upd780/README.md#fixed-address-repeated-follow-up-fdcfix)
reduces the unrolled-address confound without changing previous programs/results.
A single RAM sampler at 40D8h, IUT at **40DEh**, and measured call at 404Ah serve
all SCF/CCF cases. Verified self-modification selects seed immediate/ALU and IUT
before entry; no patching/dispatch/output occurs inside the measured span.
Control capture substitutes NOP at that IUT, rather than taking a snapshot
before the actual instruction. All results are captured before formatting.

160 rows cover AF=0028h/0029h, five seeds, SCF/CCF and eight passes, alternating
forward/reverse case order. Both zero seeds now start LD A,0, so XOR/OR share
instruction count/positions and nominal arithmetic flags. This is a **new**
seed-0 byte sequence, not relabeled old XOR-only evidence. The protocol compares
ASCII with the same captured RAM via monitor 0Bh (1452 bytes), followed by full
verified RAM recovery and only then file creation. It is not independent CPU
capture, physical-Q reset/readout, all-context isolation or an instruction-fetch
trace. One fixed address also does not constitute a code-address sweep.

Verification and evidence:

- **16/16 new unit/content/source tests PASS**, including closed repeat/seed
  coverage, forward/reverse order, repeat variation as observation, exact-error
  single-mutation negatives and the one-byte fault-fixture generator.
- **6/6 focused FDC CTests PASS** (3.12 s): all existing cases, new mapped
  self-modification execution, common-call/IUT machine bytes, final patched RAM
  image, 160 raw/text AF rows and the dedicated error-byte failure path.
- ROM-backed **H and S integration PASS** with a ROM set matching MAME's
  `pc88va2`: 160 persisted rows, raw/text agreement, reported readback-verified
  RAM restoration, all pre-existing files/system region unchanged, 5760-byte
  DAT and final DIR/A>. SDL dummy, not physical/presenter coverage.
- Completed H image **existing-DAT refusal PASS**, all files unchanged and
  refusal/DIR/prompt present.
- A digest-bound passing H fixture is reused for **one-opcode fault QA**:
  CP (HL) becomes OR (HL) in seed-patch verification. The guest displays
  `M140_FIXED_PATCH_VERIFY`, and the fault disk is byte-identical after execution.
  This is dedicated-message/no-write software safety, not a physical halt-state
  readout. No repeated positive worker run is needed for that negative.

The new COM is 3567 bytes; the new disk has 96256 free bytes and reserves 8192
for FDCFIX.DAT. Loader capacity, timeout, restoration/readback, stack/DMA cleanup
and unknown-state halt policy remain intact. New diagnostic control byte 03 is
handled only by the new derived loader; the old loaders/programs are unchanged.
An initial standalone payload-test invocation used a relative assembler path
and failed before assembly in its temporary directory. Its failure is retained;
the corrected absolute-path invocation passes, with no guest/core workaround.
Repository encoding/EOL/case/Markdown, 49 strict milestone checks, milestone
audit and staged diff/scope checks exit 0.

Private recipes, exact commands/exit status, worker/program/contracts/output
identities and generated media/captures stay outside Git. The application worker
and RAM-dump worker digests are unchanged. Existing full/stock, legacy/history/
seed media and completed physical observations are reused under their original
identities; no core/profile/source-emitter change to those programs occurred.
Physical FDCFIX execution was NOT_RUN at preparation; returned results below
are separate observations. G140, independent silicon, broader boundary/timing,
demo/media regression and clean handoff acceptance remain open.

## Returned fixed-address controls

[Returned analysis](../reports/m140_fdc_va_observations.md#returned-fixed-address-repetition-controls):
`hw-fdc-va-06` and `hw-fdc-va2-05`, labeled VA/V2/H and VA2/V2/H. Labels follow
the returned filenames/requested settings, not an independently encoded hardware
identity. Both images contain 160 valid rows with raw/text agreement, exact
control AF, preserved A/documented flags, framing, padding and loader recovery
marker. Both production content CLIs exit 0. All master files, exact COM and
boot/system region are preserved. No independent full-RAM or DIR/A> capture is
supplied by these returned images alone.

VA differs from present host in all 160 X/Y-only post results (80 SCF/80 CCF).
OR predicts 159/160. Repeat ID 6, seed 3, AF `0029`, SCF gives `0021`, while
its seven other repetitions give `0029`. This single outlier causes both the
repeat-variation and differing-per-seed-set flags; it is not two independent
findings or a deterministic seed/order rule. The post-F storage byte is at
`74D7h`; the common IUT/caller does not make result storage identical per row.
No defective cell, captured-register cause or physical Q value is demonstrated.
VA2 matches all 160 current-host results without seed/repeat variation.

Unchanged preparation evidence from
[5ed80876e909d3a6f4187a84c05ab8ba8e44a227](https://github.com/nakatamaho/vaeg/commit/5ed80876e909d3a6f4187a84c05ab8ba8e44a227)
is reused: no emulator, program, worker, test or content-contract changes in this
report-only intake. Snapshots, CLI commands/exit status, comparison and evaluated/
worker/program/contract/output identities remain out of tree. Initial local VA2
absence is retained as delivery status, then superseded by its actual capture;
it is not a failed guest run. Encoding/EOL/case/Markdown checks, 49 strict
milestone checks, audit and staged diff/scope checks exit 0.
Next work is independent capture/storage-control design, not an unchanged
physical replay or a speculative core/profile fix. G140 remains BLOCKED.

## Capture/stack/storage control prepared (FDCCAP)

The [new diagnostic](../../../tools/hardware/upd780/README.md#capturestackstorage-follow-up-fdccap)
addresses the remaining capture/storage confound. OR remains a strong candidate
within the earlier VA instruction sequence (159/160); this work does not mean
that no regularity is visible. Selected A=0 targets are not a complete A/F truth
table or a unique discriminator among every rule agreeing on those inputs.

96 rows cover AF=0028h/0029h, seed IDs 0/1/3, SCF/CCF and eight forward/reverse
passes. The shared sampler/IUT is now 4800h/4806h, not the older addresses; this
new executable does not hold all context constant versus FDCFIX. Three PUSH-AF
snapshots occupy different adjacent stack cells, then POP into HL/BC/DE before
comparisons change flags. NOP substitutes for the control IUT. Comparisons in
registers are reported separately from stores through code-RAM mailboxes, row
words, duplicate/inverted post-primary words and their immediate readbacks.
A final delayed scan checks row mirror relations before formatting.

The result region 7000h–7857h, including earlier storage location 74D7h, is filled
then read using six constant patterns (00/FF/55/AA/29/21) and low/high-address XOR.
Counts, first mismatch addresses, completion, immediate/triple flags and delayed
scan observations are retained. This is bounded result-RAM testing, not exhaustive
RAM/stack/code testing. Repeated PUSH AF, code-RAM mailboxes, CPU and later output
remain shared paths. No findings is not an independent original-register/Q read
or proof that the previous exception was caused by RAM.

Validation:

- **16/16 new direct content/source tests PASS**, including diagnostic findings
  preserved without treating complete data as integrity/conformance PASS.
- **8/8 focused FDC CTests PASS**, 3.27 s. Mapped tests verify common sampler/
  caller bytes, three exported words, raw/text rows and the patch-failure signal.
  Each of four further one-byte mutants starts from the same passing payload
  and exercises the actual RAM-error counter, register-triple comparison,
  immediate store readback or delayed mirror scan, checking the intended decoded
  findings/counts rather than an unrelated nonzero exit.
- ROM-backed **H and S integration PASS**, with a ROM set matching MAME's
  `pc88va2`: 96 persisted rows, no findings in the tested integrity contract,
  raw/text agreement, full reported readback-verified RAM recovery, original
  files/system preservation, 7936-byte DAT and DIR/A>. SDL dummy only.
- **Existing-DAT refusal PASS**, unchanged file set and refusal/DIR/prompt.
- **One-byte patch-verification fault safety PASS**: a digest-bound passing H
  fixture is reused; dedicated `M140_CAPTURE_PATCH_VERIFY` is visible and the
  fault image remains byte-identical. Not physical halt-state readout.

The COM is 5963 bytes. The private disk has 94208 free bytes and reserves 12288
for FDCCAP.DAT. Main-side raw collection is 2136 bytes, followed by original
full saved-RAM restoration/readback before file creation. Patch/timeout/recovery
errors retain unknown-state halt/reset policy. Older loaders and programs stay
unchanged. The test-only fault helper gains static program/case/message parameters
with old defaults; no production validator bypass or historical evidence upgrade.

Exact commands, worker/program/corpus/contracts, image hashes, initial/intermediate
results and generated media/captures remain out of tree. Unchanged application/
RAM-dump workers and old expensive evidence are retained under their original
identities. At preparation physical FDCCAP was **NOT_RUN**; returned data below
is separate. G140, independent silicon, absolute timing, broader boundary cases
and final regression/handoff acceptance remain open; no core/profile fix, merge,
release or cleanup follows.

## Returned FDCCAP controls

[Analysis](../reports/m140_fdc_va_observations.md#returned-fdccap-controls):
`hw-fdc-va-07` / `hw-fdc-va2-06`, filename/request labels VA/V2/H and VA2/V2/H,
not independently encoded hardware identities. Both have 96 complete rows,
production CLI exit 0, exact control AF, raw/text row agreement, preserved A/
documented flags, framing/padding/recovery marker, all master files/exact COM
and boot/system region preserved. No independent physical full-RAM or final
DIR/A> capture is supplied by these images alone.

Both have zero RAM-pattern mismatches (`7Fh` completion), zero immediate/triple
status, equal control/post triples, correct duplicates/inverses and zero delayed
scan discrepancies. The old 74D7h cell also contains `29h` as the new row 55
control-C low byte. These observations do not show a defective cell and do not
prove exhaustive RAM/stack/code integrity or explain the old outlier.

VA differs from host in all 96 X/Y-only results. OR predicts 56/96; 24 SCF plus
16 CCF rows lie outside both whole-instruction A-only and OR. For AF0029 SCF,
all 24 results are 0021; CCF gives 0030 sixteen times and 0038 eight times.
Each of three seeds varies across repeats for CCF, but all three exported words
agree on each row. Both directions contain both CCF results, eight/four each;
seed or direction alone is not a deterministic rule. AF0028 gives 0029 for both
operations throughout. VA2 matches current host in all 96 without variation.
Earlier FDCFIX's 159/160 OR fit is retained, not generalized across changed
executable/context; 39 of 96 overlapping VA descriptors change value, VA2 zero.

This report-only intake reuses unchanged program/worker/contracts and preparation
checks from 62e839c1dc714a0d6c53c7770b6b37660d88effd. Private commands, exit status,
snapshots and worker/program/corpus/contract/output identities remain out of tree.
Encoding/EOL/case/Markdown, 49 strict milestone checks, audit and staged scope/
diff checks pass. Next is agent-side shared export/context analysis, not an
unchanged hardware rerun or speculative core rule. G140 remains BLOCKED.

## G140 acceptance

Human acceptance is required after local QA, not just a worker exit or screenshot.
Collect actual FDC full-group/AF records and verified recovery; establish the
independent physical reference and relocation/Zilog controls; classify incomplete,
changed or unrun coverage explicitly. Timing uses bounded RAM loops with empty
and branch controls, multiple N/repetitions and nonuniform-VRTC quantization
bounds. Raw edge counts, host wall time and modeled T states are not physical
MHz. Physical H/S and CPU-speed observer coupling remain separate requirements.
Additional boundary probes and independent taken/untaken timing cases remain
open; no new NEC rule or upstream implementation is currently established.
The standard demo/media regression and clean static MinGW handoff obligations
still apply before the relevant implementation gate. No merge, release or
cleanup is authorized by this task reassignment.
