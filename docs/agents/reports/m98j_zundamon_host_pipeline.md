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

# M98j - Zundamon host-pipeline report

Evaluated predecessor: `3160756daeab2e6a51775a5aee595fc124f7ce02`

Status: **implementation candidate; `HOST_FIXTURE_PASS`; `LOCAL_HOST_PIPELINE_BLOCKED_FRAME_TOO_LARGE`; G98j pending**

## Result

M98j adds one source-neutral host pipeline that validates the manifest and
input, recovers exact indices, converts VA8 pixels, generates all 32 scales,
packs the final atlas, and applies both the M98h format inspector and M98i
minimal-packing validator. It writes no intermediate pixel or scale stream.

Successful output contains only the final atlas, a deterministic 32-level
contact sheet, and a combined private report. The contact sheet uses a fixed
bounded grid, checkerboard transparency, projected anchor crosses, and labels
for level, dimensions, and anchor. Its preview resampling never feeds back
into atlas data.

## Public machine evidence

The M98j standard-library suite ran seven test methods and passed:

```text
OK
M98J_TEST_PASS
```

Coverage includes byte reproducibility, input immutability, exact agreement
with independently composed M98f-M98i output, both final inspectors,
contact-sheet geometry and marker pixels, report reconciliation, the exact
three-file output set, overwrite refusal, isolated failures, and path-redacted
public/local CLI behavior.

M98b-M98i regressions also passed, for 60 passing test methods across M98b
through M98j. Python compilation, the public pipeline workflow, JSON,
repository encoding, EOL, case, diff, and M98 privacy checks passed. Generated
public and local data remains untracked.

## Local gate evidence

One existing approved local bundle was available and passed the M98d input
preflight. The unchanged M98j pipeline then stopped at the M98i complete-frame
limit because at least one generated frame exceeds one 128-KiB BMS bank. No
output directory was created. No crop, scale, pixel, or packing contract was
changed to bypass the failure.

This establishes `LOCAL_HOST_PIPELINE_BLOCKED_FRAME_TOO_LARGE`, not local
pipeline readiness and not G98j acceptance. Resolving it requires an explicit
maintainer decision outside this implementation candidate.

## Boundary

G98j remains pending. M98j makes no local-atlas, guest, BMS runtime, SGP,
VAEG, disk-image, screenshot, or physical-machine claim. M98k is unassigned.
