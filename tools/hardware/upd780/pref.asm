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
; Checked replacement fragments of capture.asm. Four CP/prefix templates,
; one sampler and caller position. No physical Q readout.
; @loop
capture_loop:
    inc hl
    ld a,(hl)
    call install_site
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
    ld (phase),a
    call install_iut
sample_dispatch:
    ld bc,(target)
    ld hl,(caller_target)
    jp (hl)
capture_return:
    call save_captures
    ld a,(phase)
    or a
    jr nz,post_return
    ld a,2
    call compare_captures
    ld hl,(record_ptr)
    call store_captures
    ld (record_ptr),hl
    ld a,1
    ld (phase),a
    ld a,(row_opcode)
    call install_iut
    jp sample_dispatch
post_return:
    ld a,4
    call compare_captures
    ld hl,(record_ptr)
    call store_captures
; @installer
install_site:
    push hl
    cp 4
    jp nc,patch_failed
    ld hl,sampler_template
    ld de,18
    or a
    jr z,template_ready
template_select:
    add hl,de
    dec a
    jr nz,template_select
template_ready:
    ld de,4800h
    ld b,18
    call patch_copy
    ld hl,4800h
    ld (sampler_target),hl
    ld hl,480ah
    ld (iut_target),hl
    ld hl,48feh
    ld (caller_target),hl
    ld de,48feh
    ld hl,caller_template
    ld b,12
    call patch_copy
    pop hl
    ret
patch_copy:
    ld a,(hl)
    ld (de),a
    ex de,hl
patch_check:
    cp (hl)
    ex de,hl
    jp nz,patch_failed
    inc hl
    inc de
    djnz patch_copy
    ret
install_iut:
    push hl
    ld hl,(iut_target)
    ld (hl),a
    cp (hl)
    jp nz,patch_failed
    pop hl
    ret
phase: db 0
sampler_target: dw 0
iut_target: dw 0
caller_target: dw 0
; @banks
    ds 4800h-$
sampler_low:
    ds 18
    ds 48feh-$
caller_low:
    ds 12
sampler_template:
TEMPLATES_DATA:
caller_template:
    ld de,0
    ld hl,0
    call 4800h
    jp capture_return
initial_table:
TABLE_DATA:
