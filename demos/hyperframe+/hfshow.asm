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
; HFSHOW.COM (M149): show a 640x400 24-bit picture on a Digital Arts
; HyPER-FRAME+ from a PC-88VA (V3 mode, MS-DOS 4.0), through I/O only.
;
;   HFSHOW [file] [/STAY]
;
; file   640 x 400 x 3 bytes, R G B per pixel, rows top to bottom
;        (default TETRA.RGB, the ray-traced sample of raytrace.py)
; /STAY  leave the board displaying when the program ends
;
; The board is initialised as the JPGVHF viewer does (without its PC-98
; BIOS call): EE=C0, D0=A5, D3=00, 8 points at Y=0191, D0=A1, 32 points at
; Y=0193, D0=E1. Each row is read from the file (1920 bytes), Y is
; selected once (D7, D6), and per pixel X (D5, D4) then R, G, B (D8, DA,
; DC), as the viewer and the M149 probes do. After the picture the program
; waits for a key and writes the viewer's stop sequence D0=00, EE=00, so
; the screen returns to the main unit (unless /STAY).
;
; Exit: 0 shown; 1 option; 2 file not found or short.

    bits 16
    cpu 186
    org 0x100

    jmp main

ROW_BYTES   equ 640 * 3

msg_usage   db 'HFSHOW (M149) [file] [/STAY]: 640x400x24 RGB picture to the HyPER-FRAME+.', 13, 10, '$'
msg_open    db 'HFSHOW: cannot open the picture file.', 13, 10, '$'
msg_short   db 'HFSHOW: the picture file is shorter than 768000 bytes.', 13, 10, '$'
msg_init    db 'HFSHOW: board initialised, sending 640x400 ...', 13, 10, '$'
msg_done    db 'HFSHOW: done. Press a key to return to the main screen.', 13, 10, '$'
msg_stay    db 'HFSHOW: done; the board keeps displaying (/STAY).', 13, 10, '$'
msg_stop    db 'HFSHOW: stopped (D0=00 EE=00).', 13, 10, '$'
def_name    db 'TETRA.RGB', 0
s_stay      db '/STAY'

opt_stay    db 0
handle      dw 0
fname       times 80 db 0

main:
    push cs
    pop ds
    push cs
    pop es
    cld
    call parse
    jnc .args
    mov dx, msg_usage
    mov ah, 0x09
    int 0x21
    mov ax, 0x4c01
    int 0x21
.args:
    mov dx, fname
    mov ax, 0x3d00
    int 0x21
    jnc .opened
    mov dx, msg_open
    mov ah, 0x09
    int 0x21
    mov ax, 0x4c02
    int 0x21
.opened:
    mov [handle], ax
    call hf_init
    mov dx, msg_init
    mov ah, 0x09
    int 0x21

    xor bx, bx                      ; y
.row:
    push bx
    mov bx, [handle]
    mov cx, ROW_BYTES
    mov dx, rowbuf
    mov ah, 0x3f
    int 0x21
    pop bx
    jc .short
    cmp ax, ROW_BYTES
    jne .short
    call send_row
    inc bx
    cmp bx, 400
    jb .row

    mov bx, [handle]
    mov ah, 0x3e
    int 0x21
    cmp byte [opt_stay], 0
    je .wait
    mov dx, msg_stay
    mov ah, 0x09
    int 0x21
    mov ax, 0x4c00
    int 0x21
.wait:
    mov dx, msg_done
    mov ah, 0x09
    int 0x21
    mov ah, 0x08
    int 0x21
    call hf_stop
    mov dx, msg_stop
    mov ah, 0x09
    int 0x21
    mov ax, 0x4c00
    int 0x21
.short:
    mov bx, [handle]
    mov ah, 0x3e
    int 0x21
    call hf_stop
    mov dx, msg_short
    mov ah, 0x09
    int 0x21
    mov ax, 0x4c02
    int 0x21

; BX = y: rowbuf (640 R G B triples) to row y.
send_row:
    pusha
    mov dx, 0x00d7
    mov al, bh
    out dx, al
    dec dx
    mov al, bl
    out dx, al
    mov si, rowbuf
    xor cx, cx
.px:
    mov dx, 0x00d5
    mov al, ch
    out dx, al
    dec dx
    mov al, cl
    out dx, al
    lodsb
    mov dx, 0x00d8
    out dx, al
    lodsb
    mov dx, 0x00da
    out dx, al
    lodsb
    mov dx, 0x00dc
    out dx, al
    inc cx
    cmp cx, 640
    jb .px
    popa
    ret

; CX = x, BX = y in the paper's byte order (D7 D6 D5 D4).
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

; The JPGVHF viewer's initialisation without its PC-98 BIOS call.
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

; The viewer's stop sequence.
hf_stop:
    push ax
    push dx
    mov dx, 0x00d0
    xor al, al
    out dx, al
    mov dx, 0x00ee
    out dx, al
    pop dx
    pop ax
    ret

; Command tail: optional file name and /STAY. CF on a bad option.
parse:
    pusha
    mov si, def_name
    mov di, fname
.cp:
    lodsb
    stosb
    or al, al
    jnz .cp
    mov si, 0x81
    mov cl, [0x80]
    xor ch, ch
    mov bx, si
    add bx, cx                      ; end of the tail
.skip:
    cmp si, bx
    jae .ok
    mov al, [si]
    cmp al, ' '
    je .blank
    cmp al, 9
    je .blank
    cmp al, 13
    je .ok
    cmp al, '/'
    je .opt
    ; a file name up to the next blank
    mov di, fname
    mov cx, 79
.name:
    cmp si, bx
    jae .nend
    mov al, [si]
    cmp al, ' '
    je .nend
    cmp al, 9
    je .nend
    cmp al, 13
    je .nend
    mov [di], al
    inc si
    inc di
    loop .name
.nend:
    mov byte [di], 0
    jmp .skip
.blank:
    inc si
    jmp .skip
.opt:
    ; /STAY, case-insensitive
    push si
    mov di, s_stay
    mov cx, 5
.cmp:
    cmp si, bx
    jae .bad_pop
    mov al, [si]
    cmp al, 'a'
    jb .up
    cmp al, 'z'
    ja .up
    sub al, 0x20
.up:
    cmp al, [di]
    jne .bad_pop
    inc si
    inc di
    loop .cmp
    add sp, 2
    mov byte [opt_stay], 1
    jmp .skip
.bad_pop:
    pop si
    popa
    stc
    ret
.ok:
    popa
    clc
    ret

rowbuf:
