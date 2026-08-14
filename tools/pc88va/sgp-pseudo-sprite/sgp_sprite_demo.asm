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
%define PORT_FB1_DSA_MIDDLE     0x022f
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

%define SPRITE_INITIAL_COUNT    16
%define SPRITE_MIN_COUNT        1
%define SPRITE_MAX_COUNT        1024
%define SPRITE_WIDTH            24
%define SPRITE_HEIGHT           24
%define SPRITE_PITCH            12
%define SPRITE_BITMAP_BYTES     (SPRITE_PITCH * SPRITE_HEIGHT)
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

start:
    push cs
    pop ds
    cld

    call save_video_state

    mov dx, message_start
    call print_string

    call initialize_video
    jc initialization_failed
    call initialize_fps_counter

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
    jmp animation_loop

animation_done:
    call restore_video_state
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

set_display_page_from_draw:
    push ax
    push dx
    mov dx, PORT_FB1_DSA_LOW
    mov ax, [draw_page_dsa_low]
    out dx, al
    inc dx
    mov al, ah
    out dx, al
    inc dx
    mov ax, [draw_page_dsa_high]
    out dx, al
    pop dx
    pop ax
    ret

flip_draw_page:
    call wait_vblank_start
    jc .failed
    call set_display_page_from_draw
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
    mov byte [draw_page_index], 0
    call set_draw_page_base
    call render_sprite_frame
    jc .failed

    mov byte [draw_page_index], 1
    call set_draw_page_base
    call render_sprite_frame
    jc .failed

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
    mov bp, [active_sprite_count]

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

    add si, SPRITE_RECORD_SIZE
    dec bp
    jnz .emit_sprite

    call emit_fps_glyph_commands
    mov ax, SGP_COMMAND_END
    stosw

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
    mov bp, FPS_GLYPH_COUNT

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

    add si, 2
    add bx, FPS_GLYPH_ADVANCE
    dec bp
    jnz .next_glyph

    pop bp
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

run_sgp_command_list:
    call wait_sgp_idle
    jc .failed

    mov dx, PORT_SGP_COMMAND
    mov ax, [sgp_command_address_low]
    out dx, al
    inc dx
    mov al, ah
    out dx, al
    inc dx
    mov ax, [sgp_command_address_high]
    out dx, al
    inc dx
    mov al, ah
    out dx, al

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


message_start:
    db "SGP pseudo-sprite demo: M5 rendered balls and count controls", 13, 10
    db "UP/+ adds a ball, DOWN/- removes one, ESC exits.", 13, 10, "$"
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
    dw 6, 40, 2, 1, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_00, 16
    dw 58, 176, -1, -2, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_01, 17
    dw 102, 8, 1, 2, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_02, 18
    dw 150, 176, 2, -1, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_03, 19
    dw 202, 8, -1, 2, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_04, 20
    dw 250, 176, -2, -1, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_05, 21
    dw 300, 8, -1, 1, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_06, 22
    dw 6, 176, 1, -1, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_07, 23
    dw 55, 108, 2, 2, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_08, 24
    dw 85, 75, -2, 1, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_09, 25
    dw 145, 115, 1, -2, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_10, 26
    dw 175, 48, -1, 2, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_11, 27
    dw 225, 98, 2, -1, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_12, 28
    dw 275, 145, -2, -2, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_13, 29
    dw 303, 60, -1, 2, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_14, 30
    dw 15, 155, 2, -1, SPRITE_WIDTH, SPRITE_HEIGHT, SPRITE_PITCH, hsv_ball_15, 31
%assign EXTRA_SPRITE_INDEX 32
%rep SPRITE_MAX_COUNT - 32
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
