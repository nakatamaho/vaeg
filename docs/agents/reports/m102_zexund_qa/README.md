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

## Run 3: `EDPRB` restarts

Continuation of run 2 on the same disk: after each stop the maintainer
rebooted and ran `EDPRB xx` with the next opcode. Disk after the runs
(read 2026-10-01, no sector errors):
`0997ae95bc58bf1b32cd20413081189ab352b7fd8fddee1025e5ae5b224903f1`.
Directory entry 37 was corrupted (user area 07h, unprintable name,
raw `07100074078b5f020bdb74128b0042` + zeros), presumably written while a
probe crashed; extraction used `--drop-entry 37`. `cbprb.txt` and
`ed00.txt` are byte-identical to the run-2 copies.

### `run3_pc88va2/`

- `ed5e.txt`, `ed60.txt` and `ed70.txt` overlap; their shared records are
  identical. Non-empty files: 00, 5e, 60, 70, ad, b5, b6, fd.
- An empty `edxx.txt` only shows that the run was started; the file is
  created before the first probe, so it does not by itself prove that
  opcode xx crashed.
- Terminations, transcribed from the maintainer's notes (details that the
  notes did not record are marked unknown):

  | Start/opcode | Observation |
  | --- | --- |
  | 54, 55 | left CP/MVA for the PC-Engine prompt (run 2 and notes) |
  | starts 56–5B | `EDPRB` itself skips to 5C |
  | 5C | machine hung |
  | 5D | `Bdos Err On A: R/O`, no record written |
  | AC | left CP/MVA (notes say "exit"; exact screen not recorded) |
  | AE, B4 | returned to the `a>` prompt at once |
  | AF, B5 | machine hung (B5 closed one record first) |
  | B7 | `Bdos Err On A: R/O` after the B6 record |
  | BC, C3–CB, CD, CE, D0–DF, E0–EC | printed its own and the next probe's header, then returned to `a>` |
  | EE–FC | left CP/MVA for the PC-Engine prompt |
  | C0–C2, CC, CF, BD–BF | not recorded |

- The run-3 analysis found a probe-design flaw: `EDPRB` sets BC=0B0Ch and
  DE=0D0Eh, which point into the program's own code, so records of opcodes
  that write through those registers can reflect self-modification. `EDPRB2`
  (run 4) supersedes it with sandboxed pointers.

## Run 4: `EDPRB2`, `EDPRB2S`

- Disk before the run (installer at `0dbce3c71c84c1c8ae72ce596bfc5800e8a2a544`):
  `40f8292c7e1961982a2c32addc81c0452f553502be82d3a10bf6a3945f454c94`.
  The write to the physical medium never verified completely; the last
  attempt differed on cylinders 9 and 15, side 0.

### `run4_pc88va2/`

- `EDPRB2S` completed: `s200.txt` holds all 27 records.
- `EDPRB2` stopped at `ED 5C 0`; a key press returned to the Ready prompt.
  After a reboot, CP/MVA reported `Bdos Err On A: Bad Sector` when the test
  disk was logged in, and the run was aborted — consistent with the
  unverified write, so the stop at 5C matches run 3 but the aborted rest of
  the run is not evidence about the opcodes.
- Disk after the run (read 2026-10-02, all 1280 sectors good):
  `c5309cbc469067fdc96c5388029e1492d390d39fd34698ba336cabed23fb9778`.
- `e200.txt` holds 201 records: ED 44, 45, 00–3F and 4C, each with input
  sets 0–2.
- First findings (see `compare.txt`; the vaeg profiles implement none of
  this yet, so real records differ from both references):
  - ED 00–3F write an input-dependent value to the pair selected by bits
    5–4 (set 0: 2800h, set 1: 232Ah, set 2: A426h) and set F to 04h/00h/80h.
    `EDPRB2S` reproduces the same values with the code moved by 100h bytes,
    so the value is not an address of the program; it also does not depend
    only on A and F, because run 3 got 2329h from the same A and F with
    other pointers and buffer contents.
  - ED 64/6C change A and (HL) exactly like RRD/RLD but also change (HL+1).
  - ED 80 copies two bytes from (HL) to (DE) with BC−1, DE+2, HL+2,
    confirming the word-transfer reading of run 3.
  - `R=` return addresses equal the first sled byte (`L=0000`): these
    opcodes consumed no operand bytes from the sled.

## Host references

| Used for | Commit | `vaeg_cpm_runner` SHA-256 |
| --- | --- | --- |
| `run1_vaeg` | `5f100c92c0855c06e90771cf0cf821044e13f0d8` | not recorded (superseded build) |
| `run2_vaeg` | `e1d34eeede0aba201e9a28a88e5b2d04c70f48fb` | not recorded (superseded build) |
| `run1_pc88va2`, `run2_pc88va2` | `f3baa4dd4c2ab9a00a12861910e91b4946d5f943` | `47e31ccea1c9cd47bb266be51428af594f39f1e5b091fff214377eb78b79c7e9` |
| `run3_pc88va2`, `run4_pc88va2` | `0dbce3c71c84c1c8ae72ce596bfc5800e8a2a544` | `4134146c4ee4be72db56411609f90a0bc5457869f1737868cdf1c69e9d8fe1c0` |

All were produced by `tools/cpmva/zex/host_reference.py` (Linux GCC
`linux-ci-gcc` build, exit status 0) and are regenerated by the same command
at the listed commit.
