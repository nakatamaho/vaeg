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

# M106 — Publish the INT 80H BIOS analysis

## Maintainer request and scope

Convert the existing private static FDD BIOS analysis to Markdown under
`docs/bios/`, commit it, and emphasize that canonical placement in `AGENTS.md`.
This task publishes independently written analysis only. It does not change
emulator behavior, resolve a guest defect, or approve another implementation
milestone.

## Deliverables

- `docs/bios/bios_int80h.md`: functions, ROM entry points, conventional-memory
  work layout, PC-Engine replacement path, and explicit verification limits.
- `AGENTS.md`: canonical directory and lowercase interrupt-based naming.
- A documentation-only milestone entry in the roadmap.

Keep private paths, asset filenames and hashes, raw disassemblies, ROMs,
manual copies, and disk images outside Git. Retain neutral source roles and
code offsets so the findings remain distinguishable from runtime evidence.

## Machine-verifiable gate

Run `tools/repo/check_encoding.py`, `tools/repo/check_eol.py`,
`tools/repo/check_case.py`, and `git diff --cached --check`.
Review the staged scope and Markdown tables, and confirm that no private
asset identities or payloads are staged. No build, boot, or hosted CI is
needed: the commit must change documentation only. Push the review branch
and report its exact commit SHA; do not claim a merge to `main`.
