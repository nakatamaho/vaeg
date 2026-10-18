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

# M126 — requested Softlib packages on utility HDDs

Maintainer-authorized main-based packaging follow-up. M125 is explicitly
on hold pending its human gate; this task does not merge M125 or change
emulator behavior.

## Scope

Install the four requested Softlib entries on both generated SASI HDDs:

- VA3DDEMO (`anum=1&gnum=23`): preserve source and documentation, build EXE
  using the pinned JWasm DOS package. Remove only the duplicate `stack`
  keyword in a temporary build copy; retain the original unchanged.
- CPMVA (`anum=2&gnum=424`): already installed; verify its files remain.
- TENIM3 (`anum=2&gnum=74`): preserve the self-extracting archive and manual,
  extract all game files into a dedicated directory. It runs through BASIC,
  not by treating the archive as the game executable.
- PCODESRC (`anum=2&gnum=406`): preserve the archive and source and build
  PCODE.COM; never load this resident BASIC-specific utility automatically.

The maintainer additionally requested the repository's FPU test and Mandelbrot
BASIC sources. Install `BASIC/FPUTEST.BAS` and `BASIC/MANDEL.BAS` with CRLF
line endings and their original license comments, plus `DOC/BASIC.TXT`.

FDD, emulator, boot configuration and existing private images are unchanged.
Public downloads are pinned by hash. Generated software and private media
remain outside Git. Known VA3DDEMO blank-display behavior remains unresolved;
packaging must not be reported as an emulator correction.

## Gate G126

Machine-verifiable packaging gate, following M105a: shell syntax, repository
encoding/EOL/case/Markdown checks, staging twice with identical payloads,
source/archive readback and expected DOS binary checks, negative hash test,
and full regeneration/readback of both private SASI images. Record commands,
exit status, image hashes and limitations here. No emulator build inputs
change, so no unchanged CMake, static MinGW or hosted emulator builds are
required. Runtime and original-hardware compatibility are not gate claims.
The maintainer requested this as a main fix and approved prioritizing it over
M125; integrate the packaging-only change after the machine gate passes.

## Verification result

Packaging evaluated at `4b26db16f9b6e3d909b20b723c5cc4b7caa04c4e`, in a
fresh local clone. Later usage-text/report edits do not alter staging code,
package identities, assembler or BASIC sources.

Commands (private input values are intentionally omitted):

```sh
python3 tests/pc88va/test_softlib_additions.py -v
python3 tools/pc88va/test_vtiming_media.py -q
bash -n tools/pc88va/build-sasi-utility-disks.sh
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
python3 tools/repo/check_md_comments.py
git diff --check
tools/pc88va/build-utility-disk.sh --source "$PCE110" --output "$NEW_FDD"
tools/pc88va/build-sasi-utility-disks.sh --source-va "$PCE105" \
  --source-va2 "$PCE110" --payload-d88 "$NEW_FDD" --output-dir "$NEW_HDDS"
```

All commands exited 0. Four Softlib tests pass, including two independent
staging runs with byte-identical results and a synthetic FAT12 HDD readback.
Three existing V480PAT packaging tests pass. Repository checks report no
findings. Mutations disabling hash checks and omitting BASIC CRLF conversion
each fail their intended focused test; the normal tests pass afterward.

Full local regeneration produced a utility FDD with 4096 bytes free, and
both complete SASI variants. All 129 FDD file payloads match the prior
utility FDD byte-for-byte (directory timestamps ignored). Thirty requested
and existing CPMVA files were read from each real generated HDD and matched
the staged/public originals byte-for-byte, including both BASIC sources,
all eleven TENIM3 members, unchanged VA3DDEMO source, and built EXE/COM.
The destination is a new local output directory; older media were not
modified. Generated output SHA-256:

| Output | SHA-256 |
|---|---|
| Utility FDD (regression only) | `9594f2a7d4d85ed51f68328cf812776494139321d5faaf0d07e0065ab2d98f05` |
| SASI VA | `d641c82ebebdaf1fee4a900a555be62c39a3b9d4c675eacc8c13c92a6de1c352` |
| SASI VA2 | `91caacb65ba1062bc561daa744958b62172cf2ea54207933ec7ac1502ddeee07` |

The existing M124 demo-build/runtime results are reused, not rerun:
`git diff --name-only 9af8cb4ea808f617251cf8ec72812ded3af1197d HEAD`
contains only task/roadmap/media documentation, the SASI wrapper, the new
HDD staging helper and its tests. No demo, emulator, vendored, FDD-builder,
shared-stager or build-system input changed. This is packaging-only
validation and does not establish new guest-runtime results. In particular,
the pre-existing VA3DDEMO blank-display issue remains open.

**G126 machine packaging gate passed.** Main integration was requested by
the maintainer; M125 remains on hold. Merged to main at
[9d3b927c](https://github.com/nakatamaho/vaeg/commit/9d3b927cdebcc4788efb1c44f09bb68b30171fb2).
Implementation: [4b26db16](https://github.com/nakatamaho/vaeg/commit/4b26db16f9b6e3d909b20b723c5cc4b7caa04c4e);
verification record: [df296dde](https://github.com/nakatamaho/vaeg/commit/df296dde4ebd721baa224082e1ad8c02e34c386b).
