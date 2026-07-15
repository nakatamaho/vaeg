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
# M38: Add normalized legacy/new Z80 differential evidence

Status: draft; blocked until the maintainer explicitly passes G37

Branch: `topic/m38-z80-differential`

## Goal and harness

Compare externally observable legacy and replacement behavior without changing
production selection. Prefer separate legacy and new runner executables plus a
trace comparator, avoiding class/symbol collisions and production-source
renaming. Emit a versioned deterministic lower-case trace: checkpoint/reason,
authoritative and mirrored registers, IFF/IM/HALT, IRQ/NMI, ordered memory and
masked-I/O transactions, acknowledge events, clocks, end balance, selected
memory hashes, and explicit `Exec()` boundaries.

Normalize at slice return, accepted-interrupt completion, HALT entry/exit,
save/load, completed I/O, and test markers. Do not fail merely because legacy
EI fuses the following instruction; compare at the next common external
checkpoint.

## Corpus and classification

Directed programs cover base, CB/ED/DD/FD/DDCB/FDCB, branches, stack, block
memory/I/O, EI/DI/RETN/RETI, HALT, IM0/1/2, NMI, R, and ZEX-relevant flags.
Event scripts cover DI assertion, EI timing, acknowledge mutation,
deassertion, HALT wake, persistent IRQ, NMI, and frame-boundary state events.
Add bounded deterministic random programs with committed public seeds and a
documented longer local/nightly run.

Classify each divergence as wrapper defect, upstream defect, old-core defect,
deliberate architectural correction, externally inert scheduling difference,
or unresolved. An allowlist requires a minimal input, first event, affected
state, evidence, safety rationale, and issue link where applicable. ZEX alone
does not classify a divergence.

Create `docs/modernization/z80-cycle-deltas.md`, recording scenario deltas and
whether they change event order, FDD behavior, interrupts, WAIT, HALT, or a
guest-visible result. Such effects are failures until resolved.

## Gate G38

Run all repository QA, directed and bounded random corpora, reproducibility,
and the recorded long command. Require green or evidence-backed divergences,
an empty/evidence-backed allowlist, cycle report, and unchanged production
core. Push exact traces/report hashes, commands, files, and SHAs, then stop for
review.
