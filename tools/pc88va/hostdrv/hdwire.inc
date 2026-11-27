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
; These routines preserve FLAGS except CF and clobber AX,CX,DX,SI.
; On refusal/failure CF=1; no retry after an ambiguous request.
%macro hd_lock 0
    pushf
    pop ax
    push ax
    cli
    test ax, 0100h                  ; IF masking does not suppress INT 1
    jnz .refuse
    cmp byte [hd_busy], 0
    jne .refuse
    inc byte [hd_busy]
    cld
%endmacro

%macro hd_unlock 0
.fail:
    mov byte [hd_busy], 0
.refuse:
    popf
    stc
    ret
.ok:
    mov byte [hd_busy], 0
    popf
    clc
    ret
%endmacro

hd_discover:
    hd_lock
    mov si, hd_np2
    call hd_command
    mov si, hd_np2
    mov cx, 4
    call hd_expect
    jc .fail
    mov si, hd_capability
    call hd_command
    mov si, hd_signature
    mov cx, 5
    call hd_expect
    jnc .ok
    hd_unlock

; Require the caller to finish opcode-specific result validation within
; the same IRQ-masked window. The validator cannot modify DS, the guard,
; or FLAGS.IF, and cannot call out of the driver. Pass its near offset in BX.
; hd_packet must contain a freshly constructed 64-byte header and any input.
hd_exchange:
    hd_lock
    mov dx, 07edh
    mov ax, hd_packet
    out dx, al
    mov al, ah
    out dx, al
    mov ax, ds
    out dx, al
    mov al, ah
    out dx, al
    mov si, hd_request
    call hd_command
    mov dx, 07edh
    xor cx, cx
    in al, dx
    or cl, al
    in al, dx
    or cl, al
    in al, dx
    mov ch, al
    in al, dx                       ; read every byte, even on failure
    cmp al, 'H'
    jne .fail
    cmp cx, 'R' << 8
    jne .fail
    cmp word [hd_packet], 'HD'
    jne .fail
    cmp word [hd_packet+2], 'S1'
    jne .fail
    mov ax, [hd_expected_verb]      ; version and opcode as a word
    cmp [hd_packet+4], ax
    jne .fail
    mov ax, [hd_expected_seq]
    cmp [hd_packet+6], ax
    jne .fail
    call bx
    jnc .ok
    hd_unlock

hd_command:
    mov dx, 07efh
    xor al, al
    out dx, al                     ; delimiter before, not after, the command
.next:
    lodsb
    test al, al
    jz .done
    out dx, al
    jmp .next
.done:
    ret

hd_expect:
.next:
    in al, dx
    cmp al, [si]
    jne .bad
    inc si
    loop .next
    clc
    ret
.bad:
    stc
    ret

hd_np2 db 'NP2', 0
hd_capability db 'hostdrv?1', 0
hd_request db 'hostdrv!1', 0
hd_signature db 'HDR1', 0
hd_busy db 0
hd_expected_verb dw 0
hd_expected_seq dw 0
