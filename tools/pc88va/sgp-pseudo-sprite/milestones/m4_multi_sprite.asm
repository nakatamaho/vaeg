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

; M4 educational excerpt: many animated SGP sprites; not a standalone COM.

SPRITE_X       equ 0
SPRITE_Y       equ 2
SPRITE_VX      equ 4
SPRITE_VY      equ 6
SPRITE_BITMAP  equ 14
SPRITE_SIZE    equ 18
SPRITE_COUNT   equ 16

; Records are already painter ordered. Later records are emitted later, so
; their nonzero pixels have priority while BITBLT zero pixels remain clear.
sprite_records:
    ; x, y, vx, vy, width, height, pitch, bitmap, priority
    dw 24, 92, 2, 0, 16, 16, 8, hsv_ball_00, 0
    dw 280, 92, -2, 0, 16, 16, 8, hsv_ball_01, 1
    ; ... fourteen further records in the complete M4 source ...

update_sprite_positions:
    mov si, sprite_records
    mov cx, [active_sprite_count]
.next:
    mov ax, [si + SPRITE_X]
    add ax, [si + SPRITE_VX]
    ; The complete source clamps at 0 and SCREEN_WIDTH-16 and reverses vx.
    mov [si + SPRITE_X], ax
    add si, SPRITE_SIZE
    loop .next
    ret

render_m4_frame:
    call build_sgp_frame_commands
    call run_sgp_command_list
    ret

; build_sgp_frame_commands emits CLS, then SET_SOURCE/SET_DEST/BITBLT for
; each active record, then FPS glyph BITBLTs and END. CPU never touches G1
; pixels; it only updates records and appends command words in RAM.
