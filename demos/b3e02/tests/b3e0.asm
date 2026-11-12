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
; B3E0.COM (M148 stage B0): read-only baseline of the PC-9801B3-E02
; ports and of the two slot memory windows. VA, VA2/VA3.
;
; Reads every B3-E02 port with six code patterns (FF / D:xx driven /
; F:... floating). Then, with interrupts off, opens B0000h-BFFFFh to the
; slot (153h SMBC=0) and E0000h-EFFFFh to the slot (152h bit 3) for a few
; reads each, and restores 153h/152h (read back). No B3-E02 port and no
; memory is written.
;
; Exit: 0 every port FF; 10h a port is driven; 11h a port floats (none
; driven); 12h 152h/153h not restored.

    bits 16
    cpu 186
    org 0x100

    jmp main

%include "b3e02.inc"

stage_str   db 'B0', 0
prog_str    db 'B3E0', 0
opt_table   dw 0
usage_str   db 'B3E0 (M148 B0): no options. Read-only port and window baseline.', 13, 10, 0

ports       dw VGA_ATTR_W, VGA_ATTR_R, VGA_STAT0, 0x0c53
            dw VGA_SEQ_I, VGA_SEQ_D, VGA_DAC_MASK, 0x0c57
            dw 0x0c58, 0x0c59, VGA_FEAT_R, VGA_MISC_R
            dw VGA_GC_I, VGA_GC_D, 0x0d54, 0x0d55
            dw VGA_STAT1_COLOR, VGA_CRTC_MONO, 0x0b55, VGA_STAT1_MONO
            dw B3_IDX, B3_DATA, 0x0faa, 0x0fab
            dw B3_FAC, B3_POS102, B3_SETUP
PORT_COUNT  equ ($ - ports) / 2

mem_offs    dw 0x0000, 0x0001, 0x8000, 0xfffe

s_p         db 'P', 0
s_mem       db 'MEM', 0
s_b0000     db 'B0000 153h=', 0
s_e0000     db 'E0000 152h=', 0
s_sum       db 'SUM', 0
s_ff_eq     db 'ff=', 0
s_drv_eq    db ' driven=', 0
s_flt_eq    db ' float=', 0
s_rest      db ' restore=', 0
s_ok        db 'OK', 0
s_ng        db 'NG', 0
s_pass_all  db ' PASS all FF', 0
s_driven    db ' DRIVEN (expected only with a board)', 0
s_floating  db ' FLOAT (not expected on a VA)', 0

n_ff        db 0
n_driven    db 0
n_float     db 0
restore_ng  db 0
bank_in     db 0
bank_set    db 0
bank_out    db 0
mem_vals    times 8 db 0

main:
    push cs
    pop ds
    push cs
    pop es
    call common_start

    ; ports, three to a line
    mov si, ports
    mov cx, PORT_COUNT
    xor bx, bx
.port:
    or bx, bx
    jnz .same_line
    call lb_reset
    push si
    mov si, s_p
    call lb_tag
    pop si
.same_line:
    lodsw
    mov dx, ax
    push bx
    call lb_port_probe
    pop bx
    mov al, [probe_class]
    cmp al, 0
    jne .not_ff
    inc byte [n_ff]
    jmp .counted
.not_ff:
    cmp al, 1
    jne .flt
    inc byte [n_driven]
    jmp .counted
.flt:
    inc byte [n_float]
.counted:
    inc bx
    cmp bx, 3
    jb .next
    call lb_emit
    xor bx, bx
.next:
    loop .port
    or bx, bx
    jz .ports_done
    call lb_emit
.ports_done:

    call window_b
    call window_e

    ; summary
    call lb_reset
    mov si, s_sum
    call lb_tag
    mov si, s_ff_eq
    call lb_str
    mov al, [n_ff]
    call lb_hex8
    mov si, s_drv_eq
    call lb_str
    mov al, [n_driven]
    call lb_hex8
    mov si, s_flt_eq
    call lb_str
    mov al, [n_float]
    call lb_hex8
    mov si, s_rest
    call lb_str
    mov si, s_ok
    cmp byte [restore_ng], 0
    je .r
    mov si, s_ng
.r:
    call lb_str
    mov ah, 0x12
    cmp byte [restore_ng], 0
    jne .verdict_rest
    mov ah, 0x10
    mov si, s_driven
    cmp byte [n_driven], 0
    jne .verdict
    mov ah, 0x11
    mov si, s_floating
    cmp byte [n_float], 0
    jne .verdict
    xor ah, ah
    mov si, s_pass_all
.verdict:
    call lb_str
.verdict_rest:
    push ax
    call lb_emit
    pop ax
    mov al, ah
    jmp common_exit

; B0000h-BFFFFh through the slot: 153h SMBC (bits 3-0) = 0.
window_b:
    pusha
    cli
    mov dx, PORT_SMBC
    in al, dx
    mov [bank_in], al
    and al, 0xf0
    out dx, al
    in al, dx
    mov [bank_set], al
    mov ax, 0xb000
    mov es, ax
    call mem_sample
    mov dx, PORT_SMBC
    mov al, [bank_in]
    out dx, al
    in al, dx
    mov [bank_out], al
    sti
    push ds
    pop es
    mov si, s_b0000
    call emit_window
    popa
    ret

; E0000h-EFFFFh through the slot: 152h bit 3 = 1.
window_e:
    pusha
    cli
    mov dx, PORT_ROMBANK
    in al, dx
    mov [bank_in], al
    or al, 0x08
    out dx, al
    in al, dx
    mov [bank_set], al
    mov ax, 0xe000
    mov es, ax
    call mem_sample
    mov dx, PORT_ROMBANK
    mov al, [bank_in]
    out dx, al
    in al, dx
    mov [bank_out], al
    sti
    push ds
    pop es
    mov si, s_e0000
    call emit_window
    popa
    ret

; ES = window: mem_vals = two reads (patterns A, B) of each mem_offs.
mem_sample:
    mov si, mem_offs
    mov di, mem_vals
    mov cx, 4
.l:
    mov bx, [si]
    call mem_read_a
    mov [di], al
    call mem_read_b
    mov [di + 1], al
    add si, 2
    add di, 2
    loop .l
    ret

    align 2
mem_read_a:
    mov al, [es:bx]
    mov ah, 0x33
    mov ah, 0x33
    mov ah, 0x33
    ret

    align 2
mem_read_b:
    mov al, [es:bx]
    mov dh, 0xcc
    mov dh, 0xcc
    mov dh, 0xcc
    ret

; SI = label: "MEM,<label>in>set>out oooo=aa/bb ..."; flags a restore miss.
emit_window:
    call lb_reset
    push si
    mov si, s_mem
    call lb_tag
    pop si
    call lb_str
    mov al, [bank_in]
    call lb_hex8
    mov al, '>'
    call lb_char
    mov al, [bank_set]
    call lb_hex8
    mov al, '>'
    call lb_char
    mov al, [bank_out]
    call lb_hex8
    mov al, [bank_out]
    cmp al, [bank_in]
    je .vals
    mov byte [restore_ng], 1
.vals:
    mov si, mem_offs
    mov di, mem_vals
    mov cx, 4
.l:
    mov al, ' '
    call lb_char
    mov ax, [si]
    call lb_hex16
    mov al, '='
    call lb_char
    mov al, [di]
    call lb_hex8
    mov al, '/'
    call lb_char
    mov al, [di + 1]
    call lb_hex8
    add si, 2
    add di, 2
    loop .l
    call lb_emit
    ret
