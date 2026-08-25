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
; THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
; IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
; OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
; IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
; INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
; BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
; AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
; TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
; OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

; NEON RELAY 4 P3 VA skeleton.
;
; This file intentionally contains only the N4-1..N4-6 hardware skeleton.  It
; does not link the original scene code.  The 8bpp BIOS argument is a P3
; calibration value: the P1 contract established the GRRES field encoding,
; but no existing payload had exercised the Graphics BIOS 8bpp call.  The
; value is therefore overrideable with -dNEON4_PIXEL_ARGS=.... and remains
; marked NOT VERIFIED until a VA/VA2 observation is recorded.

        cpu     286
        bits    16
        org     0

%ifndef NEON4_STAGE
%define NEON4_STAGE 2
%endif
%ifndef NEON4_PIXEL_ARGS
%define NEON4_PIXEL_ARGS 0808h
%endif
%ifndef NEON4_DIRECT_REGS
%define NEON4_DIRECT_REGS 0
%endif

%define VIDEO_BIOS_INT          8fh
%define KEYBOARD_BIOS_INT       82h
%define VIDEO_DATA_SEG          0338h

%define PORT_MEMORY_MAP         0153h
%define PORT_TSP_STATUS         0142h
%define PORT_SGP_COMMAND        0500h
%define PORT_SGP_CONTROL        0504h
%define PORT_SGP_STATUS         0506h
%define PORT_GVRAM_WRITE_MODE   0580h
%define PORT_COL_COMP           0106h
%define PORT_RGB_COMP           0108h
%define PORT_FB0_DSA_LOW        020eh
%define PORT_FB0_DSA_HIGH       0210h

%define MEMORY_MAP_TVRAM        041h
%define MEMORY_MAP_GVRAM        054h
%define GVRAM_CPU_WRITE_MODE    010h
%define TSP_VBLANK              040h
%define SGP_BUSY                001h

%define MODE_320X200_G0_G1      0e00eh
%define COMPOSE_G1_OVER_G0      0034h
%define G0_SEGMENT              0a000h

%define SCREEN_WIDTH            320
%define SCREEN_HEIGHT           200
%define BYTES_PER_LINE          320
%define WORDS_PER_LINE          160
%define SCREEN_WORDS            (WORDS_PER_LINE * SCREEN_HEIGHT)
%define PAGE_BYTES              (BYTES_PER_LINE * SCREEN_HEIGHT)
%define PAGE_B_SGP              020fa00h
%define PAGE_B_DSA              0000fa00h
%define PAGE_B_SGP_LOW          0fa00h

%define SGP_END                 0001h
%define SGP_SET_WORK            0003h
%define SGP_SET_COLOR           0006h
%define SGP_CLS                 000ah

; ---------------------------------------------------------------------------
; Entry and stage dispatch.
; ---------------------------------------------------------------------------
start:
        cli
        push    cs
        pop     ds
        push    cs
        pop     es
        mov     ax, 0e000h
        mov     ss, ax
        mov     sp, 0dff0h
        cld
        sti

%if NEON4_STAGE == 1
        ; N4-1: entry/stack/segment smoke.  No graphics or BIOS call.
        jmp     return_to_loader
%else
        call    va_video_enter
        jc      stage_failure
        cld
        push    cs
        pop     ds
        push    cs
        pop     es

%if NEON4_STAGE == 2
        mov     al, 024h
        call    cpu_clear_page
        jmp     wait_for_escape
%elif NEON4_STAGE == 3
        call    cpu_colour_bars
        jmp     wait_for_escape
%elif NEON4_STAGE == 4
        call    cpu_clear_page_zero
        xor     bx, bx
.vblank_loop:
        call    wait_vblank_edge
        jc      stage_failure
        inc     bx
        mov     al, bl
        and     al, 01fh
        call    cpu_marker_band
        call    keyboard_escape
        jnc     .vblank_loop
        jmp     leave_and_return
%elif NEON4_STAGE == 5
        mov     al, 024h
        call    sgp_clear_page
        jc      stage_failure
        jmp     wait_for_escape
%elif NEON4_STAGE == 6
        ; N4-6: CPU paints page A, SGP paints page B, then the FB0 DSA
        ; source is exchanged.  The DSA ports are the same FB0 pair used by
        ; the existing NEON3 VA backend; FB0/FB2 8bpp behaviour is still
        ; hardware-pending.
        mov     al, 01ch
        call    cpu_clear_page
        mov     al, 0e3h
        call    sgp_clear_page_b
        jc      stage_failure
        call    wait_vblank_edge
        jc      stage_failure
        call    set_display_page_b
        jmp     wait_for_escape
%else
%error "NEON4_STAGE must be 1..6"
%endif
%endif

stage_failure:
        ; A failed BIOS/SGP probe is shown as a red page, then held for a
        ; human gate.  No DOS service is used on this path.
        push    cs
        pop     ds
        mov     al, 0e0h
        call    cpu_clear_page
wait_for_escape:
        call    keyboard_escape
        jnc     wait_for_escape
leave_and_return:
        call    va_video_leave
return_to_loader:
        cli
        cmp     word [cs:0e006h], 5034h
        jne     .halt
        mov     ax, [cs:0e000h]
        mov     ss, ax
        mov     sp, [cs:0e002h]
        push    word [cs:0e004h]
        popf
        retf
.halt:
        sti
.halt_loop:
        hlt
        jmp     .halt_loop

; ---------------------------------------------------------------------------
; VA graphics setup.  The mode call follows the proven sprite payload order;
; only the 8bpp argument is new to P3.
; ---------------------------------------------------------------------------
va_video_enter:
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    ds
        push    es
        mov     ax, VIDEO_DATA_SEG
        mov     ds, ax
        mov     es, ax
        mov     bx, MODE_320X200_G0_G1
%if NEON4_DIRECT_REGS
        ; Enter through the proven 320x200 BIOS transaction, then switch G0
        ; to direct 8bpp with the reconstructed GRRES/FB0 registers below.
        ; This avoids guessing an unimplemented 8bpp BIOS argument while the
        ; P3 calibration is still in progress.
        mov     cx, 0404h
%else
        mov     cx, NEON4_PIXEL_ARGS
%endif
        xor     dx, dx
        xor     ax, ax
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     .failed
        ; Define the 8bpp FB0 surface and its display window explicitly.
        ; The VA BIOS mode call resets the descriptor tables; leaving them
        ; undefined makes every later framebuffer operation fail before the
        ; first pixel is written.
        push    cs
        pop     es
        mov     di, neon4_framebuffer_descriptor
        mov     ax, 0100h              ; $DefBuf, graphics screen 0.
        mov     cx, 1
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     .failed
        push    cs
        pop     es
        mov     di, neon4_window_descriptor
        mov     ax, 0200h              ; $DefWin, graphics screen 0.
        mov     cx, 1
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     .failed
        mov     ax, 0b00h
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     .failed
        mov     ax, 0900h
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     .failed
        mov     ax, 0a00h
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     .failed
        ; Select the direct RGB screen fed by G0.  The 8bpp mode and its
        ; descriptors do not by themselves enable the RGB composition path;
        ; without these two writes only the text cursor remains visible.
        mov     dx, PORT_COL_COMP
        xor     ax, ax
        out     dx, ax
        mov     dx, PORT_RGB_COMP
        mov     ax, 0008h             ; RGB screen 0 <- direct G0.
        out     dx, ax
        mov     ax, 0300h
        mov     cx, COMPOSE_G1_OVER_G0
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     .failed
%if NEON4_DIRECT_REGS
        call    configure_rgb332_mode
%endif
        mov     ax, 0b01h             ; Enable graphics output after setup.
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     .failed
        mov     dx, PORT_MEMORY_MAP
        mov     al, MEMORY_MAP_GVRAM
        out     dx, al
        mov     dx, PORT_GVRAM_WRITE_MODE
        mov     al, GVRAM_CPU_WRITE_MODE
        out     dx, al
        clc
        jmp     .done
.failed:
        stc
.done:
        pop     es
        pop     ds
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

; [NOT VERIFIED ON VA SILICON]  The field positions and FB0 byte layout are
; backed by the in-tree VAEG model and the P1 contract.  This is the P3
; calibration path for direct G0 RGB332; it is deliberately kept separate
; from the proven 4bpp BIOS entry above.
configure_rgb332_mode:
        push    ax
        push    dx
        mov     dx, 0102h
        mov     ax, 0012h             ; G0: 320 dots, 8bpp; G1 unused.
        out     dx, ax
        mov     dx, 0106h
        xor     ax, ax                ; Disable palette-composed screens.
        out     dx, ax
        mov     dx, 0108h
        mov     ax, 0008h             ; Enable direct-color G0.
        out     dx, ax
        mov     dx, 0204h             ; FB0 FBW = 320 bytes/row.
        mov     ax, BYTES_PER_LINE
        out     dx, ax
        mov     dx, 0206h             ; FB0 FBL = 200 source rows - 1.
        mov     ax, SCREEN_HEIGHT - 1
        out     dx, ax
        mov     dx, 0208h             ; DOT = 0.
        xor     ax, ax
        out     dx, ax
        mov     dx, 020ah             ; OFX = 0.
        out     dx, ax
        mov     dx, 020ch             ; OFY = 0.
        out     dx, ax
        mov     dx, 020eh             ; DSA = page A, low word.
        out     dx, ax
        mov     dx, 0210h
        out     dx, ax
        mov     dx, 0212h             ; DSH = 200 rows.
        mov     ax, SCREEN_HEIGHT
        out     dx, ax
        mov     dx, 0216h             ; DSP = top of output field.
        xor     ax, ax
        out     dx, ax
        pop     dx
        pop     ax
        ret

va_video_leave:
        push    ax
        push    cx
        push    dx
        push    ds
        push    es
        mov     ax, VIDEO_DATA_SEG
        mov     ds, ax
        mov     es, ax
        mov     ax, 0300h
        mov     cx, 0031h
        int     VIDEO_BIOS_INT
        mov     ax, 0b01h
        int     VIDEO_BIOS_INT
        mov     dx, PORT_MEMORY_MAP
        mov     al, MEMORY_MAP_TVRAM
        out     dx, al
        pop     es
        pop     ds
        pop     dx
        pop     cx
        pop     ax
        ret

; ---------------------------------------------------------------------------
; CPU direct packed 8bpp writes.  One word contains two consecutive bytes.
; ---------------------------------------------------------------------------
cpu_clear_page:
        push    ax
        push    cx
        push    di
        push    es
        xor     ah, ah
        mov     ah, al
        push    ax
        mov     dx, PORT_MEMORY_MAP
        mov     al, MEMORY_MAP_GVRAM
        out     dx, al
        mov     dx, PORT_GVRAM_WRITE_MODE
        mov     al, GVRAM_CPU_WRITE_MODE
        out     dx, al
        mov     dx, G0_SEGMENT
        mov     es, dx
        mov     di, 0
        mov     cx, SCREEN_WORDS
        pop     ax
        rep     stosw
        pop     es
        pop     di
        pop     cx
        pop     ax
        ret

cpu_clear_page_zero:
        xor     al, al
        jmp     cpu_clear_page

; Fill 32 ten-pixel columns with distinct direct RGB332 bytes.
cpu_colour_bars:
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        push    es
        mov     dx, PORT_MEMORY_MAP
        mov     al, MEMORY_MAP_GVRAM
        out     dx, al
        mov     dx, PORT_GVRAM_WRITE_MODE
        mov     al, GVRAM_CPU_WRITE_MODE
        out     dx, al
        mov     ax, G0_SEGMENT
        mov     es, ax
        xor     di, di
        xor     bx, bx
        mov     cx, SCREEN_HEIGHT
.row:
        push    cx
        mov     si, neon4_bar_values
        mov     bx, 32
.bar:
        lodsb
        mov     ah, al
        mov     dx, 5
        mov     cx, dx
        rep     stosw
        dec     bx
        jnz     .bar
        pop     cx
        loop    .row
        pop     es
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

; Draw a four-row marker band.  AL selects one of 32 direct RGB332 values.
cpu_marker_band:
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        push    es
        xor     ah, ah
        mov     si, ax
        mov     al, [cs:neon4_bar_values + si]
        mov     ah, al
        mov     dx, G0_SEGMENT
        mov     es, dx
        mov     di, 0
        mov     bx, 4
.line:
        mov     cx, 160
        rep     stosw
        dec     bx
        jnz     .line
        pop     es
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

; ---------------------------------------------------------------------------
; SGP command-list path.
; ---------------------------------------------------------------------------
sgp_clear_page:
        mov     dx, PAGE_A_SGP_LOW
        jmp     sgp_clear_with_base

sgp_clear_page_b:
        mov     dx, PAGE_B_SGP_LOW
sgp_clear_with_base:
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        push    es
        mov     [cs:sgp_colour_byte], al
        mov     [cs:sgp_destination_low], dx
        mov     ax, 0020h
        mov     [cs:sgp_destination_high], ax
        mov     di, sgp_command_list
        push    cs
        pop     es
        mov     ax, SGP_SET_WORK
        stosw
        mov     si, sgp_work_area
        call    physical_address_from_ds_si
        stosw
        mov     ax, dx
        stosw
        mov     ax, SGP_SET_COLOR
        stosw
        mov     al, [cs:sgp_colour_byte]
        mov     ah, al
        stosw
        mov     ax, SGP_CLS
        stosw
        mov     ax, [cs:sgp_destination_low]
        stosw
        mov     ax, [cs:sgp_destination_high]
        stosw
        mov     ax, SCREEN_WORDS
        stosw
        xor     ax, ax
        stosw
        mov     ax, SGP_END
        stosw
        call    run_sgp_command_list
        pop     es
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

run_sgp_command_list:
        push    ax
        push    dx
        call    wait_sgp_idle
        jc      .failed
        ; The VA BIOS reasserts CPU-data GVRAM write mode before each SGP kick.
        ; Keep the shared mode latch in the documented state on real hardware.
        mov     dx, PORT_GVRAM_WRITE_MODE
        mov     al, GVRAM_CPU_WRITE_MODE
        out     dx, al
        mov     dx, PORT_SGP_CONTROL
        xor     al, al
        out     dx, al
        mov     si, sgp_command_list
        call    physical_address_from_ds_si
        mov     bx, dx
        mov     dx, PORT_SGP_COMMAND
        out     dx, ax
        add     dx, 2
        mov     ax, bx
        out     dx, ax
        mov     dx, PORT_SGP_STATUS
        mov     al, 1
        out     dx, al
        call    wait_sgp_idle
        pop     dx
        pop     ax
        clc
        ret
.failed:
        pop     dx
        pop     ax
        stc
        ret

wait_sgp_idle:
        push    ax
        push    cx
        push    dx
        mov     dx, PORT_SGP_STATUS
        mov     cx, 0ffffh
.poll:
        in      al, dx
        test    al, SGP_BUSY
        jz      .ready
        loop    .poll
        stc
        jmp     .done
.ready:
        clc
.done:
        pop     dx
        pop     cx
        pop     ax
        ret

physical_address_from_ds_si:
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

; FB0 DSA is a pair of word ports.  This is a VAEG-backed P3 experiment;
; real VA/VA2 FB0 page switching remains hardware-pending.
set_display_page_b:
        mov     dx, PORT_FB0_DSA_LOW
        mov     ax, PAGE_B_DSA & 0ffffh
        out     dx, ax
        add     dx, 2
        xor     ax, ax
        out     dx, ax
        ret

wait_vblank_edge:
        push    ax
        push    cx
        push    dx
        mov     dx, PORT_TSP_STATUS
        mov     cx, 0ffffh
.low:
        in      al, dx
        test    al, TSP_VBLANK
        jz      .low_seen
        loop    .low
        stc
        jmp     .done
.low_seen:
        mov     cx, 0ffffh
.high:
        in      al, dx
        test    al, TSP_VBLANK
        jnz     .edge
        loop    .high
        stc
        jmp     .done
.edge:
        clc
.done:
        pop     dx
        pop     cx
        pop     ax
        ret

keyboard_escape:
        ; Use the VA keyboard contract exercised by the existing SGP demo:
        ; AH=0Ah tests for a pending key and AH=09h consumes it.  AH=00h is
        ; the returned scan code for ESC.  The older AH=01h/AH=00h pair is
        ; not a non-blocking poll on the VA2 BIOS and can leave this stage
        ; waiting forever.
        mov     ah, 0ah
        int     KEYBOARD_BIOS_INT
        jc      .none
        mov     ah, 09h
        int     KEYBOARD_BIOS_INT
        cmp     ah, 00h
        jne     .none
        stc
        ret
.none:
        clc
        ret

PAGE_A_SGP_LOW equ 0000h

align 2
sgp_command_list:
        times 32 dw 0
sgp_work_area:
        times 29 dw 0
sgp_destination_low dw 0
sgp_destination_high dw 0
sgp_colour_byte db 0
        align 2

neon4_bar_values:
        db 00h, 20h, 40h, 60h, 80h, 0a0h, 0c0h, 0e0h
        db 04h, 24h, 44h, 64h, 84h, 0a4h, 0c4h, 0e4h
        db 10h, 30h, 50h, 70h, 90h, 0b0h, 0d0h, 0f0h
        db 1ch, 3ch, 5ch, 7ch, 9ch, 0bch, 0dch, 0fch

align 16
; $DefBuf descriptor: pixel size, width, height.
neon4_framebuffer_descriptor:
        dw 8, SCREEN_WIDTH, SCREEN_HEIGHT
align 16
; $DefWin descriptor: framebuffer number, screen Y, height, source X, Y.
neon4_window_descriptor:
        dw 0, 0, SCREEN_HEIGHT, 0, 0

; These words are intentionally reserved for the P3 command builder.  The
; loader writes its return continuation at CS:0e000..0e008.
%if ($ - $$) >= 0e000h
%error "NEON4 P3 payload overlaps the loader return reserve"
%endif
