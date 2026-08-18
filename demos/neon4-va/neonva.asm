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

; VA BIOS and I/O interfaces are shared with the verified VA SGP guest tests.
; No PC-98 GRCG, EGC, PEGC, PIC, or INT 0Ah interface is used here.
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
%define G0_PAGE_SGP             0x200000
%define G1_PAGE_A_SGP           0x220000
%define G1_PAGE_B_SGP           0x227d00
%define G1_PAGE_A_DSA           0x020000
%define G1_PAGE_B_DSA           0x027d00
%define SCREEN_WORD_COUNT       0x3e80

%define SGP_END                 0x0001
%define SGP_SET_WORK            0x0003
%define SGP_SET_COLOR           0x0006
%define SGP_LINE                0x0009
%define SGP_CLS                 0x000a
%define SGP_BUSY                0x01
; LINE-specific direction bits in the VAEG SGP profile.
%define SGP_LINE_VD             0x0400
%define SGP_LINE_HD             0x0800
%define SGP_LINE_ROP            0x0005

%define SCENE_COUNT             8
%define SCENE_LENGTH            384
%define COMMAND_WORDS           1024

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
    call update_scene_time
    call build_frame_commands
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

; The original NEON4 main loop uses DOS's nonblocking console function. This
; keeps the VA port independent of a guessed keyboard BIOS subfunction.
poll_keyboard:
    mov ah, 0x06
    mov dl, 0xff
    int 0x21
    jz .none
    cmp al, 0x1b
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
    call initialize_palette
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
    call clear_g0_black

    mov byte [draw_page], 0
    call set_draw_page
    call build_frame_commands
    call run_sgp
    jc .failed
    mov byte [draw_page], 1
    call set_draw_page
    call build_frame_commands
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

initialize_palette:
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
    int INT_VIDEO
    test ax, ax
    pop si
    pop bx
    jnz .failed
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

clear_g0_black:
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
    mov ax, G0_PAGE_SGP & 0xffff
    stosw
    mov ax, G0_PAGE_SGP >> 16
    stosw
    mov ax, SCREEN_WORD_COUNT
    stosw
    xor ax, ax
    stosw
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
    call run_sgp
    ret

update_scene_time:
    inc word [global_frame]
    mov ax, [global_frame]
    xor dx, dx
    mov bx, SCENE_LENGTH
    div bx
    cmp ax, SCENE_COUNT - 1
    jbe .scene_ready
    mov ax, SCENE_COUNT - 1
    mov dx, SCENE_LENGTH - 1
.scene_ready:
    mov [scene_index], al
    mov [scene_frame], dx
    mov al, dl
    and al, 0x1f
    mov bx, sine_table
    xlat
    cbw
    add ax, 160
    mov [scene_cx], ax
    mov al, dl
    add al, 8
    and al, 0x1f
    mov bx, sine_table
    xlat
    cbw
    add ax, 100
    mov [scene_cy], ax
    ret

build_frame_commands:
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

    xor ax, ax
    mov al, [scene_index]
    shl ax, 1
    mov bx, ax
    mov si, scene_routines
    add si, bx
    call word [si]

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

; Emit a LINE descriptor. Inputs are absolute 320x200 coordinates in AX/BX/CX/DX
; and a 4-bpp palette index in SI. The descriptor follows the LINE mode word.
emit_line:
    mov [line_x0], ax
    mov [line_y0], bx
    mov [line_x1], cx
    mov [line_y1], dx
    mov [line_color], si
    mov ax, SGP_SET_COLOR
    stosw
    mov ax, [line_color]
    and ax, 0x000f
    mov bx, ax
    shl ax, 4
    or ax, bx
    mov bx, ax
    shl ax, 4
    or ax, bx
    mov bx, ax
    shl ax, 4
    or ax, bx
    stosw

    mov ax, SGP_LINE
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
    ret

; Emit a rectangle from center and half extents. The four edges are the
; reduced-resolution equivalent of a NEON4 carrier/card or wire cage.
emit_box:
    push ax
    push bx
    push cx
    push dx
    push si
    mov ax, [scene_cx]
    sub ax, [shape_half_w]
    mov bx, [scene_cy]
    sub bx, [shape_half_h]
    mov cx, [scene_cx]
    add cx, [shape_half_w]
    mov dx, bx
    mov si, [shape_color]
    call emit_line
    mov ax, cx
    mov bx, [scene_cy]
    sub bx, [shape_half_h]
    mov cx, [scene_cx]
    add cx, [shape_half_w]
    mov dx, [scene_cy]
    add dx, [shape_half_h]
    call emit_line
    mov ax, [scene_cx]
    add ax, [shape_half_w]
    mov bx, [scene_cy]
    add bx, [shape_half_h]
    mov cx, [scene_cx]
    sub cx, [shape_half_w]
    mov dx, bx
    call emit_line
    mov ax, [scene_cx]
    sub ax, [shape_half_w]
    mov bx, [scene_cy]
    add bx, [shape_half_h]
    mov cx, ax
    mov dx, [scene_cy]
    sub dx, [shape_half_h]
    call emit_line
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

; Draw a six-edge tetrahedral seed with a central top point.
emit_tetra:
    push ax
    push bx
    push cx
    push dx
    push si
    mov ax, [scene_cx]
    sub ax, [shape_half_w]
    mov bx, [scene_cy]
    add bx, [shape_half_h]
    mov cx, [scene_cx]
    add cx, [shape_half_w]
    mov dx, bx
    mov si, [shape_color]
    call emit_line
    mov ax, [scene_cx]
    add ax, [shape_half_w]
    mov bx, [scene_cy]
    add bx, [shape_half_h]
    mov cx, [scene_cx]
    mov dx, [scene_cy]
    sub dx, [shape_half_h]
    call emit_line
    mov ax, [scene_cx]
    mov bx, [scene_cy]
    sub bx, [shape_half_h]
    mov cx, [scene_cx]
    sub cx, [shape_half_w]
    mov dx, [scene_cy]
    add dx, [shape_half_h]
    call emit_line
    mov ax, [scene_cx]
    sub ax, [shape_half_w]
    mov bx, [scene_cy]
    add bx, [shape_half_h]
    mov cx, [scene_cx]
    mov dx, [scene_cy]
    sub dx, [shape_half_h]
    call emit_line
    mov ax, [scene_cx]
    add ax, [shape_half_w]
    mov bx, [scene_cy]
    add bx, [shape_half_h]
    mov cx, [scene_cx]
    mov dx, [scene_cy]
    sub dx, [shape_half_h]
    call emit_line
    mov ax, [scene_cx]
    sub ax, [shape_half_w]
    mov bx, [scene_cy]
    add bx, [shape_half_h]
    mov cx, [scene_cx]
    add cx, [shape_half_w]
    mov dx, [scene_cy]
    sub dx, [shape_half_h]
    call emit_line
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

; Scene 0: SIGNAL SEED. The original carrier panel is kept as a wireframe
; element, followed by the central tetrahedral seed.
scene_signal_seed:
    push ax
    push bx
    push cx
    push dx
    push si
    push bp
    mov ax, [scene_frame]
    shr ax, 2
    add ax, 65
    mov [scene_cx], ax
    mov word [scene_cy], 35
    mov word [shape_half_w], 34
    mov word [shape_half_h], 10
    mov word [shape_color], 2
    call emit_box
    mov al, [scene_frame]
    and al, 0x1f
    mov bx, sine_table
    xlat
    cbw
    add ax, 160
    mov [scene_cx], ax
    mov al, [scene_frame]
    add al, 8
    and al, 0x1f
    mov bx, sine_table
    xlat
    cbw
    add ax, 100
    mov [scene_cy], ax
    mov word [shape_half_w], 28
    mov word [shape_half_h], 25
    mov word [shape_color], 14
    call emit_tetra
    pop bp
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

; Scene 1: FACET ASSEMBLY. Far and near cage edges surround a changing solid.
scene_facet_assembly:
    push ax
    push bx
    push cx
    push dx
    push si
    push bp
    mov word [scene_cx], 160
    mov word [scene_cy], 100
    mov al, [scene_frame]
    and al, 0x1f
    mov bx, sine_table
    xlat
    cbw
    test ax, ax
    jns .positive
    neg ax
.positive:
    add ax, 28
    mov [shape_half_w], ax
    mov word [shape_half_h], 25
    mov word [shape_color], 2
    call emit_box
    add word [shape_half_w], 8
    add word [shape_half_h], 6
    mov word [shape_color], 1
    call emit_box
    sub word [shape_half_w], 8
    sub word [shape_half_h], 6
    mov word [shape_color], 13
    call emit_tetra
    pop bp
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

; Scene 2: MATERIAL ASSEMBLY. Three independent solids gather around the
; signal, matching the authored satellite composition without filled faces.
scene_material_assembly:
    push ax
    push bx
    push cx
    push dx
    push si
    push bp
    mov word [scene_cy], 100
    mov word [shape_half_w], 18
    mov word [shape_half_h], 15
    mov word [shape_color], 4
    mov ax, 88
    mov [scene_cx], ax
    call emit_box
    mov word [shape_color], 9
    mov ax, 160
    mov [scene_cx], ax
    call emit_tetra
    mov word [shape_color], 12
    mov ax, 232
    mov [scene_cx], ax
    call emit_box
    pop bp
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

; Scene 3: MORPH GATE. Chapter phase changes between tetra, box, and octagonal
; silhouettes. The palette step is the same coarse hue progression as NEON4.
scene_morph_gate:
    push ax
    push bx
    push cx
    push dx
    push si
    push bp
    mov word [scene_cx], 160
    mov word [scene_cy], 100
    mov word [shape_half_w], 46
    mov word [shape_half_h], 35
    mov ax, [scene_frame]
    shr ax, 6
    and al, 3
    cmp al, 0
    jne .not_tetra
    mov word [shape_color], 5
    call emit_tetra
    jmp .done
.not_tetra:
    cmp al, 1
    jne .not_box
    mov word [shape_color], 6
    call emit_box
    jmp .done
.not_box:
    mov word [shape_color], 10
    call emit_octa
.done:
    pop bp
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

; Scene 4: RASTER TRANSFER. The source used two moving raster carrier panels;
; this wireframe form preserves their relative motion and crossing diagonals.
scene_raster_transfer:
    push ax
    push bx
    push cx
    push dx
    push si
    push bp
    mov ax, [scene_frame]
    shr ax, 2
    add ax, 35
    mov [scene_cx], ax
    mov word [scene_cy], 55
    mov word [shape_half_w], 30
    mov word [shape_half_h], 12
    mov word [shape_color], 3
    call emit_box
    mov ax, [scene_frame]
    shr ax, 2
    neg ax
    add ax, 285
    mov [scene_cx], ax
    mov word [scene_cy], 145
    mov word [shape_color], 11
    call emit_box
    mov word [scene_cx], 160
    mov word [scene_cy], 100
    mov word [shape_half_w], 45
    mov word [shape_half_h], 35
    mov word [shape_color], 15
    call emit_tetra
    pop bp
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

; Scene 5: SURFACE WAVE. Six short connected ribbon sections provide the
; original FM-string visualizer's horizontal wave without CPU raster pixels.
scene_surface_wave:
    push ax
    push bx
    push cx
    push dx
    push si
    push bp
    xor bp, bp
.segment:
    mov ax, bp
    shl ax, 5
    add al, [scene_frame]
    and al, 0x1f
    mov bx, sine_table
    xlat
    cbw
    shl ax, 1
    add ax, 105
    mov [ribbon_y0], ax
    mov ax, bp
    mov bx, 38
    mul bx
    add ax, 35
    mov [ribbon_x0], ax
    mov cx, ax
    add cx, 37
    mov [ribbon_x1], cx
    mov ax, [ribbon_y0]
    mov bx, bp
    and bx, 3
    sub ax, bx
    mov [ribbon_y1], ax
    mov ax, [ribbon_x0]
    mov bx, [ribbon_y0]
    mov cx, [ribbon_x1]
    mov dx, [ribbon_y1]
    mov si, bp
    and si, 7
    add si, 5
    call emit_line
    inc bp
    cmp bp, 7
    jb .segment
    pop bp
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

; Scene 6: GRID ARRIVAL. This is the original perspective-floor chapter, not a
; permanent background pattern. It is emitted only during this scene.
scene_grid_arrival:
    push ax
    push bx
    push cx
    push dx
    push si
    push bp
    mov word [scene_cy], 78
    mov word [shape_color], 6
    xor bp, bp
.vertical:
    mov ax, bp
    sub ax, 3
    mov bx, 32
    imul bx
    add ax, 160
    mov [grid_x0], ax
    mov ax, bp
    sub ax, 3
    mov bx, 92
    imul bx
    add ax, 160
    mov [grid_x1], ax
    mov ax, [grid_x0]
    mov bx, [scene_cy]
    mov cx, [grid_x1]
    mov dx, 190
    mov si, [shape_color]
    call emit_line
    inc bp
    cmp bp, 7
    jb .vertical
    mov bp, 0
.horizontal:
    mov ax, bp
    mov cx, 16
    mul cx
    add ax, 86
    mov bx, ax
    mov ax, 40
    mov dx, bx
    mov cx, 304
    mov si, 7
    call emit_line
    inc bp
    cmp bp, 7
    jb .horizontal
    pop bp
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

; Scene 7: SOLID FINALE. The final 64 frames intentionally expose the cleared
; black page, matching the source shutter instead of leaving stale geometry.
scene_solid_finale:
    push ax
    push bx
    push cx
    push dx
    push si
    push bp
    cmp word [scene_frame], 320
    jae .black
    mov word [scene_cx], 160
    mov word [scene_cy], 100
    mov word [shape_half_w], 35
    mov word [shape_half_h], 28
    mov word [shape_color], 14
    call emit_box
    xor bp, bp
.spoke:
    mov ax, bp
    shl ax, 2
    add al, [scene_frame]
    and al, 0x1f
    mov bx, sine_table
    xlat
    cbw
    shl ax, 2
    add ax, 160
    mov [corona_x], ax
    mov ax, bp
    shl ax, 2
    add al, [scene_frame]
    add al, 8
    and al, 0x1f
    mov bx, sine_table
    xlat
    cbw
    shl ax, 2
    add ax, 100
    mov [corona_y], ax
    mov ax, 160
    mov bx, 100
    mov cx, [corona_x]
    mov dx, [corona_y]
    mov si, bp
    and si, 7
    add si, 8
    call emit_line
    inc bp
    cmp bp, 8
    jb .spoke
.black:
    pop bp
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

; Six-edge octahedron silhouette used by MORPH GATE.
emit_octa:
    push ax
    push bx
    push cx
    push dx
    push si
    mov si, [shape_color]
    mov ax, [scene_cx]
    mov bx, [scene_cy]
    sub bx, [shape_half_h]
    mov cx, [scene_cx]
    sub cx, [shape_half_w]
    mov dx, [scene_cy]
    call emit_line
    mov ax, [scene_cx]
    sub ax, [shape_half_w]
    mov bx, [scene_cy]
    mov cx, [scene_cx]
    mov dx, [scene_cy]
    add dx, [shape_half_h]
    call emit_line
    mov ax, [scene_cx]
    mov bx, [scene_cy]
    add bx, [shape_half_h]
    mov cx, [scene_cx]
    add cx, [shape_half_w]
    mov dx, [scene_cy]
    call emit_line
    mov ax, [scene_cx]
    add ax, [shape_half_w]
    mov bx, [scene_cy]
    mov cx, [scene_cx]
    mov dx, [scene_cy]
    sub dx, [shape_half_h]
    call emit_line
    mov ax, [scene_cx]
    sub ax, [shape_half_w]
    mov bx, [scene_cy]
    mov cx, [scene_cx]
    add cx, [shape_half_w]
    mov dx, [scene_cy]
    call emit_line
    mov ax, [scene_cx]
    mov bx, [scene_cy]
    sub bx, [shape_half_h]
    mov cx, [scene_cx]
    add cx, [shape_half_w]
    mov dx, [scene_cy]
    call emit_line
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

scene_routines:
    dw scene_signal_seed, scene_facet_assembly
    dw scene_material_assembly, scene_morph_gate
    dw scene_raster_transfer, scene_surface_wave
    dw scene_grid_arrival, scene_solid_finale

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
global_frame            dw 0
scene_index             db 0
scene_frame             dw 0
scene_cx                dw 160
scene_cy                dw 100
shape_half_w            dw 24
shape_half_h            dw 20
shape_color             dw 15
line_x0                 dw 0
line_y0                 dw 0
line_x1                 dw 0
line_y1                 dw 0
line_color              dw 0
line_mode               dw 0
draw_page_low           dw G1_PAGE_B_SGP & 0xffff
draw_page_high          dw G1_PAGE_B_SGP >> 16
draw_dsa_low            dw G1_PAGE_B_DSA & 0xffff
draw_dsa_high           dw G1_PAGE_B_DSA >> 16
command_address_low     dw 0
command_address_high    dw 0
ribbon_x0               dw 0
ribbon_x1               dw 0
ribbon_y0               dw 0
ribbon_y1               dw 0
grid_x0                 dw 0
grid_x1                 dw 0
corona_x                dw 0
corona_y                dw 0

message_start:
    db "NEONVA: NEON4 PC-88VA faithful wireframe port", 13, 10
    db "Eight scenes; ESC exits.", 13, 10, "$"
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
