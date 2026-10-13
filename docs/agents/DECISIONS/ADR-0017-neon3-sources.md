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
# ADR-0017: Track the NEON RELAY 3 ver1.5 sources for the NEON3 port

## Status

Accepted by the maintainer for M106 on 2026-10-08.

## Context

The NEON3 PC-88VA port (`demos/neon3/`) includes five files of the original
PC-98 NEON RELAY 3 (configuration, city geometry, scene table and data with
its music). Those files lived only in an untracked `demos/neon3_1_5/98/` on
the original development machine, which no longer exists, so the NEON3
payloads could not be rebuilt; in M106 they needed rebuilding for the
measured SGP LINE directions. The author, SimK, publishes NEON RELAY 3
ver1.5 and places the programs on the download page under the modified BSD
licence (2-clause also allowed, except the NEON RELAY 4 cat icon).

## Decision

- Track the six NEON RELAY 3 ver1.5 files that the port's build includes,
  byte-for-byte from directory `NEON3_1_5` of the published archive, under
  `external/neon3-1.5/`. `provenance.txt` records the archive and file
  SHA-256; `LICENSE.txt` records the author's statement and the licence.
- The files are not modified. Port-side changes stay in `demos/neon3/`.
- The NEON3 payloads are built from these sources. They are not compiled
  into or packaged with a vaeg executable; they are guest programs on the
  demo disks.

## Consequences

- The public ver1.5 differs from the lost private copy in parts of the
  city geometry and is 128 bytes larger; the port's unused diagnostic
  reserve was shrunk by that amount so the payload still ends below the
  loader's E000h reserve. The rebuilt NEON3 is therefore not byte-identical
  to the earlier distribution.
- The NEON RELAY 3 SURFACE EDITION archive is a different program and is
  not tracked.
