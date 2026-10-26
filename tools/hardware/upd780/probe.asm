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
; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
; AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
; IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
; ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
; LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
; CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
; SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
; INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
; CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
; ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
; POSSIBILITY OF SUCH DAMAGE.
;
; FDC RAM-resident raw AF matrix and separate timing kernels.
; Monitor-family ABI: RST 18h sends A; JP 2Ah returns to command service;
; 7F09h is the motor-active latch. Main restores all RAM below 7F00h.
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
    ld hl,values
    ld d,16
outer:
    ld b,(hl)
    ld ix,values
    ld e,16
row:
    push de
    ld c,(ix+0)
    ld a,'F'
    call char
    ld a,' '
    call char
    ld a,b
    call hexbyte
    ld a,c
    call hexbyte
    push bc
    pop af
    call afout
    push bc
    pop af
    bit 0,a
    call afout
    push bc
    pop af
    and 55h
    call afout
    push bc
    pop af
    scf
    call afout
    push bc
    pop af
    ccf
    call afout
    call newline
    pop de
    inc ix
    dec e
    jr nz,row
    inc hl
    dec d
    jr nz,outer
; TIMING_KERNELS inserted by build.py; no output inside the measured loops.
TIMING_KERNELS:
    ld hl,done_label
    call puts
    ld a,4
    call char
    jp 2ah
; These wrappers preserve the input registers, not undocumented internal Q.
afout:
    push af
    pop de
    ld a,d
    call hexbyte
    ld a,e
    jp hexbyte
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
newline:
    ld a,13
    call char
    ld a,10
    jp char
values: db 0,1,2,7,8,0fh,10h,20h,28h,40h,55h,7fh,80h,0aah,0d7h,0ffh
banner: db 'M139 FDC PROBE 1',13,10,'F: requested/readback AF, BIT/AND/SCF/CCF AF (hex)',13,10,0
done_label: db 'M139_FDC_PROBE_COMPLETE',13,10,0
