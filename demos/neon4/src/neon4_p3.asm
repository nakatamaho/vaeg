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

; NEON RELAY 4 P3/P4 VA skeleton.
;
; This file intentionally contains only the N4-1..N4-6 hardware skeleton.  It
; does not link the original scene code.  The 8bpp path follows
; topic/m97-sgp-tekumani:demos/sgp-pseudo-sprite/256: INT 8Fh enters 320x200
; G0/G1 with CX=0808h, G1 uses a 320x400 backing surface, and FB1 DSA selects
; its two 64,000-byte pages.  Scene code is not linked until the P3 gate is
; complete.

        cpu     286
        bits    16
        org     0

%ifndef NEON4_STAGE
%define NEON4_STAGE 2
%endif
%ifndef NEON4_PIXEL_ARGS
%define NEON4_PIXEL_ARGS 0808h
%endif
%ifndef NEON4_MODE
%define NEON4_MODE 0e00eh
%endif
%ifndef NEON4_USE_DEFAULT_BUFFERS
%define NEON4_USE_DEFAULT_BUFFERS 1
%endif
%ifndef NEON4_DIRECT_REGS
%define NEON4_DIRECT_REGS 0
%endif
%ifndef NEON4_DIAG_MARKER
; Direct TVRAM writes are intentionally disabled on the 8bpp path.  The
; graphics BIOS owns the text-plane mapping during mode transition; enabling
; this probe can corrupt the mode state before the first GVRAM write.
%define NEON4_DIAG_MARKER 0
%endif
%ifndef NEON4_P4_BACKEND
; P4 diagnostic backend: 0 = CPU reference, 1 = SGP command list.
%define NEON4_P4_BACKEND 0
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
%define PORT_G0_TRANSPARENCY    0124h
%define PORT_G1_TRANSPARENCY    0126h
%define PORT_FB1_FBW            0224h
%define PORT_FB1_DOT            0228h
%define PORT_FB1_OFX            022ah
%define PORT_FB1_OFY            022ch
%define PORT_FB1_DSA_LOW        022eh
%define PORT_FB1_DSA_HIGH       0230h
%define PORT_FB1_DSH            0232h
%define PORT_FB1_DSP            0236h
%define PORT_FB0_DSA_LOW        020eh
%define PORT_FB0_DSA_HIGH       0210h

%define MEMORY_MAP_TVRAM        041h
%define MEMORY_MAP_GVRAM        054h
%define GVRAM_CPU_WRITE_MODE    010h
%define TSP_VBLANK              040h
%define SGP_BUSY                001h

%define COMPOSE_G1_OVER_G0      0034h
%define RGB_COMPOSE_G1_OVER_G0  0089h
%define G0_SEGMENT              0a000h

%define SCREEN_WIDTH            320
%define SCREEN_HEIGHT           200
%define BYTES_PER_LINE          320
%define WORDS_PER_LINE          160
%define SCREEN_WORDS            (WORDS_PER_LINE * SCREEN_HEIGHT)
%define PAGE_BYTES              (BYTES_PER_LINE * SCREEN_HEIGHT)
%define PAGE_A_SGP_LOW          0000h
%define PAGE_A_SGP_HIGH         0022h
%define PAGE_B_SGP              022fa00h
%define PAGE_B_SGP_LOW          0fa00h
%define PAGE_B_SGP_HIGH         0022h
%define PAGE_A_DSA              020000h
%define PAGE_B_DSA              02fa00h

%define SGP_END                 0001h
%define SGP_SET_WORK            0003h
%define SGP_SET_COLOR           0006h
%define SGP_CLS                 000ah
%define SGP_LINE                0009h
%define SGP_LINE_COPY           0005h
; The real VA BLTMODE direction meanings are the same as the validated
; NEON3/GLASS payload: HD=0400h and VD=0800h.  VAEG's internal enum uses
; opposite names for its legacy line model; the build script can select that
; model explicitly for emulator-only comparison, but hardware is the default.
%ifndef NEON4_SGP_REAL_DIRECTION
%define NEON4_SGP_REAL_DIRECTION 1
%endif
%if NEON4_SGP_REAL_DIRECTION
%define SGP_LINE_HD             0400h
%define SGP_LINE_VD             0800h
%else
%define SGP_LINE_HD             0800h
%define SGP_LINE_VD             0400h
%endif

%define P4_LIST_WORDS           4096
%define P4_TEST_X0              040
%define P4_TEST_Y0              040
%define P4_TEST_X1              280
%define P4_TEST_Y1              160
%define P4_TEST_FILL            01ch
%define P4_TEST_EDGE            0e3h

; ---------------------------------------------------------------------------
; Entry and stage dispatch.
; ---------------------------------------------------------------------------
start:
        cli
        push    cs
        pop     ds
        push    cs
        pop     es
        ; Keep the loader-provided SS:SP until the video BIOS transaction has
        ; completed.  The VA BIOS uses the caller stack internally; the
        ; loader stack is the only stack contract verified by the existing
        ; payloads.  A private E000h stack makes INT 8Fh hang in VAEG before
        ; returning from the mode call.
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
        ; The text checkpoint is written only after the graphics BIOS has
        ; established a valid VA memory map.  Pre-mode TVRAM writes are not
        ; safe on the 8bpp BIOS path.
%if NEON4_DIAG_MARKER
        call    diag_write_text_marker
%endif
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
        call    set_display_page_a
        mov     al, 024h
        call    sgp_clear_page
        jc      stage_failure
        jmp     wait_for_escape
%elif NEON4_STAGE == 6
        ; N4-6: render both hidden G1 pages with SGP, then exchange FB1 DSA
        ; at a VBLANK edge.  This follows the SGP256 two-page sequence.
        mov     al, 01ch
        call    sgp_clear_page
        jc      stage_failure
        mov     al, 0e3h
        call    sgp_clear_page_b
        jc      stage_failure
        call    wait_vblank_edge
        jc      stage_failure
        call    set_display_page_b
        jmp     wait_for_escape
%elif NEON4_STAGE == 7
        ; P4: run the same deterministic primitive scene through either the
        ; CPU reference writer or the SGP command-list writer.
        call    p4_run_test_scene
        jc      stage_failure
        jmp     wait_for_escape
%else
%error "NEON4_STAGE must be 1..7"
%endif
%endif

stage_failure:
        ; Keep the failure path free of Text BIOS calls.  INT 83h is only
        ; safe while a text composition is active; a partial Graphics BIOS
        ; setup may leave the ROM in G0-only or an undefined composition.
        ; The red page is therefore the only P3 failure marker.
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
; VA graphics setup.  The mode call and FB1 setup follow the SGP256 payload.
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
        mov     bx, NEON4_MODE
        mov     cx, NEON4_PIXEL_ARGS
        xor     dx, dx
        xor     ax, ax
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     .failed
%if NEON4_USE_DEFAULT_BUFFERS
        ; The proven 320x200 VA payload leaves the BIOS-created descriptors
        ; untouched and verifies them before drawing.  Keep that path as the
        ; default while the 8bpp descriptor contract is unverified.
%else
        ; Define the framebuffer surface and display window explicitly only
        ; for a separately calibrated mode.  This path is not used by the
        ; default P3 diagnostic build.
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
%endif
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
        ; Select the direct RGB composition slots used by the 8bpp G1-over-G0
        ; path.  The low nibble is the highest-priority slot: G1 (9) over G0
        ; (8).  This is the same composition contract as SGP256.
        mov     dx, PORT_COL_COMP
        xor     ax, ax
        out     dx, ax
        mov     dx, PORT_RGB_COMP
        mov     ax, RGB_COMPOSE_G1_OVER_G0
        out     dx, ax
        mov     ax, 0300h
        mov     cx, COMPOSE_G1_OVER_G0
        int     VIDEO_BIOS_INT
        or      ax, ax
        jnz     .failed
        ; Keep G0 opaque and G1 index zero transparent, as in SGP256.
        mov     dx, PORT_G0_TRANSPARENCY
        xor     ax, ax
        out     dx, ax
        mov     dx, PORT_G1_TRANSPARENCY
        mov     ax, 0001h
        out     dx, ax
%if NEON4_DIRECT_REGS
        ; Optional experimental G0-direct calibration path.
        call    configure_rgb332_mode
%else
        call    configure_g1_framebuffer
        call    set_display_page_a
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
        mov     [cs:video_error_code], ax
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

; Configure the G1 8bpp backing surface used by the SGP256 path.  FB1 is a
; 320-byte-pitch, 400-line surface with a 200-line display window; DSA1
; selects the upper or lower page.
configure_g1_framebuffer:
        push    ax
        push    dx
        mov     dx, PORT_FB1_FBW
        mov     ax, BYTES_PER_LINE
        out     dx, ax
        mov     dx, PORT_FB1_DOT
        xor     ax, ax
        out     dx, ax
        mov     dx, PORT_FB1_OFX
        out     dx, ax
        mov     dx, PORT_FB1_OFY
        out     dx, ax
        mov     dx, PORT_FB1_DSH
        mov     ax, SCREEN_HEIGHT
        out     dx, ax
        mov     dx, PORT_FB1_DSP
        xor     ax, ax
        out     dx, ax
        pop     dx
        pop     ax
        ret

; [NOT VERIFIED ON VA SILICON] Optional direct G0 RGB332 calibration path.
; The default P3 path uses the BIOS 8bpp + FB1 contract above.
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
        mov     bx, PAGE_A_SGP_HIGH
        jmp     sgp_clear_with_base

sgp_clear_page_b:
        mov     dx, PAGE_B_SGP_LOW
        mov     bx, PAGE_B_SGP_HIGH
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
        mov     [cs:sgp_destination_high], bx
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

; FB1 DSA is a pair of word ports.  Keep the low/high writes separate, as in
; the SGP256 payload; byte writes can hang real VA hardware.
set_display_page_a:
        mov     dx, PORT_FB1_DSA_LOW
        mov     ax, PAGE_A_DSA & 0ffffh
        out     dx, ax
        add     dx, 2
        mov     ax, PAGE_A_DSA >> 16
        out     dx, ax
        ret

set_display_page_b:
        mov     dx, PORT_FB1_DSA_LOW
        mov     ax, PAGE_B_DSA & 0ffffh
        out     dx, ax
        add     dx, 2
        mov     ax, PAGE_B_DSA >> 16
        out     dx, ax
        ret

; ---------------------------------------------------------------------------
; P4 deterministic primitive backend.
; The test scene deliberately uses the same logical shape in both builds:
; the CPU build writes packed 8bpp G0 pixels, while the SGP build emits
; SET_COLOR/CLS/LINE commands for G1.  The visible composition is selected
; accordingly so the two outputs can be compared without sharing a writer.
; ---------------------------------------------------------------------------
p4_run_test_scene:
%if NEON4_P4_BACKEND == 0
        call    p4_cpu_scene
%else
        call    p4_sgp_scene
%endif
        clc
        ret

p4_set_cpu_composition:
        push    ax
        push    dx
        mov     dx, PORT_COL_COMP
        xor     ax, ax
        out     dx, ax
        mov     dx, PORT_RGB_COMP
        mov     ax, 0008h
        out     dx, ax
        pop     dx
        pop     ax
        ret

p4_set_sgp_composition:
        push    ax
        push    dx
        mov     dx, PORT_COL_COMP
        xor     ax, ax
        out     dx, ax
        mov     dx, PORT_RGB_COMP
        mov     ax, RGB_COMPOSE_G1_OVER_G0
        out     dx, ax
        pop     dx
        pop     ax
        ret

p4_cpu_scene:
        call    p4_set_cpu_composition
        xor     al, al
        call    cpu_clear_page
        mov     byte [p4_draw_color], P4_TEST_FILL
        call    p4_cpu_fill_rect
        mov     byte [p4_draw_color], P4_TEST_EDGE
        mov     ax, P4_TEST_X0
        mov     bx, P4_TEST_Y0
        mov     cx, P4_TEST_X1
        mov     dx, P4_TEST_Y0
        call    p4_cpu_line
        mov     ax, P4_TEST_X1
        mov     bx, P4_TEST_Y0
        mov     cx, P4_TEST_X1
        mov     dx, P4_TEST_Y1
        call    p4_cpu_line
        mov     ax, P4_TEST_X1
        mov     bx, P4_TEST_Y1
        mov     cx, P4_TEST_X0
        mov     dx, P4_TEST_Y1
        call    p4_cpu_line
        mov     ax, P4_TEST_X0
        mov     bx, P4_TEST_Y1
        mov     cx, P4_TEST_X0
        mov     dx, P4_TEST_Y0
        call    p4_cpu_line
        mov     ax, P4_TEST_X0
        mov     bx, P4_TEST_Y0
        mov     cx, P4_TEST_X1
        mov     dx, P4_TEST_Y1
        call    p4_cpu_line
        mov     ax, P4_TEST_X1
        mov     bx, P4_TEST_Y0
        mov     cx, P4_TEST_X0
        mov     dx, P4_TEST_Y1
        jmp     p4_cpu_line

p4_cpu_fill_rect:
        push    ax
        push    bx
        push    cx
        push    dx
        mov     word [p4_rect_x0], P4_TEST_X0
        mov     word [p4_rect_y0], P4_TEST_Y0
        mov     word [p4_rect_x1], P4_TEST_X1
        mov     word [p4_rect_y1], P4_TEST_Y1
        mov     dx, [p4_rect_y0]
.row:
        mov     cx, [p4_rect_x0]
.pixel:
        mov     al, [p4_draw_color]
        call    p4_cpu_put_pixel
        inc     cx
        cmp     cx, [p4_rect_x1]
        jle     .pixel
        inc     dx
        cmp     dx, [p4_rect_y1]
        jle     .row
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

; AL = colour, CX = physical X, DX = physical Y.
p4_cpu_put_pixel:
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        push    es
        mov     [p4_pixel_color], al
        mov     si, cx
        mov     ax, dx
        mov     bx, BYTES_PER_LINE
        mul     bx
        and     cx, 0fffeh
        add     ax, cx
        mov     di, ax
        mov     ax, G0_SEGMENT
        mov     es, ax
        mov     ax, [es:di]
        test    si, 1
        jnz     .odd
        mov     al, [p4_pixel_color]
        jmp     .store
.odd:
        mov     ah, [p4_pixel_color]
.store:
        mov     [es:di], ax
        pop     es
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

; AX=x0, BX=y0, CX=x1, DX=y1.  The color is p4_draw_color.
; The software reference follows the SGP LINE accumulator convention rather
; than an unrelated Bresenham tie rule, so CPU and SGP paths have identical
; endpoint ownership for the P4 comparison scene.
p4_cpu_line:
        mov     [p4_line_x0], ax
        mov     [p4_line_y0], bx
        mov     [p4_line_x1], cx
        mov     [p4_line_y1], dx
        mov     ax, [p4_line_x1]
        sub     ax, [p4_line_x0]
        jns     .x_positive
        neg     ax
        mov     word [p4_line_sx], -1
        jmp     .x_delta
.x_positive:
        mov     word [p4_line_sx], 1
.x_delta:
        mov     [p4_line_dx], ax
        mov     ax, [p4_line_y1]
        sub     ax, [p4_line_y0]
        jns     .y_positive
        neg     ax
        mov     word [p4_line_sy], -1
        jmp     .y_delta
.y_positive:
        mov     word [p4_line_sy], 1
.y_delta:
        mov     [p4_line_dy], ax
        mov     ax, [p4_line_dx]
        cmp     ax, [p4_line_dy]
        jb      .y_major

        ; X-major: denominator=width-1, numerator=height-1, and the
        ; accumulator starts at (denominator-1)/2 as in cmd_line().
        mov     [p4_line_denominator], ax
        mov     ax, [p4_line_dy]
        mov     [p4_line_numerator], ax
        mov     ax, [p4_line_denominator]
        or      ax, ax
        jz      .single_point
        dec     ax
        shr     ax, 1
        mov     [p4_line_count], ax
        mov     ax, [p4_line_denominator]
        inc     ax
        mov     [p4_line_steps], ax
.x_loop:
        mov     al, [p4_draw_color]
        mov     cx, [p4_line_x0]
        mov     dx, [p4_line_y0]
        call    p4_cpu_put_pixel
        dec     word [p4_line_steps]
        jz      .done
        mov     ax, [p4_line_x0]
        add     ax, [p4_line_sx]
        mov     [p4_line_x0], ax
        mov     ax, [p4_line_count]
        add     ax, [p4_line_numerator]
        cmp     ax, [p4_line_denominator]
        jb      .x_no_y
        sub     ax, [p4_line_denominator]
        mov     bx, [p4_line_y0]
        add     bx, [p4_line_sy]
        mov     [p4_line_y0], bx
.x_no_y:
        mov     [p4_line_count], ax
        jmp     .x_loop

.y_major:
        ; Y-major: denominator=height-1, numerator=width-1, and the
        ; accumulator starts at denominator/2 as in cmd_line().
        mov     [p4_line_denominator], ax
        mov     ax, [p4_line_dy]
        mov     [p4_line_denominator], ax
        mov     ax, [p4_line_dx]
        mov     [p4_line_numerator], ax
        mov     ax, [p4_line_denominator]
        or      ax, ax
        jz      .single_point
        shr     ax, 1
        mov     [p4_line_count], ax
        mov     ax, [p4_line_denominator]
        inc     ax
        mov     [p4_line_steps], ax
.y_loop:
        mov     al, [p4_draw_color]
        mov     cx, [p4_line_x0]
        mov     dx, [p4_line_y0]
        call    p4_cpu_put_pixel
        dec     word [p4_line_steps]
        jz      .done
        mov     ax, [p4_line_y0]
        add     ax, [p4_line_sy]
        mov     [p4_line_y0], ax
        mov     ax, [p4_line_count]
        add     ax, [p4_line_numerator]
        cmp     ax, [p4_line_denominator]
        jb      .y_no_x
        sub     ax, [p4_line_denominator]
        mov     bx, [p4_line_x0]
        add     bx, [p4_line_sx]
        mov     [p4_line_x0], bx
.y_no_x:
        mov     [p4_line_count], ax
        jmp     .y_loop

.single_point:
        mov     al, [p4_draw_color]
        mov     cx, [p4_line_x0]
        mov     dx, [p4_line_y0]
        call    p4_cpu_put_pixel
.done:
        ret

p4_sgp_scene:
        call    p4_set_sgp_composition
        xor     al, al
        call    sgp_clear_page
        jc      .done
        call    p4_build_sgp_scene
        call    run_sgp_command_list
.done:
        ret

p4_build_sgp_scene:
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        push    di
        push    es
        push    cs
        pop     es
        mov     di, sgp_command_list
        mov     ax, SGP_SET_WORK
        stosw
        mov     si, sgp_work_area
        call    physical_address_from_ds_si
        stosw
        mov     ax, dx
        stosw
        mov     ax, P4_TEST_FILL
        call    p4_sgp_emit_set_color
        mov     bx, P4_TEST_Y0
.fill_row:
        mov     ax, P4_TEST_X0
        mov     cx, (P4_TEST_X1-P4_TEST_X0+1)/2
        call    p4_sgp_emit_cls_span
        inc     bx
        cmp     bx, P4_TEST_Y1
        jle     .fill_row
        mov     ax, P4_TEST_EDGE
        call    p4_sgp_emit_set_color
        mov     ax, P4_TEST_X0
        mov     bx, P4_TEST_Y0
        mov     cx, P4_TEST_X1
        mov     dx, P4_TEST_Y0
        call    p4_sgp_emit_line
        mov     ax, P4_TEST_X1
        mov     bx, P4_TEST_Y0
        mov     cx, P4_TEST_X1
        mov     dx, P4_TEST_Y1
        call    p4_sgp_emit_line
        mov     ax, P4_TEST_X1
        mov     bx, P4_TEST_Y1
        mov     cx, P4_TEST_X0
        mov     dx, P4_TEST_Y1
        call    p4_sgp_emit_line
        mov     ax, P4_TEST_X0
        mov     bx, P4_TEST_Y1
        mov     cx, P4_TEST_X0
        mov     dx, P4_TEST_Y0
        call    p4_sgp_emit_line
        mov     ax, P4_TEST_X0
        mov     bx, P4_TEST_Y0
        mov     cx, P4_TEST_X1
        mov     dx, P4_TEST_Y1
        call    p4_sgp_emit_line
        mov     ax, P4_TEST_X1
        mov     bx, P4_TEST_Y0
        mov     cx, P4_TEST_X0
        mov     dx, P4_TEST_Y1
        call    p4_sgp_emit_line
        mov     ax, SGP_END
        stosw
        pop     es
        pop     di
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

p4_sgp_emit_set_color:
        push    bx
        mov     ah, al
        mov     bx, ax
        mov     ax, SGP_SET_COLOR
        stosw
        mov     ax, bx
        stosw
        pop     bx
        ret

; AX=x, BX=y, CX=word count.  The span is physical and page-A based.
p4_sgp_emit_cls_span:
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        mov     [p4_span_x], ax
        mov     [p4_span_y], bx
        mov     [p4_span_words], cx
        mov     ax, bx
        mov     bx, BYTES_PER_LINE
        mul     bx
        add     ax, [p4_span_x]
        adc     dx, 0
        add     ax, PAGE_A_SGP_LOW
        adc     dx, PAGE_A_SGP_HIGH
        mov     si, ax
        mov     ax, SGP_CLS
        stosw
        mov     ax, si
        stosw
        mov     ax, dx
        stosw
        mov     ax, [p4_span_words]
        stosw
        xor     ax, ax
        stosw
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
        ret

; AX=x0, BX=y0, CX=x1, DX=y1.  Endpoints are physical pixels.
p4_sgp_emit_line:
        push    ax
        push    bx
        push    cx
        push    dx
        push    si
        mov     [p4_line_x0], ax
        mov     [p4_line_y0], bx
        mov     [p4_line_x1], cx
        mov     [p4_line_y1], dx
        ; LINE copy mode is the documented 8bpp-compatible copy variant used
        ; by the NEON3 backend.  Direction bits are added below.
        mov     bx, SGP_LINE_COPY
        mov     ax, [p4_line_x1]
        sub     ax, [p4_line_x0]
        jns     .x_ok
        neg     ax
        or      bx, SGP_LINE_HD
.x_ok:
        inc     ax
        mov     [p4_line_dx], ax
        mov     ax, [p4_line_y1]
        sub     ax, [p4_line_y0]
        jns     .y_ok
        neg     ax
        or      bx, SGP_LINE_VD
.y_ok:
        inc     ax
        mov     [p4_line_dy], ax
        mov     ax, [p4_line_y0]
        mov     si, BYTES_PER_LINE
        mul     si
        mov     si, [p4_line_x0]
        and     si, 0fffeh
        add     ax, si
        adc     dx, 0
        add     ax, PAGE_A_SGP_LOW
        adc     dx, PAGE_A_SGP_HIGH
        mov     si, ax
        mov     [p4_line_address_high], dx
        mov     ax, SGP_LINE
        stosw
        mov     ax, bx
        stosw
        mov     ax, [p4_line_x0]
        and     ax, 1
        shl     ax, 4
        ; Descriptor mode 2 is packed 8bpp; bit 4 selects the odd pixel
        ; within the two-pixel word.
        or      ax, 2
        stosw
        mov     ax, [p4_line_dx]
        stosw
        mov     ax, [p4_line_dy]
        stosw
        mov     ax, BYTES_PER_LINE
        stosw
        mov     ax, si
        stosw
        mov     ax, [p4_line_address_high]
        stosw
        pop     si
        pop     dx
        pop     cx
        pop     bx
        pop     ax
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

; Put a checkpoint on the existing text plane before the first 8bpp mode
; transaction.  The text BIOS is not safe before a graphics mode has been
; established on the tested ROMs, so this diagnostic marker writes the known
; VA TVRAM character/attribute planes directly.  It is not a production text
; path and does not touch GVRAM.
diag_write_text_marker:
        push    ax
        push    bx
        push    dx
        push    si
        push    di
        push    ds
        push    es
        mov     dx, PORT_MEMORY_MAP
        mov     al, MEMORY_MAP_TVRAM
        out     dx, al
        push    cs
        pop     ds
        mov     ax, G0_SEGMENT
        mov     es, ax
        mov     si, diag_pre_mode_text
        mov     di, 0a00h             ; row 10, column 0 in the text plane
        mov     bx, 8a00h             ; matching attribute bytes
.character:
        lodsb
        test    al, al
        jz      .done
        xor     ah, ah
        stosw
        mov     byte [es:bx], 07h
        inc     bx
        jmp     .character
.done:
        ; Restore the graphics memory map before returning to the video setup.
        ; Leaving TVRAM selected would make the next stage depend on a
        ; diagnostic-only bank selection.
        mov     dx, PORT_MEMORY_MAP
        mov     al, MEMORY_MAP_GVRAM
        out     dx, al
        pop     es
        pop     ds
        pop     di
        pop     si
        pop     dx
        pop     bx
        pop     ax
        ret

align 2
sgp_command_list:
        times P4_LIST_WORDS dw 0
sgp_work_area:
        times 29 dw 0
sgp_destination_low dw 0
sgp_destination_high dw 0
sgp_colour_byte db 0
        align 2

; P4 diagnostic state.  These are scratch words only; the P4 path is entered
; after the normal BIOS mode setup and does not share state with the P3 probes.
p4_draw_color db 0
p4_pixel_color db 0
        align 2
p4_rect_x0 dw 0
p4_rect_y0 dw 0
p4_rect_x1 dw 0
p4_rect_y1 dw 0
p4_line_x0 dw 0
p4_line_y0 dw 0
p4_line_x1 dw 0
p4_line_y1 dw 0
p4_line_sx dw 0
p4_line_sy dw 0
p4_line_dx dw 0
p4_line_dy dw 0
p4_line_err dw 0
p4_line_count dw 0
p4_line_denominator dw 0
p4_line_numerator dw 0
p4_line_steps dw 0
p4_line_address_high dw 0
p4_span_x dw 0
p4_span_y dw 0
p4_span_words dw 0
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

diag_pre_mode_text db 'N4 ENTER OK', 0
video_error_code dw 0

; These words are intentionally reserved for the P3 command builder.  The
; loader writes its return continuation at CS:0e000..0e008.
%if ($ - $$) >= 0e000h
%error "NEON4 P3 payload overlaps the loader return reserve"
%endif
