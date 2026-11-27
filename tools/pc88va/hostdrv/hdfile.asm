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

; Transient regression: expects DATA.BIN on E: with bytes 00h..FFh
; repeated 20 times. Does not install or touch ports directly.
bits 16
cpu 8086
org 100h
    ; Keep the fixture's saved INT 21h frame in its own allocation,
    ; not at the top of the conventional address space.
    mov sp,stack_top
    mov dx,path
    mov ax,3d40h
    int 21h
    jc bad
    mov bx,ax
    mov di,xa
    xor cx,cx
    mov ax,5702h
    int 21h
    jc bad
    cmp cx,2
    jne bad
    cmp word [xa],0a55ah
    jne bad
    mov ax,5702h
    int 21h
    jc bad
    cmp cx,2
    jne bad
    cmp word [xa],0
    jne bad
    cmp word [xa+2],05aa5h
    jne bad
    push es
    mov ax,8000h
    mov es,ax
    xor di,di
    mov ax,5702h
    int 21h
    pop es
    jnc bad
    cmp ax,1                    ; DOS remaps 112Dh general failure for 57h
    jne bad
    push bx
    xor bx,bx
    mov ah,59h
    int 21h
    pop bx
    cmp ax,31                   ; retained extended error
    jne bad
    mov di,xa
    mov ax,5704h
    int 21h
    jnc bad
    cmp ax,1
    jne bad
    push bx
    xor bx,bx
    mov ah,59h
    int 21h
    pop bx
    cmp ax,5                    ; extended error distinguishes write denial
    jne bad
    inc byte [step]
    mov ah,45h
    int 21h
    jc bad
    mov [dup],ax
    mov ah,3eh
    int 21h
    jc bad
    mov bx,[dup]
    inc byte [step]
    mov ah,68h
    int 21h
    jc bad
    mov dx,buf
    mov cx,6000
    mov ah,3fh
    int 21h
    jc bad
    cmp ax,5120
    jne bad
    mov si,buf
    mov cx,5120
    xor ax,ax
.loop:
    cmp [si],al
    jne bad
    inc al
    inc si
    loop .loop
    inc byte [step]
    cwd
    xor cx,cx
    mov ax,4201h
    int 21h
    jc bad
    cmp ax,5120
    jne bad
    test dx,dx
    jnz bad
    mov dx,buf
    mov cx,1
    mov ah,3fh
    int 21h
    jc bad
    test ax,ax
    jnz bad
    inc byte [step]
    mov cx,-1
    mov dx,-1
    mov ax,4202h
    int 21h
    jc bad
    cmp ax,5119
    jne bad
    mov dx,buf
    mov cx,1
    mov ah,3fh
    int 21h
    jc bad
    cmp ax,1
    jne bad
    cmp byte [buf],255
    jne bad
    inc byte [step]
    mov ah,3eh
    int 21h
    jc bad
    mov dx,path
    mov ax,3d01h
    int 21h
    jnc bad
    cmp ax,5
    jne bad
    inc byte [step]
    mov si,path
    mov bx,40h
    mov cx,0                    ; sentinel, not the expected action
    mov dx,11h                  ; open existing, create missing
    mov ax,6c00h
    int 21h
    jc bad
    cmp cx,1
    jne bad
    mov bx,ax
    mov ah,3eh
    int 21h
    jc bad
    inc byte [step]
    mov si,missing
    mov bx,40h
    xor cx,cx
    mov dx,1
    mov ax,6c00h
    int 21h
    jnc bad
    cmp ax,2
    jne bad
    mov si,missing
    mov bx,40h
    xor cx,cx
    mov dx,11h
    mov ax,6c00h
    int 21h
    jnc bad
    cmp ax,5
    jne bad
    mov dx,pass
    jmp emit
bad:
    mov dx,fail
emit:
    mov ah,9
    int 21h
    mov ax,4c00h
    int 21h
path db 'E:\DATA.BIN',0
missing db 'E:\NOFILE.BIN',0
dup dw 0
xa dw 0a55ah,05aa5h
pass db 'HDFILE PASS',13,10,'$'
fail db 'HDFILE FAIL '
step db '1',13,10,'$'
buf times 6000 db 0
align 2, db 0
stack_space times 512 db 0
stack_top:
