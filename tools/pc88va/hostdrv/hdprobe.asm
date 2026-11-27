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

%macro text 1
    mov dx, %1
    call puts
%endmacro

start:
    mov ax, 0x3000
    int 0x21
    mov [version], ax
    text msg_dos
    mov ax, [version]
    call hexword               ; AH=minor, AL=major
    cmp word [version], 4
    jne .unsupported

    mov ax, 0x1100
    stc                        ; expose whether the handler clears carry
    int 0x2f
    pushf
    push cs
    pop ds
    pop word [flags]
    mov [result], ax
    text msg_1100
    call status

    mov ax, 0x5d06
    mov dx, dpl                ; 5D06 consumes UID/PID even on a query
    int 0x21
    pushf
    push ds
    push cs
    pop ds
    pop word [segval]
    pop word [flags]
    mov [offval], si
    mov [inlen], cx
    mov [alwayslen], dx
    mov [result], ax
    text msg_5d06
    call status
    test byte [flags], 1
    jnz .swap_table
    text msg_ptr
    call pointer
    text msg_cx
    mov ax, [inlen]
    call hexword
    text msg_dx
    mov ax, [alwayslen]
    call hexword

.swap_table:
    mov ax, 0x5d0b
    int 0x21
    pushf
    push ds
    push cs
    pop ds
    pop word [segval]
    pop word [flags]
    mov [offval], si
    mov [result], ax
    text msg_5d0b
    call status
    test byte [flags], 1
    jnz .lists
    text msg_ptr
    call pointer

.lists:
    mov ah, 0x52
    int 0x21
    mov [segval], es
    mov [offval], bx
    mov ax, [es:bx+0x16]       ; SYSINITVAR.CDSADDR (offset:segment)
    mov [cds_off], ax
    mov ax, [es:bx+0x18]
    mov [cds_seg], ax
    xor ax, ax
    mov al, [es:bx+0x21]       ; SYSINITVAR.CDSCOUNT
    mov [count], ax
    text msg_lists
    call pointer
    text msg_cds
    mov ax, [cds_seg]
    mov [segval], ax
    mov ax, [cds_off]
    mov [offval], ax
    call pointer
    text msg_stride
    mov ax, 0x58               ; source-derived, not inferred from RAM
    call hexword
    text msg_count
    mov ax, [count]
    call hexword
    text msg_end
    mov ax, 0x4c00
    int 0x21

.unsupported:
    text msg_unsupported
    mov ax, 0x4c01
    int 0x21

status:
    mov ax, [result]
    call hexword
    text msg_cf
    mov ax, [flags]
    and ax, 1
    jmp hexword

pointer:
    mov ax, [segval]
    call hexword
    mov dl, ':'
    call putc
    mov ax, [offval]
    jmp hexword

hexword:
    push bx
    push cx
    mov bx, ax
    mov cx, 4
.next:
    rol bx, 1
    rol bx, 1
    rol bx, 1
    rol bx, 1
    mov dl, bl
    and dl, 15
    add dl, '0'
    cmp dl, '9'
    jbe .digit
    add dl, 'A'-'9'-1
.digit:
    call putc
    loop .next
    pop cx
    pop bx
    ret

putc:
    mov ah, 2
    int 0x21
    ret

puts:
    mov ah, 9
    int 0x21
    ret

msg_dos db 'HDPROBE DOS (minor:major hex)=','$'
msg_1100 db 13,10,'1100 AX=','$'
msg_5d06 db 13,10,'5D06 AX=','$'
msg_5d0b db 13,10,'5D0B AX=','$'
msg_cf db ' CF=','$'
msg_ptr db ' PTR=','$'
msg_cx db ' CX=','$'
msg_dx db ' DX=','$'
msg_lists db 13,10,'LISTS=','$'
msg_cds db ' CDS=','$'
msg_stride db ' STRIDE(source)=','$'
msg_count db ' LASTDRIVE(count)=','$'
msg_end db 13,10,'HDPROBE END',13,10,'$'
msg_unsupported db 13,10,'Unsupported DOS version',13,10,'$'

version dw 0
flags dw 0
result dw 0
segval dw 0
offval dw 0
inlen dw 0
alwayslen dw 0
cds_seg dw 0
cds_off dw 0
count dw 0
dpl times 11 dw 0
