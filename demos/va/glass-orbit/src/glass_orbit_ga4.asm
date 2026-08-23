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

; GLASS ORBIT GA-4 TSP vertical-blank polling proof.
; Graphics BIOS owns the already-approved display and palette setup.  This
; payload only reads TSP status port 0142h.  It waits for a low-to-high VB
; transition before each CPU-written background update.  It does not assign a
; physical frame rate or change a TSP timing register.
; Evidence: docs/port/glass_ga4.md.

cpu 286
bits 16
org 0

%define VIDEO_BIOS_INT          0x8f
%define PORT_MEMORY_MAP         0x0153
%define PORT_GVRAM_WRITE_MODE   0x0580
%define PORT_TSP_STATUS         0x0142
%define MODE_G0_640X200_4BPP    0xa002
%define PIXEL_SIZE_G0_4BPP      0x0004
%define COMPOSE_G0_ONLY         0x0003
%define MEMORY_MAP_GVRAM_SINGLE 0x54
%define GVRAM_CPU_WRITE_MODE    0x10
%define TSP_STATUS_VBLANK       0x40
%define GVRAM_PAGE_WORDS        0x7d00
%define PALETTE_ENTRY_COUNT     16
%define POLL_OUTER_LIMIT        4

glass_ga4_entry:
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
        jnz     glass_ga4_failed

        ; $PalCtl mode 0, then install the GA-3 diagnostic palette profile.
        mov     ax, 0x0900
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     glass_ga4_failed
        xor     bx, bx
        mov     si, glass_ga4_palette
        mov     bp, PALETTE_ENTRY_COUNT
glass_ga4_set_palette:
        mov     ax, 0x0800
        mov     al, bl
        mov     cx, [si]
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     glass_ga4_failed
        inc     bl
        add     si, 2
        dec     bp
        jnz     glass_ga4_set_palette

        ; Show only G0, then allow graphic output.
        mov     ax, 0x0300
        mov     cx, COMPOSE_G0_ONLY
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     glass_ga4_failed
        mov     ax, 0x0b01
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     glass_ga4_failed

        ; Map CPU writes to the G0 single-plane GVRAM aperture at A000:0000.
        mov     dx, PORT_MEMORY_MAP
        mov     al, MEMORY_MAP_GVRAM_SINGLE
        out     dx, al
        mov     dx, PORT_GVRAM_WRITE_MODE
        mov     al, GVRAM_CPU_WRITE_MODE
        out     dx, al

        ; Start from black. Every later colour update is preceded by a VB
        ; low-to-high observation through glass_ga4_wait_vblank_start.
        xor     ax, ax
        call    glass_ga4_fill_page
        mov     byte [glass_ga4_colour_index], 1
        mov     word [glass_ga4_vblank_count], 0

; Keep the observed checkpoint at a stable address for the M74 debug scripts.
times 0x0200 - ($ - $$) db 0
glass_ga4_frame_ready:
        call    glass_ga4_wait_vblank_start
        jc      glass_ga4_failed

        inc     word [glass_ga4_vblank_count]
        xor     bx, bx
        mov     bl, [glass_ga4_colour_index]
        shl     bx, 1
        mov     ax, [glass_ga4_colour_words + bx]
        call    glass_ga4_fill_page

        inc     byte [glass_ga4_colour_index]
        cmp     byte [glass_ga4_colour_index], PALETTE_ENTRY_COUNT
        jb      glass_ga4_colour_ready
        mov     byte [glass_ga4_colour_index], 1
glass_ga4_colour_ready:
        push    cs
        pop     es
        mov     bx, [glass_ga4_vblank_count]
        mov     ax, 0x4744             ; "GD" GA-4 success marker.
        jmp     glass_ga4_frame_ready

glass_ga4_failed:
        push    cs
        pop     es
        mov     bx, [glass_ga4_vblank_count]
        mov     ax, 0x47e4             ; debugger-visible failure marker.

glass_ga4_idle:
        hlt
        jmp     glass_ga4_idle

; AX is a packed word with one palette index in every nibble.  The function
; writes exactly one 640x200 packed-4bpp page through the proven GA-2 aperture.
glass_ga4_fill_page:
        push    cx
        push    di
        push    es
        mov     cx, GVRAM_PAGE_WORDS
        mov     di, 0
        mov     dx, 0xa000
        mov     es, dx
        rep     stosw
        pop     es
        pop     di
        pop     cx
        ret

; Observe one complete TSP VB edge.  A bounded wait prevents a broken TSP
; state from being mistaken for a successful update.  The bound is diagnostic
; only and makes no claim about a real PC-88VA frame period.
glass_ga4_wait_vblank_start:
        mov     dx, PORT_TSP_STATUS
        mov     bx, POLL_OUTER_LIMIT
glass_ga4_wait_display:
        mov     cx, 0xffff
glass_ga4_display_poll:
        in      al, dx
        test    al, TSP_STATUS_VBLANK
        jz      glass_ga4_display_seen
        loop    glass_ga4_display_poll
        dec     bx
        jnz     glass_ga4_wait_display
        stc
        ret
glass_ga4_display_seen:
        mov     bx, POLL_OUTER_LIMIT
glass_ga4_wait_vblank:
        mov     cx, 0xffff
glass_ga4_vblank_poll:
        in      al, dx
        test    al, TSP_STATUS_VBLANK
        jnz     glass_ga4_vblank_seen
        loop    glass_ga4_vblank_poll
        dec     bx
        jnz     glass_ga4_wait_vblank
        stc
        ret
glass_ga4_vblank_seen:
        clc
        ret

; Diagnostic palette profile from the manual's palette-mode-0 reset values.
glass_ga4_palette:
        dw 0x0000, 0x001f, 0x03e0, 0x03ff
        dw 0xfc00, 0xfc1f, 0xffe0, 0xffff
        dw 0x7def, 0x0015, 0x02a0, 0x02b5
        dw 0xac00, 0xac15, 0xaea0, 0xaeb5

; Index 0 is reserved for the initial clear.  The live proof cycles 1..15.
glass_ga4_colour_words:
        dw 0x0000, 0x1111, 0x2222, 0x3333
        dw 0x4444, 0x5555, 0x6666, 0x7777
        dw 0x8888, 0x9999, 0xaaaa, 0xbbbb
        dw 0xcccc, 0xdddd, 0xeeee, 0xffff

glass_ga4_colour_index:
        db 0
glass_ga4_vblank_count:
        dw 0

%if ($ - $$) > 0xef00
%error "GLASS ORBIT GA-4 payload overlaps the fixed stack reserve"
%endif
