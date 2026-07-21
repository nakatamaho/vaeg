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
# M55: HOSTFAT GUI, save-state identity, and safety finish

Status: **blocked on explicit G54 approval**

## Goal

Turn the accepted M54 read-only block-device prototype into a persistent,
reviewable frontend feature while preserving its immutable snapshot and
write-protect contract.

## Authorized future scope after G54

- Add an enable switch and host-folder picker to Configure.
- Persist the setting using the portable executable-local/user-state policy.
- Build snapshots without freezing the SDL2/ImGui UI and report progress and
  precise failures.
- Define an explicit unmount/rebuild/reset refresh workflow; never expose a
  changing FAT view to a mounted guest.
- Give each snapshot a deterministic identity and define transactional
  save-state load behavior for matching, missing, and mismatched snapshots.
- Complete canonical-root containment, symlink/reparse-point rejection,
  race-resistant file-copy validation, depth/count/size limits, and diagnostics.
- Add deterministic 8.3 alias generation and an explicitly documented policy
  for host Unicode names that cannot be represented safely.
- Add GUI, state, security, cross-platform, and human regression coverage.

## Hard non-goals

- No host writes, delete, rename, or metadata updates.
- No INT 2FH redirector or dependency on MS-DOS SDA/CDS internals.
- No live sector synthesis from a directory that can change while mounted.
- No silent state substitution when snapshot identity does not match.
- No M55 implementation before the maintainer explicitly passes G54.

## Planned G55 human gate

- Configure, persist, restart, rebuild, unmount, and reset the host drive.
- Confirm UI responsiveness during snapshot creation.
- Confirm matching save-state continuation and fail-closed mismatch handling.
- Repeat root/subdirectory reads and host write-protection checks on Linux and
  Windows, followed by the standard V3/VA-demo/OS gate.
