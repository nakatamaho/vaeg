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
; B3E5.COM (M148 stage B4b): does the PC-88VA bank RAM path reach the
; PC-9801B3-E02 VRAM window? VA only; needs B2.
;
; Technical manual 3.2.1/3.3: with RAM bank control port 1D0h = n (n > 0)
; the CPU's 80000h-9FFFFh go to the expansion slot (bank RAM, 128KB x
; 255). How the bank number reaches a board is not documented; if the VA
; drives it on the high slot address lines (slot address = n * 20000h +
; offset), the board window F60000h (reg 01h = E0h) is bank 7Bh and
; 0B0000h (reg 01h = 10h) is bank 05h, upper half.
;
; Windows used: reg 01h = 00h (baseline), E0h (F60000h) and 10h
; (0B0000h). With A20-A23 = 1 on the slot, E0h could also match CPU
; 60000h-6FFFFh, which nothing touches while interrupts are off; F00000h
; and F20000h (CPU 00000h-2FFFFh) are never selected.
;
; Each pass, interrupts off: reg 01h set, for n = 01h..FFh: OUT 1D0h,n and
; read 8000:0000, 8000:8000 and 9000:0000; then OUT 1D0h,00h and reg 01h
; restored. Banks that read anything but FFh in the baseline (another
; board) are listed and never written. For up to four banks whose E0h/10h
; reads differ from the baseline (and whose baseline is all FFh), the B4a
; two-bank pattern test is run in the half that differed.
;
; Exit: 0 a pattern test passed; 3 no board; 10h CR27; 11h no bank
; differed; 12h state not restored; 13h banks differed, no test passed.

    bits 16
    cpu 186
    org 0x100

    jmp main

%include "b3e02.inc"
%include "window.inc"

stage_str   db 'B4', 0
prog_str    db 'B3E5', 0
opt_table   dw 0
usage_str   db 'B3E5 (M148 B4b): no options. VRAM window through bank RAM 1D0h, VA only.', 13, 10, 0

PORT_RAMBANK equ 0x01d0
NPASS       equ 3

pass_reg1   db 0x00, 0xe0, 0x10
test_offs   dw 0x0000, 0x0001, 0x8000, 0xffff
pat0        db 0xa5, 0x5a, 0x3c, 0xc3
pat1        db 0x96, 0x69, 0x0f, 0xf0

s_sum       db 'SUM', 0
s_no_board  db 'NO-BOARD reg 00h is not 60h', 0
s_bad_cr27  db 'STOP CR27 is not 98h-9Bh', 0
s_base      db 'BASE', 0
s_diff      db 'DIFF', 0
s_test      db 'TEST', 0
s_reg01     db 'reg01=', 0
s_count     db ' n=', 0
s_banks     db ' banks:', 0
s_more      db ' ...', 0
s_bank_eq   db 'bank=', 0
s_seg_eq    db ' seg=', 0
s_pre       db ' pre=', 0
s_b0        db ' b0=', 0
s_b1        db ' b1=', 0
s_v_pass    db ' PASS', 0
s_v_nobank  db ' NOBANK', 0
s_v_none    db ' NONE', 0
s_v_other   db ' OTHER', 0
s_pass_any  db 'PASS VRAM through bank RAM (1D0h)', 0
s_no_diff   db 'FAIL no bank changed with the board window', 0
s_no_pass   db 'FAIL banks changed, no pattern test passed', 0
s_bad_rest  db 'FAIL not restored', 0

cur_pass    db 0
cur_reg1    db 0
n_diff      dw 0
n_tests     db 0
any_pass    db 0
any_diff    db 0
t_bank      db 0
t_seg       dw 0
dummy       db 0
v_pre       times 4 db 0
v_b0        times 4 db 0
v_b1        times 4 db 0
verdict     db 0

main:
    push cs
    pop ds
    push cs
    pop es
    call common_start
    call require_va
    call chip_start
    call chip_setup
    mov si, s_cfg
    call emit_regs

    ; the three sweeps
    xor bx, bx
.sweep:
    mov al, [pass_reg1 + bx]
    mov [cur_reg1], al
    mov [cur_pass], bl
    call sweep
    inc bx
    cmp bx, NPASS
    jb .sweep

    ; baseline: banks that are not all FFh
    mov byte [cur_pass], 0
    mov byte [cur_reg1], 0
    call list_base
    ; each window: banks that differ from the baseline, then tests
    mov bx, 1
.windows:
    mov [cur_pass], bl
    mov al, [pass_reg1 + bx]
    mov [cur_reg1], al
    push bx
    call list_diff
    call run_tests
    pop bx
    inc bx
    cmp bx, NPASS
    jb .windows

    call chip_restore
    mov si, s_rest
    call emit_regs
    call board_unwake
    call reg3_restore
    jnc .sum
    mov byte [restore_ng], 1
.sum:
    call lb_reset
    mov si, s_sum
    call lb_tag
    mov ah, 0x12
    mov si, s_bad_rest
    cmp byte [restore_ng], 0
    jne .verdict
    xor ah, ah
    mov si, s_pass_any
    cmp byte [any_pass], 0
    jne .verdict
    mov ah, 0x13
    mov si, s_no_pass
    cmp byte [any_diff], 0
    jne .verdict
    mov ah, 0x11
    mov si, s_no_diff
.verdict:
    call lb_str
    push ax
    call lb_emit
    pop ax
    mov al, ah
    jmp common_exit

; DI = &samples[cur_pass][bank][0] for BL = bank.
sample_ptr:
    push ax
    push bx
    push dx
    mov al, [cur_pass]
    xor ah, ah
    mov di, 256 * 3
    mul di                          ; AX = pass * 768
    mov di, ax
    xor bh, bh
    mov ax, bx
    add di, ax
    add di, ax
    add di, ax
    add di, samples
    pop dx
    pop bx
    pop ax
    ret

; One sweep with reg 01h = cur_reg1; fills samples[cur_pass].
sweep:
    pusha
    push es
    cli
    mov ah, 0x01
    mov al, [cur_reg1]
    call board_write
    mov bl, 1
.bank:
    push dx
    call sample_ptr
    pop dx
    mov dx, PORT_RAMBANK
    mov al, bl
    out dx, al
    mov ax, 0x8000
    mov es, ax
    mov al, [es:0x0000]
    mov [di], al
    mov al, [es:0x8000]
    mov [di + 1], al
    mov ax, 0x9000
    mov es, ax
    mov al, [es:0x0000]
    mov [di + 2], al
    inc bl
    jnz .bank
    mov dx, PORT_RAMBANK
    xor al, al
    out dx, al
    mov ah, 0x01
    mov al, [reg1_in]
    call board_write
    sti
    pop es
    popa
    ret

; CF when the three samples of bank BL in pass cur_pass are all FFh.
bank_all_ff:
    push di
    push dx
    call sample_ptr
    pop dx
    cmp byte [di], 0xff
    jne .no
    cmp byte [di + 1], 0xff
    jne .no
    cmp byte [di + 2], 0xff
    jne .no
    pop di
    stc
    ret
.no:
    pop di
    clc
    ret

; AL = bit mask (1: 8000:0000, 2: 8000:8000, 4: 9000:0000) of the samples
; of bank BL that differ between cur_pass and the baseline.
bank_diff:
    push bx
    push cx
    push si
    push di
    push dx
    mov cl, [cur_pass]
    mov byte [cur_pass], 0
    call sample_ptr
    mov si, di
    mov [cur_pass], cl
    call sample_ptr
    pop dx
    xor al, al
    mov ah, [si]
    cmp ah, [di]
    je .s1
    or al, 1
.s1:
    mov ah, [si + 1]
    cmp ah, [di + 1]
    je .s2
    or al, 2
.s2:
    mov ah, [si + 2]
    cmp ah, [di + 2]
    je .done
    or al, 4
.done:
    pop di
    pop si
    pop cx
    pop bx
    ret

; Append bank BL with its three samples of cur_pass: " bb:xxxxxx".
lb_bank:
    push ax
    push di
    push dx
    mov al, ' '
    call lb_char
    mov al, bl
    call lb_hex8
    mov al, ':'
    call lb_char
    call sample_ptr
    mov al, [di]
    call lb_hex8
    mov al, [di + 1]
    call lb_hex8
    mov al, [di + 2]
    call lb_hex8
    pop dx
    pop di
    pop ax
    ret

; Line start "<tag>,reg01=xx" and the bank lists, five banks a line.
list_start:
    push si
    call lb_reset
    call lb_tag
    mov si, s_reg01
    call lb_str
    mov al, [cur_reg1]
    call lb_hex8
    pop si
    ret

; BASE: banks of the baseline that are not all FFh.
list_base:
    pusha
    mov si, s_base
    call list_start
    mov bl, 1
    xor cx, cx
.bank:
    call bank_all_ff
    jc .next
    call list_add
.next:
    inc bl
    jnz .bank
    call list_end
    popa
    ret

; DIFF: banks whose cur_pass samples differ from the baseline.
list_diff:
    pusha
    mov si, s_diff
    call list_start
    mov bl, 1
    xor cx, cx
.bank:
    call bank_diff
    or al, al
    jz .next
    mov byte [any_diff], 1
    call list_add
.next:
    inc bl
    jnz .bank
    call list_end
    popa
    ret

; CX = count so far: add bank BL (up to 20 listed, five a line).
list_add:
    inc cx
    cmp cx, 20
    ja .done
    push cx
    dec cx
    jz .first
    mov ax, cx
    push dx
    xor dx, dx
    push bx
    mov bx, 5
    div bx
    pop bx
    pop dx
    or dx, dx
    jnz .same
    call lb_emit
    push si
    mov si, [list_tag]
    call lb_tag
    pop si
    jmp .same
.first:
    mov [list_tag], si
.same:
    pop cx
    call lb_bank
.done:
    ret

; Finish the list: " n=xx" (and " ..." past 20).
list_end:
    cmp cx, 20
    jbe .n
    push si
    mov si, s_more
    call lb_str
    pop si
.n:
    push si
    mov si, s_count
    call lb_str
    pop si
    mov ax, cx
    call lb_dec16
    call lb_emit
    ret

; Pattern tests for up to four differing banks of cur_pass whose
; baseline is all FFh.
run_tests:
    pusha
    mov byte [n_tests], 0
    mov bl, 1
.bank:
    cmp byte [n_tests], 4
    jae .done
    call bank_diff
    or al, al
    jz .next
    mov cl, [cur_pass]
    mov byte [cur_pass], 0
    call bank_all_ff
    mov [cur_pass], cl
    jnc .next
    mov word [t_seg], 0x9000
    test al, 3
    jz .seg
    mov word [t_seg], 0x8000
.seg:
    mov [t_bank], bl
    call pattern_test
    call emit_test
    inc byte [n_tests]
.next:
    inc bl
    jnz .bank
.done:
    popa
    ret

; Two-bank pattern test at t_seg with 1D0h = t_bank, reg 01h = cur_reg1.
pattern_test:
    pusha
    push es
    cli
    mov ah, 0x01
    mov al, [cur_reg1]
    call board_write
    mov dx, PORT_RAMBANK
    mov al, [t_bank]
    out dx, al
    mov es, [t_seg]
    mov al, 0x00
    call set_bank
    mov di, v_pre
    call read4
    mov al, 0x00
    call set_bank
    mov si, pat0
    call write4
    mov al, 0x10
    call set_bank
    mov si, pat1
    call write4
    mov al, [dummy]
    inc al
    mov [dummy], al
    mov al, 0x00
    call set_bank
    mov di, v_b0
    call read4
    mov al, 0x10
    call set_bank
    mov di, v_b1
    call read4
    mov al, 0x00
    call set_bank
    mov dx, PORT_RAMBANK
    xor al, al
    out dx, al
    mov ah, 0x01
    mov al, [reg1_in]
    call board_write
    sti
    pop es
    ; verdict as B3E4
    mov byte [verdict], 0
    mov si, v_b0
    mov di, pat0
    call cmp4
    jne .not_pass
    mov si, v_b1
    mov di, pat1
    call cmp4
    jne .not_pass
    mov byte [any_pass], 1
    jmp .done
.not_pass:
    mov byte [verdict], 1
    mov si, v_b0
    mov di, pat1
    call cmp4
    jne .not_nobank
    mov si, v_b1
    mov di, pat1
    call cmp4
    je .done
.not_nobank:
    mov byte [verdict], 2
    mov si, v_b0
    mov cx, 8
.ff:
    cmp byte [si], 0xff
    jne .other
    inc si
    loop .ff
    jmp .done
.other:
    mov byte [verdict], 3
.done:
    popa
    ret

; "TEST,bank=xx reg01=xx seg=xxxx pre=.. b0=.. b1=.. VERDICT".
emit_test:
    pusha
    call lb_reset
    mov si, s_test
    call lb_tag
    mov si, s_bank_eq
    call lb_str
    mov al, [t_bank]
    call lb_hex8
    mov al, ' '
    call lb_char
    mov si, s_reg01
    call lb_str
    mov al, [cur_reg1]
    call lb_hex8
    mov si, s_seg_eq
    call lb_str
    mov ax, [t_seg]
    call lb_hex16
    call lb_emit
    mov si, s_test
    call lb_tag
    mov si, s_pre
    call lb_str
    mov si, v_pre
    call lb_hex4b
    mov si, s_b0
    call lb_str
    mov si, v_b0
    call lb_hex4b
    mov si, s_b1
    call lb_str
    mov si, v_b1
    call lb_hex4b
    mov al, [verdict]
    mov si, s_v_pass
    cmp al, 0
    je .v
    mov si, s_v_nobank
    cmp al, 1
    je .v
    mov si, s_v_none
    cmp al, 2
    je .v
    mov si, s_v_other
.v:
    call lb_str
    call lb_emit
    popa
    ret

list_tag    dw 0
samples     times NPASS * 256 * 3 db 0
