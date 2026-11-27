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

; DS must be CS. The caller must enforce explicit /VAEG opt-in, exclude
; callbacks, and own the bounce buffer before constructing the request.

cpu 8086
bits 16
org 100h

; Read-only DOS profile probe: no attach, vector hook, CDS write or port I/O.
    call hd_profile
    mov dx, incompatible
    jc refused
    mov al, 0ffh
    call hd_pick_cds
    mov dx, exhausted
    jc refused
    mov [picked], di
    mov [picked+2], es
    mov [letter], al
    call hd_pick_cds               ; the same slot when requested explicitly
    jc failed
    cmp al, [letter]
    jne failed
    cmp di, [picked]
    jne failed
    mov ax, es
    cmp ax, [picked+2]
    jne failed
    mov al, [hd_default]
    call hd_pick_cds
    jnc failed
    add byte [letter], 'A'
    mov dx, passed
    mov ah, 9
    int 21h
    mov ax, 4c00h
    int 21h
failed:
    mov dx, mismatch
refused:
    mov ah, 9
    int 21h
    mov ax, 4c01h
    int 21h

passed db 'HDPROFILE PASS '
letter db '?',13,10,'$'
incompatible db 'HDPROFILE INCOMPATIBLE',13,10,'$'
exhausted db 'HDPROFILE NO FREE CDS',13,10,'$'
mismatch db 'HDPROFILE CDS MISMATCH',13,10,'$'
picked dd 0

%include 'hdprofile.inc'
