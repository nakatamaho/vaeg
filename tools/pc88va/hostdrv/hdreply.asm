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

; Synthetic headers only. Never accesses either shared port.
    cld
    mov si, cases
.next:
    lodsw
    test ax, ax
    jz pass
    mov [setup], ax
    lodsw
    mov [mutation], ax
    mov [next_case], si
    call make_reply
    call hd_validate
    jc fail
    call make_reply
    mov bx, [mutation]
    xor byte [hd_packet+bx], 1
    call hd_validate
    jnc fail
    mov si, [next_case]
    inc byte [case_id]
    jmp .next

make_reply:
    call word [setup]
    call hd_snapshot
    mov byte [hd_packet+2], 'S'
    jmp word [generate]

query:
    xor cx, cx
    xor dx, dx
    mov al, 0
    call hd_prepare
    mov word [generate], query_ok
    ret
query_ok:
    mov word [hd_packet+8], 1
    mov word [hd_packet+38], 1
    ret

open:
    mov cx, 5
    xor dx, dx
    mov al, 3
    call hd_prepare
    mov word [hd_packet+22], 1
    mov word [hd_packet+26], 1
    mov byte [hd_packet+64], 'A'
    mov word [generate], open_ok
    ret
open_ok:
    mov word [hd_packet+12], 1
    mov word [hd_packet+40], 10
    mov word [hd_packet+44], 1
    mov word [hd_packet+50], 1
    ret
open_denied:
    call open
    mov word [generate], denied
    ret
denied:
    mov word [hd_packet+32], 5
    ret

read:
    mov cx, 5
    xor dx, dx
    mov al, 4
    call hd_prepare
    mov word [hd_packet+12], 1
    mov word [hd_packet+20], 2
    mov word [hd_packet+28], 2
    mov word [generate], read_ok
    ret
read_ok:
    mov word [hd_packet+34], 2
    mov word [hd_packet+36], 2
    ret

space:
    mov cx, 5
    xor dx, dx
    mov al, 2
    call hd_prepare
    mov word [hd_packet+28], 16
    mov word [generate], space_ok
    ret
space_ok:
    mov word [hd_packet+36], 16
    mov si, 64
.clear:
    mov word [hd_packet+si], 0
    add si, 2
    cmp si, 80
    jb .clear
    ret

pass:
    mov dx, pass_text
    mov ah, 9
    int 21h
    mov ax, 4c00h
    int 21h
fail:
    mov dx, fail_text
    mov ah, 9
    int 21h
    mov dl, [case_id]
    mov ah, 2
    int 21h
    mov ax, 4c01h
    int 21h

; Each row starts from a passing fixture and flips one bit.
; Case IDs are ASCII A–L in table order.
cases dw query,2, query,4, query,6, query,8, query,38
      dw open,12, open,45, open,50, open_denied,40
      dw read,34, read,36, space,72, 0
setup dw 0
mutation dw 0
next_case dw 0
generate dw 0
case_id db 'A'
pass_text db 'HDREPLY PASS',13,10,'$'
fail_text db 'HDREPLY FAIL ','$'

%include 'hdwire.inc'
%include 'hdpacket.inc'
hd_packet times 4160 db 0
