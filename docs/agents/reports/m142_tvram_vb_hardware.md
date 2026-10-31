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
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# M142 — returned TVRAM/VB hardware measurements

**Ten hardware result screens received and transcribed; cause OPEN;
correction NOT_IMPLEMENTED; G142 NOT_READY.** This follow-up supersedes the
checkpoint's lack-of-hardware-results blocker, not its retained historical
measurements or the earlier gate acceptances.

Task: [M142](../tasks/M142_runtime_portability.md). Related:
[#75](https://github.com/nakatamaho/vaeg/issues/75),
[historical issue table](m142_runtime_progress.md#d--75-published-screen-5-measurements-not-a-new-hardware-run),
[source-built diagnostic kit](../../../tools/hardware/tvram_vb/README.md).

## Receipt and interpretation boundaries

All ten photographs show the variant/program identifier, a ROM-derived model
label and **5 DONE**. All A–X rows and all mean/minimum/maximum/SD columns are
readable. In all 240 rows, minimum = mean = maximum and SD = 0.0. The tables
below retain every mean; the other three columns are consequently recoverable
without discarding information. These were image transcriptions, not OCR or
an automated physical timing validation.

- `tvram-vb-hw-01`–`05`: **VA** label, respectively TSPSPR, V032A, V032E,
  V128A, V128E.
- `tvram-vb-hw-06`–`10`: **VA2/3** label, respectively TSPSPR, V032A,
  V032E, V128A, **V256E**. The last screen says V256E, not V128E; it is
  retained as a 256-byte measurement, not silently relabeled or rejected.
  There is no VA2/3 V128E result in this receipt.
- VA2 and VA3 cannot be distinguished by this ROM-word check. The labels do
  not attest absolute CPU frequency, monitor timing or physical board details.
- The intended commands have argument `1`; the DONE heading does not preserve
  the pass count, SYNC parameters or key history. SD = 0 does not independently
  prove repeated-run stability or a pass count. Photographs identify the
  programs, not cryptographic readback of the executed COMs.
- Final kit variants autostart screen 5 in the source; stock TSPSPR requires
  four SPACE advances. These completed photographs establish the correct
  result state, not why earlier SPACE operation was confusing. No retake,
  repeated baseline or additional hardware run is requested at this point.

Private originals are unchanged. Original/conversion bindings, raw captures,
commands, logs and transcription revisions remain in out-of-tree evidence.
Only independently written numerical analysis and neutral receipt identifiers
are published here.

## Complete hardware table — VA label

Q/R/W count the chunk size of their **column**, not a common byte rate.
S/X are the program's VB polling fractions in 1/1000-frame units; they are
not independent oscilloscope measurements or absolute CPU-clock calibration.
Other row names and numbers are retained as displayed.

| Row | TSPSPR (128B) | V032A (32B) | V032E (32B) | V128A (128B) | V128E (128B) |
|---|---:|---:|---:|---:|---:|
| A CPU ADD LOOP | 1741 | 1826 | 1826 | 1826 | 1826 |
| B RAM W MOVSW | 8767 | 8771 | 8771 | 8771 | 8771 |
| C G1 B WRITE | 1220 | 1220 | 1220 | 1220 | 1220 |
| D G1 W MOVSW | 7651 | 7659 | 7659 | 7659 | 7659 |
| E TV B WRITE OFF | 1826 | 1840 | 1836 | 1843 | 1836 |
| F TV B WRITE 31 | 1843 | 1825 | 1850 | 1846 | 1839 |
| G TV B READ OFF | 2129 | 1848 | 1814 | 1834 | 1828 |
| H TV B READ 31 | 1864 | 1851 | 1851 | 1831 | 1834 |
| I TV W MOVSW OFF | 3340 | 3339 | 3339 | 3339 | 3339 |
| J TV W MOVSW 31 | 3329 | 3329 | 3329 | 3329 | 3329 |
| K TV W ATTR 31 | 3330 | 3329 | 3328 | 3328 | 3328 |
| L TV W READ 31 | 3320 | 3318 | 3318 | 3318 | 3318 |
| M TV W STOSW 31 | 4262 | 4258 | 4257 | 4257 | 4256 |
| N TV W MOVSW 8 | 3334 | 3333 | 3333 | 3333 | 3333 |
| O TV W MOVSW 31M32 | 3316 | 3316 | 3316 | 3316 | 3316 |
| P TV W MOVSW 31C64 | 3334 | 3333 | 3333 | 3333 | 3333 |
| Q TV VB chunk 31 | 602 | 2104 | 2100 | 602 | 600 |
| R G1 VB chunk | 1442 | 3608 | 3600 | 1446 | 1448 |
| S VB 1/1000 FRM 31 | 20 | 20 | 20 | 20 | 20 |
| T SPRON CMDS 31 | 11784 | 11779 | 11778 | 11785 | 11779 |
| U TV W MOVSW S200 | 3252 | 3251 | 3251 | 3251 | 3251 |
| V TV B WRITE S200 | 1310 | 1306 | 1316 | 1306 | 1306 |
| W TV VB chunk S200 | 2407 | 7823 | 7800 | 2407 | 2400 |
| X VB 1/1000 S200 | 86 | 86 | 86 | 86 | 86 |

## Complete hardware table — VA2/3 label

| Row | TSPSPR (128B) | V032A (32B) | V032E (32B) | V128A (128B) | V256E (256B) |
|---|---:|---:|---:|---:|---:|
| A CPU ADD LOOP | 1755 | 1841 | 1841 | 1841 | 1841 |
| B RAM W MOVSW | 8827 | 8831 | 8831 | 8831 | 8831 |
| C G1 B WRITE | 1232 | 1232 | 1232 | 1232 | 1232 |
| D G1 W MOVSW | 7672 | 7676 | 7676 | 7676 | 7676 |
| E TV B WRITE OFF | 1186 | 1182 | 1171 | 1196 | 1185 |
| F TV B WRITE 31 | 1221 | 1246 | 1267 | 1253 | 1264 |
| G TV B READ OFF | 1215 | 1223 | 1212 | 1220 | 1206 |
| H TV B READ 31 | 1221 | 1208 | 1208 | 1197 | 1197 |
| I TV W MOVSW OFF | 3368 | 3368 | 3368 | 3368 | 3368 |
| J TV W MOVSW 31 | 3359 | 3359 | 3359 | 3359 | 3359 |
| K TV W ATTR 31 | 3360 | 3359 | 3359 | 3359 | 3359 |
| L TV W READ 31 | 3349 | 3348 | 3348 | 3348 | 3348 |
| M TV W STOSW 31 | 4286 | 4281 | 4281 | 4281 | 4280 |
| N TV W MOVSW 8 | 3365 | 3365 | 3365 | 3365 | 3365 |
| O TV W MOVSW 31M32 | 3347 | 3347 | 3347 | 3347 | 3347 |
| P TV W MOVSW 31C64 | 3364 | 3364 | 3364 | 3364 | 3364 |
| Q TV VB chunk 31 | 602 | 2107 | 2107 | 602 | 301 |
| R G1 VB chunk | 1505 | 3897 | 3884 | 1505 | 903 |
| S VB 1/1000 FRM 31 | 20 | 20 | 20 | 20 | 20 |
| T SPRON CMDS 31 | 10835 | 10832 | 10829 | 10825 | 10823 |
| U TV W MOVSW S200 | 3282 | 3282 | 3282 | 3282 | 3282 |
| V TV B WRITE S200 | 1315 | 1290 | 1290 | 1311 | 1304 |
| W TV VB chunk S200 | 2407 | 7825 | 7803 | 2407 | 1200 |
| X VB 1/1000 S200 | 86 | 86 | 86 | 86 | 86 |

## Fresh, unchanged-worker software comparison

Evaluated source:
[`30bceba24c102eb3ee60bdfcf90e0e4b93836b3c`](https://github.com/nakatamaho/vaeg/commit/30bceba24c102eb3ee60bdfcf90e0e4b93836b3c).
Worker SHA-256:
`8ee71b64e92175b4e965902b4f29bb7112d3318a949ec6f997524a57ca7358be`.
This is the existing native diagnostic worker, recording ON, local encoders
OFF, shaders OFF, **SDL dummy**. It is not Windows execution or final-client
presentation coverage. No emulator code, wait, phase or guest payload was
changed to obtain this comparison.

Ten fresh sessions use writable copies of the existing kit disk, isolated
configuration/HOME, no other mounted media, nominal standard 8 MHz and 15 kHz.
The parameterized command is:

```sh
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy HOME="$CASE/home" \
  "$WORKER" --cfg "$CASE/vaeg.cfg" --no-bkupmem --roms "$ROM_DIR" \
  --model "$MODEL" --fdd1 "$CASE/fixture.d88" --fdd2 none \
  --sasi1 none --sasi2 none --scsi0 none --scsi1 none --scsi2 none \
  --scsi3 none --scsi4 none --scsi5 none --scsi6 none \
  --nowait --frameskip full --headless-input-script "$CASE/input.txt" \
  --screenshot "12000:$CASE/final.png"
```

The input types the matching program with argument `1`. Stock cases then
wait 300 frames and advance four screens, 120 frames apart; variants receive
no SPACE. The script uses a SPACE followed by an ignored dot for stock,
because the input parser trims trailing whitespace. All ten workers exit 0;
all ten captures were separately inspected for matching program/model and
DONE. Q/R/S/W/X were manually transcribed from the original captures with
magnified/scan-gap-removed inspection copies. These are observed guest
results, **not a new automated readout-contract PASS**. Exact commands,
input/worker/ROM/corpus/runner identities and output digests are retained
locally. This is not a rerun or upgrade of the earlier full CTest suite.

In R/S/W/X cells, the notation is **hardware / vaeg**, not a quotient.

| Receipt | Model label | Program | B/chunk | HW Q | vaeg Q | R: HW / vaeg | S: HW / vaeg | W: HW / vaeg | X: HW / vaeg |
|---|---|---|---:|---:|---:|---|---|---|---|
| tvram-vb-hw-01 | VA | TSPSPR | 128 | 602 | 2408 | 1442 / 5113 | 20 / 85 | 2407 / 2407 | 86 / 87 |
| tvram-vb-hw-02 | VA | V032A | 32 | 2104 | 7826 | 3608 / 14423 | 20 / 85 | 7823 / 7823 | 86 / 87 |
| tvram-vb-hw-03 | VA | V032E | 32 | 2100 | 7826 | 3600 / 14423 | 20 / 85 | 7800 / 7823 | 86 / 87 |
| tvram-vb-hw-04 | VA | V128A | 128 | 602 | 2408 | 1446 / 4816 | 20 / 85 | 2407 / 2407 | 86 / 87 |
| tvram-vb-hw-05 | VA | V128E | 128 | 600 | 2408 | 1448 / 4816 | 20 / 85 | 2400 / 2407 | 86 / 87 |
| tvram-vb-hw-06 | VA2/3 | TSPSPR | 128 | 602 | 875 | 1505 / 1505 | 20 / 21 | 2407 / 2407 | 86 / 87 |
| tvram-vb-hw-07 | VA2/3 | V032A | 32 | 2107 | 2124 | 3897 / 3913 | 20 / 21 | 7825 / 8174 | 86 / 87 |
| tvram-vb-hw-08 | VA2/3 | V032E | 32 | 2107 | 2104 | 3884 / 3913 | 20 / 21 | 7803 / 8180 | 86 / 87 |
| tvram-vb-hw-09 | VA2/3 | V128A | 128 | 602 | 901 | 1505 / 1505 | 20 / 21 | 2407 / 2407 | 86 / 87 |
| tvram-vb-hw-10 | VA2/3 | V256E | 256 | 301 | 602 | 903 / 903 | 20 / 21 | 1200 / 1200 | 86 / 87 |

## Findings and unresolved cause

1. **The tested entry-policy change does not remove the discrepancy.**
   Physical 32-byte Q is 2104 → 2100 on the VA label and 2107 → 2107 on
   VA2/3 for A → E. These observations do not demonstrate a large improvement
   from rejecting an already active VB. They are not an exhaustive phase test.
2. **The VA2/3 Q ratio depends strongly on chunk size.** Stock 128-byte Q is
   602 / 875 (hardware / vaeg). The matched 32-byte controls are 2107 / 2124
   and 2107 / 2104, whereas V128A is 602 / 901 and V256E is 301 / 602.
   A single 1.45-times TVRAM slowdown is not an established mechanism. Stock
   and V128A have different code layout; retain both rather than pretending
   the A variant is byte-identical to stock.
3. **Completed-chunk quantization is relevant, but its underlying timing
   error is not yet identified.** The source counts a whole chunk before
   checking VB again. With about 300 frame intervals, 602 versus 901 is
   approximately two versus three chunks per interval, and 301 versus 602
   approximately one versus two. These are not exact independently measured
   frame totals. Small window/poll/copy timing differences near a chunk
   threshold can produce a large count ratio without that ratio describing
   a continuous byte-throughput error.
4. **Do not discard the controls.** VA2/3 stock and V128A G1 R both remain
   1505 / 1505; V256E R is 903 / 903. Long-blank 128-byte W is 2407 / 2407,
   and 256-byte W is 1200 / 1200. The 32-byte long-blank comparisons also
   remain recorded, not assumed identical. S is 20 / 21; that polling result
   alone does not establish the correct VB edge or an absolute time scale.
5. **The new VA control disagrees with the earlier VA blank assumption.**
   Every returned VA-label first-phase S is **20**, while its long-blank X is
   **86**. The historical issue table instead has VA S = 86. The unchanged
   emulator returns S = 85 on the VA and extends its status VB to 23 rasters
   regardless of SYNC. The two hardware corpora must not be silently combined
   as identical conditions, nor may this contradiction be hidden by retaining
   only the previously favorable VA ratio. Reconcile the SYNC/run conditions
   and current model against both records before changing the accepted model.

A simple arithmetic counterexample explains the quantization warning, not
hardware timing: let a chunk cost B + 8 arbitrary units, and available time
be 264 versus 274 units. Post-copy polling yields 7/2/1 versus 7/3/2 chunks
for 32/128/256-byte work. A time difference below 4% then looks like 50% or
100% in selected counts. **These arbitrary constants are not calibrated,
are not emulator parameters and authorize no wait or phase correction.**

The subsequent [instruction-cost/SYNC follow-up](m142_tvram_vb_events.md)
records nine completed diagnostic executions with this unchanged worker:
128/256-byte extra copies are authorized near the short-window threshold;
REP redispatch affects charged costs; selecting 200-line SYNC before screen 5
also yields a large S on the VA2 model without a VA hold. That is an alternative
condition, not proof of the historical VA key/SYNC history. A physical limiter,
corrected VB specification and tested emulator correction remain **unresolved**.
No source/binary rebuild, hosted CI, milestone merge or gate acceptance is
inferred from these documentation/evidence follow-ups.
