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

bits 16
org 0x100

%ifndef MILESTONE_STAGE
%define MILESTONE_STAGE         6
%endif

%define KEYBOARD_BIOS_INT       0x82
%define CALENDAR_BIOS_INT       0x8c
%define VIDEO_BIOS_INT          0x8f
%define VIDEO_BIOS_DATA_SEG     0x0338
%define VIDEO_MODE_OFFSET       0x000f
%define VIDEO_G0_BPP_OFFSET     0x0011
%define VIDEO_G1_BPP_OFFSET     0x0012

%define PORT_MEMORY_MAP         0x0153
%define PORT_G0_TRANSPARENCY    0x0124
%define PORT_G1_TRANSPARENCY    0x0126
%define PORT_FB1_DSA_LOW        0x022e
%define PORT_FB1_DSA_HIGH        0x0230
%define PORT_TSP_STATUS         0x0142
%define PORT_SGP_COMMAND        0x0500
%define PORT_SGP_CONTROL        0x0504
%define PORT_SGP_STATUS         0x0506
%define PORT_GVRAM_WRITE_MODE   0x0580

%define MEMORY_MAP_GVRAM_SINGLE 0x54
%define GVRAM_CPU_WRITE_MODE    0x10

%define MODE_320X200_G0_G1      0xe00e
%define PIXEL_SIZE_G0_G1_4BPP   0x0404
%define COMPOSE_G1_OVER_G0      0x0034
%define TSP_STATUS_VBLANK       0x40

%define G0_SEGMENT              0xa000
%define SCREEN_WIDTH            320
%define SCREEN_HEIGHT           200
%define SCREEN_PITCH            160

%define SGP_COMMAND_END         0x0001
%define SGP_COMMAND_SET_WORK    0x0003
%define SGP_COMMAND_SET_SOURCE  0x0004
%define SGP_COMMAND_SET_DEST    0x0005
%define SGP_COMMAND_SET_COLOR   0x0006
%define SGP_COMMAND_BITBLT      0x0007
%define SGP_COMMAND_CLS         0x000a
%define SGP_BITBLT_COPY_XPAR    0x0105
%define SGP_BUSY                0x01

%define BALL_COUNT              16
%if MILESTONE_STAGE == 5
%define BULLET_COUNT            0
%define SPRITE_INITIAL_COUNT    BALL_COUNT
%define SPRITE_MIN_COUNT        1
%define SPRITE_MAX_COUNT        256
%elif MILESTONE_STAGE == 4
%define BULLET_COUNT            0
%define SPRITE_INITIAL_COUNT    BALL_COUNT
%define SPRITE_MIN_COUNT        1
%define SPRITE_MAX_COUNT        BALL_COUNT
%elif MILESTONE_STAGE == 3
%define BULLET_COUNT            0
%define SPRITE_INITIAL_COUNT    1
%define SPRITE_MIN_COUNT        1
%define SPRITE_MAX_COUNT        BALL_COUNT
%else
%define BULLET_COUNT            32
%define SPRITE_INITIAL_COUNT    BALL_COUNT
%define SPRITE_MIN_COUNT        1
%define SPRITE_MAX_COUNT        256
%endif
%define BULLET_FIRST_INDEX      BALL_COUNT
%define SPRITE_WIDTH            24
%define SPRITE_HEIGHT           24
%define SPRITE_PITCH            12
%define SPRITE_BITMAP_BYTES     (SPRITE_PITCH * SPRITE_HEIGHT)
%define BULLET_WIDTH            8
%define BULLET_HEIGHT           8
%define BULLET_PITCH            4
%define BULLET_BITMAP_BYTES     (BULLET_PITCH * BULLET_HEIGHT)
%define FPS_GLYPH_COUNT         11
%define FPS_GLYPH_WIDTH         4
%define FPS_GLYPH_HEIGHT        7
%define FPS_GLYPH_PITCH         2
%define FPS_GLYPH_X             260
%define FPS_GLYPH_Y             4
%define FPS_GLYPH_ADVANCE       5
%define G1_PAGE_A_SGP_BASE      0x220000
%define G1_PAGE_B_SGP_BASE      0x227d00
%define G1_PAGE_A_DSA           0x020000
%define G1_PAGE_B_DSA           0x027d00
%define SCREEN_WORD_COUNT       0x3e80
%define SGP_COMMAND_WORD_COUNT  (11 + (SPRITE_MAX_COUNT + FPS_GLYPH_COUNT) * 16)

%define RGB565(r,g,b)           ((((g) & 0x3f) << 10) | (((r) & 0x1f) << 5) | ((b) & 0x1f))
%define BALL_PAIR(a,b)          ((((a) & 0x0f) << 4) | ((b) & 0x0f))

; Pre-rendered 24x24 sphere raster: D=shadow, A=base hue,
; B=adjacent hue, F=highlight, and zero=transparent.
%macro DEFINE_HSV_BALL 3
%1:
    db BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0)
    db BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 13), BALL_PAIR(13, 13), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(13, 13), BALL_PAIR(13, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0)
    db BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(13, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%3, 13), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0)
    db BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(13, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %3), BALL_PAIR(%3, 13), BALL_PAIR(0, 0), BALL_PAIR(0, 0)
    db BALL_PAIR(0, 0), BALL_PAIR(0, 13), BALL_PAIR(%2, %2), BALL_PAIR(15, 15), BALL_PAIR(15, 15), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%3, %3), BALL_PAIR(13, 0), BALL_PAIR(0, 0)
    db BALL_PAIR(0, 0), BALL_PAIR(0, %2), BALL_PAIR(%2, 15), BALL_PAIR(15, 15), BALL_PAIR(15, 15), BALL_PAIR(15, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%3, %3), BALL_PAIR(13, 0), BALL_PAIR(0, 0)
    db BALL_PAIR(0, 0), BALL_PAIR(13, %2), BALL_PAIR(15, 15), BALL_PAIR(15, 15), BALL_PAIR(15, 15), BALL_PAIR(15, 15), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%3, %3), BALL_PAIR(%3, 13), BALL_PAIR(0, 0)
    db BALL_PAIR(0, 13), BALL_PAIR(%2, %2), BALL_PAIR(15, 15), BALL_PAIR(15, 15), BALL_PAIR(15, 15), BALL_PAIR(15, 15), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%3, %3), BALL_PAIR(%3, 13), BALL_PAIR(13, 0)
    db BALL_PAIR(0, 13), BALL_PAIR(%2, %2), BALL_PAIR(15, 15), BALL_PAIR(15, 15), BALL_PAIR(15, 15), BALL_PAIR(15, 15), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%3, %3), BALL_PAIR(%3, 13), BALL_PAIR(13, 0)
    db BALL_PAIR(0, 13), BALL_PAIR(%2, %2), BALL_PAIR(%2, 15), BALL_PAIR(15, 15), BALL_PAIR(15, 15), BALL_PAIR(15, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %3), BALL_PAIR(%3, %3), BALL_PAIR(%3, 13), BALL_PAIR(13, 0)
    db BALL_PAIR(0, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, 15), BALL_PAIR(15, 15), BALL_PAIR(15, 15), BALL_PAIR(15, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %3), BALL_PAIR(%3, %3), BALL_PAIR(13, 13), BALL_PAIR(13, 0)
    db BALL_PAIR(0, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, 15), BALL_PAIR(15, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%3, %3), BALL_PAIR(%3, %3), BALL_PAIR(13, 13), BALL_PAIR(13, 0)
    db BALL_PAIR(0, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%3, %3), BALL_PAIR(%3, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 0)
    db BALL_PAIR(0, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %3), BALL_PAIR(%3, %3), BALL_PAIR(%3, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 0)
    db BALL_PAIR(0, 13), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%3, %3), BALL_PAIR(%3, %3), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 0)
    db BALL_PAIR(0, 13), BALL_PAIR(%3, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%3, %3), BALL_PAIR(%3, %3), BALL_PAIR(%3, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 0)
    db BALL_PAIR(0, 13), BALL_PAIR(%3, %3), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%2, %2), BALL_PAIR(%3, %3), BALL_PAIR(%3, %3), BALL_PAIR(%3, %3), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 0)
    db BALL_PAIR(0, 0), BALL_PAIR(13, %3), BALL_PAIR(%3, %3), BALL_PAIR(%3, %3), BALL_PAIR(%3, %3), BALL_PAIR(%3, %3), BALL_PAIR(%3, %3), BALL_PAIR(%3, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(0, 0)
    db BALL_PAIR(0, 0), BALL_PAIR(0, %3), BALL_PAIR(%3, %3), BALL_PAIR(%3, %3), BALL_PAIR(%3, %3), BALL_PAIR(%3, %3), BALL_PAIR(%3, %3), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 0), BALL_PAIR(0, 0)
    db BALL_PAIR(0, 0), BALL_PAIR(0, 13), BALL_PAIR(13, %3), BALL_PAIR(%3, %3), BALL_PAIR(%3, %3), BALL_PAIR(%3, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 0), BALL_PAIR(0, 0)
    db BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(0, 0), BALL_PAIR(0, 0)
    db BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0)
    db BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 13), BALL_PAIR(13, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0)
    db BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0)
%endmacro

%define SPRITE_X_OFFSET         0
%define SPRITE_Y_OFFSET         2
%define SPRITE_VX_OFFSET        4
%define SPRITE_VY_OFFSET        6
%define SPRITE_WIDTH_OFFSET     8
%define SPRITE_HEIGHT_OFFSET    10
%define SPRITE_PITCH_OFFSET     12
%define SPRITE_BITMAP_OFFSET    14
%define SPRITE_PRIORITY_OFFSET  16
%define SPRITE_RECORD_SIZE      18

%if MILESTONE_STAGE == 1
start:
    push cs
    pop ds
    cld
    mov dx, message_start
    call print_string
    mov ax, 0x4c00
    int 0x21
%else
start:
    push cs
    pop ds
    cld

    call save_video_state

    mov dx, message_start
    call print_string

    call initialize_video
%endif
    jc initialization_failed
    call initialize_fps_counter
    call initialize_m6_counters

%if MILESTONE_STAGE == 2
m2_wait_loop:
    call poll_keyboard
    jc animation_done
    jmp m2_wait_loop
%else
animation_loop:
    call poll_keyboard
    jc animation_done

    push cs
    pop ds
    call update_sprite_positions
    call render_sprite_frame
    jc animation_failed
    call update_fps_counter
    call flip_draw_page
    jc animation_failed
    call record_completed_frame
    jmp animation_loop
%endif

animation_done:
    call restore_video_state
    call print_m6_summary
    mov dx, message_done
    call print_string
    mov ax, 0x4c00
    int 0x21

initialization_failed:
    call restore_video_state
    mov dx, message_failed
    call print_string
    mov ax, 0x4c01
    int 0x21

animation_failed:
    call restore_video_state
    call print_m6_summary
    mov dx, message_animation_failed
    call print_string
    mov ax, 0x4c02
    int 0x21

set_draw_page_base:
    push ax
    cmp byte [draw_page_index], 0
    jne .page_b

    mov ax, G1_PAGE_A_SGP_BASE & 0xffff
    mov [draw_page_sgp_low], ax
    mov ax, G1_PAGE_A_SGP_BASE >> 16
    mov [draw_page_sgp_high], ax
    mov ax, G1_PAGE_A_DSA & 0xffff
    mov [draw_page_dsa_low], ax
    mov ax, G1_PAGE_A_DSA >> 16
    mov [draw_page_dsa_high], ax
    pop ax
    ret

.page_b:
    mov ax, G1_PAGE_B_SGP_BASE & 0xffff
    mov [draw_page_sgp_low], ax
    mov ax, G1_PAGE_B_SGP_BASE >> 16
    mov [draw_page_sgp_high], ax
    mov ax, G1_PAGE_B_DSA & 0xffff
    mov [draw_page_dsa_low], ax
    mov ax, G1_PAGE_B_DSA >> 16
    mov [draw_page_dsa_high], ax
    pop ax
    ret

; DSA1 is a pair of word registers; byte writes can hang real hardware.
set_display_page_from_draw:
    push ax
    push dx
    mov dx, PORT_FB1_DSA_LOW
    mov ax, [draw_page_dsa_low]
    out dx, ax
    add dx, 2
    mov ax, [draw_page_dsa_high]
    out dx, ax
    pop dx
    pop ax
    ret

flip_draw_page:
    call wait_vblank_start
    jc .failed
    call set_display_page_from_draw
    inc word [m6_page_flips_lo]
    jnz .page_flip_count_ready
    inc word [m6_page_flips_hi]
.page_flip_count_ready:
    xor byte [draw_page_index], 1
    call set_draw_page_base
    clc
    ret

.failed:
    stc
    ret
initialize_video:
    mov bx, MODE_320X200_G0_G1
    mov cx, PIXEL_SIZE_G0_G1_4BPP
    xor dx, dx
    mov byte [video_mode_changed], 1
    xor ax, ax
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed

    mov ax, 0x0b00
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed

    mov ax, 0x0900
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed

    mov ax, 0x0a00
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed

    call initialize_sprite_palette
    jc .failed

    mov ax, 0x0300
    mov cx, COMPOSE_G1_OVER_G0
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed

    call verify_default_buffers
    jc .failed

    mov dx, PORT_G0_TRANSPARENCY
    xor ax, ax
    out dx, ax

    mov dx, PORT_G1_TRANSPARENCY
    mov ax, 0x0001
    out dx, ax

    mov dx, PORT_MEMORY_MAP
    mov al, MEMORY_MAP_GVRAM_SINGLE
    out dx, al

    mov dx, PORT_GVRAM_WRITE_MODE
    mov al, GVRAM_CPU_WRITE_MODE
    out dx, al

    push cs
    pop ds
    call draw_g0_checkerboard
%if MILESTONE_STAGE >= 3
    mov byte [draw_page_index], 0
    call set_draw_page_base
    call render_sprite_frame
    jc .failed

    mov byte [draw_page_index], 1
    call set_draw_page_base
    call render_sprite_frame
    jc .failed
%endif

    call set_display_page_from_draw
    xor byte [draw_page_index], 1
    call set_draw_page_base

    mov ax, 0x0b01
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed

    clc
    ret

.failed:
    stc
    ret

initialize_sprite_palette:
    push bx
    push si
    xor bx, bx
    mov si, palette_values

.next_entry:
    mov ax, 0x0800
    mov al, bl
    mov cx, [si]
    push bx
    push si
    call set_palette_entry
    pop si
    pop bx
    jc .failed

    add si, 2
    inc bx
    cmp bx, 16
    jb .next_entry

    pop si
    pop bx
    clc
    ret

.failed:
    pop si
    pop bx
    stc
    ret

set_palette_entry:
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed
    clc
    ret

.failed:
    stc
    ret

verify_default_buffers:
    xor cx, cx
    mov ax, 0x0700
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed
    call verify_buffer_geometry
    jc .failed

    xor cx, cx
    mov ax, 0x0701
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed
    call verify_buffer_geometry
    ret

.failed:
    stc
    ret

verify_buffer_geometry:
    cmp word [es:di + 0], 4
    jne .failed
    cmp word [es:di + 2], SCREEN_WIDTH
    jne .failed
    cmp word [es:di + 4], SCREEN_HEIGHT
    jne .failed
    clc
    ret

.failed:
    stc
    ret

draw_g0_checkerboard:
    mov ax, G0_SEGMENT
    mov es, ax
    xor di, di
    xor bx, bx

.row:
    mov al, 0xdd
    test bl, 0x10
    jz .tile_row_ready
    mov al, 0xee

.tile_row_ready:
    mov cx, 20

.tile:
    mov ah, al
    push cx
    mov cx, 4
    rep stosw
    pop cx
    xor al, 0x33
    loop .tile

    inc bx
    cmp bx, SCREEN_HEIGHT
    jne .row
    ret


poll_keyboard:
    mov ah, 0x0a
    int KEYBOARD_BIOS_INT
    jc .no_key

    mov ah, 0x09
    int KEYBOARD_BIOS_INT
    cmp ah, 0x00
    je .escape
    cmp ah, 0x3a
    je .increase_count
    cmp ah, 0x3d
    je .decrease_count
    cmp al, '+'
    je .increase_count
    cmp al, '-'
    je .decrease_count

.no_key:
    clc
    ret

.increase_count:
    push cs
    pop ds
    cmp word [active_sprite_count], SPRITE_MAX_COUNT
    jae .no_key
    inc word [active_sprite_count]
    clc
    ret

.decrease_count:
    push cs
    pop ds
    cmp word [active_sprite_count], SPRITE_MIN_COUNT
    jbe .no_key
    dec word [active_sprite_count]
    clc
    ret

.escape:
    stc
    ret

initialize_fps_counter:
    push ax
    push bx
    push cx
    push dx
    push si

    push cs
    pop ds
    mov word [fps_frame_counter], 0
    mov word [fps_value], 0
    mov byte [fps_warmup], 1
    mov ah, 0x02
    int CALENDAR_BIOS_INT
    push cs
    pop ds
    mov [fps_last_second], dh
    call format_fps_glyphs

    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

update_fps_counter:
    push ax
    push bx
    push cx
    push dx
    push si

    push cs
    pop ds
    inc word [fps_frame_counter]
    mov ah, 0x02
    int CALENDAR_BIOS_INT
    push cs
    pop ds
    cmp dh, [fps_last_second]
    je .done
    mov [fps_last_second], dh

    cmp byte [fps_warmup], 0
    je .store_measurement
    mov byte [fps_warmup], 0
    mov word [fps_frame_counter], 0
    jmp .done

.store_measurement:
    mov ax, [fps_frame_counter]
    mov [fps_value], ax
    mov word [fps_frame_counter], 0
    call format_fps_glyphs

.done:
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

initialize_m6_counters:
    xor ax, ax
    mov [m6_frames_lo], ax
    mov [m6_frames_hi], ax
    mov [m6_page_flips_lo], ax
    mov [m6_page_flips_hi], ax
    mov [m6_sgp_commands_frame], ax
    mov [m6_sgp_bitblts_frame], ax
    mov [m6_sgp_pixels_frame_lo], ax
    mov [m6_sgp_pixels_frame_hi], ax
    mov [m6_sgp_source_bytes_frame_lo], ax
    mov [m6_sgp_source_bytes_frame_hi], ax
    mov [m6_total_commands_lo], ax
    mov [m6_total_commands_hi], ax
    mov [m6_total_bitblts_lo], ax
    mov [m6_total_bitblts_hi], ax
    mov [m6_total_pixels_lo], ax
    mov [m6_total_pixels_hi], ax
    mov [m6_total_source_bytes_lo], ax
    mov [m6_total_source_bytes_hi], ax
    mov [m6_missed_vblank_lo], ax
    mov [m6_missed_vblank_hi], ax
    ret

record_completed_frame:
    inc word [m6_frames_lo]
    jnz .ready
    inc word [m6_frames_hi]
.ready:
    ret

m6_accumulate_frame_counters:
    push ax
    push dx

    mov ax, [m6_sgp_commands_frame]
    add [m6_total_commands_lo], ax
    adc word [m6_total_commands_hi], 0

    mov ax, [m6_sgp_bitblts_frame]
    add [m6_total_bitblts_lo], ax
    adc word [m6_total_bitblts_hi], 0

    mov ax, [m6_sgp_pixels_frame_lo]
    mov dx, [m6_sgp_pixels_frame_hi]
    add [m6_total_pixels_lo], ax
    adc [m6_total_pixels_hi], dx

    mov ax, [m6_sgp_source_bytes_frame_lo]
    mov dx, [m6_sgp_source_bytes_frame_hi]
    add [m6_total_source_bytes_lo], ax
    adc [m6_total_source_bytes_hi], dx

    pop dx
    pop ax
    ret

print_m6_summary:
    push ax
    push dx

    mov dx, message_m6_summary
    call print_string
    mov dx, label_m6_frames
    call print_string
    mov ax, [m6_frames_lo]
    mov dx, [m6_frames_hi]
    call print_u32
    mov dx, label_m6_page_flips
    call print_string
    mov ax, [m6_page_flips_lo]
    mov dx, [m6_page_flips_hi]
    call print_u32
    mov dx, label_m6_last_commands
    call print_string
    mov ax, [m6_sgp_commands_frame]
    xor dx, dx
    call print_u32
    mov dx, label_m6_last_bitblts
    call print_string
    mov ax, [m6_sgp_bitblts_frame]
    xor dx, dx
    call print_u32
    mov dx, label_m6_last_pixels
    call print_string
    mov ax, [m6_sgp_pixels_frame_lo]
    mov dx, [m6_sgp_pixels_frame_hi]
    call print_u32
    mov dx, label_m6_last_bytes
    call print_string
    mov ax, [m6_sgp_source_bytes_frame_lo]
    mov dx, [m6_sgp_source_bytes_frame_hi]
    call print_u32
    mov dx, label_m6_total_commands
    call print_string
    mov ax, [m6_total_commands_lo]
    mov dx, [m6_total_commands_hi]
    call print_u32
    mov dx, label_m6_total_bitblts
    call print_string
    mov ax, [m6_total_bitblts_lo]
    mov dx, [m6_total_bitblts_hi]
    call print_u32
    mov dx, label_m6_total_pixels
    call print_string
    mov ax, [m6_total_pixels_lo]
    mov dx, [m6_total_pixels_hi]
    call print_u32
    mov dx, label_m6_total_bytes
    call print_string
    mov ax, [m6_total_source_bytes_lo]
    mov dx, [m6_total_source_bytes_hi]
    call print_u32
    mov dx, label_m6_active
    call print_string
    mov ax, [active_sprite_count]
    xor dx, dx
    call print_u32
    mov dx, label_m6_missed
    call print_string
    mov ax, [m6_missed_vblank_lo]
    mov dx, [m6_missed_vblank_hi]
    call print_u32

    pop dx
    pop ax
    ret

print_u32:
    push ax
    push bx
    push cx
    push dx
    push si
    push di

    mov [m6_print_value_lo], ax
    mov [m6_print_value_hi], dx
    mov di, m6_number_buffer + 11
    mov byte [di], '$'
    dec di
    mov bx, 10
.next_digit:
    xor dx, dx
    mov ax, [m6_print_value_hi]
    div bx
    mov [m6_print_value_hi], ax
    mov ax, [m6_print_value_lo]
    div bx
    mov [m6_print_value_lo], ax
    add dl, '0'
    mov [di], dl
    dec di
    cmp word [m6_print_value_hi], 0
    jne .next_digit
    cmp word [m6_print_value_lo], 0
    jne .next_digit
    inc di
    mov dx, di
    mov ah, 0x09
    int 0x21

    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

format_fps_glyphs:
    push ax
    push bx
    push dx
    push si

    mov ax, [fps_value]
    cmp ax, 999
    jbe .value_ready
    mov ax, 999

.value_ready:
    xor dx, dx
    mov bx, 100
    div bx
    mov si, ax
    shl si, 1
    mov bx, [digit_glyph_table + si]
    mov [fps_glyph_pointers + 6], bx

    mov ax, dx
    xor dx, dx
    mov bx, 10
    div bx
    mov si, ax
    shl si, 1
    mov bx, [digit_glyph_table + si]
    mov [fps_glyph_pointers + 8], bx

    mov si, dx
    shl si, 1
    mov bx, [digit_glyph_table + si]
    mov [fps_glyph_pointers + 10], bx

    mov ax, [active_sprite_count]
    xor dx, dx
    mov bx, 1000
    div bx
    mov si, ax
    shl si, 1
    mov bx, [digit_glyph_table + si]
    mov [fps_glyph_pointers + 14], bx

    mov ax, dx
    xor dx, dx
    mov bx, 100
    div bx
    mov si, ax
    shl si, 1
    mov bx, [digit_glyph_table + si]
    mov [fps_glyph_pointers + 16], bx

    mov ax, dx
    xor dx, dx
    mov bx, 10
    div bx
    mov si, ax
    shl si, 1
    mov bx, [digit_glyph_table + si]
    mov [fps_glyph_pointers + 18], bx

    mov si, dx
    shl si, 1
    mov bx, [digit_glyph_table + si]
    mov [fps_glyph_pointers + 20], bx

    pop si
    pop dx
    pop bx
    pop ax
    ret

wait_vblank_start:
    mov dx, PORT_TSP_STATUS
    mov bx, 4

.wait_display_interval:
    mov cx, 0xffff

.poll_display_interval:
    in al, dx
    test al, TSP_STATUS_VBLANK
    jz .display_interval_seen
    loop .poll_display_interval
    dec bx
    jnz .wait_display_interval
    inc word [m6_missed_vblank_lo]
    jnz .display_timeout_count_ready
    inc word [m6_missed_vblank_hi]
.display_timeout_count_ready:
    stc
    ret

.display_interval_seen:
    mov bx, 4

.wait_vblank_interval:
    mov cx, 0xffff

.poll_vblank_interval:
    in al, dx
    test al, TSP_STATUS_VBLANK
    jnz .ready
    loop .poll_vblank_interval
    dec bx
    jnz .wait_vblank_interval
    inc word [m6_missed_vblank_lo]
    jnz .vblank_timeout_count_ready
    inc word [m6_missed_vblank_hi]
.vblank_timeout_count_ready:
    stc
    ret

.ready:
    clc
    ret

update_sprite_positions:
    push ax
    push cx
    push dx
    push si

    mov si, sprite_records
    mov cx, [active_sprite_count]

.next_sprite:
    mov ax, [si + SPRITE_X_OFFSET]
    add ax, [si + SPRITE_VX_OFFSET]
    test ax, 0x8000
    jz .check_x_maximum
    xor ax, ax
    neg word [si + SPRITE_VX_OFFSET]
    jmp .store_x

.check_x_maximum:
    mov dx, SCREEN_WIDTH
    sub dx, [si + SPRITE_WIDTH_OFFSET]
    cmp ax, dx
    jbe .store_x
    mov ax, dx
    neg word [si + SPRITE_VX_OFFSET]

.store_x:
    mov [si + SPRITE_X_OFFSET], ax

    mov ax, [si + SPRITE_Y_OFFSET]
    add ax, [si + SPRITE_VY_OFFSET]
    test ax, 0x8000
    jz .check_y_maximum
    xor ax, ax
    neg word [si + SPRITE_VY_OFFSET]
    jmp .store_y

.check_y_maximum:
    mov dx, SCREEN_HEIGHT
    sub dx, [si + SPRITE_HEIGHT_OFFSET]
    cmp ax, dx
    jbe .store_y
    mov ax, dx
    neg word [si + SPRITE_VY_OFFSET]

.store_y:
    mov [si + SPRITE_Y_OFFSET], ax
    add si, SPRITE_RECORD_SIZE
    loop .next_sprite

    pop si
    pop dx
    pop cx
    pop ax
    ret

render_sprite_frame:
    call build_sgp_frame_commands
    call run_sgp_command_list
    ret

build_sgp_frame_commands:
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push bp
    push es

    push ds
    pop es
    xor ax, ax
    mov [m6_sgp_bitblts_frame], ax
    mov [m6_sgp_pixels_frame_lo], ax
    mov [m6_sgp_pixels_frame_hi], ax
    mov [m6_sgp_source_bytes_frame_lo], ax
    mov [m6_sgp_source_bytes_frame_hi], ax
    mov di, sgp_command_list

    mov ax, SGP_COMMAND_SET_WORK
    stosw
    mov si, sgp_work_area
    call physical_address_from_ds_si
    stosw
    mov ax, dx
    stosw

    mov ax, SGP_COMMAND_SET_COLOR
    stosw
    xor ax, ax
    stosw

    mov ax, SGP_COMMAND_CLS
    stosw
    mov ax, [draw_page_sgp_low]
    stosw
    mov ax, [draw_page_sgp_high]
    stosw
    mov ax, SCREEN_WORD_COUNT
    stosw
    xor ax, ax
    stosw

    mov si, sprite_records
    mov ax, [active_sprite_count]
    mov [m6_emit_remaining], ax

.emit_sprite:
    mov ax, SGP_COMMAND_SET_SOURCE
    stosw
    mov ax, 0x0001
    stosw
    mov ax, [si + SPRITE_WIDTH_OFFSET]
    stosw
    mov ax, [si + SPRITE_HEIGHT_OFFSET]
    stosw
    mov ax, [si + SPRITE_PITCH_OFFSET]
    stosw
    push si
    mov si, [si + SPRITE_BITMAP_OFFSET]
    call physical_address_from_ds_si
    pop si
    stosw
    mov ax, dx
    stosw

    mov ax, SGP_COMMAND_SET_DEST
    stosw
    mov ax, [si + SPRITE_X_OFFSET]
    and ax, 0x0003
    shl ax, 4
    or ax, 0x0001
    stosw
    mov ax, [si + SPRITE_WIDTH_OFFSET]
    stosw
    mov ax, [si + SPRITE_HEIGHT_OFFSET]
    stosw
    mov ax, SCREEN_PITCH
    stosw

    mov ax, [si + SPRITE_Y_OFFSET]
    mov bx, SCREEN_PITCH
    mul bx
    mov bx, [si + SPRITE_X_OFFSET]
    and bx, 0xfffc
    shr bx, 1
    add ax, bx
    adc dx, 0
    add ax, [draw_page_sgp_low]
    adc dx, [draw_page_sgp_high]
    stosw
    mov ax, dx
    stosw

    mov ax, SGP_COMMAND_BITBLT
    stosw
    mov ax, SGP_BITBLT_COPY_XPAR
    stosw
    inc word [m6_sgp_bitblts_frame]

    mov ax, [si + SPRITE_WIDTH_OFFSET]
    mov bx, [si + SPRITE_HEIGHT_OFFSET]
    mul bx
    add [m6_sgp_pixels_frame_lo], ax
    adc [m6_sgp_pixels_frame_hi], dx
    mov ax, [si + SPRITE_PITCH_OFFSET]
    mov bx, [si + SPRITE_HEIGHT_OFFSET]
    mul bx
    add [m6_sgp_source_bytes_frame_lo], ax
    adc [m6_sgp_source_bytes_frame_hi], dx

    add si, SPRITE_RECORD_SIZE
    dec word [m6_emit_remaining]
    jz short .sprites_done
    jmp .emit_sprite

.sprites_done:
    call emit_fps_glyph_commands
    mov ax, SGP_COMMAND_END
    stosw

    mov ax, di
    sub ax, sgp_command_list
    shr ax, 1
    mov [m6_sgp_commands_frame], ax
    call m6_accumulate_frame_counters

    mov si, sgp_command_list
    call physical_address_from_ds_si
    mov [sgp_command_address_low], ax
    mov [sgp_command_address_high], dx

    pop es
    pop bp
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

emit_fps_glyph_commands:
    push ax
    push bx
    push cx
    push dx
    push si
    push bp

    mov si, fps_glyph_pointers
    mov bx, FPS_GLYPH_X
    mov ax, FPS_GLYPH_COUNT
    mov [m6_emit_remaining], ax

.next_glyph:
    mov ax, SGP_COMMAND_SET_SOURCE
    stosw
    mov ax, 0x0001
    stosw
    mov ax, FPS_GLYPH_WIDTH
    stosw
    mov ax, FPS_GLYPH_HEIGHT
    stosw
    mov ax, FPS_GLYPH_PITCH
    stosw
    push si
    mov si, [si]
    call physical_address_from_ds_si
    pop si
    stosw
    mov ax, dx
    stosw

    mov ax, SGP_COMMAND_SET_DEST
    stosw
    mov ax, bx
    and ax, 0x0003
    shl ax, 4
    or ax, 0x0001
    stosw
    mov ax, FPS_GLYPH_WIDTH
    stosw
    mov ax, FPS_GLYPH_HEIGHT
    stosw
    mov ax, SCREEN_PITCH
    stosw

    mov ax, FPS_GLYPH_Y * SCREEN_PITCH
    xor dx, dx
    mov cx, bx
    and cx, 0xfffc
    shr cx, 1
    add ax, cx
    adc dx, 0
    add ax, [draw_page_sgp_low]
    adc dx, [draw_page_sgp_high]
    stosw
    mov ax, dx
    stosw

    mov ax, SGP_COMMAND_BITBLT
    stosw
    mov ax, SGP_BITBLT_COPY_XPAR
    stosw
    inc word [m6_sgp_bitblts_frame]
    add word [m6_sgp_pixels_frame_lo], FPS_GLYPH_WIDTH * FPS_GLYPH_HEIGHT
    adc word [m6_sgp_pixels_frame_hi], 0
    add word [m6_sgp_source_bytes_frame_lo], FPS_GLYPH_PITCH * FPS_GLYPH_HEIGHT
    adc word [m6_sgp_source_bytes_frame_hi], 0

    add si, 2
    add bx, FPS_GLYPH_ADVANCE
    dec word [m6_emit_remaining]
    jz short .glyphs_done
    jmp .next_glyph

.glyphs_done:
    pop bp
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

; The command pointer uses word writes at 0500h and 0502h.
; Control and start remain byte writes at 0504h and 0506h.
run_sgp_command_list:
    call wait_sgp_idle
    jc .failed

    mov dx, PORT_SGP_COMMAND
    mov ax, [sgp_command_address_low]
    out dx, ax
    add dx, 2
    mov ax, [sgp_command_address_high]
    out dx, ax

    mov dx, PORT_SGP_CONTROL
    xor al, al
    out dx, al

    mov dx, PORT_SGP_STATUS
    mov al, SGP_BUSY
    out dx, al

    call wait_sgp_idle
    ret

.failed:
    stc
    ret

wait_sgp_idle:
    mov dx, PORT_SGP_STATUS
    mov cx, 0xffff

.poll:
    in al, dx
    test al, SGP_BUSY
    jz .ready
    loop .poll
    stc
    ret

.ready:
    clc
    ret

physical_address_from_ds_si:
    mov ax, ds
    xor dx, dx
    mov cx, 4

.shift_segment:
    shl ax, 1
    rcl dx, 1
    loop .shift_segment

    add ax, si
    adc dx, 0
    ret

save_video_state:
    mov dx, PORT_MEMORY_MAP
    in al, dx
    mov [saved_memory_map], al
    test al, 0x10
    jz .save_bios_state

    mov byte [saved_single_plane], 1
    mov dx, PORT_GVRAM_WRITE_MODE
    in al, dx
    mov [saved_write_mode], al

.save_bios_state:
    mov ax, VIDEO_BIOS_DATA_SEG
    mov es, ax
    mov ax, [es:VIDEO_MODE_OFFSET]
    mov [saved_video_mode], ax
    mov al, [es:VIDEO_G0_BPP_OFFSET]
    mov [saved_g0_bpp], al
    mov al, [es:VIDEO_G1_BPP_OFFSET]
    mov [saved_g1_bpp], al
    ret

restore_video_state:
    push cs
    pop ds

    mov dx, PORT_MEMORY_MAP
    mov al, [saved_memory_map]
    out dx, al

    cmp byte [video_mode_changed], 0
    je .restore_write_mode

    mov bx, [saved_video_mode]
    mov cl, [saved_g0_bpp]
    mov ch, [saved_g1_bpp]
    xor dx, dx
    xor ax, ax
    int VIDEO_BIOS_INT

    mov ax, 0x0a00
    int VIDEO_BIOS_INT

    mov dx, PORT_MEMORY_MAP
    mov al, [saved_memory_map]
    out dx, al

.restore_write_mode:
    cmp byte [saved_single_plane], 0
    je .done
    mov dx, PORT_GVRAM_WRITE_MODE
    mov al, [saved_write_mode]
    out dx, al

.done:
    ret

print_string:
    mov ah, 0x09
    int 0x21
    ret

saved_memory_map:
    db 0
saved_write_mode:
    db 0
saved_single_plane:
    db 0
video_mode_changed:
    db 0
saved_video_mode:
    dw 0
saved_g0_bpp:
    db 0
saved_g1_bpp:
    db 0
sgp_command_address_low:
    dw 0
sgp_command_address_high:
    dw 0
active_sprite_count:
    dw SPRITE_INITIAL_COUNT
m6_emit_remaining:
    dw 0
fps_frame_counter:
    dw 0
fps_value:
    dw 0
fps_last_second:
    db 0
fps_warmup:
    db 1
draw_page_index:
    db 1
draw_page_sgp_low:
    dw G1_PAGE_B_SGP_BASE & 0xffff
draw_page_sgp_high:
    dw G1_PAGE_B_SGP_BASE >> 16
draw_page_dsa_low:
    dw G1_PAGE_B_DSA & 0xffff
draw_page_dsa_high:
    dw G1_PAGE_B_DSA >> 16

; M6 diagnostics are 32-bit counters and wrap only after 0xffffffff.
m6_frames_lo:
    dw 0
m6_frames_hi:
    dw 0
m6_page_flips_lo:
    dw 0
m6_page_flips_hi:
    dw 0
m6_sgp_commands_frame:
    dw 0
m6_sgp_bitblts_frame:
    dw 0
m6_sgp_pixels_frame_lo:
    dw 0
m6_sgp_pixels_frame_hi:
    dw 0
m6_sgp_source_bytes_frame_lo:
    dw 0
m6_sgp_source_bytes_frame_hi:
    dw 0
m6_total_commands_lo:
    dw 0
m6_total_commands_hi:
    dw 0
m6_total_bitblts_lo:
    dw 0
m6_total_bitblts_hi:
    dw 0
m6_total_pixels_lo:
    dw 0
m6_total_pixels_hi:
    dw 0
m6_total_source_bytes_lo:
    dw 0
m6_total_source_bytes_hi:
    dw 0
m6_missed_vblank_lo:
    dw 0
m6_missed_vblank_hi:
    dw 0
m6_print_value_lo:
    dw 0
m6_print_value_hi:
    dw 0
m6_number_buffer:
    times 12 db 0

message_start:
%if MILESTONE_STAGE == 1
    db "SGPDEMO1: M1 hardware inventory diagnostic", 13, 10
    db "See the M1 investigation report for verified interfaces.", 13, 10, "$"
%elif MILESTONE_STAGE == 2
    db "SGPDEMO2: M2 video bring-up (Graphic 0 background)", 13, 10
    db "ESC exits.", 13, 10, "$"
%elif MILESTONE_STAGE == 3
    db "SGPDEMO3: M3 transparent SGP BITBLT", 13, 10
    db "ESC exits.", 13, 10, "$"
%elif MILESTONE_STAGE == 4
    db "SGPDEMO4: M4 multiple pseudo-sprites", 13, 10
    db "ESC exits.", 13, 10, "$"
%elif MILESTONE_STAGE == 5
    db "SGPDEMO5: M5 double-buffered pseudo-sprites", 13, 10
    db "UP/+ adds balls (max 256), DOWN/- removes one, ESC exits.", 13, 10, "$"
%else
    db "SGPDEMO6: M6 stress/counters", 13, 10
    db "UP/+ adds a sprite (max 256), DOWN/- removes one, ESC exits.", 13, 10, "$"
%endif
message_m6_summary:
    db 13, 10, "M6 SGP counters (last frame and totals):", 13, 10, "$"
label_m6_frames:
    db "Frames: ", 13, 10, "$"
label_m6_page_flips:
    db "Page flips: ", 13, 10, "$"
label_m6_last_commands:
    db "Last command words: ", 13, 10, "$"
label_m6_last_bitblts:
    db "Last BITBLTs: ", 13, 10, "$"
label_m6_last_pixels:
    db "Last source pixels: ", 13, 10, "$"
label_m6_last_bytes:
    db "Last source bytes: ", 13, 10, "$"
label_m6_total_commands:
    db "Total command words: ", 13, 10, "$"
label_m6_total_bitblts:
    db "Total BITBLTs: ", 13, 10, "$"
label_m6_total_pixels:
    db "Total source pixels: ", 13, 10, "$"
label_m6_total_bytes:
    db "Total source bytes: ", 13, 10, "$"
label_m6_active:
    db "Active sprites: ", 13, 10, "$"
label_m6_missed:
    db "Missed VBLANK waits: ", 13, 10, "$"
message_done:
    db "Video state restored.", 13, 10, "$"
message_failed:
    db "Video or SGP initialization failed.", 13, 10, "$"
message_animation_failed:
    db "Animation synchronization failed.", 13, 10, "$"

align 2, db 0
palette_values:
    dw RGB565(0, 0, 0)
    dw RGB565(29, 9, 4)
    dw RGB565(29, 34, 4)
    dw RGB565(29, 60, 4)
    dw RGB565(17, 60, 4)
    dw RGB565(4, 60, 4)
    dw RGB565(4, 60, 17)
    dw RGB565(4, 60, 29)
    dw RGB565(4, 34, 29)
    dw RGB565(4, 9, 29)
    dw RGB565(17, 9, 29)
    dw RGB565(29, 9, 29)
    dw RGB565(29, 9, 17)
    dw RGB565(3, 6, 3)
    dw RGB565(9, 18, 9)
    dw RGB565(31, 63, 31)

align 2, db 0
sgp_command_list:
    times SGP_COMMAND_WORD_COUNT dw 0

align 2, db 0
sprite_records:
    ; x, y, vx, vy, width, height, pitch, bitmap, painter priority
    dw 24, 92, 2, 0, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_00, 0
    dw 280, 92, -2, 0, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_01, 1
    dw 11, 13, 1, 1, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_02, 2
    dw 71, 21, -1, 1, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_03, 3
    dw 131, 31, 2, 1, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_04, 4
    dw 211, 19, -2, 1, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_05, 5
    dw 285, 45, -1, 2, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_06, 6
    dw 33, 131, 2, -1, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_07, 7
    dw 97, 168, 1, -2, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_08, 8
    dw 161, 141, -1, -1, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_09, 9
    dw 231, 155, -2, -1, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_10, 10
    dw 294, 176, -1, -2, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_11, 11
    dw 45, 59, 1, 2, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_12, 12
    dw 119, 105, -2, 1, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_13, 13
    dw 193, 63, 1, -2, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_14, 14
    dw 261, 119, -1, 1, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_15, 15
%assign BULLET_INDEX BULLET_FIRST_INDEX
%rep BULLET_COUNT
    dw ((BULLET_INDEX * 17) % (SCREEN_WIDTH - BULLET_WIDTH)), ((BULLET_INDEX * 29) % (SCREEN_HEIGHT - BULLET_HEIGHT)), ((BULLET_INDEX * 3) % 5) - 2, ((BULLET_INDEX * 5) % 5) - 2, BULLET_WIDTH, BULLET_HEIGHT, BULLET_PITCH, bullet_bitmap, BULLET_INDEX
%assign BULLET_INDEX BULLET_INDEX + 1
%endrep
%assign EXTRA_SPRITE_INDEX BULLET_FIRST_INDEX + BULLET_COUNT
%rep SPRITE_MAX_COUNT - (BULLET_FIRST_INDEX + BULLET_COUNT)
    dw ((EXTRA_SPRITE_INDEX * 37) % (SCREEN_WIDTH - SPRITE_WIDTH)), ((EXTRA_SPRITE_INDEX * 53) % (SCREEN_HEIGHT - SPRITE_HEIGHT)), ((EXTRA_SPRITE_INDEX * 3) % 5) - 2, ((EXTRA_SPRITE_INDEX * 5) % 7) - 3, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_00 + ((EXTRA_SPRITE_INDEX % 16) * SPRITE_BITMAP_BYTES), EXTRA_SPRITE_INDEX
%assign EXTRA_SPRITE_INDEX EXTRA_SPRITE_INDEX + 1
%endrep


align 2, db 0
sgp_work_area:
    times 58 db 0

align 2, db 0
DEFINE_HSV_BALL hsv_ball_00, 1, 1
DEFINE_HSV_BALL hsv_ball_01, 1, 2
DEFINE_HSV_BALL hsv_ball_02, 2, 2
DEFINE_HSV_BALL hsv_ball_03, 3, 3
DEFINE_HSV_BALL hsv_ball_04, 4, 4
DEFINE_HSV_BALL hsv_ball_05, 4, 5
DEFINE_HSV_BALL hsv_ball_06, 5, 5
DEFINE_HSV_BALL hsv_ball_07, 6, 6
DEFINE_HSV_BALL hsv_ball_08, 7, 7
DEFINE_HSV_BALL hsv_ball_09, 7, 8
DEFINE_HSV_BALL hsv_ball_10, 8, 8
DEFINE_HSV_BALL hsv_ball_11, 9, 9
DEFINE_HSV_BALL hsv_ball_12, 10, 10
DEFINE_HSV_BALL hsv_ball_13, 10, 11
DEFINE_HSV_BALL hsv_ball_14, 11, 11
DEFINE_HSV_BALL hsv_ball_15, 12, 12

; Small 8x8 4-bpp bullet used by the M6 stress prefix.
align 2, db 0
bullet_bitmap:
    db BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0), BALL_PAIR(0, 0)
    db BALL_PAIR(0, 0), BALL_PAIR(0, 15), BALL_PAIR(15, 0), BALL_PAIR(0, 0)
    db BALL_PAIR(0, 15), BALL_PAIR(15, 15), BALL_PAIR(15, 15), BALL_PAIR(15, 0)
    db BALL_PAIR(15, 15), BALL_PAIR(15, 12), BALL_PAIR(12, 15), BALL_PAIR(15, 15)
    db BALL_PAIR(15, 12), BALL_PAIR(12, 12), BALL_PAIR(12, 12), BALL_PAIR(12, 15)
    db BALL_PAIR(15, 15), BALL_PAIR(15, 12), BALL_PAIR(12, 15), BALL_PAIR(15, 15)
    db BALL_PAIR(0, 15), BALL_PAIR(15, 15), BALL_PAIR(15, 15), BALL_PAIR(15, 0)
    db BALL_PAIR(0, 0), BALL_PAIR(0, 13), BALL_PAIR(13, 0), BALL_PAIR(0, 0)

align 2, db 0
fps_glyph_pointers:
    dw glyph_f, glyph_p, glyph_s, glyph_0, glyph_0, glyph_0, glyph_c, glyph_0, glyph_0, glyph_0, glyph_0
digit_glyph_table:
    dw glyph_0, glyph_1, glyph_2, glyph_3, glyph_4, glyph_5, glyph_6, glyph_7, glyph_8, glyph_9

glyph_f:
    db 0xff, 0xff, 0xf0, 0x00, 0xff, 0xf0, 0xf0, 0x00, 0xf0, 0x00, 0xf0, 0x00, 0xf0, 0x00
glyph_p:
    db 0xff, 0xf0, 0xf0, 0x0f, 0xf0, 0x0f, 0xff, 0xf0, 0xf0, 0x00, 0xf0, 0x00, 0xf0, 0x00
glyph_s:
    db 0x0f, 0xff, 0xf0, 0x00, 0xf0, 0x00, 0x0f, 0xf0, 0x00, 0x0f, 0x00, 0x0f, 0xff, 0xf0
glyph_c:
    db 0xff, 0xf0, 0xf0, 0x00, 0xf0, 0x00, 0xf0, 0x00, 0xf0, 0x00, 0xf0, 0x00, 0xff, 0xf0
glyph_0:
    db 0x0f, 0xf0, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0x0f, 0xf0
glyph_1:
    db 0x00, 0xf0, 0x0f, 0xf0, 0x00, 0xf0, 0x00, 0xf0, 0x00, 0xf0, 0x00, 0xf0, 0x0f, 0xff
glyph_2:
    db 0xff, 0xf0, 0x00, 0x0f, 0x00, 0x0f, 0x0f, 0xf0, 0xf0, 0x00, 0xf0, 0x00, 0xff, 0xff
glyph_3:
    db 0xff, 0xf0, 0x00, 0x0f, 0x00, 0x0f, 0x0f, 0xf0, 0x00, 0x0f, 0x00, 0x0f, 0xff, 0xf0
glyph_4:
    db 0xf0, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xff, 0xff, 0x00, 0x0f, 0x00, 0x0f, 0x00, 0x0f
glyph_5:
    db 0xff, 0xff, 0xf0, 0x00, 0xf0, 0x00, 0xff, 0xf0, 0x00, 0x0f, 0x00, 0x0f, 0xff, 0xf0
glyph_6:
    db 0x0f, 0xff, 0xf0, 0x00, 0xf0, 0x00, 0xff, 0xf0, 0xf0, 0x0f, 0xf0, 0x0f, 0x0f, 0xf0
glyph_7:
    db 0xff, 0xff, 0x00, 0x0f, 0x00, 0xf0, 0x00, 0xf0, 0x0f, 0x00, 0x0f, 0x00, 0x0f, 0x00
glyph_8:
    db 0x0f, 0xf0, 0xf0, 0x0f, 0xf0, 0x0f, 0x0f, 0xf0, 0xf0, 0x0f, 0xf0, 0x0f, 0x0f, 0xf0
glyph_9:
    db 0x0f, 0xf0, 0xf0, 0x0f, 0xf0, 0x0f, 0x0f, 0xff, 0x00, 0x0f, 0x00, 0x0f, 0xff, 0xf0
