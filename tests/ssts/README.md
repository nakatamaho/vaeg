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

Configure a test build with the verified checkout to enable the external CI
comparison in CTest:

```sh
cmake -S . -B build/m43-ssts -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DVAEG_ENABLE_TESTS=ON \
  -DVAEG_ENABLE_ARCHIVE_DROP=OFF \
  -DVAEG_SSTS_V20_ROOT=/path/to/cache/singlesteptests-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21
cmake --build build/m43-ssts
ctest --test-dir build/m43-ssts --output-on-failure \
  -R vaeg_upd9002_ssts
```

Without `VAEG_SSTS_V20_ROOT`, ordinary hosted builds run the synthetic
fail-closed selftest and verify the committed summaries, while the external
comparison appears as an explicit CTest skip. Such a skip does not satisfy a
human M43 or later uPD9002-series gate.

The CI profile takes the first 500 empty-prefetch-queue records per resolved
opcode/form after sorting by the stable upstream test hash. The full profile
uses every empty-queue record. Run the full gate profile with:

```sh
python3 tools/qa/upd9002_ssts.py run \
  --dataset-root /path/to/cache/singlesteptests-v20-9efbd02b8ec1a3aad347c2b59672ad25f3bcdb21 \
  --manifest tests/ssts/v20_dataset_manifest.json \
  --support-map tools/qa/golden/upd9002_support_map_m42.csv \
  --worker build/m43-ssts/sdl2/vaeg \
  --profile full \
  --shard-timeout 300 \
  --output /tmp/v20_native_full.json \
  --failure-directory /tmp/v20_native_full_failures \
  --expect tests/ssts/baseline/v20_native_full.json
```

The result summaries and gzip-compressed failure sidecars under
`tests/ssts/baseline/` are machine-readable. Each sidecar is verified through
the SHA-256 of its uncompressed canonical JSON, so gzip implementation details
are not part of the semantic baseline. Print a compact verified summary with:

```sh
python3 tools/qa/upd9002_ssts.py report \
  --summary tests/ssts/baseline/v20_native_full.json
```
