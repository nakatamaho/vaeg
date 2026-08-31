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
; THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
; WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
; MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
; EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
; SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
; WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
; OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
; ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
%define PORT_PALETTE_COMPOSE    0x0106
%define PORT_RGB_COMPOSE        0x0108
%define PORT_FB1_FBW            0x0224
%define PORT_FB1_DOT            0x0228
%define PORT_FB1_OFX            0x022a
%define PORT_FB1_OFY            0x022c
%define PORT_FB1_DSA_LOW        0x022e
%define PORT_FB1_DSA_HIGH       0x0230
%define PORT_FB1_DSH            0x0232
%define PORT_FB1_DSP            0x0236
%define PORT_TSP_STATUS         0x0142
%define PORT_SGP_COMMAND        0x0500
%define PORT_SGP_CONTROL        0x0504
%define PORT_SGP_STATUS         0x0506
%define PORT_GVRAM_WRITE_MODE   0x0580

%define MEMORY_MAP_GVRAM_SINGLE 0x54
%define GVRAM_CPU_WRITE_MODE    0x10
%define MODE_320X200_G0_G1      0xe00e
%define PIXEL_SIZE_G0_G1_8BPP   0x0808
%define COMPOSE_G1_OVER_G0      0x0034
%define RGB_COMPOSE_G1_OVER_G0  0x0089
%define TSP_STATUS_VBLANK       0x40

%define G0_SEGMENT              0xa000
%define PAYLOAD_SEGMENT         0x3000
%define SCREEN_WIDTH            320
%define SCREEN_HEIGHT           200
%define SCREEN_PITCH            320
%define G1_BACKING_HEIGHT       400
%define G1_PAGE_A_SGP_BASE      0x220000
%define G1_PAGE_A_DSA           0x020000
%define G1_BACKING_WORD_COUNT   0xfa00

%define MARKER_WIDTH            16
%define MARKER_HEIGHT           16
%define MARKER_PITCH            16
%define MARKER_X                152
%define MARKER_Y                92
%define MARKER_DEST_OFFSET      (MARKER_Y * SCREEN_PITCH + MARKER_X)

%define SGP_COMMAND_END         0x0001
%define SGP_COMMAND_SET_WORK    0x0003
%define SGP_COMMAND_SET_SOURCE  0x0004
%define SGP_COMMAND_SET_DEST    0x0005
%define SGP_COMMAND_SET_COLOR   0x0006
%define SGP_COMMAND_BITBLT      0x0007
%define SGP_COMMAND_CLS         0x000a
%define SGP_BITBLT_COPY_XPAR    0x0105
%define SGP_BUSY                0x01

%define IDLE_CHECKPOINT_IP      0x0800
%define IDLE_CHECKPOINT_OFFSET  (IDLE_CHECKPOINT_IP - 0x0100)

start:
    ; PC-Engine may choose a different COM load segment.  Relocate this small
    ; source-built image to the reference demo's fixed private-RAM segment so
    ; the bounded debug checkpoint has one deterministic CS:IP address.
    mov ax, cs
    cmp ax, PAYLOAD_SEGMENT
    je relocated_start
    pushf
    pop bp
    cli
    push cs
    pop ds
    mov ax, PAYLOAD_SEGMENT
    mov es, ax
    mov si, 0x0100
    mov di, 0x0100
    mov cx, program_end - $$
    cld
    rep movsb
    push bp
    popf
    push word PAYLOAD_SEGMENT
    push word relocated_start
    retf

relocated_start:
    push cs
    pop ds
    cld
    call save_video_state
    mov dx, message_start
    call print_string
    call initialize_video
    jc initialization_failed

idle_loop:
    call wait_vblank_start
    jc runtime_failed
    call poll_escape
    jc normal_exit

    ; The debug harness captures this stable register signature at 0800h.
    ; SI=0101h means one SGP list was submitted and completed.
    push cs
    pop ds
    push cs
    pop es
    mov ax, 0x984b
    mov bx, SCREEN_WIDTH
    mov cx, SCREEN_HEIGHT
    mov dx, PIXEL_SIZE_G0_G1_8BPP
    mov si, 0x0101
    mov di, MARKER_Y
    mov bp, MARKER_X
    jmp idle_checkpoint

idle_resume:
    jmp idle_loop

normal_exit:
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

runtime_failed:
    call restore_video_state
    mov dx, message_runtime_failed
    call print_string
    mov ax, 0x4c02
    int 0x21

initialize_video:
    mov bx, MODE_320X200_G0_G1
    mov cx, PIXEL_SIZE_G0_G1_8BPP
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

    mov ax, 0x0300
    mov cx, COMPOSE_G1_OVER_G0
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed

    ; The proven 8-bpp path is direct GGGRRRBB, with G1 above G0.
    xor ax, ax
    mov dx, PORT_PALETTE_COMPOSE
    out dx, ax
    mov ax, RGB_COMPOSE_G1_OVER_G0
    mov dx, PORT_RGB_COMPOSE
    out dx, ax

    mov dx, PORT_G0_TRANSPARENCY
    xor ax, ax
    out dx, ax
    mov dx, PORT_G1_TRANSPARENCY
    mov ax, 0x0001
    out dx, ax

    call configure_g1_framebuffer
    mov dx, PORT_MEMORY_MAP
    mov al, MEMORY_MAP_GVRAM_SINGLE
    out dx, al
    mov dx, PORT_GVRAM_WRITE_MODE
    mov al, GVRAM_CPU_WRITE_MODE
    out dx, al

    call draw_g0_checkerboard
    call build_marker_commands
    call run_sgp_command_list
    jc .failed
    call display_page_a

    mov ax, 0x0b01
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed
    clc
    ret
.failed:
    stc
    ret

configure_g1_framebuffer:
    push ax
    push dx
    mov dx, PORT_FB1_FBW
    mov ax, SCREEN_PITCH
    out dx, ax
    mov dx, PORT_FB1_DOT
    xor ax, ax
    out dx, ax
    mov dx, PORT_FB1_OFX
    out dx, ax
    mov dx, PORT_FB1_OFY
    out dx, ax
    mov dx, PORT_FB1_DSH
    mov ax, SCREEN_HEIGHT
    out dx, ax
    mov dx, PORT_FB1_DSP
    xor ax, ax
    out dx, ax
    pop dx
    pop ax
    ret

display_page_a:
    mov dx, PORT_FB1_DSA_LOW
    mov ax, G1_PAGE_A_DSA & 0xffff
    out dx, ax
    add dx, 2
    mov ax, G1_PAGE_A_DSA >> 16
    out dx, ax
    ret

; Write a nonzero 16x16 checkerboard to the complete 320x200 G0 page.
draw_g0_checkerboard:
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push bp
    push es
    mov ax, G0_SEGMENT
    mov es, ax
    xor di, di
    xor bx, bx
.row:
    mov dx, checker_row_a
    mov bp, checker_row_b
    test bl, 0x10
    jz .phase_ready
    xchg dx, bp
.phase_ready:
    mov cx, 20
.tile:
    mov si, dx
    test cl, 1
    jz .tile_source_ready
    mov si, bp
.tile_source_ready:
    push cx
    mov cx, 16
    rep movsb
    pop cx
    dec cx
    jnz .tile
    inc bx
    cmp bx, SCREEN_HEIGHT
    jne .row
    pop es
    pop bp
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

build_marker_commands:
    push ax
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

    ; Clear the complete 320x400 G1 backing surface in this sole submission.
    mov ax, SGP_COMMAND_CLS
    stosw
    mov ax, G1_PAGE_A_SGP_BASE & 0xffff
    stosw
    mov ax, G1_PAGE_A_SGP_BASE >> 16
    stosw
    mov ax, G1_BACKING_WORD_COUNT
    stosw
    xor ax, ax
    stosw

    mov ax, SGP_COMMAND_SET_SOURCE
    stosw
    mov ax, 2
    stosw
    mov ax, MARKER_WIDTH
    stosw
    mov ax, MARKER_HEIGHT
    stosw
    mov ax, MARKER_PITCH
    stosw
    mov si, synthetic_marker
    call physical_address_from_ds_si
    stosw
    mov ax, dx
    stosw

    mov ax, SGP_COMMAND_SET_DEST
    stosw
    mov ax, 2
    stosw
    mov ax, MARKER_WIDTH
    stosw
    mov ax, MARKER_HEIGHT
    stosw
    mov ax, SCREEN_PITCH
    stosw
    mov ax, (G1_PAGE_A_SGP_BASE + MARKER_DEST_OFFSET) & 0xffff
    stosw
    mov ax, (G1_PAGE_A_SGP_BASE + MARKER_DEST_OFFSET) >> 16
    stosw

    mov ax, SGP_COMMAND_BITBLT
    stosw
    mov ax, SGP_BITBLT_COPY_XPAR
    stosw
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
    pop ax
    ret

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
    mov bx, 4
.outer:
    mov cx, 0xffff
.poll:
    in al, dx
    test al, SGP_BUSY
    jz .ready
    loop .poll
    dec bx
    jnz .outer
    stc
    ret
.ready:
    clc
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
    mov dx, VIDEO_BIOS_DATA_SEG
    mov es, dx
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
    je .done
    mov bx, [saved_video_mode]
    mov cl, [saved_g0_bpp]
    mov ch, [saved_g1_bpp]
    xor dx, dx
    xor ax, ax
    int VIDEO_BIOS_INT
.done:
    ret

print_string:
    mov ah, 0x09
    int 0x21
    ret

message_start:
    db "M98K_INIT: 320x200 G0/G1 8-bpp synthetic-marker baseline", 13, 10
    db "One bounded SGP submission; ESC restores video and exits.", 13, 10, "$"
message_done:
    db "M98K_EXIT: video state restored.", 13, 10, "$"
message_initialization_failed:
    db "M98K_FAIL: video or SGP initialization failed.", 13, 10, "$"
message_runtime_failed:
    db "M98K_FAIL: bounded VBLANK wait timed out.", 13, 10, "$"

; Keep the capture PC stable even when surrounding helper code changes.
times IDLE_CHECKPOINT_OFFSET - ($ - $$) db 0x90
idle_checkpoint:
    jmp idle_resume

align 2, db 0
sgp_command_list:
    times 64 dw 0
sgp_work_area:
    times 29 dw 0

align 2, db 0
checker_row_a:
    times 8 db 0x24
    times 8 db 0x49
checker_row_b:
    times 8 db 0x49
    times 8 db 0x24

align 2, db 0
; Exactly 16 rows of 16 bytes: stride 16, no row padding.  00h is
; transparent; 1ch, e0h, and 03h are distinct GGGRRRBB direct colors.
synthetic_marker:
    db 0xe0,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c
    db 0x1c,0xe0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1c
    db 0x1c,0x00,0xe0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1c
    db 0x1c,0x00,0x00,0xe0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1c
    db 0x1c,0x00,0x00,0x00,0xe0,0x00,0x00,0x00,0x00,0x00,0x03,0x03,0x03,0x03,0x00,0x1c
    db 0x1c,0x00,0x00,0x00,0x00,0xe0,0x00,0x00,0x00,0x00,0x03,0x03,0x03,0x03,0x00,0x1c
    db 0x1c,0x00,0x00,0x00,0x00,0x00,0xe0,0x00,0x00,0x00,0x03,0x03,0x03,0x03,0x00,0x1c
    db 0x1c,0x00,0x00,0x00,0x00,0x00,0x00,0xe0,0x00,0x00,0x03,0x03,0x03,0x03,0x00,0x1c
    db 0x1c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xe0,0x00,0x00,0x00,0x00,0x00,0x00,0x1c
    db 0x1c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xe0,0x00,0x00,0x00,0x00,0x00,0x1c
    db 0x1c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xe0,0x00,0x00,0x00,0x00,0x1c
    db 0x1c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xe0,0x00,0x00,0x00,0x1c
    db 0x1c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xe0,0x00,0x00,0x1c
    db 0x1c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xe0,0x00,0x1c
    db 0x1c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xe0,0x1c
    db 0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0xe0
synthetic_marker_end:

%if synthetic_marker_end - synthetic_marker != MARKER_PITCH * MARKER_HEIGHT
%error "synthetic marker must be exactly 16x16 bytes"
%endif

align 2, db 0
sgp_command_address_low: dw 0
sgp_command_address_high: dw 0
saved_video_mode: dw 0
saved_memory_map: db 0
saved_g0_bpp: db 0
saved_g1_bpp: db 0
video_mode_changed: db 0
program_end:
