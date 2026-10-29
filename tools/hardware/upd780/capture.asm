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
org 4000h
    di
    ld sp,7effh
    xor a
    out (0f8h),a
    ld (7f09h),a
    ld a,5
    call char
    call ram_test
    ld hl,initial_table
    ld de,7000h
    ld bc,RAW_SIZE
    ldir
    ld hl,(pre_count)
    ld (7008h),hl
    ld hl,(pre_first)
    ld (700ah),hl
    ld a,7fh
    ld (700ch),a
    ld a,ROW_COUNT
    ld (remaining),a
    ld hl,7014h
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
    ld (target),bc
    ld (record_ptr),hl
    xor a
    ld (record_status),a
    call install_iut
    ld bc,(target)
control_call:
    call sampler
    call save_captures
    ld a,2
    call compare_captures
    ld hl,(record_ptr)
    call store_captures
    ld (record_ptr),hl
    ld a,(row_opcode)
    call install_iut
    ld bc,(target)
measured_call:
    call sampler
    call save_captures
    ld a,4
    call compare_captures
    ld hl,(record_ptr)
    call store_captures
mirror_load:
    ld de,(cap_a)
    call store_word
    ld de,(cap_a)
    ld a,d
    cpl
    ld d,a
    ld a,e
    cpl
    ld e,a
    call store_word
    ld a,(record_status)
    ld (hl),a
    inc hl
    ld a,(remaining)
    dec a
    ld (remaining),a
    jp nz,capture_loop
    call delayed_scan
    ld hl,banner
    call puts
    ld hl,7014h
    ld b,ROW_COUNT
print_loop:
    ld a,'D'
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
    call space
    call nextword
    call space
    call nextword
    call space
    call nextword
    call space
    call nextword
    call space
    call nextword
    call space
    call nextword
    call space
    ld a,(hl)
    inc hl
    call hexbyte
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
save_captures:
    ; No stack scratch: preserve the three popped words in separate mailboxes.
    ld (cap_a),de
    ld (cap_b),bc
    ld (cap_c),hl
    ret
compare_captures:
    ; DE/BC/HL still hold the original three words; A selects the status bit.
    ld (compare_bit),a
    ld a,d
    cp b
    jr nz,capture_diff
    cp h
    jr nz,capture_diff
    ld a,e
    cp c
    jr nz,capture_diff
    cp l
    ret z
capture_diff:
    ld a,(compare_bit)
    ld b,a
    ld a,(record_status)
    or b
    ld (record_status),a
    ret
store_captures:
    ld de,(cap_a)
    call store_word
    ld de,(cap_b)
    call store_word
    ld de,(cap_c)
    jp store_word
store_word:
    ld (hl),e
    ld a,(hl)
    cp e
    call nz,store_diff
    inc hl
    ld (hl),d
    ld a,(hl)
    cp d
    call nz,store_diff
    inc hl
    ret
store_diff:
    ld a,(record_status)
    or 1
    ld (record_status),a
    ret
ram_test:
    ld ix,patterns
    ld a,6
    ld (pattern_remaining),a
pattern_loop:
    ld a,(ix+0)
    ld (pattern_value),a
    ld hl,7000h
    ld de,RAW_SIZE
pattern_fill:
    ld a,(pattern_value)
    ld (hl),a
    inc hl
    dec de
    ld a,d
    or e
    jr nz,pattern_fill
    ld hl,7000h
    ld de,RAW_SIZE
pattern_read:
    ld a,(pattern_value)
pattern_check:
    cp (hl)
    call nz,pre_failure
    inc hl
    dec de
    ld a,d
    or e
    jr nz,pattern_read
    inc ix
    ld a,(pattern_remaining)
    dec a
    ld (pattern_remaining),a
    jr nz,pattern_loop
    ld hl,7000h
    ld de,RAW_SIZE
address_fill:
    ld a,h
    xor l
    ld (hl),a
    inc hl
    dec de
    ld a,d
    or e
    jr nz,address_fill
    ld hl,7000h
    ld de,RAW_SIZE
address_read:
    ld a,h
    xor l
    cp (hl)
    call nz,pre_failure
    inc hl
    dec de
    ld a,d
    or e
    jr nz,address_read
    ret
pre_failure:
    push hl
    ld hl,(pre_count)
    ld a,h
    or l
    jr nz,pre_not_first
    pop hl
    ld (pre_first),hl
    push hl
    ld hl,0
pre_not_first:
    inc hl
    ld (pre_count),hl
    pop hl
    ret
delayed_scan:
    ld hl,7014h
    ld b,ROW_COUNT
scan_loop:
    push bc
    ld de,11
    add hl,de
    ld (scan_ptr),hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    ld bc,5
    add hl,bc
    ld a,(hl)
    cp e
    jr nz,scan_bad
    inc hl
    ld a,(hl)
    cp d
    jr nz,scan_bad
    inc hl
    ld a,(hl)
    cpl
    cp e
    jr nz,scan_bad
    inc hl
    ld a,(hl)
    cpl
    cp d
    jr z,scan_next
scan_bad:
    ld hl,(delayed_count)
    ld a,h
    or l
    jr nz,scan_not_first
    ld hl,(scan_ptr)
    ld (delayed_first),hl
    ld hl,0
scan_not_first:
    inc hl
    ld (delayed_count),hl
scan_next:
    ld hl,(scan_ptr)
    ld de,11
    add hl,de
    pop bc
    djnz scan_loop
    ld hl,(delayed_count)
    ld (700eh),hl
    ld hl,(delayed_first)
    ld (7010h),hl
    ret
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
record_status: db 0
compare_bit: db 0
pattern_remaining: db 0
pattern_value: db 0
target: dw 0
record_ptr: dw 0
cap_a: dw 0
cap_b: dw 0
cap_c: dw 0
pre_count: dw 0
pre_first: dw 0
delayed_count: dw 0
delayed_first: dw 0
scan_ptr: dw 0
patterns: db 0,0ffh,55h,0aah,29h,21h
seed_templates:
    db 3eh,0,0afh,3eh,0,0b7h,3eh,8,0b7h,3eh,20h,0b7h,3eh,28h,0b7h
banner: db 'M140 FDC CAPTURE 1',13,10,0
raw_label: db 'M140_CAPTURE_RAW',13,10,0
    ds 4800h-$
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
    push af
    push af
    pop hl
    pop bc
    pop de
    ret
initial_table:
TABLE_DATA:
