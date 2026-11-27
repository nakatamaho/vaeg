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

%define HD_SEARCH_MAX 32
%define HD_SEARCH_ROW 6           ; token dword and owner PSP word

hd_classify_search:
    push es
    mov cx, 21
    call hd_dma_span
    jc .chain
    mov al, [hd_drive]
    or al, 80h
    cmp [es:di], al
    jne .chain
    cmp word [es:di+1], 'HD'
    jne .chain
    pop es
    mov ax, 111ch
    clc
    ret
.chain:
    pop es
    stc
    ret

hd_search_dispatch:
    cmp ax, 111dh
    je hd_reap_searches
    cmp ax, 111bh
    je hd_find_first
    cmp ax, 111ch
    je hd_find_next
    jmp hd_file_dispatch

hd_find_first:
    cmp byte [hd_latched], 0
    jne hd_file_fault
    call hd_verify_tables
    jc hd_file_fault
    call hd_search_dma
    jc hd_file_fault
    mov al, 6
    call hd_session_prepare
    call hd_copy_path
    jc hd_return_error
    call hd_make_sft_name         ; BX points to the final component
    jc hd_search_end
    sub bx, hd_packet+64
    jz .root
    dec bx                        ; omit the final separator
.root:
    mov [hd_packet+22], bx
    lea ax, [bx+11]
    mov [hd_packet+26], ax
    mov word [hd_packet+28], 11
    push es
    push ds
    pop es
    lea di, [bx+hd_packet+64]
    mov si, hd_open_name
    mov cx, 11
    rep movsb
    les di, [hd_sda]
    xor ax, ax
    mov al, [es:di+24dh]          ; SATTRIB
    and al, 3fh
    mov [hd_packet+30], ax
    mov ax, [es:di+10h]           ; CurrentPDB
    mov [hd_search_owner], ax
    pop es
    call hd_submit
    jc hd_file_ambiguous
    test ax, ax
    jnz hd_search_status
    call hd_validate_find_name
    jc hd_file_ambiguous
    cmp word [hd_search_used], HD_SEARCH_MAX*HD_SEARCH_ROW
    jb .register
    mov bx, hd_searches           ; host evicts oldest on successful OPEN search
    call hd_remove_search
.register:
    mov bx, [hd_search_used]
    mov ax, [hd_packet+12]
    mov [bx+hd_searches], ax
    mov ax, [hd_packet+14]
    mov [bx+hd_searches+2], ax
    mov ax, [hd_search_owner]
    mov [bx+hd_searches+4], ax
    add word [hd_search_used], HD_SEARCH_ROW
    mov word [hd_search_ordinal], 1
    jmp hd_publish_search

hd_find_next:
    cmp byte [hd_latched], 0
    jne hd_search_end
    call hd_search_dma
    jc hd_file_fault
    mov al, 7
    call hd_session_prepare
    push es
    les di, [hd_sda]
    mov bx, [es:di+10h]
    les di, [hd_search_dma_ptr]
    mov ax, [hd_generation]
    cmp [es:di+3], ax
    jne .stale
    mov ax, [hd_generation+2]
    cmp [es:di+5], ax
    jne .stale
    mov ax, [es:di+7]
    mov dx, [es:di+9]
    mov cx, [es:di+11]
    pop es
    test cx, cx
    jz hd_search_end
    cmp cx, 4096
    ja hd_search_end
    mov si, hd_searches
    mov di, si
    add di, [hd_search_used]
.lookup:
    cmp si, di
    jae hd_search_end
    cmp [si], ax
    jne .skip
    cmp [si+2], dx
    jne .skip
    cmp [si+4], bx
    je .send
.skip:
    add si, HD_SEARCH_ROW
    jmp .lookup
.send:
    mov [hd_packet+12], ax
    mov [hd_packet+14], dx
    mov [hd_packet+16], cx
    inc cx
    mov [hd_search_ordinal], cx
    mov word [hd_packet+28], 11
    call hd_submit
    jc hd_return_error
    test ax, ax
    jnz hd_search_status
    call hd_validate_find_name
    jc hd_file_fault
    jmp hd_publish_search
.stale:
    pop es
hd_search_end:
    mov ax, 18
    jmp hd_return_error
hd_search_status:
    cmp ax, 21
    je hd_search_end
    jmp hd_return_error

hd_search_dma:
    push es
    mov cx, 53
    call hd_dma_span
    jc .done
    mov [hd_search_dma_ptr], di
    mov [hd_search_dma_ptr+2], es
.done:
    pop es
    ret

; Check fixed fields before consuming the reply: padding at the end only,
; no syntax bytes as single bytes, and no DBCS pair crossing a field.
hd_validate_find_name:
    mov si, hd_packet+64
    cmp byte [si], ' '
    je .bad
    mov cx, 8
    call .field
    jc .bad
    mov cx, 3
.field:
    xor bx, bx                    ; padding seen
.byte:
    lodsb
    cmp al, ' '
    je .space
    test bx, bx
    jnz .bad
    cmp al, ' '
    jb .bad
    cmp al, 7fh
    je .bad
    call hd_is_lead
    jc .pair
    mov di, .illegal
.check_single:
    cmp byte [di], 0
    je .next
    cmp al, [di]
    je .bad
    inc di
    jmp .check_single
.space:
    inc bx
    jmp .next
.pair:
    dec cx
    jz .bad
    lodsb
    cmp al, 40h
    jb .bad
    cmp al, 7fh
    je .bad
    cmp al, 0fch
    ja .bad
.next:
    loop .byte
    clc
    ret
.bad:
    stc
    ret
.illegal db '"*+,./:;<=>?[\]|', 0

hd_publish_search:
    push es
    les di, [hd_search_dma_ptr]
    push di
    xor ax, ax
    mov cx, 53
    rep stosb
    pop di
    mov al, [hd_drive]
    or al, 80h
    mov [es:di], al
    mov word [es:di+1], 'HD'
    mov ax, [hd_generation]
    mov [es:di+3], ax
    mov ax, [hd_generation+2]
    mov [es:di+5], ax
    mov ax, [hd_packet+12]
    mov [es:di+7], ax
    mov ax, [hd_packet+14]
    mov [es:di+9], ax
    mov ax, [hd_search_ordinal]
    mov [es:di+11], ax
    mov ax, [hd_packet+44]
    mov [es:di+21+11], al
    mov ax, [hd_packet+46]
    mov [es:di+21+22], ax
    mov ax, [hd_packet+48]
    mov [es:di+21+24], ax
    mov ax, [hd_packet+40]
    mov [es:di+21+28], ax
    mov ax, [hd_packet+42]
    mov [es:di+21+30], ax
    add di, 21
    mov si, hd_packet+64
    mov cx, 11
    rep movsb
    cmp byte [es:di-11], 0e5h
    jne .done
    mov byte [es:di-11], 05h
.done:
    pop es
    jmp hd_dispatch.done

hd_reap_searches:
    push es
    les di, [hd_sda]
    mov ax, [es:di+10h]
    pop es
    mov [hd_search_owner], ax
    mov bx, hd_searches
.next:
    mov ax, bx
    sub ax, hd_searches
    cmp ax, [hd_search_used]
    jae .chain
    cmp [bx+4], word 0
    je .skip
    mov ax, [hd_search_owner]
    cmp [bx+4], ax
    jne .skip
    call hd_release_search
    jnc .next
    jmp .chain
.skip:
    add bx, HD_SEARCH_ROW
    jmp .next
.chain:
    jmp hd_reap_fcbs               ; also chains the untouched broadcast frame

; Transient uninstall calls this code with the resident data segment.
hd_drain_searches:
    cmp word [hd_search_used], 0
    je .done
    mov bx, hd_searches
    call hd_release_search
    jnc hd_drain_searches
    ret
.done:
    clc
    ret

hd_release_search:
    cmp byte [hd_latched], 0
    jne .bad
    push bx
    mov al, 9
    call hd_session_prepare
    pop bx
    mov ax, [bx]
    mov [hd_packet+12], ax
    mov ax, [bx+2]
    mov [hd_packet+14], ax
    push bx
    call hd_submit
    pop bx
    jc .bad
    test ax, ax
    jz hd_remove_search
    cmp ax, 18                    ; expired/evicted
    je hd_remove_search
    cmp ax, 21                    ; detached, no host snapshots remain
    je hd_remove_search
.bad:
    mov byte [hd_latched], 1
    stc
    ret

; Compact by moving towards the lower address. BX remains at the slot.
hd_remove_search:
    push es
    push ds
    pop es
    mov di, bx
    lea si, [bx+HD_SEARCH_ROW]
    mov cx, [hd_search_used]
    add cx, hd_searches
    sub cx, si
    rep movsb
    sub word [hd_search_used], HD_SEARCH_ROW
    pop es
    clc
    ret

hd_searches times HD_SEARCH_MAX*HD_SEARCH_ROW db 0
hd_search_used dw 0
hd_search_dma_ptr dd 0
hd_search_ordinal dw 0
hd_search_owner dw 0
