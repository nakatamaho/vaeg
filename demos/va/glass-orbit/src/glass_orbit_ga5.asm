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

; GLASS ORBIT GA-5 SGP clear proof.
; The list is exactly SET WORK, SET COLOR, CLS, END. The CPU reads GVRAM only
; after SGP idle to validate the completed SGP result, then installs the GA-2
; nibble-order probe. It does not assign an SGP duration or timing contract.
; Evidence: docs/port/glass_ga5.md.

cpu 286
bits 16
org 0

%define VIDEO_BIOS_INT          0x8f
%define PORT_MEMORY_MAP         0x0153
%define PORT_GVRAM_WRITE_MODE   0x0580
%define PORT_SGP_COMMAND        0x0500
%define PORT_SGP_CONTROL        0x0504
%define PORT_SGP_STATUS         0x0506
%define MODE_G0_640X200_4BPP    0xa002
%define PIXEL_SIZE_G0_4BPP      0x0004
%define COMPOSE_G0_ONLY         0x0003
%define MEMORY_MAP_GVRAM_SINGLE 0x54
%define GVRAM_CPU_WRITE_MODE    0x10
%define SGP_BUSY                0x01
%define G0_SGP_BASE             0x200000
%define GVRAM_PAGE_WORDS        0x7d00
%define SGP_COMMAND_END         0x0001
%define SGP_COMMAND_SET_WORK    0x0003
%define SGP_COMMAND_SET_COLOR   0x0006
%define SGP_COMMAND_CLS         0x000a

glass_ga5_entry:
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
        jnz     glass_ga5_failed

        ; Restore the documented palette and display only G0.
        mov     ax, 0x0a00
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     glass_ga5_failed
        mov     ax, 0x0300
        mov     cx, COMPOSE_G0_ONLY
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     glass_ga5_failed
        mov     ax, 0x0b01
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     glass_ga5_failed

        ; Map CPU reads and the final validation probe to G0 at A000:0000.
        mov     dx, PORT_MEMORY_MAP
        mov     al, MEMORY_MAP_GVRAM_SINGLE
        out     dx, al
        mov     dx, PORT_GVRAM_WRITE_MODE
        mov     al, GVRAM_CPU_WRITE_MODE
        out     dx, al

        call    glass_ga5_build_sgp_list
        call    glass_ga5_run_sgp_list
        jc      glass_ga5_failed
        call    glass_ga5_verify_sgp_clear
        jc      glass_ga5_failed

        ; This probe is deliberately the same as GA-2. The preceding verifier
        ; has already checked every word produced by SGP CLS.
        mov     word [es:0], 0x3412
        mov     word [es:2], 0x7856

        push    cs
        pop     es
        mov     bx, GVRAM_PAGE_WORDS
        mov     ax, 0x4745             ; "GE" GA-5 success marker.
        jmp     glass_ga5_idle

glass_ga5_failed:
        push    cs
        pop     es
        mov     ax, 0x47e5             ; debugger-visible failure marker.

; Keep the observed idle point stable for the M74 debug harness.
times 0x0100 - ($ - $$) db 0
glass_ga5_idle:
        hlt
        jmp     glass_ga5_idle

; Build the documented minimal command list in main RAM. The work area is
; explicitly zeroed storage; this does not assert an undocumented work format.
glass_ga5_build_sgp_list:
        push    ax
        push    dx
        push    si
        push    di
        push    es
        push    ds
        pop     es
        mov     di, glass_ga5_command_list

        mov     ax, SGP_COMMAND_SET_WORK
        stosw
        mov     si, glass_ga5_work_area
        call    glass_ga5_physical_address_from_ds_si
        stosw
        mov     ax, dx
        stosw

        mov     ax, SGP_COMMAND_SET_COLOR
        stosw
        mov     ax, 0x5555
        stosw

        mov     ax, SGP_COMMAND_CLS
        stosw
        mov     ax, G0_SGP_BASE & 0xffff
        stosw
        mov     ax, G0_SGP_BASE >> 16
        stosw
        mov     ax, GVRAM_PAGE_WORDS
        stosw
        xor     ax, ax
        stosw

        mov     ax, SGP_COMMAND_END
        stosw
        mov     si, glass_ga5_command_list
        call    glass_ga5_physical_address_from_ds_si
        mov     [glass_ga5_command_address_low], ax
        mov     [glass_ga5_command_address_high], dx

        pop     es
        pop     di
        pop     si
        pop     dx
        pop     ax
        ret

; Submit only after observing idle, then wait for END to return SGP to idle.
glass_ga5_run_sgp_list:
        call    glass_ga5_wait_sgp_idle
        jc      .failed
        mov     dx, PORT_SGP_COMMAND
        mov     ax, [glass_ga5_command_address_low]
        out     dx, ax
        add     dx, 2
        mov     ax, [glass_ga5_command_address_high]
        out     dx, ax
        mov     dx, PORT_SGP_CONTROL
        xor     al, al
        out     dx, al
        mov     dx, PORT_SGP_STATUS
        mov     al, SGP_BUSY
        out     dx, al
        call    glass_ga5_wait_sgp_idle
        ret
.failed:
        stc
        ret

glass_ga5_wait_sgp_idle:
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

; Independent CPU-aperture readback of all words that the SGP CLS owns.
glass_ga5_verify_sgp_clear:
        mov     ax, 0xa000
        mov     es, ax
        xor     di, di
        mov     cx, GVRAM_PAGE_WORDS
        mov     bx, 0x5555
.word:
        cmp     word [es:di], bx
        jne     .failed
        add     di, 2
        loop    .word
        clc
        ret
.failed:
        stc
        ret

; Convert DS:SI to the 20-bit physical address required by SGP command ports.
; AX is the low word and DX the high word on return.
glass_ga5_physical_address_from_ds_si:
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
glass_ga5_command_list:
        times 24 db 0
glass_ga5_command_address_low:
        dw 0
glass_ga5_command_address_high:
        dw 0
align 2
glass_ga5_work_area:
        times 58 db 0

%if ($ - $$) > 0xef00
%error "GLASS ORBIT GA-5 payload overlaps the fixed stack reserve"
%endif
