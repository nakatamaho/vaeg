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
; HFV2.COM (M149): display-enable sweep for the HyPER-FRAME+. The board
; is a pass-through/DAC mux (74HC4053 x2) whose select bit is unknown;
; with CN2 (main RGB input) unconnected the monitor shows "no signal"
; even after a successful draw. Writes bounded candidate values to the
; control ports D0 and EE, holding each for 8 seconds so the
; monitor's behaviour can be watched and timed. VA, MS-DOS 4.0.
;
; Sequence: PRE probe, the JPGVHF initialisation without its PC-98 BIOS
; call (EE=C0, D0=A5, D3=00, 8 points at Y=0191, D0=A1, 32 points at
; Y=0193, D0=E1), colour bars on rows 0-63 (eight bars, eight rows
; each), then:
;   phase 1, D3 = 01 02 03 04 08 10 20 40 80 FF with D0=E1 (8 s each)
;   phase 2, D0 = A1 A3 A5 A9 B1 E1 E5 21 61 FF with D3=01 (8 s each)
; D3 was never varied before: the viewer only writes D3=00 once, and the
; earlier D0/EE sweeps (two runs, 8 s holds, no monitor change) left it
; at 00 throughout.
; Each write is logged with the DOS clock (the VA's clock starts at
; 2000-01-31 00:00:00 at power-on, so the log times match a stopwatch).
; The board is left at D0=E1, EE=C0 (displaying attempt); photograph the
; monitor, then power the machine off. /STOP writes only the stop
; sequence D0=00, EE=00.
;
; Exit: 0 sweep done; 1 option; 2 machine; 4 log write failed.

    bits 16
    cpu 186
    org 0x100

    jmp main

%define LOG_NAME 'A:\HFLOG.TXT'
%define HDR_TAG '#HFVA,'
%include "vaprobe.inc"

stage_str   db 'V3', 0
prog_str    db 'HFV3', 0
opt_stop    db 0
opt_table   dw s_o_stop, opt_stop
            dw 0
s_o_stop    db '/STOP', 0
usage_str   db 'HFV3 (M149) [/STOP]: HyPER-FRAME+ D3 sweep.', 13, 10, 0

pre_ports   dw 0x00d0, 0x00d3, 0x00d8, 0x00da, 0x00dc, 0x00ee
NPRE        equ ($ - pre_ports) / 2

d3_values   db 0x01, 0x02, 0x03, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0xff
D3_COUNT    equ $ - d3_values
d0_values   db 0xa1, 0xa3, 0xa5, 0xa9, 0xb1, 0xe1, 0xe5, 0x21, 0x61, 0xff
D0_COUNT    equ $ - d0_values
ee_values   db 0xc0, 0x40, 0x80, 0x00, 0xff
EE_COUNT    equ $ - ee_values

bars        db 0xff, 0xff, 0xff,  0xff, 0xff, 0x00,  0x00, 0xff, 0xff
            db 0x00, 0xff, 0x00,  0xff, 0x00, 0xff,  0xff, 0x00, 0x00
            db 0x00, 0x00, 0xff,  0x00, 0x00, 0x00

s_pre       db 'PRE', 0
s_init      db 'INIT', 0
s_init_txt  db 'EE=C0 D0=A5 D3=00 8xD8 D0=A1 32xDA D0=E1', 0
s_draw      db 'DRAW', 0
s_draw_txt  db 'rows 0-63 colour bars', 0
s_sweep     db 'SWEEP', 0
s_port_eq   db 'port=', 0
s_val_eq    db ' value=', 0
s_t_eq      db ' t=', 0
s_hold      db ' hold=8s', 0
s_done      db 'DONE', 0
s_done_txt  db 'left D0=E1 D3=01 EE=C0; photograph, then power off', 0
s_off       db 'OFF', 0
s_off_txt   db 'D0=00 EE=00', 0
s_sum       db 'SUM', 0
s_pass2     db 'DONE sweep; note any monitor change and its log time', 0
s_stop      db 'stopped', 0

main:
    push cs
    pop ds
    push cs
    pop es
    call common_start

    cmp byte [opt_stop], 0
    je .run
    mov dx, 0x00d0
    xor al, al
    out dx, al
    mov dx, 0x00ee
    out dx, al
    call lb_reset
    mov si, s_off
    call lb_tag
    mov si, s_off_txt
    call lb_str
    call lb_emit
    call lb_reset
    mov si, s_sum
    call lb_tag
    mov si, s_stop
    call lb_str
    call lb_emit
    xor al, al
    jmp common_exit

.run:
    call lb_reset
    mov si, s_pre
    call lb_tag
    mov si, pre_ports
    mov cx, NPRE
.pre:
    lodsw
    mov dx, ax
    call lb_port_probe
    loop .pre
    call lb_emit

    call lb_reset
    mov si, s_init
    call lb_tag
    call hf_init
    mov si, s_init_txt
    call lb_str
    call lb_emit

    call lb_reset
    mov si, s_draw
    call lb_tag
    call draw_small
    mov si, s_draw_txt
    call lb_str
    call lb_emit

    ; phase 1: D3 sweep with D0 = E1, EE = C0
    mov si, d3_values
    mov cx, D3_COUNT
    mov word [sweep_port], 0x00d3
.s0:
    push cx
    push si
    mov dx, 0x00d0
    mov al, 0xe1
    out dx, al
    lodsb
    call sweep_step
    pop si
    pop cx
    inc si
    loop .s0

    ; phase 2: D0 sweep with D3 = 01
    mov dx, 0x00d3
    mov al, 0x01
    out dx, al
    mov si, d0_values
    mov cx, D0_COUNT
    mov word [sweep_port], 0x00d0
.s1:
    push cx
    push si
    lodsb
    call sweep_step
    pop si
    pop cx
    inc si
    loop .s1

    ; leave in the active state for the photograph
    mov dx, 0x00d0
    mov al, 0xe1
    out dx, al
    mov dx, 0x00ee
    mov al, 0xc0
    out dx, al
    call lb_reset
    mov si, s_done
    call lb_tag
    mov si, s_done_txt
    call lb_str
    call lb_emit
    call lb_reset
    mov si, s_sum
    call lb_tag
    mov si, s_pass2
    call lb_str
    call lb_emit
    xor al, al
    jmp common_exit

; AL = value for [sweep_port]: log the write with the DOS time, write it,
; hold about 5 s watching nothing (the user watches the monitor).
sweep_step:
    pusha
    mov [cur_val], al
    call log_sweep
    mov dx, [sweep_port]
    mov al, [cur_val]
    out dx, al
    call wait5
    popa
    ret

log_sweep:
    pusha
    call lb_reset
    mov si, s_sweep
    call lb_tag
    mov si, s_port_eq
    call lb_str
    mov ax, [sweep_port]
    call lb_hex16
    mov si, s_val_eq
    call lb_str
    mov al, [cur_val]
    call lb_hex8
    mov si, s_t_eq
    call lb_str
    mov ah, 0x2c
    int 0x21
    push dx
    mov al, ch
    call lb_dec2
    mov al, ':'
    call lb_char
    pop dx
    push dx
    mov al, cl
    call lb_dec2
    mov al, ':'
    call lb_char
    pop dx
    mov al, dh
    call lb_dec2
    mov si, s_hold
    call lb_str
    call lb_emit
    popa
    ret

; BL = seconds to hold, watching the DOS clock's seconds field. The
; count lives in memory: INT 21h AH=2Ch returns CX and DX, so a register
; counter would be clobbered (the 1-second holds of the first run).
wait_secs:
    pusha
    mov byte [sec_cnt], 0
    mov ah, 0x2c
    int 0x21
    mov al, dh
    mov [last_sec], al
.hold:
    mov ah, 0x2c
    int 0x21
    mov al, dh
    cmp al, [last_sec]
    je .hold
    mov [last_sec], al
    inc byte [sec_cnt]
    mov al, [sec_cnt]
    cmp al, bl
    jb .hold
    popa
    ret

wait5:
    mov bl, 8
    call wait_secs
    ret

; CX:DX = DOS 2Ch output.
get_time:
    push ax
    mov ah, 0x2c
    int 0x21
    pop ax
    ret

; J1's initialisation without the BIOS call.
hf_init:
    pusha
    mov dx, 0x00ee
    mov al, 0xc0
    out dx, al
    mov dx, 0x00d0
    mov al, 0xa5
    out dx, al
    mov dx, 0x00d3
    xor al, al
    out dx, al
    mov bx, 0x0191
    mov cx, 0x0180
.p8:
    call sel_byte
    mov dx, 0x00d8
    xor al, al
    out dx, al
    inc cx
    cmp cx, 0x0188
    jb .p8
    mov dx, 0x00d0
    mov al, 0xa1
    out dx, al
    mov bx, 0x0193
    mov cx, 0x0080
.p32:
    call sel_byte
    mov dx, 0x00da
    xor al, al
    out dx, al
    inc cx
    cmp cx, 0x00a0
    jb .p32
    mov dx, 0x00d0
    mov al, 0xe1
    out dx, al
    popa
    ret

; CX = x, BX = y, byte coordinates in the P1 order (D7 D6 D5 D4).
sel_byte:
    push ax
    push dx
    mov dx, 0x00d7
    mov al, bh
    out dx, al
    dec dx
    mov al, bl
    out dx, al
    dec dx
    mov al, ch
    out dx, al
    dec dx
    mov al, cl
    out dx, al
    pop dx
    pop ax
    ret

; Colour bars on rows 0-63: eight bars, eight rows each.
draw_small:
    pusha
    xor bx, bx
.row:
    mov dx, 0x00d7
    xor al, al
    out dx, al
    mov dx, 0x00d6
    mov al, bl
    out dx, al
    xor cx, cx
    mov si, bars
.bar:
    mov di, 80
.px:
    call sel_byte
    mov dx, 0x00d8
    mov al, [si]
    out dx, al
    mov dx, 0x00da
    mov al, [si + 1]
    out dx, al
    mov dx, 0x00dc
    mov al, [si + 2]
    out dx, al
    inc cx
    dec di
    jnz .px
    add si, 3
    cmp cx, 640
    jb .bar
    inc bx
    cmp bx, 64
    jb .row
    popa
    ret

sweep_port  dw 0x00d0
cur_val     db 0
sec_cnt     db 0
last_sec    db 0
