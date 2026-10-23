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
;
; z80asm 1.8. N88 disk BASIC caller reserves B000h-DFFFh with CLEAR ,&HAFFF.
; Assumes SCREEN 1, WIDTH 80,25, main RAM selected (no GVRAM/dictionary bank).
; Does not change CRTC or DMA. Two SPACE presses advance then restore/return.
; Interrupts are disabled while F000h banks are temporarily modified.

        org 0b000h
        jp start
results:
        db 0,0,0,0,0,0,0 ; B003: IN31, IN32, P0 char, P1 char, P0 IN32, P1 IN32, restored
saved_sp:
        dw 0
start:
        push af
        push bc
        push de
        push hl
        push ix
        push iy
        ld a,i
        di
        push af
        ld (saved_sp),sp
        ld sp,0bff0h
        in a,(031h)
        ld (results),a
        in a,(032h)
        ld (results+1),a
        xor a
        ld (results+6),a
        call bank0
        ld hl,0f000h
        ld de,0c000h
        ld bc,01000h
        ldir
        call bank1
        ld hl,0f000h
        ld de,0d000h
        ld bc,01000h
        ldir
        call bank0
        call clear_screen
        ld hl,0f3c8h
        ld de,pattern0
        call put_string
        call bank1
        call clear_screen
        ld hl,0f3c8h
        ld de,pattern1
        call put_string
        ld de,phase0
        call phase_both
        call bank0
        ld a,(0f3c8h)
        ld (results+2),a
        in a,(032h)
        ld (results+4),a
        call space
        ld de,phase1
        call phase_both
        call bank1
        ld a,(0f3c8h)
        ld (results+3),a
        in a,(032h)
        ld (results+5),a
        call space
        call bank0
        ld hl,0c000h
        ld de,0f000h
        ld bc,01000h
        ldir
        call bank1
        ld hl,0d000h
        ld de,0f000h
        ld bc,01000h
        ldir
        call bank0
        ld hl,0c000h
        call compare
        jp nz,finish
        call bank1
        ld hl,0d000h
        call compare
        jp nz,finish
        ld a,1
        ld (results+6),a
finish:
        ld a,(results+1)
        out (032h),a
        ld sp,(saved_sp)
        pop af
        jp po,disabled
        pop iy
        pop ix
        pop hl
        pop de
        pop bc
        pop af
        ei
        ret
disabled:
        pop iy
        pop ix
        pop hl
        pop de
        pop bc
        pop af
        ret
bank0:
        ld a,(results+1)
        and 0efh
        out (032h),a
        ret
bank1:
        ld a,(results+1)
        or 010h
        out (032h),a
        ret
; Verify all 4096 bytes of each CPU bank before returning to BASIC.
compare:
        ld de,0f000h
        ld bc,01000h
compare_loop:
        ld a,(de)
        cp (hl)
        ret nz
        inc hl
        inc de
        dec bc
        ld a,b
        or c
        jp nz,compare_loop
        ret
; Conventional transparent attributes: first column 0, remaining columns 80.
clear_screen:
        ld hl,0f3c8h
        ld c,25
row:
        ld b,80
characters:
        ld (hl),020h
        inc hl
        djnz characters
        ld (hl),0
        inc hl
        ld (hl),0e8h
        inc hl
        ld b,19
attributes:
        ld (hl),80
        inc hl
        ld (hl),0e8h
        inc hl
        djnz attributes
        dec c
        jp nz,row
        ld hl,0f440h
        ld de,instructions
        jp put_string
put_string:
        ld a,(de)
        or a
        ret z
        ld (hl),a
        inc hl
        inc de
        jp put_string
phase_both:
        push de
        call bank0
        ld hl,0f4b8h
        call put_string
        pop de
        call bank1
        ld hl,0f4b8h
        jp put_string
; SPACE is port 09h bit 6, active low. Require release before each new press.
space:
        in a,(09h)
        and 040h
        jp z,space
space_press:
        in a,(09h)
        and 040h
        jp nz,space_press
        ret
pattern0:
        db 'A: WRITTEN THROUGH CPU BANK 0 (TMODE CLEAR)',0
pattern1:
        db 'B: WRITTEN THROUGH CPU BANK 1 (TMODE SET)  ',0
instructions:
        db 'SPACE: ADVANCE PHASE, THEN SPACE AGAIN TO RESTORE AND RETURN',0
phase0:
        db 'PHASE 0: CPU TMODE=0. RECORD THE A/B LINE ABOVE.',0
phase1:
        db 'PHASE 1: CPU TMODE=1. RECORD THE A/B LINE ABOVE.',0
