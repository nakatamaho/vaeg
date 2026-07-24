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
# uPD9002 semantics campaign renumbering after G56

## Reason

The repository used M52-M56 for work unrelated to the uPD9002 semantics campaign. The maintainer has
accepted G56 as passed because the intended G56 implementation was not feasible. Historical M52-M56
records must remain untouched.

The semantics campaign is shifted by +5 while preserving dependency order:

| Previous package | Updated package | Purpose |
|---|---|---|
| M52 / G52 | M57 / G57 | Remove frozen Win9x/i286x tier |
| M53 / G53 | M58 / G58 | SST ratchet, epoch, and gap taxonomy |
| M54 / G54 | M59 / G59 | Semantic evidence pack |
| M55a / G55a | M60a / G60a | FLAGS materialization |
| M55b / G55b | M60b / G60b | Synchronous interrupt frame |
| M55c / G55c | M60c / G60c | IRET |
| M56 / G56 | M61 / G61 | Canary/shared EA-decode primitive |
| M57a / G57a | M62a / G62a | AAM |
| M57b1 / G57b1 | M62b1 / G62b1 | 0F2A |
| M57b2 / G57b2 | M62b2 / G62b2 | Conditional 0F28 |
| M57c / G57c | M62c / G62c | BCD adjust family |
| M58 / G58 | M63 / G63 | Shift family |
| M59 / G59 | M64 / G64 | DIV/IDIV |
| M60 / G60 | M65 / G65 | Residue re-plan |
| M61a / G61a | M66a / G66a | Drop CPU286 state compatibility |
| M61b / G61b | M66b / G66b | Remove active I286/i286c identity |
| M62 / G62 | M67 / G67 | Divergence consolidation |

## Starting-gate rule

The exact approved G56 SHA must be obtained from the repository's approved G56 report or ROADMAP.
Do not substitute G51, current HEAD, a branch tip, or an inferred SHA. If one exact approved SHA is
not available, stop before M57.

G56's acceptance as "implementation impossible" is historical status only. It does not waive any
M57-M67 gate, SST comparison, or evidence requirement.
