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
; CP/M controller for the PC-88 intelligent FDC monitor (0B/0C/0D).
; No BDOS disk operation is permitted between backup and verified restoration.
; Constants PAYLOAD_SIZE, LOGNAME and the embedded image are supplied by build.py.
org 100h
    jp start
start:
    ld (oldsp),sp
    ld sp,main_stack_top
    ld hl,(6)
    ld de,workspace_end
    or a
    sbc hl,de
    jp c,no_space
    ld a,i
    push af
    pop hl
    ld (oldiff),hl
    ld de,fcb
    ld c,15
    call 5
    inc a
    jp nz,exists
    ld hl,logbuf
    ld (logptr),hl
    ld de,logbuf+1
    ld bc,12287
    ld (hl),1ah
    ldir
    in a,(40h)
    and 20h
    ld (lastsync),a
    di
    ; Both 8255s use input A, output B, output upper C.
    ld a,91h
    out (0ffh),a
    xor a
    out (0feh),a
    ld a,0bh
    call command
    ld hl,4000h
    ld de,3f00h
    call parameters
    ld hl,backup
    ld de,3f00h
backup_loop:
    call receive
    ld (hl),a
    inc hl
    dec de
    ld a,d
    or e
    jr nz,backup_loop
    ld a,0ch
    call command
    ld hl,4000h
    ld de,3f00h
    call parameters
    ld hl,payload
    ld de,PAYLOAD_SIZE
upload_loop:
    ld a,(hl)
    call send
    inc hl
    dec de
    ld a,d
    or e
    jr nz,upload_loop
    ; Deterministic initial RAM, identical to the mapped host worker. Never
    ; expose stale firmware buffer bytes to the diagnostic CRC or scratch.
    ld de,3f00h-PAYLOAD_SIZE
clear_loop:
    xor a
    call send
    dec de
    ld a,d
    or e
    jr nz,clear_loop
    ld a,0dh
    call command
    ld a,40h
    call send
    xor a
    call send
stream:
    call receive
    cp 5
    jr z,long_wait_ready
    cp 4
    jr z,restore
    cp 1
    jr z,begin_time
    cp 2
    jr z,end_time
    call emit
    jr stream
long_wait_ready:
    ld a,1
    ld (streaming),a
    jr stream
begin_time:
    ld hl,0
    ld (edges),hl
    xor a
    ld (edgeoverflow),a
    inc a
    ld (timing),a
    jr stream
end_time:
    xor a
    ld (timing),a
    ld a,(edgeoverflow)
    or a
    jp nz,timer_wrap
    ld hl,(edges)
    push hl
    ld a,' '
    call emit
    ld a,'E'
    call emit
    ld a,'='
    call emit
    pop hl
    ld a,h
    call hexbyte
    ld a,l
    call hexbyte
    ld a,13
    call emit
    ld a,10
    call emit
    jr stream
restore:
    xor a
    ld (streaming),a
    ld a,0ch
    call command
    ld hl,4000h
    ld de,3f00h
    call parameters
    ld hl,backup
    ld de,3f00h
restore_loop:
    ld a,(hl)
    call send
    inc hl
    dec de
    ld a,d
    or e
    jr nz,restore_loop
    ld a,0bh
    call command
    ld hl,4000h
    ld de,3f00h
    call parameters
    ld hl,backup
    ld de,3f00h
verify_loop:
    call receive
    cp (hl)
    jp nz,fatal
    inc hl
    dec de
    ld a,d
    or e
    jr nz,verify_loop
    call enable
    ld hl,restored_msg
    call emit_message
    ; Only now may BDOS touch the disk.
    ld de,fcb
    ld c,22
    call 5
    inc a
    jp z,log_fail
    ld a,1
    ld (fileopen),a
    ld hl,(logptr)
    ld (hl),1ah
    inc hl
    ld (logptr),hl
    ld hl,logbuf
save_loop:
    push hl
    ex de,hl
    ld c,26
    call 5
    ld de,fcb
    ld c,21
    call 5
    pop hl
    or a
    jp nz,log_fail
    ld de,128
    add hl,de
    ld de,(logptr)
    push hl
    or a
    sbc hl,de
    pop hl
    jr c,save_loop
    ld de,fcb
    ld c,16
    call 5
    inc a
    jp z,log_fail
    xor a
    ld (fileopen),a
finish:
    ; CP/M CCP assumes default DMA at 0080h on transient return.
    ld de,80h
    ld c,26
    call 5
    ld sp,(oldsp)
    ret
parameters:
    ld a,h
    call send
    ld a,l
    call send
    ld a,d
    call send
    ld a,e
    jp send
command:
    call short_budget
    push af
    ld a,0fh
    out (0ffh),a
    call wait_rfd
    ld a,0eh
    out (0ffh),a
    pop af
send:
    call short_budget
    push af
    call wait_rfd
    pop af
    out (0fdh),a
    ld a,09h
    out (0ffh),a
wait_dac:
    call tick
    in a,(0feh)
    and 4
    jr z,wait_dac
    ld a,08h
    out (0ffh),a
wait_no_dac:
    call tick
    in a,(0feh)
    and 4
    jr nz,wait_no_dac
    ret
wait_rfd:
    call tick
    in a,(0feh)
    and 2
    jr z,wait_rfd
    ret
receive:
    call short_budget
    ld a,(streaming)
    or a
    jr z,receive_short
    push hl
    ld hl,1000h
    ld (budget+2),hl
    pop hl
receive_short:
    ld a,0bh
    out (0ffh),a
wait_dav:
    call tick
    in a,(0feh)
    and 1
    jr z,wait_dav
    ld a,0ah
    out (0ffh),a
    in a,(0fch)
    push af
    ld a,0dh
    out (0ffh),a
wait_no_dav:
    call tick
    in a,(0feh)
    and 1
    jr nz,wait_no_dav
    ld a,0ch
    out (0ffh),a
    pop af
    ret
; VRTC transition counts, NOT microseconds/MHz. The physical reference
; frequency must be bound separately. Both edges are counted.
short_budget:
    push af
    push hl
    ld hl,0
    ld (budget),hl
    ld hl,4
    ld (budget+2),hl
    pop hl
    pop af
    ret
tick:
    push af
    push hl
    ld hl,budget
    ld a,(hl)
    sub 1
    ld (hl),a
    inc hl
    ld a,(hl)
    sbc a,0
    ld (hl),a
    inc hl
    ld a,(hl)
    sbc a,0
    ld (hl),a
    inc hl
    ld a,(hl)
    sbc a,0
    ld (hl),a
    jp c,timeout
    in a,(40h)
    and 20h
    ld hl,lastsync
    cp (hl)
    jr z,tick_no_edge
    ld (hl),a
    ld hl,(edges)
    inc hl
    ld (edges),hl
    ld a,h
    or l
    jr nz,tick_no_edge
    ld a,(timing)
    ld (edgeoverflow),a
tick_no_edge:
    pop hl
    pop af
    ret
emit:
    push hl
    push de
    push bc
    push af
    ld hl,(logptr)
    ld de,logend-128
    or a
    sbc hl,de
    jp nc,log_overflow
    ld hl,(logptr)
    pop af
    ld (hl),a
    inc hl
    ld (logptr),hl
    ld e,a
    ld c,2
    call 5
    pop bc
    pop de
    pop hl
    ret
emit_message:
    ld a,(hl)
    cp '$'
    ret z
    inc hl
    call emit
    jr emit_message
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
    jp emit
enable:
    ld hl,(oldiff)
    bit 2,l
    ret z
    ei
    ret
no_space:
    ld de,space_msg
    jr message
exists:
    ld de,exists_msg
    jr message
log_fail:
    ld a,(fileopen)
    or a
    jr z,log_fail_closed
    ld de,fcb
    ld c,16
    call 5
    xor a
    ld (fileopen),a
log_fail_closed:
    ld de,log_msg
message:
    ld c,9
    call 5
    jp finish
log_overflow:
    ld de,capacity_msg
    jr fatal_common
timeout:
    ld de,timeout_msg
    jr fatal_common
timer_wrap:
    ld de,timer_msg
    jr fatal_common
fatal:
    ld de,fatal_msg
fatal_common:
    ; Unknown FDC execution/recovery state: never attempt disk I/O/warm boot.
    ld sp,main_stack_top
    ld c,9
    call 5
    di
fatal_halt:
    halt
    jr fatal_halt
oldsp: dw 0
oldiff: dw 0
edges: dw 0
lastsync: db 0
streaming: db 0
timing: db 0
edgeoverflow: db 0
budget: dw 0,0
logptr: dw 0
fileopen: db 0
space_msg: db 'M139_TPA_SPACE',13,10,'$'
exists_msg: db 'M139_LOG_EXISTS: rename existing output first',13,10,'$'
log_msg: db 'M139_LOG_WRITE_FAILED',13,10,'$'
fatal_msg: db 'M139_FDC_RECOVERY_FAILED: reset machine; no disk I/O',13,10,'$'
capacity_msg: db 'M139_LOG_CAPACITY: reset machine; no disk I/O',13,10,'$'
timeout_msg: db 'M139_FDC_TIMEOUT: reset machine; no disk I/O',13,10,'$'
timer_msg: db 'M139_TIMER_WRAP: reset machine; no disk I/O',13,10,'$'
restored_msg: db 'M139_FDC_RAM_RESTORED',13,10,'$'
fcb: db 0
    db LOGNAME
    ds 24
payload:
    incbin 'payload.bin'
main_stack:
    ds 512
main_stack_top:
image_end:
backup: equ image_end
logbuf: equ backup+3f00h
logend: equ logbuf+12288
workspace_end: equ logend
