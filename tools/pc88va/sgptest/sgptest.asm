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

; SGPTEST.COM: SGP regression test for the PC-88VA and vaeg (M109a).
;
; Every test runs one SGP command list whose source and destination are
; buffers in this program's own segment of main RAM, so no screen mode is
; changed and nothing outside the program is written. The destination area
; is filled with a known pattern first and checked with CRC-16/CCITT over
; the whole area, guard bytes before and after included, so a transfer that
; runs over its rectangle changes the CRC as well. Each result is printed
; with the value vaeg gives: "=" when equal, "*" and vaeg's value when not.
; The last line times one large transfer (port 506h polls and VRTC frames).
; Output uses the text BIOS (INT 83h), which works on the self-booting SGP
; disk, under PC-Engine and under MS-DOS. The test list is in README.md.

bits 16
cpu 186
org 0x100

%define PORT_SYSTEM          0x40      ; bit 5: VRTC
%define PORT_MEMORY_MAP      0x153     ; bit 4: single-plane GVRAM, SGP on
%define PORT_SGP_COMMAND     0x500
%define PORT_SGP_CONTROL     0x504
%define PORT_SGP_STATUS      0x506
%define SGP_BUSY             0x01
%define SGP_ABORT            0x02

%define TEST_SIZE            26        ; bytes per test-table entry
%define C_END                0x0001
%define C_SET_WORK           0x0003
%define C_SET_SOURCE         0x0004
%define C_SET_DEST           0x0005
%define C_SET_COLOR          0x0006
%define C_BITBLT             0x0007
%define C_PATBLT             0x0008
%define C_LINE               0x0009
%define C_CLS                0x000a

; Pixel modes in descriptor word 0 (bits 1:0) and start dot (bits 7:4).
%define M1                   0
%define M4                   1
%define M8                   2
%define M16                  3
%define DESC(mode, dot)      ((mode) | ((dot) << 4))

; Main-RAM buffers after the program (all in this 64 KiB segment).
%define WORK                 0x4000    ; SGP work area, 64 bytes
%define LIST                 0x4040    ; command list, up to 192 bytes
%define SRC                  0x4100    ; 4bpp source, 16 bytes x 16 lines
%define SRC12                0x4200    ; 4bpp source, 12 bytes x 16 lines
%define SRCW                 0x4300    ; 8/16bpp source, 32 bytes x 16 lines (512)
%define DSTAREA              0x4600    ; checked area: guard, 32 lines, guard
%define GUARD                256
%define DPITCH               64
%define DLINES               32
%define DSTAREA_SIZE         (GUARD + DPITCH * DLINES + GUARD)
%define DST                  (DSTAREA + GUARD)
%define BIGSRC               0x5000    ; timing: 160 bytes x 100 lines
%define BIGDST               0x9000
%define BIGPITCH             160
%define BIGLINES             100

%ifndef EXPECTED_NONE
%include "sgptest-expected.inc"
%endif
%macro EXP 1
%ifdef EXP_%1
    dw EXP_%1
%else
    dw 0
%endif
%endmacro

start:
    push cs
    pop ds
    push cs
    pop es
    cld
%ifdef DUMP
    jmp dump_main
%endif
    mov si, title
    call puts
    call fill_sources

    mov word [test_index], 1
    mov si, tests
.next:
    cmp word [si], 0
    je .tables_done
    push si
    call fill_dest
    call build_blt_list
    call run_list
    pop si
    add si, TEST_SIZE
    call report
    jmp .next
.tables_done:
    call fill_dest                 ; LINE: four lines in 4bpp
    mov si, line_list_template
    call build_copy_list
    call run_list
    call report
    call fill_dest                 ; CLS: 100 words of the colour
    mov si, cls_list_template
    call build_copy_list
    call run_list
    call report

    mov si, crlf
    call puts
    mov si, summary
    call puts
    mov ax, [differences]
    call putdec
    mov si, crlf
    call puts

    call timing
    mov ax, 0x4c00
    int 0x21

; ---- test table: op, mode, colour, source (4 words + offset),
; destination (4 words + offset). Width and height are in pixels, FBW in
; bytes as the descriptor holds it.
%macro TEST 13
    dw %1, %2, %3, %4, %5, %6, %7, %8, %9, %10, %11, %12, %13
%endmacro

tests:
; T01 BITBLT 4bpp copy, dots 0 -> 0
    TEST C_BITBLT, 0x0005, 0, DESC(M4,0), 24, 12, 16, SRC,   DESC(M4,0), 24, 12, DPITCH, DST + 4*DPITCH + 8
; T02 destination dot 1
    TEST C_BITBLT, 0x0005, 0, DESC(M4,0), 24, 12, 16, SRC,   DESC(M4,1), 24, 12, DPITCH, DST + 4*DPITCH + 8
; T03 destination dot 3
    TEST C_BITBLT, 0x0005, 0, DESC(M4,0), 24, 12, 16, SRC,   DESC(M4,3), 24, 12, DPITCH, DST + 4*DPITCH + 8
; T04 source dot 2
    TEST C_BITBLT, 0x0005, 0, DESC(M4,2), 24, 12, 16, SRC,   DESC(M4,0), 24, 12, DPITCH, DST + 4*DPITCH + 8
; T05 VD (bottom-up), source FBW +16: both start on their last line
    TEST C_BITBLT, 0x0805, 0, DESC(M4,0), 24, 12, 16, SRC + 11*16, DESC(M4,0), 24, 12, DPITCH, DST + 20*DPITCH + 8
; T06 VD, source 12-byte lines, FBW FFF3h (the M109a case)
    TEST C_BITBLT, 0x0905, 0, DESC(M4,0), 24, 12, 0xfff3, SRC12, DESC(M4,3), 24, 12, DPITCH, DST + 20*DPITCH + 8
; T07 VD, FBW FFF2h
    TEST C_BITBLT, 0x0905, 0, DESC(M4,0), 24, 12, 0xfff2, SRC12, DESC(M4,3), 24, 12, DPITCH, DST + 20*DPITCH + 8
; T08 VD, FBW FFF4h
    TEST C_BITBLT, 0x0905, 0, DESC(M4,0), 24, 12, 0xfff4, SRC12, DESC(M4,3), 24, 12, DPITCH, DST + 20*DPITCH + 8
; T09 VD, FBW FFF0h
    TEST C_BITBLT, 0x0905, 0, DESC(M4,0), 24, 12, 0xfff0, SRC12, DESC(M4,3), 24, 12, DPITCH, DST + 20*DPITCH + 8
; T10 source FBW 000Ch (12-byte lines)
    TEST C_BITBLT, 0x0005, 0, DESC(M4,0), 24, 12, 0x000c, SRC12, DESC(M4,0), 24, 12, DPITCH, DST + 4*DPITCH + 8
; T11 source FBW 000Dh
    TEST C_BITBLT, 0x0005, 0, DESC(M4,0), 24, 12, 0x000d, SRC12, DESC(M4,0), 24, 12, DPITCH, DST + 4*DPITCH + 8
; T12 source FBW 000Eh
    TEST C_BITBLT, 0x0005, 0, DESC(M4,0), 24, 12, 0x000e, SRC12, DESC(M4,0), 24, 12, DPITCH, DST + 4*DPITCH + 8
; T13 HD (right to left): addresses and dots name the right end
    TEST C_BITBLT, 0x0405, 0, DESC(M4,3), 24, 12, 16, SRC + 10, DESC(M4,3), 24, 12, DPITCH, DST + 4*DPITCH + 18
; T14 transparent (TP=1, colour 0 not written)
    TEST C_BITBLT, 0x0105, 0, DESC(M4,0), 24, 12, 16, SRC,   DESC(M4,1), 24, 12, DPITCH, DST + 4*DPITCH + 8
; T15 8bpp copy, dot 1
    TEST C_BITBLT, 0x0005, 0, DESC(M8,0), 10, 10, 32, SRCW,  DESC(M8,1), 10, 10, DPITCH, DST + 4*DPITCH + 8
; T16 16bpp copy
    TEST C_BITBLT, 0x0005, 0, DESC(M16,0), 8, 10, 32, SRCW,  DESC(M16,0), 8, 10, DPITCH, DST + 4*DPITCH + 8
; T17 1bpp source expanded to 4bpp with colour 0x5555
    TEST C_BITBLT, 0x0005, 0x5555, DESC(M1,0), 24, 12, 4, SRC, DESC(M4,1), 24, 12, DPITCH, DST + 4*DPITCH + 8
; T18 PATBLT, 8x4 pattern tiled over 24x12 at dot 1
    TEST C_PATBLT, 0x0005, 0, DESC(M4,0), 8, 4, 16, SRC,     DESC(M4,1), 24, 12, DPITCH, DST + 4*DPITCH + 8
    dw 0

; T19 LINE: four lines of colour 0x9999 (the measured M106 directions:
; HD = 0800h, VD = 0400h).
line_list_template:
    dw C_SET_COLOR, 0x9999
    dw C_LINE, 0x0000, DESC(M4,1), 20, 7, DPITCH, DST + 2*DPITCH + 4, 0
    dw C_LINE, 0x0000, DESC(M4,2), 5, 18, DPITCH, DST + 2*DPITCH + 20, 0
    dw C_LINE, 0x0800, DESC(M4,0), 15, 9, DPITCH, DST + 12*DPITCH + 40, 0
    dw C_LINE, 0x0400, DESC(M4,3), 9, 15, DPITCH, DST + 28*DPITCH + 50, 0
    dw C_END
    dw 0xffff                      ; template end
; T20 CLS: 100 words of colour 0x1234 from DST + 6 lines
cls_list_template:
    dw C_SET_COLOR, 0x1234
    dw C_CLS, DST + 6*DPITCH, 0, 100, 0
    dw C_END
    dw 0xffff

expected:
    EXP T01
    EXP T02
    EXP T03
    EXP T04
    EXP T05
    EXP T06
    EXP T07
    EXP T08
    EXP T09
    EXP T10
    EXP T11
    EXP T12
    EXP T13
    EXP T14
    EXP T15
    EXP T16
    EXP T17
    EXP T18
    EXP T19
    EXP T20

; ---- list building. Words that hold buffer offsets in this segment are
; converted to physical addresses: in the BLT table by position, in the
; copied templates for LINE and CLS by marking them in the template.

; DI = LIST; SET_WORK first.
begin_list:
    mov di, LIST
    mov ax, C_SET_WORK
    stosw
    mov ax, WORK
    call stos_phys
    ret

; AX = offset in this segment: store its physical address (two words).
stos_phys:
    push dx
    call phys
    stosw
    mov ax, dx
    stosw
    pop dx
    ret

; AX = offset -> DX:AX = CS * 16 + offset.
phys:
    push bx
    push cx
    mov bx, ax
    mov ax, cs
    xor dx, dx
    mov cx, 4
.s: shl ax, 1
    rcl dx, 1
    loop .s
    add ax, bx
    adc dx, 0
    pop cx
    pop bx
    ret

; SI = test entry.
build_blt_list:
    call begin_list
    mov ax, C_SET_COLOR
    stosw
    mov ax, [si + 4]
    stosw
    mov ax, C_SET_SOURCE
    stosw
    lea bx, [si + 6]
    call stos_desc
    mov ax, C_SET_DEST
    stosw
    lea bx, [si + 16]
    call stos_desc
    mov ax, [si]
    stosw
    mov ax, [si + 2]
    stosw
    mov ax, C_END
    stosw
    ret

; BX = four descriptor words and a buffer offset.
stos_desc:
    mov cx, 4
.w: mov ax, [bx]
    stosw
    add bx, 2
    loop .w
    mov ax, [bx]
    jmp stos_phys

; SI = template: after SET_WORK, copy words; in LINE the fifth descriptor
; word and in CLS the first operand are offsets and become addresses (the
; template keeps a 0 high word after each, which the conversion replaces).
build_copy_list:
    call begin_list
.l: lodsw
    cmp ax, 0xffff
    je .done
    stosw
    cmp ax, C_SET_COLOR
    je .one
    cmp ax, C_LINE
    je .line
    cmp ax, C_CLS
    je .cls
    jmp .l
.one:
    movsw
    jmp .l
.line:
    movsw                          ; mode
    movsw                          ; mode/dot
    movsw                          ; width
    movsw                          ; height
    movsw                          ; FBW
    lodsw                          ; offset
    add si, 2
    call stos_phys
    jmp .l
.cls:
    lodsw
    add si, 2
    call stos_phys
    movsw                          ; count low
    movsw                          ; count high
    jmp .l
.done:
    ret

; ---- run the list at LIST; CF = 1 on timeout (then aborted).
run_list:
    push si
    cli
    mov dx, PORT_MEMORY_MAP
    in al, dx
    mov [saved_map], al
    or al, 0x10
    out dx, al
    sti
    mov ax, LIST
    call phys
    push dx
    mov dx, PORT_SGP_COMMAND
    out dx, ax
    pop ax
    add dx, 2
    out dx, ax
    mov dx, PORT_SGP_CONTROL
    xor al, al
    out dx, al
    mov dx, PORT_SGP_STATUS
    mov al, SGP_BUSY
    out dx, al
    xor bx, bx                     ; BX:CX polls
    xor cx, cx
    mov byte [timed_out], 0
.poll:
    in al, dx
    test al, SGP_BUSY
    jz .idle
    add cx, 1
    adc bx, 0
    cmp bx, 0x0020
    jb .poll
    mov byte [timed_out], 1
    mov dx, PORT_SGP_CONTROL
    mov al, SGP_ABORT
    out dx, al
    xor al, al
    out dx, al
.idle:
    mov [polls], cx
    mov [polls + 2], bx
    cli
    mov dx, PORT_MEMORY_MAP
    mov al, [saved_map]
    out dx, al
%ifdef DUMP
    in al, dx
    mov [map_after], al
%endif
    sti
    pop si
    ret

; ---- results

; Print "Tnn CRC =" or "Tnn CRC *EXP" for the destination area, four to a
; line.
report:
    push si
    mov si, DSTAREA
    mov cx, DSTAREA_SIZE
    call crc16
    mov [crc], ax
    mov al, 'T'
    call putc
    mov ax, [test_index]
    call put2
    mov al, ' '
    call putc
    cmp byte [timed_out], 0
    je .crc
    mov si, timeout_text
    call puts
    inc word [differences]
    jmp .sep
.crc:
    mov ax, [crc]
    call puthex
    mov bx, [test_index]
    dec bx
    shl bx, 1
    mov ax, [expected + bx]
    cmp ax, [crc]
    jne .diff
    mov si, same_text
    call puts
    jmp .sep
.diff:
    inc word [differences]
    push ax
    mov si, diff_text
    call puts
    pop ax
    call puthex
.sep:
    mov ax, [test_index]
    inc word [test_index]
    test al, 3
    jnz .space
    mov si, crlf
    call puts
    pop si
    ret
.space:
    mov si, gap_text
    call puts
    pop si
    ret

; ---- timing: 320 x 100 pixels 4bpp main RAM to main RAM.
timing:
    call begin_list
    mov ax, C_SET_SOURCE
    stosw
    mov bx, big_desc
    call stos_desc
    mov ax, C_SET_DEST
    stosw
    mov bx, big_desc_dest
    call stos_desc
    mov ax, C_BITBLT
    stosw
    mov ax, 0x0005
    stosw
    mov ax, C_END
    stosw
    ; frames are counted by a VRTC rising edge seen between polls
    mov dx, PORT_SYSTEM
    in al, dx
    and al, 0x20
    mov [vrtc_last], al
    mov word [frames], 0
    call run_list_frames
    mov si, timing_text
    call puts
    mov ax, [polls + 2]
    or ax, ax
    jz .low
    call puthex
    mov ax, [polls]
    call puthex
    jmp .frames
.low:
    mov ax, [polls]
    call putdec
.frames:
    mov si, frames_text
    call puts
    mov ax, [frames]
    call putdec
    cmp byte [timed_out], 0
    je .ok
    mov si, timeout_text
    call puts
.ok:
    mov si, crlf
    jmp puts

; As run_list, counting VRTC rising edges while polling.
run_list_frames:
    cli
    mov dx, PORT_MEMORY_MAP
    in al, dx
    mov [saved_map], al
    or al, 0x10
    out dx, al
    sti
    mov ax, LIST
    call phys
    push dx
    mov dx, PORT_SGP_COMMAND
    out dx, ax
    pop ax
    add dx, 2
    out dx, ax
    mov dx, PORT_SGP_CONTROL
    xor al, al
    out dx, al
    mov dx, PORT_SGP_STATUS
    mov al, SGP_BUSY
    out dx, al
    xor bx, bx
    xor cx, cx
    mov byte [timed_out], 0
.poll:
    mov dx, PORT_SGP_STATUS
    in al, dx
    test al, SGP_BUSY
    jz .idle
    add cx, 1
    adc bx, 0
    mov dx, PORT_SYSTEM
    in al, dx
    and al, 0x20
    cmp al, [vrtc_last]
    je .same
    mov [vrtc_last], al
    or al, al
    jz .same
    inc word [frames]
.same:
    cmp bx, 0x0020
    jb .poll
    mov byte [timed_out], 1
    mov dx, PORT_SGP_CONTROL
    mov al, SGP_ABORT
    out dx, al
    xor al, al
    out dx, al
.idle:
    mov [polls], cx
    mov [polls + 2], bx
    cli
    mov dx, PORT_MEMORY_MAP
    mov al, [saved_map]
    out dx, al
    sti
    ret

big_desc:
    dw DESC(M4,0), 320, BIGLINES, BIGPITCH, BIGSRC
big_desc_dest:
    dw DESC(M4,0), 320, BIGLINES, BIGPITCH, BIGDST

; ---- buffers

; Sources: byte i = i * 37 + 11 (some zero nibbles, for TP=1). SRC12 and
; SRCW use other multipliers so that no two sources are alike.
fill_sources:
    mov di, SRC
    mov cx, 0x400
    xor bx, bx
.s: mov al, bl
    mov ah, 37
    mul ah
    add al, 11
    cmp di, SRC12
    jb .st
    xor al, 0x5a
    cmp di, SRCW
    jb .st
    add al, bl
.st:
    stosb
    inc bx
    loop .s
    mov di, BIGSRC
    mov cx, BIGPITCH * BIGLINES
    xor bx, bx
.b: mov al, bl
    xor al, bh
    stosb
    inc bx
    loop .b
    ret

; Destination area: byte i = i XOR A5h.
fill_dest:
    push si
    mov di, DSTAREA
    mov cx, DSTAREA_SIZE
    xor bx, bx
.l: mov al, bl
    xor al, 0xa5
    stosb
    inc bx
    loop .l
    pop si
    ret

; CRC-16/CCITT (polynomial 1021h, initial FFFFh) of CX bytes at DS:SI.
crc16:
    mov dx, 0xffff
.byte:
    lodsb
    xor dh, al
    mov bx, 8
.bit:
    shl dx, 1
    jnc .nx
    xor dx, 0x1021
.nx:
    dec bx
    jnz .bit
    loop .byte
    mov ax, dx
    ret

; ---- output through the text BIOS (INT 83h AH = 00h)

putc:
    push ax
    push dx
    xor ah, ah
    mov dx, ax
    int 0x83
    pop dx
    pop ax
    ret

puts:
    push ax
.l: lodsb
    or al, al
    jz .r
    call putc
    jmp .l
.r: pop ax
    ret

puthex:
    push cx
    mov cx, 4
.n: rol ax, 4
    push ax
    and al, 0x0f
    add al, '0'
    cmp al, '9'
    jbe .d
    add al, 7
.d: call putc
    pop ax
    loop .n
    pop cx
    ret

; AX as two decimal digits
put2:
    push ax
    push bx
    mov bl, 10
    div bl
    add ax, 0x3030
    call putc
    mov al, ah
    call putc
    pop bx
    pop ax
    ret

; AX unsigned decimal
putdec:
    push ax
    push bx
    push cx
    push dx
    mov bx, 10
    xor cx, cx
.d: xor dx, dx
    div bx
    push dx
    inc cx
    or ax, ax
    jnz .d
.p: pop ax
    add al, '0'
    call putc
    loop .p
    pop dx
    pop cx
    pop bx
    pop ax
    ret

%ifdef DUMP
; ---- SGPDUMP (M109a): T07 and T09 side by side. Both whole checked areas
; are kept; the CRCs, the first and last offsets (from DST) where the two
; differ, and 14 rows of 13 bytes from the first difference are shown,
; with port 153h as read before SGP use and after its restore.
%define DUMP_A   0xd000
%define DUMP_B   0xdc00
%define DUMP_ROWS 11
%define DUMP_COLS 13

dump_main:
    mov si, dump_title
    call puts
    mov dx, PORT_MEMORY_MAP
    in al, dx
    mov [map_before], al
    call fill_sources
    mov si, tests + 6*TEST_SIZE        ; T07
    mov di, DUMP_A
    call dump_one
    mov [crc_a], ax
    mov si, tests + 8*TEST_SIZE        ; T09
    mov di, DUMP_B
    call dump_one
    mov [crc_b], ax
    mov si, map_text
    call puts
    mov al, [map_before]
    call puthex2
    mov si, map_set_text
    call puts
    mov al, [map_before]
    or al, 0x10
    call puthex2
    mov si, map_after_text
    call puts
    mov al, [map_after]
    call puthex2
    mov si, crlf
    call puts
    mov si, crc_text
    call puts
    mov ax, [crc_a]
    call puthex
    mov al, ' '
    call putc
    mov ax, [crc_b]
    call puthex
    ; first and last differing offsets
    mov word [first], 0xffff
    mov word [last], 0xffff
    xor bx, bx
.cmp:
    mov al, [DUMP_A + bx]
    cmp al, [DUMP_B + bx]
    je .same
    cmp word [first], 0xffff
    jne .f
    mov [first], bx
.f: mov [last], bx
.same:
    inc bx
    cmp bx, DSTAREA_SIZE
    jb .cmp
    mov si, diff_range_text
    call puts
    mov ax, [first]
    sub ax, GUARD
    call puthex
    mov al, '-'
    call putc
    mov ax, [last]
    sub ax, GUARD
    call puthex
    mov si, crlf
    call puts
    ; window: from the row and even column of the first difference
    mov ax, [first]
    cmp ax, 0xffff
    jne .w
    mov ax, GUARD + 8*DPITCH + 8
.w: and ax, 0xfffe
    mov [window], ax
    mov si, dump_head
    call puts
    xor bp, bp
.row:
    mov ax, [window]
    mov bx, bp
    mov cl, 6
    shl bx, cl                         ; BP * DPITCH (64)
    add ax, bx
    push ax
    sub ax, GUARD
    call puthex
    mov al, ' '
    call putc
    pop ax
    push ax
    mov si, DUMP_A
    add si, ax
    call dump_hexrow
    mov al, ' '
    call putc
    pop ax
    mov si, DUMP_B
    add si, ax
    call dump_hexrow
    mov si, crlf
    call puts
    inc bp
    cmp bp, DUMP_ROWS
    jb .row
    mov ax, 0x4c00
    int 0x21

; SI = test entry, DI = buffer: run the test, copy the whole checked
; area; AX = its CRC.
dump_one:
    push di
    push si
    call fill_dest
    pop si
    call build_blt_list
    call run_list
    pop di
    mov si, DSTAREA
    mov cx, DSTAREA_SIZE
    rep movsb
    mov si, DSTAREA
    mov cx, DSTAREA_SIZE
    call crc16
    ret

; SI = bytes: DUMP_COLS of them as hex.
dump_hexrow:
    mov cx, DUMP_COLS
.b: lodsb
    call puthex2
    loop .b
    ret

; AL as two hex digits.
puthex2:
    push ax
    push cx
    mov ah, al
    xor al, al
    mov cx, 2
.n: rol ax, 4
    push ax
    and al, 0x0f
    add al, '0'
    cmp al, '9'
    jbe .d
    add al, 7
.d: call putc
    pop ax
    loop .n
    pop cx
    pop ax
    ret

dump_title:     db 'SGPDUMP (vaeg M109a): T07 (FBW FFF2h) and T09 (FFF0h)', 13, 10, 0
map_text:       db '153h before ', 0
map_set_text:   db ', set ', 0
map_after_text: db ', after restore ', 0
crc_text:       db 'CRC T07 T09: ', 0
diff_range_text: db '  differ from DST+', 0
dump_head:      db 'DST+ T07                       T09', 13, 10, 0
map_before:     db 0
map_after:      db 0
crc_a:          dw 0
crc_b:          dw 0
first:          dw 0
last:           dw 0
window:         dw 0
%endif

title:          db 'SGPTEST (vaeg M109a): CRC, "=" as vaeg, "*" vaeg value', 13, 10, 0
same_text:      db ' =    ', 0
diff_text:      db ' *', 0
gap_text:       db '  ', 0
timeout_text:   db 'TIMEOUT', 0
summary:        db 'differences from vaeg: ', 0
timing_text:    db 'T21 320x100 4bpp: polls ', 0
frames_text:    db ' frames ', 0
crlf:           db 13, 10, 0

align 2
test_index:     dw 0
differences:    dw 0
crc:            dw 0
polls:          dw 0, 0
frames:         dw 0
saved_map:      db 0
timed_out:      db 0
vrtc_last:      db 0

program_end:
; the program must end below its buffers (a negative TIMES count fails)
times -((program_end - $$ + 0x100) > WORK) db 0
