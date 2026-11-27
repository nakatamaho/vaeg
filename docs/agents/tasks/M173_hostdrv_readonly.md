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

# M173 — HOSTDRV read-only: transport, guest redirector and host backends

**Status:** assigned 2026-11-27 by the maintainer; branch
`milestone/m173-hostdrv-readonly` from `main` (`ed5899a9`).
The v1 packet, shared-port and transient DOS CDS/SFT fixtures are
implemented. Linux file authorization, pinned CP932 naming and the Linux
DOS namespace are implemented, including the Shift-JIS alias amendment.
The Linux wire service and lifecycle adapter are wired to `--hostdrv-dir`.
The guarded 8086 wire helper, opcode-specific reply validator and transient
DOS profile and CDS selection probes are implemented. The guarded
resident-stack entry and CDS dependency scan are wired into a development
`HOSTDRV.COM` with opt-in install/remove, a CP932 profile check and owned
chdir/attribute queries, ordinary file handles, enumeration and capacity.
Extended open now publishes the action in saved user CX under the approved
DOS IFS contract. The approved empty-list `112Dh` query is implemented.
The host-internal native RAM span query is implemented and tested. The
approved CPU-store amendment removes the upper conventional RAM blocker
for the saved DOS frame and output buffers, without widening host copies.
Minimal read-only FCB allocation, transfers, search and cleanup are
implemented. The remaining functional and lifecycle checks, Windows
backend and static G173 handoff remain open. See the
[progress report](../reports/m173_hostdrv_readonly.md) for remaining gates.
Not ready for G173.
**Maintainer-approved exception (2026-11-27):** M173 may begin while G171
is pending. This neither accepts G171 nor waives G173 or any merge or
release approval. The one-task-per-session rule still applies.

## Basis

The approved M172 design, `docs/agents/research/m172_hostdrv_design.md`
(G172 passed 2026-11-27; decisions (a)–(d) as recommended). This milestone
is the design's **read-only milestone** (item 2 of its split), staged as
decision (c) requires: prove the DOS CDS/SFT ABI and the v1 transport with
a small synthetic fixture first, then the host backends, then guest
behaviour. No folder GUI in this milestone (session command-line option
only; GUI and persistence follow later). No write verbs.

## Scope

- **Host (VAEG):** v1 protocol on the 07EDH/07EFH channel exactly as
  designed (response marker, sequence, versioning, emulator-only
  discovery, exclusivity, error mapping, handle model, states installed /
  detached-but-resident / uninstalled with the save-state and reset
  rules); read-only backend for **Linux and Windows** with the designed
  containment (fail closed), type-before-data rejection, Shift-JIS ↔ UTF-8
  naming with stable 8.3 aliases and identity rules; macOS only if the
  POSIX path shares every guarantee, otherwise disabled with a clear
  message. A session option (e.g. `--hostdrv-dir PATH`). Never write the
  host (including times).
  M173 naming amendment (maintainer approved, 2026-11-27): use folded
  Shift-JIS alias prefixes and extensions per the design amendment; test
  Japanese and mixed names, truncation at a lead byte, `~10` growth,
  unmappable characters and single-byte half-width katakana.
- **Guest:** `HOSTDRV.COM` for MS-DOS 4.0 VA (NASM, 8086-level like
  HOSTFAT): install on a free drive (or one given), refuse cleanly when
  LASTDRIVE is exhausted or discovery fails, service the read-only
  INT 2FH AH=11H verbs per the design (handles, EXT open read-only,
  find-first/next, seeks, attributes read, disk info, close/process-exit
  cleanup that always retires DOS references), minimal FCB support per
  decision (a), mutating verbs answered with write-protect/access denied
  as designed; uninstall.
- **Synthetic fixture** proving CDS/SFT and transport before folder work,
  as the design's read-only milestone lists, with the listed negative
  cases (unspecified opcodes/masks/lengths, stale success dword, wrong
  marker/sequence, busy refusal).

## Checks and G173

- Automated: unit/protocol tests ROM-free where possible; CTest entries;
  repository checks; the design's read-only test list, each item either
  run or marked NOT RUN with reason.
- Guest checks (VAEG VA and VA2, MS-DOS 4.0 VA): DIR, CD, TYPE, COPY to a
  local drive, child execution and redirection, Japanese file names,
  live additions/removals, JFP/FEP coexistence, SUBST/JOIN, LASTDRIVE
  exhaustion, uninstall, both reset paths, save/load rules.
- G173 human gate: the standard clean static MinGW handoff; the maintainer
  runs `HOSTDRV` on MS-DOS 4.0 VA under the Windows build with a host
  folder and performs simple operations. Open a PR; do not merge before
  G173. Follow AGENTS.md on media/test-result disclosure.
