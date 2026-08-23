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

; GLASS ORBIT GA-2 packed-4bpp CPU-fill proof.
; The local test loader copies this image to 2000:0000 and makes a far jump
; there. Graphics BIOS owns entry to 640x200 single-plane 4bpp mode; CPU
; writes only the documented GVRAM mapping after that call.
; Evidence: docs/port/glass_ga2.md.

cpu 286
bits 16
org 0

%define VIDEO_BIOS_INT          0x8f
%define PORT_MEMORY_MAP         0x0153
%define PORT_GVRAM_WRITE_MODE   0x0580
%define MODE_G0_640X200_4BPP    0xa002
%define PIXEL_SIZE_G0_4BPP      0x0004
%define COMPOSE_G0_ONLY         0x0003
%define MEMORY_MAP_GVRAM_SINGLE 0x54
%define GVRAM_CPU_WRITE_MODE    0x10
%define G0_PAGE_WORDS           0x7d00

glass_ga2_entry:
        cli
        mov     ax, cs
        mov     ds, ax
        mov     es, ax
        mov     ss, ax
        mov     sp, 0xf000
        cld
        sti

        ; $ScnMode: single-plane, G0 640x200, graphics enabled, 4 bpp.
        mov     bx, MODE_G0_640X200_4BPP
        mov     cx, PIXEL_SIZE_G0_4BPP
        xor     dx, dx
        xor     ax, ax
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     glass_ga2_failed

        ; Reset the documented palette, keep only G0 in the composition,
        ; and enable the graphics display. No BIOS drawing primitive is used.
        mov     ax, 0x0a00
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     glass_ga2_failed
        mov     ax, 0x0300
        mov     cx, COMPOSE_G0_ONLY
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     glass_ga2_failed
        mov     ax, 0x0b01
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     glass_ga2_failed

        ; Map CPU writes to the G0 single-plane GVRAM aperture at A000:0000.
        mov     dx, PORT_MEMORY_MAP
        mov     al, MEMORY_MAP_GVRAM_SINGLE
        out     dx, al
        mov     dx, PORT_GVRAM_WRITE_MODE
        mov     al, GVRAM_CPU_WRITE_MODE
        out     dx, al

        mov     ax, 0xa000
        mov     es, ax
        xor     di, di
        mov     ax, 0x5555             ; palette index 5 in all four nibbles.
        mov     cx, G0_PAGE_WORDS      ; 640 * 200 / 4 pixels per word.
        rep     stosw

        ; Bytes 12h,34h,56h,78h prove high-nibble-first packed pixels:
        ; the first eight dots are palette indices 1,2,3,4,5,6,7,8.
        mov     word [es:0], 0x3412
        mov     word [es:2], 0x7856

        push    cs
        pop     es
        mov     ax, 0x4742             ; "GB" GA-2 success marker.
        jmp     glass_ga2_idle

glass_ga2_failed:
        mov     ax, 0x47e2             ; debugger-visible failure marker.

; The debugger captures the state at this fixed address after the visible
; framebuffer write has completed.
times 0x0100 - ($ - $$) db 0
glass_ga2_idle:
        hlt
        jmp     glass_ga2_idle

%if ($ - $$) > 0xef00
%error "GLASS ORBIT GA-2 payload overlaps the fixed stack reserve"
%endif
