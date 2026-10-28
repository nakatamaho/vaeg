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
; M140: all AF captures precede formatted output. 7000h holds the raw table,
; read by the main controller through monitor 0Bh BEFORE restoring FDC RAM.
; The fixed AF seed is not a proven physical Q reset or readout.
org 4000h
    di
    ld sp,7effh
    xor a
    out (0f8h),a
    ld (7f09h),a
    ld a,5
    call char
    ld hl,initial_table
    ld de,7000h
    ld bc,RAW_SIZE
    ldir
CAPTURES:
    ld hl,banner
    call puts
    ld hl,7008h
    ld b,ROW_COUNT
print_rows:
    ld a,'B'
    call char
    call space
    ld a,(hl)
    inc hl
    call hexbyte
    call space
    ld a,(hl)
    inc hl
    call char
    call space
    call nextword
    call space
    call nextword
    call space
    call nextword
    ld a,13
    call char
    ld a,10
    call char
    djnz print_rows
    ld hl,raw_label
    call puts
    ld a,4
    call char
    jp 2ah
nextword:
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
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
banner: db 'M140 FDC SEED 1',13,10,0
raw_label: db 'M140_SEED_RAW',13,10,0
initial_table:
TABLE_DATA:
