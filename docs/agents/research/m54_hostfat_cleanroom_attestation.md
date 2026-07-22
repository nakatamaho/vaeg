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
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-->
# HOSTFAT clean-room implementation attestation

I independently authored `hostfat.asm` from the clean-room factual contract.
The implementation expression—including all labels, control flow, instruction
choices, data layout beyond mandated byte contracts, comments, and
formatting—is my own and was not copied, adapted, or compared against any prior
implementation or output.

## Inputs consulted

The complete list of informational inputs consulted is:

1. `docs/agents/research/m54_hostfat_cleanroom_spec.md`, read completely from
   line 1 through line 220.
2. My pre-existing general knowledge of NASM syntax and NEC V30/8086-family
   real-mode assembly. I did not consult an external manual, web page, example
   driver, or other source while producing this implementation.

NASM 3.01 was used only to assemble the independently authored source.
Standard local shell utilities were used only to create the isolated working
directory, read the authorized specification, and validate the newly produced
artifact. After authorship, I inspected the header, required strings, size,
and hash of my own generated `hostfat.sys`; that artifact was not an
implementation input and was not compared with any previous output. No other
externally supplied file, document, archive, binary, source, report, history,
web page, or generated output was consulted.

## Prohibited-input affirmation

I affirm that I did not view or consult:

- any `RDBMS*` file, archive, document, source, or binary;
- any existing or historical `tools/pc88va/hostfat/hostfat.asm`;
- any pre-existing or externally supplied compiled `HOSTFAT.SYS`, any
  disassembly of one, or any derived output;
- `tools/pc88va/hostfat/check_driver.py`;
- any M54 report, commit, diff, or Git history; or
- any source implementing an analogous PC-Engine driver.

I did not search the vaeg repository, inspect any other vaeg file, run a Git
command, or compare this work with a previous output.

## Build result

NASM 3.01 assembled `hostfat.asm` successfully as the 512-byte flat binary
`hostfat.sys`.

- `hostfat.asm` SHA-256:
  `0b1d7fb586fe5fdfb2b6077943dbc6eb9bcd14aa6a271d34fe30d1d8ca6d4368`
- `hostfat.sys` SHA-256:
  `665882e22cbf7f3106cfedc6fb2cdae09cf9434296a4f4a9caf2599e2382ec29`

Signed-off-by: OpenAI Codex clean-room implementation agent  
Date: 2026-07-22 (Asia/Tokyo)
