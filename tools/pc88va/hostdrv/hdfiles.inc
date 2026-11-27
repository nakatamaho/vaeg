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
; IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
; SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
; TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
; PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
; LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
; NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
; EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

%define HD_FILE_MAX 64
%define HD_FILE_ROW 10             ; SFT far pointer, token, serial (0=free)

hd_classify_file:
    cmp ax, 1106h
    jb .chain
    cmp ax, 110ah
    jbe .lookup
    cmp ax, 1121h
    je .lookup
    cmp ax, 112dh
    jne .chain
.lookup:
    call hd_find_file
    mov ax, [es:bp+HD_R_AX]
    ret
.chain:
    stc
    ret

; Frame -> BX registry row. Check exact far address before dereferencing.
hd_find_file:
    push es
    mov di, [es:bp+HD_R_DI]
    mov dx, [es:bp+HD_R_ES]
    mov bx, hd_files
    mov cx, HD_FILE_MAX
.next:
    cmp word [bx+8], 0
    je .skip
    cmp [bx], di
    jne .skip
    cmp [bx+2], dx
    je .verify
.skip:
    add bx, HD_FILE_ROW
    loop .next
    jmp .bad
.verify:
    mov es, dx
    mov ax, [bx+8]
    cmp [es:di+0bh], ax
    jne .bad
    mov ax, [bx+4]
    cmp [es:di+19h], ax
    jne .bad
    mov ax, [bx+6]
    cmp [es:di+1bh], ax
    jne .bad
    cmp word [es:di+1dh], HD_TAG
    jne .bad
    cmp word [es:di+37h], hd_ifs
    jne .bad
    mov ax, cs
    cmp [es:di+39h], ax
    jne .bad
    pop es
    clc
    ret
.bad:
    pop es
    stc
    ret

hd_file_dispatch:
    cmp ax, 1116h
    je hd_open_file
    cmp ax, 1106h
    jb hd_path_dispatch
    cmp ax, 110ah
    jbe .owned
    cmp ax, 1121h
    je .owned
    cmp ax, 112dh
    jne hd_path_dispatch
.owned:
    call hd_find_file
    jc .invalid
    mov [hd_file_row], bx
    mov ax, [es:bp+HD_R_AX]
    cmp ax, 1106h
    je hd_close_file
    cmp ax, 1108h
    je hd_read_file
    cmp ax, 1107h
    je hd_file_stat
    cmp ax, 1121h
    je hd_seek_end
    cmp ax, 112dh
    je .unsupported
    mov ax, 5                     ; writes and locks are not supported
    jmp hd_return_error
.unsupported:
    mov ax, 1
    jmp hd_return_error
.invalid:
    mov ax, 6
    jmp hd_return_error

hd_open_file:
    cmp byte [hd_latched], 0
    jne .fault
    call hd_verify_tables
    jc .fault
    push es
    les di, [hd_sda]
    les di, [es:di+27eh]          ; THISSFT
    mov [hd_open_sft], di
    mov [hd_open_sft+2], es
    call hd_validate_open_slot
    jc .bad_sft
    les di, [hd_open_sft]
    mov ax, [es:di+2]
    test ah, 80h                 ; FCB dispatch is not enabled yet
    jnz .denied_sft
    cmp word [es:di], -1          ; DOS's reserved ordinary SFT
    jne .bad_sft
    pop es
    and ax, 77h
    test al, 7
    jnz .denied
    mov [hd_open_mode], ax
    mov bx, hd_files
    mov cx, HD_FILE_MAX
.free:
    cmp word [bx+8], 0
    je .found
    add bx, HD_FILE_ROW
    loop .free
    mov ax, 4
    jmp hd_return_error
.found:
    mov [hd_file_row], bx
    mov al, 3
    call hd_session_prepare
    call hd_copy_path
    jc hd_return_error
    call hd_make_sft_name
    jc hd_return_error
    mov ax, [hd_open_mode]
    mov [hd_packet+30], ax
    call hd_submit
    jc hd_file_ambiguous
    test ax, ax
    jnz hd_return_error
    push es
    les di, [hd_open_sft]
    mov bx, [hd_file_row]
    mov [bx], di
    mov [bx+2], es
    mov ax, [hd_packet+12]
    mov [bx+4], ax
    mov [es:di+19h], ax
    mov ax, [hd_packet+14]
    mov [bx+6], ax
    mov [es:di+1bh], ax
    mov ax, [hd_packet+50]
    mov [bx+8], ax
    mov [es:di+0bh], ax
    mov word [es:di+1dh], HD_TAG
    mov byte [es:di+1fh], 0
    inc byte [hd_file_count]
    mov al, [hd_packet+44]
    mov [es:di+4], al
    xor ax, ax
    mov al, [hd_drive]
    or ax, 8040h                 ; remote, clean, DOS drive number
    mov [es:di+5], ax
    mov word [es:di+7], hd_device
    mov [es:di+9], cs
    mov ax, [hd_packet+46]
    mov [es:di+0dh], ax
    mov ax, [hd_packet+48]
    mov [es:di+0fh], ax
    mov ax, [hd_packet+40]
    mov [es:di+11h], ax
    mov ax, [hd_packet+42]
    mov [es:di+13h], ax
    xor ax, ax
    mov [es:di+15h], ax
    mov [es:di+17h], ax
    mov [es:di+2bh], ax
    mov [es:di+2dh], ax
    mov [es:di+33h], ax
    mov [es:di+35h], ax
    push ds
    lds si, [hd_sda]
    mov ax, [si+1eh]             ; DOS USER_ID
    mov [es:di+2fh], ax
    mov ax, [si+1ch]
    mov [es:di+31h], ax
    pop ds
    mov word [es:di+37h], hd_ifs
    mov [es:di+39h], cs
    add di, 20h
    mov si, hd_open_name
    mov cx, 11
    rep movsb
    pop es
    jmp hd_dispatch.done
.bad_sft:
    pop es
.fault:
    mov ax, 31
    jmp hd_return_error
.denied_sft:
    pop es
.denied:
    mov ax, 5
    jmp hd_return_error

; Validate THISSFT against the ordinary table, not just a RAM span. A
; malformed chain cannot authorize a pointer into a header or entry body.
hd_validate_open_slot:
    les di, [hd_sft_head]
    mov si, 256
.block:
    cmp di, -1
    je .bad
    mov cx, 6
    call hd_dos_span
    jc .bad
    mov ax, [es:di+4]
    mov cx, HD_SFT_SIZE
    mul cx
    test dx, dx
    jnz .bad
    mov bx, ax
    add ax, 6
    jc .bad
    mov cx, ax
    call hd_dos_span
    jc .bad
    mov ax, es
    cmp ax, [hd_open_sft+2]
    jne .next
    mov ax, [hd_open_sft]
    sub ax, di
    jb .next
    sub ax, 6
    jb .next
    cmp ax, bx
    jae .next
    xor dx, dx
    mov cx, HD_SFT_SIZE
    div cx
    test dx, dx
    jnz .bad
    clc
    ret
.next:
    les di, [es:di]
    dec si
    jnz .block
.bad:
    stc
    ret

; Convert the canonical final component to the padded SFT name. A trail
; byte is never a separator (including 5Ch); the host validates encoding.
hd_make_sft_name:
    push es
    push ds
    pop es
    mov di, hd_open_name
    mov al, ' '
    mov cx, 11
    rep stosb
    pop es
    mov si, hd_packet+64
    mov cx, [hd_packet+22]
    jcxz .bad
    mov bx, si
.scan:
    lodsb
    cmp al, 5ch
    jne .lead
    mov bx, si
.lead:
    call hd_is_lead
    jnc .scan_next
    dec cx
    jz .bad
    inc si
.scan_next:
    loop .scan
    mov cx, si
    sub cx, bx
    jz .bad
    mov si, bx
    mov di, hd_open_name
    mov dx, di
    add dx, 8
.copy:
    lodsb
    cmp al, '.'
    jne .store
    cmp dx, hd_open_name+8
    jne .bad
    mov di, dx
    add dx, 3
    jmp .next
.store:
    cmp di, dx
    jae .bad
    mov [di], al
    inc di
    call hd_is_lead
    jnc .next
    dec cx
    jz .bad
    cmp di, dx
    jae .bad
    lodsb
    mov [di], al
    inc di
.next:
    loop .copy
    clc
    ret
.bad:
    mov ax, 2
    stc
    ret

hd_is_lead:
    cmp al, 81h
    jb .no
    cmp al, 9fh
    jbe .yes
    cmp al, 0e0h
    jb .no
    cmp al, 0fch
    jbe .yes
.no:
    clc
    ret
.yes:
    stc
    ret

; Final reference retirement is local, even with no usable transport.
hd_close_file:
    push es
    les di, [bx]
    cmp word [es:di], 0
    je .invalid
    cmp word [es:di], -1
    je .invalid
    dec word [es:di]
    jnz .nonfinal
    mov word [bx+8], 0
    dec byte [hd_file_count]
    xor ax, ax
    mov [es:di+19h], ax
    mov [es:di+1bh], ax
    mov [es:di+1dh], ax
    pop es
    test si, si                  ; caller TF: do not touch the ports
    jnz hd_file_ambiguous
    cmp byte [hd_latched], 0
    jne hd_file_ambiguous
    mov al, 5
    call hd_prepare_file
    call hd_submit
    jc hd_file_ambiguous
    test ax, ax
    jnz hd_return_error
    jmp hd_dispatch.done
.nonfinal:
    pop es
    jmp hd_dispatch.done
.invalid:
    pop es
    mov ax, 6
    jmp hd_return_error

hd_file_ambiguous:
    mov byte [hd_latched], 1
hd_file_fault:
    mov ax, 31
    jmp hd_return_error

hd_file_stat:
    call hd_submit_fstat
    jc hd_return_error
    jmp hd_dispatch.done

hd_submit_fstat:
    cmp byte [hd_latched], 0
    jne .blocked
    mov al, 8
    call hd_prepare_file
    call hd_submit
    jc .error
    test ax, ax
    jz .ok
.error:
    stc
.ok:
    ret
.blocked:
    mov ax, 31
    stc
    ret

hd_seek_end:
    call hd_submit_fstat
    jc hd_return_error
    mov ax, [hd_packet+40]
    mov dx, [hd_packet+42]
    add ax, [es:bp+HD_R_DX]
    adc dx, [es:bp+HD_R_CX]
    js hd_file_fault
    mov [es:bp+HD_R_AX], ax
    mov [es:bp+HD_R_DX], dx
    jmp hd_dispatch.done

hd_read_file:
    mov ax, [es:bp+HD_R_CX]
    mov [hd_read_left], ax
    mov word [es:bp+HD_R_CX], 0
    test ax, ax
    jnz .nonempty
    jmp hd_dispatch.done
.nonempty:
    cmp byte [hd_latched], 0
    jne hd_file_fault
    push es
    mov cx, [hd_read_left]
    call hd_dma_span
    jc .bad_range
    mov [hd_read_dma], di
    mov [hd_read_dma+2], es
    pop es
.next:
    mov al, 4
    call hd_prepare_file
    push es
    les di, [bx]
    mov ax, [es:di+15h]
    mov dx, [es:di+17h]
    pop es
    test dx, 8000h
    jnz .range
    mov [hd_packet+16], ax
    mov [hd_packet+18], dx
    mov cx, [hd_read_left]
    cmp cx, 4096
    jbe .count
    mov cx, 4096
.count:
    mov [hd_packet+20], cx
    mov [hd_packet+28], cx
    call hd_submit
    jc .error
    test ax, ax
    jnz .error
    mov cx, [hd_packet+34]
    jcxz .success
    push es
    les di, [hd_read_dma]
    mov si, hd_packet+64
    mov dx, cx
    rep movsb
    mov [hd_read_dma], di
    mov bx, [hd_file_row]
    les di, [bx]
    add [es:di+15h], dx
    adc word [es:di+17h], 0
    pop es
    add [es:bp+HD_R_CX], dx
    sub [hd_read_left], dx
    jz .success
    cmp dx, [hd_packet+20]
    je .next
.success:
    jmp hd_dispatch.done
.bad_range:
    pop es
.range:
    mov ax, 31
.error:
    cmp word [es:bp+HD_R_CX], 0
    jne .success
    jmp hd_return_error

; CX = transfer size, return ES:DI = DMAADD on success. Never accept
; writes into the resident image (including its private stack).
hd_dma_span:
    les di, [hd_sda]
    les di, [es:di+0ch]
    call hd_dos_span
    jc .bad
    mov bx, es
    mov dx, cs
    mov ax, di
    add ax, cx
    dec ax
    mov cl, 4
    shr ax, cl
    add ax, bx
    cmp ax, dx
    jb .ok
    mov ax, di
    shr ax, cl
    add ax, bx
    add dx, (hd_resident_end-$$+100h+15)/16
    cmp ax, dx
    jb .bad
.ok:
    clc
    ret
.bad:
    stc
    ret

hd_session_prepare:
    mov cx, [hd_generation]
    mov dx, [hd_generation+2]
    jmp hd_prepare

; Return BX at the selected registry row, reusable across packet builds.
hd_prepare_file:
    call hd_session_prepare
    mov bx, [hd_file_row]
    mov ax, [bx+4]
    mov [hd_packet+12], ax
    mov ax, [bx+6]
    mov [hd_packet+14], ax
    ret

hd_files times HD_FILE_MAX*HD_FILE_ROW db 0
hd_file_count db 0
hd_file_row dw 0
hd_open_sft dd 0
hd_open_mode dw 0
hd_open_name times 11 db 0
hd_read_left dw 0
hd_read_dma dd 0
