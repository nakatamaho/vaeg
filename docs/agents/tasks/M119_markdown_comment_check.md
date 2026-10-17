<!--
Copyright (c) 2026 Nakata Maho

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
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

# M119 — check Markdown files for HTML comments that hide content

Status: **complete; G119 machine-verifiable gate passed on 2026-10-17; merged to `main` at [ad4a606](https://github.com/nakatamaho/vaeg/commit/ad4a606e6b690c918f2482bcbf9acadc900350d4)**

## Request

After the Rel.20261017 release notes rendered as raw HTML, the maintainer
approved (2026-10-17) a repository check that catches the cause.

## Cause found in M118

Thirteen Markdown files (`CHANGES.20261017.md`, `demos/sgp-boot/README.md`
and the M110-M118 task files) had a license header comment that lost its
last line and closing `-->` when copied by line count. GitHub treats a line
starting with `<!--` as an HTML block comment that lasts to the first line
containing `-->`, so those files rendered blank and the release notes as
raw tags. Fixed in M118
([6eba34e7](https://github.com/nakatamaho/vaeg/commit/6eba34e7d3349c4a5c1b5a6c31c03bf65fa271f4)).

## Work

- `tools/repo/check_md_comments.py` (BSD): over git-tracked `*.md` outside
  `external/`, reports an HTML block comment that never closes
  (`MD_COMMENT_UNCLOSED`) or that closes but hides a heading line
  (`MD_COMMENT_SPANS_HEADING`), with the line of its `<!--`. An unclosed
  `<!--` inside a line (shown as text) and fenced code are not reported.
  `--report` lists without failing.
- `tests/repo/test_check_md_comments.py`, CTest
  `vaeg_repo_check_md_comments`: a fixture that passes; three single
  mutations, each asserting its code and line (header without `-->`, a
  block comment left open, a comment no longer inside a fence); one run in
  a temporary Git repository (exit status, `external/` skipped, `--report`).
- CI `repo invariants` job runs the check.

## Verification

- Current tree: 0 findings. The commit before the M118 fix: 13 findings,
  exactly the 13 files.
- GitHub's Markdown API on the fixture: the passing fixture shows both
  headings and the literal inline marker; the header mutation hides both
  headings, the fence mutation hides the second; the checker agrees.
- Mutation checks of the checker: without the heading rule two tests fail;
  without fence tracking four fail.

## Gate G119 (machine-verifiable)

`python3 tools/repo/check_md_comments.py` exits 0 on `main`, CTest
`vaeg_repo_check_md_comments` passes, and the hosted `repo invariants` job
passes.

### G119

- Passed on 2026-10-17: the check reports 0 findings, CTest
  `vaeg_repo_check_md_comments` passes, and hosted build run 38978374227 on
  [1a1ca6a0](https://github.com/nakatamaho/vaeg/commit/1a1ca6a06435bbbb9468be616cca4f113f41662b)
  passed every job, its `repo invariants` job running the check.
