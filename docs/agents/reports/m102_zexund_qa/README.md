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
# M102 undocumented-opcode QA outputs

Outputs of the M102 CP/M programs run from a generated
`cpmva-zexall-test.d88` under CP/MVA, in vaeg and on a real NEC PC-88VA2 in V3
mode (the µPD9002 Z80 emulation mode entered with BRKEM). The layout follows
[`m101_zexall_qa`](../m101_zexall_qa/README.md): `raw/` holds byte-exact
output files (lower-cased names, binary in `.gitattributes`),
`manifest.sha256` their SHA-256, and `compare.txt` the full output of
`tools/cpmva/zex/compare.py raw --ref <ref>`; sections without a file are
reported as `SKIP`.

Files were extracted with `tools/cpmva/zex/extract_d88.py --lowercase
--skip-com`. The real-machine disks were written and read with a KryoFlux
and converted to D88 with the HxC Floppy Emulator software. The PC-Engine
boot disk is private and not identified.

## Run 1: `ZEXIY`, `ZEXUND`, `ZEXED`

- Disk before the run (installer at `5f100c92c0855c06e90771cf0cf821044e13f0d8`):
  `2a19f6f73036a81a45a746f0d305d74aec8faec6793f1f27a81725016e5b5581`.

### `run1_vaeg/`

- vaeg: fully static MinGW `vaeg.exe` from `5f100c92`, SHA-256
  `03dea3f0469cf13c43f13d3790e45ad3e9f9bf3b32f31bf260692bd21f3a07c6`.
- Disk after the run:
  `144e0a8f957359f7908a2b3638cd84cad8a08607ec8e7171eef1e1dae6e0ee62`.
- `compare.txt` uses the host reference of `5f100c92`: all three files are
  byte-identical to its uPD9002 outputs.

### `run1_pc88va2/`

- Disk after the run:
  `432d4d3b750ed31db93e0bdc46baaa73e5cd50d9e630a5efffb8b5c5776e6338`. The
  medium was faulty: cylinder 7 could not be written, so the maintainer
  filled it with `BAD2A2E.SYS` and copied `ZEXUND.COM` elsewhere (the copy
  that ran is identical to the generated program), and three sectors in
  unallocated blocks 122, 126 and 136 read with CRC errors. Extraction used
  `--allow-unused-errors`.
- Only `ZEXIY.TXT` was closed. It is byte-identical to the uPD9002 host
  output, which resolves O3 (IYH/IYL behave like IXH/IXL: Zilog plus R1).
- `ZEXUND` and `ZEXED` did not close their files. Their results, transcribed
  from a photograph of the screen:

  | Program | Group | Result |
  | --- | --- | --- |
  | `ZEXUND` | `rot (ix,iy+d)->r (d7)` | OK |
  | | `rot (ix,iy+d)->r (ff)` | expected `15ef6011`, found `c3095ab8` |
  | | `bit n,(ix,iy+d) all (d7)` and `(ff)` | expected `05dd9c19`, found `8930d9be` |
  | | `res,set (ix,iy+d)->r (d7)` and `(ff)` | expected `30bdcc4c`, found `59e0b8dc` |
  | | `neg all encodings (d7)` | no result; the program returned to the CP/M prompt by itself |
  | `ZEXED` | `ed 00-3f nop (d7)` and `(ff)` | expected `69eff883`, found `5c757184` |
  | | `ed 77,7f nop (d7)` and `(ff)` | expected `04bc3bb4`, found `1b78e549` |
  | | `ed 80-9f nop (d7)` and `(ff)` | expected `b44cca1b`, found `735d94ec` |
  | | `ed a4-bf holes nop (d7)` | the machine hung |

  With R12/R13 (DD/FD CB register forms) the vaeg uPD9002 profile gives
  exactly the transcribed `rot`, `bit` and `res,set` CRCs.

## Run 2: `CBPRB`, `EDPRB`

- Disk before the run (installer at `e1d34eeede0aba201e9a28a88e5b2d04c70f48fb`):
  `03fb57c2ac174cbd4f9b8cf11497c5b73309b5a0f6fe8975ad8c397bbfbf2f79`.

### `run2_vaeg/`

- vaeg: fully static MinGW `vaeg.exe` from `e1d34eee`, SHA-256
  `4a3f8408ed654f0208d303587198fd3405cf6ed0db169c4533a017b5d5879331`.
- Disk after the run:
  `7b8b1bc201ab4fe49b929fb0db8cb1b843064a768fd6acd94e2f62decba383d3`.
- `compare.txt` uses the host reference of `e1d34eee`: both files are
  byte-identical to its uPD9002 outputs.

### `run2_pc88va2/`

- Disk after the run:
  `4997e106f8f0aa913b7404f99b3f34a78f58b0779f46df761548afdefa6f44e5`
  (no sector errors).
- `CBPRB.TXT` is complete (1,024 lines). It is byte-identical to the uPD9002
  host output of `f3baa4dd4c2ab9a00a12861910e91b4946d5f943`, which includes
  R12/R13.
- `ED00.TXT` holds 67 records: the controls ED 44 and ED 45, ED 00–3F and
  ED 4C. The screen then showed `ED 54 Unknown error is detect`, and CP/MVA
  returned to the PC-Engine prompt (photograph); the remaining opcodes were
  not run.

## Host references

| Used for | Commit | `vaeg_cpm_runner` SHA-256 |
| --- | --- | --- |
| `run1_vaeg` | `5f100c92c0855c06e90771cf0cf821044e13f0d8` | not recorded (superseded build) |
| `run2_vaeg` | `e1d34eeede0aba201e9a28a88e5b2d04c70f48fb` | not recorded (superseded build) |
| `run1_pc88va2`, `run2_pc88va2` | `f3baa4dd4c2ab9a00a12861910e91b4946d5f943` | `47e31ccea1c9cd47bb266be51428af594f39f1e5b091fff214377eb78b79c7e9` |

All were produced by `tools/cpmva/zex/host_reference.py` (Linux GCC
`linux-ci-gcc` build, exit status 0) and are regenerated by the same command
at the listed commit.
