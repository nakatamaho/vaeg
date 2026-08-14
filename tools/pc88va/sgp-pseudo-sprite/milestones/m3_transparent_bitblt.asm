; Copyright (c) 2026 Nakata Maho
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions
; are met:
; 1. Redistributions of source code must retain the above copyright
;    notice, this list of conditions and the following disclaimer.
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
; USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
; ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
; (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
; THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

; M3 educational excerpt: one transparent SGP BITBLT; not a standalone COM.

; SGP command words live in the DOS-loaded main-RAM COM image. The complete
; source converts DS:offset to the SGP physical address before writing the
; command pointer to ports 0500h-0503h.

SGP_COMMAND_SET_WORK   equ 0003h
SGP_COMMAND_SET_SOURCE equ 0004h
SGP_COMMAND_SET_DEST   equ 0005h
SGP_COMMAND_BITBLT     equ 0007h
SGP_COMMAND_END        equ 0001h
SGP_BITBLT_COPY_XPAR   equ 0105h       ; TP-MOD=1, copy ROP=5
G1_PAGE_A_SGP_BASE     equ 220000h

m3_command_list:
    dw SGP_COMMAND_SET_WORK
    dw work_area_low, work_area_high
    dw SGP_COMMAND_SET_SOURCE
    dw 0001h, 8, 8, 4                  ; 4-bpp, 8x8, source pitch 4
    dw sprite_bitmap_low, sprite_bitmap_high
    dw SGP_COMMAND_SET_DEST
    dw 0001h, 8, 8, 160                ; aligned destination, pitch 160
    dw G1_PAGE_A_SGP_BASE & 0xffff
    dw G1_PAGE_A_SGP_BASE >> 16
    dw SGP_COMMAND_BITBLT
    dw SGP_BITBLT_COPY_XPAR
    dw SGP_COMMAND_END

; Color index zero is both the source skip value and G1's composition mask.
; The nonzero diamond is copied by SGP; zero corners leave the G0 pattern.
m3_sprite_bitmap:
    db 00h, 00h, 30h, 03h
    db 00h, 35h, 55h, 03h
    db 35h, 55h, 55h, 53h
    db 35h, 55h, 55h, 53h
    db 00h, 35h, 55h, 03h
    db 00h, 00h, 30h, 03h
    times 2*4 db 0
