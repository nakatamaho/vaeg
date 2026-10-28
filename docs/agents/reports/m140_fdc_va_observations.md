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
below, followed by original-VA/V2/S and V2/H results. VA2/V2/H remains pending.

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
recorded below; VA2/H follow-up is still absent. No NEC-specific rule, hidden-Q
root cause, mode-dependent FDC clock or absolute MHz is established. No backend
change follows.

Current physical matrix:

| Machine/setting | Legacy suite | Controlled history suite |
| --- | --- | --- |
| VA / V2 / H | Complete; 35 AF input discrepancies | Both 512/512; byte-identical to VA/S history logs, including 84 FDCHIST post-AF differences |
| VA / V2 / S | Not collected | FDCHIST 512/512 with 84 post-AF differences; FDCFLAG 512/512 matches mapped host |
| VA2 / V2 / H | Not collected | Pending, not inferred from renamed file |
| VA2 / V2 / S | Complete; zero AF discrepancies | FDCHIST 512/512 + FDCFLAG 512/512; all mapped-host rows match |

Use a fresh copy of the same r2 history image for VA2/V2/H next.
The existing full-ZEX and original-VA controlled histories need not be repeated.
Original-VA controlled S/H flags now compare equal; timing reference/calibration,
VA2 controlled H/S and same-machine legacy timing comparisons remain open; all hashes/private filenames stay
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
controlled follow-up below is now received; VA2/H remains the next collection.
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
a machine identity encoded in the deterministic logs. VA2/V2/H remains absent.
No file named H in the earlier corrected S submission supplies that missing
case. Next collection is only the two new programs on VA2/V2/H, using a fresh
copy; repeated original-VA or full-ZEX runs are not needed for this comparison.

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
