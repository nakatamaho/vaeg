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
; W1PROBE.COM (stage W1, docs/modernization/wsr-e-pseudo-sprite-plan.md):
; is a WSR-E answering at 4nE0h, and do word OUTs reach it? MS-DOS 4.0,
; V3 mode.
;
; Writes only: 46E8h, 42E0h, 5AE0h (wake-up as AGS_CLGD.COM), SR06,
; GR09, GR0A (restored to 00h), and the CRTC index. Never 40E1h: the
; monitor stays on the VA.
;
;   PRE     probes of 45E0h and 4FE0h before the wake-up
;   SR06B   byte path: SR06=12h reads 12h, SR06=00h reads 0Fh
;   R0      calibration (after the board fields are known)
;   SR06W   word path: OUT 44E0h,AX with AL=06h, AH=12h/00h
;   CR27    chip id (recorded, not judged)
;   GR09B/GR0AB  byte path: 55h, AAh read back
;   GR09W/GR0AW  word path: OUT 4EE0h,AX, read back by byte
;   P       board ports 40E1h, 42E1h, 46E8h, 51E1h, 59E1h, 5BE1h
;   SUM     PASS when every check passed
;
; Exit code: 0 pass, 1 board found but a check failed, 2 no board or no
; VRTC interrupt.

    bits 16
    cpu 186
    org 0x100

    jmp main

%include "wsre.inc"

stage_str   db 'W1', 0
prog_str    db 'W1PROBE', 0

pre_ports       dw WSRE_SEQ_DATA, WSRE_GC_DATA
PRE_PORT_COUNT  equ ($ - pre_ports) / 2
id_ports        dw WSRE_BOARD_40E1, WSRE_BOARD_42E1, WSRE_SETUP
                dw WSRE_ID_51E1, WSRE_ID_59E1, WSRE_ID_5BE1
ID_PORT_COUNT   equ ($ - id_ports) / 2

s_pre       db 'PRE', 0
s_sr06b     db 'SR06B', 0
s_sr06w     db 'SR06W', 0
s_v12       db '12', 0
s_v00       db '00', 0
s_cr27      db 'CR27', 0
s_id        db 'ID', 0
s_gr09b     db 'GR09B', 0
s_gr0ab     db 'GR0AB', 0
s_gr09w     db 'GR09W', 0
s_gr0aw     db 'GR0AW', 0
s_v55       db '55', 0
s_vaa       db 'AA', 0
s_no_board  db 'NO-BOARD', 0

board_found     db 0
sr06_unlocked   db 0
sr06_locked     db 0

main:
    push cs
    pop ds
    push cs
    pop es
    cld
    call common_start
    jnc .run
    mov al, 2
    jmp common_exit
.run:
    call probe_pre_ports
    call wake_board
    call detect_board
    cmp byte [board_found], 0
    jne .board
    mov word [board_str], s_none
    call emit_sr06_byte
    mov word [row_name], s_sum
    mov word [row_param], prog_str
    call check_begin
    mov si, s_no_board
    call lb_str
    call lb_emit
    mov al, 2
    jmp common_exit
.board:
    mov word [board_str], s_yes
    call read_cr27
    call run_r0
    call emit_sr06_byte
    call test_sr06_word
    call emit_cr27
    call test_gr_byte
    call test_gr_word
    call probe_id_ports
    call lock_board
    call emit_summary
    xor al, al
    cmp word [fail_count], 0
    je .exit
    mov al, 1
.exit:
    jmp common_exit

; Before anything is written: does the board drive its data ports?
probe_pre_ports:
    pusha
    mov si, pre_ports
    mov cx, PRE_PORT_COUNT
.next:
    lodsw
    mov dx, ax
    push si
    call probe_port
    mov ax, dx
    mov di, param_buf
    call hex16_to_mem
    mov word [row_name], s_pre
    mov word [row_param], param_buf
    call check_begin
    call lb_probe_result
    call lb_emit
    pop si
    loop .next
    popa
    ret

; Wake-up as AGS_CLGD.COM (plan, section 12).
wake_board:
    pusha
    mov dx, WSRE_SETUP
    mov al, 0x18
    out dx, al
    mov dx, WSRE_MISC
    mov al, 0x01
    out dx, al
    mov dx, WSRE_SETUP
    mov al, 0x08
    out dx, al
    mov dx, WSRE_MISC
    mov al, 0xe3
    out dx, al
    mov dx, WSRE_STATUS
    xor al, al
    out dx, al
    popa
    ret

; AH = value: SR06 written by bytes, AL = read back.
sr06_byte:
    push dx
    mov dx, WSRE_SEQ_INDEX
    mov al, 0x06
    out dx, al
    inc dh
    mov al, ah
    out dx, al
    in al, dx
    pop dx
    ret

; AH = value: SR06 written by one word OUT, AL = read back by byte.
sr06_word:
    push dx
    mov dx, WSRE_SEQ_INDEX
    mov al, 0x06
    out dx, ax
    out dx, al
    inc dh
    in al, dx
    pop dx
    ret

; Byte path decides whether a board is there.
detect_board:
    pusha
    mov ah, 0x12
    call sr06_byte
    mov [sr06_unlocked], al
    mov ah, 0x00
    call sr06_byte
    mov [sr06_locked], al
    cmp byte [sr06_unlocked], 0x12
    jne .done
    cmp byte [sr06_locked], 0x0f
    jne .done
    mov byte [board_found], 1
.done:
    popa
    ret

emit_sr06_byte:
    pusha
    mov word [row_name], s_sr06b
    mov word [row_param], s_v12
    call check_begin
    mov al, [sr06_unlocked]
    mov ah, 0x12
    call lb_expect
    call lb_emit
    mov word [row_param], s_v00
    call check_begin
    mov al, [sr06_locked]
    mov ah, 0x0f
    call lb_expect
    call lb_emit
    popa
    ret

test_sr06_word:
    pusha
    mov ah, 0x12
    call sr06_word
    mov bl, al
    mov ah, 0x00
    call sr06_word
    mov bh, al
    mov word [row_name], s_sr06w
    mov word [row_param], s_v12
    call check_begin
    mov al, bl
    mov ah, 0x12
    call lb_expect
    call lb_emit
    mov word [row_param], s_v00
    call check_begin
    mov al, bh
    mov ah, 0x0f
    call lb_expect
    call lb_emit
    popa
    ret

; CR27 with the extensions unlocked; kept in cr27_str for every later
; line. Leaves SR06 unlocked.
read_cr27:
    pusha
    mov ah, 0x12
    call sr06_byte
    mov dx, WSRE_CRTC_INDEX
    mov al, 0x27
    out dx, al
    inc dh
    in al, dx
    mov [cr27_value], al
    mov ah, al
    shr al, 4
    call nibble_ascii
    mov [cr27_str], al
    mov al, ah
    call nibble_ascii
    mov [cr27_str + 1], al
    popa
    ret

nibble_ascii:
    and al, 0x0f
    add al, '0'
    cmp al, '9'
    jbe .done
    add al, 'A' - '0' - 10
.done:
    ret

emit_cr27:
    pusha
    mov word [row_name], s_cr27
    mov word [row_param], s_id
    call check_begin
    mov al, [cr27_value]
    call lb_hex8
    call lb_emit
    popa
    ret

; BL = GR index, AH = value: byte writes, AL = read back.
gr_byte:
    push dx
    mov dx, WSRE_GC_INDEX
    mov al, bl
    out dx, al
    inc dh
    mov al, ah
    out dx, al
    dec dh
    mov al, bl
    out dx, al
    inc dh
    in al, dx
    pop dx
    ret

; BL = GR index, AH = value: one word OUT, AL = read back by byte.
gr_word:
    push dx
    mov dx, WSRE_GC_INDEX
    mov al, bl
    out dx, ax
    out dx, al
    inc dh
    in al, dx
    pop dx
    ret

; SI = row name, BL = index, DI -> gr_byte or gr_word: 55h and AAh rows,
; then the register back to 00h by bytes.
gr_pair:
    pusha
    mov [row_name], si
    mov word [row_param], s_v55
    mov ah, 0x55
    call di
    push ax
    call check_begin
    pop ax
    mov ah, 0x55
    call lb_expect
    call lb_emit
    mov word [row_param], s_vaa
    mov ah, 0xaa
    call di
    push ax
    call check_begin
    pop ax
    mov ah, 0xaa
    call lb_expect
    call lb_emit
    mov ah, 0x00
    call gr_byte
    popa
    ret

test_gr_byte:
    pusha
    mov di, gr_byte
    mov si, s_gr09b
    mov bl, 0x09
    call gr_pair
    mov si, s_gr0ab
    mov bl, 0x0a
    call gr_pair
    popa
    ret

test_gr_word:
    pusha
    mov di, gr_word
    mov si, s_gr09w
    mov bl, 0x09
    call gr_pair
    mov si, s_gr0aw
    mov bl, 0x0a
    call gr_pair
    popa
    ret

; Board ports, recorded only.
probe_id_ports:
    pusha
    mov si, id_ports
    mov cx, ID_PORT_COUNT
.next:
    lodsw
    mov dx, ax
    call probe_and_emit
    loop .next
    popa
    ret

; Leave the extensions locked (SR06 = 00h).
lock_board:
    push ax
    mov ah, 0x00
    call sr06_byte
    pop ax
    ret

cr27_value  db 0
