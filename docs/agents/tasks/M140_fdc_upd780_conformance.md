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
files/programs and boot/system sectors are preserved. **G140 BLOCKED**:
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
Current physical VA2 results and physical execution of these new probes remain
pending, not PASS.

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
