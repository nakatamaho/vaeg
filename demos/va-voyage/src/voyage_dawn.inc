; Copyright (c) 2026 Nakata Maho
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions are met:
; 1. Redistributions of source code must retain the above copyright notice,
;    this list of conditions and the following disclaimer.
; 2. Redistributions in binary form must reproduce the above copyright notice,
;    this list of conditions and the following disclaimer in the documentation
;    and/or other materials provided with the distribution.
; THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
; WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
; MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
; IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
; SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
; WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
; OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
; OF THE POSSIBILITY OF SUCH DAMAGE.

; DI always remains the advancing SGP command-list pointer. No PUSHA here.
; Frame staging is frozen before projection; musical time remains live.
dawn_background:
    mov word [dawn_span_count], 0
    mov word [dawn_mix], 0
    test word [frame_loop], 1
    jz .sky
    mov ax, [frame_tick]
    mov bx, 8
    mul bx
    mov bx, PERIOD
    div bx
    mov [dawn_mix], ax
.sky:
    mov word [dawn_y], 0
.row:
    mov ax, [dawn_mix]
    mov bx, 5
    mul bx
    add ax, [dawn_y]
    mov bp, ax
    mov al, [ds:dawn_sky_colours+bp] ; BP defaults to caller SS, not guest DS.
    mov ah, al
    call colour
    ; One contiguous 20-row CLS band replaces twenty horizontal LINEs.
    ; 320*20 bytes = 3200 words, always within the owned 64000-byte page.
    mov ax, 0ah
    stosw
    call page_address
    mov cx, [dawn_y]
.band_offset:
    jcxz .band_address
    add ax, 6400
    adc dx, 0
    loop .band_offset
.band_address:
    stosw
    mov ax, dx
    stosw
    mov ax, 3200
    stosw
    xor ax, ax
    stosw
    inc word [dawn_y]
    call clock_poll
    call music_service
    cmp word [dawn_y], 5
    jb .row
    test word [frame_loop], 1
    jz .done
    mov ax, [bank]
    shl ax, 1
    mov bx, 235
    sub bx, ax
    mov [dawn_sun_x], bx
    mov ax, [dawn_mix]
    mov bx, 7
    mul bx
    mov bx, 110
    sub bx, ax
    add bx, [heave]
    mov [dawn_sun_y], bx
    mov word [dawn_y], 0
.sun:
    mov ax, 0fcfch
    cmp word [dawn_y], 14
    jb .sun_colour
    mov ax, 7c7ch
.sun_colour:
    call colour
    mov bx, [dawn_y]
    xor ax, ax
    mov al, [dawn_sun_widths+bx]
    mov dx, [dawn_sun_x]
    sub dx, ax
    mov [x1], dx
    mov dx, [dawn_sun_x]
    add dx, ax
    mov [x2], dx
    mov ax, [dawn_sun_y]
    add ax, [dawn_y]
    sub ax, 14
    call dawn_span
    inc word [dawn_y]
    call clock_poll
    cmp word [dawn_y], 29
    jb .sun
.done:
    ret

; Min/max silhouette envelope of the projected mesh perimeter, not per-cell
; triangles. Half-open scan rows avoid shared-vertex double counting.
dawn_surface:
    push di
    mov di, dawn_left
    mov ax, 320
    mov cx, 200
    rep stosw
    mov di, dawn_right
    mov ax, -1
    mov cx, 200
    rep stosw
    pop di
    mov word [dawn_edge], 0
.edge:
    mov bx, [dawn_edge]
    mov si, [dawn_boundary+bx]
    mov ax, [mesh+si]
    mov cx, [mesh+si+2]
    mov si, [dawn_boundary+bx+2]
    mov dx, [mesh+si]
    mov bp, [mesh+si+2]
    cmp cx, bp
    je .next_edge
    jb .ordered
    xchg ax, dx
    xchg cx, bp
.ordered:
    mov [dawn_edge_x], ax
    mov [dawn_edge_y], cx
    mov [dawn_edge_end], bp
    sub bp, cx
    mov [dawn_edge_dy], bp
    sub dx, ax
    mov ax, dx
    cwd
    idiv word [dawn_edge_dy]
    mov [dawn_step_whole], ax
    mov ax, dx
    mov cx, 256
    imul cx
    idiv word [dawn_edge_dy]
    mov [dawn_step_frac], ax
    mov word [dawn_fraction], 0
.dda:
    mov bx, [dawn_edge_y]
    shl bx, 1
    mov ax, [dawn_edge_x]
    cmp ax, 0
    jge .nonnegative
    xor ax, ax
.nonnegative:
    cmp ax, 319
    jle .clipped
    mov ax, 319
.clipped:
    cmp ax, [dawn_left+bx]
    jge .right
    mov [dawn_left+bx], ax
.right:
    cmp ax, [dawn_right+bx]
    jle .advance
    mov [dawn_right+bx], ax
.advance:
    mov ax, [dawn_step_whole]
    add [dawn_edge_x], ax
    mov ax, [dawn_step_frac]
    add ax, [dawn_fraction]
    jns .positive_fraction
    add ax, 256
    dec word [dawn_edge_x]
    jmp .fraction
.positive_fraction:
    cmp ax, 256
    jb .fraction
    sub ax, 256
    inc word [dawn_edge_x]
.fraction:
    mov [dawn_fraction], ax
    inc word [dawn_edge_y]
    mov ax, [dawn_edge_y]
    cmp ax, [dawn_edge_end]
    jb .dda
.next_edge:
    call clock_poll
    call music_service
    add word [dawn_edge], 2
    cmp word [dawn_edge], 76
    jb .edge
    mov word [dawn_y], 0
.fill:
    mov bx, [dawn_y]
    shl bx, 1
    mov ax, [dawn_left+bx]
    cmp ax, [dawn_right+bx]
    jg .next_row
    mov [x1], ax
    mov ax, [dawn_right+bx]
    mov [x2], ax
    mov bx, [dawn_y]
    shr bx, 1
    shr bx, 1
    shr bx, 1
    xor ax, ax
    mov al, [scene_phase]
    shr ax, 1
    shr ax, 1
    shr ax, 1
    add bx, ax
    and bx, 7
    mov ax, [dawn_mix]
    shl ax, 1
    shl ax, 1
    shl ax, 1
    add bx, ax
    mov al, [dawn_sea_colours+bx]
    mov ah, al
    call colour
    mov ax, [dawn_y]
    call dawn_span
    ; Music pulse warms the sun's narrow, wave-wobbled reflection.
    cmp word [dawn_mix], 2
    jb .next_row
    cmp word [dawn_y], 90
    jb .next_row
    mov ax, [dawn_y]
    sub ax, 90
    shr ax, 1
    shr ax, 1
    shr ax, 1
    add ax, 4
    mov cx, ax
    xor bx, bx
    mov bl, [scene_phase]
    add bx, [dawn_y]
    and bx, 255
    shl bx, 1
    mov ax, [sine_table+bx]
    mov cl, 5
    sar ax, cl
    ; Restore reflection half-width after using CL for the shift.
    mov cx, [dawn_y]
    sub cx, 90
    shr cx, 1
    shr cx, 1
    shr cx, 1
    add cx, 4
    add ax, [dawn_sun_x]
    mov dx, ax
    sub ax, cx
    add dx, cx
    cmp ax, [x1]
    jge .reflect_left
    mov ax, [x1]
.reflect_left:
    cmp dx, [x2]
    jle .reflect_right
    mov dx, [x2]
.reflect_right:
    cmp ax, dx
    jg .next_row
    mov [x1], ax
    mov [x2], dx
    mov ax, 7c7ch
    cmp byte [frame_pulse], 10
    jb .reflect_colour
    mov ax, 0fcfch
.reflect_colour:
    call colour
    mov ax, [dawn_y]
    call dawn_span
.next_row:
    call clock_poll
    call music_service
    inc word [dawn_y]
    cmp word [dawn_y], 200
    jb .fill
    ret

dawn_span:
    inc word [dawn_span_count]
    mov [y1], ax
    mov [y2], ax
    call line
    ret

; Bounded register-preserving publication/list/stage diagnostic snapshot.
dawn_observe:
    pusha
    mov ax, [list_end]
    sub ax, command_list
    shr ax, 1
    mov bx, [frames_drawn]
    mov cx, [frame_loop]
    mov dx, [dawn_mix]
    mov si, [frame_tick]
    mov di, [dawn_span_count]
    push ax
    xor ax, ax
    mov al, [wait_failed]
    mov bp, ax                 ; bounded-wait failure flag, not saved map bits.
    pop ax
.snapshot:
    nop
    popa
    ret

align 2, db 0
dawn_span_count: dw 0
frame_loop: dw 0
dawn_mix: dw 0
dawn_y: dw 0
dawn_sun_x: dw 235
dawn_sun_y: dw 110
dawn_edge: dw 0
dawn_edge_x: dw 0
dawn_edge_y: dw 0
dawn_edge_end: dw 0
dawn_edge_dy: dw 0
dawn_step_whole: dw 0
dawn_step_frac: dw 0
dawn_fraction: dw 0
dawn_left: times 200 dw 320
dawn_right: times 200 dw -1
%include "dawn_tables.inc"
