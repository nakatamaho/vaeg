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

# M145 — Hyper Frame+ initial VA hardware qualification

Status: **IN_PROGRESS; G145 NOT_READY.**
Branch: `milestone/m145-hyperframe-probe`; commit prefix `M145:`.
Base: accepted main at
[cb4edc1a05221971c5a048236eeeb172d2abf73e](https://github.com/nakatamaho/vaeg/commit/cb4edc1a05221971c5a048236eeeb172d2abf73e).

## Assignment and separation from WSR-E

The maintainer explicitly paused WSR-E and requested a separate milestone
trying Hyper Frame+ on the VA. This authorizes switching the active task,
not accepting or merging M143. Its source and evidence remain on the
[separate WSR-E branch checkpoint](https://github.com/nakatamaho/vaeg/commit/5fe759705dec089b471c8ee40bd6506c76f34997),
with G143 NOT_READY. The maintainer suspects WSR-E seating after comparison
with Hyper Frame+; this is an unconfirmed observation, not an established
root cause of the preceding FF reads. Do not request further WSR-E operations
as part of this task, force insertion, or remove retained M143 work/evidence.

This branch starts from accepted main, not the unaccepted WSR-E implementation.
Hyper Frame+ is a different board family. Never apply the Cirrus SR06/CR27,
WSR-E 7Ah handshake, rotary address assumptions or wake sequence to it.
Previously supplied board photographs show Digital Arts markings and custom
logic/DAC parts, not a basis for importing the WSR-E port map.

## Bounded first objective

1. Establish the Hyper Frame+ identification, port map, read side effects,
   switch settings and any required enable sequence from its own sources.
   Keep Hyper Frame, Hyper Frame+ and Super Frame variants distinct.
2. Independently write the smallest VA/V3 MS-DOS 4.0 diagnostic justified
   by that evidence: `cpu 186`, machine ROM word/banner, explicit program
   and build identity, bounded execution and a separate log file. Default
   operation must not write unknown device ports or scan arbitrary I/O.
3. Keep any justified write experiment an explicit opt-in, document port
   ownership and residual state, and never restore unknown FF as a saved
   value. Do not promise complete restoration without readback evidence.
4. Build a separate pinned MS-DOS 4.0 D88, retaining its system files,
   AUTOEXEC and license. No automatic probe at boot; no supplied commercial
   drivers, images or manuals on the distributable probe disk.
5. Validate instruction/I/O/log contracts with isolated controls and actual
   DOS execution where meaningful. Preserve input/output identities, command
   exits, negative outcomes and missing coverage. Synthetic success and an
   emulator's absent-device reads are not hardware validation.
6. Obtain and analyze the returned hardware log with exact COM identity.
   A negative identification is a recorded result, not proof of absence,
   faulty seating, board damage or VA bus incompatibility.

If a safe board-specific access contract cannot be established, stop before
producing an invented probe and report the exact missing input. Do not run
whole multi-board detectors or original installers on the host or VA.
Independent analysis may inspect private supplied software and perform bounded
synthetic execution of identified routines, with no live hardware access.

## Private evidence and hardware handling

Original photos, archives, converted documents, disassembly, returned media,
raw captures and their identities remain outside Git under task-specific
scratch. Tracked analysis uses neutral source IDs and independently written
summaries; publicly available references have pinned provenance.

All card changes require power off and unplugging first. Do not force,
bend, grind, or use mounting screws to pull a card into alignment. The user
chooses installation and switch changes; software observations do not prove
mechanical contact or power integrity.

## Scope and gate

Initial scope is identification/access qualification, not full graphics,
VRAM mapping, BLT, PCM, relay/display routing, performance, an emulator board
implementation, release packaging or publication. Review any necessary
extension of probe write scope before implementation.

G145 remains NOT_READY until the qualified probe, reproducible media and
hardware evidence have been reviewed and the maintainer explicitly accepts
the gate. A source/probe handoff is not gate acceptance and does not waive
applicable repository-wide gate checks. No full media/demo/presenter campaign
or new application executable is claimed by starting this task.

## Initial findings

- A supplied multi-board DOS image viewer explicitly lists HyperFrame/+ as
  built-in support, separately from its external Cirrus/WAB driver. This is
  a candidate reference for static analysis, not a reason to execute the
  whole viewer or assume PC-98 BIOS calls work on the VA.
- Static analysis of that viewer (neutral reference **D1**) follows the MZ
  load/relocation layout, the PC-98 graphics dispatch and three framebuffer
  candidate paths. The paths use the low 00D0h-family ports but have different
  register and pixel-data protocols. Their exact assignment to Hyper Frame,
  Hyper Frame+, Super Frame variants and the other listed board is not yet
  qualified. A shared address is not a shared device protocol.
- These detection paths already include initialization and pixel-data writes;
  they are not harmless presence queries. One also writes outside the
  00D0h-family range. Do not copy them wholesale or label an arbitrary raw
  port scan as a board identification test. Full-program paths use PC-98 BIOS
  and machine work areas that must not be transplanted to VA.
- The VA manual places low 0000h-00FFh ports in system area 0, not the
  1000h-FEFFh user area used in the preceding WSR-E work. Do not assume that
  absence of a vaeg handler proves a real port is available or side-effect
  free. No actual VA access to these candidate ports was made here.
- Exact Hyper Frame+ port/read-side-effect and SW1 contracts: **OPEN**.
  A Hyper Frame+ manual's I/O/SW1 pages, dedicated documented sample source,
  or dedicated setup/display software would allow the family mapping to be
  checked without guessing. The supplied photographs alone do not establish
  those meanings.
- New probe/media production is **BLOCKED pending that qualification**;
  control tests and hardware results are **NOT_RUN**. No new D88, board
  implementation or live original-software execution is claimed.

## Initial checkpoint validation

Documentation-only scope: roadmap plus this task. Encoding/EOL/case/Markdown
comment checks and staged `git diff --check` return 0 (zero reported
violations/findings); milestone-ID selftest/discovery/audit returns 0 with
49 checks passed, 225 tasks, 177 reports and 166 roadmap rows. Command output
is retained in task scratch. No application/probe build or behavioral rerun
is required or claimed for this planning/static-analysis checkpoint.
