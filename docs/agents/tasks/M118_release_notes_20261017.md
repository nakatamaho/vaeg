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

# M118 — Rel.20261017 release notes

Status: **complete; G118 passed on 2026-10-16; merged to `main` at [c5d518f](https://github.com/nakatamaho/vaeg/commit/c5d518f53603f15ac9c5b899103344668c788adf)**

## Request

Write `CHANGES.20261017.md` from the commits since Rel.20260908, keeping
what matters to users; add the V1/V2 and N modes to the README features and
a README News entry (maintainer, 2026-10-16).

## Work

- `CHANGES.20261017.md`: the 802 commits from Rel.20260908 to `badfa255`,
  summarized from the roadmap rows and the bug-fix ledger; open defects
  (for example GLASS, NEON3 and NEON4 on the original VA) are left out.
- README: features for the V1/V2 and N modes, PC-8801/PC-8001 machines,
  PCG, tape images, 8087, state slots, lamp bar and rotation; the outdated
  "`BRKEM2` is not yet supported" corrected; News entry for Rel.20261017.

- Screenshots from the maintainer (2026-10-16 captures, named for the
  release date): `docs/images/vaeg-20261017-155514-0000091005-000.png`
  (PC-8801mkIISR demonstration, V2 mode) and
  `docs/images/vaeg-20261017-155419-0000036240-000.png` (N88-BASIC V2),
  `docs/images/vaeg-20261017-162748-0000092236-000.png` (N-BASIC, N mode) and
  `docs/images/vaeg-20261017-173043-0000094917-000.png` (a cassette tape loading in
  V1/V2 BASIC), the last two added on 2026-10-16,
  shown in the README News entry; `CHANGES.20261017.md` links the same
  files as release assets of `rel-20261017`, as in Rel.20260908.

## Follow-up: compatible-mode HALT (maintainer, 2026-10-16)

After G118 the maintainer approved fixing, on this branch, the
compatible-mode `HALT` that a native interrupt did not end (found by
msx88va M02; a game port had worked around it with `EI; JR $`). Ledger entry
"M118 — a compatible-mode HALT never ended";
[correcting commit](https://github.com/nakatamaho/vaeg/commit/3bb56ce4b79468c3fc44ae0f6c9402b4c42f5bd7).

Checks (`linux-ci-gcc`, VA2 model where a model applies):

- `ctest`: 93 cases, 89 passed, 4 skipped (ROM or corpus not configured),
  0 failed. The new HALT case of `vaeg_upd9002_brkem_upd70008` fails
  without the correction.
- `tools/qa/m103b_basic_boot.py run` with maintainer-local VA2 ROMs and the
  V2 BASIC disk: exit 0.
- a compatible-mode test disk (M01), menu 1: `ei-halt` PASS, A=55, one VRTC between
  HALT and resume (before: hung on that line); the other probes unchanged.
- msx88va `msx.d88` built without its vaeg workaround: C-BIOS reaches its
  "No cartridge found" screen (before: blank, halted in its first wait).
- `check_encoding`, `check_eol`, `check_case`: pass. `clang-format-mp-22`
  is not installed on this host; clang-format 18 with the 22-only
  `KeepEmptyLines` key removed reports no change in the three files.

## Gate G118 (human)

The maintainer reviews the release notes and the README changes.

### G118

- Passed on 2026-10-16 (maintainer).

## Follow-up: hosted CI before the release (2026-10-17)

Hosted CI had failed on `main` since about M107b (ASan heap-use-after-free
in the selftest; Windows and macOS trace-enabled selftests). Three selftest
fixtures from M113-M115 used the I/O table without a live machine; each now
starts and terminates its own
([95846b95](https://github.com/nakatamaho/vaeg/commit/95846b952860aa7e6f32d4865e1c7d090de2d80e)). Local
`linux-ci-asan` and `linux-ci-gcc` CTest: 93/93 each (four skips). Ledger
entry "M118 — PC-8801, PC-8001 and PCG selftests accessed a destroyed
machine".

## Follow-up: release packaging (2026-10-17)

The first `rel-20261017` release run (tag at `3f926d7e`) stopped in the
Linux Package step: `stage-librashader-assets.sh` pins the SHA-256 of
`docs/licenses/THIRD_PARTY_NOTICES.md`, which M115a extended with the
X88000 and QUASI88 credits without updating the pin. No release or asset
was published. The pin now matches the current notices
([84549e10](https://github.com/nakatamaho/vaeg/commit/84549e101ae89eea6d689a9e0e1098f7c4eaf099)); the workflow's
Linux Package step, replayed locally on the `linux-ci-gcc` build, passes
`check-librashader-package.py` and `check_zex_archive.py`. The tag was moved
to the corrected commit.

The second run built and published the Linux package, but the macOS job
(release runner `macos-14`) failed to compile `cpu/upd8087/upd8087.c`: its
Apple clang rejects a `static const` array initialized from other `const`
objects ("initializer element is not a compile-time constant"); the build
workflow's `macos-latest` and GCC accept it. The FLD-constant table is now
an automatic array, which every C99 compiler accepts
([53ecfc35](https://github.com/nakatamaho/vaeg/commit/53ecfc35946d5b3898166013d590fbd5b413a16b)); CTest 93/93 and
the selftest pass. The run was cancelled and the tag moved again.

## Release published (2026-10-17)

[Rel.20261017](https://github.com/nakatamaho/vaeg/releases/tag/rel-20261017),
tag `rel-20261017` at
[31577624](https://github.com/nakatamaho/vaeg/commit/315776242341bb452f40fa2bf50df01ceae591c7)
(hosted build CI green). The release workflow built and uploaded the three
packages; the downloaded Linux and Windows binaries report
`Rel.20261017` and embed that commit. The four screenshots were attached
and the release published.

| Asset | SHA-256 |
|---|---|
| `vaeg-rel20261017-linux-x86_64.tar.gz` | `205b0d6db8bd2baf683771eab187de442377dc6a978d007d1e4feb14aed7d7d6` |
| `vaeg-rel20261017-macos-arm64.tar.gz` | `c6f406f3e2510f75eef7d7628934e6767ae3c81d9c041ee565132287445329d3` |
| `vaeg-rel20261017-win-x86_64.zip` | `8b5966665d093836b78e088acd4acae5e0028fb96af717fc7731e3a6d7b5227c` |
