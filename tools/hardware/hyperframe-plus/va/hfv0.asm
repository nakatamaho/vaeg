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
; HFV0.COM (M149): does a HyPER-FRAME+ answer on the PC-88VA at all?
; MS-DOS 4.0, V3 mode. Nothing is displayed; this is the response check.
;
; Default (read only): every D0-family port (D0, D3, D4, D5, D6, D7, D8,
; DA, DC, EE) is read six times with different code patterns
; (FF / D:xx / F:...), the M144/M148 method.
;
; /W (bounded write test, the first two writes of the JPGVHF viewer's
; cold initialisation and a two-pixel write-read-restore):
;   1. save IN EE (only if driven), OUT EE=C0, OUT D0=A5
;   2. IN D0 (readback of the control write), IN D3, IN EE
;   3. pixels (0,0) and (1,0), byte coordinates (D7 D6 D5 D4), P1 order:
;      save R/G/B (IN D8/DA/DC), write A5/5A/A5 and 3C/C3/3C, read back,
;      restore the saved values, read back the restore
;   4. stop sequence D0=00, EE=00 (the viewer's D0=00 -> EE=00)
; A port that was undriven at entry is never "restored" to FF.
;
; Exit: 0 evidence of the board (a driven port, or a readback);
; 1 option; 2 machine; 3 nothing driven and no readback; 11h the /W
; readbacks disagree; 4 log write failed.

    bits 16
    cpu 186
    org 0x100

    jmp main

%define LOG_NAME 'A:\HFLOG.TXT'
%define HDR_TAG '#HFVA,'
%include "vaprobe.inc"

stage_str   db 'V0', 0
prog_str    db 'HFV0', 0
opt_w       db 0
opt_table   dw s_o_w, opt_w
            dw 0
s_o_w       db '/W', 0
usage_str   db 'HFV0 (M149) [/W]: HyPER-FRAME+ response check. Default read-only.', 13, 10, 0

ports       dw 0x00d0, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x00d7, 0x00d8, 0x00da, 0x00dc, 0x00ee
NPORT       equ ($ - ports) / 2

s_probe     db 'PROBE', 0
s_w         db 'W-EE', 0
s_w0        db 'W-D0', 0
s_wpx       db 'W-PIXEL', 0
s_p0        db ' p0=', 0
s_p1        db ' p1=', 0
s_p0rb      db ' p0rb=', 0
s_p1rb      db ' p1rb=', 0
s_wr_eq     db ' written=', 0
s_rb_eq     db ' rb=', 0
s_in_eq     db ' in=', 0
s_sum       db 'SUM', 0
s_driven    db 'DRIVEN ports; board answers on the VA', 0
s_nothing   db 'NO-RESPONSE all ports read FF', 0
s_w_ok      db 'PASS writes reach the board', 0
s_w_no      db 'FAIL write readbacks disagree', 0
s_skip      db 'PROBE all FF; /W still run', 0
s_st        db 'stop=D0=00,EE=00', 0

sel_ports   db 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00  ; D7 D6 D5 D4 for x=0/1
n_driven    db 0
ee_in       db 0
ee_in_cls   db 0
d0_in       db 0
d0_rb       db 0
d3_rb       db 0
ee_rb       db 0
px_save     times 6 db 0
px_want     times 6 db 0
px_got      times 6 db 0
px_rb       times 6 db 0
rb_ok       db 0
pix_ok      db 0

main:
    push cs
    pop ds
    push cs
    pop es
    call common_start

    ; PROBE: two ports a line
    call lb_reset
    mov si, s_probe
    call lb_tag
    mov si, ports
    mov cx, NPORT
    xor bx, bx
    mov byte [line_open], 0
.p:
    cmp byte [line_open], 0
    jne .open
    mov [port_ptr], si
    call lb_reset
    mov si, s_probe
    call lb_tag
    mov byte [line_open], 1
    mov si, [port_ptr]
.open:
    lodsw
    mov dx, ax
    push bx
    call lb_port_probe
    pop bx
    cmp byte [probe_class], 1
    jne .notd
    inc byte [n_driven]
.notd:
    inc bx
    test bx, 1
    jz .line
    mov al, ' '
    call lb_char
    loop .p
.line:
    call lb_emit
    mov byte [line_open], 0
    loop .p
.pdone:

    cmp byte [opt_w], 0
    je .sum
    cmp byte [n_driven], 0
    je .wskip
    jmp .wrun
.wskip:
    call lb_reset
    mov si, s_probe
    call lb_tag
    mov si, s_skip
    call lb_str
    call lb_emit
.wrun:
    ; save EE if driven
    mov dx, 0x00ee
    call probe_port
    mov [ee_in_cls], al
    mov al, [probe_values]
    mov [ee_in], al

    mov dx, 0x00d0
    in al, dx
    mov [d0_in], al
    mov dx, 0x00ee
    mov al, 0xc0
    out dx, al
    mov dx, 0x00d0
    mov al, 0xa5
    out dx, al
    mov dx, 0x00d0
    in al, dx
    mov [d0_rb], al
    mov dx, 0x00d3
    in al, dx
    mov [d3_rb], al
    mov dx, 0x00ee
    in al, dx
    mov [ee_rb], al
    call lb_reset
    mov si, s_w
    call lb_tag
    mov si, s_in_eq
    call lb_str
    mov al, [ee_in]
    call lb_hex8
    mov si, s_wr_eq
    call lb_str
    mov al, 0xc0
    call lb_hex8
    mov si, s_rb_eq
    call lb_str
    mov al, [ee_rb]
    call lb_hex8
    call lb_emit
    call lb_reset
    mov si, s_w0
    call lb_tag
    mov si, s_in_eq
    call lb_str
    mov al, [d0_in]
    call lb_hex8
    mov si, s_wr_eq
    call lb_str
    mov al, 0xa5
    call lb_hex8
    mov si, s_rb_eq
    call lb_str
    mov al, [d0_rb]
    call lb_hex8
    mov si, s_rb_eq
    call lb_str
    mov al, [d3_rb]
    call lb_hex8
    call lb_emit
    cmp byte [d0_rb], 0xa5
    jne .wpixels
    mov byte [rb_ok], 1

.wpixels:
    ; pixel (0,0) then (1,0): save, write, read, restore, read
    mov byte [cur_x], 0
    call pixel_pair
    mov byte [cur_x], 1
    call pixel_pair

    ; stop sequence (the viewer's D0=00 -> EE=00)
    mov dx, 0x00d0
    xor al, al
    out dx, al
    mov dx, 0x00ee
    xor al, al
    out dx, al
    call lb_reset
    mov si, s_w
    call lb_tag
    mov si, s_st
    call lb_str
    call lb_emit

    cmp byte [pix_ok], 0
    je .sum
    mov byte [rb_ok], 1

.sum:
    call lb_reset
    mov si, s_sum
    call lb_tag
    cmp byte [opt_w], 0
    je .plain
    mov ah, 0x11
    mov si, s_w_no
    cmp byte [rb_ok], 0
    jne .v
    jmp .verdict
.plain:
    xor ah, ah
    mov si, s_driven
    cmp byte [n_driven], 0
    jne .v
    mov ah, 3
    mov si, s_nothing
    jmp .verdict
.v:
    cmp byte [rb_ok], 0
    je .verdict
    xor ah, ah
    mov si, s_w_ok
.verdict:
    call lb_str
    push ax
    call lb_emit
    pop ax
    mov al, ah
    jmp common_exit

; cur_x = 0/1: save both pixels' RGB, write patterns, read back, restore,
; read back the restore. Fills px_save/px_want/px_got/px_rb.
pixel_pair:
    pusha
    ; select (cur_x, 0), P1 byte order
    mov dx, 0x00d7
    xor al, al
    out dx, al
    mov dx, 0x00d6
    xor al, al
    out dx, al
    mov dx, 0x00d5
    xor al, al
    out dx, al
    mov dx, 0x00d4
    mov al, [cur_x]
    out dx, al
    ; save
    mov dx, 0x00d8
    in al, dx
    mov [px_save], al
    mov dx, 0x00da
    in al, dx
    mov [px_save + 1], al
    mov dx, 0x00dc
    in al, dx
    mov [px_save + 2], al
    ; write pattern
    mov al, [cur_x]
    add al, 0xa5
    mov [px_want], al
    xor al, 0x5a
    mov [px_want + 1], al
    xor al, 0x5a
    mov [px_want + 2], al
    mov dx, 0x00d8
    mov al, [px_want]
    out dx, al
    mov dx, 0x00da
    mov al, [px_want + 1]
    out dx, al
    mov dx, 0x00dc
    mov al, [px_want + 2]
    out dx, al
    ; read back
    mov dx, 0x00d8
    in al, dx
    mov [px_got], al
    mov dx, 0x00da
    in al, dx
    mov [px_got + 1], al
    mov dx, 0x00dc
    in al, dx
    mov [px_got + 2], al
    ; restore
    mov dx, 0x00d8
    mov al, [px_save]
    out dx, al
    mov dx, 0x00da
    mov al, [px_save + 1]
    out dx, al
    mov dx, 0x00dc
    mov al, [px_save + 2]
    out dx, al
    mov dx, 0x00d8
    in al, dx
    mov [px_rb], al
    mov dx, 0x00da
    in al, dx
    mov [px_rb + 1], al
    mov dx, 0x00dc
    in al, dx
    mov [px_rb + 2], al
    ; the restore must read back as saved; the write must read back as written
    mov si, px_want
    mov di, px_got
    call same3
    jne .pp
    mov si, px_save
    mov di, px_rb
    call same3
    jne .pp
    mov byte [pix_ok], 1
.pp:
    call emit_pixel
    popa
    ret

; ZF/NZ via cmp chain: si vs di, 3 bytes.
same3:
    push cx
    push si
    push di
    mov cx, 3
.l:
    mov al, [si]
    cmp al, [di]
    jne .e
    inc si
    inc di
    loop .l
    cmp cx, cx
.e:
    pop di
    pop si
    pop cx
    ret

; "W-PIXEL,x=n want=rrggbb got=rrggbb restored=rrggbb rrback=rrggbb"
emit_pixel:
    pusha
    call lb_reset
    mov si, s_wpx
    call lb_tag
    mov al, 'x'
    call lb_char
    mov al, '='
    call lb_char
    mov al, [cur_x]
    call lb_hex8
    mov si, s_p0
    call lb_str
    mov si, px_want
    call hex3
    mov si, s_p1
    call lb_str
    mov si, px_got
    call hex3
    mov si, s_p0rb
    call lb_str
    mov si, px_save
    call hex3
    mov si, s_p1rb
    call lb_str
    mov si, px_rb
    call hex3
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

cur_x       db 0
line_open   db 0
port_ptr    dw 0
