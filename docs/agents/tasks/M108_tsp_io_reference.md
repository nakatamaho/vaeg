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
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# M108 — Publish a port-oriented TSP reference

## Maintainer authorization

The maintainer requests a readable README I/O block map and a detailed,
independently written Japanese TSP reference at `docs/io/io_tsp.md`, using
port/name/target/chip/function/bit-description/commentary/related sections.
Publish known information with explicit evidence limits, not a copied manual.
This documentation-only task is independent of the pending M107 demo gate.
It authorizes no emulator changes, new hardware experiments, or subsequent
implementation milestone.

## Closed scope

- `README.md`: the supplied eight-block map and a link to the TSP reference.
- `docs/io/io_tsp.md`: ports 0142H/0146H, unresolved 0143H, status bits,
  handshake, command and parameter details, standard SYNC vectors,
  ROM-used transfer paths, tables/address units, related board controls,
  implementation gaps, and existing measurement references.
- This task and its roadmap row.

Separate VA documentation, generic device specifications, current source
behavior, existing hardware records, and unknowns. In particular, distinguish
command polling with mask 05H from parameter polling with mask 01H; the
existing timing tools and current BUSY lifetime do not support an unconditional
05H parameter wait. Do not change the earlier reconstructed specification or
claim a new hardware confirmation. No private input paths, asset filenames,
raw disassemblies, media, captures, or hashes may be published.

## Machine-verifiable gate

Generate the reference out of tree before placement. Check UTF-8/LF, Markdown
table widths and local links, all eight status masks, all 12 fourteen-byte
SYNC vectors and their VAD/RM fields, decoder parameter counts, the supported
command set, command/parameter wait masks, and the explicit verification
limits. Compare the implementation statements with `io/tsp.c`, `io/tsp.h`,
and `io/videova.c` in the evaluated base. Record that base in the commit/PR
handoff; no expensive behavior run is needed because emulator sources do not
change.

Run before push:

```sh
python3 tools/repo/check_encoding.py --expect utf8
python3 tools/repo/check_eol.py --enforce
python3 tools/repo/check_case.py
git diff --cached --check
```

Review the staged closed scope and absence of private identities. Push the
documentation branch and report the full SHA. No build, boot, hosted CI,
private integration run, bug-fix ledger entry, or archived-reference change
is part of this gate. Passing it establishes document checks, not full TSP
hardware conformance.

## Local validation record

- Evaluated base: `3d8f6e351da448b86e80c2a82286bf65cf05c4d9`.
- Out-of-tree generated-content check: PASS; UTF-8/LF, BSD header, table
  widths, local paths/anchors, eight status masks, twelve SYNC vectors with
  RM/VAD checks, thirteen decoder parameter counts, sixteen decoded commands,
  separate polling masks, verification limits, and private-path scan.
- Generated reference copied byte-for-byte into the publication tree (`cmp`
  passed). The four changed paths are the closed documentation scope above.
- Encoding checker: `0 violation(s)`; EOL checker: `0 violation(s)`;
  case checker: `0 finding(s)`; staged diff check: exit 0, no output.
- No emulator source/build input changes, runtime execution, hosted CI,
  private evidence publication, or archived-reference changes.
