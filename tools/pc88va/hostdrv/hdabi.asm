; Copyright (c) 2026 Nakata Maho
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions
; are met:
; 1. Redistributions of source code must retain the above copyright
;    notice, this list of conditions and the following disclaimer.
; 2. Redistributions in binary form must reproduce the above copyright
;    notice, this list of conditions and the following disclaimer in the
;    documentation and/or other materials provided with the distribution.
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

cpu 8086
bits 16
org 100h

; Transient synthetic redirector. No host ports, no TSR, no file backing.
; Exercise DOS APIs while the temporary CDS and vector are installed.
%define CDS_SIZE 58h
%define SDA_SFT 27eh
%define SDA_CDS 282h
%define SDA_PATH 292h
%define SDA_DMA 0ch
%define SDA_UID 1eh
%define SFT_REFS 0
%define SFT_FLAGS 5
%define SFT_SERIAL 0bh
%define SFT_SIZE 11h
%define SFT_POSITION 15h
%define SFT_NAME 20h
%define SFT_UID 2fh
%define SFT_IFS 37h

start:
    mov ax, 3000h
    int 21h
    cmp ax, 4
    jne unsupported
    mov ax, 1100h
    int 2fh
    test al, al
    jnz unsupported
    mov ax, 5d06h
    mov dx, dpl
    int 21h
    jc unsupported
    push ds
    push cs
    pop ds
    pop word [sda+2]
    mov [sda], si
    cmp cx, 78ch
    jne unsupported
    cmp dx, 1ah
    jne unsupported
    mov ax, si
    add ax, cx
    jc unsupported
    mov ah, 19h
    int 21h
    inc al
    mov [minimum], al
    mov ah, 52h
    int 21h
    mov al, [es:bx+20h]
    cmp al, [minimum]
    jbe .physical
    mov [minimum], al
.physical:
    mov al, [es:bx+21h]
    cmp al, 26
    ja unsupported
    cmp al, [minimum]
    jbe no_free_cds
    les di, [es:bx+16h]
    mov dl, al
    mov cl, CDS_SIZE
    mul cl
    add di, ax
    jc unsupported
    sub di, CDS_SIZE
    mov ax, di
    add ax, CDS_SIZE
    jc unsupported
    mov al, dl                    ; count backward from LASTDRIVE
.pick:
    dec al
    cmp al, [minimum]
    jb no_free_cds
    cmp word [es:di+43h], 0
    je .found
    sub di, CDS_SIZE
    jmp .pick
.found:
    add al, 'A'
    mov [file_path], al
    mov [bad_path], al
    mov [dir_path], al
    mov [find_path], al
    mov [cds], di
    mov [cds+2], es
    mov ax, 352fh
    int 21h
    mov [previous], bx
    mov [previous+2], es
    mov dx, redirector
    mov ax, 252fh
    int 21h

    push ds
    lds si, [cds]
    push cs
    pop es
    mov di, saved_cds
    mov cx, CDS_SIZE
    cld
    rep movsb
    pop ds
    les di, [cds]
    push di
    xor ax, ax
    mov cx, CDS_SIZE / 2
    rep stosw
    pop di
    mov al, [file_path]
    mov ah, ':'
    stosw
    mov byte [es:di], 5ch
    sub di, 2
    mov word [es:di+43h], 0c000h
    mov word [es:di+45h], remote_device
    mov [es:di+47h], cs
    mov word [es:di+4fh], 2
    mov byte [es:di+51h], 4
    mov word [es:di+52h], ifs_header
    mov [es:di+54h], cs

    mov byte [step], '1'
    mov ax, 1100h
    stc
    int 2fh
    jc finish
    cmp ax, 11ffh
    jne finish

    mov byte [step], '2'
    mov bl, [file_path]
    sub bl, 'A'-1
    mov ax, 4409h
    int 21h
    jc finish
    test dx, 1000h                 ; remote CDS via the IFS header
    jz finish
    mov dx, dir_path
    mov ah, 3bh
    int 21h
    jc finish
    les di, [cds]
    cmp word [es:di+3], 'SU'
    jne finish
    cmp word [es:di+5], 'B'
    jne finish

    mov byte [step], '3'
    mov dx, bad_path
    mov ax, 3d00h
    int 21h
    jnc unexpected_open
    cmp ax, 2
    jne finish

    mov byte [step], '4'
    mov dx, file_path
    mov ax, 3d00h
    int 21h
    jc finish
    mov [handle], ax
    mov ax, [slot]
    mov [first_slot], ax
    mov ax, [slot+2]
    mov [first_slot+2], ax
    mov bx, [handle]
    mov ax, 440ah
    int 21h
    jc finish
    test dx, 8000h                 ; DOS classified the SFT as remote
    jz finish
    mov bx, [handle]
    mov ax, 440ch                 ; generic handle IOCTL refuses remote IFS
    xor cx, cx
    xor dx, dx
    int 21h
    jnc finish
    cmp ax, 1
    jne finish
    mov bx, [handle]
    mov ah, 45h
    int 21h
    jc finish
    mov [duplicate], ax
    les di, [slot]
    cmp word [es:di+SFT_REFS], 2
    jne finish

    mov byte [step], '5'
    mov bx, [handle]
    mov cx, 4
    mov dx, data
    mov ah, 3fh
    int 21h
    jc finish
    cmp ax, 3
    jne finish
    cmp word [data], 'AB'
    jne finish
    cmp word [data+2], 'C'
    jne finish
    les di, [slot]
    cmp word [es:di+SFT_POSITION], 3
    jne finish

    mov byte [step], '6'
    mov byte [detached], 1
    mov bx, [handle]
    mov cx, 1
    mov dx, data
    mov ah, 3fh
    int 21h
    jnc finish
    mov byte [step], 'A'
    cmp ax, 5                     ; DOS maps not-ready to access denied for 3Fh
    jne finish
    xor bx, bx
    mov ax, 5900h
    int 21h
    cmp ax, 21                    ; original redirector status
    jne finish
    mov byte [step], 'B'
    mov bx, [handle]
    mov word [handle], -1
    mov ah, 3eh
    int 21h
    jc finish
    les di, [slot]
    mov byte [step], 'C'
    cmp word [es:di+SFT_REFS], 1
    jne finish
    mov byte [step], 'D'
    mov bx, [duplicate]
    mov word [duplicate], -1
    mov ah, 3eh
    int 21h
    jnc finish
    mov byte [step], 'E'
    cmp ax, 6
    jne finish
    mov byte [step], 'F'
    les di, [slot]
    cmp word [es:di+SFT_REFS], 0
    jne finish
    cmp byte [owned], 0
    jne finish

    mov byte [step], '7'
    mov byte [detached], 0
    mov dx, file_path
    mov ax, 3d00h
    int 21h
    jc finish
    mov [handle], ax
    les di, [slot]
    cmp di, [first_slot]
    jne finish
    mov ax, es
    cmp ax, [first_slot+2]
    jne finish
    cmp word [es:di+SFT_SERIAL], 2
    jne finish
    cmp word [es:di+SFT_POSITION], 0
    jne finish
    cmp word [es:di+SFT_POSITION+2], 0
    jne finish
    mov bx, [handle]
    mov word [handle], -1
    mov ah, 3eh
    int 21h
    jc finish
    cmp byte [owned], 0
    jne finish
    cmp byte [chained], 0
    je finish

    mov byte [step], '8'
    mov ah, 2fh
    int 21h
    mov [previous_dta], bx
    mov [previous_dta+2], es
    mov byte [dta_changed], 1
    mov dx, dta
    mov ah, 1ah
    int 21h
    mov dx, find_path
    xor cx, cx
    mov ah, 4eh
    int 21h
    jc finish
    cmp word [dta+1], 'HD'
    jne finish
    cmp byte [dta+1eh], 'F'
    jne finish
    push ds
    pop es
    mov si, dta
    mov di, dta_copy
    mov cx, 43
    cld
    rep movsb
    call next_result
    call check_end

    mov byte [step], '9'
    mov dx, dta_copy
    mov ah, 1ah
    int 21h
    call next_result
    call check_end
    mov byte [step], 0
    jmp finish

next_result:
    mov ah, 4fh
    int 21h
    jc .failed
    mov ah, 2fh
    int 21h
    cmp byte [es:bx+1eh], 'N'
    jne .failed
    ret
.failed:
    pop ax
    jmp finish

check_end:
    mov ah, 4fh
    int 21h
    jnc next_result.failed
    cmp ax, 18
    jne next_result.failed
    ret

unexpected_open:
    mov [handle], ax
finish:
    cmp byte [dta_changed], 0
    je .close
    push ds
    lds dx, [previous_dta]
    mov ah, 1ah
    int 21h
    pop ds
.close:
    mov bx, [duplicate]
    call cleanup_handle
    mov bx, [handle]
    call cleanup_handle
    les di, [cds]
    mov si, saved_cds
    mov cx, CDS_SIZE
    cld
    rep movsb
    push ds
    lds dx, [previous]
    mov ax, 252fh
    int 21h
    pop ds
    mov dx, passed
    cmp byte [step], 0
    je emit
    mov al, [step]
    mov [failure], al
    mov dx, failed
    jmp emit
no_free_cds:
    mov dx, exhausted
    mov byte [step], 254
    jmp emit
unsupported:
    mov dx, skipped
    mov byte [step], 255
emit:
    mov ah, 9
    int 21h
    mov ah, 4ch
    mov al, [step]
    int 21h
cleanup_handle:
    cmp bx, -1
    je .done
    mov ah, 3eh
    int 21h
.done:
    ret

redirector:
    pushf
    cmp ax, 1100h
    je .installation
    cmp ax, 1116h
    je .path
    cmp ax, 1105h
    je .path
    cmp ax, 111bh
    je .path
    cmp ax, 111ch
    je .find_next
    cmp ax, 1106h
    je .handle
    cmp ax, 1108h
    je .handle
    cmp ax, 1123h
    jne .chain
    inc byte [cs:chained]
.chain:
    popf
    jmp far [cs:previous]
.installation:
    mov al, 0ffh
    jmp .success
.path:
    push ds
    push si
    lds si, [cs:sda]
    push ax
    mov ax, [si+SDA_CDS]
    cmp ax, [cs:cds]
    jne .not_path
    mov ax, [si+SDA_CDS+2]
    cmp ax, [cs:cds+2]
    jne .not_path
    pop ax
    push bx
    mov bx, [si+SDA_PATH]
    mov si, [si+SDA_UID]
    push dx
    mov dx, [bx]
    cmp dx, [cs:file_path]
    pop dx
    jne .path_error
    cmp byte [bx+2], 5ch
    jne .path_error
    cmp al, 5
    je .chdir
    cmp al, 1bh
    je .find_first
    cmp word [bx+3], 'FI'
    jne .path_error
    cmp word [bx+5], 'LE'
    jne .path_error
    cmp word [bx+7], '.D'
    jne .path_error
    cmp word [bx+9], 'AT'
    jne .path_error
    cmp byte [bx+11], 0
    jne .path_error
    cmp byte [cs:owned], 0
    jne .path_error
    push es
    push di
    les di, [cs:sda]
    les di, [es:di+SDA_SFT]
    mov [cs:slot], di
    mov [cs:slot+2], es
    mov byte [es:di+4], 1
    mov word [es:di+SFT_FLAGS], 8000h
    inc word [cs:serial]
    mov bx, [cs:serial]
    mov [es:di+SFT_SERIAL], bx
    mov word [es:di+SFT_SIZE], 3
    mov word [es:di+SFT_SIZE+2], 0
    mov word [es:di+SFT_POSITION], 0
    mov word [es:di+SFT_POSITION+2], 0
    mov [es:di+SFT_UID], si
    mov word [es:di+SFT_IFS], ifs_header
    mov [es:di+SFT_IFS+2], cs
    add di, SFT_NAME
    push cs
    pop ds
    mov si, name83
    push cx
    mov cx, 11
    cld
    rep movsb
    pop cx
    pop di
    pop es
    mov byte [cs:owned], 1
    jmp .path_ok
.chdir:
    cmp word [bx+3], 'SU'
    jne .path_error
    cmp word [bx+5], 'B'
    jne .path_error
.path_ok:
    pop bx
    pop si
    pop ds
    jmp .success
.path_error:
    pop bx
    pop si
    pop ds
    mov ax, 2
    jmp .error
.not_path:
    pop ax
    pop si
    pop ds
    jmp .chain
.find_first:
    pop bx
    pop si
    pop ds
    push ds
    push si
    push es
    push di
    push cx
    lds si, [cs:sda]
    les di, [si+SDA_DMA]
    push di
    xor ax, ax
    mov cx, 53
    cld
    rep stosb
    pop di
    mov al, [cs:file_path]
    sub al, 'A'
    or al, 80h
    mov [es:di], al
    mov word [es:di+1], 'HD'
    mov word [es:di+3], 1         ; synthetic generation
    mov word [es:di+7], 1         ; synthetic search token
    mov word [es:di+11], 1        ; next ordinal
    mov si, name83
    jmp .find_record
.find_next:
    push ds
    push si
    push es
    push di
    push cx
    lds si, [cs:sda]
    les di, [si+SDA_DMA]
    mov al, [cs:file_path]
    sub al, 'A'
    or al, 80h
    cmp [es:di], al
    jne .not_search
    cmp word [es:di+1], 'HD'
    jne .not_search
    cmp word [es:di+3], 1
    jne .end_search
    cmp word [es:di+7], 1
    jne .end_search
    cmp word [es:di+11], 1
    jne .end_search
    inc word [es:di+11]
    mov si, next83
.find_record:
    add di, 21
    push cs
    pop ds
    mov cx, 11
    cld
    rep movsb
    pop cx
    pop di
    pop es
    pop si
    pop ds
    jmp .success
.end_search:
    pop cx
    pop di
    pop es
    pop si
    pop ds
    mov ax, 18
    jmp .error
.not_search:
    pop cx
    pop di
    pop es
    pop si
    pop ds
    mov ax, 111ch
    jmp .chain
.handle:
    cmp byte [cs:owned], 1
    jne .chain
    cmp di, [cs:slot]
    jne .chain
    push bx
    mov bx, es
    cmp bx, [cs:slot+2]
    pop bx
    jne .chain
    cmp al, 6
    je .close
    cmp byte [cs:detached], 0
    jne .offline
    cmp word [es:di+SFT_POSITION+2], 0
    jne .eof
    cmp word [es:di+SFT_POSITION], 0
    jne .eof
    cmp cx, 3                     ; only the controlled fixture read is valid
    jb .unsupported_read
    push ds
    push si
    push di
    push es
    lds si, [cs:sda]
    les di, [si+SDA_DMA]
    cmp di, 0fffdh
    ja .read_range
    mov word [es:di], 'AB'
    mov byte [es:di+2], 'C'
    pop es
    pop di
    pop si
    pop ds
    mov cx, 3
    mov [es:di+SFT_POSITION], cx
    jmp .success
.read_range:
    pop es
    pop di
    pop si
    pop ds
.unsupported_read:
    xor cx, cx
    mov ax, 31
    jmp .error
.eof:
    xor cx, cx
    jmp .success
.offline:
    xor cx, cx
    mov ax, 21
    jmp .error
.close:
    cmp word [es:di+SFT_REFS], 0
    je .bad_handle
    dec word [es:di+SFT_REFS]
    jnz .success
    mov byte [cs:owned], 0
    cmp byte [cs:detached], 0
    je .success
.bad_handle:
    mov ax, 6
.error:
    popf
    push bp
    mov bp, sp
    or word [bp+6], 1
    pop bp
    iret
.success:
    popf
    push bp
    mov bp, sp
    and word [bp+6], 0fffeh
    pop bp
    iret

ifs_header:
    dd -1
    db 'HDABI   '
    dw 4800h, 0
    dd 0
    dw 0
remote_device:
    dd -1
    dw 0                          ; SDEVATT as read by IOCTL 4409h
cds dd 0
sda dd 0
slot dd 0
first_slot dd 0
previous dd 0
previous_dta dd 0
dta_changed db 0
serial dw 0
handle dw -1
duplicate dw -1
owned db 0
detached db 0
minimum db 0
chained db 0
step db 0
file_path db 'X:',5ch,'FILE.DAT',0
bad_path db 'X:',5ch,'MISS.DAT',0
dir_path db 'X:',5ch,'SUB',0
find_path db 'X:',5ch,'*.DAT',0
name83 db 'FILE    DAT'
next83 db 'NEXT    DAT'
passed db 'HDABI PASS',13,10,'$'
failed db 'HDABI FAIL '
failure db '0',13,10,'$'
skipped db 'HDABI UNSUPPORTED',13,10,'$'
exhausted db 'HDABI NO FREE CDS',13,10,'$'
saved_cds times CDS_SIZE db 0
dpl times 11 dw 0
data dd 0
dta times 43 db 0
dta_copy times 43 db 0
