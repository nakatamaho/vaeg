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

%define HD_MUX 0d7a0h
%define HD_TAG 4448h
%define HD_ABI 3
%define HD_VECTOR (2fh * 4)

    jmp transient

%include "hdresident.inc"

; Read-only classifier. No DOS or channel access on the caller stack.
hd_classify:
    mov ax, [es:bp+HD_R_AX]
    cmp ax, 1100h
    je .owned
    cmp ax, HD_MUX
    jne .path_verb
    cmp word [es:bp+HD_R_BX], HD_TAG
    jne .chain
    jmp .owned
.path_verb:
    cmp ax, 1105h
    je .cds
    cmp ax, 110fh
    je .cds
    cmp ax, 110ch
    je .cds
    cmp ax, 1101h
    je .cds
    cmp ax, 1103h
    je .cds
    cmp ax, 110eh
    je .cds
    cmp ax, 1111h
    je .cds
    cmp ax, 1113h
    je .cds
    cmp ax, 1117h
    je .cds
    cmp ax, 1116h
    je .cds
    cmp ax, 111bh
    je .cds
    cmp ax, 111ch
    je hd_classify_search
    cmp ax, 111dh
    je .owned
    jmp hd_classify_file
.cds:
    les di, [hd_sda]
    mov bx, [hd_cds]
    cmp [es:di+282h], bx          ; THISCDS, never direct/UNC
    jne .chain
    mov bx, [hd_cds+2]
    cmp [es:di+284h], bx
    jne .chain
.owned:
    clc
    ret
.chain:
    stc
    ret

hd_dispatch:
    cmp ax, 1100h
    jne .other
    mov byte [es:bp+HD_R_AX], 0ffh
    jmp .done
.other:
    cmp ax, HD_MUX
    jne hd_search_dispatch
.identify:
    mov word [es:bp+HD_R_AX], HD_MUX+1
    mov word [es:bp+HD_R_CX], HD_ABI
    mov [es:bp+HD_R_ES], cs
    mov word [es:bp+HD_R_DI], hd_identity
.done:
    and word [es:bp+HD_R_FLAGS], 0fffeh
    clc
    ret

; AX is the owned verb, ES:BP the immutable caller frame except for outputs.
hd_path_dispatch:
    cmp ax, 110ch
    je hd_disk_space
    cmp ax, 1105h
    je .stat
    cmp ax, 110fh
    je .stat
    mov ax, 5                     ; mutation: no port access
    jmp hd_return_error
.stat:
    cmp byte [hd_latched], 0
    jne .latched
    call hd_verify_tables
    jc .latched
    mov al, 1                     ; STAT
    mov cx, [hd_generation]
    mov dx, [hd_generation+2]
    call hd_prepare
    call hd_copy_path
    jc hd_return_error
    call hd_submit
    jc hd_return_error
    cmp word [es:bp+HD_R_AX], 1105h
    jne .attrs
    cmp ax, 2                     ; absent final component is a missing dir
    je .not_dir
    test ax, ax
    jnz hd_return_error
    test byte [hd_packet+44], 10h
    jz .not_dir
    jmp hd_dispatch.done
.attrs:
    test ax, ax
    jnz hd_return_error
    mov ax, [hd_packet+44]
    mov [es:bp+HD_R_AX], ax
    mov ax, [hd_packet+46]
    mov [es:bp+HD_R_CX], ax
    mov ax, [hd_packet+48]
    mov [es:bp+HD_R_DX], ax
    mov ax, [hd_packet+42]
    mov [es:bp+HD_R_BX], ax
    mov ax, [hd_packet+40]
    mov [es:bp+HD_R_DI], ax
    jmp hd_dispatch.done
.not_dir:
    mov ax, 3
    jmp hd_return_error
.latched:
    mov ax, 31
hd_return_error:
    mov [es:bp+HD_R_AX], ax
    or word [es:bp+HD_R_FLAGS], 1
    clc                           ; consume the interrupt, not the DOS CF
    ret

; Convert validated LE u64 capacity to 512-byte sectors, then to the
; smallest legal allocation unit that permits a 16-bit total.
hd_disk_space:
    cmp byte [hd_latched], 0
    jne hd_file_fault
    mov al, 2
    call hd_session_prepare
    mov word [hd_packet+28], 16
    call hd_submit
    jc hd_return_error
    test ax, ax
    jnz hd_return_error
    mov cx, 9
.to_sectors:
    call .halve
    loop .to_sectors
    mov bx, 1
.units:
    mov ax, [hd_packet+66]
    or ax, [hd_packet+68]
    or ax, [hd_packet+70]
    jz .fits
    cmp bx, 64
    je .capped
    shl bx, 1
    call .halve
    jmp .units
.fits:
    mov dx, [hd_packet+64]
    jmp .return
.capped:
    mov dx, 0ffffh
.return:
    mov bh, 0f8h
    mov [es:bp+HD_R_AX], bx
    mov [es:bp+HD_R_BX], dx
    mov word [es:bp+HD_R_CX], 512
    mov word [es:bp+HD_R_DX], 0
    jmp hd_dispatch.done
.halve:
    shr word [hd_packet+70], 1
    rcr word [hd_packet+68], 1
    rcr word [hd_packet+66], 1
    rcr word [hd_packet+64], 1
    ret

; Copy at most 127 canonical relative bytes from DOSGroup. Check each
; byte's span before consulting it; a path may end at a RAM boundary.
; Split only the fixed drive prefix here: the host parses CP932 components.
hd_copy_path:
    push es
    les di, [hd_sda]
    mov di, [es:di+292h]           ; WFP_START
    mov cx, 3
    call hd_dos_span
    jc .bad
    mov al, [hd_cds_image]
    cmp [es:di], al
    jne .bad
    cmp word [es:di+1], 5c3ah
    jne .bad
    add di, 3
    jz .bad
    mov bx, hd_packet+64
    mov cx, 1
.next:
    call hd_dos_span
    jc .bad
    mov al, [es:di]
    test al, al
    jz .complete
    cmp bx, hd_packet+64+127
    jae .bad
    mov [bx], al
    inc bx
    inc di
    jnz .next
.bad:
    pop es
    mov ax, 3
    stc
    ret
.complete:
    sub bx, hd_packet+64
    mov [hd_packet+22], bx
    mov [hd_packet+26], bx
    pop es
    clc
    ret

; Cached DOSGroup addresses were established with the public 65xx calls.
; Recheck the bytes before any path use; do not call DOS recursively.
hd_verify_tables:
    push es
    les di, [hd_file_case]
    cmp word [es:di], 128
    jne .bad
    add di, 2
    mov al, 80h
.case_byte:
    scasb
    jne .bad
    inc al
    jnz .case_byte
    les di, [hd_dbcs]
    cmp word [es:di], 6
    jne .bad
    cmp word [es:di+2], 9f81h
    jne .bad
    cmp word [es:di+4], 0fce0h
    jne .bad
    cmp word [es:di+6], 0
    jne .bad
    pop es
    clc
    ret
.bad:
    pop es
    stc
    ret

%include "hdwire.inc"
%include "hdpacket.inc"
%include "hdprofile.inc"
%include "hdfiles.inc"
%include "hdsearch.inc"

hd_identity db "HOSTDRV", HD_ABI
hd_generation dd 0
hd_cds dd 0
hd_drive db 0
hd_latched db 0
hd_file_case dd 0
hd_dbcs dd 0
hd_saved_cds times 58h db 0
hd_cds_image:
    db "X:", 5ch, 0
    times 43h-4 db 0
    dw 0c000h
    dw hd_device, 0
    dd 0
    dw 0, 2
    db 4
    dw hd_ifs, 0
    dw 0
hd_ifs:
    dd -1
    db "HOSTDRV "
    dw 4800h, 0
    dd 0
    dw 0
hd_device:
    dd -1
    dw 0

hd_packet times 4160 db 0
hd_resident_end:

; Exact opt-in (also required for uninstall). Optional single drive or /U.
transient:
    cld
    xor cx, cx
    mov cl, [80h]
    cmp cx, 127
    ja refuse
    mov si, 81h
    mov di, optin
    call skip_spaces
    mov bx, 5
.optin:
    jcxz refuse_near
    lodsb
    cmp al, [di]
    jne refuse_near
    inc di
    dec cx
    dec bx
    jnz .optin
    mov byte [requested], 0ffh
    jcxz options_done
    cmp byte [si], " "
    jne refuse_near
    call skip_spaces
    jcxz options_done
    cmp cx, 2
    jb refuse_near
    lodsw
    sub cx, 2
    cmp ax, "/U"
    jne .drive
    mov byte [remove], 1
    jmp .trailing
.drive:
    cmp ah, ":"
    jne refuse_near
    and al, 0dfh
    sub al, "A"
    cmp al, 26
    jae refuse_near
    mov [requested], al
.trailing:
    call skip_spaces
    jcxz options_done
refuse_near:
    jmp refuse

skip_spaces:
    jcxz .done
    cmp byte [si], " "
    jne .done
    inc si
    dec cx
    jmp skip_spaces
.done:
    ret

options_done:
    call hd_profile
    jc refuse_near
    push ds
    pop es
    mov di, hd_packet
    mov cx, 4160
    call hd_dos_span
    jc refuse_near
    mov ax, HD_MUX
    mov bx, HD_TAG
    int 2fh
    cmp byte [remove], 0
    jne uninstall
    cmp ax, HD_MUX+1
    je refuse_near
    mov ax, 1100h
    int 2fh
    test al, al
    jnz refuse_near
    call check_install_nls
    jc refuse_near
    mov al, [requested]
    call hd_pick_cds
    jc refuse_near
    mov [hd_cds], di
    mov [hd_cds+2], es
    mov [hd_drive], al
    add al, "A"
    mov [hd_cds_image], al
    mov [hd_cds_image+47h], cs
    mov [hd_cds_image+54h], cs

    mov ax, [2ch]
    test ax, ax
    jz .environment_done
    mov es, ax
    mov ah, 49h
    int 21h
    jc refuse_near
    mov word [2ch], 0
.environment_done:
    pushf
    cli
    mov al, [hd_drive]
    call hd_pick_cds
    jc .failed
    cmp di, [hd_cds]
    jne .failed
    mov ax, es
    cmp ax, [hd_cds+2]
    jne .failed
    call hd_discover
    jc .failed
    xor ax, ax
    xor cx, cx
    xor dx, dx
    call hd_prepare
    call hd_submit
    jc .failed
    test ax, ax
    jnz .failed
    mov cx, [hd_packet+8]
    mov dx, [hd_packet+10]
    mov [hd_generation], cx
    mov [hd_generation+2], dx
    mov al, 0ah
    call hd_prepare
    call hd_submit
    jc .attach_ambiguous
    test ax, ax
    jnz .failed

    ; Nothing fallible between confirmed ATTACH and publication.
    push ds
    lds si, [hd_cds]
    push cs
    pop es
    mov di, hd_saved_cds
    mov cx, HD_CDS_SIZE
    rep movsb
    pop ds
    les di, [hd_cds]
    mov si, hd_cds_image
    mov cx, HD_CDS_SIZE
    rep movsb
    xor ax, ax
    mov es, ax
    mov ax, [es:HD_VECTOR]
    mov [hd_previous], ax
    mov ax, [es:HD_VECTOR+2]
    mov [hd_previous+2], ax
    mov word [es:HD_VECTOR], hd_interrupt
    mov [es:HD_VECTOR+2], cs
    popf
    mov ax, 3100h
    mov dx, (hd_resident_end-$$+100h+15)/16
    int 21h                       ; this DOS profile does not return
    ; Unexpected return: do not leave the hook pointing into this block
    ; when DOS frees it. Leave the host guard in place for a full reset.
    push cs
    pop ds
    pushf
    cli
    cld
    call restore_publication
    popf
    jmp recovery
.attach_ambiguous:
    popf
    jmp recovery
.failed:
    popf
    jmp refuse

uninstall:
    cmp ax, HD_MUX+1
    jne refuse
    cmp cx, HD_ABI
    jne refuse
    cmp di, hd_identity
    jne refuse
    mov di, 100h
    mov cx, hd_resident_end-$$
    call hd_dos_span
    jc refuse
    mov di, hd_identity
    mov si, hd_identity
    mov cx, 8
    repe cmpsb
    jne refuse
    mov [tsr_segment], es
    mov al, [es:hd_drive]
    cmp al, [hd_default]
    je refuse
    cmp al, [hd_physical]
    jb refuse
    cmp al, [hd_cds_count]
    jae refuse
    call hd_check_dependencies
    jc refuse
    mov cl, HD_CDS_SIZE
    mul cl
    add ax, [hd_cds_base]
    cmp ax, [es:hd_cds]
    jne refuse
    mov ax, [hd_cds_base+2]
    cmp ax, [es:hd_cds+2]
    jne refuse

    pushf
    cli
    xor ax, ax
    mov es, ax
    cmp word [es:HD_VECTOR], hd_interrupt
    jne .not_top
    mov ax, [tsr_segment]
    cmp [es:HD_VECTOR+2], ax
    jne .not_top
    push ds
    mov ds, ax
    cmp byte [hd_resident_busy], 0
    jne .restore_ds
    cmp byte [hd_busy], 0
    jne .restore_ds
    cmp byte [hd_latched], 0
    jne .restore_ds
    cmp byte [hd_file_count], 0
    jne .restore_ds
    les di, [hd_cds]
    mov al, [hd_cds_image]
    cmp [es:di], al
    jne .restore_ds
    cmp word [es:di+1], 5c3ah
    jne .restore_ds
    add di, 43h
    mov si, hd_cds_image+43h
    mov cx, HD_CDS_SIZE-43h
    repe cmpsb
    jne .restore_ds
    call hd_drain_searches
    jc .restore_ds
    mov cx, [hd_generation]
    mov dx, [hd_generation+2]
    pop ds
    mov [hd_generation], cx
    mov [hd_generation+2], dx
    mov al, 0bh
    call hd_prepare
    call hd_submit
    jc .detach_ambiguous
    test ax, ax
    jz .revoked
    ; This host's validated DETACH for our former generation returns 21
    ; if there is no live attachment (e.g. after a warm reset).
    cmp ax, 21
    jne .blocked
.revoked:
    push ds
    mov ds, [tsr_segment]
    call restore_publication
    pop ds
    popf
    mov es, [tsr_segment]
    mov ah, 49h
    int 21h
    jc recovery
    mov al, 0ch
    mov cx, [hd_generation]
    mov dx, [hd_generation+2]
    call hd_prepare
    call hd_submit
    jc recovery
    test ax, ax
    jnz recovery
    mov dx, removed
    xor al, al
    jmp emit
.detach_ambiguous:
    mov es, [tsr_segment]
    mov byte [es:hd_latched], 1
    jmp .blocked
.restore_ds:
    pop ds
.blocked:
    popf
    jmp recovery
.not_top:
    popf
    jmp refuse

; Use DOS's public profile queries only in transient context. The installed
; tables must live in the profile's DOSGroup, not an external provider.
check_install_nls:
    mov ax, 6601h
    int 21h
    jc .bad
    cmp bx, 932
    jne .bad
    cmp dx, 932
    jne .bad
    mov ax, 6504h
    call .query
    jc .bad
    mov cx, 130
    call hd_dos_span
    jc .bad
    mov [hd_file_case], di
    mov [hd_file_case+2], es
    mov ax, 6507h
    call .query
    jc .bad
    mov cx, 8
    call hd_dos_span
    jc .bad
    mov [hd_dbcs], di
    mov [hd_dbcs+2], es
    cld
    jmp hd_verify_tables
.query:
    push ds
    pop es
    mov di, nls_reply
    mov [di], byte 0
    mov bl, al
    push bx
    mov bx, 0ffffh
    mov dx, bx
    mov cx, 5
    int 21h
    pop bx
    jc .bad
    cmp cx, 5
    jne .bad
    cmp [nls_reply], bl
    jne .bad
    les di, [nls_reply+1]
    mov ax, es
    cmp ax, [hd_sda+2]
    jne .bad
    clc
    ret
.bad:
    stc
    ret

; DS=resident segment, IF masked. Restore the saved image and old vector.
restore_publication:
    les di, [hd_cds]
    mov si, hd_saved_cds
    mov cx, HD_CDS_SIZE
    rep movsb
    xor ax, ax
    mov es, ax
    mov ax, [hd_previous]
    mov [es:HD_VECTOR], ax
    mov ax, [hd_previous+2]
    mov [es:HD_VECTOR+2], ax
    ret

refuse:
    mov dx, refused
    mov al, 1
    jmp emit
recovery:
    mov dx, reset_required
    mov al, 2
emit:
    push ax
    mov ah, 9
    int 21h
    pop ax
    mov ah, 4ch
    int 21h

optin db '/VAEG'
requested db 0
remove db 0
tsr_segment dw 0
nls_reply times 5 db 0
removed db 'HOSTDRV removed',13,10,'$'
refused db 'HOSTDRV refused; use /VAEG [X:] or /VAEG /U',13,10,'$'
reset_required db 'HOSTDRV transaction or cleanup not confirmed; full reset required',13,10,'$'

%if hd_resident_end-$$+100h > 12288
    %error HOSTDRV resident footprint exceeds 12 KiB
%endif
