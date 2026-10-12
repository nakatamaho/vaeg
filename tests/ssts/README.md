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
# SingleStepTests V20 comparison data

The check is `tools/qa/sst_v20.py` with the baseline `v20/baseline_ci.json`;
see `docs/modernization/cpu-conformance-checks.md`.

The uPD9002 comparison uses the MIT-licensed `v1_native` corpus from
<https://github.com/SingleStepTests/v20> at the exact commit recorded in
`v20_dataset_manifest.json`. The upstream corpus is external test data and is
not included in vaeg source or release archives.

Acquire the approved checkout into an external cache:

```sh
python3 tools/qa/upd9002_ssts.py acquire --cache-root /path/to/cache
```

Verify every compressed corpus shard, the metadata, and the license before use:

```sh
python3 tools/qa/upd9002_ssts.py verify \
  --dataset-root /path/to/cache/singlesteptests-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21 \
  --manifest tests/ssts/v20_dataset_manifest.json
```

The manifest records SHA-256 identities at three levels: canonical test
records, ordered opcode/form test sets, and the complete dataset. README and
metadata version strings are informational and are deliberately excluded from
the dataset identity. Any missing file, changed digest, unknown metadata status,
or unknown schema field fails closed.

The adapter follows the pinned OUTS fixture convention: a non-DS segment
override may leave the source byte at its DS-relative `initial.ram` address
while the expected MEMR cycle identifies the effective overridden address.
The byte is mirrored only when the expected cycle and default source agree;
missing or conflicting evidence fails closed. Synchronous interrupt results
come from a test-only event seam, rather than inference from final CS:IP, so
software `INT 00`, DIV/IDIV type 0, and ordinary arrival at the IVT0 target
remain distinct.

Configure a test build with the verified checkout to enable the comparison in
CTest:

```sh
cmake --preset linux-ci-gcc \
  -DVAEG_SSTS_V20_ROOT=/path/to/cache/singlesteptests-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21
cmake --build --preset linux-ci-gcc
ctest --test-dir build/linux-ci-gcc --output-on-failure -R '^vaeg_sst_v20_'
```

After an intended improvement, regenerate and commit the baseline:

```sh
python3 tools/qa/sst_v20.py run --dataset-root "$V20" \
  --worker build/linux-ci-gcc/sdl2/vaeg --output /tmp/sst-v20-ci.json
python3 tools/qa/sst_v20.py update --result /tmp/sst-v20-ci.json \
  --baseline tests/ssts/v20/baseline_ci.json
```

`campaigns/g70/` holds the dispatch tables read by
`tools/qa/upd9002_protected_deletion.py`. The G43-G70 scoreboards, campaigns,
evidence, and validators were removed in M108; they remain available at commit
[884b7dca](https://github.com/nakatamaho/vaeg/commit/884b7dca97b6576007baa235b81a2597fa5f0c22).
