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
