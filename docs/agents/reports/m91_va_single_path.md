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
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
-->

# M91: native V3 VA single-path report

## Status

M91 is in progress on topic/m91-va-single-path. G91 is pending.

## Evidence boundary

The maintainer-provided docs/tekumani and docs/98io trees are local, read-only
reference material and are not copied into this branch. Tekumani is the
authority for built-in PC-88VA functions. The PC-98 files identify inherited
implementations that only share an address with VA hardware.

## Initial audit

| Area | Existing implementation | M91 decision |
| --- | --- | --- |
| I/O dispatch | io/iocore.c owns separate common and va maps selected by iomode_va. | Make the former VA map canonical and remove the runtime selector. |
| Mode control | Emulator-private port FFD0H changes iomode_va and memmode_va. | Remove the selector and its port binding. |
| 005CH-005FH | io/artic.c implements the PC-98 ARTIC timestamp/wait interface described by docs/98io/io_tstmp.txt. | Remove it. Tekumani assigns the ports to VA1/V2 GVRAM selection/status, which M91 defers. |
| CGROM | io/cgrom.c exposes the inherited PC-98 00A1H-class window; io/cgromva.c exposes VA V3 014CH-014FH. | Remove io/cgrom files; retain io/cgromva files and fontmem. |
| CPU memory | cpu/upd9002/memory.c selects an inherited map or memoryva through memmode_va. | Make memoryva unconditional and use explicit raw main-RAM helpers. |
| Initialization | iocore reset/bind tables initialize inherited handlers before VA handlers. | Retain shared state used by VA devices, but bind only VA and separately owned expansion routes. |

## Port-number collision rule

005CH-005FH demonstrates why numeric matching is insufficient. Tekumani
documents VA1/V2 GVRAM plane selection and status there. The inherited source
implements a PC-98 timestamp counter and hardware wait. The semantics are not
compatible, so M91 removes the PC-98 implementation without pretending that it
implements the deferred VA1/V2 feature.

## Validation

Pending implementation.

## Gate

G91 remains a human gate. No V3 boot, bundled-demo, OS, or device result is
claimed until the maintainer tests the final pushed candidate.
