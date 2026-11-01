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
; W1PORT.COM (stage W1, docs/modernization/wsr-e-pseudo-sprite-plan.md):
; the PC-88VA side of the WSR-E path, with or without the board. MS-DOS
; 4.0, V3 mode. It never writes a WSR-E port.
;
;   R0      calibration, LOOP x1024 in 600 VRTCs, three runs
;   P       port probes: does anything drive the port when it is read?
;           00E0h, 00E1h, 00E8h are the low bytes of the WSR-E ports;
;           the 4nE0h/4nE1h/46E8h rows show what the board (if present)
;           answers
;   B152    port 152h: word read, low byte written back, area 0 released
;           to the slot (1000b) and read back, original restored;
;           interrupts off meanwhile
;   SUM     PASS when 00E0h/00E1h/00E8h are not DRIVEN and port 152h
;           reads back what was written
;
; Exit code: 0 pass, 1 fail, 2 no VRTC interrupt.

    bits 16
    cpu 186
    org 0x100

    jmp main

%include "wsre.inc"

stage_str   db 'W1', 0
prog_str    db 'W1PORT', 0

; Ports whose low byte the WSR-E uses (must not be driven by the VA in
; V3 mode), then the WSR-E ports themselves.
low_ports       dw 0x00e0, 0x00e1, 0x00e8
LOW_PORT_COUNT  equ ($ - low_ports) / 2
board_ports     dw WSRE_ATTR, WSRE_BOARD_40E1, WSRE_SEQ_INDEX, WSRE_SEQ_DATA
                dw WSRE_SETUP, WSRE_GC_INDEX, WSRE_GC_DATA, WSRE_CRTC_INDEX
                dw WSRE_CRTC_DATA, WSRE_STATUS, WSRE_BOARD_42E1
                dw WSRE_ID_51E1, WSRE_ID_59E1, WSRE_ID_5BE1
BOARD_PORT_COUNT equ ($ - board_ports) / 2

s_b152      db 'B152', 0
s_read      db 'READ', 0
s_same      db 'SAME', 0
s_slot      db 'SLOT', 0
s_restore   db 'RESTORE', 0

rom_bank_word   dw 0
rom_bank_same   db 0
rom_bank_slot   db 0
rom_bank_back   db 0
rom_bank_write  db 0

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
    call run_r0
    call probe_low_ports
    call probe_board_ports
    call test_rom_bank
    call emit_summary
    xor al, al
    cmp word [fail_count], 0
    je .exit
    mov al, 1
.exit:
    jmp common_exit

; Low-byte partners: a DRIVEN result is a failure.
probe_low_ports:
    pusha
    mov si, low_ports
    mov cx, LOW_PORT_COUNT
.next:
    lodsw
    mov dx, ax
    call probe_and_emit
    cmp al, 1
    jne .ok
    inc word [fail_count]
.ok:
    loop .next
    popa
    ret

; WSR-E ports: recorded only.
probe_board_ports:
    pusha
    mov si, board_ports
    mov cx, BOARD_PORT_COUNT
.next:
    lodsw
    mov dx, ax
    call probe_and_emit
    loop .next
    popa
    ret

; Port 152h. Only the low nibble (ROM area 0) is compared: the VA and VA2
; report the other bits differently.
test_rom_bank:
    pusha
    mov dx, PORT_ROM_BANK
    in ax, dx
    mov [rom_bank_word], ax
    cli
    mov al, [rom_bank_word]
    out dx, al
    in al, dx
    mov [rom_bank_same], al
    mov al, [rom_bank_word]
    and al, 0xf0
    or al, 0x08
    mov [rom_bank_write], al
    out dx, al
    in al, dx
    mov [rom_bank_slot], al
    mov al, [rom_bank_word]
    out dx, al
    in al, dx
    mov [rom_bank_back], al
    sti

    mov word [row_name], s_b152
    mov word [row_param], s_read
    call check_begin
    mov ax, [rom_bank_word]
    call lb_hex16
    call lb_emit

    mov word [row_param], s_same
    call check_begin
    mov al, [rom_bank_same]
    and al, 0x0f
    mov ah, [rom_bank_word]
    and ah, 0x0f
    call lb_expect
    call lb_emit

    mov word [row_param], s_slot
    call check_begin
    mov al, [rom_bank_slot]
    and al, 0x0f
    mov ah, 0x08
    call lb_expect
    call lb_emit

    mov word [row_param], s_restore
    call check_begin
    mov al, [rom_bank_back]
    and al, 0x0f
    mov ah, [rom_bank_word]
    and ah, 0x0f
    call lb_expect
    call lb_emit
    popa
    ret
