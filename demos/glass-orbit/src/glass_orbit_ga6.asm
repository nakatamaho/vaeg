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

; GLASS ORBIT GA-6 source-window page exchange proof.
;
; G0 has one 640x400 packed-4bpp FB0 source and one 640x200 window.  The
; two visible pages are its Y=0 and Y=200 source regions.  Page selection is
; intentionally made only through the PC-88VA Graphics BIOS $RollTo service
; after a TSP vertical-blank observation; this avoids a raw DSA-only update.
; ESC uses the PC-88VA Keyboard BIOS character services, not a DOS service.
; Evidence: docs/port/glass_ga6.md.

cpu 286
bits 16
org 0

%define VIDEO_BIOS_INT          0x8f
%define KEYBOARD_BIOS_INT       0x82
%define PORT_MEMORY_MAP         0x0153
%define PORT_GVRAM_WRITE_MODE   0x0580
%define PORT_TSP_STATUS         0x0142
%define PORT_SGP_COMMAND        0x0500
%define PORT_SGP_CONTROL        0x0504
%define PORT_SGP_STATUS         0x0506
%define MODE_G0_640X200_4BPP    0xa002
%define PIXEL_SIZE_G0_4BPP      0x0004
%define COMPOSE_G0_ONLY         0x0003
%define MEMORY_MAP_GVRAM_SINGLE 0x54
%define GVRAM_CPU_WRITE_MODE    0x10
%define TSP_STATUS_VBLANK       0x40
%define SGP_BUSY                0x01
%define G0_PAGE_A_SGP_BASE      0x200000
%define G0_PAGE_B_SGP_BASE      0x20fa00
%define GVRAM_PAGE_WORDS        0x7d00
%define PAGE_A_SOURCE_Y         0
%define PAGE_B_SOURCE_Y         200
%define SGP_COMMAND_END         0x0001
%define SGP_COMMAND_SET_WORK    0x0003
%define SGP_COMMAND_SET_COLOR   0x0006
%define SGP_COMMAND_CLS         0x000a
%define POLL_OUTER_LIMIT        4
%define LOADER_RETURN_SS        0x0f00
%define LOADER_RETURN_SP        0x0f02
%define LOADER_RETURN_FLAGS     0x0f04

glass_ga6_entry:
        cli
        mov     ax, cs
        mov     ds, ax
        mov     es, ax
        mov     ss, ax
        mov     sp, 0xf000
        cld
        sti

        ; $ScnMode: G0 640x200, single-plane, graphics enabled, 4 bpp.
        mov     bx, MODE_G0_640X200_4BPP
        mov     cx, PIXEL_SIZE_G0_4BPP
        xor     dx, dx
        xor     ax, ax
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     glass_ga6_failed

        ; Keep graphics hidden while FB0 and its window are defined.
        mov     ax, 0x0b00
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     glass_ga6_failed
        mov     ax, 0x0a00
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     glass_ga6_failed

        ; $DefBuf G0: one 640x400 packed-4bpp source framebuffer.
        push    ds
        pop     es
        mov     ax, 0x0100
        mov     cx, 1
        mov     di, glass_ga6_framebuffer_descriptor
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     glass_ga6_failed

        ; $DefWin G0: one 640x200 window initially placed at source Y=0.
        mov     ax, 0x0200
        mov     cx, 1
        mov     di, glass_ga6_window_descriptor
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     glass_ga6_failed

        ; G0 is the sole compositor input.  The default palette keeps index
        ; 1 blue and index 2 red, making page identity visible.
        mov     ax, 0x0300
        mov     cx, COMPOSE_G0_ONLY
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     glass_ga6_failed

        mov     dx, PORT_MEMORY_MAP
        mov     al, MEMORY_MAP_GVRAM_SINGLE
        out     dx, al
        mov     dx, PORT_GVRAM_WRITE_MODE
        mov     al, GVRAM_CPU_WRITE_MODE
        out     dx, al

        ; Clear each source half independently through the already-proved
        ; SET_WORK -> SET_COLOR -> CLS -> END SGP sequence.
        mov     ax, 0x1111
        mov     bx, G0_PAGE_A_SGP_BASE & 0xffff
        mov     dx, G0_PAGE_A_SGP_BASE >> 16
        call    glass_ga6_clear_sgp_page
        jc      glass_ga6_failed
        mov     ax, 0x2222
        mov     bx, G0_PAGE_B_SGP_BASE & 0xffff
        mov     dx, G0_PAGE_B_SGP_BASE >> 16
        call    glass_ga6_clear_sgp_page
        jc      glass_ga6_failed

        mov     byte [glass_ga6_page_index], 0
        call    glass_ga6_wait_vblank_start
        jc      glass_ga6_failed
        call    glass_ga6_select_visible_page
        jc      glass_ga6_failed

        mov     ax, 0x0b01
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     glass_ga6_failed

%ifdef GLASS_GA6_CAPTURE_PAGE
        mov     byte [glass_ga6_page_index], GLASS_GA6_CAPTURE_PAGE
        call    glass_ga6_wait_vblank_start
        jc      glass_ga6_failed
        call    glass_ga6_select_visible_page
        jc      glass_ga6_failed
        jmp     glass_ga6_set_checkpoint_marker
%endif

glass_ga6_loop:
        call    glass_ga6_escape_pressed
        jc      glass_ga6_exit
        call    glass_ga6_wait_vblank_start
        jc      glass_ga6_failed
        xor     byte [glass_ga6_page_index], 1
        call    glass_ga6_select_visible_page
        jc      glass_ga6_failed

glass_ga6_set_checkpoint_marker:
        push    cs
        pop     es
        xor     bx, bx
        mov     bl, [glass_ga6_page_index]
        mov     ax, 0x4746             ; "GF" GA-6 success marker.
        jmp     glass_ga6_checkpoint

; Keep the observed checkpoint at a stable address for the M74 debug harness.
times 0x0100 - ($ - $$) db 0
glass_ga6_checkpoint:
%ifdef GLASS_GA6_CAPTURE_PAGE
        ; Keep the selected static page visible while still polling the VA
        ; Keyboard BIOS, so GLASSP6A and GLASSP6B have the same ESC exit as
        ; the alternating-page GLASSP6 variant.
        call    glass_ga6_escape_pressed
        jc      glass_ga6_exit
        jmp     glass_ga6_checkpoint
%endif
        jmp     glass_ga6_loop

glass_ga6_failed:
        push    cs
        pop     es
        xor     bx, bx
        mov     ax, 0x47e6             ; debugger-visible failure marker.

glass_ga6_failed_idle:
        hlt
        jmp     glass_ga6_failed_idle

; $RollTo G0/FB0 is the documented coupled source-window change.  It updates
; the selected FB0 source Y instead of writing DSA alone.
glass_ga6_select_visible_page:
        push    bx
        push    cx
        push    dx
        xor     bx, bx
        xor     cx, cx
        xor     dx, dx
        cmp     byte [glass_ga6_page_index], 0
        je      .selected
        mov     dx, PAGE_B_SOURCE_Y
.selected:
        mov     ax, 0x0500             ; $RollTo, G0, FB0.
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     .failed
        clc
        jmp     .done
.failed:
        stc
.done:
        pop     dx
        pop     cx
        pop     bx
        ret

; The PC-88VA Keyboard BIOS $SnsChar/$GetChar pair is used here rather than
; the primitive functions.  The VA CP/M BIOS checks CF after $SnsChar and
; receives the scan code in AH from $GetChar.  ESC is scan code zero.
glass_ga6_escape_pressed:
        mov     ah, 0x01             ; $SnsChar: CF clear when input is ready.
        int     KEYBOARD_BIOS_INT
        jc      .none
        mov     ah, 0x00             ; $GetChar: AH is the keyboard scan code.
        int     KEYBOARD_BIOS_INT
        cmp     ah, 0
        je      .escape
.none:
        clc
        ret
.escape:
        stc
        ret

; Stop graphics through the VA Graphics BIOS, then return to the local COM
; loader using the stack state it saved before entering this bare payload.
; No DOS interrupt is used by this exit path.
glass_ga6_exit:
        mov     ax, 0x0b00
        int     VIDEO_BIOS_INT
        cli
        mov     ax, [cs:LOADER_RETURN_SS]
        mov     ss, ax
        mov     sp, [cs:LOADER_RETURN_SP]
        push    word [cs:LOADER_RETURN_FLAGS]
        popf
        retf

; AX=color word, BX:DX=SGP physical base.  The generated list is deliberately
; the exact GA-5 primitive sequence, with only colour and target changed.
glass_ga6_clear_sgp_page:
        mov     [glass_ga6_clear_color], ax
        mov     [glass_ga6_clear_base_low], bx
        mov     [glass_ga6_clear_base_high], dx
        call    glass_ga6_build_clear_list
        call    glass_ga6_run_sgp_list
        ret

glass_ga6_build_clear_list:
        push    ax
        push    dx
        push    si
        push    di
        push    es
        push    ds
        pop     es
        mov     di, glass_ga6_command_list

        mov     ax, SGP_COMMAND_SET_WORK
        stosw
        mov     si, glass_ga6_work_area
        call    glass_ga6_physical_address_from_ds_si
        stosw
        mov     ax, dx
        stosw

        mov     ax, SGP_COMMAND_SET_COLOR
        stosw
        mov     ax, [glass_ga6_clear_color]
        stosw

        mov     ax, SGP_COMMAND_CLS
        stosw
        mov     ax, [glass_ga6_clear_base_low]
        stosw
        mov     ax, [glass_ga6_clear_base_high]
        stosw
        mov     ax, GVRAM_PAGE_WORDS
        stosw
        xor     ax, ax
        stosw

        mov     ax, SGP_COMMAND_END
        stosw
        mov     si, glass_ga6_command_list
        call    glass_ga6_physical_address_from_ds_si
        mov     [glass_ga6_command_address_low], ax
        mov     [glass_ga6_command_address_high], dx

        pop     es
        pop     di
        pop     si
        pop     dx
        pop     ax
        ret

glass_ga6_run_sgp_list:
        call    glass_ga6_wait_sgp_idle
        jc      .failed
        mov     dx, PORT_SGP_COMMAND
        mov     ax, [glass_ga6_command_address_low]
        out     dx, ax
        add     dx, 2
        mov     ax, [glass_ga6_command_address_high]
        out     dx, ax
        mov     dx, PORT_SGP_CONTROL
        xor     al, al
        out     dx, al
        mov     dx, PORT_SGP_STATUS
        mov     al, SGP_BUSY
        out     dx, al
        call    glass_ga6_wait_sgp_idle
        ret
.failed:
        stc
        ret

glass_ga6_wait_sgp_idle:
        mov     dx, PORT_SGP_STATUS
        mov     cx, 0xffff
.poll:
        in      al, dx
        test    al, SGP_BUSY
        jz      .ready
        loop    .poll
        stc
        ret
.ready:
        clc
        ret

; Observe one whole display-to-VB transition.  The bound is diagnostic only;
; it does not establish a PC-88VA timing value.
glass_ga6_wait_vblank_start:
        mov     dx, PORT_TSP_STATUS
        mov     bx, POLL_OUTER_LIMIT
.wait_display:
        mov     cx, 0xffff
.display_poll:
        in      al, dx
        test    al, TSP_STATUS_VBLANK
        jz      .display_seen
        loop    .display_poll
        dec     bx
        jnz     .wait_display
        stc
        ret
.display_seen:
        mov     bx, POLL_OUTER_LIMIT
.wait_vblank:
        mov     cx, 0xffff
.vblank_poll:
        in      al, dx
        test    al, TSP_STATUS_VBLANK
        jnz     .ready
        loop    .vblank_poll
        dec     bx
        jnz     .wait_vblank
        stc
        ret
.ready:
        clc
        ret

; Convert DS:SI to the physical address required by SGP command ports.
; AX is the low word and DX the high word on return.
glass_ga6_physical_address_from_ds_si:
        mov     ax, ds
        xor     dx, dx
        mov     cx, 4
.shift:
        shl     ax, 1
        rcl     dx, 1
        loop    .shift
        add     ax, si
        adc     dx, 0
        ret

align 2
glass_ga6_framebuffer_descriptor:
        dw 4, 640, 400
glass_ga6_window_descriptor:
        dw 0, 0, 200, 0, 0

align 2
glass_ga6_command_list:
        times 24 db 0
glass_ga6_command_address_low:
        dw 0
glass_ga6_command_address_high:
        dw 0
glass_ga6_clear_color:
        dw 0
glass_ga6_clear_base_low:
        dw 0
glass_ga6_clear_base_high:
        dw 0
align 2
glass_ga6_work_area:
        times 58 db 0
glass_ga6_page_index:
        db 0

; The local loader writes its return context here before the far transfer.
times LOADER_RETURN_SS - ($ - $$) db 0
glass_ga6_loader_return_ss:
        dw 0
glass_ga6_loader_return_sp:
        dw 0
glass_ga6_loader_return_flags:
        dw 0

%if ($ - $$) > 0xef00
%error "GLASS ORBIT GA-6 payload overlaps the fixed stack reserve"
%endif
