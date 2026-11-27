; Copyright (c) 2026 Nakata Maho
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions are met:
; 1. Redistributions of source code must retain the above copyright notice,
;    this list of conditions and the following disclaimer.
; 2. Redistributions in binary form must reproduce the above copyright notice,
;    this list of conditions and the following disclaimer in the documentation
;    and/or other materials provided with the distribution.
;
; THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
; IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
; OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
; IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
; INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
; NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
; DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
; THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
; (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
; THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

; HDRES.COM: transient, no hook installation and no port access.
; Synthetic interrupt frames exercise the same entry and IRET as INT 2Fh.
bits 16
cpu 8086
org 100h

%macro invoke 0
    pushf
    push cs
    call hd_interrupt
%endmacro

    jmp start
%include 'hdresident.inc'

start:
    mov [hd_previous], word previous
    mov [hd_previous+2], cs
    mov [caller_sp], sp
    mov byte [stage], '1'
    mov ax, 111eh                  ; unowned call
    mov bx, 1234h
    std
    stc
    invoke
    jnc failure
    mov dx, ds
    mov si, cs
    cmp dx, si
    jne failure
    pushf
    pop dx
    test dx, 0400h
    jz failure
    cld
    cmp ax, 111eh
    jne failure
    cmp bx, 1234h
    jne failure
    cmp byte [chains], 1
    jne failure
    cmp sp, [caller_sp]
    jne failure

    mov byte [stage], '2'
    mov dx, 3412h
    push dx                       ; DOS caller-owned argument
    mov ax, 1108h
    mov cx, 19
    std
    stc
    invoke
    jc failure
    pushf
    pop dx
    test dx, 0400h
    jz failure
    cld
    pop dx
    cmp dx, 3412h
    jne failure
    cmp cx, 3
    jne failure
    cmp sp, [caller_sp]
    jne failure
    cmp byte [hd_resident_busy], 0
    jne failure
    cmp byte [workers], 1
    jne failure

    mov byte [stage], '3'
    mov byte [hd_resident_busy], 1
    mov ax, 1108h
    mov cx, 19
    clc
    invoke
    jnc failure
    cmp ax, 31
    jne failure
    cmp cx, 0
    jne failure
    cmp byte [workers], 1
    jne failure
    cmp byte [hd_resident_busy], 1
    jne failure

    mov byte [stage], '4'
    mov byte [hd_resident_busy], 0
    mov ax, 111dh                 ; owned cleanup, then chain broadcast
    mov bx, 1234h
    std
    stc
    invoke
    jnc failure
    cld
    cmp ax, 111dh
    jne failure
    cmp bx, 1234h
    jne failure
    cmp byte [chains], 2
    jne failure
    cmp byte [workers], 2
    jne failure
    cmp byte [hd_resident_busy], 0
    jne failure
    cmp sp, [caller_sp]
    jne failure
    mov dx, passed
    xor bx, bx
    jmp emit

hd_classify:
    mov ax, [es:bp+HD_R_AX]
    cmp ax, 1108h
    je .owned
    cmp ax, 111dh
    je .owned
    stc
    jmp .scratch
.owned:
    clc
.scratch:
    ; Classifiers can load unrelated DOS segments and use BP as scratch.
    mov dx, 0
    mov ds, dx
    mov es, dx
    mov bp, dx
    ret

hd_dispatch:
    cmp sp, hd_resident_stack_top-6
    jne failure
    mov dx, ss
    mov cx, cs
    cmp dx, cx
    jne failure
    pushf
    pop dx
    test dx, 0700h                 ; DF, IF and TF must be clear
    jnz failure
    cmp byte [hd_resident_busy], 1
    jne failure
    inc byte [workers]
    cmp ax, 111dh
    je .chain
    cmp word [es:bp+HD_R_ARG], 3412h
    jne failure
    cmp word [es:bp+HD_R_CX], 19
    jne failure
    mov word [es:bp+HD_R_CX], 3
    and word [es:bp+HD_R_FLAGS], 0fffeh
    clc
    ret
.chain:
    stc
    ret

previous:
    jnc failure
    push dx
    pushf
    pop dx
    test dx, 0400h
    pop dx
    jz failure
    inc byte [cs:chains]
    iret

failure:
    push cs
    pop ds
    push cs
    pop ss
    mov sp, [caller_sp]
    cld
    mov dx, failed
    mov bl, 1
emit:
    push bx
    mov ah, 9
    int 21h
    pop ax
    mov ah, 4ch
    int 21h

caller_sp dw 0
workers db 0
chains db 0
passed db 'HDRES PASS',13,10,'$'
failed db 'HDRES FAIL '
stage db '0',13,10,'$'
