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

# M140 — returned physical VA FDC observations

## Scope and identities

Neutral observation ID: `hw-fdc-va-02`. The maintainer identifies the target as
physical **VA, V2 mode, H setting**, subsequently confirmed by the maintainer
as `VA V2 H`. This means a VA in V2/H, not a VA2 machine. Firmware/board
revision, elapsed wall time and an independently approved VRTC reference have
not been supplied. VA2 and S-setting coverage are not established. This is
operator-supplied configuration metadata, not an independent ROM-set comparison
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
is now prepared and software-validated; its physical results remain pending.

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
