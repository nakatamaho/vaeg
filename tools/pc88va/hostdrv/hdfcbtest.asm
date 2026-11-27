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

cpu 8086
bits 16
org 0x100

; Synthetic fixture for DOS 4.0 VA, FCBS=4,0. Run from the local drive
; with its FCB cache otherwise unused. Pass the HOSTDRV drive letter.
; Root: ERR.DAT plus FCB1.DAT through FCB5.DAT, starting with 128
; bytes of 5Ah each. FCB5.DAT also holds 128 bytes of 59h at offset 10000h.
; Inject DOS 31 once for READ of ERR.DAT at position zero. Do not inject
; a transport error: the live session is needed for the recovery checks.

%define FCB_SIZE 37
%define FCB_SFN 0x18
%define FCB_SERIAL 0x1c

start:
    cmp byte [0x80], 2              ; space and one drive letter
    jne usage
    cmp byte [0x81], ' '
    jne usage
    mov al, [0x82]
    and al, 0xdf
    sub al, 'A'-1
    cmp al, 1
    jb usage
    cmp al, 26
    ja usage
    mov si, error_fcb
    mov cx, 6
.set_drives:
    mov [si], al
    add si, FCB_SIZE
    loop .set_drives
    mov [find_fcb], al

    mov ax, 0x3000
    int 0x21
    cmp ax, 4
    jne usage
    mov ah, 0x52
    int 0x21
    les di, [es:bx+0x1a]
    cmp word [es:di+4], 4
    jne usage

    mov ah, 0x1a
    mov dx, transfer
    int 0x21
    mov byte [failure], 1
    mov dx, error_fcb
    call open_fcb

    inc byte [failure]
    mov dx, error_fcb
    mov ah, 0x14
    int 0x21
    cmp al, 1                      ; FCB-record error, also used for EOF
    jne failed
    xor bx, bx
    mov ax, 0x5900
    int 0x21
    cmp ax, 31                     ; proves it was not fabricated EOF
    jne failed
    cmp byte [transfer], 0
    jne failed

    inc byte [failure]
    mov dx, error_fcb
    mov ah, 0x15
    int 0x21
    cmp al, 1
    jne failed
    xor bx, bx
    mov ax, 0x5900
    int 0x21
    cmp ax, 5                      ; DOS might deny before INT 2Fh/1109h
    jne failed
    mov dx, error_fcb
    call close_fcb

    inc byte [failure]
    mov word [next_fcb], working_fcbs
.open_next:
    mov dx, [next_fcb]
    call open_fcb                  ; fifth allocation must evict an SFT
    add word [next_fcb], FCB_SIZE
    cmp word [next_fcb], fcbs_end
    jb .open_next

    inc byte [failure]
    mov al, [last_fcb+FCB_SFN]
    mov si, working_fcbs
.find_victim:
    cmp al, [si+FCB_SFN]
    je .victim
    add si, FCB_SIZE
    cmp si, last_fcb
    jb .find_victim
    jmp failed
.victim:
    mov ax, [last_fcb+FCB_SERIAL]
    test ax, ax
    jz failed
    cmp ax, [si+FCB_SERIAL]
    je failed
    mov dx, si
    call close_fcb                 ; stale FCB close must not touch replacement

    inc byte [failure]
    mov dx, last_fcb
    mov ah, 0x14
    int 0x21
    test al, al
    jnz failed
    mov si, transfer
    mov cx, 128
.check_byte:
    cmp byte [si], 0x5a
    jne failed
    inc si
    loop .check_byte
    mov word [last_fcb+33], 512    ; 512 * 128 = 10000h byte offset
    mov dx, last_fcb
    mov ah, 21h                   ; random read
    int 21h
    test al, al
    jnz failed
    mov si, transfer
    mov cx, 128
.check_random_byte:
    cmp byte [si], 59h
    jne failed
    inc si
    loop .check_random_byte

    inc byte [failure]
    mov word [next_fcb], working_fcbs
.close_next:
    mov dx, [next_fcb]
    call close_fcb
    add word [next_fcb], FCB_SIZE
    cmp word [next_fcb], fcbs_end
    jb .close_next
    inc byte [failure]
    mov dx, find_fcb
    mov ah, 11h
    int 21h
    test al, al
    jnz failed
    cmp byte [transfer+4], '1'
    jne failed
    mov dx, find_fcb
    mov ah, 12h
    int 21h
    test al, al
    jnz failed
    cmp byte [transfer+4], '2'
    jne failed
    inc byte [failure]
    mov dx, last_fcb
    call open_fcb                 ; leave one open for DOS process exit
    mov dx, msg_pass
    mov ah, 9
    int 0x21
    mov ax, 0x4c00
    int 0x21

open_fcb:
    mov ah, 0x0f
    jmp short checked_fcb
close_fcb:
    mov ah, 0x10
checked_fcb:
    int 0x21
    test al, al
    jnz failed
    ret

usage:
    mov dx, msg_usage
    mov ah, 9
    int 0x21
    mov ax, 0x4cff
    int 0x21
failed:
    mov al, [failure]
    add al, '0'
    mov [fail_code], al
    mov dx, msg_fail
    mov ah, 9
    int 0x21
    mov ah, 0x4c
    mov al, [failure]
    int 0x21

%macro fcb 1
    db 0, %1
    times FCB_SIZE-12 db 0
%endmacro
error_fcb: fcb 'ERR     DAT'
working_fcbs: fcb 'FCB1    DAT'
    fcb 'FCB2    DAT'
    fcb 'FCB3    DAT'
    fcb 'FCB4    DAT'
last_fcb: fcb 'FCB5    DAT'
fcbs_end:
find_fcb: fcb 'FCB?    DAT'
next_fcb dw 0
failure db 0
msg_pass db 'HDFCBTST PASS',13,10,'$'
msg_fail db 'HDFCBTST FAIL '
fail_code db '0',13,10,'$'
msg_usage db 'HDFCBTST drive-letter (DOS 4.0 VA, FCBS=4,0)',13,10,'$'
transfer times 128 db 0
