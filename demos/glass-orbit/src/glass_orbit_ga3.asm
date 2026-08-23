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

; GLASS ORBIT GA-3 palette and color-bar proof.
; The local test loader copies this image to 2000:0000 and makes a far jump
; there. Graphics BIOS owns mode and palette setup. CPU writes the diagnostic
; palette-index bars only after that setup; no SGP operation is submitted.
; Evidence: docs/port/glass_ga3.md.

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
%define BAR_WORDS               10
%define BAR_COUNT               16
%define ROW_COUNT               200

glass_ga3_entry:
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
        jnz     glass_ga3_failed

        ; $PalCtl mode 0, then explicitly set every palette-set-0 entry.
        mov     ax, 0x0900
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     glass_ga3_failed
        xor     bx, bx
        mov     si, glass_ga3_palette
        mov     bp, BAR_COUNT
glass_ga3_set_palette:
        mov     ax, 0x0800
        mov     al, bl
        mov     cx, [si]
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     glass_ga3_failed
        inc     bl
        add     si, 2
        dec     bp
        jnz     glass_ga3_set_palette

        ; Show only G0, then allow graphic output.
        mov     ax, 0x0300
        mov     cx, COMPOSE_G0_ONLY
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     glass_ga3_failed
        mov     ax, 0x0b01
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     glass_ga3_failed

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
        mov     bp, ROW_COUNT
glass_ga3_draw_row:
        mov     si, glass_ga3_bar_words
        mov     bx, BAR_COUNT
glass_ga3_draw_bar:
        mov     ax, [si]
        mov     cx, BAR_WORDS
        rep     stosw
        add     si, 2
        dec     bx
        jnz     glass_ga3_draw_bar
        dec     bp
        jnz     glass_ga3_draw_row

        push    cs
        pop     es
        mov     ax, 0x4743             ; "GC" GA-3 success marker.
        jmp     glass_ga3_idle

glass_ga3_failed:
        mov     ax, 0x47e3             ; debugger-visible failure marker.

; The debugger captures the state at this fixed address after the visible
; framebuffer write has completed.
times 0x0100 - ($ - $$) db 0
glass_ga3_idle:
        hlt
        jmp     glass_ga3_idle

; Diagnostic palette profile, written through Graphics BIOS $SetPal. The
; values are the manual's palette-mode-0 reset values and are not the later
; visual-tuned GLASS face-shading palette.
glass_ga3_palette:
        dw 0x0000, 0x001f, 0x03e0, 0x03ff
        dw 0xfc00, 0xfc1f, 0xffe0, 0xffff
        dw 0x7def, 0x0015, 0x02a0, 0x02b5
        dw 0xac00, 0xac15, 0xaea0, 0xaeb5

; Each 16-color packed word repeats one palette index across four dots.
glass_ga3_bar_words:
        dw 0x0000, 0x1111, 0x2222, 0x3333
        dw 0x4444, 0x5555, 0x6666, 0x7777
        dw 0x8888, 0x9999, 0xaaaa, 0xbbbb
        dw 0xcccc, 0xdddd, 0xeeee, 0xffff

%if ($ - $$) > 0xef00
%error "GLASS ORBIT GA-3 payload overlaps the fixed stack reserve"
%endif
