; Copyright (c) 2026 Nakata Maho
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions are met:
; 1. Redistributions of source code must retain the above copyright notice,
;    this list of conditions and the following disclaimer.
; 2. Redistributions in binary form must reproduce the above copyright notice,
;    this list of conditions and the following disclaimer in the documentation
;    and/or other materials provided with the distribution.
; THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
; WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
; MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
; IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
; SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
; WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
; OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
; OF THE POSSIBILITY OF SUCH DAMAGE.

%ifndef VOYAGE_AUDIO
%define VOYAGE_AUDIO 1
%endif
%if VOYAGE_AUDIO != 0 && VOYAGE_AUDIO != 1
%error "VOYAGE_AUDIO must be 0 or 1"
%endif

%include "opna_score.inc"

; Exclusive OPNA and first 4096 bytes of sound DRAM. No recording, hooks,
; prescaler writes or automatic fallback to the three-channel OPN device.
audio_init:
%if VOYAGE_AUDIO == 0
    ret
%else
    pusha
    mov byte [audio_present], 1
    mov dx, 44h
    call opna_ready
    jc .done
    mov dx, 46h
    call opna_ready
    jc .done
    ; Both read-only status windows passed before any address/data write.
    mov dx, 44h
    mov al, 7
    out dx, al
    call opna_ready
    jc .done
    inc dx
    in al, dx
    and al, 0c0h
    mov [mixer_directions], al
    mov ax, 3027h
    call opn_write
    mov ax, 8029h           ; six FM voices; no enabled interrupt sources.
    call opn_write
    xor bx, bx
.channel:
    mov ax, 14b0h           ; feedback 2 / algorithm 4.
    call voice_write
    mov ah, [fm_pan+bx]
    mov al, 0b4h
    call voice_write
    mov si, fm_parameters
    mov cx, (fm_parameters_end-fm_parameters)/2
.operator:
    lodsw
    cmp al, 40h
    jb .parameter
    cmp al, 50h
    jae .attack
    cmp bl, 2
    jb .parameter
    add ah, 16             ; chord/echo/shimmer quieter than lead/bass.
    jmp .parameter
.attack:
    cmp al, 60h
    jae .parameter
    cmp bl, 2
    jb .parameter
    mov ah, 12h
.parameter:
    call voice_write
    loop .operator
    inc bx
    cmp bx, 6
    jb .channel
    ; Mute inherited voices before enabling our three tone lanes.
    call mute_channels
    mov al, 7
    mov ah, [mixer_directions]
    or ah, 38h             ; all tones enabled, all noise disabled.
    call opn_write
    mov ax, 0100h          ; reset ADPCM before taking its memory ownership.
    call opna_write_high
    mov ax, 0c201h         ; L/R, 8-bit DRAM, not ROM.
    call opna_write_high
    mov ax, 8010h          ; clear old flags.
    call opna_write_high
    mov ax, 1310h          ; mask timer/zero flags; expose BRDY and EOS.
    call opna_write_high
    mov ax, 0002h
    call opna_write_high
    mov ax, 0003h
    call opna_write_high
    mov ax, 7f04h          ; inclusive stop and limit: units 0..127.
    call opna_write_high
    mov ax, 0005h
    call opna_write_high
    mov ax, 7f0ch
    call opna_write_high
    mov ax, 000dh
    call opna_write_high
    mov ax, 6000h          ; CPU -> external sound DRAM, never recording.
    call opna_write_high
    mov si, opna_samples
    mov cx, opna_samples_end-opna_samples
.upload:
    call adpcm_ready
    jc .done
    lodsb
    mov ah, al
    mov al, 8
    call opna_write_high
    jc .done
    inc word [adpcm_bytes_uploaded]
    loop .upload
    mov ax, 0100h
    call opna_write_high
    mov ax, ((OPNA_DELTA_N & 255) << 8) | 9
    call opna_write_high
    mov ax, ((OPNA_DELTA_N >> 8) << 8) | 0ah
    call opna_write_high
    mov ax, 0aa0bh
    call opna_write_high
.done:
    popa
    ret
%endif

music_service:
    pushf
    pusha
    cmp byte [running], 0
    je .done
    mov ax, [tick]
    cmp ax, [music_tick]
    je .done
    mov dx, ax
    sub dx, [music_tick]
    cmp word [music_tick], 0ffffh
    je .first
    test dx, dx
    jns .decay
    add dx, PERIOD
.decay:
    xor ax, ax
    mov al, [pulse]
    sub ax, dx
    jns .pulse
    xor ax, ax
.pulse:
    mov [pulse], al
    xor bx, bx
.envelope:
    xor ax, ax
    mov al, [ssg_volumes+bx]
    sub ax, dx
    jns .volume
    xor ax, ax
.volume:
    mov [ssg_volumes+bx], al
    mov ah, al
    mov al, 8
    add al, bl
    call opn_write
    inc bx
    cmp bx, 3
    jb .envelope
.first:
    mov ax, [tick]
    mov [music_tick], ax
    mov bx, 128
    mul bx
    mov bx, PERIOD
    div bx
    cmp ax, [music_step]
    je .done
    mov [music_step], ax
    mov bp, ax
    mov si, ax
    shl si, 1
    xor bx, bx
.fm:
    cmp bl, 0
    jne .not_bass
    test bp, 1
    jnz .next_fm
.not_bass:
    cmp bl, 2
    jb .note
    cmp bl, 4
    jne .pad
    test bp, 1
    jz .next_fm
    jmp .note
.pad:
    test bp, 15
    jnz .next_fm
.note:
    mov di, bx
    shl di, 1
    mov di, [opna_fm_lanes+di]
    add di, si
    mov ax, [di]
    call play_note
.next_fm:
    inc bx
    cmp bx, 6
    jb .fm
    xor bx, bx
.ssg:
    mov di, bx
    shl di, 1
    mov di, [opna_ssg_lanes+di]
    add di, si
    mov cx, [di]
    mov al, bl
    shl al, 1
    mov ah, cl
    call opn_write
    inc al
    mov ah, ch
    and ah, 15
    call opn_write
    mov ah, [ssg_peaks+bx]
    mov [ssg_volumes+bx], ah
    mov al, 8
    add al, bl
    call opn_write
    inc bx
    cmp bx, 3
    jb .ssg
    mov bx, bp
    xor ax, ax
    mov al, [opna_drum_score+bx]
    test al, al
    jz .done
    mov byte [pulse], 20
    call adpcm_trigger
.done:
    popa
    popf
    ret

; BX=0..5. All key codes use low bank; pitch uses selected voice bank.
play_note:
    push ax
    push cx
    mov cx, ax
    mov al, 28h
    mov ah, [fm_keys+bx]
    call opn_write
    mov al, 0a4h
    mov ah, ch
    call voice_write
    mov al, 0a0h
    mov ah, cl
    call voice_write
    mov al, 28h
    mov ah, [fm_keys+bx]
    or ah, 0f0h
    call opn_write
    push di
    mov di, bx
    shl di, 1
    inc word [fm_note_calls+di]
    pop di
    pop cx
    pop ax
    ret

; AX=1..3 original sample. Reset (not repeat) before changing start/end.
adpcm_trigger:
    cmp byte [audio_present], 0
    je .done
    dec ax
    mov bx, ax
    shl bx, 1
    shl bx, 1
    mov ax, 0100h
    call opna_write_high
    mov cx, [opna_sample_units+bx]
    mov al, 2
    mov ah, cl
    call opna_write_high
    mov al, 3
    mov ah, ch
    call opna_write_high
    mov cx, [opna_sample_units+bx+2]
    mov al, 4
    mov ah, cl
    call opna_write_high
    mov al, 5
    mov ah, ch
    call opna_write_high
    mov ax, 8010h
    call opna_write_high
    mov ax, 0a000h         ; external memory one-shot playback.
    call opna_write_high
    jc .done
    inc word [adpcm_triggers]
.done:
    ret

mute_channels:
    xor bx, bx
.fm:
    mov al, 28h
    mov ah, [fm_keys+bx]
    call opn_write
    inc bx
    cmp bx, 6
    jb .fm
    mov ax, 0008h
    call opn_write
    mov ax, 0009h
    call opn_write
    mov ax, 000ah
    call opn_write
    ret

audio_stop:
    pusha
    call mute_channels
    mov ax, 0100h
    call opna_write_high
    mov ax, 000bh
    call opna_write_high
    mov ax, 8010h
    call opna_write_high
    mov ax, 1f10h
    call opna_write_high
    mov al, 7
    mov ah, [mixer_directions]
    or ah, 3fh
    call opn_write
    mov ax, 3027h
    call opn_write
    popa
    ret

; AL is the base operator/channel register; add local voice index.
voice_write:
    push ax
    push dx
    add al, bl
    mov dx, 44h
    cmp bl, 3
    jb .write
    sub al, 3
    mov dx, 46h
.write:
    call opna_bank_write
    pop dx
    pop ax
    ret
opn_write:
    push dx
    mov dx, 44h
    call opna_bank_write
    pop dx
    ret
opna_write_high:
    push dx
    mov dx, 46h
    call opna_bank_write
    pop dx
    ret

; AL=register, AH=value, DX=status/address. Guard BOTH address and data.
opna_bank_write:
    cmp byte [audio_present], 0
    je .disabled
    push ax
    call opna_ready
    jc .done
    out dx, al
    call opna_ready
    jc .done
    inc dx
    mov al, ah
.data_write:
    out dx, al
    dec dx
    clc
.done:
    pop ax
    ret
.disabled:
    stc
    ret

opna_ready:
    push ax
    push cx
    mov cx, 4000h
.poll:
    call clock_poll
    in al, dx
    cmp al, 0ffh
    je .failed
    test al, 80h
    jz .ready
    loop .poll
.failed:
    mov byte [audio_present], 0
    mov byte [audio_error], 1
    stc
    jmp .done
.ready:
    clc
.done:
    pop cx
    pop ax
    ret

adpcm_ready:
    cmp byte [audio_present], 0
    je .disabled
    push ax
    push cx
    push dx
    mov cx, 4000h
    mov dx, 46h
.poll:
    call clock_poll
    in al, dx
    cmp al, 0ffh
    je .failed
    test al, 80h
    jnz .next
    test al, 8
    jnz .ready
.next:
    loop .poll
.failed:
    mov byte [audio_present], 0
    mov byte [audio_error], 2
    stc
    jmp .done
.ready:
    clc
.done:
    pop dx
    pop cx
    pop ax
    ret
.disabled:
    stc
    ret

; Diagnostic snapshots expose attempted note dispatch, not audible synthesis.
; Present=1/error=0 plus the actual bank-write counter proves guarded dispatch.
; All caller registers are restored; no I/O or clock ownership here.
audio_observe:
    pusha
    mov ax, [adpcm_bytes_uploaded]
    mov bx, [adpcm_triggers]
    mov cl, [audio_present]
    mov ch, [audio_error]
    mov dx, [fm_note_calls]
    mov si, [fm_note_calls+2]
    mov di, [fm_note_calls+4]
    mov bp, [fm_note_calls+6]
.snapshot:
    nop
    mov ax, [fm_note_calls+8]
    mov bx, [fm_note_calls+10]
    mov cl, [ssg_volumes]
    mov ch, [ssg_volumes+1]
    xor dx, dx
    mov dl, [ssg_volumes+2]
    mov si, [music_step]
    mov di, [tick]
    mov bp, [loops_completed]
.more:
    nop
    popa
    ret

fm_keys: db 0,1,2,4,5,6
fm_pan: db 0c0h,80h,0c0h,40h,40h,80h
ssg_peaks: db 9,7,8
ssg_volumes: db 0,0,0
fm_parameters:
    db 30h,02h,34h,01h,38h,01h,3ch,01h
    db 40h,32h,44h,28h,48h,24h,4ch,1ch
    db 50h,1fh,54h,1fh,58h,1fh,5ch,1fh
    db 60h,08h,64h,08h,68h,08h,6ch,08h
    db 70h,04h,74h,04h,78h,04h,7ch,04h
    db 80h,5ah,84h,5ah,88h,5ah,8ch,5ah
    db 90h,00h,94h,00h,98h,00h,9ch,00h
fm_parameters_end:
audio_present: db 0
mixer_directions: db 0
audio_error: db 0
pulse: db 0
align 2, db 0
music_tick: dw 0ffffh
music_step: dw 0ffffh
adpcm_bytes_uploaded: dw 0
adpcm_triggers: dw 0
fm_note_calls: times 6 dw 0
