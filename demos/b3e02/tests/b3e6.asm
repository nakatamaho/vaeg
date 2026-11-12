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
; B3E6.COM (M148 stage B4c): does the PC-9801B3-E02 see the PC-88VA's
; memory cycles at all, and is there a window below 1MB? VA only; needs
; B2.
;
; Part L (latch): the BLT fills VRAM 000000h-0000FFh with X and
; 010000h-0100FFh with the complement (ROP 00h = 00h, ROP 0Eh = FFh), so
; that a latch the BLT may have loaded holds the complement. Then the CPU
; reads one byte at a candidate address with a candidate board window,
; and the VGA latch is read through CR22 (GR04 = 00h). If the board turned
; the CPU read into a VRAM read at window offset 0, the latch is X, for
; X = 00h and for X = FFh, even when the CPU itself gets its own RAM or
; FFh. A control with the window off must not follow. Candidates:
;   reg01=00h 6000:0000           control
;   reg01=E0h 6000:0000           F60000h if the slot's A20-A23 are 1
;   reg01=C0h 4000:0000           F40000h if the slot's A20-A23 are 1
;   reg01=10h B000:0000           0B0000h, system memory as it is (TVRAM)
;   reg01=10h B000:0000 SMBC=0    0B0000h through the slot
;   reg01=00h A000:0000 SMBC=0    the chip's VGA window (GR06 = 05h)
; reg01 = A0h/80h (CPU 00000h/20000h if A20-A23 are 1) are never used;
; C0h/E0h are skipped when this program lies at or above 30000h. Where
; the latch follows, a write is tried too: the byte is saved, 5Ah written
; and read back (latch), the byte restored.
;
; Part M (memory-mapped I/O window, 98 I/O documentation: reg04 15MB mode
; puts a 4KB window at 0DF000h/0DB000h/0D7000h/0D3000h; the -85/-91 form
; of reg01 with bit 7 = 0 opens one at 0C0000h + n*1000h): with reg01 =
; 00h and then 40h..67h, read the first four bytes of the 4KB block that
; value names (C0000h-DFFFFh through SMBC=0, E0000h-E7FFFh through 152h
; bit 3). Blocks that differ from the reg01 = 00h reading are listed, and
; the reg01 = 00h readings of D3000h/D7000h/DB000h/DF000h are shown.
; Read only; VRAM 000000h-0000FFh is filled with 00h first, so that a
; window onto it differs from an empty slot (FFh).
;
; Writes: board reg 03h bit 0 and reg 01h (restored), the wake-up, the
; window.inc chip set-up and GR20-GR32 (restored), VRAM 000000h-0000FFh,
; VRAM 010000h-0100FFh, and for an echoing candidate the saved-and-restored test byte.
;
; Exit: 0 a candidate echoed or a block answered; 3 no board; 10h CR27;
; 11h nothing; 12h state not restored; 14h the control followed (method
; not usable).

    bits 16
    cpu 186
    org 0x100

    jmp main

%include "b3e02.inc"
%include "window.inc"

stage_str   db 'B4', 0
prog_str    db 'B3E6', 0
opt_table   dw 0
usage_str   db 'B3E6 (M148 B4c): no options. Latch echo and low windows, VA only.', 13, 10, 0

test_offs   dw 0x0000, 0x0001, 0x8000, 0xffff

; candidates: reg01, segment, SMBC=0 flag, needs program below 30000h
cand_tab    db 0x00
            dw 0x6000
            db 0, 1
            db 0xe0
            dw 0x6000
            db 0, 1
            db 0xc0
            dw 0x4000
            db 0, 1
            db 0x10
            dw 0xb000
            db 0, 0
            db 0x10
            dw 0xb000
            db 1, 0
            db 0x00
            dw 0xa000
            db 1, 0
CAND_COUNT  equ ($ - cand_tab) / 5

blt_regs    db 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27
            db 0x28, 0x29, 0x2a, 0x2c, 0x2d, 0x2e, 0x30, 0x32
BLT_COUNT   equ $ - blt_regs
; fill 256 x 1 at 0, pitch 256 (ROP set separately)
blt_vals    db 0xff, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01
            db 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

s_sum       db 'SUM', 0
s_no_board  db 'NO-BOARD reg 00h is not 60h', 0
s_bad_cr27  db 'STOP CR27 is not 98h-9Bh', 0
s_where     db 'WHERE', 0
s_cs_eq     db 'cs=', 0
s_dip_eq    db ' reg04=', 0
s_latch     db 'LATCH', 0
s_reg01     db 'reg01=', 0
s_seg_eq    db ' seg=', 0
s_smbc0     db ' smbc0', 0
s_l_in      db ' in=', 0
s_l00       db ' f00>', 0
s_lff       db ' fFF>', 0
s_st        db ' st=', 0
s_w5a       db ' w5A>', 0
s_skip      db ' SKIP (program too high)', 0
s_echo      db ' ECHO', 0
s_noecho    db ' NO', 0
s_wecho     db ' WRITE-ECHO', 0
s_low       db 'LOW', 0
s_lowbase   db 'LOWBASE', 0
s_blocks    db ' blocks:', 0
s_n_eq      db ' n=', 0
s_pass_echo db 'PASS the board sees CPU memory cycles (LATCH)', 0
s_pass_low  db 'PASS a block below 1MB answered (LOW)', 0
s_nothing   db 'FAIL no echo, no low block', 0
s_bad_ctrl  db 'FAIL the control followed: latch method not usable', 0
s_bad_rest  db 'FAIL not restored', 0

blt_saved   times BLT_COUNT db 0
cur_reg1    db 0
cur_seg     dw 0
cur_smbc0   db 0
smbc_in     db 0
rombank_in  db 0
latch_in    db 0
latch00     db 0
latchff     db 0
latch5a     db 0
blt_st      db 0
any_echo    db 0
ctrl_bad    db 0
any_low     db 0
n_low       dw 0
prog_high   db 0
dip         db 0
low_base    times 4 db 0
low_val     times 4 db 0
low_blk     dw 0

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
    call where

    ; save GR20-GR32
    mov si, blt_regs
    mov di, blt_saved
    mov cx, BLT_COUNT
.save:
    mov dx, VGA_GC_I
    mov ah, [si]
    call idx_read
    mov [di], al
    inc si
    inc di
    loop .save

    ; part L
    mov si, cand_tab
    mov cx, CAND_COUNT
    xor bx, bx
.cand:
    push cx
    push si
    mov al, [si]
    mov [cur_reg1], al
    mov ax, [si + 1]
    mov [cur_seg], ax
    mov al, [si + 3]
    mov [cur_smbc0], al
    mov al, [si + 4]
    and al, [prog_high]
    call latch_test
    pop si
    pop cx
    add si, 5
    inc bx
    loop .cand

    ; VRAM 000000h-0000FFh = 00h for part M (FFh is the empty-slot value)
    mov ax, 0x0000
    call blt_fill
    ; restore GR20-GR32
    mov si, blt_regs
    mov di, blt_saved
    mov cx, BLT_COUNT
.rest:
    mov dx, VGA_GC_I
    mov ah, [si]
    mov al, [di]
    call idx_write
    inc si
    inc di
    loop .rest

    ; part M
    call low_scan

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
    mov ah, 0x14
    mov si, s_bad_ctrl
    cmp byte [ctrl_bad], 0
    jne .verdict
    xor ah, ah
    mov si, s_pass_echo
    cmp byte [any_echo], 0
    jne .verdict
    mov si, s_pass_low
    cmp byte [any_low], 0
    jne .verdict
    mov ah, 0x11
    mov si, s_nothing
.verdict:
    call lb_str
    push ax
    call lb_emit
    pop ax
    mov al, ah
    jmp common_exit

; WHERE,cs=xxxx reg04=xx; prog_high when the program ends at or above
; 30000h.
where:
    pusha
    mov ah, 0x04
    call board_read
    mov [dip], al
    mov ax, cs
    add ax, 0x1000                  ; 64KB segment
    cmp ax, 0x3000
    jb .low
    mov byte [prog_high], 1
.low:
    call lb_reset
    mov si, s_where
    call lb_tag
    mov si, s_cs_eq
    call lb_str
    mov ax, cs
    call lb_hex16
    mov si, s_dip_eq
    call lb_str
    mov al, [dip]
    call lb_hex8
    call lb_emit
    popa
    ret

; AL = rop, AH = destination bits 23-16: BLT fill of 256 bytes there.
; blt_st = GR31 afterwards.
blt_fill:
    pusha
    mov bl, al
    mov bh, ah
    mov si, blt_regs
    mov di, blt_vals
    mov cx, BLT_COUNT
.l:
    mov dx, VGA_GC_I
    mov ah, [si]
    mov al, [di]
    cmp ah, 0x32
    jne .dst
    mov al, bl
.dst:
    cmp ah, 0x2a
    jne .w
    mov al, bh
.w:
    call idx_write
    inc si
    inc di
    loop .l
    mov dx, VGA_GC_I
    mov ah, 0x31
    mov al, 0x02
    call idx_write
    mov cx, 0xffff
.wait:
    call idx_read
    test al, 0x01
    jz .done
    loop .wait
.done:
    mov [blt_st], al
    popa
    ret

; CR22 (latch, plane GR04 = 0) -> AL.
read_latch:
    push dx
    push ax
    call crtc_port
    pop ax
    mov ah, 0x22
    call idx_read
    pop dx
    ret

; One CPU read of cur_seg:0000 with reg01 = cur_reg1 (SMBC=0 when
; cur_smbc0), interrupts off; AL = latch afterwards.
probe_read:
    push bx
    push dx
    push es
    cli
    call open_window
    mov es, [cur_seg]
    mov bl, [es:0x0000]
    call close_window
    sti
    pop es
    pop dx
    pop bx
    call read_latch
    ret

open_window:
    push ax
    push dx
    cmp byte [cur_smbc0], 0
    je .reg1
    mov dx, PORT_SMBC
    in al, dx
    mov [smbc_in], al
    and al, 0xf0
    out dx, al
.reg1:
    mov ah, 0x01
    mov al, [cur_reg1]
    call board_write
    pop dx
    pop ax
    ret

close_window:
    push ax
    push dx
    mov ah, 0x01
    mov al, [reg1_in]
    call board_write
    cmp byte [cur_smbc0], 0
    je .done
    mov dx, PORT_SMBC
    mov al, [smbc_in]
    out dx, al
.done:
    pop dx
    pop ax
    ret

; AL = 0: skip. Fill 00h, read, fill FFh, read; report.
latch_test:
    pusha
    mov [latch5a], byte 0
    push ax
    call lb_reset
    mov si, s_latch
    call lb_tag
    mov si, s_reg01
    call lb_str
    mov al, [cur_reg1]
    call lb_hex8
    mov si, s_seg_eq
    call lb_str
    mov ax, [cur_seg]
    call lb_hex16
    cmp byte [cur_smbc0], 0
    je .nosm
    mov si, s_smbc0
    call lb_str
.nosm:
    pop ax
    ; the C0h/E0h candidates need the program below 30000h
    or al, al
    jz .run
    mov si, s_skip
    call lb_str
    call lb_emit
    jmp .done
.run:
    call read_latch
    mov [latch_in], al
    mov ax, 0x0000                  ; 00h at 000000h
    call blt_fill
    mov ax, 0x010e                  ; FFh at 010000h
    call blt_fill
    call probe_read
    mov [latch00], al
    mov ax, 0x000e                  ; FFh at 000000h
    call blt_fill
    mov ax, 0x0100                  ; 00h at 010000h
    call blt_fill
    call probe_read
    mov [latchff], al
    mov si, s_l_in
    call lb_str
    mov al, [latch_in]
    call lb_hex8
    mov si, s_l00
    call lb_str
    mov al, [latch00]
    call lb_hex8
    mov si, s_lff
    call lb_str
    mov al, [latchff]
    call lb_hex8
    mov si, s_st
    call lb_str
    mov al, [blt_st]
    call lb_hex8
    cmp byte [latch00], 0x00
    jne .no
    cmp byte [latchff], 0xff
    jne .no
    ; followed
    cmp byte [cur_reg1], 0x00
    jne .echo
    cmp byte [cur_smbc0], 0
    jne .echo
    mov byte [ctrl_bad], 1
.echo:
    mov byte [any_echo], 1
    mov si, s_echo
    call lb_str
    call write_test
    mov si, s_w5a
    call lb_str
    mov al, [latch5a]
    call lb_hex8
    cmp al, 0x5a
    jne .emit
    mov si, s_wecho
    call lb_str
    jmp .emit
.no:
    mov si, s_noecho
    call lb_str
.emit:
    call lb_emit
.done:
    popa
    ret

; Save the byte, write 5Ah, read it, restore it; latch5a = latch.
write_test:
    pusha
    push es
    cli
    call open_window
    mov es, [cur_seg]
    mov bl, [es:0x0000]
    mov byte [es:0x0000], 0x5a
    mov al, [es:0x0000]
    call read_latch_nocli
    mov [latch5a], al
    mov [es:0x0000], bl
    call close_window
    sti
    pop es
    popa
    ret

read_latch_nocli:
    jmp read_latch

; Part M.
low_scan:
    pusha
    ; reg01 = 00h readings of the DIP blocks
    call lb_reset
    mov si, s_lowbase
    call lb_tag
    mov bx, 0xd300
.dip:
    mov [low_blk], bx
    mov byte [cur_reg1], 0x00
    call low_read
    mov al, ' '
    call lb_char
    mov ax, bx
    call lb_hex16
    mov al, ':'
    call lb_char
    mov si, low_val
    call lb_hex4b
    add bx, 0x0400
    cmp bx, 0xe300
    jb .dip
    call lb_emit
    ; reg01 = 40h..67h against reg01 = 00h
    call lb_reset
    mov si, s_low
    call lb_tag
    mov si, s_blocks
    call lb_str
    mov cl, 0x40
.v:
    mov al, cl
    and al, 0x3f
    xor ah, ah
    shl ax, 8                       ; segment C000h + n*100h
    add ax, 0xc000
    mov [low_blk], ax
    mov byte [cur_reg1], 0x00
    call low_read
    mov si, low_val
    mov di, low_base
    movsw
    movsw
    mov [cur_reg1], cl
    call low_read
    mov si, low_val
    mov di, low_base
    call cmp4
    je .next
    mov byte [any_low], 1
    inc word [n_low]
    cmp word [n_low], 6
    ja .next
    mov al, ' '
    call lb_char
    mov al, cl
    call lb_hex8
    mov al, '@'
    call lb_char
    mov ax, [low_blk]
    call lb_hex16
    mov al, ':'
    call lb_char
    mov si, low_val
    call lb_hex4b
.next:
    inc cl
    cmp cl, 0x68
    jb .v
    mov si, s_n_eq
    call lb_str
    mov ax, [n_low]
    call lb_dec16
    call lb_emit
    popa
    ret

; low_val = four bytes at low_blk:0000 with reg01 = cur_reg1, through
; SMBC=0 (below E000h) or 152h bit 3; interrupts off.
low_read:
    pusha
    push es
    cli
    mov ax, [low_blk]
    cmp ax, 0xe000
    jae .rom
    mov dx, PORT_SMBC
    in al, dx
    mov [smbc_in], al
    and al, 0xf0
    out dx, al
    jmp .open
.rom:
    mov dx, PORT_ROMBANK
    in al, dx
    mov [rombank_in], al
    or al, 0x08
    out dx, al
.open:
    mov ah, 0x01
    mov al, [cur_reg1]
    call board_write
    mov es, [low_blk]
    mov ax, [es:0x0000]
    mov [low_val], ax
    mov ax, [es:0x0002]
    mov [low_val + 2], ax
    mov ah, 0x01
    mov al, [reg1_in]
    call board_write
    cmp word [low_blk], 0xe000
    jae .rom_back
    mov dx, PORT_SMBC
    mov al, [smbc_in]
    out dx, al
    jmp .done
.rom_back:
    mov dx, PORT_ROMBANK
    mov al, [rombank_in]
    out dx, al
.done:
    sti
    pop es
    popa
    ret
