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
; B3E4.COM (M148 stage B4a): can the PC-88VA CPU reach the CL-GD5428
; VRAM of a PC-9801B3-E02, without a tester for slot pins A20-A23? VA
; only; needs B2 (it sets board reg 03h bit 0 itself, never the relay).
;
; Only windows that cannot overlap main RAM whatever A20-A23 carry:
;   W1  board reg 01h = 10h (window 0B0000h), seen at VA B0000h with
;       153h SMBC = 0. If A20-A23 are 1 on the slot, the board sees
;       FBxxxxh and nothing matches. Reg 01h is 10h only inside the test.
;   W2  board reg 01h unchanged (00h), chip VGA window GR06 = 05h
;       (A0000h, 64KB), seen at VA A0000h with SMBC = 0.
;
; Chip set-up for the test (all incoming values saved and restored):
; SR06=12h, Misc |= 02h (RAM enable), SR00=03h, SR02=0Fh, SR04=0Eh,
; SR07=01h (packed 8 bpp), GR01=00h, GR03=00h, GR04=00h, GR05=40h,
; GR06=05h, GR08=FFh, GR0B=00h (one bank, 4KB steps), GR09=GR0A=00h.
;
; Each window, interrupts off: SMBC=0, read the four test offsets
; (pre), write pattern 0 with GR09=00h and pattern 1 with GR09=10h
; (64KB further), touch a main-RAM variable, read both banks back,
; GR09=00h, restore SMBC (and reg 01h). Verdict: PASS (both banks read
; back), NOBANK (both read pattern 1: RAM, GR09 not effective), NONE
; (all FF), OTHER.
;
; Exit: 0 a window passed; 3 no board; 10h CR27 not a CL-GD5428;
; 11h no window passed; 12h state not restored.

    bits 16
    cpu 186
    org 0x100

    jmp main

%include "b3e02.inc"
%include "window.inc"

stage_str   db 'B4', 0
prog_str    db 'B3E4', 0
opt_table   dw 0
usage_str   db 'B3E4 (M148 B4a): no options. VRAM windows 0B0000h and A0000h, VA only.', 13, 10, 0

test_offs   dw 0x0000, 0x0001, 0x8000, 0xffff
pat0        db 0xa5, 0x5a, 0x3c, 0xc3
pat1        db 0x96, 0x69, 0x0f, 0xf0

s_win       db 'WIN', 0
s_w1        db 'B0000 reg01=10', 0
s_w2        db 'A0000 GR06=05', 0
s_pre       db ' pre=', 0
s_b0        db ' b0=', 0
s_b1        db ' b1=', 0
s_v_pass    db ' PASS', 0
s_v_nobank  db ' NOBANK', 0
s_v_none    db ' NONE', 0
s_v_other   db ' OTHER', 0
s_sum       db 'SUM', 0
s_no_board  db 'NO-BOARD reg 00h is not 60h', 0
s_bad_cr27  db 'STOP CR27 is not 98h-9Bh', 0
s_pass_w1   db 'PASS VRAM through 0B0000h (A20-A23 = 0 on the slot)', 0
s_pass_w2   db 'PASS VRAM through the VGA window A0000h', 0
s_none      db 'FAIL no window reached the VRAM', 0
s_bad_rest  db 'FAIL not restored', 0

smbc_in     db 0
use_reg1    db 0
dummy       db 0
v_pre       times 4 db 0
v_b0        times 4 db 0
v_b1        times 4 db 0
verdict     db 0
verdict_w1  db 0
verdict_w2  db 0

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

    mov byte [use_reg1], 1
    mov ax, 0xb000
    call window_test
    mov al, [verdict]
    mov [verdict_w1], al
    mov si, s_w1
    call emit_window

    mov byte [use_reg1], 0
    mov ax, 0xa000
    call window_test
    mov al, [verdict]
    mov [verdict_w2], al
    mov si, s_w2
    call emit_window

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
    mov si, s_pass_w1
    cmp byte [verdict_w1], 0
    je .verdict
    mov si, s_pass_w2
    cmp byte [verdict_w2], 0
    je .verdict
    mov ah, 0x11
    mov si, s_none
.verdict:
    call lb_str
    push ax
    call lb_emit
    pop ax
    mov al, ah
    jmp common_exit

; AX = window segment. Fills v_pre, v_b0, v_b1 and verdict.
window_test:
    pusha
    mov es, ax
    cli
    mov dx, PORT_SMBC
    in al, dx
    mov [smbc_in], al
    and al, 0xf0
    out dx, al
    cmp byte [use_reg1], 0
    je .open
    mov ah, 0x01
    mov al, 0x10
    call board_write
.open:
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
    ; something else on the bus between the writes and the reads
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
    cmp byte [use_reg1], 0
    je .close
    mov ah, 0x01
    mov al, [reg1_in]
    call board_write
.close:
    mov dx, PORT_SMBC
    mov al, [smbc_in]
    out dx, al
    sti
    push ds
    pop es
    ; verdict
    mov byte [verdict], 0
    mov si, v_b0
    mov di, pat0
    call cmp4
    jne .not_pass
    mov si, v_b1
    mov di, pat1
    call cmp4
    je .done
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

; SI = label: "WIN,<label> pre=.. b0=.. b1=.. VERDICT".
emit_window:
    pusha
    push si
    call lb_reset
    mov si, s_win
    call lb_tag
    pop si
    call lb_str
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
