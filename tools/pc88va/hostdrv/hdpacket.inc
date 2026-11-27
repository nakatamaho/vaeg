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

; The caller serializes access to the whole packet, including preparation
; and consuming its results. DS = program segment; ES is preserved.
; AL = opcode, DX:CX = mount (zero for QUERY). Payload is not cleared.
hd_prepare:
    push es
    push ds
    pop es
    cld
    mov di, hd_packet
    push ax
    xor ax, ax
    mov bx, cx
    mov cx, 32
    rep stosw
    pop ax
    mov ah, al
    mov al, 1
    mov [hd_packet], word 'HD'
    mov [hd_packet+2], word 'R1'
    mov [hd_packet+4], ax
    inc word [hd_expected_seq]
    mov ax, [hd_expected_seq]
    mov [hd_packet+6], ax
    mov [hd_packet+8], bx
    mov [hd_packet+10], dx
    pop es
    ret

; Send the prepared request once. CF denotes transport/validation failure
; (AX=31); otherwise AX is the validated, possibly nonzero, DOS status.
; The caller decides whether a transport failure latches the session shut.
hd_submit:
    call hd_snapshot
    mov bx, hd_validate
    call hd_exchange
    mov ax, 31
    jc .done
    mov ax, [hd_packet+32]
.done:
    ret

hd_snapshot:
    push es
    push ds
    pop es
    cld
    mov ax, [hd_packet+4]
    mov [hd_expected_verb], ax
    mov si, hd_packet
    mov di, hd_reply_header
    mov cx, 32
    rep movsw
    mov byte [hd_reply_header+2], 'S'
    pop es
    ret

; Run within hd_exchange's masked window. Construct the sole acceptable
; reply header from the captured request, checking variable outputs before
; incorporating them. The failed-DOS-status path allows no other changes.
hd_validate:
    mov ax, [hd_packet+32]
    mov [hd_reply_header+32], ax
    test ax, ax
    jz .success
    mov si, .status_codes
    cmp ax, 255
    ja .bad
.status_loop:
    cmp al, [si]
    je .compare
    inc si
    cmp byte [si], 0
    jne .status_loop
.bad:
    stc
    ret
.success:
    xor dx, dx                      ; expected payload length
    mov bl, [hd_reply_header+5]
    cmp bl, 0                        ; QUERY
    jne .token
    mov ax, [hd_packet+8]
    mov cx, [hd_packet+10]
    mov [hd_reply_header+8], ax
    mov [hd_reply_header+10], cx
    or ax, cx
    jz .bad
    mov word [hd_reply_header+38], 1
    jmp .length
.token:
    cmp bl, 3                        ; OPEN
    jne .search_token
    mov ax, [hd_packet+50]
    test ax, ax
    jz .bad
    mov [hd_reply_header+50], ax
.search_token:
    cmp bl, 3
    je .new_token
    cmp bl, 6                        ; FIND_FIRST
    jne .result
.new_token:
    mov ax, [hd_packet+12]
    mov cx, [hd_packet+14]
    mov [hd_reply_header+12], ax
    mov [hd_reply_header+14], cx
    or ax, cx
    jz .bad
.result:
    cmp bl, 4                        ; READ
    jne .space
    mov dx, [hd_packet+34]
    cmp dx, [hd_reply_header+20]
    ja .bad
    mov [hd_reply_header+34], dx
    jmp .length
.space:
    cmp bl, 2                        ; SPACE
    jne .find
    mov dx, 16
    mov ax, [hd_packet+72]
    or ax, [hd_packet+74]
    or ax, [hd_packet+76]
    or ax, [hd_packet+78]
    jnz .bad
    jmp .length
.find:
    cmp bl, 6
    je .find_length
    cmp bl, 7                        ; FIND_NEXT
    jne .stat
.find_length:
    mov dx, 11
    jmp .metadata
.stat:
    cmp bl, 1                        ; STAT
    je .metadata
    cmp bl, 3
    je .metadata
    cmp bl, 8                        ; FSTAT
    jne .length
.metadata:
    mov ax, [hd_packet+44]
    test ax, ~0037h & 0ffffh
    jnz .bad
    cmp word [hd_packet+42], 8000h
    jae .bad
    test al, 10h
    jnz .directory
    cmp bl, 1
    jne .copy_metadata
    cmp word [hd_reply_header+22], 0  ; empty STAT path must be root
    je .bad
    jmp .copy_metadata
.directory:
    cmp bl, 3
    je .bad
    cmp bl, 8
    je .bad
    mov ax, [hd_packet+40]
    or ax, [hd_packet+42]
    jnz .bad
.copy_metadata:
    mov si, 40
.metadata_word:
    mov ax, [hd_packet+si]
    mov [hd_reply_header+si], ax
    add si, 2
    cmp si, 50
    jb .metadata_word
.length:
    mov [hd_reply_header+36], dx
.compare:
    xor si, si
.next_word:
    mov ax, [hd_packet+si]
    cmp ax, [hd_reply_header+si]
    jne .bad
    add si, 2
    cmp si, 64
    jb .next_word
    clc
    ret
.status_codes db 1,2,3,4,5,6,12,17,18,21,31,32,33,39,80,0

hd_reply_header times 64 db 0
