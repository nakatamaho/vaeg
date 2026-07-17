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
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->
# M43 uPD9002 SingleStepTests V20 baseline report

## Disposition and scope

M43 adds an external semantic comparison baseline and test-only adapter. It
does not change active CPU instruction semantics, dispatch, timing, prefetch,
DMA ordering, interrupts, state layout, CPU_SHUT behavior, protected-mode
code, or public CPU names. M44 has not started. This report requests human
G43 review and does not claim that G43 passed.

The work started from accepted M42 commit
`336227f093d37e3c60bc50333216d66668755cef` on branch
`topic/m43-upd9002-singlestep-v20-baseline`. The ending and remote SHA are
recorded in the final validation section after the evidence commit is pushed.

## Commits and changed files

The implementation and immutable-baseline commits preceding this report are:

1. `7510b2e02bc0891957597ef5fc655caa72d5c2e1` - `M43: add SingleStepTests V20 baseline task`
2. `ba6bc297e598f72c646b471ea3be7c2f4533f098` - `M43: pin SingleStepTests V20 dataset identity`
3. `7af8bdcb8886cc66c17348f3e4f8032b945b2d85` - `M43: classify V20 corpus forms fail closed`
4. `3ad0f7c5ad9c2c16d1328bca4fd7d91e35302d43` - `M43: add contained V20 direct-harness worker`
5. `e7a985d473330b27d0fc9252a1e38a301c31b7a9` - `M43: run deterministic V20 comparison profiles`
6. `1865d46c5ea85a6a376ff3c634656b7cb10d2ab9` - `M43: integrate reproducible V20 profile checks`
7. `949bffce836124fcdd99f7e48cc468244e8b10f9` - `M43: record V20 CI and full comparison baselines`

The final documentation commit is the commit containing this report and is
identified by the completion response and pushed branch. Changed active/test
infrastructure files are `CMakeLists.txt`, `i286c/memory.c`, `io/iocore.c`,
`sdl2/np2.c`, `tests/upd9002/direct_harness.c`,
`tests/upd9002/direct_harness.h`, new `tests/upd9002/ssts_worker.c`, new
`tests/upd9002/ssts_worker.h`, and new `tools/qa/upd9002_ssts.py`. Dataset and
baseline files are `tests/ssts/README.md`,
`tests/ssts/v20_dataset_manifest.json`,
`tests/ssts/baseline/upd9002_v20_known_gaps.json`, both profile summaries, and
their hexadecimal-sharded deterministic failure sidecars. Documentation files
are the M43 task and report, `assets/NOTICE.md`, ADR-0012, ROADMAP, and
`docs/modernization/BUILD.md`. No file was renamed or deleted.

## Upstream identity and license

| Field | Pinned value |
|---|---|
| Repository | `https://github.com/SingleStepTests/v20` |
| Suite | `v1_native` |
| Commit | `9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21` |
| License | MIT, Copyright (c) 2024 Daniel Balsom |
| License SHA-256 | `cb3882ef501e91281c1948f71ed62bfca6562345a98f5fc1b69824f62b6e1557` |
| README version | `1.0.3` (informational) |
| metadata version | `1.0.2` (informational) |
| metadata SHA-256 | `71c12e705960941a73981891852674649c3332539579634ea34d1dae40c1795a` |
| compressed-corpus manifest SHA-256 | `12f1d146f7070ed9a83fa516cf9dc2a6771b572d93bd92ab26e251c3be8f0294` |
| dataset digest | `1d2e9c0e14101f05379d938245af68f3219c16f638fce019ad2a1946084930a4` |
| dataset ID | `ssts-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21-1d2e9c0e14101f05379d938245af68f3219c16f638fce019ad2a1946084930a4` |

The version strings do not participate in dataset identity. The external
checkout was detached at the exact 40-character commit and was clean. The
corpus is approximately 1.7 GiB and is not committed or packaged by vaeg.
`assets/NOTICE.md` records the external test-data license without describing
the corpus as a bundled asset.

Identity has three content-addressed levels:

1. `record_digest` is SHA-256 of canonical JSON for one exact record.
2. `opcode_testset_digest` is SHA-256 of the form identity and its sorted
   record digests.
3. `dataset_digest` covers the pinned metadata digest, compressed-corpus
   manifest digest, all ordered form digests/counts, and identity policy
   `vaeg-upd9002-ssts-v20-v1`.

The manifest contains 360 shards and 3,125,000 records: 1,562,502 have an
empty initial prefetch queue and 1,562,498 have a populated queue.

## Observed metadata schema

The parser accepts only the exact pinned schema and fails closed on new keys,
shapes, statuses, or forms. The observed primary metadata has 282 opcode
entries and 17 group parents. Primary status counts are:

| Status | Count |
|---|---:|
| `normal` | 243 |
| `prefix` | 10 |
| `fpu` | 10 |
| `extension` | 1 |
| `undocumented` | 1 |
| missing status | 17 |

Primary architecture counts are `86` 221, `v30` 32, `186` 10, and missing
19. The 136 register/subentries contain statuses `normal` 92, `undefined` 27,
`alias` 11, and `undocumented` 6; their architecture counts are `86` 116,
`186` 19, and missing 1. The word `invalid` appears in upstream explanatory
text but is absent from the pinned metadata vocabulary and is rejected by the
adapter selftest.

The primary `0F` entry is the expected coarse `extension` marker. A literal
reading of the task's coarse-only description is incomplete: the pinned
metadata also has explicit keys for represented `0Fxx` shards. The adapter
still resolves every such record using its actual second byte, structural
form, prefix state, and the M42 support map; the primary `0F` entry cannot
classify the extension space by itself.

## Adapter and containment

The adapter reuses `tests/upd9002/direct_harness.*`; it does not implement a
second CPU executor. Test-only seams provide deterministic flat 1 MiB RAM,
deterministic `0xff` input, ordered byte I/O, and a hidden worker entry in the
test-bearing SDL2 executable. Initial registers, segments, IP, FLAGS, RAM, and
queue applicability are translated explicitly. Final registers, segments,
IP, metadata-masked FLAGS, the union of initial/final RAM addresses, ordered
external I/O, and termination are translated back to canonical records.

Each opcode shard runs in a child process. A shard timeout or abnormal exit is
recursively bisected until the individual record is isolated. Result classes
distinguish pass, semantic failure, target gap, upstream nonblocking,
unsupported fixture, expected divergence, timeout, crash, normal completion,
Type-0 exception, and HALT. The pinned corpus contains no HLT shard, so a
separate ROM-less synthetic worker test exercises the HALT category. V20
maximum-mode cycle and prefetch details remain diagnostic; they do not gate
uPD9002/V52 timing.

`VAEG_ENABLE_TESTS=OFF` does not compile the worker, flat-memory seam, or test
I/O seam. A production binary string audit found no worker option.

## Deterministic profiles and results

The CI profile selects at most 500 empty-queue records per form after sorting
by stable upstream test hash. Its selection digest is
`0be9aeb1bfad2db3e10e9abd4ba2fbf2921a7899824b04ac9a219607657db073`.
The full profile selects every empty-queue record and has selection digest
`3d856d20aa5ef2170b193cb8b2710d3dec577948c89b50083de8dfee90357010`.

| Result | CI | Full |
|---|---:|---:|
| Selected records | 180,000 | 1,562,502 |
| Applicable/executed | 166,821 | 1,443,876 |
| Passed | 155,984 | 1,358,343 |
| Semantic failures | 10,837 | 85,533 |
| Known-target-gap skips | 8,179 | 68,626 |
| Upstream-nonblocking skips | 5,000 | 50,000 |
| Total skips | 13,179 | 118,626 |
| Prefetched unsupported fixtures, reported separately | 1,562,498 | 1,562,498 |
| Normal termination | 165,546 | 1,431,171 |
| Type-0 termination | 1,275 | 12,705 |
| Timeout | 0 | 0 |
| Crash | 0 | 0 |
| Approved expected divergence | 0 | 0 |
| Command exit status | 0 | 0 |

A semantic failure means the instruction completed without timeout or crash,
but at least one blocking architectural, masked-FLAGS, RAM, I/O, or termination
field differed from the V20 oracle. It is not a target-gap classification.
M43 records these mismatches and does not repair the CPU.

Both profiles were rerun into independent `/tmp` output directories using
`--expect`. Their summaries, failure counts, canonical failure contents, and
signature indices reproduced exactly. The CI signature-index digest is
`96a4abdab6497467b6ec9e760a07fb540b7a6a3aa1dad27f5ac8c39715126d8e`;
the full digest is
`19f6cecb1d79868a1257da2491c4107ac64e60a302336ceac2f0fab3ecc30a7b`.

## Applicability and known gaps

Every one of the 3,125,000 records has exactly one classification:

| Classification | Count |
|---|---:|
| `applicable` | 1,443,876 |
| `known_target_gap` | 68,626 |
| `unsupported_fixture` | 1,562,498 |
| `upstream_nonblocking` | 50,000 |
| `expected_target_divergence` | 0 |

The machine-readable known-gap baseline has 40 exact structural selectors and
68,626 resolved full-profile test hashes. They consist of 12 absent/reserved
`0Fxx` forms, opcode `63`, and 27 exact REPC/REPNC string/I/O forms. Each rule
records opcode, second byte where applicable, repeat prefix, ModR/M constraint,
support-map mode/subopcode/target, full sorted resolved hashes, count, and hash
set digest. `segment_prefix_constraint` and `lock_prefix_constraint` are
`dispatch-neutral-any` only where the M42 final graph proves those prefixes do
not change the absent target. This is a structural constraint, not an `idx`
or blanket allowlist.

The M42 inventory's 508 support-map target-gap rows are not reused as an
approved count. M43 independently resolves the pinned corpus. Its 40 rules are
selectors and its 68,626 value is a record count, so neither number is directly
equivalent to M42's form-row count. Expansion after G43 is prohibited.

## Failure signatures

The fixed-order signature content stores dataset ID, profile, canonical record
digest, upstream test hash, opcode/form, classification, initial-state digest,
expected and actual state, masked FLAGS comparison, ascending RAM differences,
ordered I/O differences, exception/HALT result, timeout/crash status,
instruction bytes, sorted mismatch kinds, and fixed-order register
differences. Lowercase zero-padded hexadecimal and canonical JSON feed SHA-256.
A known failure is keyed by both record digest and signature digest.

Complete readable contents are stored as deterministic gzip sidecars. The
summary records SHA-256 of each uncompressed canonical JSON payload, so gzip
library details are not semantic identity. CI mismatch groups are registers
3,957, RAM 3,292, RAM+registers 3,206, I/O 244, and I/O+registers 138. Full
groups are registers 33,976, RAM 30,911, RAM+registers 18,787, I/O 1,204,
I/O+registers 641, RAM+termination 10, and RAM+registers+termination 4.

Representative full signatures, with complete content in the sidecars, are:

| Mismatch | Form | Upstream test hash | Signature SHA-256 |
|---|---|---|---|
| RAM | `0F20` | `45629c32445ab4b97ec4365ef8e575b1a9df191a` | `7f5b51fe9b40a22852a5b868f3e0a39dc6eeee9f437da02b04ce255396ca5ca5` |
| registers | `0F2A` | `4adf7c385afec8b14e0f1b054e89fc2ad1ff2997` | `529e09ca8ac0033161ea89f942785b68e358faa21f74428bd79572c51895169b` |
| I/O | `6E` | `c978f32e0cdfb083f50ccafa5b3b3ee18dd0e986` | `850a5c90b7cc0f0c10d6d12c1d8d8702d9cf0363aa730756bc1abfee1fe3ad10` |
| RAM+termination | `CD` | `1d744c11c86f6697aebba2dfcafb6f127ad2f9a9` | `5c13cbc541c8c6be65c59d2ba3e4161016cebce6290bfe86cb57a203899d7188` |

## Difficult instruction families

| Family | CI result | Full result |
|---|---|---|
| REPC string/I/O | 235 exact gaps | 846 exact gaps |
| REPNC string/I/O | 244 exact gaps | 872 exact gaps |
| REPE/REPNE CMPS access ordering | 230 pass | 1,260 pass |
| C0/C1 and D2/D3 count handling | 12,170 pass, 3,830 fail | 60,861 pass, 19,139 fail |
| DIV/IDIV | 725 pass, 1,275 fail | 7,300 pass, 12,700 fail |
| REP-prefixed IDIV | 16 pass, 66 fail | 169 pass, 571 fail |
| `0F` extensions | 5,814 pass, 686 fail, 6,000 gap | 51,913 pass, 5,087 fail, 56,000 gap |

All four M42 explicit F6/F7 DIV/IDIV replacement forms occur. Corpus records
also reach the represented M42-patched native, segment-prefix, REP, FLAGS,
shift, IRET, AAM/AAD/XLAT, LOOP, and grouped F6/F7 paths. Failures are frozen
by individual record and signature; this report does not infer a root cause
from opcode family alone.

## M42 and production preservation

The M42 final graph, construction provenance, support map, 156-case harness
manifest, trace baseline, raw state/CPU_SHUT fixtures, and ABI fixture compare
byte-for-byte with accepted M42. Dispatch generation, generator selftest, ABI,
trace equivalence, direct harness, reset/executed/CPU_SHUT fixtures, ROM-less
selftest, and smoke pass. The accepted CPU_SHUT upper-FLAGS initializer anomaly
is unchanged. No M23 golden existed at the M42 starting point; all existing
pre-M42 tracked test goldens remain unmodified.

## Validation

All commands below returned zero unless explicitly noted. Exact profile
commands are documented in `tests/ssts/README.md`; both were executed once to
record and once with `--expect` to prove reproduction.

```sh
python3 tools/qa/upd9002_ssts.py verify \
  --dataset-root /tmp/singlesteptests-v20-9efbd02b \
  --manifest tests/ssts/v20_dataset_manifest.json
python3 tools/qa/upd9002_ssts.py selftest
python3 tools/qa/upd9002_ssts.py report \
  --summary tests/ssts/baseline/v20_native_ci.json
python3 tools/qa/upd9002_ssts.py report \
  --summary tests/ssts/baseline/v20_native_full.json
```

The two required comparison profiles were reproduced with these exact command
forms (the first recording run used the committed output paths; the second run
used `/tmp/m43-repro/` and added the shown `--expect` arguments):

```sh
python3 tools/qa/upd9002_ssts.py run \
  --dataset-root /tmp/singlesteptests-v20-9efbd02b \
  --manifest tests/ssts/v20_dataset_manifest.json \
  --support-map tools/qa/golden/upd9002_support_map_m42.csv \
  --worker build/m43-linux-debug/sdl2/vaeg \
  --profile ci --shard-timeout 300 \
  --output /tmp/m43-repro/v20_native_ci.json \
  --failure-directory /tmp/m43-repro/v20_native_ci_failures \
  --expect tests/ssts/baseline/v20_native_ci.json
python3 tools/qa/upd9002_ssts.py run \
  --dataset-root /tmp/singlesteptests-v20-9efbd02b \
  --manifest tests/ssts/v20_dataset_manifest.json \
  --support-map tools/qa/golden/upd9002_support_map_m42.csv \
  --worker build/m43-linux-debug/sdl2/vaeg \
  --profile full --shard-timeout 300 \
  --output /tmp/m43-repro/v20_native_full.json \
  --failure-directory /tmp/m43-repro/v20_native_full_failures \
  --expect tests/ssts/baseline/v20_native_full.json
```

Both commands returned zero. CI executed 166,821 applicable records; full
executed 1,443,876. Neither run crashed or timed out. The command's success
means the complete committed semantic-failure signatures reproduced, not that
the target matched the V20 oracle for every applicable record.

M42 generation and immutable-baseline preservation were checked with:

```sh
python3 tools/qa/upd9002_dispatch.py --root .
python3 tools/qa/upd9002_dispatch.py --root . --selftest
git diff --exit-code 336227f093d37e3c60bc50333216d66668755cef -- \
  tools/qa/golden/upd9002_final_dispatch_graph.csv \
  tools/qa/golden/upd9002_dispatch_provenance_m42.csv \
  tools/qa/golden/upd9002_support_map_m42.csv \
  tests/upd9002/harness_manifest.csv \
  tests/upd9002/trace_baseline.txt \
  tests/upd9002/state_fixtures_m42.txt tests/upd9002/abi_g41.txt
```

All returned zero. The generated final graph and every named M42 file are
byte-identical to the accepted starting commit.

The final native GCC test-bearing build used:

```sh
cmake --build build/m43-linux-debug -j2
ctest --test-dir build/m43-linux-debug --output-on-failure
ctest --test-dir build/m43-linux-debug --output-on-failure \
  -R 'vaeg_upd9002_(dispatch|abi|trace)'
env SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  build/m43-linux-debug/sdl2/vaeg --selftest
env SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  build/m43-linux-debug/sdl2/vaeg --smoke
```

The configured GCC comparison build ran the external CI profile through
CTest in 179.45 seconds with no skip on the final rerun. All 24 tests passed,
as did
manual ROM-less selftest and smoke. The equivalent Clang 21.1.8 build passed
all 24 CTests with the external corpus intentionally absent/visibly skipped,
then passed selftest and smoke. The tests-disabled GCC 15.2.0 production build
passed build, selftest, and smoke and contained no worker option.

The Clang and production-only commands were:

```sh
cmake --build build/m43-clang -j2
ctest --test-dir build/m43-clang --output-on-failure
env SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  build/m43-clang/sdl2/vaeg --selftest
env SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  build/m43-clang/sdl2/vaeg --smoke
cmake --build build/m43-production -j2
if strings build/m43-production/sdl2/vaeg | \
  grep -q -- '--upd9002-ssts-worker'; then exit 1; fi
env SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  build/m43-production/sdl2/vaeg --selftest
env SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  build/m43-production/sdl2/vaeg --smoke
```

The GCC ASan/UBSan build passed the four M43 SSTS tests with
`UBSAN_OPTIONS=halt_on_error=1` and `ASAN_OPTIONS=detect_leaks=0`. The full
24-test matrix, selftest, and smoke passed with non-halting UBSan. It reproduced
the accepted pre-existing signed arithmetic diagnostics in
`sound/tms3631c.c`, `sound/psggenc.c`, `sound/psggeng.c`, and
`common/parts.c`; no diagnostic names M43 code. Leak detection is disabled
because LeakSanitizer cannot run under the managed ptrace environment.

```sh
cmake --build build/linux-ci-asan -j2
env ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 \
  ctest --test-dir build/linux-ci-asan --output-on-failure \
  -R vaeg_upd9002_ssts
env ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=print_stacktrace=0 \
  ctest --test-dir build/linux-ci-asan --output-on-failure
```

The first command group passed all five M43 SSTS tests (four executed and the
external-data case visibly skipped because this build lacks the corpus path).
The second passed 24/24 with the same external skip. An exploratory rerun with
`UBSAN_OPTIONS=print_stacktrace=1` returned 8 because ASLR-dependent sanitizer
stack addresses entered the M42 test's intentionally exact stderr comparison;
the standard non-stacktrace run reproduced 24/24. A separate
`halt_on_error=1` trace run stops at the pre-existing negative shift in
`sound/tms3631c.c`, before M43 code. Neither exploratory result is classified
as an M43 semantic or harness failure.

MinGW-w64 cross-build and Wine execution used:

```sh
cmake --build build/m43-mingw -j2
env WINEDEBUG=-all WINEPREFIX=/tmp/vaeg-m43-wine \
  WINEPATH=/usr/lib/gcc/x86_64-w64-mingw32/13-win32 \
  wine64 build/m43-mingw/vaeg_upd9002_abi.exe \
  'Z:\tmp\vaeg-m43-upd9002\tests\upd9002\abi_g41.txt'
env WINEDEBUG=-all WINEPREFIX=/tmp/vaeg-m43-wine \
  WINEPATH=/usr/lib/gcc/x86_64-w64-mingw32/13-win32 \
  SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  wine64 build/m43-mingw/sdl2/vaeg.exe --selftest
env WINEDEBUG=-all WINEPREFIX=/tmp/vaeg-m43-wine \
  WINEPATH=/usr/lib/gcc/x86_64-w64-mingw32/13-win32 \
  SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
  wine64 build/m43-mingw/sdl2/vaeg.exe --smoke
```

The cross-build had no pending work after its successful 553-step build. ABI,
156 direct-harness cases, reset/executed/CPU_SHUT fixtures, selftest, and smoke
passed under Wine. Wine is not native Windows execution.

Final repository checks used:

```sh
python3 tools/repo/check_encoding.py --expect utf8 --exclude hlp/
python3 tools/repo/check_eol.py --enforce
python3 tools/repo/check_case.py
python3 tools/repo/find_unreferenced.py --report
git diff --check
```

They returned zero. Encoding, EOL, and case checks reported zero findings;
the unreferenced report retained the 69 pre-existing findings.

Native Windows/macOS execution is provided only by hosted CI after the final
branch push. The exact final/remote SHA, hosted URL/result, and clean status
are reported in the completion response; this report does not mislabel the
MinGW cross-build or Wine execution as native.

## Deviations, limitations, and human review

* The authoritative M43 task file was maintainer-provided but absent from the
  accepted M42 tree. It was copied byte-identically and committed before M43
  implementation; this was non-material to CPU behavior.
* The pinned metadata contains explicit `0Fxx` entries in addition to the
  coarse primary `0F` entry. ADR-0012 records this source-evidence correction.
* Populated-prefetch-queue records are unsupported fixtures because the M42
  harness cannot faithfully inject V20 prefetch state. They are counted and
  never silently ignored.
* FPU/undefined oracle entries are upstream nonblocking, not target-gap or
  semantic-failure conversions.
* The 85,533 full semantic failures are investigation evidence. M43 neither
  approves them as permanent divergences nor changes CPU behavior.
* Human G43 review must approve the 40 narrow known-gap selectors, verify both
  profile identities/counts/signature indices, accept the explicit external
  corpus/cache policy, confirm M42 byte preservation, and review the recorded
  semantic-failure baseline. G43 has not been declared passed here.
