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
; One sampler/IUT address, verified live-RAM patches BEFORE the CALL, eight
; forward/reverse passes. NOP replaces the IUT only for the A/F control capture.
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
    ld a,ROW_COUNT
    ld (remaining),a
    ld hl,7008h
capture_loop:
    inc hl
    ld a,(hl)
    call install_seed
    inc hl
    ld a,(hl)
    cp 'S'
    ld a,37h
    jr z,opcode_ready
    ld a,3fh
opcode_ready:
    ld (row_opcode),a
    inc hl
    ld c,(hl)
    inc hl
    ld b,(hl)
    inc hl
    xor a
    call install_iut
control_call:
    call sampler
    ld (hl),e
    inc hl
    ld (hl),d
    inc hl
    ld a,(row_opcode)
    call install_iut
measured_call:
    call sampler
    ld (hl),e
    inc hl
    ld (hl),d
    inc hl
    ld a,(remaining)
    dec a
    ld (remaining),a
    jp nz,capture_loop
    ld hl,banner
    call puts
    ld hl,7008h
    ld b,ROW_COUNT
print_loop:
    ld a,'R'
    call char
    call space
    ld a,(hl)
    inc hl
    call hexbyte
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
    djnz print_loop
    ld hl,raw_label
    call puts
    ld a,4
    call char
    jp 2ah
install_seed:
    push hl
    ld e,a
    ld d,0
    ld hl,seed_templates
    add hl,de
    add hl,de
    add hl,de
    ld de,sampler_seed
    ld b,3
patch_loop:
    ld a,(hl)
    ld (de),a
    ex de,hl
patch_check:
    cp (hl)
    ex de,hl
    jp nz,patch_failed
    inc hl
    inc de
    djnz patch_loop
    pop hl
    ret
install_iut:
    ld (sampler_iut),a
    ld d,a
    ld a,(sampler_iut)
    cp d
    jp nz,patch_failed
    ret
sampler:
    di
sampler_seed:
    ld a,0
sampler_alu:
    xor a
    push bc
    pop af
sampler_iut:
    nop
    push af
    pop de
    ret
patch_failed:
    ld a,3
    rst 18h
    di
patch_halt:
    halt
    jr patch_halt
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
remaining: db 0
row_opcode: db 0
seed_templates:
    db 3eh,0,0afh,3eh,0,0b7h,3eh,8,0b7h,3eh,20h,0b7h,3eh,28h,0b7h
banner: db 'M140 FDC FIXED 1',13,10,0
raw_label: db 'M140_FIXED_RAW',13,10,0
initial_table:
TABLE_DATA:
