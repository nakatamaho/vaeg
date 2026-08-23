; GLASS ORBIT original work:
; Developed by ChatGPT Plus
; Supervised by SimK, Neko Project 21/W Developer
; Ported By Maho Nakata, 2026.
;
; Copyright (c) 2026 Nakata Maho
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions are met:
; 1. Redistributions of source code must retain the above copyright notice,
;    this list of conditions and the following disclaimer.
; 2. Redistributions in binary form must reproduce the above copyright
;    notice, this list of conditions and the following disclaimer in the
;    documentation and/or other materials provided with the distribution.
;
; THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
; IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
; OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
; IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
; INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
; BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
; USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
; THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
; (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
; THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

; GLASS ORBIT GA-1 bare-payload contract.
; The local test loader copies this image to 2000:0000 and makes a far jump
; there. This image establishes every segment and stack register it uses.
; It deliberately makes no BIOS, TSP, SGP, GVRAM, or DOS call.

cpu 286
bits 16
org 0

glass_ga1_entry:
        cli
        mov     ax, cs
        mov     ds, ax
        mov     es, ax
        mov     ss, ax
        mov     sp, 0xf000
        cld
        sti

        ; Retain the P0 geometry/data closure in the first bare execution.
        call    glass_geometry_step
        mov     ax, 0x4741             ; "GA" marker for the capture checker.
        jmp     glass_ga1_idle

; The debugger captures the pre-HLT state at this fixed entry address.
times 0x0100 - ($ - $$) db 0
glass_ga1_idle:
        hlt
        jmp     glass_ga1_idle

%include "glass_geometry.inc"
%include "glass_data.inc"

; Leave 256 bytes below SP for the bare payload stack. Later GA stages must
; keep code/data/list allocations below this boundary or move the contract in
; a separately reviewed change.
%if ($ - $$) > 0xef00
%error "GLASS ORBIT GA-1 payload overlaps the fixed stack reserve"
%endif
