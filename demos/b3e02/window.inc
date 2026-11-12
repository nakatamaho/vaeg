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
; window.inc (M148): chip set-up for the VRAM window probes, after the
; B3 snapshot (docs/agents/tasks/M148, 8.4). Needs b3e02.inc. The program
; defines test_offs (four words), s_sum, s_no_board and s_bad_cr27.

    jmp window_inc_end

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

s_cfg       db 'CFG', 0
s_rest      db 'RESTORE', 0
s_misc_eq   db 'misc=', 0
s_reg1_eq   db ' reg01=', 0
s_sr_eq     db ' SR', 0
s_gr_eq     db ' GR', 0

saved       times SETUP_COUNT db 0
readback    times SETUP_COUNT db 0
misc_in     db 0
misc_set    db 0
misc_out    db 0
sr06_save   db 0
reg1_in     db 0
reg1_out    db 0
restore_ng  db 0
regs_label  dw 0

; Board check, reg 03h bit 0, reg 01h saved, wake-up, CR27 check.
; Exits 3 (no board) or 10h (CR27) itself, everything undone.
chip_start:
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
    ret

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


window_inc_end:
