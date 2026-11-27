; Copyright (c) 2026 Nakata Maho
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions are met:
; 1. Redistributions of source code must retain the above copyright notice,
;    this list of conditions and the following disclaimer.
; 2. Redistributions in binary form must reproduce the above copyright
;    notice, this list of conditions and the following disclaimer in the
;    documentation and/or other materials provided with the distribution.
;
; THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
; IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
; OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
; IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
; SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
; TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
; PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
; LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
; NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
; SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

cpu 8086
bits 16
org 100h

; VAEG only: HDQUERY /VAEG. Never run with the switch on real hardware.
; Does not claim a CDS, hook DOS, or create a resident attachment.
    xor cx, cx
    mov cl, [80h]
    sub cx, 6
    jc fail
    cmp word [81h], ' /'
    jne fail
    cmp word [83h], 'VA'
    jne fail
    cmp word [85h], 'EG'
    jne fail
    mov si, 87h
    jcxz opted_in
trailing_space:
    cmp byte [si], ' '
    jne fail
    inc si
    loop trailing_space
opted_in:
    mov byte [hd_busy], 1
    call hd_discover
    jnc fail
    cmp byte [hd_busy], 1
    jne fail
    mov byte [hd_busy], 0
    call hd_discover
    jc fail
    mov ax, [hd_packet+4]
    mov [hd_expected_verb], ax
    mov ax, [hd_packet+6]
    mov [hd_expected_seq], ax
    mov bx, validate_query
    call hd_exchange
    jc fail
    mov dx, pass_text
    mov ah, 9
    int 21h
    mov ax, 4c00h
    int 21h
fail:
    mov dx, fail_text
    mov ah, 9
    int 21h
    mov ax, 4c01h
    int 21h

validate_query:
    mov ax, [hd_packet+8]
    or ax, [hd_packet+10]
    jz .bad
    cmp word [hd_packet+38], 1
    jne .bad
    mov si, 12
.zero:
    cmp si, 38
    je .skip_caps
    cmp word [hd_packet+si], 0
    jne .bad
.skip_caps:
    add si, 2
    cmp si, 64
    jb .zero
    clc
    ret
.bad:
    stc
    ret

pass_text db 'HDQUERY PASS',13,10,'$'
fail_text db 'HDQUERY refused or failed',13,10,'$'
%include 'hdwire.inc'

hd_packet db 'HDR1', 1, 0
          dw 1
          times 4160-8 db 0
