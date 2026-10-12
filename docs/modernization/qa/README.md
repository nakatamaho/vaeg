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

# vaeg QA

vaeg QA is organised in three levels by the private inputs a run needs. A
check is only as correct as its oracle, so every check below names the
oracle it compares with and what that oracle cannot see.

| Level | ROMs | Disk media | Runs on | Status |
|---|---|---|---|---|
| External | none | none; only pinned public inputs and committed real-machine records | hosted GitHub Actions and any checkout | implemented |
| Local B | maintainer ROMs | none | maintainer machine | defined, not implemented |
| Local A | maintainer ROMs | PC-Engine system media and other private NEC media | maintainer machine | partly implemented (single harnesses, see below) |

CPU-specific boundaries are documented separately:

- [Z80-compatible CPUs](z80-family.md): the uPD9002 uPD70008-compatible
  mode entered with BRKEM, and the FDC subsystem uPD780C.
- [uPD9002 native mode](upd9002-native.md): the V30-family instruction set
  compared with the SingleStepTests V20 corpus.

## Oracle order

When two sources disagree, the higher one decides:

1. Measurements on a real PC-88VA (VA, VA2), recorded byte for byte with the
   program and media identities (`docs/agents/reports/`).
2. NEC documentation and the VA ROMs read statically (monitor tables,
   firmware code), as recorded in `docs/modernization/` and `docs/bios/`.
3. A related reference CPU: the SingleStepTests V20 corpus for the native
   instruction set, and the stock Zilog Z80 CRCs carried by ZEX.
4. The emulator's own earlier output (determinism and regression only).

A pass against level 3 or 4 is evidence about that reference, not about the
PC-88VA. The CPU documents state, per item, which level a check reaches.

## External level

Runs from a plain checkout with no ROMs and no media. Inputs are either built
from tracked sources or fetched at pinned, digest-verified revisions:

| Input | Pin and verification |
|---|---|
| SingleStepTests V20 `v1_native` | upstream commit and per-file SHA-256 in `tests/ssts/v20_dataset_manifest.json` |
| ZEXDOC/ZEXALL binaries | suzukiplan/z80 commit and SHA-256 in `tests/z80_compat/fetch_zex.py` |
| CP/M probe programs | built from `external/zex/` and `tools/cpmva/zex/` with z80asm 1.8 |
| Real-machine records | committed under `docs/agents/reports/m101_zexall_qa/` and `m102_zexund_qa/` |

Hosted CI (`.github/workflows/build.yml`):

| Job | Checks |
|---|---|
| `lint` | encoding, EOL, path case |
| `guest-drivers` | NASM guest-driver bundle and its manifest |
| `upd9002-ssts` | uPD9002 native mode against the V20 baseline |
| `compat-conformance` | Z80 backend contracts, role ZEX and role probe checks |
| `linux` gcc/clang, `linux-asan`, `windows`, `macos` | build, ROM-less `--smoke`, the full CTest suite |
| `windows-release` | release build, smoke, PE import audit |

What no external check can see: anything that needs NEC firmware or guest
media to run — BIOS initialisation, IPL, mode switching under firmware,
display output driven by guest software, FDC traffic through the real
subsystem firmware, sound output, operating systems. `--smoke` without ROMs
runs with its uniform-screen detector disabled.

## Local level B (ROMs, no disk)

Purpose: prove that the real firmware initialises the machine as emulated.
Planned cases: ROM-backed `--smoke` for `va` and `va2`; diskless boot target
per V3/V2/V1 mode checked through `--screen-tvram-dump`; ROM BASIC round trip
where present; determinism of repeated runs; state save/load round trip.
None of these is registered in CTest yet, and the expected diskless screens
are not recorded.

## Local level A (ROMs and PC-Engine media)

Purpose: replace the machine-checkable part of the human gate (boot in V3
mode, run the bundled demo, boot an OS and perform simple operations).

Existing harnesses:

| Harness | State |
|---|---|
| `tools/qa/m103b_basic_boot.py` (V2 BASIC) | registered as `vaeg_m103b_basic_boot`; skipped unless `VAEG_V1V2_ROM_DIR` and `VAEG_V1V2_BASIC_MEDIA` are set |
| `tools/qa/m75_storage_regression.py` run modes (SASI/SCSI/HOSTFAT) | manual only |
| `tools/m74-diagnostics/run_debug_case.sh` | manual only |
| CP/MVA under PC-Engine (`tools/cpmva/`) | manual; the source of the M101/M102 real-machine records |

Planned: PC-Engine V3 boot and prompt, `DIR`/`VER` responses, one bundled
demo run, and the existing harnesses above under one skip contract.

Private inputs stay outside Git; tracked files use neutral identifiers only
(see `AGENTS.md`).
