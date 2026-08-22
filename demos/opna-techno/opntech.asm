; Copyright (c) 2026 Nakata Maho
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions are met:
; 1. Redistributions of source code must retain the above copyright notice,
;    this list of conditions and the following disclaimer.
; 2. Redistributions in binary form must reproduce the above copyright
;    notice, this list of conditions and the following disclaimer in the
;    documentation and/or other materials provided with the distribution.
;
; THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
; IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
; OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
; IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
; INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
; BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
; USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
; THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
; (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
; THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

; PC-88VA Music BIOS demonstration.
; The Music BIOS is the documented INT 8Bh interface to the YM2608 OPNA.
; This program uses FM play mode 5 and the documented Set note function
; (AH=06h).  The BIOS timer interrupt controls each note gate while a small
; VBLANK scheduler advances four independent musical streams.
; The four streams below are 30 bars at 60 quarter notes per minute: the
; permanent loop is therefore exactly two minutes in the documented model.

bits 16
org 0x100

%define MUSIC_BIOS_INT       0x8b
%define TEXT_BIOS_INT        0x83
%define KEYBOARD_BIOS_INT    0x82
%define QUEUE_BYTES          0x0400
%define QUEUE_PARAGRAPHS     ((QUEUE_BYTES * 6 + 15) / 16)
%define RHYTHM_DATA_BYTES    0x0200
%define RHYTHM_DATA_PARAGRAPHS ((RHYTHM_DATA_BYTES + 15) / 16)
%define RHYTHM_QUEUE_BYTES   0x0040
%define RHYTHM_QUEUE_PARAGRAPHS ((RHYTHM_QUEUE_BYTES + 15) / 16)
%define ALLOCATION_PARAGRAPHS (QUEUE_PARAGRAPHS + RHYTHM_DATA_PARAGRAPHS + RHYTHM_QUEUE_PARAGRAPHS)
%define TSP_STATUS_PORT       0x0142
%define TSP_STATUS_VBLANK     0x40

%macro MUSIC_CALL 0
    int MUSIC_BIOS_INT
    ; The documented interface has no DS return value.  Re-establish the COM
    ; data segment after every BIOS transition rather than relying on it.
    push cs
    pop ds
%endmacro

start:
    push cs
    pop ds
    push cs
    pop es

    ; Allocate the six-channel PLAY queues and the independent rhythm work
    ; areas outside the COM PSP.  Both Music BIOS initialization functions
    ; require the caller to retain these buffers for the playback lifetime.
    mov bx, ALLOCATION_PARAGRAPHS
    mov ah, 0x48
    int 0x21
    jc allocation_failed
    mov [allocation_segment], ax
    mov [queue_segment], ax
    add ax, QUEUE_PARAGRAPHS
    mov [rhythm_data_segment], ax
    add ax, RHYTHM_DATA_PARAGRAPHS
    mov [rhythm_queue_segment], ax
    mov es, [queue_segment]
    mov dx, QUEUE_BYTES
    mov al, 1                      ; asynchronous data mode
    xor ah, ah                    ; Music BIOS Initialize
    MUSIC_CALL

    ; Enable the documented OPNA six-FM-voice play mode.  This demo uses the
    ; direct Set note function, so it does not depend on a PLAY-data terminator.
    mov ah, 0x16
    mov dl, 0x05                  ; FM6 mode, documented for OPNA
    MUSIC_CALL
    mov ah, 0x08
    mov dl, 60                     ; 60 quarter notes/minute
    MUSIC_CALL
    mov ah, 0x13
    xor al, al                     ; documented BGM mode
    MUSIC_CALL

    ; Initialize2 is mandatory before using OPNA rhythm functions.  Its data
    ; and queue buffers are separate from the six FM PLAY queues above.
    mov ax, [rhythm_data_segment]
    mov ds, ax
    mov si, RHYTHM_DATA_BYTES
    mov es, [rhythm_queue_segment]
    mov dx, RHYTHM_QUEUE_BYTES
    mov ah, 0x1d
    MUSIC_CALL
    or al, al
    jnz rhythm_initialization_failed

    ; Music BIOS Set volume accepts 80h..BFh for OPNA rhythm level 0..63.
    mov ah, 0x12
    mov ch, 0xff
    mov dl, 0xa4
    MUSIC_CALL

    ; The BIOS initializes a default FM voice.  Set conservative per-channel
    ; levels so the four-part pattern remains clear on both VA2 and VAEG.
    mov ah, 0x12
    mov ch, 0
    mov dl, 56
    MUSIC_CALL
    mov ah, 0x12
    mov ch, 1
    mov dl, 48
    MUSIC_CALL
    mov ah, 0x12
    mov ch, 2
    mov dl, 40
    MUSIC_CALL
    mov ah, 0x12
    mov ch, 3
    mov dl, 52
    MUSIC_CALL

    mov word [bass_pointer], bass_data
    mov word [lead_pointer], lead_data
    mov word [arp_pointer], arp_data
    mov word [pulse_pointer], pulse_data
    mov word [sixteenth_index], 0

    mov si, message
    call text_puts

wait_escape:
    call poll_escape
    jc .exit
    call play_sixteenth
    call wait_sixteenth
    jmp wait_escape

.exit:
    mov ah, 0x02                  ; stop playback and reset Music BIOS work
    mov al, 1
    MUSIC_CALL
    mov es, [queue_segment]
    mov ah, 0x49                  ; release the DOS queue allocation
    int 0x21
    mov ax, 0x4c00
    int 0x21

allocation_failed:
    mov si, allocation_message
    call text_puts
    mov ax, 0x4c01
    int 0x21

rhythm_initialization_failed:
    mov si, rhythm_initialization_message
    call text_puts
    mov es, [allocation_segment]
    mov ah, 0x49
    int 0x21
    mov ax, 0x4c01
    int 0x21

; Text BIOS Putstr uses DS:SI and a NUL terminator.  DX bit 15 selects the
; current text attribute.  This is the PC-88VA display path; DOS INT 21h is
; deliberately limited to the COM program's allocation and termination.
text_puts:
    push ax
    push bx
    push cx
    push dx
    push si
    push di
    push bp
    push ds
    push es
    mov ah, 0x02
    mov dx, 0x8000
    int TEXT_BIOS_INT
    pop es
    pop ds
    pop bp
    pop di
    pop si
    pop dx
    pop cx
    pop bx
    pop ax
    ret

; Existing VA demos use the Keyboard BIOS primitive sense/get pair to obtain
; scan codes without DOS line input.  AH=00h is the documented ESC scan code.
; Carry set means that the caller should leave the loop.
poll_escape:
    mov ah, 0x0a
    int KEYBOARD_BIOS_INT
    push cs
    pop ds
    jc .no_key
    mov ah, 0x09
    int KEYBOARD_BIOS_INT
    push cs
    pop ds
    cmp ah, 0x00
    je .escape
.no_key:
    clc
    ret
.escape:
    stc
    ret

play_sixteenth:
    mov ax, [sixteenth_index]
    test al, 3
    jnz .skip_bass
    mov ch, 0
    mov si, [bass_pointer]
    mov bx, bass_data_end
    mov di, bass_data
    call play_stream_event
    mov [bass_pointer], si
.skip_bass:
    test word [sixteenth_index], 1
    jnz .skip_lead
    mov ch, 1
    mov si, [lead_pointer]
    mov bx, lead_data_end
    mov di, lead_data
    call play_stream_event
    mov [lead_pointer], si
.skip_lead:
    mov ch, 2
    mov si, [arp_pointer]
    mov bx, arp_data_end
    mov di, arp_data
    call play_stream_event
    mov [arp_pointer], si
    test word [sixteenth_index], 1
    jnz .advance
    mov ch, 3
    mov si, [pulse_pointer]
    mov bx, pulse_data_end
    mov di, pulse_data
    call play_stream_event
    mov [pulse_pointer], si
.advance:
    call play_rhythm_step
    inc word [sixteenth_index]
    cmp word [sixteenth_index], 480
    jb .done
    mov word [sixteenth_index], 0
.done:
    ret

; Rhythm was initialized through the Music BIOS.  Write register2 is the
; documented BIOS path to OPNA port 46h/47h registers.  The small pattern is
; deliberately independent from the four FM streams: bit 0 is the low drum,
; bit 1 is the snare-like voice, and bit 3 is the hat-like voice in VAEG's
; established OPNA rhythm order.  Hardware conformance of that instrument map
; remains a later real-VA check; this program does not access I/O ports itself.
play_rhythm_step:
    mov bx, [sixteenth_index]
    and bx, 0x000f
    mov dl, [rhythm_steps + bx]
    or dl, dl
    jz .done
    mov al, 0x10
    mov ah, 0x1e
    MUSIC_CALL
.done:
    ret

; DS:SI addresses a two-byte delayed-note record.  The format is identical to
; the direct Set note arguments: DH is key/rest and DL is length.  A normal
; sounding note has a clear MSB; the manual reserves 80h+note for a tied key.
play_stream_event:
    mov dh, [si]
    mov dl, [si + 1]
    add si, 2
    cmp si, bx
    jb .have_next
    mov si, di
.have_next:
    push si
    mov ah, 0x06
    MUSIC_CALL
    pop si
    ret

wait_sixteenth:
    mov cx, 15                     ; 15 fields at 60 Hz = one 1/16 note
.field:
    call wait_vblank_start
    loop .field
    ret

wait_vblank_start:
    mov dx, TSP_STATUS_PORT
.wait_not_vblank:
    in al, dx
    test al, TSP_STATUS_VBLANK
    jnz .wait_not_vblank
.wait_vblank:
    in al, dx
    test al, TSP_STATUS_VBLANK
    jz .wait_vblank
    ret

message:
    db "VA2 OPNA techno loop: 120 seconds; ESC exits.", 13, 10, 0
allocation_message:
    db "Unable to allocate the Music BIOS queue.", 13, 10, 0
rhythm_initialization_message:
    db "VA2 OPNA rhythm initialization failed.", 13, 10, 0

align 2, db 0
queue_segment:
    dw 0
allocation_segment:
    dw 0
rhythm_data_segment:
    dw 0
rhythm_queue_segment:
    dw 0

sixteenth_index:
    dw 0
bass_pointer:
    dw 0
lead_pointer:
    dw 0
arp_pointer:
    dw 0
pulse_pointer:
    dw 0

; Sixteenth-note rhythm masks.  The recurring hat, four-on-the-floor low drum,
; and backbeat voice make the FM phrase audibly rhythmic without depending on
; any emulator-only audio API.
rhythm_steps:
    db 0x09, 0x00, 0x08, 0x00
    db 0x0a, 0x00, 0x08, 0x00
    db 0x09, 0x00, 0x08, 0x00
    db 0x0a, 0x00, 0x08, 0x00

; A quarter note is 30h in the Music BIOS delayed-note format.  Each macro
; expands to one bar, and 30 bars at tempo 60 make a two-minute loop.
%macro BASS_C 0
    db 0x18, 0x30, 0x18, 0x30, 0x1b, 0x30, 0x18, 0x30
%endmacro

%macro BASS_F 0
    db 0x15, 0x30, 0x15, 0x30, 0x18, 0x30, 0x15, 0x30
%endmacro

%macro BASS_G 0
    db 0x1a, 0x30, 0x1a, 0x30, 0x1d, 0x30, 0x1a, 0x30
%endmacro

%macro BASS_A 0
    db 0x1b, 0x30, 0x1b, 0x30, 0x1f, 0x30, 0x1b, 0x30
%endmacro

%macro BASS_REST 0
    db 0x70, 0x30, 0x70, 0x30, 0x70, 0x30, 0x70, 0x30
%endmacro

%macro LEAD_C 0
    db 0x30, 0x18, 0x30, 0x18, 0x33, 0x18, 0x30, 0x18
    db 0x36, 0x18, 0x33, 0x18, 0x30, 0x18, 0x2d, 0x18
%endmacro

%macro LEAD_F 0
    db 0x2d, 0x18, 0x2d, 0x18, 0x30, 0x18, 0x35, 0x18
    db 0x38, 0x18, 0x35, 0x18, 0x30, 0x18, 0x2d, 0x18
%endmacro

%macro LEAD_G 0
    db 0x32, 0x18, 0x32, 0x18, 0x35, 0x18, 0x39, 0x18
    db 0x3c, 0x18, 0x39, 0x18, 0x35, 0x18, 0x32, 0x18
%endmacro

%macro LEAD_A 0
    db 0x33, 0x18, 0x33, 0x18, 0x37, 0x18, 0x3a, 0x18
    db 0x3e, 0x18, 0x3a, 0x18, 0x37, 0x18, 0x33, 0x18
%endmacro

%macro LEAD_REST 0
    db 0x70, 0x18, 0x70, 0x18, 0x70, 0x18, 0x70, 0x18
    db 0x70, 0x18, 0x70, 0x18, 0x70, 0x18, 0x70, 0x18
%endmacro

%macro ARP_C 0
    db 0x48, 0x0c, 0x4b, 0x0c, 0x4f, 0x0c, 0x4b, 0x0c
    db 0x48, 0x0c, 0x4b, 0x0c, 0x4f, 0x0c, 0x54, 0x0c
    db 0x4f, 0x0c, 0x4b, 0x0c, 0x48, 0x0c, 0x4b, 0x0c
    db 0x4f, 0x0c, 0x4b, 0x0c, 0x48, 0x0c, 0x45, 0x0c
%endmacro

%macro ARP_F 0
    db 0x45, 0x0c, 0x48, 0x0c, 0x4c, 0x0c, 0x48, 0x0c
    db 0x45, 0x0c, 0x48, 0x0c, 0x4c, 0x0c, 0x51, 0x0c
    db 0x4c, 0x0c, 0x48, 0x0c, 0x45, 0x0c, 0x48, 0x0c
    db 0x4c, 0x0c, 0x48, 0x0c, 0x45, 0x0c, 0x42, 0x0c
%endmacro

%macro ARP_G 0
    db 0x4a, 0x0c, 0x4d, 0x0c, 0x51, 0x0c, 0x4d, 0x0c
    db 0x4a, 0x0c, 0x4d, 0x0c, 0x51, 0x0c, 0x56, 0x0c
    db 0x51, 0x0c, 0x4d, 0x0c, 0x4a, 0x0c, 0x4d, 0x0c
    db 0x51, 0x0c, 0x4d, 0x0c, 0x4a, 0x0c, 0x47, 0x0c
%endmacro

%macro ARP_A 0
    db 0x4b, 0x0c, 0x4f, 0x0c, 0x52, 0x0c, 0x4f, 0x0c
    db 0x4b, 0x0c, 0x4f, 0x0c, 0x52, 0x0c, 0x57, 0x0c
    db 0x52, 0x0c, 0x4f, 0x0c, 0x4b, 0x0c, 0x4f, 0x0c
    db 0x52, 0x0c, 0x4f, 0x0c, 0x4b, 0x0c, 0x48, 0x0c
%endmacro

%macro ARP_REST 0
    times 16 db 0x70, 0x0c
%endmacro

%macro PULSE_BAR 0
    db 0x0c, 0x18, 0x70, 0x18, 0x0c, 0x18, 0x70, 0x18
    db 0x0c, 0x18, 0x70, 0x18, 0x0c, 0x18, 0x70, 0x18
%endmacro

%macro PULSE_REST 0
    times 8 db 0x70, 0x18
%endmacro

bass_data:
    ; Intro (4), drive (8), bridge (4), break (2), final (12) = 30 bars.
    BASS_C
    BASS_C
    BASS_F
    BASS_G
%rep 2
    BASS_C
    BASS_C
    BASS_F
    BASS_G
%endrep
    BASS_A
    BASS_F
    BASS_C
    BASS_G
    BASS_REST
    BASS_REST
%rep 3
    BASS_C
    BASS_C
    BASS_F
    BASS_G
%endrep
bass_data_end:

lead_data:
%rep 4
    LEAD_REST
%endrep
%rep 2
    LEAD_C
    LEAD_C
    LEAD_F
    LEAD_G
%endrep
    LEAD_A
    LEAD_F
    LEAD_C
    LEAD_G
    LEAD_REST
    LEAD_REST
%rep 3
    LEAD_C
    LEAD_C
    LEAD_F
    LEAD_G
%endrep
lead_data_end:

arp_data:
    ARP_C
    ARP_C
    ARP_F
    ARP_G
%rep 2
    ARP_C
    ARP_C
    ARP_F
    ARP_G
%endrep
    ARP_A
    ARP_F
    ARP_C
    ARP_G
    ARP_REST
    ARP_REST
%rep 3
    ARP_C
    ARP_C
    ARP_F
    ARP_G
%endrep
arp_data_end:

pulse_data:
%rep 16
    PULSE_BAR
%endrep
    PULSE_REST
    PULSE_REST
%rep 12
    PULSE_BAR
%endrep
pulse_data_end:
