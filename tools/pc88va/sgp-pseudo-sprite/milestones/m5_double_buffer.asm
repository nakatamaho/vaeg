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

; M5 educational excerpt: hidden-page rendering and page flip.

; These starts were traced in VAEG's single-plane two-screen layout and in the
; repository's PC-88VA framebuffer documentation, not guessed constants.
G1_PAGE_A_SGP_BASE equ 220000h
G1_PAGE_B_SGP_BASE equ 227d00h
G1_PAGE_A_DSA      equ 020000h
G1_PAGE_B_DSA      equ 027d00h
PORT_FB1_DSA_LOW   equ 022eh
PORT_FB1_DSA_MID   equ 022fh
PORT_FB1_DSA_HIGH  equ 0230h
PORT_TSP_STATUS    equ 0142h
TSP_STATUS_VBLANK  equ 40h

; Each frame's SGP CLS and BITBLTs use draw_page_sgp_low/high. The current
; source's full command builder replaces the old fixed page address with these
; two words, so the CPU still never copies sprite pixels.
set_display_page_from_draw:
    mov dx, PORT_FB1_DSA_LOW
    mov ax, [draw_page_dsa_low]
    out dx, al
    inc dx
    mov al, ah
    out dx, al
    inc dx
    mov ax, [draw_page_dsa_high]
    out dx, al
    ret

flip_draw_page:
    call wait_vblank_start             ; wait low, then high on port 0142h
    jc .failed
    call set_display_page_from_draw   ; DSA1 changes only at VBLANK
    xor byte [draw_page_index], 1
    call set_draw_page_base            ; next frame renders the other page
    clc
    ret
.failed:
    stc
    ret

m5_frame_flow:
    call update_sprite_positions
    call render_sprite_frame            ; SGP renders hidden page and waits END
    call update_fps_counter
    call flip_draw_page                 ; VBLANK-synchronized display exchange
    jmp m5_frame_flow
