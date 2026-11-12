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
; B3E2.COM (M148 stage B2): does the CL-GD5428 on a PC-9801B3-E02 answer
; on a PC-88VA after the wake-up? VA only; needs B1 (ID 60h).
;
;   PRE   0902h 0904h 0C52h 0C54h 0C5Ch 0C5Eh 0B54h 0D54h; CR27 through
;         0B54h/0B55h and 0D54h/0D55h; SR06 lock test
;   WAKE  0904h=00h, 0902h=01h, 0904h=20h (0902h read in setup)
;   POST  the same reads after the wake-up
;   AFTER the wake-up undone (0904h=00h, 0902h=incoming, 0904h=20h)
;
; PASS: CR27 is 98h-9Bh (CL-GD5428) on the CRTC selected by Misc bit 0,
; and SR06 reads 12h after 12h and 0Fh after 00h. Writes: the CRTC and
; Sequencer indexes (restored), SR06 (left locked unless it came in
; unlocked), 0902h/0904h, and the board index 0FA2h (left 00h).
; /MMIO also sets board reg 03h bit 0 (memory-mapped I/O) for the run and
; restores it; the relay bit is never changed.
;
; Exit: 0 pass; 3 no board; 10h CR27 not a CL-GD5428; 11h SR06 lock not
; working (CR27 passed); 12h state not restored.

    bits 16
    cpu 186
    org 0x100

    jmp main

%include "b3e02.inc"

stage_str   db 'B2', 0
prog_str    db 'B3E2', 0
opt_mmio    db 0
opt_table   dw s_opt_mmio, opt_mmio
            dw 0
s_opt_mmio  db '/MMIO', 0
usage_str   db 'B3E2 (M148 B2) [/MMIO]: wake-up, CR27, SR06 lock. VA only.', 13, 10, 0

snap_ports  dw B3_POS102, B3_SETUP, VGA_STAT0, VGA_SEQ_I
            dw VGA_MISC_R, VGA_GC_I, VGA_CRTC_MONO, VGA_CRTC_COLOR
SNAP_COUNT  equ ($ - snap_ports) / 2

s_pre       db 'PRE', 0
s_post      db 'POST', 0
s_after     db 'AFTER', 0
s_wake      db 'WAKE', 0
s_mmio      db 'MMIO', 0
s_cr27      db 'CR27', 0
s_sel       db ' sel=', 0
s_sr06t     db 'SR06', 0
s_pos_in    db '0902h-in-setup=', 0
s_pos_set   db ' 0902h-after-01=', 0
s_pos_out   db '0902h-restored=', 0
s_reg3      db 'reg03 in=', 0
s_set       db ' set=', 0
s_back      db ' restored=', 0
s_sum       db 'SUM', 0
s_no_board  db 'NO-BOARD reg 00h is not 60h', 0
s_pass_all  db 'PASS CR27=CL-GD5428 and SR06 lock', 0
s_bad_cr27  db 'FAIL CR27 is not 98h-9Bh: chip not reached', 0
s_bad_sr06  db 'FAIL SR06 lock (CR27 passed)', 0
s_bad_rest  db 'FAIL not restored', 0

cur_label   dw s_pre
gc_pre      db 0
gc_pre_cls  db 0
gc_after    db 0
gc_after_cls db 0
reg3_in     db 0
reg3_set    db 0
reg3_out    db 0
cr27_val    db 0xff
cr27_cls    db 0
cr27_port   dw 0
sr06_ok     db 0
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
    cmp byte [opt_mmio], 0
    je .pre
    mov ah, 0x03
    call board_read
    mov [reg3_in], al
    or al, 0x01
    mov ah, 0x03
    call board_write
    call board_read
    mov [reg3_set], al
.pre:
    mov word [cur_label], s_pre
    call snapshot
    mov al, [gc_snap]
    mov [gc_pre], al
    mov al, [gc_snap_cls]
    mov [gc_pre_cls], al

    call board_wake
    call lb_reset
    mov si, s_wake
    call lb_tag
    mov si, s_pos_in
    call lb_str
    mov al, [pos_in]
    call lb_hex8
    mov si, s_pos_set
    call lb_str
    mov al, [pos_set]
    call lb_hex8
    call lb_emit

    mov word [cur_label], s_post
    call snapshot
    ; judge CR27 on the CRTC that Misc bit 0 selects
    call crtc_port
    mov [cr27_port], dx
    mov al, [cr27_mono]
    mov ah, [cr27_mono_cls]
    cmp dx, VGA_CRTC_MONO
    je .cr
    mov al, [cr27_color]
    mov ah, [cr27_color_cls]
.cr:
    mov [cr27_val], al
    mov [cr27_cls], ah
    mov al, [sr06_snap_ok]
    mov [sr06_ok], al

    call board_unwake
    mov word [cur_label], s_after
    call lb_reset
    mov si, s_after
    call lb_tag
    mov si, s_pos_out
    call lb_str
    mov al, [pos_out]
    call lb_hex8
    mov dx, VGA_GC_I
    call lb_port_probe
    mov [gc_after], al
    mov al, [probe_class]
    mov [gc_after_cls], al
    mov dx, VGA_MISC_R
    call lb_port_probe
    call lb_emit
    ; the GC index reads as before the wake-up
    mov al, [gc_after_cls]
    cmp al, [gc_pre_cls]
    jne .gc_ng
    cmp al, 1
    jne .gc_ok
    mov al, [gc_after]
    cmp al, [gc_pre]
    je .gc_ok
.gc_ng:
    mov byte [restore_ng], 1
.gc_ok:

    cmp byte [opt_mmio], 0
    je .sum
    mov ah, 0x03
    mov al, [reg3_in]
    call board_write
    call board_read
    mov [reg3_out], al
    cmp al, [reg3_in]
    je .mmio_line
    mov byte [restore_ng], 1
.mmio_line:
    call lb_reset
    mov si, s_mmio
    call lb_tag
    mov si, s_reg3
    call lb_str
    mov al, [reg3_in]
    call lb_hex8
    mov si, s_set
    call lb_str
    mov al, [reg3_set]
    call lb_hex8
    mov si, s_back
    call lb_str
    mov al, [reg3_out]
    call lb_hex8
    call lb_emit

.sum:
    call lb_reset
    mov si, s_sum
    call lb_tag
    mov ah, 0x10
    mov si, s_bad_cr27
    cmp byte [cr27_cls], 1
    jne .verdict
    mov al, [cr27_val]
    and al, 0xfc
    cmp al, 0x98
    jne .verdict
    mov ah, 0x11
    mov si, s_bad_sr06
    cmp byte [sr06_ok], 0
    je .verdict
    mov ah, 0x12
    mov si, s_bad_rest
    cmp byte [restore_ng], 0
    jne .verdict
    xor ah, ah
    mov si, s_pass_all
.verdict:
    call lb_str
    push ax
    call lb_emit
    pop ax
    mov al, ah
    jmp common_exit

; Three lines labelled [cur_label]: the snap_ports (two lines), then CR27
; through both CRTCs and the SR06 lock test.
snapshot:
    pusha
    mov si, snap_ports
    mov cx, SNAP_COUNT
    xor bx, bx
.port:
    or bx, bx
    jnz .same
    call lb_reset
    push si
    mov si, [cur_label]
    call lb_tag
    pop si
.same:
    lodsw
    mov dx, ax
    push bx
    call lb_port_probe
    pop bx
    cmp dx, VGA_GC_I
    jne .not_gc
    mov [gc_snap], al
    mov al, [probe_class]
    mov [gc_snap_cls], al
.not_gc:
    inc bx
    cmp bx, 4
    jb .next
    call lb_emit
    xor bx, bx
.next:
    loop .port
    or bx, bx
    jz .cr27
    call lb_emit
.cr27:
    call lb_reset
    mov si, [cur_label]
    call lb_tag
    mov si, s_cr27
    call lb_str
    mov dx, VGA_CRTC_MONO
    call cr27_probe
    mov [cr27_mono], al
    mov al, [probe_class]
    mov [cr27_mono_cls], al
    mov dx, VGA_CRTC_COLOR
    call cr27_probe
    mov [cr27_color], al
    mov al, [probe_class]
    mov [cr27_color_cls], al
    mov si, s_sel
    call lb_str
    call crtc_port
    mov ax, dx
    call lb_hex16
    mov byte [sr06_snap_ok], 0
    call sr06_test
    jc .sr06
    mov byte [sr06_snap_ok], 1
.sr06:
    call lb_sr06
    call lb_emit
    popa
    ret

; DX = CRTC index port: index 27h, probe the data port (" pppp=..."),
; restore the incoming index. AL = first reading.
cr27_probe:
    push bx
    in al, dx
    mov [crtc_in], al
    mov al, 0x27
    out dx, al
    inc dx
    call lb_port_probe
    dec dx
    push ax
    mov al, [crtc_in]
    out dx, al
    pop ax
    pop bx
    ret

gc_snap         db 0
gc_snap_cls     db 0
cr27_mono       db 0
cr27_mono_cls   db 0
cr27_color      db 0
cr27_color_cls  db 0
crtc_in         db 0
sr06_snap_ok    db 0
