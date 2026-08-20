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

# M97 - SGP Technical Manual command completion report

Evaluated baseline: `79ce89af64958cd85cdffa030890fb24a2af8148`

Status: **implementation in progress**

## 1. Rejected QA milestone removal

The unmerged `topic/m97-deterministic-qa` branch contained only the rejected
M97/M98 QA foundation after the evaluated `main` baseline. The replacement
branch was recreated from the baseline. No QA source, generated D88, fake
BIOS, capture frontend, guest injection, task, or report from that branch is
part of this candidate.

Maintainer-local untracked references and private media were not removed or
modified.

## 2. Manual-derived implementation matrix

| Area | Manual-derived behavior | M97 action | Hardware status |
|---|---|---|---|
| Command address | Word writes at `0500h` and `0502h`, even address | Preserve | Documented |
| Start/status | Start and BUSY at `0506h` | Preserve | Documented |
| Abort/IRQ | Control at `0504h`, IRQ at END | Preserve; timing deferred | Functional documented; ordering unresolved |
| SET WORK | Even address, stable writable 58-byte area | Preserve address only | Internal layout unresolved |
| Descriptors | Start dot, mode, 12-bit dimensions, aligned pitch/address | Correct original-VA profile | Documented for original VA |
| ROP | Sixteen Boolean functions | Verify current table | Documented |
| `TP=2` | Transfer only where destination pixel is zero | Verify current final-mask path | Documented |
| PATBLT | Repeat source in two dimensions | Preserve and regress | Documented normal case |
| LINE | `VD=0800h`, `HD=0400h` | Correct masks | Documented; raster tie rules unresolved |
| CLS | Fill a contiguous word count | Preserve | Documented normal case |
| SCAN RIGHT | Search boundary color and update width | Implement | Documented normal case |
| SCAN LEFT | Search boundary color and update left edge/width | Implement | Documented normal case |
| Thirteenth command | Manual says thirteen but names twelve | No implementation | Unresolved |
| Timing/contention | No recovered command-cycle table | No change | Hardware pending |

## 3. Evidence corrections

Direct reading of the Technical Manual resolves two stale conclusions in the
existing reconstruction:

- the documented ROP order matches the current VAEG implementation;
- SCAN always searches for SET COLOR and documents first-pixel, found, and
  not-found results; it does not expose an undocumented equality selector.

LINE direction bits also use the same `VD` and `HD` positions as BITBLT and
PATBLT. Exact discrete-line tie breaking remains unresolved.

## 4. Implementation and validation

To be completed by M97b-M97d.

## 5. Human gate

G97 is pending. It is a VAEG visual-regression gate and does not require or
claim a real-hardware run.
