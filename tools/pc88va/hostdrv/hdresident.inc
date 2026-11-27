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

; Register image on the caller stack, addressed via ES:BP. Only output
; fields and the carry bit in HD_R_FLAGS may be changed on an owned call.
%define HD_R_ES       0
%define HD_R_DS       2
%define HD_R_BP       4
%define HD_R_DI       6
%define HD_R_SI       8
%define HD_R_DX      10
%define HD_R_CX      12
%define HD_R_BX      14
%define HD_R_AX      16
%define HD_R_FLAGS   24
%define HD_R_ARG     26

%macro hd_restore_registers 1
    pop es
    pop ds
    pop bp
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
%if %1
    popf
%else
    add sp, 2
%endif
%endmacro

; Supply hd_classify (DS=CS, ES:BP=frame, CF set if unowned). It runs on
; the caller stack and must be bounded, read-only, and reentrant, with no
; hardware/DOS calls. For owned calls pass its AX action to hd_dispatch.
; hd_dispatch runs on the private stack, also with DS=CS and ES:BP=frame.
; CF clear consumes the call; CF set chains a broadcast with frame intact.
; Neither helper may enable IRQs or invoke a DOS, firmware or guest callback.
; SI on dispatch is the saved TF mask: nonzero is permitted only for CLOSE,
; which must retire the reference locally without issuing any port I/O.
hd_interrupt:
    pushf
    cli
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push bp
    push ds
    push es
    mov bp, sp
    push ss
    pop es
    push cs
    pop ds
    cld
    call hd_classify
    mov bp, sp
    push cs
    pop ds
    push ss
    pop es
    jc .chain
    cmp byte [hd_resident_busy], 0
    jne .refuse
    mov si, [es:bp+HD_R_FLAGS]
    and si, 0100h
    jz .dispatch
    cmp word [es:bp+HD_R_AX], 1106h
    jne .refuse
.dispatch:
    mov byte [hd_resident_busy], 1
    mov dx, ss
    mov cx, cs
    mov ss, cx
    mov sp, hd_resident_stack_top
    push dx
    push bp
    call hd_dispatch
    pop bx                         ; caller SP
    pop dx                         ; caller SS
    mov ss, dx
    mov sp, bx
    mov byte [cs:hd_resident_busy], 0
    jc .chain
    jmp .handled
.refuse:
    cmp word [es:bp+HD_R_AX], 111dh
    je .chain                      ; never consume an abort broadcast
    cmp word [es:bp+HD_R_AX], 1108h
    jne .status
    mov word [es:bp+HD_R_CX], 0
.status:
    mov word [es:bp+HD_R_AX], 31
    or word [es:bp+HD_R_FLAGS], 1
.handled:
    hd_restore_registers 0
    iret
.chain:
    hd_restore_registers 1
    jmp far [cs:hd_previous]

hd_previous dd 0
hd_resident_busy db 0
align 2, db 0
hd_resident_stack times 512 db 0
hd_resident_stack_top:
