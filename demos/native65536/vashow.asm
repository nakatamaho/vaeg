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

; VASHOW.COM (M149): a 640x200 65536-colour picture on the PC-88VA's own
; graphics, Graphic 0 single-plane 16 bpp (G6 R5 B5 words), MS-DOS 4.0.
;
;   VASHOW [file]
;
; file   640 x 200 words, little endian, rows top to bottom (256000 bytes);
;        default: the first *.G16 file of the current directory, e.g.
;        TETRA.G16 or MANDEL.G16 made by build-d88.py.
;
; The picture is read in 32 KiB pieces into a buffer after the program and
; copied to GVRAM through the bank-4 window at A0000h. The memory map is
; switched to GVRAM only for each copy and restored before every DOS call.
; A key restores the video state the program found.

bits 16
cpu 186
org 0x100

%define VIDEO_BIOS_INT          0x8f
%define VIDEO_BIOS_DATA_SEG     0x0338
%define VIDEO_MODE_OFFSET       0x000f
%define VIDEO_G0_BPP_OFFSET     0x0011
%define VIDEO_G1_BPP_OFFSET     0x0012

%define PORT_GRMODE             0x0100
%define PORT_GRRES              0x0102
%define PORT_PALETTE_COMPOSE    0x0106
%define PORT_RGB_COMPOSE        0x0108
%define PORT_MEMORY_MAP         0x0153
%define PORT_FB0_FBW            0x0204
%define PORT_FB0_FBL            0x0206
%define PORT_FB0_DOT            0x0208
%define PORT_FB0_OFX            0x020a
%define PORT_FB0_OFY            0x020c
%define PORT_FB0_DSA_LOW        0x020e
%define PORT_FB0_DSA_HIGH       0x0210
%define PORT_FB0_DSH            0x0212
%define PORT_FB0_DSP            0x0216
%define PORT_GVRAM_WRITE_MODE   0x0580

%define MODE_640X200_G0_ONLY    0xa002
%define PIXEL_SIZE_G0_16BPP     0x0010
%define COMPOSE_G0_ONLY         0x0003
%define GRMODE_200_SINGLE       0xb462      ; as the 320x200 65536-colour demos
%define GRRES_640_16BPP         0x0303      ; G0 (and G1) 640 dots, 16 bpp
%define COMPOSE_G0_DIRECT       0x0008
%define MEMORY_MAP_GVRAM_SINGLE 0x54
%define GVRAM_CPU_WRITE_MODE    0x10

%define SCREEN_WIDTH            640
%define SCREEN_HEIGHT           200
%define SCREEN_PITCH            1280
%define PICTURE_BYTES           256000
%define CHUNK_BYTES             0x8000

    jmp main

msg_usage   db 'VASHOW (M149) [file]: 640x200 65536-colour picture (G6 R5 B5 words).', 13, 10, '$'
msg_open    db 'VASHOW: cannot open the picture file.', 13, 10, '$'
msg_none    db 'VASHOW: no *.G16 picture in the current directory.', 13, 10, '$'
msg_memory  db 'VASHOW: not enough memory for the 32 KiB buffer.', 13, 10, '$'
msg_video   db 'VASHOW: the video BIOS refused the 640x200 16-bpp mode.', 13, 10, '$'
msg_short   db 'VASHOW: the picture file is shorter than 256000 bytes.', 13, 10, '$'
msg_done    db 'VASHOW: video state restored.', 13, 10, '$'
g16_mask    db '*.G16', 0

align 2, db 0
handle      dw 0
buf_seg     dw 0
gv_seg      dw 0
remaining_lo dw 0
remaining_hi dw 0
saved_video_mode dw 0
saved_g0_bpp db 0
saved_g1_bpp db 0
saved_memory_map db 0
saved_write_mode db 0
saved_single_plane db 0
video_mode_changed db 0
line_doubling_set db 0
read_failed db 0
fname       times 80 db 0
dta         times 43 db 0

align 2, db 0
g0_framebuffer_descriptor:
    dw 16, SCREEN_WIDTH, SCREEN_HEIGHT
g0_window_descriptor:
    dw 0, 0, SCREEN_HEIGHT, 0, 0

main:
    push cs
    pop ds
    push cs
    pop es
    cld
    call parse
    jnc .args
    mov dx, msg_usage
    jmp fail1
.args:
    cmp byte [fname], 0
    jne .named
    ; no file given: the first *.G16 of the current directory
    mov dx, dta
    mov ah, 0x1a
    int 0x21
    mov dx, g16_mask
    xor cx, cx
    mov ah, 0x4e
    int 0x21
    pushf
    mov dx, 0x80                    ; back to the PSP's default DTA
    mov ah, 0x1a
    int 0x21
    popf
    jnc .found
    mov dx, msg_none
    jmp fail2
.found:
    mov si, dta + 0x1e
    mov di, fname
.fn:
    lodsb
    stosb
    or al, al
    jnz .fn
.named:
    ; the buffer is the 64 KiB segment after the program
    mov ax, cs
    add ax, 0x1000
    mov [buf_seg], ax
    add ax, CHUNK_BYTES / 16
    cmp [0x0002], ax
    jae .memory_ok
    mov dx, msg_memory
    jmp fail2
.memory_ok:
    mov dx, fname
    mov ax, 0x3d00
    int 0x21
    jnc .opened
    mov dx, msg_open
    jmp fail2
.opened:
    mov [handle], ax
    mov word [remaining_lo], PICTURE_BYTES & 0xffff
    mov word [remaining_hi], PICTURE_BYTES >> 16
    mov word [gv_seg], 0xa000
    call save_video_state
    call initialize_video
    jnc .load
    call close_file
    call restore_video_state
    mov dx, msg_video
    jmp fail2

.load:
    ; CX = min(remaining, CHUNK_BYTES)
    mov cx, CHUNK_BYTES
    cmp word [remaining_hi], 0
    jne .size_ok
    cmp [remaining_lo], cx
    jae .size_ok
    mov cx, [remaining_lo]
.size_ok:
    push cx
    push ds
    mov bx, [handle]
    mov ds, [cs:buf_seg]
    xor dx, dx
    mov ah, 0x3f
    int 0x21
    pop ds
    pop cx
    jc .short
    cmp ax, cx
    jne .short
    call copy_to_gvram              ; CX bytes from buf_seg:0
    sub [remaining_lo], cx
    sbb word [remaining_hi], 0
    mov ax, cx
    shr ax, 4
    add [gv_seg], ax
    mov ax, [remaining_lo]
    or ax, [remaining_hi]
    jnz .load
    jmp .loaded
.short:
    mov byte [read_failed], 1
.loaded:
    call close_file
    cmp byte [read_failed], 0
    jne .failed_read
    mov ah, 0x08                    ; wait for a key
    int 0x21
    call restore_video_state
    mov dx, msg_done
    mov ah, 0x09
    int 0x21
    mov ax, 0x4c00
    int 0x21
.failed_read:
    call restore_video_state
    mov dx, msg_short
    jmp fail2

fail1:
    mov ah, 0x09
    int 0x21
    mov ax, 0x4c01
    int 0x21
fail2:
    mov ah, 0x09
    int 0x21
    mov ax, 0x4c02
    int 0x21

close_file:
    mov bx, [handle]
    mov ah, 0x3e
    int 0x21
    ret

; CX bytes (even) from buf_seg:0 to GVRAM at gv_seg:0, through bank 4.
copy_to_gvram:
    pusha
    push ds
    push es
    mov dx, PORT_MEMORY_MAP
    mov al, MEMORY_MAP_GVRAM_SINGLE
    out dx, al
    mov dx, PORT_GVRAM_WRITE_MODE
    mov al, GVRAM_CPU_WRITE_MODE
    out dx, al
    mov es, [gv_seg]
    mov ds, [buf_seg]
    xor si, si
    xor di, di
    shr cx, 1
    rep movsw
    pop es
    pop ds
    mov dx, PORT_MEMORY_MAP
    mov al, [saved_memory_map]
    out dx, al
    popa
    ret

initialize_video:
    mov bx, MODE_640X200_G0_ONLY
    mov cx, PIXEL_SIZE_G0_16BPP
    xor dx, dx
    mov byte [video_mode_changed], 1
    xor ax, ax                      ; ScnMode
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed
    push cs
    pop es
    mov ax, 0x0100                  ; DefBuf: G0 640x200 16 bpp
    mov cx, 1
    mov di, g0_framebuffer_descriptor
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed
    mov ax, 0x0200                  ; DefWin: 200 lines
    mov cx, 1
    mov di, g0_window_descriptor
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed
    mov ax, 0x0300                  ; Compose: G0 only, no text over it
    mov cx, COMPOSE_G0_ONLY
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed
    mov dx, PORT_GRMODE
    mov ax, GRMODE_200_SINGLE
    out dx, ax
    mov dx, PORT_GRRES
    mov ax, GRRES_640_16BPP
    out dx, ax
    mov dx, PORT_FB0_FBW
    mov ax, SCREEN_PITCH
    out dx, ax
    mov dx, PORT_FB0_FBL
    mov ax, SCREEN_HEIGHT
    out dx, ax
    xor ax, ax
    mov dx, PORT_FB0_DOT
    out dx, ax
    mov dx, PORT_FB0_OFX
    out dx, ax
    mov dx, PORT_FB0_OFY
    out dx, ax
    mov dx, PORT_FB0_DSA_LOW
    out dx, ax
    mov dx, PORT_FB0_DSA_HIGH
    out dx, ax
    mov dx, PORT_FB0_DSP
    out dx, ax
    mov dx, PORT_FB0_DSH
    mov ax, SCREEN_HEIGHT
    out dx, ax
    xor ax, ax
    mov dx, PORT_PALETTE_COMPOSE
    out dx, ax
    mov ax, COMPOSE_G0_DIRECT
    mov dx, PORT_RGB_COMPOSE
    out dx, ax
    call clear_gvram
    mov ax, 0x0b01                  ; ScnDsp: graphics on
    int VIDEO_BIOS_INT
    test ax, ax
    jnz .failed
    call line_doubling_on
    clc
    ret
.failed:
    stc
    ret

; Black the whole 256000-byte picture area before loading.
clear_gvram:
    pusha
    push es
    mov dx, PORT_MEMORY_MAP
    mov al, MEMORY_MAP_GVRAM_SINGLE
    out dx, al
    mov dx, PORT_GVRAM_WRITE_MODE
    mov al, GVRAM_CPU_WRITE_MODE
    out dx, al
    mov bx, 0xa000
    mov si, 4                       ; 4 x 64000 bytes
.part:
    mov es, bx
    xor di, di
    xor ax, ax
    mov cx, 32000
    rep stosw
    add bx, 64000 / 16
    dec si
    jnz .part
    mov dx, PORT_MEMORY_MAP
    mov al, [saved_memory_map]
    out dx, al
    pop es
    popa
    ret

save_video_state:
    push es
    mov dx, PORT_MEMORY_MAP
    in al, dx
    mov [saved_memory_map], al
    test al, 0x10
    jz .bios_state
    mov byte [saved_single_plane], 1
    mov dx, PORT_GVRAM_WRITE_MODE
    in al, dx
    mov [saved_write_mode], al
.bios_state:
    mov ax, VIDEO_BIOS_DATA_SEG
    mov es, ax
    mov ax, [es:VIDEO_MODE_OFFSET]
    mov [saved_video_mode], ax
    mov al, [es:VIDEO_G0_BPP_OFFSET]
    mov [saved_g0_bpp], al
    mov al, [es:VIDEO_G1_BPP_OFFSET]
    mov [saved_g1_bpp], al
    pop es
    ret

; As the 65536-colour demos (M109): on a 24.8 kHz display (port 1CBh bit 3)
; show 200-line graphics with each line on two rasters (port 100h RSM = 01).
line_doubling_on:
    push ax
    push dx
    mov dx, 0x01cb
    in al, dx
    test al, 0x08
    jz .done
    mov dx, PORT_GRMODE
    in ax, dx
    test al, 0x02
    jz .done
    test al, 0xc0
    jnz .done
    or al, 0x40
    out dx, ax
    mov byte [line_doubling_set], 1
.done:
    pop dx
    pop ax
    ret

line_doubling_off:
    cmp byte [line_doubling_set], 0
    je .done
    push ax
    push dx
    mov dx, PORT_GRMODE
    in ax, dx
    and al, 0x3f
    out dx, ax
    mov byte [line_doubling_set], 0
    pop dx
    pop ax
.done:
    ret

restore_video_state:
    call line_doubling_off
    mov dx, PORT_MEMORY_MAP
    mov al, [saved_memory_map]
    out dx, al
    cmp byte [video_mode_changed], 0
    je .write_mode
    mov bx, [saved_video_mode]
    mov cl, [saved_g0_bpp]
    mov ch, [saved_g1_bpp]
    xor dx, dx
    xor ax, ax                      ; ScnMode
    int VIDEO_BIOS_INT
    mov ax, 0x0a00                  ; ResPal
    int VIDEO_BIOS_INT
    mov dx, PORT_MEMORY_MAP
    mov al, [saved_memory_map]
    out dx, al
.write_mode:
    cmp byte [saved_single_plane], 0
    je .done
    mov dx, PORT_GVRAM_WRITE_MODE
    mov al, [saved_write_mode]
    out dx, al
.done:
    ret

; Command tail: an optional file name. CF set on an unknown /option.
parse:
    pusha
    mov byte [fname], 0
    mov si, 0x81
.skip:
    lodsb
    cmp al, ' '
    je .skip
    cmp al, 9
    je .skip
    cmp al, 13
    je .ok
    cmp al, '/'
    je .bad
    mov di, fname
    mov cx, 78
.copy:
    stosb
    lodsb
    cmp al, ' '
    jbe .end
    loop .copy
.end:
    mov byte [di], 0
.ok:
    popa
    clc
    ret
.bad:
    popa
    stc
    ret
