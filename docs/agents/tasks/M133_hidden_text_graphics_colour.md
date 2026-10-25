<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->

# M133 — Hidden text retains monochrome graphics colours (PR #56)

Status: **human gate PASSED**; maintainer explicitly requested merge.

## Scope and correction

Preserve the original authorship and commit of [PR #56](https://github.com/nakatamaho/vaeg/pull/56),
[bcfb11aa74b7856c10e9ce8868897673cde7dfc3](https://github.com/nakatamaho/vaeg/commit/bcfb11aa74b7856c10e9ce8868897673cde7dfc3).
In multiplane 1-bit/pixel text-merged graphics, `148h` bit 7 used to clear
both the character raster and its attribute-colour raster. The composer uses
that colour raster for graphics dots, so hiding characters also blacked out
the graphics. Continue generating attribute colours and clear only the
character raster in this mode. Other text-hidden modes and TSP display-stop
retain the existing full blanking. No archived reference-tier source or
provenance is changed.

## Verification and accepted handoff

- Static review traced `maketextva_raster()`, `videova_textmerge()` and
  `scrndrawva_compose_raster()`. No blocking code defect was identified.
- [Candidate CI](https://github.com/nakatamaho/vaeg/actions/runs/39935773271):
  all ten reported jobs SUCCESS. This is existing candidate evidence, not a
  new run of the latest-main integration.
- Clean clone of the candidate, using
  `tools/release/build-mingw-static-container.sh --commit
  bcfb11aa74b7856c10e9ce8868897673cde7dfc3 --recording` with a fresh
  task-specific work directory: completed successfully. Recording ON;
  Windows Media Foundation post-stop MP4; bundled local H.264/AAC encoders
  OFF. Build identity and Windows-system-only PE imports passed.
- Executable SHA-256:
  `d67812b64d316b3a3d3c8787cbfb2f86d20baba800d0ed2e8028f480d88f5d11`.
  Executable, build report and dependency/build logs remain outside Git.
- The maintainer chose Windows executable verification, then stated
  **gate passed**, and subsequently explicitly requested merge.
  [Acceptance record](https://github.com/nakatamaho/vaeg/pull/56#issuecomment-6395895474).
  Do not infer individually reported runtime cases from this acceptance.
- Dedicated new raster regression tests, agent-run Windows execution,
  latest-main integration runtime/build and this task's full demo/media
  regeneration: **NOT_RUN**. The prior review recommended raster regression
  tests; they are not claimed implemented or passed. The existing human
  acceptance and explicit merge request are recorded without upgrading these
  coverage gaps.

## Merge-time repository checks

On the merged working tree, all commands below exited 0:

```text
python3 tools/repo/check_case.py          # 0 finding(s)
python3 tools/repo/check_encoding.py      # no output
python3 tools/repo/check_eol.py           # no output
python3 tools/repo/check_md_comments.py   # 0 finding(s)
git diff --check                         # no output
git diff --cached --check                # no output
```

The source integration is exactly the PR's 18 added / 3 removed lines in
`vram/maketextva.c` relative to the current main. Existing M131/M132 changes
are retained, not replaced by the older candidate tree. No hosted CI rerun
was requested during this bookkeeping/merge operation.

## Retention

Keep the accepted Windows executable and build report. No user-supplied
inputs, shared caches, remote branches or task directories are deleted by
this merge. No later milestone is started.
