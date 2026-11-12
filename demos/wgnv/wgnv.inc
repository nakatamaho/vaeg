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
;
; wgnv.inc (M148): Melco WGN-V2/V4 (S3 Trio64V+, C bus) helpers for the
; PC-88VA probes. Needs ../b3e02/b3e02.inc. Board facts are from an
; independent reading of the Melco Windows 95 driver set (WGN-V series
; Ver 2.012), docs/agents/tasks/M148, section 9:
;   51E1h+n  id C3h/C4h, 59E1h+n  type 04h/F4h, n = 0, 2, .. 0Eh
;   53E0h/53E1h, 54E0h/54E1h, 55E0h/55E1h (+n): board data/index pairs
;   "E bank" mode: a 32KB window at E0000h; the VGA/S3 registers appear
;   in it at their port offsets (3C0h-3DAh) while the 54E0h data bits
;   3-2 are 00b, VRAM while they are 10b.

    jmp wgnv_inc_end

wv_n        dw 0                    ; board offset n
wv_id       db 0
wv_type     db 0
p53e0       dw 0
p53e1       dw 0
p54e0       dw 0
p54e1       dw 0
p55e0       dw 0
p55e1       dw 0
e_bank_in   db 0

; CF clear when a board answers: wv_n, wv_id, wv_type and the ports set.
wv_find:
    pusha
    xor bx, bx
.l:
    mov dx, 0x51e1
    or dx, bx
    in al, dx
    cmp al, 0xc3
    je .id
    cmp al, 0xc4
    jne .next
.id:
    mov ah, al
    mov dx, 0x59e1
    or dx, bx
    in al, dx
    cmp al, 0x04
    je .found
    cmp al, 0xf4
    je .found
.next:
    add bx, 2
    cmp bx, 0x10
    jb .l
    popa
    stc
    ret
.found:
    mov [wv_n], bx
    mov [wv_id], ah
    mov [wv_type], al
    mov ax, 0x53e0
    or ax, bx
    mov [p53e0], ax
    inc ax
    mov [p53e1], ax
    mov ax, 0x54e0
    or ax, bx
    mov [p54e0], ax
    inc ax
    mov [p54e1], ax
    mov ax, 0x55e0
    or ax, bx
    mov [p55e0], ax
    inc ax
    mov [p55e1], ax
    popa
    clc
    ret

; AH = index, AL = data: OUT [p54e1],AH; OUT [p54e0],AL.
breg54:
    push dx
    push ax
    mov dx, [p54e1]
    mov al, ah
    out dx, al
    pop ax
    mov dx, [p54e0]
    out dx, al
    pop dx
    ret

; AH = index, AL = data for the 53E0h pair.
breg53:
    push dx
    push ax
    mov dx, [p53e1]
    mov al, ah
    out dx, al
    pop ax
    mov dx, [p53e0]
    out dx, al
    pop dx
    ret

; AL = 00h (registers) or 08h (VRAM): 54E0h bits 3-2 with the current
; index, as the driver does.
e_mode:
    push dx
    push ax
    mov dx, [p54e0]
    mov ah, al
    in al, dx
    and al, 0xf3
    or al, ah
    out dx, al
    pop ax
    pop dx
    ret

; Interrupts off, E0000h-EFFFFh to the slot (152h bit 3), ES = E000h.
e_open:
    push ax
    push dx
    cli
    mov dx, PORT_ROMBANK
    in al, dx
    mov [e_bank_in], al
    or al, 0x08
    out dx, al
    mov ax, 0xe000
    mov es, ax
    pop dx
    pop ax
    ret

; 152h back, ES = DS, interrupts on.
e_close:
    push ax
    push dx
    mov dx, PORT_ROMBANK
    mov al, [e_bank_in]
    out dx, al
    push ds
    pop es
    sti
    pop dx
    pop ax
    ret

; In the window (e_open, register mode): AH = CRTC index, AL = data.
cr_write:
    push ax
    xchg al, ah
    mov [es:0x3d4], ax
    pop ax
    ret

; AH = CRTC index -> AL.
cr_read:
    mov [es:0x3d4], ah
    mov al, [es:0x3d5]
    ret

wgnv_inc_end:
