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

# M98i - Zundamon BMS bank-packing report

Evaluated predecessor: `7db0dce837542b6632f0cd322ab4fb334d45cfd4`

Status: **G98i machine gate passed on 2026-08-31; `HOST_BMS_PACKING_PASS`**

## Result

M98i adds a deterministic sequential packer for the 32 M98g scale frames.
Each complete payload starts at a 16-byte-aligned offset in a 128-KiB logical
BMS bank. A frame that would cross the current bank starts at offset zero in
the next bank. The algorithm does not split, reorder, backfill, or duplicate
frames, and derives the minimum bank count for that fixed order.

The resulting atlas retains the compact M98h version-1 file layout. Logical
bank tails are accounted for in the report but are not serialized. The
packer validates the complete M98g scale set before encoding, sends the final
container through the independent M98h inspector, and then applies a separate
M98i minimal-packing validator. This preserves the M98h format fixture as
format-valid while distinguishing it from production packing.

The generated JSON report reconciles useful pixels, row padding, BMS frame
alignment, bank-boundary padding, per-bank payload and occupied bytes,
compact file alignment, payload region, complete file size, and required bank
count. CLI output contains only neutral success tokens or stable error codes.

## Machine evidence

The M98i standard-library suite ran eight test methods and passed:

```text
OK
M98I_TEST_PASS
```

Coverage includes independent exact-fit, alignment-fit, one-byte-overflow,
and multi-bank plan oracles; a large neutral multi-bank scale set; complete
M98h format validation; rejection of the format-only nonminimal fixture;
isolated scale-stream and plan corruptions; oversized-frame rejection;
metrics reconciliation; deterministic output; overwrite refusal; and
path-redacted CLI success and failure.

M98b-M98h regressions also passed, for 53 passing test methods across M98b
through M98i. Python compilation, the public build/dual-inspector workflow,
JSON validation, repository encoding, EOL, case, diff, and M98 privacy checks
passed. No generated atlas, report, or maintainer-supplied data is tracked.

## Boundary

G98i is a public host-packing gate only. M98i makes no local integration,
guest, BMS runtime, SGP, VAEG, disk-image, screenshot, or physical-machine
claim. M98j remains unassigned.
