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
; IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
; WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
; DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
; INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
; (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
; SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
; HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
; STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
; IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
; POSSIBILITY OF SUCH DAMAGE.
;
; HFV1.COM (M149): show a test pattern on a Digital Arts HyPER-FRAME+
; from a PC-88VA in V3 mode (MS-DOS 4.0), through I/O only.
;
; Ports (docs/reverse_engineering.md, docs/hyper_frame_plus_spec.md):
; D0 control, D3, D4/D5 X low/high, D6/D7 Y low/high, D8/DA/DC R/G/B,
; EE. None of them is in the PC-88VA technical manual's port list; EC
; (V1/V2 kanji ROM on the VA) and the memory window are never touched.
;
; Default: read D0/D3/D8/DA/DC/EE (six code patterns), then the J1
; viewer's initialisation without its PC-98 BIOS call:
;   EE=C0, D0=A5, D3=00; Y=0191, X=0180..0187: D8=00 (8 points);
;   D0=A1; Y=0193, X=0080..009F: DA=00 (32 points); D0=E1
; then draw 640x400: eight colour bars (y 0..299: white, yellow, cyan,
; green, magenta, red, blue, black, 80 pixels each) and a grey ramp
; (y 300..399), the J1 way: Y once per row, per pixel X then R, G, B.
; Twelve pixels are read back (coordinates, then IN D8/DA/DC). The board
; is left displaying; HFV1 /OFF writes the J1 stop sequence D0=00, EE=00.
;
; Options: /OFF stop only; /NOINIT skip the initialisation; /WORD select
; coordinates with word OUTs (D6, D4) as J1 does, instead of bytes in the
; P1 order D7, D6, D5, D4.
;
; Exit: 0 every read-back pixel matches; 11h a pixel differs (the board
; may not answer, or may not support reads); 1 option.

    bits 16
    cpu 186
    org 0x100

    jmp main

%define LOG_NAME 'A:\HFLOG.TXT'
%define HDR_TAG '#HFVA,'
%include "vaprobe.inc"

stage_str   db 'H1', 0
prog_str    db 'HFV1', 0
opt_off     db 0
opt_noinit  db 0
opt_word    db 0
opt_table   dw s_o_off, opt_off
            dw s_o_noinit, opt_noinit
            dw s_o_word, opt_word
            dw 0
s_o_off     db '/OFF', 0
s_o_noinit  db '/NOINIT', 0
s_o_word    db '/WORD', 0
usage_str   db 'HFV1 (M149) [/OFF] [/NOINIT] [/WORD]: HyPER-FRAME+ test pattern.', 13, 10, 0

pre_ports   dw 0x00d0, 0x00d3, 0x00d8, 0x00da, 0x00dc, 0x00ee
NPRE        equ ($ - pre_ports) / 2

; bar colours R, G, B
bars        db 0xff, 0xff, 0xff,  0xff, 0xff, 0x00,  0x00, 0xff, 0xff
            db 0x00, 0xff, 0x00,  0xff, 0x00, 0xff,  0xff, 0x00, 0x00
            db 0x00, 0x00, 0xff,  0x00, 0x00, 0x00
; read-back points: x, y
rb_pts      dw 40, 10,  120, 10,  200, 10,  280, 10,  360, 10,  440, 10
            dw 520, 10,  600, 10,  0, 350,  320, 350,  639, 350,  639, 399
NRB         equ ($ - rb_pts) / 4

s_pre       db 'PRE', 0
s_init      db 'INIT', 0
s_init_txt  db 'EE=C0 D0=A5 D3=00 8xD8 D0=A1 32xDA D0=E1', 0
s_skip      db 'skipped (/NOINIT)', 0
s_draw      db 'DRAW', 0
s_mode_b    db 'coords=byte', 0
s_mode_w    db 'coords=word', 0
s_time      db ' time=', 0
s_cs        db 'cs', 0
s_rb        db 'RB', 0
s_off       db 'OFF', 0
s_off_txt   db 'D0=00 EE=00', 0
s_sum       db 'SUM', 0
s_pass_all  db 'PASS read-back matches; look at the HF+ monitor', 0
s_mismatch  db 'MISMATCH read-back differs; look at the HF+ monitor anyway', 0
s_off_done  db 'stopped', 0

t0          dw 0, 0
t1          dw 0, 0
n_bad       db 0
rb_exp      times 3 db 0
rb_got      times 3 db 0

main:
    push cs
    pop ds
    push cs
    pop es
    call common_start

    cmp byte [opt_off], 0
    je .run
    mov dx, 0x00d0
    xor al, al
    out dx, al
    mov dx, 0x00ee
    out dx, al
    call lb_reset
    mov si, s_off
    call lb_tag
    mov si, s_off_txt
    call lb_str
    call lb_emit
    call lb_reset
    mov si, s_sum
    call lb_tag
    mov si, s_off_done
    call lb_str
    call lb_emit
    xor al, al
    jmp common_exit

.run:
    ; PRE
    call lb_reset
    mov si, s_pre
    call lb_tag
    mov si, pre_ports
    mov cx, NPRE
.pre:
    lodsw
    mov dx, ax
    call lb_port_probe
    loop .pre
    call lb_emit

    ; INIT
    call lb_reset
    mov si, s_init
    call lb_tag
    mov si, s_skip
    cmp byte [opt_noinit], 0
    jne .init_msg
    call hf_init
    mov si, s_init_txt
.init_msg:
    call lb_str
    call lb_emit

    ; DRAW
    call get_time
    mov [t0], cx
    mov [t0 + 2], dx
    call draw
    call get_time
    mov [t1], cx
    mov [t1 + 2], dx
    call lb_reset
    mov si, s_draw
    call lb_tag
    mov si, s_mode_b
    cmp byte [opt_word], 0
    je .m
    mov si, s_mode_w
.m:
    call lb_str
    mov si, s_time
    call lb_str
    call elapsed_cs
    call lb_dec16
    mov si, s_cs
    call lb_str
    call lb_emit

    ; read-back
    mov si, rb_pts
    mov cx, NRB
.rb:
    push cx
    push si
    mov cx, [si]
    mov bx, [si + 2]
    call expected
    call hf_select
    mov dx, 0x00d8
    in al, dx
    mov [rb_got], al
    mov dx, 0x00da
    in al, dx
    mov [rb_got + 1], al
    mov dx, 0x00dc
    in al, dx
    mov [rb_got + 2], al
    call emit_rb
    pop si
    pop cx
    add si, 4
    loop .rb

    call lb_reset
    mov si, s_sum
    call lb_tag
    xor ah, ah
    mov si, s_pass_all
    cmp byte [n_bad], 0
    je .v
    mov ah, 0x11
    mov si, s_mismatch
.v:
    call lb_str
    push ax
    call lb_emit
    pop ax
    mov al, ah
    jmp common_exit

; CX = x, BX = y: select the coordinates (bytes D7 D6 D5 D4, or words).
hf_select:
    push ax
    push dx
    cmp byte [opt_word], 0
    jne .word
    mov dx, 0x00d7
    mov al, bh
    out dx, al
    dec dx
    mov al, bl
    out dx, al
    dec dx
    mov al, ch
    out dx, al
    dec dx
    mov al, cl
    out dx, al
    pop dx
    pop ax
    ret
.word:
    mov dx, 0x00d6
    mov ax, bx
    out dx, ax
    mov dx, 0x00d4
    mov ax, cx
    out dx, ax
    pop dx
    pop ax
    ret

; BX = y only.
hf_sel_y:
    push ax
    push dx
    cmp byte [opt_word], 0
    jne .word
    mov dx, 0x00d7
    mov al, bh
    out dx, al
    dec dx
    mov al, bl
    out dx, al
    pop dx
    pop ax
    ret
.word:
    mov dx, 0x00d6
    mov ax, bx
    out dx, ax
    pop dx
    pop ax
    ret

; CX = x only.
hf_sel_x:
    push ax
    push dx
    cmp byte [opt_word], 0
    jne .word
    mov dx, 0x00d5
    mov al, ch
    out dx, al
    dec dx
    mov al, cl
    out dx, al
    pop dx
    pop ax
    ret
.word:
    mov dx, 0x00d4
    mov ax, cx
    out dx, ax
    pop dx
    pop ax
    ret

; J1's initialisation without the BIOS call.
hf_init:
    pusha
    mov dx, 0x00ee
    mov al, 0xc0
    out dx, al
    mov dx, 0x00d0
    mov al, 0xa5
    out dx, al
    mov dx, 0x00d3
    xor al, al
    out dx, al
    mov bx, 0x0191
    mov cx, 0x0180
.p8:
    call hf_select
    mov dx, 0x00d8
    xor al, al
    out dx, al
    inc cx
    cmp cx, 0x0188
    jb .p8
    mov dx, 0x00d0
    mov al, 0xa1
    out dx, al
    mov bx, 0x0193
    mov cx, 0x0080
.p32:
    call hf_select
    mov dx, 0x00da
    xor al, al
    out dx, al
    inc cx
    cmp cx, 0x00a0
    jb .p32
    mov dx, 0x00d0
    mov al, 0xe1
    out dx, al
    popa
    ret

; CX = x, BX = y -> rb_exp (R, G, B).
expected:
    push ax
    push bx
    push si
    cmp bx, 300
    jae .ramp
    mov ax, cx
    push dx
    xor dx, dx
    push cx
    mov cx, 80
    div cx
    pop cx
    pop dx
    mov si, ax
    add si, si
    add si, ax                      ; bar * 3
    add si, bars
    mov al, [si]
    mov [rb_exp], al
    mov al, [si + 1]
    mov [rb_exp + 1], al
    mov al, [si + 2]
    mov [rb_exp + 2], al
    jmp .done
.ramp:
    call ramp
    mov [rb_exp], al
    mov [rb_exp + 1], al
    mov [rb_exp + 2], al
.done:
    pop si
    pop bx
    pop ax
    ret

; CX = x -> AL = x * 51 / 128 (0..254).
ramp:
    push dx
    mov ax, cx
    mov dx, 51
    mul dx
    shr ax, 7
    pop dx
    ret

; The whole 640x400 pattern.
draw:
    pusha
    xor bx, bx
.row:
    call hf_sel_y
    xor cx, cx
    cmp bx, 300
    jae .ramp_row
    mov si, bars
.bar:
    mov di, 80
.px:
    call hf_sel_x
    mov dx, 0x00d8
    mov al, [si]
    out dx, al
    mov dx, 0x00da
    mov al, [si + 1]
    out dx, al
    mov dx, 0x00dc
    mov al, [si + 2]
    out dx, al
    inc cx
    dec di
    jnz .px
    add si, 3
    cmp cx, 640
    jb .bar
    jmp .next
.ramp_row:
    call hf_sel_x
    call ramp
    mov dx, 0x00d8
    out dx, al
    mov dx, 0x00da
    out dx, al
    mov dx, 0x00dc
    out dx, al
    inc cx
    cmp cx, 640
    jb .ramp_row
.next:
    inc bx
    cmp bx, 400
    jb .row
    popa
    ret

; CX:DX = DOS time (CH hour, CL minute, DH second, DL 1/100).
get_time:
    push ax
    mov ah, 0x2c
    int 0x21
    pop ax
    ret

; AX = t1 - t0 in 1/100 s (same hour assumed).
elapsed_cs:
    push bx
    mov ax, [t0]
    call time_cs
    mov bx, ax
    mov ax, [t1]
    call time_cs
    sub ax, bx
    jnc .ok
    add ax, 360000 % 65536          ; hour boundary between t0 and t1
.ok:
    pop bx
    ret

; AX = CX:DX of DOS 2Ch (CH hour, CL minute, DH second, DL 1/100)
; -> AX = ((hour*60+minute)*60+second)*100+1/100 (mod 65536).
time_cs:
    push bx
    push cx
    push dx
    mov bl, ch                      ; hour
    xor bh, bh
    mov cl, al                      ; minute
    xor ch, ch
    mov ax, bx
    mov dl, 60
    mul dl                          ; hour*60
    add ax, cx                      ; + minute
    mov dl, 60
    mul dl                          ; *60 -> seconds
    mov cl, bh
    xor ch, ch
    add ax, cx                      ; + second
    mov dl, 100
    mul dl                          ; *100
    mov ch, dh                      ; 1/100
    xor ch, ch
    add ax, cx
    pop dx
    pop cx
    pop bx
    ret

; RB,x=xxxx y=xxxx exp=rrggbb got=rrggbb OK|DIFF
emit_rb:
    pusha
    push cx
    push bx
    call lb_reset
    mov si, s_rb
    call lb_tag
    mov al, 'x'
    call lb_char
    mov al, '='
    call lb_char
    pop bx
    pop ax
    push bx
    call lb_hex16
    mov al, ' '
    call lb_char
    mov al, 'y'
    call lb_char
    mov al, '='
    call lb_char
    pop ax
    call lb_hex16
    mov al, ' '
    call lb_char
    mov si, rb_exp
    call hex3
    mov al, '>'
    call lb_char
    mov si, rb_got
    call hex3
    mov si, rb_exp
    mov di, rb_got
    mov cx, 3
.c:
    mov al, [si]
    cmp al, [di]
    jne .diff
    inc si
    inc di
    loop .c
    mov al, ' '
    call lb_char
    mov al, 'O'
    call lb_char
    mov al, 'K'
    call lb_char
    jmp .e
.diff:
    inc byte [n_bad]
    mov al, ' '
    call lb_char
    mov al, 'D'
    call lb_char
    mov al, 'I'
    call lb_char
    mov al, 'F'
    call lb_char
    mov al, 'F'
    call lb_char
.e:
    call lb_emit
    popa
    ret

hex3:
    push cx
    mov cx, 3
.l:
    lodsb
    call lb_hex8
    loop .l
    pop cx
    ret
