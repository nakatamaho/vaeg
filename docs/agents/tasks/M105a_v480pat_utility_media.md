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

# M105a - V480PAT on utility FDD and SASI HDD media

Maintainer-authorized packaging follow-up to completed M104/M105, based on
`main`; commit prefix `M105a:`. No dependency on the pending voyage gates.

## Scope

- Build the unchanged `tools/pc88va/vtiming/v480pat.asm` with NASM.
- Install `\BIN\V480PAT.COM`, `\SRC\VTIMING\V480PAT.ASM`, and
  `\DOC\VTIMING.TXT` on the generated utility floppy and both SASI variants.
- Stage the same files through the common development-tool staging manifest;
  install the floppy COM after DIET so it stays byte-identical to NASM output.
- Keep the source in its existing repository location. HOSTFAT is optional
  for transfers, not a source container or a prerequisite for V480PAT.
- Explain the measured 320x224/320x240 and near-60-Hz examples, monitor safety,
  host NASM build, and the distinction between timing restoration and the
  graphics mode left in place by this test.

No emulator, guest assembly, driver, boot configuration, archived reference,
private asset, or tracked binary changes. Existing private media are not
modified in place. Generated PC-Engine media stay outside Git.

## Gate G105a (machine-verifiable packaging only)

Run:

```sh
python3 tools/pc88va/test_vtiming_media.py -v
bash -n tools/pc88va/stage-vtiming.sh tools/pc88va/stage-development-tools.sh \
  tools/pc88va/build-utility-disk.sh tools/pc88va/build-sasi-utility-disks.sh
python3 tools/repo/check_encoding.py
python3 tools/repo/check_eol.py
python3 tools/repo/check_case.py
git diff --check
```

Tests rebuild the staged source, require exactly the three selected paths,
and read all three files back byte-for-byte from a synthetic utility D88
and generated SASI HDDs both with and without a transplanted floppy payload.
Builder-wiring checks cover both SASI variants and the post-DIET floppy copy.
No private integration assets or hosted CI are needed for this gate.
CMake and clean MinGW builds are not applicable: no emulator build inputs
change. M104/M105 runtime and real-hardware evidence are reused, not rerun or
extended; this gate establishes packaging only, not a new boot or CRT claim.

Paste actual check output into the PR and review the final commit scope before
pushing. Stop after this task; do not merge the PR without maintainer approval.

## Local result and limits

All three packaging tests and the shell syntax, encoding, EOL, case and diff
checks pass. The synthetic media contain three V480PAT files totaling 30,211
bytes, including the unchanged assembly source and licensed instructions.

A full common-stager run using the cached public ISH package stops before
V480PAT staging: Lhasa extracts lowercase `ishva.com`, while the existing
stager requests `ISHVA.COM`. The identical failure (exit 1, missing package
member) was reproduced with the unmodified `main` stager. This pre-existing
archive-case issue is not corrected here. Therefore full private utility
FDD/HDD regeneration and guest boot are not claimed; the isolated source build,
filesystem installation/readback, manifest consumption and builder wiring are
verified. No hosted CI or unchanged emulator builds were requested.
