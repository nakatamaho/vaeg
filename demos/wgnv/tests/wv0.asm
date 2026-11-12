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
; WV0.COM (M148, WGN-V track stage V0): read-only. Which of the Melco
; WGN-V board ports answer on a PC-88VA/VA2, and what does the E0000h
; window read before anything is set? Ports 51E1h, 59E1h, 53E0h/53E1h,
; 54E0h/54E1h, 55E0h/55E1h at every board offset n = 0..0Eh, six code
; patterns each (FF / D:xx / F:...). E0000h through 152h bit 3 with
; interrupts off (152h restored). Nothing is written to the board.
;
; Exit: 0 a board answered (51E1h+n = C3h/C4h and 59E1h+n = 04h/F4h);
; 3 none; 12h 152h not restored.

    bits 16
    cpu 186
    org 0x100

    jmp main

%define LOG_NAME 'A:\WVLOG.TXT'
%define HDR_TAG '#WGNV,'
%include "b3e02.inc"
%include "wgnv.inc"

stage_str   db 'V0', 0
prog_str    db 'WV0', 0
opt_table   dw 0
usage_str   db 'WV0 (M148 V0): no options. Read-only WGN-V port scan.', 13, 10, 0

port_base   dw 0x51e1, 0x59e1, 0x53e0, 0x53e1, 0x54e0, 0x54e1, 0x55e0, 0x55e1
NPORT       equ ($ - port_base) / 2
e_offs      dw 0x0000, 0x0004, 0x03c2, 0x03c3, 0x03cc, 0x03d4, 0x03d5, 0x7fff

s_p         db 'P', 0
s_n_eq      db 'n=', 0
s_ewin      db 'EWIN', 0
s_found     db 'FOUND', 0
s_id_eq     db ' id=', 0
s_type_eq   db ' type=', 0
s_sum       db 'SUM', 0
s_board     db 'BOARD answered', 0
s_none      db 'NO-BOARD 51E1h+n/59E1h+n do not answer', 0
s_bad_rest  db 'FAIL 152h not restored', 0
s_152       db '152h=', 0

bank_out    db 0
e_vals      times 8 db 0

main:
    push cs
    pop ds
    push cs
    pop es
    call common_start

    xor bx, bx
.n:
    call lb_reset
    mov si, s_p
    call lb_tag
    mov si, s_n_eq
    call lb_str
    mov al, bl
    call lb_hex8
    mov si, port_base
    mov cx, NPORT
.p:
    lodsw
    or ax, bx
    mov dx, ax
    push bx
    call lb_port_probe
    pop bx
    cmp cx, 5
    jne .same
    call lb_emit
    push si
    mov si, s_p
    call lb_tag
    pop si
.same:
    loop .p
    call lb_emit
    add bx, 2
    cmp bx, 0x10
    jb .n

    ; E0000h window, read only
    call e_open
    mov si, e_offs
    mov di, e_vals
    mov cx, 8
.e:
    mov bx, [si]
    mov al, [es:bx]
    mov [di], al
    add si, 2
    inc di
    loop .e
    call e_close
    mov dx, PORT_ROMBANK
    in al, dx
    mov [bank_out], al
    call lb_reset
    mov si, s_ewin
    call lb_tag
    mov si, s_152
    call lb_str
    mov al, [e_bank_in]
    call lb_hex8
    mov al, '>'
    call lb_char
    mov al, [bank_out]
    call lb_hex8
    mov si, e_offs
    mov di, e_vals
    mov cx, 8
.ep:
    mov al, ' '
    call lb_char
    lodsw
    call lb_hex16
    mov al, '='
    call lb_char
    mov al, [di]
    call lb_hex8
    inc di
    loop .ep
    call lb_emit

    call lb_reset
    mov si, s_sum
    call lb_tag
    mov ah, 0x12
    mov si, s_bad_rest
    mov al, [bank_out]
    cmp al, [e_bank_in]
    jne .verdict
    call wv_find
    jc .none
    mov si, s_found
    call lb_str
    mov al, ' '
    call lb_char
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
    mov al, ' '
    call lb_char
    xor ah, ah
    mov si, s_board
    jmp .verdict
.none:
    mov ah, 3
    mov si, s_none
.verdict:
    call lb_str
    push ax
    call lb_emit
    pop ax
    mov al, ah
    jmp common_exit
