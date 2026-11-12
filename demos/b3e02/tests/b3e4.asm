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

stage_str   db 'B4', 0
prog_str    db 'B3E4', 0
opt_table   dw 0
usage_str   db 'B3E4 (M148 B4a): no options. VRAM windows 0B0000h and A0000h, VA only.', 13, 10, 0

; chip set-up: index port, index, value
setup_tab   dw VGA_SEQ_I
            db 0x00, 0x03
            dw VGA_SEQ_I
            db 0x02, 0x0f
            dw VGA_SEQ_I
            db 0x04, 0x0e
            dw VGA_SEQ_I
            db 0x07, 0x01
            dw VGA_GC_I
            db 0x01, 0x00
            dw VGA_GC_I
            db 0x03, 0x00
            dw VGA_GC_I
            db 0x04, 0x00
            dw VGA_GC_I
            db 0x05, 0x40
            dw VGA_GC_I
            db 0x06, 0x05
            dw VGA_GC_I
            db 0x08, 0xff
            dw VGA_GC_I
            db 0x0b, 0x00
            dw VGA_GC_I
            db 0x09, 0x00
            dw VGA_GC_I
            db 0x0a, 0x00
SETUP_COUNT equ ($ - setup_tab) / 4

test_offs   dw 0x0000, 0x0001, 0x8000, 0xffff
pat0        db 0xa5, 0x5a, 0x3c, 0xc3
pat1        db 0x96, 0x69, 0x0f, 0xf0

s_cfg       db 'CFG', 0
s_rest      db 'RESTORE', 0
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
s_misc_eq   db 'misc=', 0
s_reg1_eq   db ' reg01=', 0
s_sr_eq     db ' SR', 0
s_gr_eq     db ' GR', 0
s_sum       db 'SUM', 0
s_no_board  db 'NO-BOARD reg 00h is not 60h', 0
s_bad_cr27  db 'STOP CR27 is not 98h-9Bh', 0
s_pass_w1   db 'PASS VRAM through 0B0000h (A20-A23 = 0 on the slot)', 0
s_pass_w2   db 'PASS VRAM through the VGA window A0000h', 0
s_none      db 'FAIL no window reached the VRAM', 0
s_bad_rest  db 'FAIL not restored', 0

saved       times SETUP_COUNT db 0
readback    times SETUP_COUNT db 0
misc_in     db 0
misc_set    db 0
misc_out    db 0
sr06_save   db 0
reg1_in     db 0
reg1_out    db 0
smbc_in     db 0
use_reg1    db 0
dummy       db 0
v_pre       times 4 db 0
v_b0        times 4 db 0
v_b1        times 4 db 0
verdict     db 0
verdict_w1  db 0
verdict_w2  db 0
restore_ng  db 0

main:
    push cs
    pop ds
    push cs
    pop es
    call common_start
    call require_va
    call board_present
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
    mov byte [opt_mmio], 1
    call reg3_apply
    mov ah, 0x01
    call board_read
    mov [reg1_in], al
    call board_wake
    call crtc_port
    mov ah, 0x27
    call idx_read
    and al, 0xfc
    cmp al, 0x98
    je .chip
    call board_unwake
    call reg3_restore
    call lb_reset
    mov si, s_sum
    call lb_tag
    mov si, s_bad_cr27
    call lb_str
    call lb_emit
    mov al, 0x10
    jmp common_exit

.chip:
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

; Save the incoming values, unlock, enable RAM, apply setup_tab.
chip_setup:
    pusha
    mov dx, VGA_SEQ_I
    mov ah, 0x06
    call idx_read
    mov [sr06_save], al
    mov al, 0x12
    call idx_write
    mov dx, VGA_MISC_R
    in al, dx
    mov [misc_in], al
    or al, 0x02
    mov dx, VGA_MISC_W
    out dx, al
    mov si, setup_tab
    mov di, saved
    mov cx, SETUP_COUNT
.l:
    mov dx, [si]
    mov ah, [si + 2]
    call idx_read
    mov [di], al
    mov al, [si + 3]
    call idx_write
    call idx_read
    mov [di + readback - saved], al
    add si, 4
    inc di
    loop .l
    mov dx, VGA_MISC_R
    in al, dx
    mov [misc_set], al
    popa
    ret

; Restore setup_tab in reverse, Misc, then SR06; restore_ng on a miss.
chip_restore:
    pusha
    mov si, setup_tab + (SETUP_COUNT - 1) * 4
    mov di, saved + SETUP_COUNT - 1
    mov cx, SETUP_COUNT
.l:
    mov dx, [si]
    mov ah, [si + 2]
    mov al, [di]
    call idx_write
    call idx_read
    mov [di + readback - saved], al
    cmp al, [di]
    je .ok
    mov byte [restore_ng], 1
.ok:
    sub si, 4
    dec di
    loop .l
    mov dx, VGA_MISC_W
    mov al, [misc_in]
    out dx, al
    mov dx, VGA_MISC_R
    in al, dx
    mov [misc_out], al
    cmp al, [misc_in]
    je .misc_ok
    mov byte [restore_ng], 1
.misc_ok:
    mov dx, VGA_SEQ_I
    mov ah, 0x06
    mov al, [sr06_save]
    cmp al, 0x12
    je .sr06
    xor al, al
.sr06:
    call idx_write
    mov ah, 0x01
    call board_read
    mov [reg1_out], al
    cmp al, [reg1_in]
    je .done
    mov byte [restore_ng], 1
.done:
    popa
    ret

; SI = label: "<label>,misc=xx reg01=xx SR00=xx ... GR0A=xx" from
; readback[], two lines.
emit_regs:
    pusha
    mov [regs_label], si
    call lb_reset
    call lb_tag
    mov si, s_misc_eq
    call lb_str
    mov al, [misc_set]
    cmp word [regs_label], s_cfg
    je .misc
    mov al, [misc_out]
.misc:
    call lb_hex8
    mov si, s_reg1_eq
    call lb_str
    mov al, [reg1_in]
    cmp word [regs_label], s_cfg
    je .reg1
    mov al, [reg1_out]
.reg1:
    call lb_hex8
    mov si, setup_tab
    mov di, readback
    mov cx, SETUP_COUNT
    xor bx, bx
.l:
    cmp bx, 7
    jne .same
    call lb_emit
    push si
    mov si, [regs_label]
    call lb_tag
    pop si
.same:
    push si
    mov ax, [si]
    mov si, s_sr_eq
    cmp ax, VGA_SEQ_I
    je .name
    mov si, s_gr_eq
.name:
    call lb_str
    pop si
    mov al, [si + 2]
    call lb_hex8
    mov al, '='
    call lb_char
    mov al, [di]
    call lb_hex8
    add si, 4
    inc di
    inc bx
    loop .l
    call lb_emit
    popa
    ret

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

; AL = GR09 (one bank, 4KB steps).
set_bank:
    push ax
    push dx
    mov dx, VGA_GC_I
    mov ah, 0x09
    call idx_write
    pop dx
    pop ax
    ret

; ES = window: DS:DI[0..3] = the bytes at test_offs.
read4:
    push bx
    push cx
    push si
    mov si, test_offs
    mov cx, 4
.l:
    mov bx, [si]
    mov al, [es:bx]
    mov [di], al
    add si, 2
    inc di
    loop .l
    pop si
    pop cx
    pop bx
    ret

; ES = window: the bytes DS:SI[0..3] to test_offs.
write4:
    push bx
    push cx
    push di
    mov di, test_offs
    mov cx, 4
.l:
    mov bx, [di]
    lodsb
    mov [es:bx], al
    add di, 2
    loop .l
    pop di
    pop cx
    pop bx
    ret

; ZF when the four bytes at SI and DI are equal.
cmp4:
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
    cmp cx, cx                      ; all equal: ZF (INC cleared it)
.e:
    pop di
    pop si
    pop cx
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

; Four bytes at SI as hex.
lb_hex4b:
    push cx
    push si
    mov cx, 4
.l:
    lodsb
    call lb_hex8
    loop .l
    pop si
    pop cx
    ret

regs_label  dw 0
