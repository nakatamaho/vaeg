; Copyright (c) 2026 Nakata Maho
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions are met:
; 1. Redistributions of source code must retain the above copyright notice,
;    this list of conditions and the following disclaimer.
; 2. Redistributions in binary form must reproduce the above copyright notice,
;    this list of conditions and the following disclaimer in the documentation
;    and/or other materials provided with the distribution.
;
; THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
; WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
; MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
; EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
; SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
; WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
; OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
; ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

; Local PC-Engine loader for one GA-6 bare image.  It saves the COM return
; continuation in the bare payload before transfer, so ESC can return through
; the VA Keyboard BIOS path without a DOS interrupt in the payload.

cpu 286
bits 16
org 0x100

%ifndef GLASS_GA6_PAYLOAD_FILE
%error "GLASS_GA6_PAYLOAD_FILE must name the raw GA-6 payload"
%endif

%define LOADER_RETURN_SS        0x0f00
%define LOADER_RETURN_SP        0x0f02
%define LOADER_RETURN_FLAGS     0x0f04

glass_ga6_loader_entry:
        ; Preserve the caller's flags before making the copy/transfer atomic.
        ; The raw ESC return restores these flags with the caller's stack.
        pushf
        pop     bx
        cli
        push    cs
        pop     ds
        mov     ax, 0x2000
        mov     es, ax
        xor     di, di
        mov     si, glass_ga6_payload
        mov     cx, glass_ga6_payload_end - glass_ga6_payload
        cld
        rep     movsb

        push    cs
        push    word glass_ga6_loader_return
        mov     ax, ss
        mov     dx, sp
        mov     [es:LOADER_RETURN_SS], ax
        mov     [es:LOADER_RETURN_SP], dx
        mov     [es:LOADER_RETURN_FLAGS], bx
        push    word 0x2000
        push    word 0x0000
        retf

glass_ga6_loader_return:
        ret

glass_ga6_payload:
        incbin GLASS_GA6_PAYLOAD_FILE
glass_ga6_payload_end:

%if (glass_ga6_payload_end - glass_ga6_payload) > 61184
%error "GLASS ORBIT GA-6 loader payload exceeds the fixed stack boundary"
%endif
