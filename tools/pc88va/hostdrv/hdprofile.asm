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
    call dependency_fixture
    jc dependency_failed
    add byte [letter], 'A'
    mov dx, passed
    mov ah, 9
    int 21h
    mov ax, 4c00h
    int 21h
failed:
    mov dx, mismatch
    jmp refused
dependency_failed:
    mov dx, dependency_mismatch
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
dependency_mismatch db 'HDPROFILE DEPENDENCY '
dependency_stage db '0',13,10,'$'

%include 'hdprofile.inc'

; All mutations below affect only this transient's synthetic table. The
; passing baseline is A: physical/default, B: SUBST to A:, C:/D: free.
dependency_fixture:
    mov word [hd_cds_base], synthetic_cds
    mov [hd_cds_base+2], cs
    mov byte [hd_cds_count], 4
    mov byte [hd_physical], 1
    mov byte [hd_default], 0
    mov byte [dependency_stage], '1'
    call highest_d
    jc .bad
    mov byte [dependency_stage], '2'
    mov byte [substitute], 'd'     ; one-byte mutation of passing table
    mov al, 3
    call hd_pick_cds
    jnc .bad
    mov al, 0ffh
    call hd_pick_cds
    jc .bad
    cmp al, 2
    jne .bad
    mov byte [substitute], 'A'
    call highest_d
    jc .bad
    mov byte [dependency_stage], '3'
    xor word [splice_flags], 6000h ; make the prepared splice active
    mov al, 3
    call hd_check_dependencies
    jnc .bad
    xor word [splice_flags], 6000h
    call highest_d
    jc .bad
    mov byte [dependency_stage], '4'
    mov byte [substitute+1], 0    ; malformed mapping: refuse all candidates
    mov al, 0ffh
    call hd_pick_cds
    cmc
    ret
.bad:
    stc
    ret

highest_d:
    mov al, 0ffh
    call hd_pick_cds
    jc .bad
    cmp al, 3
    jne .bad
    clc
    ret
.bad:
    stc
    ret

synthetic_cds:
    db 'D:',5ch,'MOUNT',0
    times 43h-($-synthetic_cds) db 0
splice_flags dw 4000h
    times HD_CDS_SIZE-($-synthetic_cds) db 0
substitute:
    db 'A:',5ch,0
    times 43h-($-substitute) db 0
    dw 5000h
    times HD_CDS_SIZE-($-substitute) db 0
    times 2*HD_CDS_SIZE db 0
