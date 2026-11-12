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
; B3E3.COM (M148 stage B3): register snapshot of the CL-GD5428 on a
; PC-9801B3-E02 after the wake-up, on a PC-88VA. The board has no BIOS,
; so these are the power-on values (clock, SR0F/SR17 memory settings)
; that the window stage starts from. VA only; needs B2 (CR27).
;
; Reads Misc, Feature, Input Status 0, the DAC state and the incoming
; SR/GR/CR indexes, then SR00-SR1F, GR00-GR3F and CR00-CR3F with SR06
; unlocked (12h). Writes: 0902h/0904h (wake-up, undone at the end), the
; SR/GR/CR indexes (restored after every read), SR06 (left locked unless
; it came in unlocked), and the board index 0FA2h (left 00h).
; /MMIO and /RELAY set board reg 03h bit 0 / bit 1 for the run, as in
; B3E2; use the options with which B3E2 passed.
;
; Exit: 0 snapshot taken; 3 no board; 10h CR27 not 98h-9Bh (stage B2 not
; passed; nothing dumped).

    bits 16
    cpu 186
    org 0x100

    jmp main

%include "b3e02.inc"

stage_str   db 'B3', 0
prog_str    db 'B3E3', 0
opt_table   dw s_opt_mmio, opt_mmio
            dw s_opt_relay, opt_relay
            dw 0
usage_str   db 'B3E3 (M148 B3) [/MMIO] [/RELAY]: CL-GD5428 register snapshot, VA only.', 13, 10, 0

s_state     db 'STATE', 0
s_misc      db 'misc=', 0
s_feat      db ' feat=', 0
s_stat0     db ' stat0=', 0
s_dacst     db ' dacstate=', 0
s_sri       db 'sr-idx=', 0
s_gri       db ' gr-idx=', 0
s_cri       db ' cr-idx=', 0
s_crtc      db ' crtc=', 0
s_cr27      db ' cr27=', 0
s_sr        db 'SR', 0
s_gr        db 'GR', 0
s_cr        db 'CR', 0
s_unlock    db 'UNLOCK', 0
s_sum       db 'SUM', 0
s_no_board  db 'NO-BOARD reg 00h is not 60h', 0
s_bad_cr27  db 'STOP CR27 is not 98h-9Bh: run B3E2 first', 0
s_done      db 'DONE snapshot taken', 0

crtc        dw VGA_CRTC_MONO
cr27_value  db 0
sr06_orig   db 0
dump_name   dw 0
dump_label  times 8 db 0

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
    call reg3_apply
    call board_wake
    call crtc_port
    mov [crtc], dx
    mov ah, 0x27
    call idx_read
    mov [cr27_value], al

    ; state line (all reads)
    call lb_reset
    mov si, s_state
    call lb_tag
    mov si, s_misc
    mov dx, VGA_MISC_R
    call lb_in
    mov si, s_feat
    mov dx, VGA_FEAT_R
    call lb_in
    mov si, s_stat0
    mov dx, VGA_STAT0
    call lb_in
    mov si, s_dacst
    mov dx, 0x0c57
    call lb_in
    call lb_emit
    mov si, s_state
    call lb_tag
    mov si, s_sri
    mov dx, VGA_SEQ_I
    call lb_in
    mov si, s_gri
    mov dx, VGA_GC_I
    call lb_in
    mov si, s_cri
    mov dx, [crtc]
    call lb_in
    mov si, s_crtc
    call lb_str
    mov ax, [crtc]
    call lb_hex16
    mov si, s_cr27
    call lb_str
    mov al, [cr27_value]
    call lb_hex8
    call lb_emit

    mov al, [cr27_value]
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
    ; unlock the extensions for the dump
    mov dx, VGA_SEQ_I
    mov ah, 0x06
    call idx_read
    mov [sr06_orig], al
    mov al, 0x12
    call idx_write
    mov ah, 0x06
    call idx_read
    call lb_reset
    mov si, s_unlock
    call lb_tag
    mov si, s_sr06
    call lb_str
    push ax
    mov al, [sr06_orig]
    call lb_hex8
    pop ax
    mov si, s_sr06_12
    call lb_str
    call lb_hex8
    call lb_emit

    mov dx, VGA_SEQ_I
    mov si, s_sr
    mov cx, 0x20
    call dump
    mov dx, VGA_GC_I
    mov si, s_gr
    mov cx, 0x40
    call dump
    mov dx, [crtc]
    mov si, s_cr
    mov cx, 0x40
    call dump

    ; relock unless it came in unlocked
    mov dx, VGA_SEQ_I
    mov ah, 0x06
    mov al, [sr06_orig]
    cmp al, 0x12
    je .relock
    xor al, al
.relock:
    call idx_write
    call board_unwake
    call reg3_restore

    call lb_reset
    mov si, s_sum
    call lb_tag
    mov si, s_done
    call lb_str
    call lb_emit
    xor al, al
    jmp common_exit

; SI = label, DX = port: append "<label>xx" (one plain IN).
lb_in:
    call lb_str
    in al, dx
    call lb_hex8
    ret

; DX = index port, SI = name ("SR"), CX = count (multiple of 16): lines
; "<stage>,<prog>,SR00,xx xx ..." of 16 registers each.
dump:
    pusha
    mov [dump_name], si
    xor bx, bx                      ; BL = register
.line:
    call lb_reset
    ; label "SRnn"
    mov si, stage_str
    call lb_str
    mov al, ','
    call lb_char
    mov si, prog_str
    call lb_str
    mov al, ','
    call lb_char
    mov si, [dump_name]
    call lb_str
    mov al, bl
    call lb_hex8
    mov al, ','
    call lb_char
    push cx
    mov cx, 16
.reg:
    mov ah, bl
    call idx_read
    call lb_hex8
    cmp cx, 1
    je .no_sp
    mov al, ' '
    call lb_char
.no_sp:
    inc bl
    loop .reg
    pop cx
    call lb_emit
    sub cx, 16
    ja .line
    popa
    ret
