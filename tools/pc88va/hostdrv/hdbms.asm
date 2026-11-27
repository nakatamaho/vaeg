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


; Run on the DOS profile with a fresh session and a child HOSTDRV.COM.
; Require the BMS-specific refusal followed by HDBMS PASS.

cpu 8086
bits 16
org 100h
    jmp start
%include 'hdprofile.inc'
start:
    cld
    cli
    mov sp, stack_top
    sti
    mov bx, (stack_top-$$+100h+15)/16
    mov ah, 4ah
    int 21h
    jc fail
    ; Consume free conventional RAM below the BMS window. DOS then
    ; allocates the child's environment and PSP above this block.
    xor bx,bx
    mov ah,48h
    int 21h
    jc fail
    cmp ax, 8000h
    jae fail
    mov es,ax
    mov bx,8000h
    sub bx,ax
    mov ah,4ah
    int 21h
    jc fail
    call hd_profile
    jc fail
    xor ax,ax
    mov es,ax
    mov ax,[es:0bch]
    mov [vector],ax
    mov ax,[es:0beh]
    mov [vector+2],ax
    xor ax,ax
    mov al,[hd_cds_count]
    mov cl,HD_CDS_SIZE
    mul cl
    mov [cds_len], ax
    lds si,[hd_cds_base]
    push cs
    pop es
    mov di, cds
    mov cx,ax
    rep movsb
    push cs
    pop ds
    mov [params+4],ds
    mov [params+8],ds
    mov [params+12],ds
    mov bx,params
    mov dx,child
    mov ax,4b00h
    int 21h
    push cs
    pop ds
    jc fail
    mov ah,4dh
    int 21h
    cmp ax,1
    jne fail
    xor ax,ax
    mov es,ax
    mov ax,[es:0bch]
    cmp ax,[vector]
    jne fail
    mov ax,[es:0beh]
    cmp ax,[vector+2]
    jne fail
    les di,[hd_cds_base]
    mov si,cds
    mov cx,[cds_len]
    cld
    repe cmpsb
    jne fail
    mov dx,ok
    mov ah,9
    int 21h
    mov ax,4c00h
    int 21h
fail:
    push cs
    pop ds
    mov dx,bad
    mov ah,9
    int 21h
    mov ax,4c01h
    int 21h
child db 'HOSTDRV.COM',0
tail db 6,' /VAEG',13
params dw 0,tail,0,fcb,0,fcb,0
fcb times 16 db 0
cds_len dw 0
vector dw 0,0
cds times 26*58h db 0
ok db 'HDBMS PASS',13,10,'$'
bad db 'M173_HOSTDRV_BMS_FIXTURE',13,10,'$'
times 512 db 0
stack_top:
