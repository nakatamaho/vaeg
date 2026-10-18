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

Status: in progress. Stop for maintainer review before merging to main.
