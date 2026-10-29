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
; OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
; OF THE POSSIBILITY OF SUCH DAMAGE.
;
; M140: no monitor/output, CALL, snapshot or dispatch between predecessor and
; SCF/CCF. Each generated straight-line span starts DI; XOR A; PUSH BC; POP AF.
; A separate identical-prefix control span captures pre-AF. It does NOT insert
; a PUSH/POP snapshot into the measured instruction adjacency. No Q rule assumed.
org 4000h
    di
    ld sp,7effh
    xor a
    out (0f8h),a
    ld (7f09h),a
    ld a,5
    call char
    ld hl,banner
    call puts
    ld hl,a_values
    ld d,4
outer:
    ld b,(hl)
    ld ix,f_values
    ld e,8
inner:
    ld c,(ix+0)
    push hl
    push de
    call run_histories
    pop de
    pop hl
    inc ix
    dec e
    jr nz,inner
    inc hl
    dec d
    jr nz,outer
    ld hl,done_label
    call puts
    ld a,4
    call char
    jp 2ah
run_histories:
HISTORY_CALLS:
    ret
; Print only after all three spans have captured their results in RAM.
printrow:
    ld (post_af),de
    ld (operation),a
    ld a,'Q'
    call char
    call space
    ld a,(history_id)
    add a,'0'
    call char
    call space
    ld a,(operation)
    call char
    call space
    ld a,b
    call hexbyte
    ld a,c
    call hexbyte
    call space
    ld de,(before_af)
    call wordout
    call space
    ld de,(post_af)
    call wordout
    ld a,13
    call char
    ld a,10
    jp char
wordout:
    ld a,d
    call hexbyte
    ld a,e
    jp hexbyte
space:
    ld a,' '
    jp char
puts:
    ld a,(hl)
    or a
    ret z
    inc hl
    call char
    jr puts
char:
    push af
    push bc
    push de
    push hl
    push ix
    push iy
    rst 18h
    pop iy
    pop ix
    pop hl
    pop de
    pop bc
    pop af
    ret
hexbyte:
    push af
    rrca
    rrca
    rrca
    rrca
    call nibble
    pop af
nibble:
    and 15
    add a,'0'
    cp '9'+1
    jr c,hexout
    add a,7
hexout:
    jp char
before_af: dw 0
scf_af: dw 0
ccf_af: dw 0
post_af: dw 0
history_id: db 0
operation: db 0
banner: db 'M140 FDC HISTORY 1 PROFILE',13,10,0
done_label: db 'M140_FDC_HISTORY_COMPLETE',13,10,0
HISTORY_DATA:
HISTORY_ROUTINES:
