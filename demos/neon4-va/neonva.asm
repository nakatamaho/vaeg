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
; BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
; DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
; THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
; (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
; THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

bits 16
org 0x100

; PC-88VA interfaces used by the first port. Values are taken from the
; existing VA SGP demo and the local VA documentation; no PC-98 ports are used.
%define INT_KEYBOARD            0x82
%define INT_VIDEO               0x8f
%define VIDEO_DATA_SEG          0x0338
%define VIDEO_MODE_OFFSET       0x000f
%define VIDEO_G0_BPP_OFFSET     0x0011
%define VIDEO_G1_BPP_OFFSET     0x0012

%define PORT_MEMORY_MAP         0x0153
%define PORT_G0_TRANSPARENCY    0x0124
%define PORT_G1_TRANSPARENCY    0x0126
%define PORT_FB1_DSA_LOW        0x022e
%define PORT_FB1_DSA_HIGH       0x0230
%define PORT_TSP_STATUS         0x0142
%define PORT_SGP_COMMAND        0x0500
%define PORT_SGP_CONTROL        0x0504
%define PORT_SGP_STATUS         0x0506
%define PORT_GVRAM_WRITE_MODE   0x0580

%define MEMORY_MAP_GVRAM_SINGLE 0x54
%define GVRAM_CPU_WRITE_MODE    0x10
%define MODE_320X200_G0_G1      0xe00e
%define PIXEL_SIZE_4BPP         0x0404
%define COMPOSE_G1_OVER_G0      0x0034
%define TSP_STATUS_VBLANK       0x40

%define SCREEN_WIDTH            320
%define SCREEN_HEIGHT           200
%define SCREEN_PITCH            160
%define G0_SEGMENT              0xa000

%define G1_PAGE_A_SGP           0x220000
%define G1_PAGE_B_SGP           0x227d00
%define G1_PAGE_A_DSA           0x020000
%define G1_PAGE_B_DSA           0x027d00
%define SCREEN_WORD_COUNT       0x3e80

%define SGP_END                 0x0001
%define SGP_SET_WORK            0x0003
%define SGP_SET_DEST            0x0005
%define SGP_SET_COLOR           0x0006
%define SGP_LINE                0x0009
%define SGP_CLS                 0x000a
%define SGP_BUSY                0x01
; LINE-specific direction bits in the VAEG SGP profile.
%define SGP_LINE_VD             0x0400
%define SGP_LINE_HD             0x0800
%define SGP_LINE_ROP            0x0005

%define COMMAND_WORDS           512
%define LINE_COUNT              16

start:
    push cs
    pop ds
    cld
    call save_video_state
    call initialize_video
    jc .failed
    mov dx, message_start
    call print_string

.frame:
    call poll_keyboard
    jc .exit
    call update_scene
    call build_command_list
    call run_sgp
    jc .failed
    call wait_vblank
    jc .failed
    call set_display_page
    call swap_pages
    jmp .frame

.exit:
    call restore_video_state
    mov dx, message_done
    call print_string
    mov ax, 0x4c00
    int 0x21

.failed:
    call restore_video_state
    mov dx, message_failed
    call print_string
    mov ax, 0x4c01
    int 0x21

print_string:
    mov ah, 0x09
    int 0x21
    ret

poll_keyboard:
    mov ah, 0x0a
    int INT_KEYBOARD
    jc .none
    mov ah, 0x09
    int INT_KEYBOARD
    cmp ah, 0
    je .escape
.none:
    clc
    ret
.escape:
    stc
    ret

initialize_video:
    mov bx, MODE_320X200_G0_G1
    mov cx, PIXEL_SIZE_4BPP
    xor dx, dx
    xor ax, ax
    int INT_VIDEO
    test ax, ax
    jnz .failed
    mov byte [video_mode_changed], 1

    mov ax, 0x0b00
    int INT_VIDEO
    test ax, ax
    jnz .failed
    mov ax, 0x0900
    int INT_VIDEO
    test ax, ax
    jnz .failed
    mov ax, 0x0a00
    int INT_VIDEO
    test ax, ax
    jnz .failed
    call set_palette
    jc .failed
    mov ax, 0x0300
    mov cx, COMPOSE_G1_OVER_G0
    int INT_VIDEO
    test ax, ax
    jnz .failed

    mov dx, PORT_G0_TRANSPARENCY
    xor ax, ax
    out dx, ax
    mov dx, PORT_G1_TRANSPARENCY
    mov ax, 1
    out dx, ax
    mov dx, PORT_MEMORY_MAP
    mov al, MEMORY_MAP_GVRAM_SINGLE
    out dx, al
    mov dx, PORT_GVRAM_WRITE_MODE
    mov al, GVRAM_CPU_WRITE_MODE
    out dx, al
    call draw_g0_background

    mov byte [draw_page], 0
    call set_draw_page
    call build_command_list
    call run_sgp
    jc .failed
    mov byte [draw_page], 1
    call set_draw_page
    call build_command_list
    call run_sgp
    jc .failed
    call set_display_page
    mov byte [draw_page], 0
    call set_draw_page
    mov ax, 0x0b01
    int INT_VIDEO
    test ax, ax
    jnz .failed
    clc
    ret
.failed:
    stc
    ret

set_palette:
    push bx
    push si
    xor bx, bx
    mov si, palette
.next:
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
    inc bl
    cmp bl, 16
    jb .next
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
    int INT_VIDEO
    test ax, ax
    jnz .failed
    clc
    ret
.failed:
    stc
    ret

; The initial bring-up uses CPU writes only for Graphic 0. This is not the
; sprite renderer: all animated geometry below is emitted as SGP LINE commands.
draw_g0_background:
    mov ax, G0_SEGMENT
    mov es, ax
    xor di, di
    xor bx, bx
.row:
    mov al, 0xdd
    test bl, 0x10
    jz .tile_row
    mov al, 0xee
.tile_row:
    mov cx, 20
.tile:
    mov ah, al
    push cx
    mov cx, 4
    rep stosw
    pop cx
    xor al, 0x33
    loop .tile
    inc bl
    cmp bl, SCREEN_HEIGHT
    jb .row
    push cs
    pop es
    ret

update_scene:
    inc byte [scene_phase]
    mov al, [scene_phase]
    and ax, 0x001f
    mov bx, sine_table
    xlat
    cbw
    add ax, 160
    mov [scene_cx], ax
    mov al, [scene_phase]
    add al, 8
    and ax, 0x001f
    mov bx, sine_table
    xlat
    cbw
    add ax, 100
    mov [scene_cy], ax
    ret

build_command_list:
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push es
    push ds
    pop es
    mov di, command_list

    mov ax, SGP_SET_WORK
    stosw
    mov si, work_area
    call physical_address_from_ds_si
    stosw
    mov ax, dx
    stosw

    mov ax, SGP_SET_COLOR
    stosw
    xor ax, ax
    stosw
    mov ax, SGP_CLS
    stosw
    mov ax, [draw_page_low]
    stosw
    mov ax, [draw_page_high]
    stosw
    mov ax, SCREEN_WORD_COUNT
    stosw
    xor ax, ax
    stosw

    mov si, line_specs
    mov cx, LINE_COUNT
.line:
    call emit_line
    add si, 10
    loop .line
    mov ax, SGP_END
    stosw

    mov si, command_list
    call physical_address_from_ds_si
    mov [command_address_low], ax
    mov [command_address_high], dx
    pop es
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

; Emit one line specification: dx0, dy0, dx1, dy1, colour index.
emit_line:
    push ax
    push bx
    push cx
    push dx
    push si
    push bp
    mov ax, [si]
    add ax, [scene_cx]
    mov [line_x0], ax
    mov ax, [si + 2]
    add ax, [scene_cy]
    mov [line_y0], ax
    mov ax, [si + 4]
    add ax, [scene_cx]
    mov [line_x1], ax
    mov ax, [si + 6]
    add ax, [scene_cy]
    mov [line_y1], ax

    mov ax, SGP_SET_COLOR
    stosw
    xor bx, bx
    mov bl, [si + 8]
    and bx, 0x000f
    mov ax, bx
    shl ax, 4
    or ax, bx
    mov bx, ax
    shl ax, 4
    or ax, bx
    mov bx, ax
    shl ax, 4
    or ax, bx
    stosw

    mov ax, SGP_LINE_ROP
    mov bx, [line_x1]
    cmp bx, [line_x0]
    jge .x_forward
    or ax, SGP_LINE_HD
.x_forward:
    mov bx, [line_y1]
    cmp bx, [line_y0]
    jge .y_forward
    or ax, SGP_LINE_VD
.y_forward:
    mov [line_mode], ax
    mov ax, SGP_LINE
    stosw
    mov ax, [line_mode]
    stosw
    mov ax, [line_x0]
    and ax, 3
    shl ax, 4
    or ax, 1
    stosw
    mov ax, [line_x1]
    sub ax, [line_x0]
    jns .width_positive
    neg ax
.width_positive:
    inc ax
    stosw
    mov ax, [line_y1]
    sub ax, [line_y0]
    jns .height_positive
    neg ax
.height_positive:
    inc ax
    stosw
    mov ax, SCREEN_PITCH
    stosw

    mov ax, [line_y0]
    xor dx, dx
    mov bx, SCREEN_PITCH
    mul bx
    mov bx, [line_x0]
    and bx, 0xfffc
    shr bx, 1
    add ax, bx
    adc dx, 0
    add ax, [draw_page_low]
    adc dx, [draw_page_high]
    stosw
    mov ax, dx
    stosw
    pop bp
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

run_sgp:
    call wait_sgp_idle
    jc .failed
    mov dx, PORT_GVRAM_WRITE_MODE
    mov al, GVRAM_CPU_WRITE_MODE
    out dx, al
    mov dx, PORT_SGP_COMMAND
    mov ax, [command_address_low]
    out dx, ax
    add dx, 2
    mov ax, [command_address_high]
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

wait_vblank:
    mov dx, PORT_TSP_STATUS
    mov cx, 0xffff
.wait_low:
    in al, dx
    test al, TSP_STATUS_VBLANK
    jnz .wait_low
.wait_high:
    in al, dx
    test al, TSP_STATUS_VBLANK
    jz .wait_high
    clc
    ret

set_display_page:
    push ax
    push dx
    mov dx, PORT_FB1_DSA_LOW
    mov ax, [draw_dsa_low]
    out dx, ax
    add dx, 2
    mov ax, [draw_dsa_high]
    out dx, ax
    pop dx
    pop ax
    ret

set_draw_page:
    cmp byte [draw_page], 0
    jne .page_b
    mov word [draw_page_low], G1_PAGE_A_SGP & 0xffff
    mov word [draw_page_high], G1_PAGE_A_SGP >> 16
    mov word [draw_dsa_low], G1_PAGE_A_DSA & 0xffff
    mov word [draw_dsa_high], G1_PAGE_A_DSA >> 16
    ret
.page_b:
    mov word [draw_page_low], G1_PAGE_B_SGP & 0xffff
    mov word [draw_page_high], G1_PAGE_B_SGP >> 16
    mov word [draw_dsa_low], G1_PAGE_B_DSA & 0xffff
    mov word [draw_dsa_high], G1_PAGE_B_DSA >> 16
    ret

swap_pages:
    xor byte [draw_page], 1
    call set_draw_page
    ret

physical_address_from_ds_si:
    mov ax, ds
    xor dx, dx
    mov cx, 4
.shift:
    shl ax, 1
    rcl dx, 1
    loop .shift
    add ax, si
    adc dx, 0
    ret

save_video_state:
    mov dx, PORT_MEMORY_MAP
    in al, dx
    mov [saved_memory_map], al
    test al, 0x10
    jz .bios
    mov byte [saved_single_plane], 1
    mov dx, PORT_GVRAM_WRITE_MODE
    in al, dx
    mov [saved_write_mode], al
.bios:
    mov ax, VIDEO_DATA_SEG
    mov es, ax
    mov ax, [es:VIDEO_MODE_OFFSET]
    mov [saved_video_mode], ax
    mov al, [es:VIDEO_G0_BPP_OFFSET]
    mov [saved_g0_bpp], al
    mov al, [es:VIDEO_G1_BPP_OFFSET]
    mov [saved_g1_bpp], al
    push cs
    pop es
    ret

restore_video_state:
    push cs
    pop ds
    mov dx, PORT_MEMORY_MAP
    mov al, [saved_memory_map]
    out dx, al
    cmp byte [video_mode_changed], 0
    je .write_mode
    mov bx, [saved_video_mode]
    mov cl, [saved_g0_bpp]
    mov ch, [saved_g1_bpp]
    xor dx, dx
    xor ax, ax
    int INT_VIDEO
    mov ax, 0x0a00
    int INT_VIDEO
    mov dx, PORT_MEMORY_MAP
    mov al, [saved_memory_map]
    out dx, al
.write_mode:
    cmp byte [saved_single_plane], 0
    je .done
    mov dx, PORT_GVRAM_WRITE_MODE
    mov al, [saved_write_mode]
    out dx, al
.done:
    ret

saved_memory_map       db 0
saved_write_mode       db 0
saved_single_plane     db 0
video_mode_changed     db 0
saved_video_mode       dw 0
saved_g0_bpp            db 0
saved_g1_bpp            db 0
draw_page               db 1
scene_phase             db 0
scene_cx                dw 160
scene_cy                dw 100
line_x0                 dw 0
line_y0                 dw 0
line_x1                 dw 0
line_y1                 dw 0
line_mode               dw 0
draw_page_low           dw G1_PAGE_B_SGP & 0xffff
draw_page_high          dw G1_PAGE_B_SGP >> 16
draw_dsa_low            dw G1_PAGE_B_DSA & 0xffff
draw_dsa_high           dw G1_PAGE_B_DSA >> 16
command_address_low     dw 0
command_address_high    dw 0

message_start:
    db "NEONVA: PC-88VA SGP wireframe port", 13, 10
    db "SGP LINE scene; ESC exits.", 13, 10, "$"
message_done:
    db "Video state restored.", 13, 10, "$"
message_failed:
    db "Video or SGP initialization failed.", 13, 10, "$"

align 2, db 0
palette:
    dw 0x0000, 0x1111, 0x2222, 0x3333
    dw 0x4444, 0x5555, 0x6666, 0x7777
    dw 0x8888, 0x9999, 0xaaaa, 0xbbbb
    dw 0xcccc, 0xdddd, 0xeeee, 0xffff

; Signed endpoints for three nested rectangular solids. The order is the
; painter order used by the original demo's geometric scene.
align 2, db 0
line_specs:
    dw -76, -45,  76, -45,  1
    dw  76, -45,  76,  45,  2
    dw  76,  45, -76,  45,  3
    dw -76,  45, -76, -45,  4
    dw -52, -29,  52, -29,  5
    dw  52, -29,  52,  29,  6
    dw  52,  29, -52,  29,  7
    dw -52,  29, -52, -29,  8
    dw -29, -16,  29, -16,  9
    dw  29, -16,  29,  16, 10
    dw  29,  16, -29,  16, 11
    dw -29,  16, -29, -16, 12
    dw -76, -45, -52, -29, 13
    dw  76, -45,  52, -29, 14
    dw  76,  45,  52,  29, 15
    dw -76,  45, -52,  29,  6

; A short signed sine-like table is sufficient for a smooth, deterministic
; translation. It is not a PC-98 hardware table.
sine_table:
    db 0, 3, 6, 9, 12, 14, 15, 16
    db 16, 16, 15, 14, 12, 9, 6, 3
    db 0, -3, -6, -9, -12, -14, -15, -16
    db -16, -16, -15, -14, -12, -9, -6, -3

align 2, db 0
command_list:
    times COMMAND_WORDS dw 0
work_area:
    times 58 db 0
