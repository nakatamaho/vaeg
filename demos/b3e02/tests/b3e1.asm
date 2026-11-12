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
; B3E1.COM (M148 stage B1): does the PC-9801B3-E02 board control
; (0FA2h index / 0FA3h data) answer on a PC-88VA? VA only.
;
; Writes only the index 0FA2h (00h, 01h, 03h, 04h), each read back, and
; reads 0FA3h with six code patterns. The incoming index (when 0FA2h is
; driven and reads 00h-07h) is restored, otherwise 00h is left.
;
;   PRE  0FA2h and 0FA3h before anything is written
;   R00  ID: 60h = PC-9801-96 / PC-9801B3-E02 (chip, bus, version decoded)
;   R01  VRAM window (80h F20000h, A0h F00000h, C0h F40000h, E0h F60000h)
;   R03  bit1 relay, bit0 memory-mapped I/O
;   R04  DIP switches (bit4 15MB mode, bits 3-2 / 1-0 window)
;
; Exit: 0 ID 60h and every index read back; 3 no board (reg 00h at 0FA3h
; is not driven); 10h another ID; 11h an index did not read back;
; 12h the index was not restored.

    bits 16
    cpu 186
    org 0x100

    jmp main

%include "b3e02.inc"

stage_str   db 'B1', 0
prog_str    db 'B3E1', 0
opt_table   dw 0
usage_str   db 'B3E1 (M148 B1): no options. Board control 0FA2h/0FA3h, VA only.', 13, 10, 0

regs        db 0x00, 0x01, 0x03, 0x04
REG_COUNT   equ $ - regs

s_pre       db 'PRE', 0
s_reg       db 'REG', 0
s_idx_eq    db ' idx', 0
s_data_eq   db ' data', 0
s_dec       db 'DEC', 0
s_id        db 'id=', 0
s_chip      db ' chip(76,3)=', 0
s_bus       db ' bus(54)=', 0
s_ver       db ' ver(210)=', 0
s_win       db ' win=', 0
s_dip15     db ' dip15MB=', 0
s_dipw16    db ' win16(32)=', 0
s_dipw15    db ' win15(10)=', 0
s_relay     db ' relay=', 0
s_mmio      db ' mmio=', 0
s_rest      db 'RESTORE', 0
s_in_eq     db 'idx-in=', 0
s_out_eq    db ' written=', 0
s_rb_eq     db ' readback=', 0
s_sum       db 'SUM', 0
s_pass_id   db 'PASS ID 60h, index read back', 0
s_no_board  db 'NO-BOARD 0FA3h (reg 00h) not driven', 0
s_bad_id    db 'FAIL ID is not 60h', 0
s_bad_idx   db 'FAIL index readback', 0
s_bad_rest  db 'FAIL index not restored', 0
s_win_f20   db 'F20000', 0
s_win_f00   db 'F00000', 0
s_win_f40   db 'F40000', 0
s_win_f60   db 'F60000', 0
s_win_0b0   db '0B0000', 0
s_win_unk   db 'other', 0

cur_reg     db 0
idx_in      db 0
idx_in_cls  db 0
idx_restore db 0
idx_rb      db 0
data0_cls   db 0
idx_bad     db 0
reg_vals    times 8 db 0xff

main:
    push cs
    pop ds
    push cs
    pop es
    call common_start
    call require_va

    ; before anything is written
    call lb_reset
    mov si, s_pre
    call lb_tag
    mov dx, B3_IDX
    call lb_port_probe
    mov [idx_in], al
    mov al, [probe_class]
    mov [idx_in_cls], al
    mov dx, B3_DATA
    call lb_port_probe
    call lb_emit

    ; each register: index write, index readback, data probe
    mov si, regs
    mov cx, REG_COUNT
.reg:
    lodsb
    mov [cur_reg], al
    call lb_reset
    push si
    mov si, s_reg
    call lb_tag
    pop si
    mov al, [cur_reg]
    call lb_hex8
    mov dx, B3_IDX
    out dx, al
    push si
    mov si, s_idx_eq
    call lb_str
    pop si
    call lb_port_probe
    cmp al, [cur_reg]
    jne .idx_bad
    cmp byte [probe_class], 1
    je .idx_ok
.idx_bad:
    mov byte [idx_bad], 1
.idx_ok:
    push si
    mov si, s_data_eq
    call lb_str
    pop si
    mov dx, B3_DATA
    call lb_port_probe
    xor bh, bh
    mov bl, [cur_reg]
    mov [reg_vals + bx], al
    or bl, bl
    jnz .not0
    mov ah, [probe_class]
    mov [data0_cls], ah
.not0:
    call lb_emit
    loop .reg

    ; restore the incoming index
    mov al, 0x00
    cmp byte [idx_in_cls], 1
    jne .write_idx
    cmp byte [idx_in], 0x07
    ja .write_idx
    mov al, [idx_in]
.write_idx:
    mov [idx_restore], al
    mov dx, B3_IDX
    out dx, al
    in al, dx
    mov [idx_rb], al
    call lb_reset
    mov si, s_rest
    call lb_tag
    mov si, s_in_eq
    call lb_str
    mov al, [idx_in]
    call lb_hex8
    mov si, s_out_eq
    call lb_str
    mov al, [idx_restore]
    call lb_hex8
    mov si, s_rb_eq
    call lb_str
    mov al, [idx_rb]
    call lb_hex8
    call lb_emit

    call decode

    ; verdict
    call lb_reset
    mov si, s_sum
    call lb_tag
    mov ah, 0x03
    mov si, s_no_board
    cmp byte [data0_cls], 1
    jne .verdict
    mov ah, 0x10
    mov si, s_bad_id
    cmp byte [reg_vals], 0x60
    jne .verdict
    mov ah, 0x11
    mov si, s_bad_idx
    cmp byte [idx_bad], 0
    jne .verdict
    mov ah, 0x12
    mov si, s_bad_rest
    mov al, [idx_rb]
    cmp al, [idx_restore]
    jne .verdict
    xor ah, ah
    mov si, s_pass_id
.verdict:
    call lb_str
    push ax
    call lb_emit
    pop ax
    mov al, ah
    jmp common_exit

; DEC line: the fields of R00, R01, R03 and R04.
decode:
    pusha
    call lb_reset
    mov si, s_dec
    call lb_tag
    mov si, s_id
    call lb_str
    mov bl, [reg_vals + 0]
    mov al, bl
    call lb_hex8
    mov si, s_chip
    call lb_str
    mov al, bl
    and al, 0xc8
    call lb_hex8
    mov si, s_bus
    call lb_str
    mov al, bl
    shr al, 4
    and al, 0x03
    call lb_hex8
    mov si, s_ver
    call lb_str
    mov al, bl
    and al, 0x07
    call lb_hex8
    ; R01 window
    mov si, s_win
    call lb_str
    mov al, [reg_vals + 1]
    call lb_hex8
    mov al, '/'
    call lb_char
    mov al, [reg_vals + 1]
    mov si, s_win_f20
    cmp al, 0x80
    je .w
    mov si, s_win_f00
    cmp al, 0xa0
    je .w
    mov si, s_win_f40
    cmp al, 0xc0
    je .w
    mov si, s_win_f60
    cmp al, 0xe0
    je .w
    mov si, s_win_0b0
    cmp al, 0x10
    je .w
    mov si, s_win_unk
.w:
    call lb_str
    ; R03
    mov bl, [reg_vals + 3]
    mov si, s_relay
    call lb_str
    mov al, bl
    shr al, 1
    and al, 1
    call lb_hex8
    mov si, s_mmio
    call lb_str
    mov al, bl
    and al, 1
    call lb_hex8
    call lb_emit
    ; R04 DIP
    call lb_reset
    mov si, s_dec
    call lb_tag
    mov bl, [reg_vals + 4]
    mov si, s_dip15
    call lb_str
    mov al, bl
    shr al, 4
    and al, 1
    call lb_hex8
    mov si, s_dipw16
    call lb_str
    mov al, bl
    shr al, 2
    and al, 3
    call lb_hex8
    mov si, s_dipw15
    call lb_str
    mov al, bl
    and al, 3
    call lb_hex8
    mov al, ' '
    call lb_char
    mov al, 'R'
    call lb_char
    mov al, '4'
    call lb_char
    mov al, '='
    call lb_char
    mov al, bl
    call lb_hex8
    call lb_emit
    popa
    ret
