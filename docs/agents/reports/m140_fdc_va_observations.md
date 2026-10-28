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

# M140 — returned physical VA/VA2 FDC observations

## Scope and identities

Neutral observation ID: `hw-fdc-va-02`. The maintainer identifies the target as
physical **VA, V2 mode, H setting**, subsequently confirmed by the maintainer
as `VA V2 H`. This means a VA in V2/H, not a VA2 machine. Firmware/board
revision, elapsed wall time and an independently approved VRTC reference have
not been supplied. This initial case does not establish VA2 or S coverage;
separate VA2/V2/S follow-ups are recorded below. This is operator-supplied
configuration metadata, not an independent ROM-set comparison
or a measurement of the FDC clock.

The returned media contains all three complete diagnostic logs. All original
files, including the three COM programs, are byte-identical to the handoff;
the boot/system sector region is also unchanged. The handoff's generated
programs are byte-identical across the M139/M140 administrative split and
external-patch extraction. Private input, media/log hashes, raw records and
captures remain outside Git. An out-of-tree intake manifest binds the received
snapshot, normalized copy, extracted files, reader source digest, commands,
exit statuses and result reports. The user-supplied file remains untouched.

The same-image host control is the preserved mapped production uPD780 run
recorded in the [pre-split report](m140_fdc_pre_split_record.md), not a fresh
run or independent physical Zilog oracle. Its worker SHA-256 is
`4f8a14266448b76d82490d87dd48cc4733f42e8b46ff77b135647f9f49541fce`;
the worker still matches, and no emulator execution source changes here.

## Results

| Case | Content validation | Comparison/classification |
| --- | --- | --- |
| NEG-only smoke | 1/1 group, completion and recovery markers | CRC agrees with mapped host and stock expectation |
| Full relocated ZEX | 67/67 groups, exact names/order, completion and recovery | **67/67 actual CRCs agree with the identical-image mapped host** |
| Full ZEX versus stock-address expectations | 29/67 differ | The same 29 differences exist in the mapped host; not evidence of NEC differences |
| AF matrix | 256/256 requested pairs, valid register/result structure | 35 distinct input pairs differ from the current host arithmetic expectation |
| Timing | 67/67 rows, correct case/repeat/N set | Positive/increasing endpoint slopes; bounded middle residual and repeat checks PASS |
| Persistence/recovery | All logs have the recovery marker; files persisted on returned media | Controller-reported verified RAM restoration, not an independent physical RAM dump |
| Original media contents | All original files and boot/system region preserved | PASS for byte comparison; not a post-run guest DIR screenshot |

All three reader CLI invocations exit 0 with `COMPLETE_OBSERVATIONS`, not a
silicon-conformance verdict. Raw edge measurements have **no physical MHz or
seconds conversion** without an independently approved reference. There is only
one operator-confirmed main V2/H setting; an S-setting comparison and physical
H/S observer-coupling coverage remain open. Do not substitute host model cycles or nominal clock for this.

### AF discrepancy is restricted to SCF/CCF undocumented bits

Per-operation comparison against the current host expectation:

| Operation after POP AF | Differing pairs out of 256 |
| --- | ---: |
| POP AF readback | 0 |
| BIT 0,A | 0 |
| AND 55h | 0 |
| SCF | 34 |
| CCF | 35 |

These are 35 unique input pairs, not 69 unique cases. Every differing bit is
within F bits 3/5 (X/Y); A and the documented flag bits agree. There is no
justification for changing BIT's H behavior or AF writable-bit policy here.

The diagnostic reloads AF before each operation, but its output wrappers and
monitor transport do not explicitly control hidden Q/instruction history.
The current arithmetic helper does not model that history. Thus this is a
**measured discrepancy with the current host baseline**, not yet a demonstrated
NEC-specific rule or an independently established Zilog failure. A simple
unconditional OR of old-F/A X/Y is also not a rule fitting the entire capture.
Do not change the emulator or globally propagate a new FDC profile to non-VA
main CPUs based on these records alone. Next work is a minimal history-controlled
SCF/CCF probe and independent Zilog control, not another unchanged full ZEX run.
The [FDCHIST/FDCFLAG r2 disk](../tasks/M140_fdc_upd780_conformance.md#history-controlled-follow-up-disk-r2)
was prepared and software-validated. VA2/V2/S physical results are now recorded
below, followed by original-VA/V2/S and V2/H results and a newly supplied
VA2/V2/H capture. All four controlled-history settings are now collected.

## VA2 / V2 / S follow-ups

The maintainer supplies two distinct captures, both explicitly identified as
**VA2 hardware, V2 mode, S setting**:

| Neutral case | Received coverage | Result |
| --- | --- | --- |
| `hw-fdc-va2-01` | Original NEG/full-ZEX/AF/timing suite | NEG 1/1 and full ZEX 67/67 complete; actual full CRCs match both the mapped host and VA/V2/H exactly; 256 AF pairs and 67 timing rows validate, with zero current-host AF differences |
| `hw-fdc-va2-02` | New FDCHIST/FDCFLAG suite | Both 512-record logs complete, with zero mapped-host differences in requested AF, control pre-AF or post-AF across all 1024 rows |

The second capture is not a duplicate of the first; it contains the new
history logs rather than the legacy logs. A temporary H-looking filename was
explicitly corrected by the maintainer to S before intake. **Neither capture
is evidence for VA2/V2/H.** Do not infer settings from filenames or reclassify
these as two H/S measurements.

All original CP/M input files and the boot/system region are preserved in both
captures. The first contains the three exact legacy COMs; two supplementary
text files do not match the complete legacy handoff, so not every packaged
file is claimed unchanged. The second matches every file of the intended r2
history package, including both exact COMs. Every received diagnostic log
passes completion/recovery validation. The corrected S snapshot, normalized
copy, raw logs, per-program comparisons and private identity manifests are
retained outside Git; the supplied files are not modified.

The VA/V2/H legacy probe had 35 differing AF inputs; the VA2/V2/S legacy probe
has zero. This is a **cross-machine and cross-setting** comparison, not an
isolated machine or H/S effect. The VA2/S controlled histories also match the
current mapped host. Subsequent original-VA S/H controlled-history cases are
recorded below, followed by a separate newly supplied VA2/H capture. No
NEC-specific rule, hidden-Q root cause, mode-dependent FDC clock or absolute
MHz is established. No backend change follows.

Current physical matrix:

| Machine/setting | Legacy suite | Controlled history suite |
| --- | --- | --- |
| VA / V2 / H | Complete; 35 AF input discrepancies | Both 512/512; byte-identical to VA/S history logs, including 84 FDCHIST post-AF differences |
| VA / V2 / S | Not collected | FDCHIST 512/512 with 84 post-AF differences; FDCFLAG 512/512 matches mapped host |
| VA2 / V2 / H | Not collected | Both 512/512; all mapped-host rows match and logs are byte-identical to VA2/S |
| VA2 / V2 / S | Complete; zero AF discrepancies | FDCHIST 512/512 + FDCFLAG 512/512; all mapped-host rows match |

All four controlled-history settings are now collected; no unchanged rerun
of these histories or full ZEX is needed. Both reported machines' controlled
S/H flags compare equal within that machine. Timing reference/calibration,
same-machine legacy timing comparisons and independent silicon controls remain open; all hashes/private filenames stay
in the out-of-tree evidence, not this report. No emulator or validator source
changes in this report-only follow-up, so unchanged expensive software runs
are reused rather than repeated.

## Original VA / V2 / S controlled-history follow-up

Neutral case `hw-fdc-va-03` is assigned **original VA / V2 / S** from the
maintainer-supplied filename. Both exact r2 COMs, all original CP/M input files
and the boot/system region match their source. Two supplementary text files
do not match the complete r2 package, so full-package identity is not claimed.
Both logs pass completion/recovery and all documented precondition checks;
the received image remains unchanged and raw evidence stays outside Git.

| Comparison against mapped host and VA2/V2/S | FDCHIST | FDCFLAG |
| --- | ---: | ---: |
| Complete rows | 512 | 512 |
| Rows with post-AF differences | 84 (42 SCF, 42 CCF) | 0 |
| Rows with pre-AF differences | 56 | 0 |
| Rows differing in either column | 140 | 0 |

Every post-AF differing bit is X/Y (F bits 3/5); A and the documented flags
match. The 84 differences occur only after history 0 (no extra instruction),
1 (NOP) and 2 (LD A,A), 28 rows per history. Histories 3 through 7 have no
post-AF differences. The 56 pre-AF differences occur in the separate control
spans for histories 6 (SCF) and 7 (CCF), 28 rows per history. A control result
is shared by the two operation rows: 56 rows are **28 distinct control spans**,
not 56 independent repetitions. Pre- and post-differing row sets are disjoint.
FDCFLAG is byte-identical to the VA2/S log; all its control/result rows also
match the mapped host.

Thus **same-labeled V2/S configurations now show a cross-machine observation
difference** with identical programs. It cannot be dismissed solely as an H
versus S mismatch. This does not identify the physical CPU variant, firmware
revision, hidden-Q behavior or a universal NEC rule. The actual internal Q
value was not measured; the seed provides a fixed instruction sequence, not a
proven Q reset. An unconditional OR of old-F/A X/Y does not fit all observations,
even in the restricted FDCHIST matrix. Do not implement that shortcut.

The earlier VA/H legacy difference count (35 distinct requested AF inputs)
and this count (84 operation/history rows in a different matrix) are not the
same unit of coverage and must not be compared as an increase. The VA/H
controlled follow-up and the separate VA2/H follow-up below are now received.
Neither full ZEX nor new emulator builds are needed. The current evidence
supports a measured difference, not a tested backend fix or G140 acceptance. This follow-up changes reports only and reuses unchanged
software/host execution evidence.

## Original VA / V2 / H controlled-history follow-up

Case `hw-fdc-va-04` is explicitly identified by the maintainer as original
VA / V2 / H. Both programs produce complete 512-row logs. Each entire persisted
log is **byte-identical to its original-VA/V2/S counterpart**, not just equal
in an aggregate discrepancy count. Thus FDCHIST again has 84 post-AF X/Y
rows (42 SCF, 42 CCF), 56 pre-AF rows and 140 differing rows in total against
mapped host; FDCFLAG has zero differences. Program bytes, all original CP/M
input files and system region match; two supplementary package texts differ,
so full-package identity is not asserted. The supplied image is untouched.

This establishes equal observed controlled flag results for operator-labeled
S/H captures on the same reported VA. It is **not a timing comparison**, a
physical clock measurement or a general claim that H/S cannot affect other
observer paths. Configuration/execution assignment is operator metadata, not
a machine identity encoded in the deterministic logs. The separate VA2/V2/H
follow-up below is now received; the earlier corrected S submission is still
S and is not retroactively relabeled. Repeated original-VA or full-ZEX runs
are not needed for this comparison.

## VA2 / V2 / H controlled-history follow-up and complete matrix

Case `hw-fdc-va2-03` is newly supplied and explicitly identified as VA2 / V2 / H.
It is saved as a separate snapshot, without relabeling the earlier corrected
S capture that temporarily used an H-looking name. Both 512-row logs pass
coverage, precondition, completion and recovery checks. Their entire persisted
bytes equal the respective VA2/V2/S logs. Requested/control/result AF all match
the identical-image mapped host; against original-VA/H there are 140 differing
FDCHIST rows (56 pre-AF only, 84 post-AF only), and zero FDCFLAG differences.

Both exact COMs, all original CP/M input files and the boot/system region are
preserved. Two supplementary package texts differ, so complete-package identity
is not asserted. The received file is read-only to the analysis; snapshots,
normalization copies, raw records and identity/command/output manifests remain
outside Git. Configuration/execution assignment is operator metadata, not a
nonce or hardware identity encoded in the deterministic logs.

The controlled matrix is now complete for both reported machines in V2/H
and V2/S, 512 rows per program per setting (4096 operation rows in total).
Each machine's two persisted logs match between H and S. The observed difference
tracks the reported machine in these samples, not the H/S label. This does not
identify a CPU revision, firmware cause or universal NEC behavior. It does not
validate timing, observer frequency, V1, independent Zilog silicon or any new
backend correction. Do not rerun unchanged histories or full ZEX merely to
reconfirm this complete matrix. Next work is agent-side discrepancy reduction,
explicit seed/history reasoning and independent controls before proposing a
profile or upstream fix; physical timing/reference work remains separate.

## Offline reduction and separate software Q controls

`tools/hardware/upd780/reduction.py` reuses all eight controlled logs without
executing the emulator or requesting another physical run. It validates each
complete log before comparing **complete AF**, preserving A even when nonzero.
Differences remain observations; its completion status is not conformance PASS.

The separate software reference is SingleStepTests/z80 at commit
[`ebe1875d48f374bcfd4b505d8eb8ee751568b5f7`](https://github.com/SingleStepTests/z80/tree/ebe1875d48f374bcfd4b505d8eb8ee751568b5f7),
files `v1/37.json` and `v1/3f.json`, 1000 records each. The tool pins their public
SHA-256 values and checks only the A/F/Q slice. Its
[README](https://github.com/SingleStepTests/z80/blob/ebe1875d48f374bcfd4b505d8eb8ee751568b5f7/README.MD)
states that the tests were generated from a translated, corrected Ares core;
these are **software-generated references, not measured Zilog silicon**. They
are independent of this fork's arithmetic helper, not an independent physical
oracle. Downloads, license and acquisition manifests remain out of tree; no
third-party code or corpus is newly vendored.

All 2000 records agree with `XY = ((F XOR Q) OR A) AND 28h`, with initial
Q restricted to zero or F in this corpus, A preserved and final Q equal to final
F. The current core's A-only X/Y expression differs on **229 SCF + 219 CCF =
448** reference records. This is an algebraic comparison of the A/F/Q contract,
not 2000 new worker executions, bus/cycle validation or a full-core SST result.
It does not authorize replacing the measured NEC behavior with the corpus model.

As a separate source-level cross-check, MAME 0.280, commit
[`ec9abd86c6c9029f67e9cf4908ef5426b78d3eab`](https://github.com/mamedev/mame/tree/ec9abd86c6c9029f67e9cf4908ef5426b78d3eab),
uses per-instruction Q/QT in
[`z80.lst`](https://github.com/mamedev/mame/blob/ec9abd86c6c9029f67e9cf4908ef5426b78d3eab/src/devices/cpu/z80/z80.lst).
Its POP AF/set_f path does not classify POP AF as an arithmetic flag writer.
For these straight-line spans, that suggests OR after histories 0–2, and A-only
after histories 3–7. The tool reports this as a **software history candidate**,
not a measurement of physical Q. MAME itself was not built or executed here.

Per setting, using the separately captured control AF as the input:

| Reported machine | Program | A-only post mismatches | Unconditional-OR post mismatches | Software-history post / pre mismatches | Post results outside both A-only and OR |
|---|---|---:|---:|---:|---:|
| VA, either H or S | FDCHIST | 84 | 62 | 6 / 4 | 6 |
| VA, either H or S | FDCFLAG | 0 | 24 | 24 / 16 | 0 |
| VA2, either H or S | FDCHIST | 0 | 84 | 84 / 56 | 0 |
| VA2, either H or S | FDCFLAG | 0 | 24 | 24 / 16 | 0 |

Pre counts are operation rows sharing control spans, not additional independent
executions. In particular, the software history candidate improves the VA
FDCHIST comparison but **does not explain FDCFLAG**; it is not a fitted NEC rule.

The six VA post counterexamples reduce to one requested/control AF, `0029`,
with the three predecessors none, NOP and LD A,A, each followed by SCF or CCF:

| Operation | Measured VA AF | A-only prediction | OR prediction |
|---|---:|---:|---:|
| SCF | `0021` | `0001` | `0029` |
| CCF | `0030` | `0010` | `0038` |

These are minimal derived observations, not a raw log reproduction. They recur
in the retained H and S cases. Here A has neither X/Y bit and old F has both;
VA retains Y but drops X. Even choosing A-only versus OR separately for every
instruction cannot produce that partial result. This rules out that two-choice
shortcut, **not every possible bitwise/internal-Q model**. Physical Q was never
read, and a richer latch, firmware, transport or harness cause is not excluded.
Do not infer a silicon revision, a universal NEC rule or a CPU correction.

Next useful new work would hold target AF fixed while independently varying
last flag-writer seed X/Y and use a distinct result-readback control; the current
fixed seed and common output path do not supply those controls. Such a bounded,
distinct probe must be designed and software-validated before asking for new
hardware execution. The existing four-setting histories/full ZEX need no rerun.
Timing/reference work remains separate.

Validation: **46/46 direct unit tests PASS**, **2/2 focused CTest PASS**. Tests
include nonzero-A preservation, the partial-X/Y reduction, reference identity,
and passing-fixture single-mutation content negatives with exact stable codes.
The production analysis CLI checks the pinned 2000 reference rows and all 4096
physical operation rows, exit 0. An initial exploratory comparison accidentally
compared complete AF against F-only, producing invalid counts for nonzero A;
that output is retained as invalid and superseded, not used in this table. The
corrected tool preserves A and has an explicit nonzero-A regression. Initial
reference-download 404s and resolved pinned identities are also retained.
No worker, probe, loader, CPU profile or vendored implementation is changed;
unchanged expensive execution evidence is reused. Raw inputs, exact commands,
contract/tool identities and generated analysis stay outside Git.

## Seed/readback control preparation record

The [FDCSEED follow-up](../tasks/M140_fdc_upd780_conformance.md#bounded-seed--raw-readback-probe-fdcseed)
is now implemented and software-validated: 80 rows vary target AF and flag-writer
seed, retaining the original XOR-A anchor. An OR-A zero seed separates opcode
history from the same nominal arithmetic flags, with three additional seed X/Y
patterns. No claim is made that those instructions establish a physical Q value.

All control/IUT snapshots precede formatting. The FDC's ASCII is compared with
a raw table read through monitor 0Bh before full verified RAM recovery and file
save. This controls formatting/readback paths sharing the same capture RAM,
not an independent second silicon observation. No root cause is established by
software success. Mapped execution and ROM-backed H/S persistence/refusal checks
pass, and old distribution/history disks rebuild byte-identically. At this
preparation stage physical execution of the new probe was NOT_RUN; returned
results are recorded separately below. No earlier capture is relabeled or
promoted. Do not repeat unchanged full ZEX or the completed history matrix.

## Returned FDCSEED seed/readback controls

New cases `hw-fdc-va-05` and `hw-fdc-va2-04` were returned under the labels
**VA/V2/H** and **VA2/V2/H**, consistent with the requested initial settings.
These are the labels supplied through the returned filenames, not independently
encoded machine/mode/speed identities. No firmware or silicon revision is
established by those names. Captures, normalization copies, raw mixed-format
DAT files, exact commands and input/program/contract/output identities remain
outside Git; the received images are unmodified.

Both DAT files contain **80 complete operation rows**, all expected descriptors,
exact target/control AF, preserved A/documented flags, valid binary framing and
restoration marker. FDC ASCII and subsequent monitor-0Bh binary readback agree
on every result. Each complete master file (including program and help), the
exact generated COM, and the boot/system region are preserved. Both production
`seed.py check` commands exit 0 with `COMPLETE_OBSERVATIONS`. This is completion
of the diagnostic contract, not physical CPU conformance PASS. The restoration
marker reports the loader's full readback-verified recovery; there is no separate
physical full-RAM dump or captured post-return DIR/A> in these submissions.

| Returned label | Rows | Post-AF differences from present host | Seed-dependent target/operation groups |
|---|---:|---:|---:|
| VA/V2/H | 80 | 30 (15 SCF, 15 CCF), only X/Y | 1 |
| VA2/V2/H | 80 | 0 | 0 |

VA has six host-differing operation rows for each of the five seed recipes.
For the XOR-A anchor, all eight rows overlapping the earlier H controlled
capture agree in requested/control/result AF, on each reported machine. The
new program thus retains the earlier reduced anchor result within that overlap;
it does not retroactively extend the coverage of the earlier capture.

The VA seed-dependent group is **CCF with target/control AF `0029`**:

| Seed recipe | VA post-AF | VA2 post-AF |
|---|---:|---:|
| `XOR A` | `0030` | `0010` |
| `LD A,0; OR A` | `0038` | `0010` |
| `LD A,08h; OR A` | `0030` | `0010` |
| `LD A,20h; OR A` | `0030` | `0010` |
| `LD A,28h; OR A` | `0030` | `0010` |

VA's SCF at that AF yields `0021` for all five recipes; VA2 yields `0001`
throughout. XOR-A and zero OR-A have the same nominal arithmetic A/F result
before loading target AF, yet that one VA CCF row differs. This is one observed
group, **not a demonstrated deterministic seed rule or a physical Q read**.
Each straight-line seed/IUT span has its own code address and preceding context;
this run does not isolate opcode history from address/context or repeatability.

Because the raw bytes and ASCII agree, a solely FDC hexadecimal-rendering
explanation does not account for the observed values, assuming the supported
monitor readback contract. Both paths still share captured RAM and the device;
they do not independently validate the original AF capture, eliminate common
capture/RAM faults, establish firmware identity or supply a Zilog silicon oracle.
No CPU/profile correction, general NEC rule or G140 acceptance follows.

Next agent-side work is an address-controlled, repeated minimal experiment
before proposing further physical collection: avoid confounding seed/opcode
changes with different IUT locations. Do not request an unchanged full ZEX,
four-setting history or FDCSEED rerun merely to reproduce these completed logs.
Timing/reference work is separate. Software execution evidence from the
unchanged preparation commit is reused under the same program, worker and
content-contract identities; new physical intake is not a new software test run.

## Returned-image intake correction

The first strict reader attempt failed `M139_D88_TRACKS`; that original failure
and capture are retained. The D88 stores 40 cylinders/two heads with active
track-table slots `4*C+H`, but sector C/H remain logical. The builder expects
contiguous `2*C+H` slots. This is a representation difference, not damaged CP/M
files or an instruction-execution failure.

[26108cb2c3701b8511ba8a76064cb1db187bf74d](https://github.com/nakatamaho/vaeg/commit/26108cb2c3701b8511ba8a76064cb1db187bf74d)
adds a content-only readback normalization for exactly that 2D layout. It
validates the closed active-slot set, strict sector geometry/bounds/order and
normal status, and changes only the track table in an in-memory copy. Every
sector header and payload remains byte-identical. The writable-media builder
stays strict. Other malformed layouts remain errors; no general bypass exists.
Separate normalized and production-reader reports preserve the initial FAIL.

Verification: **24/24 unit tests PASS**, **2/2 focused CTest PASS** (2.78 s).
A passing synthetic capture is checked before each single-mutation negative:
extra active track, wrong media type, wrong sector ID, or error status, each
with its intended stable error code. The source-preserving round trip composes
normalization with actual CP/M extraction. Private-input readback then confirms
the production reader extracts the same three validated logs as the independently
checked header-only normalized copy. The initial synthetic fixture's wrong
2DD media default and its failed test outputs are retained; it was corrected
to the intended 2D fixture, not accommodated by relaxing the production rule.

## Gate status

[G140](../tasks/M140_fdc_upd780_conformance.md) remains BLOCKED: SCF/CCF history
controls, independent Zilog controls, reference/calibrated timing, physical H/S,
further boundary/timing cases and the required regression/handoff/acceptance
remain open. No NEC backend correction or upstream submission is established.
No private assets are uploaded to hosted CI; unchanged expensive runs are reused.
