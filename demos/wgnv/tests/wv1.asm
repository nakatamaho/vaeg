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
; WV1.COM (M148, WGN-V track stage V1): set a Melco WGN-V2/V4 up in the
; driver's "E bank" mode (32KB window at E0000h, reached on the PC-88VA
; through 152h bit 3) and check the S3 Trio64V+ through it. VA, VA2.
;
; Sequence of the Windows 95 driver's E-bank initialisation (M148 task,
; section 9), without its OUT 6Ah (a PC-98 graphics mode port):
;   54h pair: 02h=02h, 0Ch=00h, 40h=40h; window offset 0004h = 03h;
;   40h=00h, 10h=00h (no I/O wait), 80h=00h
;   53E1h=FFh, 53E0h=80h, 53E1h=7Fh
;   54E1h=0Dh, 54E1h|=80h, save 54E1h/54E0h, 54h pair 80h=80h,
;   53h pair 15h=15h, 53E1h=00h, 54E1h=FFh, 54E0h/54E1h restored
;   55E1h=E2h, 55E0h=E8h
;   54E1h|=01h, 54E0h|=01h, 54E1h restored; 54E0h bits 3-2 = 00b
;   window: 3C3h=01h, 3C6h=FFh, 3C2h=23h, CR38=48h, CR39=A5h, CR40|=01h,
;   CR11&=7Fh, CR35&=CFh; CR2E/CR2D must read 11h/88h (Trio64V+)
; Then CR30, CR2F, CR36, CR31, CR35, CR51, CR58-CR5A, CR6A are read,
; SR08=06h, SR11=41h written as the driver does, and VRAM is tested with
; 54E0h bits 3-2 = 10b: four bytes written at 0000h, 0001h, 4000h, 7FFFh,
; read back, the window switched to the registers (CR2D read) and back,
; and read again. Finally CR39/CR38 = 00h and the window left in VRAM
; mode as the driver leaves it. Every window access has interrupts off.
; Power the machine off and on before booting another OS.
;
; Exit: 0 Trio64V+ and VRAM; 3 no board; 10h CR2E/CR2D wrong; 11h VRAM
; did not read back.

    bits 16
    cpu 186
    org 0x100

    jmp main

%define LOG_NAME 'A:\WVLOG.TXT'
%define HDR_TAG '#WGNV,'
%include "b3e02.inc"
%include "wgnv.inc"

stage_str   db 'V1', 0
prog_str    db 'WV1', 0
opt_table   dw 0
usage_str   db 'WV1 (M148 V1): no options. WGN-V E-bank set-up, chip id, VRAM.', 13, 10, 0

cr_list     db 0x30, 0x2f, 0x36, 0x31, 0x35, 0x51, 0x58, 0x59, 0x5a, 0x6a, 0x40, 0x11
NCR         equ $ - cr_list
v_offs      dw 0x0000, 0x0001, 0x4000, 0x7fff
v_pat       db 0x5a, 0xa5, 0x3c, 0xc3

s_found     db 'FOUND', 0
s_n_eq      db 'n=', 0
s_id_eq     db ' id=', 0
s_type_eq   db ' type=', 0
s_pre       db 'PRE', 0
s_win       db 'EWIN', 0
s_init      db 'INIT', 0
s_s0        db 's54E1=', 0
s_s1        db ' s54E0=', 0
s_after     db ' 54E1=', 0
s_after0    db ' 54E0=', 0
s_id        db ' CR2E/2D=', 0
s_regs      db 'REGS', 0
s_cr        db ' CR', 0
s_vram      db 'VRAM', 0
s_pre_eq    db 'pre=', 0
s_r1_eq     db ' r1=', 0
s_cr2d_eq   db ' cr2d=', 0
s_r2_eq     db ' r2=', 0
s_sum       db 'SUM', 0
s_no_board  db 'NO-BOARD', 0
s_bad_id    db 'FAIL CR2E/CR2D is not 11h/88h: no Trio64V+ in the window', 0
s_bad_vram  db 'FAIL VRAM did not read back', 0
s_pass_all  db 'PASS Trio64V+ and VRAM through E0000h', 0
s_eq        db '=', 0

pre_vals    times 6 db 0
win_vals    times 4 db 0
sv54e1      db 0
sv54e0      db 0
aft54e1     db 0
aft54e0     db 0
cr2e        db 0
cr2d        db 0
cr_vals     times NCR db 0
v_pre       times 4 db 0
v_r1        times 4 db 0
v_r2        times 4 db 0
v_cr2d      db 0
ok_id       db 0
ok_vram     db 0

main:
    push cs
    pop ds
    push cs
    pop es
    call common_start
    call wv_find
    jnc .board
    call lb_reset
    mov si, s_sum
    call lb_tag
    mov si, s_no_board
    call lb_str
    call lb_emit
    mov al, 3
    jmp common_exit
.board:
    call lb_reset
    mov si, s_found
    call lb_tag
    mov si, s_n_eq
    call lb_str
    mov ax, [wv_n]
    call lb_hex8
    mov si, s_id_eq
    call lb_str
    mov al, [wv_id]
    call lb_hex8
    mov si, s_type_eq
    call lb_str
    mov al, [wv_type]
    call lb_hex8
    call lb_emit

    ; PRE: the six board ports as read
    call lb_reset
    mov si, s_pre
    call lb_tag
    mov si, p53e0
    mov cx, 6
.pre:
    lodsw
    mov dx, ax
    call lb_port_probe
    loop .pre
    call lb_emit

    ; the window before the set-up
    call e_open
    mov al, [es:0x0004]
    mov [win_vals], al
    mov al, [es:0x03c3]
    mov [win_vals + 1], al
    mov al, [es:0x03d4]
    mov [win_vals + 2], al
    mov al, [es:0x03d5]
    mov [win_vals + 3], al
    call e_close
    call lb_reset
    mov si, s_win
    call lb_tag
    mov si, win_vals
    mov cx, 4
.w:
    lodsb
    call lb_hex8
    mov al, ' '
    call lb_char
    loop .w
    call lb_emit

    call init
    call lb_reset
    mov si, s_init
    call lb_tag
    mov si, s_s0
    call lb_str
    mov al, [sv54e1]
    call lb_hex8
    mov si, s_s1
    call lb_str
    mov al, [sv54e0]
    call lb_hex8
    mov si, s_after
    call lb_str
    mov al, [aft54e1]
    call lb_hex8
    mov si, s_after0
    call lb_str
    mov al, [aft54e0]
    call lb_hex8
    mov si, s_id
    call lb_str
    mov al, [cr2e]
    call lb_hex8
    mov al, '/'
    call lb_char
    mov al, [cr2d]
    call lb_hex8
    call lb_emit

    cmp byte [cr2e], 0x11
    jne .regs
    cmp byte [cr2d], 0x88
    jne .regs
    mov byte [ok_id], 1
.regs:
    call read_regs
    call vram_test
    call finish

    call lb_reset
    mov si, s_sum
    call lb_tag
    mov ah, 0x10
    mov si, s_bad_id
    cmp byte [ok_id], 0
    je .verdict
    mov ah, 0x11
    mov si, s_bad_vram
    cmp byte [ok_vram], 0
    je .verdict
    xor ah, ah
    mov si, s_pass_all
.verdict:
    call lb_str
    push ax
    call lb_emit
    pop ax
    mov al, ah
    jmp common_exit

; The driver's E-bank initialisation (header).
init:
    pusha
    mov ax, 0x0202
    call breg54
    mov ax, 0x0c00
    call breg54
    mov ax, 0x4040
    call breg54
    call e_open
    mov byte [es:0x0004], 0x03
    call e_close
    mov ax, 0x4000
    call breg54
    mov ax, 0x1000
    call breg54
    mov ax, 0x8000
    call breg54
    mov dx, [p53e1]
    mov al, 0xff
    out dx, al
    mov dx, [p53e0]
    mov al, 0x80
    out dx, al
    mov dx, [p53e1]
    mov al, 0x7f
    out dx, al
    mov dx, [p54e1]
    mov al, 0x0d
    out dx, al
    in al, dx
    or al, 0x80
    out dx, al
    in al, dx
    mov [sv54e1], al
    mov dx, [p54e0]
    in al, dx
    mov [sv54e0], al
    mov ax, 0x8080
    call breg54
    mov ax, 0x1515
    call breg53
    mov dx, [p53e1]
    xor al, al
    out dx, al
    mov dx, [p54e1]
    mov al, 0xff
    out dx, al
    mov dx, [p54e0]
    mov al, [sv54e0]
    out dx, al
    mov dx, [p54e1]
    mov al, [sv54e1]
    out dx, al
    mov dx, [p55e1]
    mov al, 0xe2
    out dx, al
    mov dx, [p55e0]
    mov al, 0xe8
    out dx, al
    cli
    mov dx, [p54e1]
    in al, dx
    mov ah, al
    or al, 0x01
    out dx, al
    mov dx, [p54e0]
    in al, dx
    or al, 0x01
    out dx, al
    mov dx, [p54e1]
    mov al, ah
    out dx, al
    sti
    mov al, 0x00
    call e_mode
    mov dx, [p54e1]
    in al, dx
    mov [aft54e1], al
    mov dx, [p54e0]
    in al, dx
    mov [aft54e0], al
    ; registers through the window
    call e_open
    mov byte [es:0x03c3], 0x01
    mov byte [es:0x03c6], 0xff
    mov byte [es:0x03c2], 0x23
    mov ax, 0x3848
    call cr_write
    mov ax, 0x39a5
    call cr_write
    mov ah, 0x40
    call cr_read
    or al, 0x01
    call cr_write
    mov ah, 0x11
    call cr_read
    and al, 0x7f
    call cr_write
    mov ah, 0x35
    call cr_read
    and al, 0xcf
    call cr_write
    mov ah, 0x2e
    call cr_read
    mov [cr2e], al
    mov ah, 0x2d
    call cr_read
    mov [cr2d], al
    call e_close
    popa
    ret

; REGS line: cr_list; SR08=06h, SR11=41h as the driver.
read_regs:
    pusha
    call e_open
    mov si, cr_list
    mov di, cr_vals
    mov cx, NCR
.l:
    lodsb
    mov ah, al
    call cr_read
    mov [di], al
    inc di
    loop .l
    mov word [es:0x03c4], 0x0608
    mov word [es:0x03c4], 0x4111
    call e_close
    call lb_reset
    mov si, s_regs
    call lb_tag
    mov si, cr_list
    mov di, cr_vals
    mov cx, NCR
    xor bx, bx
.p:
    cmp bx, 6
    jne .same
    call lb_emit
    push si
    mov si, s_regs
    call lb_tag
    pop si
.same:
    push si
    mov si, s_cr
    call lb_str
    pop si
    lodsb
    call lb_hex8
    mov al, '='
    call lb_char
    mov al, [di]
    call lb_hex8
    inc di
    inc bx
    loop .p
    call lb_emit
    popa
    ret

; VRAM pattern through the window (header).
vram_test:
    pusha
    mov al, 0x08
    call e_mode
    call e_open
    mov si, v_offs
    mov di, v_pre
    call v_read
    mov si, v_offs
    mov bx, v_pat
    mov cx, 4
.w:
    push bx
    mov al, [bx]
    mov bx, [si]
    mov [es:bx], al
    pop bx
    inc bx
    add si, 2
    loop .w
    mov si, v_offs
    mov di, v_r1
    call v_read
    call e_close
    mov al, 0x00
    call e_mode
    call e_open
    mov ah, 0x2d
    call cr_read
    mov [v_cr2d], al
    call e_close
    mov al, 0x08
    call e_mode
    call e_open
    mov si, v_offs
    mov di, v_r2
    call v_read
    call e_close
    ; verdict
    mov si, v_r1
    mov di, v_pat
    call cmp4b
    jne .emit
    mov si, v_r2
    mov di, v_pat
    call cmp4b
    jne .emit
    cmp byte [v_cr2d], 0x88
    jne .emit
    mov byte [ok_vram], 1
.emit:
    call lb_reset
    mov si, s_vram
    call lb_tag
    mov si, s_pre_eq
    call lb_str
    mov si, v_pre
    call lb_4
    mov si, s_r1_eq
    call lb_str
    mov si, v_r1
    call lb_4
    mov si, s_cr2d_eq
    call lb_str
    mov al, [v_cr2d]
    call lb_hex8
    mov si, s_r2_eq
    call lb_str
    mov si, v_r2
    call lb_4
    call lb_emit
    popa
    ret

; SI = offsets, DI = four bytes (ES = window).
v_read:
    push cx
    push bx
    mov cx, 4
.l:
    mov bx, [si]
    mov al, [es:bx]
    mov [di], al
    add si, 2
    inc di
    loop .l
    pop bx
    pop cx
    ret

; ZF when the four bytes at SI and DI are equal.
cmp4b:
    push cx
    push si
    push di
    mov cx, 4
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

lb_4:
    push cx
    mov cx, 4
.l:
    lodsb
    call lb_hex8
    loop .l
    pop cx
    ret

; Lock CR38/CR39 and leave the window in VRAM mode, as the driver.
finish:
    pusha
    mov al, 0x00
    call e_mode
    call e_open
    mov ax, 0x3900
    call cr_write
    mov ax, 0x3800
    call cr_write
    call e_close
    mov al, 0x08
    call e_mode
    popa
    ret
