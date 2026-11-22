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

; nasm -f bin -o PRNAUX.COM tests/va_prnaux.asm
bits 16
org 100h
    mov ax, 0f000h
    mov es, ax
    mov dx, va1
    cmp word [es:0fffeh], 0ffffh
    je .model
    mov dx, va91
    cmp word [es:0fffeh], 0fffdh
    je .model
    mov dx, va23
.model:
    mov ah, 9
    int 21h

    mov dx, 2
    mov ah, 8                   ; bound printer wait to two seconds
    int 89h
    xor bx, bx
.print:
    mov dx, bx
    mov ah, 3                   ; raw byte (AH=1 translates TAB)
    push bx
    int 89h
    pop bx
    mov dx, printer_fail
    jc fail
    inc bx
    cmp bx, 256
    jb .print
    xor dx, dx                 ; restore the default printer timeout
    mov ah, 8
    int 89h

    xor ax, ax
    mov dx, 4e37h              ; x16, 8N1, RTS/DTR, RX/TX enabled
    int 8ah
    mov ah, 8
    mov dh, 8                  ; 9600 bps
    int 8ah
    mov dx, serial_fail
    jc fail
    mov si, pattern
.exchange:
    mov dx, 20                 ; two-second receive timeout
    mov ah, 0dh
    push si
    int 8ah
    pop si
    jc .serial_error
    cmp dl, [si]
    jne .serial_error
    xor dh, dh
    mov ah, 1
    push si
    int 8ah
    pop si
    jc .serial_error
    inc si
    cmp si, pattern_end
    jb .exchange
    mov ah, 0eh
    int 8ah
    mov dx, passed
    mov ah, 9
    int 21h
    mov ax, 4c00h
    int 21h
.serial_error:
    mov ah, 0eh
    int 8ah
    mov dx, serial_fail
fail:
    mov ah, 9
    int 21h
    mov ax, 4c01h
    int 21h

pattern db 'VArx-0123456789'
pattern_end:
va1 db 'PC-88VA', 13, 10, '$'
va91 db 'PC-88VA + PC-88VA-91', 13, 10, '$'
va23 db 'PC-88VA2/VA3', 13, 10, '$'
passed db 'PRNAUX PASS', 13, 10, '$'
printer_fail db 'PRNAUX FAIL: printer', 13, 10, '$'
serial_fail db 'PRNAUX FAIL: serial', 13, 10, '$'
