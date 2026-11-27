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

; MS-DOS 4.0 VA profile (FreeDOS-88VA/MS-DOS msdos4-va.2).
; See docs/agents/research/m172_hostdrv_design.md for source/linked layout.
; This is a structural compatibility check, not authentication of DOS.
; Transient only: no DOS API calls from a resident INT 2Fh handler.
%define HD_CDS_SIZE 58h
%define HD_SFT_SIZE 3bh
%define HD_SDA_SIZE 78ch
%define HD_SDA_PREFIX 1ah

; DS=CS. CF=1 unsupported. On success save bounded tables for install.
hd_profile:
    mov ax, 3000h
    int 21h
    cmp ax, 4
    jne .bad

    mov ax, 5d06h
    mov dx, hd_dpl
    int 21h
    push ds
    pop es
    push cs
    pop ds
    jc .bad
    cmp cx, HD_SDA_SIZE
    jne .bad
    cmp dx, HD_SDA_PREFIX
    jne .bad
    mov di, si
    call hd_dos_span
    jc .bad
    mov [hd_sda], di
    mov [hd_sda+2], es

    mov ax, 5d0bh
    int 21h
    push ds
    pop es
    push cs
    pop ds
    jc .bad
    mov di, si
    mov cx, 14
    call hd_dos_span
    jc .bad
    cmp word [es:di], 2
    jne .bad
    mov ax, [hd_sda+2]
    cmp [es:di+4], ax
    jne .bad
    cmp [es:di+10], ax
    jne .bad
    mov ax, [hd_sda]
    cmp [es:di+8], ax
    jne .bad
    add ax, HD_SDA_PREFIX
    cmp [es:di+2], ax
    jne .bad
    cmp word [es:di+6], HD_SDA_SIZE-HD_SDA_PREFIX
    jne .bad
    cmp word [es:di+12], 8000h | HD_SDA_PREFIX
    jne .bad

    mov ah, 34h
    int 21h
    mov ax, es
    cmp ax, [hd_sda+2]
    jne .bad
    dec bx
    cmp bx, [hd_sda]
    jne .bad
    cmp word [es:bx], 0            ; ErrorMode and InDOS both clear
    jne .bad

    mov ah, 19h
    int 21h
    mov [hd_default], al
    mov ah, 52h
    int 21h
    mov di, bx
    mov cx, 34h
    call hd_dos_span
    jc .bad
    mov ax, es
    cmp ax, [hd_sda+2]             ; SYSINITVAR shares DOSGroup
    jne .bad
    cmp word [es:di+2ch], 'NU'      ; embedded NUL device in list of lists
    jne .bad
    cmp word [es:di+2eh], 'L '
    jne .bad
    mov al, [es:di+21h]
    cmp al, 26
    ja .bad
    cmp al, [hd_default]
    jbe .bad
    cmp al, [es:di+20h]
    jb .bad
    mov [hd_cds_count], al
    mov al, [es:di+20h]
    mov [hd_physical], al
    mov ax, [es:di+04h]
    mov [hd_sft_head], ax
    mov ax, [es:di+06h]
    mov [hd_sft_head+2], ax
    mov ax, [es:di+1ah]
    mov [hd_fcb_head], ax
    mov ax, [es:di+1ch]
    mov [hd_fcb_head+2], ax
    les di, [es:di+16h]
    xor ax, ax
    mov al, [hd_cds_count]
    mov cl, HD_CDS_SIZE
    mul cl
    mov cx, ax
    call hd_dos_span
    jc .bad
    mov [hd_cds_base], di
    mov [hd_cds_base+2], es
    les di, [hd_sft_head]
    cmp di, 0ffffh
    je .bad
    call hd_sft_chain
    jc .bad
    les di, [hd_fcb_head]
    jmp hd_sft_chain
.bad:
    stc
    ret

; ES:DI, CX=length. Check no segment wrap and [10000h,80000h)
; native ordinary RAM. Preserves ES:DI and CX; clobbers AX/DX only.
hd_dos_span:
    call hd_span_paragraphs
    jc .bad
    cmp dx, 8000h
    jae .bad
    clc
    ret
.bad:
    stc
    ret

; Return inclusive first/last paragraphs in AX/DX. Reject empty spans,
; segment wrap, and addresses outside the 20-bit linear address space.
hd_span_paragraphs:
    jcxz .bad
    mov ax, di
    add ax, cx
    dec ax                         ; inclusive last offset
    cmp ax, di
    jb .bad
    mov dx, es
    shr ax, 1
    shr ax, 1
    shr ax, 1
    shr ax, 1
    add ax, dx
    jc .bad
    xchg ax, dx                    ; DX = last, AX = segment
    push cx
    mov cx, di
    shr cx, 1
    shr cx, 1
    shr cx, 1
    shr cx, 1
    add ax, cx                     ; first <= last, cannot overflow
    pop cx
    cmp ax, 1000h
    jb .bad
    clc
    ret
.bad:
    stc
    ret

; A bounded conventional-memory table, not an arbitrary memory scan.
; DOS permits zero FCB cache entries. An FFFF offset ends either chain.
hd_sft_chain:
    mov bp, 256
.next:
    cmp di, 0ffffh
    je .end
    mov cx, 6
    call hd_dos_span
    jc .bad
    mov ax, [es:di+4]
    mov cx, HD_SFT_SIZE
    mul cx
    test dx, dx
    jnz .bad
    add ax, 6
    jc .bad
    mov cx, ax
    call hd_dos_span
    jc .bad
    les di, [es:di]
    dec bp
    jnz .next
.bad:
    stc
    ret
.end:
    clc
    ret

; AL=zero-based requested drive or FFh for highest available. Return
; AL=drive and ES:DI=CDS if free and no mapping refers to that drive.
; Does not write DOS state or touch ports.
hd_pick_cds:
    mov bl, al
    cmp al, 0ffh
    jne .candidate
    mov al, [hd_cds_count]
    dec al
.candidate:
    cmp al, [hd_cds_count]
    jae .bad
    cmp al, [hd_physical]
    jb .bad
    cmp al, [hd_default]
    jbe .bad
    les di, [hd_cds_base]
    mov cl, HD_CDS_SIZE
    mul cl
    add di, ax
    cmp word [es:di+43h], 0
    jne .used
    div cl
    call hd_check_dependencies
    jnc .found
    jmp .skip
.used:
    div cl
.skip:
    cmp bl, 0ffh
    jne .bad
    dec al
    jmp .candidate
.found:
    ret
.bad:
    stc
    ret

; AL=target drive in the already validated CDS table. CF=1 if a local
; substitution or splice names that drive, or a mapping cannot be parsed.
; Both utilities store the backing drive as the first byte of CDS.text.
; Call with freshly checked table/default-drive state before teardown too.
; Preserves all registers. Checks dependency only, not ownership of target.
hd_check_dependencies:
    push ax
    push bx
    push cx
    push di
    push es
    mov bl, al
    xor cx, cx
    mov cl, [hd_cds_count]
    jcxz .unsafe
    les di, [hd_cds_base]
.next:
    test word [es:di+43h], 3000h    ; splice or local/SUBST
    jz .advance
    cmp word [es:di+1], 5c3ah      ; ':\' canonical absolute prefix
    jne .unsafe
    mov al, [es:di]
    and al, 0dfh                    ; fold possible lowercase drive
    sub al, 'A'
    cmp al, [hd_cds_count]
    jae .unsafe
    cmp al, bl
    je .unsafe
.advance:
    add di, HD_CDS_SIZE
    loop .next
    clc
    jmp .done
.unsafe:
    stc
.done:
    pop es
    pop di
    pop cx
    pop bx
    pop ax
    ret

hd_dpl times 11 dw 0
hd_sda dd 0
hd_cds_base dd 0
hd_sft_head dd 0
hd_fcb_head dd 0
hd_default db 0
hd_physical db 0
hd_cds_count db 0
