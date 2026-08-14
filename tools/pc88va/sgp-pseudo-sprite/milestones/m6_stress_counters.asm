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
; THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
; WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
; MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
; EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
; SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
; WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
; OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
; ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

; M6 educational excerpt: bounded bullet stress and SGP transfer counters.
; The complete source keeps the same command-list format for every record.
SPRITE_MAX_COUNT       equ 256
BALL_COUNT             equ 16
BULLET_COUNT           equ 32
BULLET_FIRST_INDEX     equ BALL_COUNT

; Records 0..15 are shaded balls. Records 16..47 are small bullets. The
; remaining records deliberately reuse ball bitmaps to stress the same SGP
; path without allocating a second command format.
sprite_records:
    ; x, y, vx, vy, width, height, pitch, bitmap, priority
    ; ... the complete source emits BALL_COUNT ball records here ...
%assign bullet_index BULLET_FIRST_INDEX
%rep BULLET_COUNT
    dw ((bullet_index * 17) % (320 - 8)), ((bullet_index * 29) % (200 - 8))
    dw ((bullet_index * 3) % 5) - 2, ((bullet_index * 5) % 5) - 2
    dw 8, 8, 4, bullet_bitmap, bullet_index
%assign bullet_index bullet_index + 1
%endrep

; This is called after each emitted BITBLT. A transparent BITBLT still counts
; its full source rectangle; color-zero rejection happens inside the SGP.
count_bitblt_source:
    inc word [m6_sgp_bitblts_frame]
    mov ax, [current_sprite_width]
    mul word [current_sprite_height]
    add [m6_sgp_pixels_frame_lo], ax
    adc [m6_sgp_pixels_frame_hi], dx
    mov ax, [current_sprite_pitch]
    mul word [current_sprite_height]
    add [m6_sgp_source_bytes_frame_lo], ax
    adc [m6_sgp_source_bytes_frame_hi], dx
    ret

; The command list itself is counted from the final STOSW position. CLS and
; SET_* words are command traffic, while source pixels/bytes above describe
; the RAM-to-G1 BITBLT payload.
finish_m6_frame_counters:
    mov ax, di
    sub ax, sgp_command_list
    shr ax, 1
    mov [m6_sgp_commands_frame], ax
    call m6_accumulate_frame_counters
    ret

; A successful VBLANK exchange records one completed page flip and one frame.
record_completed_frame:
    inc word [m6_frames_lo]
    jnz .frame_ready
    inc word [m6_frames_hi]
.frame_ready:
    inc word [m6_page_flips_lo]
    jnz .flip_ready
    inc word [m6_page_flips_hi]
.flip_ready:
    ret
