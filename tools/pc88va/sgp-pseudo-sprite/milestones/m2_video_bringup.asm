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
; IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
; OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
; IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
; INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
; BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
; USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
; ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
; (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
; THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;
; M2 educational excerpt: video bring-up only; not a standalone COM.

; The complete program supplies the BIOS wrapper, palette table, and
; draw_g0_checkerboard routine. No SGP command is needed for this gate.

bits 16
org 0x100

%define VIDEO_BIOS_INT        0x8f
%define MODE_320X200_G0_G1   0xe00e
%define PIXEL_SIZE_4BPP      0x0404
%define COMPOSE_G1_OVER_G0   0x0034
%define PORT_G0_TRANSPARENCY 0x0124
%define PORT_G1_TRANSPARENCY 0x0126
%define PORT_MEMORY_MAP      0x0153
%define PORT_GVRAM_WRITE_MODE 0x0580
%define MEMORY_MAP_SINGLE    0x54
%define GVRAM_CPU_WRITE     0x10

initialize_m2_video:
    mov bx, MODE_320X200_G0_G1
    mov cx, PIXEL_SIZE_4BPP
    xor dx, dx
    xor ax, ax
    int VIDEO_BIOS_INT                 ; AH=00h: enter 320x200, G0/G1, 4 bpp
    test ax, ax
    jnz .failed

    ; Palette setup (the complete source calls the 08xx palette BIOS function).
    ; G0 stays opaque; G1 index zero reveals G0 at composition time.
    mov ax, 0x0300
    mov cx, COMPOSE_G1_OVER_G0
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed

    mov dx, PORT_G0_TRANSPARENCY
    xor ax, ax
    out dx, ax
    mov dx, PORT_G1_TRANSPARENCY
    mov ax, 0x0001
    out dx, ax

    mov dx, PORT_MEMORY_MAP
    mov al, MEMORY_MAP_SINGLE
    out dx, al
    mov dx, PORT_GVRAM_WRITE_MODE
    mov al, GVRAM_CPU_WRITE_MODE
    out dx, al

    call draw_g0_checkerboard          ; CPU bring-up write, G0 only
    mov ax, 0x0b01
    int VIDEO_BIOS_INT                  ; enable display after initialization
    ret

.failed:
    stc
    ret
