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
%define VIDEO_BIOS_INT          0x8f
%define VIDEO_BIOS_DATA_SEG     0x0338
%define VIDEO_MODE_OFFSET       0x000f
%define VIDEO_G0_BPP_OFFSET     0x0011
%define VIDEO_G1_BPP_OFFSET     0x0012

%define PORT_MEMORY_MAP         0x0153
%define PORT_G0_TRANSPARENCY    0x0124
%define PORT_G1_TRANSPARENCY    0x0126
%define PORT_FB1_DSA_LOW        0x022e
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

%define SCREEN_WIDTH            320
%define SCREEN_HEIGHT           200
%define SCREEN_PITCH            160
%define G0_SEGMENT              0xa000
%define G1_PAGE_A_SGP_BASE      0x220000
%define G1_PAGE_B_SGP_BASE      0x227d00
%define G1_PAGE_A_DSA           0x020000
%define G1_PAGE_B_DSA           0x027d00
%define SCREEN_WORD_COUNT       0x3e80

%define SGP_COMMAND_END         0x0001
%define SGP_COMMAND_SET_WORK    0x0003
%define SGP_COMMAND_SET_COLOR   0x0006
%define SGP_COMMAND_LINE        0x0009
%define SGP_COMMAND_CLS         0x000a
%define SGP_LINE_COPY           0x0005
%define SGP_LINE_HD             0x0400
%define SGP_LINE_VD             0x0800
%define SGP_BUSY                0x01

%define RGB565(r,g,b)           ((((g) & 0x3f) << 10) | (((r) & 0x1f) << 5) | ((b) & 0x1f))

%define SHAPE_VERTICES          0
%define SHAPE_EDGES             2
%define SHAPE_PROJECTED         4
%define SHAPE_VERTEX_COUNT      6
%define SHAPE_EDGE_COUNT        7
%define SHAPE_CENTER_X          8
%define SHAPE_CENTER_Y          10
%define SHAPE_PHASE_Y           12
%define SHAPE_PHASE_X           13
%define SHAPE_SPEED_Y           14
%define SHAPE_SPEED_X           15
%define SHAPE_SCALE_PHASE       16
%define SHAPE_SCALE_SPEED       17
%define SHAPE_BASE_SCALE        18
%define SHAPE_SCALE_AMPLITUDE   20
%define SHAPE_BRIGHT_COLOR      22
%define SHAPE_DIM_COLOR         24
%define SHAPE_RECORD_SIZE       26

%define PROJECTED_VERTEX_SIZE   6
%define SHAPE_COUNT             4
%define COMMAND_LIST_WORDS      1024

start:
    push cs
    pop ds
    cld

    call save_video_state
    mov dx, message_start
    call print_string
    call initialize_video
    jc initialization_failed

animation_loop:
    call poll_escape
    jc animation_done

    call build_frame_commands
    call run_sgp_command_list
    jc animation_failed
    call wait_vblank_start
    jc animation_failed
    call display_draw_page
    xor byte [draw_page_index], 1
    call select_draw_page
    call advance_shape_phases
    jmp animation_loop

animation_done:
    call restore_video_state
    mov dx, message_done
    call print_string
    mov ax, 0x4c00
    int 0x21

initialization_failed:
    call restore_video_state
    mov dx, message_initialization_failed
    call print_string
    mov ax, 0x4c01
    int 0x21

animation_failed:
    call restore_video_state
    mov dx, message_animation_failed
    call print_string
    mov ax, 0x4c02
    int 0x21

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

    call initialize_palette
    jc .failed
    mov ax, 0x0300
    mov cx, COMPOSE_G1_OVER_G0
    int VIDEO_BIOS_INT
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

    call draw_background

    mov byte [draw_page_index], 0
    call select_draw_page
    call build_frame_commands
    call run_sgp_command_list
    jc .failed
    call display_draw_page
    mov byte [draw_page_index], 1
    call select_draw_page

    mov ax, 0x0b01
    int VIDEO_BIOS_INT
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
    mov si, palette_values
.next:
    mov ax, 0x0800
    mov al, bl
    mov cx, [si]
    push bx
    push si
    int VIDEO_BIOS_INT
    pop si
    pop bx
    test ax, ax
    jnz .failed
    add si, 2
    inc bx
    cmp bx, 16
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

; Draw a subdued grid once on Graphic 0. Animated geometry remains entirely
; on the transparent Graphic 1 layer.
draw_background:
    push ax
    push bx
    push cx
    push di
    push es
    mov ax, G0_SEGMENT
    mov es, ax
    xor di, di
    xor bx, bx
.row:
    mov ax, 0x0000
    test bl, 0x0f
    jnz .row_color_ready
    mov ax, 0xdddd
.row_color_ready:
    mov cx, SCREEN_PITCH / 2
.word:
    test di, 0x000f
    jnz .store
    or ax, 0x000d
.store:
    stosw
    and ax, 0xddd0
    loop .word
    inc bx
    cmp bx, SCREEN_HEIGHT
    jb .row
    pop es
    pop di
    pop cx
    pop bx
    pop ax
    ret

select_draw_page:
    cmp byte [draw_page_index], 0
    jne .page_b
    mov word [draw_page_sgp_low], G1_PAGE_A_SGP_BASE & 0xffff
    mov word [draw_page_sgp_high], G1_PAGE_A_SGP_BASE >> 16
    mov word [draw_page_dsa_low], G1_PAGE_A_DSA & 0xffff
    mov word [draw_page_dsa_high], G1_PAGE_A_DSA >> 16
    ret
.page_b:
    mov word [draw_page_sgp_low], G1_PAGE_B_SGP_BASE & 0xffff
    mov word [draw_page_sgp_high], G1_PAGE_B_SGP_BASE >> 16
    mov word [draw_page_dsa_low], G1_PAGE_B_DSA & 0xffff
    mov word [draw_page_dsa_high], G1_PAGE_B_DSA >> 16
    ret

; DSA1 uses word registers. Byte access to these ports can hang real hardware.
display_draw_page:
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

    ; CLS leaves SET COLOR at zero, so the first nonzero edge color must be
    ; emitted even when that edge uses palette index 15.
    mov word [last_line_color], 0
    mov bx, shape_records
    mov cx, SHAPE_COUNT
.shape:
    mov [current_shape], bx
    push bx
    push cx
    call project_shape
    call emit_shape_edges
    pop cx
    pop bx
    add bx, SHAPE_RECORD_SIZE
    loop .shape

    mov ax, SGP_COMMAND_END
    stosw
    mov si, sgp_command_list
    call physical_address_from_ds_si
    mov [sgp_command_address_low], ax
    mov [sgp_command_address_high], dx

    pop es
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

; Project one signed-byte vertex set with two Q7 rotations and a small
; perspective term. The pulsating scale is independent for each solid.
project_shape:
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    mov bx, [current_shape]
    mov si, [bx + SHAPE_VERTICES]
    mov di, [bx + SHAPE_PROJECTED]

    mov al, [bx + SHAPE_PHASE_Y]
    call load_sine_cosine
    mov [sin_y], dx
    mov [cos_y], ax
    mov al, [bx + SHAPE_PHASE_X]
    call load_sine_cosine
    mov [sin_x], dx
    mov [cos_x], ax

    mov al, [bx + SHAPE_SCALE_PHASE]
    and ax, 0x003f
    mov bx, ax
    mov al, [sin_table + bx]
    cbw
    mov bx, [current_shape]
    imul word [bx + SHAPE_SCALE_AMPLITUDE]
    sar ax, 7
    add ax, [bx + SHAPE_BASE_SCALE]
    mov [current_scale], ax

    xor ch, ch
    mov cl, [bx + SHAPE_VERTEX_COUNT]
.vertex:
    mov al, [si]
    cbw
    mov [vertex_x], ax
    mov al, [si + 1]
    cbw
    mov [vertex_y], ax
    mov al, [si + 2]
    cbw
    mov [vertex_z], ax
    add si, 3

    mov ax, [vertex_x]
    imul word [cos_y]
    mov dx, ax
    mov ax, [vertex_z]
    imul word [sin_y]
    add ax, dx
    sar ax, 7
    mov [rotated_x], ax

    mov ax, [vertex_z]
    imul word [cos_y]
    mov dx, ax
    mov ax, [vertex_x]
    imul word [sin_y]
    sub dx, ax
    sar dx, 7
    mov [rotated_z1], dx

    mov ax, [vertex_y]
    imul word [cos_x]
    mov dx, ax
    mov ax, [rotated_z1]
    imul word [sin_x]
    sub dx, ax
    sar dx, 7
    mov [rotated_y], dx

    mov ax, [vertex_y]
    imul word [sin_x]
    mov dx, ax
    mov ax, [rotated_z1]
    imul word [cos_x]
    add ax, dx
    sar ax, 7
    mov [rotated_z], ax

    mov dx, ax
    sar dx, 3
    add dx, [current_scale]
    mov [perspective_scale], dx

    mov ax, [rotated_x]
    imul word [perspective_scale]
    sar ax, 7
    mov bx, [current_shape]
    add ax, [bx + SHAPE_CENTER_X]
    stosw

    mov ax, [rotated_y]
    imul word [perspective_scale]
    sar ax, 7
    neg ax
    add ax, [bx + SHAPE_CENTER_Y]
    stosw
    mov ax, [rotated_z]
    stosw
    dec cx
    jz short .vertices_done
    jmp .vertex

.vertices_done:

    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

; AL is a 0..63 phase. Return cosine in AX and sine in DX, both signed Q7.
load_sine_cosine:
    push bx
    xor ah, ah
    and ax, 0x003f
    mov bx, ax
    mov al, [sin_table + bx]
    cbw
    mov dx, ax
    add bx, 16
    and bx, 0x003f
    mov al, [sin_table + bx]
    cbw
    pop bx
    ret

emit_shape_edges:
    push ax
    push bx
    push cx
    push dx
    push si
    push bp
    mov bx, [current_shape]
    mov si, [bx + SHAPE_EDGES]
    xor ch, ch
    mov cl, [bx + SHAPE_EDGE_COUNT]
.edge:
    mov al, [si]
    inc si
    call projected_vertex_address
    mov ax, [bp]
    mov [line_x1], ax
    mov ax, [bp + 2]
    mov [line_y1], ax
    mov ax, [bp + 4]
    mov [line_z1], ax

    mov al, [si]
    inc si
    call projected_vertex_address
    mov ax, [bp]
    mov [line_x2], ax
    mov ax, [bp + 2]
    mov [line_y2], ax
    mov ax, [bp + 4]
    add ax, [line_z1]

    mov bx, [current_shape]
    test ax, ax
    js .dim
    mov ax, [bx + SHAPE_BRIGHT_COLOR]
    jmp .color_ready
.dim:
    mov ax, [bx + SHAPE_DIM_COLOR]
.color_ready:
    call emit_set_color
    call emit_line
    loop .edge

    pop bp
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

; Convert the vertex index in AL to a pointer in BP.
projected_vertex_address:
    push ax
    push bx
    xor ah, ah
    mov bp, ax
    shl ax, 1
    shl bp, 1
    shl bp, 1
    add bp, ax
    mov bx, [current_shape]
    add bp, [bx + SHAPE_PROJECTED]
    pop bx
    pop ax
    ret

emit_set_color:
    cmp ax, [last_line_color]
    je .done
    mov [last_line_color], ax
    push ax
    mov ax, SGP_COMMAND_SET_COLOR
    stosw
    pop ax
    stosw
.done:
    ret

; Emit one documented SGP LINE command. Direction is encoded with
; VD=0800h and HD=0400h; width and height include both endpoints.
emit_line:
    push ax
    push bx
    push cx
    push dx
    mov ax, SGP_COMMAND_LINE
    stosw
    mov bx, SGP_LINE_COPY

    mov ax, [line_x2]
    sub ax, [line_x1]
    jns .x_positive
    neg ax
    or bx, SGP_LINE_HD
.x_positive:
    inc ax
    mov [line_width], ax

    mov ax, [line_y2]
    sub ax, [line_y1]
    jns .y_positive
    neg ax
    or bx, SGP_LINE_VD
.y_positive:
    inc ax
    mov [line_height], ax

    mov ax, bx
    stosw
    mov ax, [line_x1]
    and ax, 3
    shl ax, 4
    or ax, 1
    stosw
    mov ax, [line_width]
    stosw
    mov ax, [line_height]
    stosw
    mov ax, SCREEN_PITCH
    stosw

    mov ax, [line_y1]
    mov bx, SCREEN_PITCH
    mul bx
    mov bx, [line_x1]
    and bx, 0xfffc
    shr bx, 1
    add ax, bx
    adc dx, 0
    add ax, [draw_page_sgp_low]
    adc dx, [draw_page_sgp_high]
    stosw
    mov ax, dx
    stosw

    pop dx
    pop cx
    pop bx
    pop ax
    ret

advance_shape_phases:
    push ax
    push bx
    push cx
    mov bx, shape_records
    mov cx, SHAPE_COUNT
.next:
    mov al, [bx + SHAPE_SPEED_Y]
    add [bx + SHAPE_PHASE_Y], al
    and byte [bx + SHAPE_PHASE_Y], 0x3f
    mov al, [bx + SHAPE_SPEED_X]
    add [bx + SHAPE_PHASE_X], al
    and byte [bx + SHAPE_PHASE_X], 0x3f
    mov al, [bx + SHAPE_SCALE_SPEED]
    add [bx + SHAPE_SCALE_PHASE], al
    and byte [bx + SHAPE_SCALE_PHASE], 0x3f
    add bx, SHAPE_RECORD_SIZE
    loop .next
    pop cx
    pop bx
    pop ax
    ret

poll_escape:
    mov ah, 0x0a
    int KEYBOARD_BIOS_INT
    jc .none
    mov ah, 0x09
    int KEYBOARD_BIOS_INT
    cmp ah, 0
    je .escape
.none:
    clc
    ret
.escape:
    stc
    ret

wait_vblank_start:
    mov dx, PORT_TSP_STATUS
    mov bx, 4
.wait_display:
    mov cx, 0xffff
.display_poll:
    in al, dx
    test al, TSP_STATUS_VBLANK
    jz .display_seen
    loop .display_poll
    dec bx
    jnz .wait_display
    stc
    ret
.display_seen:
    mov bx, 4
.wait_vblank:
    mov cx, 0xffff
.vblank_poll:
    in al, dx
    test al, TSP_STATUS_VBLANK
    jnz .ready
    loop .vblank_poll
    dec bx
    jnz .wait_vblank
    stc
    ret
.ready:
    clc
    ret

; The command address ports are word ports. The control and busy latches are
; byte ports. SET WORK is always the first command in every list.
run_sgp_command_list:
    call wait_sgp_idle
    jc .failed
    mov dx, PORT_GVRAM_WRITE_MODE
    mov al, GVRAM_CPU_WRITE_MODE
    out dx, al
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
    jz .bios_state
    mov byte [saved_single_plane], 1
    mov dx, PORT_GVRAM_WRITE_MODE
    in al, dx
    mov [saved_write_mode], al
.bios_state:
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
    je .write_mode
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
.write_mode:
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

message_start:
    db "SGP wireframe: tetrahedron, cuboid, dodecahedron, icosahedron", 13, 10
    db "All animated edges are drawn by SGP LINE. ESC exits.", 13, 10, "$"
message_done:
    db "Video state restored.", 13, 10, "$"
message_initialization_failed:
    db "Video or SGP initialization failed.", 13, 10, "$"
message_animation_failed:
    db "Animation synchronization failed.", 13, 10, "$"

align 2, db 0
palette_values:
    dw RGB565(0, 0, 0)
    dw RGB565(31, 18, 4)
    dw RGB565(31, 52, 4)
    dw RGB565(20, 63, 6)
    dw RGB565(5, 48, 12)
    dw RGB565(4, 58, 31)
    dw RGB565(3, 40, 31)
    dw RGB565(5, 20, 31)
    dw RGB565(10, 8, 31)
    dw RGB565(25, 8, 31)
    dw RGB565(31, 8, 23)
    dw RGB565(31, 9, 10)
    dw RGB565(31, 34, 12)
    dw RGB565(2, 4, 8)
    dw RGB565(9, 16, 25)
    dw RGB565(31, 63, 31)

; Signed Q7 sine values for one complete turn in 64 steps.
sin_table:
    db 0, 12, 25, 37, 49, 60, 71, 81
    db 90, 98, 106, 112, 117, 122, 125, 127
    db 127, 127, 125, 122, 117, 112, 106, 98
    db 90, 81, 71, 60, 49, 37, 25, 12
    db 0, -12, -25, -37, -49, -60, -71, -81
    db -90, -98, -106, -112, -117, -122, -125, -127
    db -127, -127, -125, -122, -117, -112, -106, -98
    db -90, -81, -71, -60, -49, -37, -25, -12

align 2, db 0
shape_records:
    dw tetrahedron_vertices, tetrahedron_edges, tetrahedron_projected
    db 4, 6
    dw 80, 50
    db 0, 7, 1, 2, 0, 1
    dw 82, 14, 0xeeee, 0x4444

    dw cuboid_vertices, cuboid_edges, cuboid_projected
    db 8, 12
    dw 240, 50
    db 11, 0, 2, 1, 16, 2
    dw 82, 12, 0x7777, 0x5555

    dw dodecahedron_vertices, dodecahedron_edges, dodecahedron_projected
    db 20, 30
    dw 80, 146
    db 23, 41, 1, 1, 32, 1
    dw 88, 12, 0xaaaa, 0x8888

    dw icosahedron_vertices, icosahedron_edges, icosahedron_projected
    db 12, 30
    dw 240, 146
    db 37, 19, 2, 3, 48, 2
    dw 86, 13, 0x3333, 0x2222

tetrahedron_vertices:
    db 48, 48, 48, -48, -48, 48, -48, 48, -48, 48, -48, -48
tetrahedron_edges:
    db 0,1, 0,2, 0,3, 1,2, 1,3, 2,3

cuboid_vertices:
    db -50,-34,-40, -50,-34,40, -50,34,-40, -50,34,40
    db 50,-34,-40, 50,-34,40, 50,34,-40, 50,34,40
cuboid_edges:
    db 0,1, 0,2, 0,4, 1,3, 1,5, 2,3, 2,6
    db 3,7, 4,5, 4,6, 5,7, 6,7

dodecahedron_vertices:
    db -28,-28,-28, -28,-28,28, -28,28,-28, -28,28,28
    db 28,-28,-28, 28,-28,28, 28,28,-28, 28,28,28
    db 0,-17,-45, 0,-17,45, 0,17,-45, 0,17,45
    db -17,-45,0, -17,45,0, 17,-45,0, 17,45,0
    db -45,0,-17, -45,0,17, 45,0,-17, 45,0,17
dodecahedron_edges:
    db 0,8, 0,12, 0,16, 1,9, 1,12, 1,17
    db 2,10, 2,13, 2,16, 3,11, 3,13, 3,17
    db 4,8, 4,14, 4,18, 5,9, 5,14, 5,19
    db 6,10, 6,15, 6,18, 7,11, 7,15, 7,19
    db 8,10, 9,11, 12,14, 13,15, 16,17, 18,19

icosahedron_vertices:
    db 0,-31,-50, 0,-31,50, 0,31,-50, 0,31,50
    db -31,-50,0, -31,50,0, 31,-50,0, 31,50,0
    db -50,0,-31, -50,0,31, 50,0,-31, 50,0,31
icosahedron_edges:
    db 0,2, 0,4, 0,6, 0,8, 0,10, 1,3, 1,4, 1,6, 1,9, 1,11
    db 2,5, 2,7, 2,8, 2,10, 3,5, 3,7, 3,9, 3,11, 4,6, 4,8
    db 4,9, 5,7, 5,8, 5,9, 6,10, 6,11, 7,10, 7,11, 8,9, 10,11

align 2, db 0
tetrahedron_projected: times 4 * PROJECTED_VERTEX_SIZE db 0
cuboid_projected: times 8 * PROJECTED_VERTEX_SIZE db 0
dodecahedron_projected: times 20 * PROJECTED_VERTEX_SIZE db 0
icosahedron_projected: times 12 * PROJECTED_VERTEX_SIZE db 0

align 2, db 0
sgp_command_list: times COMMAND_LIST_WORDS dw 0
sgp_work_area: times 58 db 0

saved_memory_map: db 0
saved_write_mode: db 0
saved_single_plane: db 0
video_mode_changed: db 0
saved_video_mode: dw 0
saved_g0_bpp: db 0
saved_g1_bpp: db 0
draw_page_index: db 0
draw_page_sgp_low: dw 0
draw_page_sgp_high: dw 0
draw_page_dsa_low: dw 0
draw_page_dsa_high: dw 0
sgp_command_address_low: dw 0
sgp_command_address_high: dw 0
current_shape: dw 0
last_line_color: dw 0
sin_x: dw 0
cos_x: dw 0
sin_y: dw 0
cos_y: dw 0
current_scale: dw 0
perspective_scale: dw 0
vertex_x: dw 0
vertex_y: dw 0
vertex_z: dw 0
rotated_x: dw 0
rotated_y: dw 0
rotated_z1: dw 0
rotated_z: dw 0
line_x1: dw 0
line_y1: dw 0
line_z1: dw 0
line_x2: dw 0
line_y2: dw 0
line_width: dw 0
line_height: dw 0
