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
# M35: Minimal suzukiplan Z80 interrupt extension

Status: draft; blocked until the maintainer explicitly passes G34

Branch: `topic/m35-z80-upstream-extension`

## Entry and scope

Read the repository rules, roadmap, conventions, master plan, ADR-0011, and
M34 evidence in the prescribed order. Record branch, SHA, and status. Work
against a clean `suzukiplan/z80` checkout based on
`e3926769a790fab0af1c34a5540e317f8d4f0ddc`, not a vaeg vendored copy. vaeg
changes are limited to patch/fork provenance and evidence. Do not vendor,
integrate, write the wrapper, replace disassembly, or change emulator behavior.

## Required extension

Add a general-purpose level-sensitive IRQ setter without changing existing
one-shot API semantics. Assertion persists until explicitly deasserted;
acceptance clears CPU IFF state but not the external line. Add an unsigned
eight-bit acknowledge callback invoked exactly once at actual maskable-
interrupt acceptance, after EI inhibition, never for assertion alone or NMI.
Support normal `std::function` and `Z80_NO_FUNCTIONAL` builds.

For IM0, route the callback byte through the normal decoder as the raw first
opcode. Do not fetch that byte or advance PC; fetch operands and following
prefix bytes from current memory PC. IM1 invokes and ignores the byte before
entering `0x0038`; IM2 invokes once and uses it as vector low byte. Make the
level state inspectable/restorable. Add no broader state API unless direct
implementation evidence disproves ADR-0011's public-state finding.

## Required upstream/fork tests

Cover DI assertion, EI delay, acknowledge value changed by the inhibited
instruction, deassertion before acceptance, persistent assertion across later
EI, HALT wake, and NMI non-acknowledge. Cover IM0 RST, `0x00`, `0x7f`, a
multi-byte control transfer, CB, ED, DD, FD, DDCB, and FDCB with remaining
bytes fetched from memory. Cover IM1 acknowledge and IM2 callback/vector read
order. Run the full existing suite and both callback configurations.

## Upstream-first evidence

Prefer an upstream pull request. Otherwise publish an immutable minimal MIT
fork commit. Record base SHA, result SHA, repository, issue/PR, license, patch
scope, commands, and complete results. If publishing is unavailable, create a
directly applicable `git format-patch`, record its SHA-256, and stop for the
maintainer to supply an approved immutable commit. Do not proceed to M36 on an
uncommitted working tree or mutable branch name.

## Gate G35

G35 requires review of the focused patch, unchanged MIT license, all upstream
and focused tests green in both callback configurations, and an approved
immutable upstream/fork commit. Push the M35 vaeg evidence branch, report exact
SHAs and status, and stop.
