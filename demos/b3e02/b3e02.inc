; Copyright (c) 2026 Nakata Maho
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions
; are met:
; 1. Redistributions of source code must retain the above copyright
;    notice, this list of conditions and the following disclaimer.
; 2. Redistributions in binary form must reproduce the above copyright
;    notice, this list of conditions and the following disclaimer in the
;    documentation and/or other materials provided with the distribution.
;
; THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
; IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
; WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
; DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
; INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
; (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
; SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
; HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
; STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
; IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
; POSSIBILITY OF SUCH DAMAGE.
;
; b3e02.inc (M148): common code of the PC-9801B3-E02 probes for the
; PC-88VA (MS-DOS 4.0). The including program defines:
;   stage_str  db 'B0', 0        stage of docs/agents/tasks/M148
;   prog_str   db 'B3E0', 0      program name
;   opt_table  dw string, flag ... 0   accepted options (may be empty)
;   usage_str  db '...', 0
; and BUILD_ID on the NASM command line ('xxxxxxxx').
;
; Every line goes to the screen (handle 1) and is appended to
; A:\B3LOG.TXT. Values are always hexadecimal text; no raw byte is
; printed. Line format: <stage>,<prog>,<tag>,<fields>.
;
; Exit codes: 0 pass, 1 argument, 2 machine, 3 no board, 4 log write
; failed (otherwise pass), 10h and up: stage-specific failures.

%ifndef BUILD_ID
%define BUILD_ID '00000000'
%endif

PORT_ROMBANK    equ 0x0152
PORT_SMBC       equ 0x0153

B3_IDX          equ 0x0fa2          ; board control index
B3_DATA         equ 0x0fa3          ; board control data
B3_FAC          equ 0x0fac
B3_POS102       equ 0x0902          ; 102h (video subsystem enable)
B3_SETUP        equ 0x0904          ; 46E8h-like setup / 102h access

VGA_ATTR_W      equ 0x0c50
VGA_ATTR_R      equ 0x0c51
VGA_STAT0       equ 0x0c52          ; read: input status 0, write: misc
VGA_MISC_W      equ 0x0c52
VGA_SEQ_I       equ 0x0c54
VGA_SEQ_D       equ 0x0c55
VGA_DAC_MASK    equ 0x0c56          ; never read in a loop: hidden DAC
VGA_FEAT_R      equ 0x0c5a
VGA_MISC_R      equ 0x0c5c
VGA_GC_I        equ 0x0c5e
VGA_GC_D        equ 0x0c5f
VGA_CRTC_MONO   equ 0x0b54          ; 3B4h
VGA_CRTC_COLOR  equ 0x0d54          ; 3D4h
VGA_STAT1_MONO  equ 0x0b5a
VGA_STAT1_COLOR equ 0x0d5a

LB_SIZE         equ 200

; ---------------------------------------------------------------------------
; Start and exit.

; Parse options, machine, log, header. Exits 1 on an unknown option.
common_start:
    cld
    call parse_args
    jnc .args_ok
    mov dx, usage_str
    call print_asciz
    mov al, 1
    mov ah, 0x4c
    int 0x21
.args_ok:
    call detect_machine
    call log_open
    call emit_header
    ret

; AL = exit code. Writes the END line, closes the log, exits.
common_exit:
    mov [exit_code], al
    call lb_reset
    mov si, s_end
    call lb_tag
    mov si, s_exit_eq
    call lb_str
    mov al, [exit_code]
    call lb_hex8
    mov si, s_log_ok
    cmp byte [log_error], 0
    je .log_state
    mov si, s_log_ng
.log_state:
    call lb_str
    call lb_emit
    call log_close
    mov al, [exit_code]
    cmp byte [log_error], 0
    je .exit
    mov dx, s_log_warn
    call print_asciz
    or al, al
    jnz .exit
    mov al, 4
.exit:
    mov ah, 0x4c
    int 0x21

; Command tail: blank-separated tokens, each must match an opt_table
; entry (case-insensitive). CF on an unknown token.
parse_args:
    pusha
    mov si, 0x81
    mov cl, [0x80]
    xor ch, ch
    mov bx, si
    add bx, cx                      ; end of the tail
.skip:
    cmp si, bx
    jae .ok
    mov al, [si]
    cmp al, ' '
    je .blank
    cmp al, 9
    je .blank
    cmp al, 13
    je .ok
    jmp .token
.blank:
    inc si
    jmp .skip
.token:
    ; DI walks opt_table
    mov di, opt_table
.next_opt:
    mov dx, [di]
    or dx, dx
    jz .bad
    push si
    push di
    mov di, dx
.cmp:
    mov al, [di]
    or al, al
    jz .opt_end
    cmp si, bx
    jae .nomatch
    mov ah, [si]
    cmp ah, 'a'
    jb .upper
    cmp ah, 'z'
    ja .upper
    sub ah, 0x20
.upper:
    cmp al, ah
    jne .nomatch
    inc si
    inc di
    jmp .cmp
.opt_end:
    ; the token must end here
    cmp si, bx
    jae .match
    mov ah, [si]
    cmp ah, ' '
    je .match
    cmp ah, 9
    je .match
    cmp ah, 13
    je .match
.nomatch:
    pop di
    pop si
    add di, 4
    jmp .next_opt
.match:
    pop di
    add sp, 2                       ; drop the saved SI, keep the new one
    mov di, [di + 2]
    mov byte [di], 1
    jmp .skip
.bad:
    popa
    stc
    ret
.ok:
    popa
    clc
    ret

; machine_word, machine_str from F000:FFFE.
detect_machine:
    pusha
    push es
    mov ax, 0xf000
    mov es, ax
    mov ax, [es:0xfffe]
    pop es
    mov [machine_word], ax
    mov si, s_va
    cmp ax, 0xffff
    je .set
    mov si, s_va2
    cmp ax, 0xfffe
    je .set
    mov si, s_va91
    cmp ax, 0xfffd
    je .set
    mov si, s_unknown
.set:
    mov [machine_str], si
    popa
    ret

; Exit 2 unless the machine is a PC-88VA (F000:FFFE = FFFFh).
require_va:
    cmp word [machine_word], 0xffff
    je .ok
    call lb_reset
    mov si, s_machine
    call lb_tag
    mov si, s_va_only
    call lb_str
    call lb_emit
    mov al, 2
    jmp common_exit
.ok:
    ret

; #B3E02,<stage>,<prog>,build=..,date=YYYY-MM-DD,time=HH:MM:SS,machine=..,FFFF
emit_header:
    pusha
    call lb_reset
    mov si, s_hash
    call lb_str
    mov si, stage_str
    call lb_str
    mov al, ','
    call lb_char
    mov si, prog_str
    call lb_str
    mov si, s_build_eq
    call lb_str
    mov si, build_str
    call lb_str
    mov si, s_date_eq
    call lb_str
    mov ah, 0x2a
    int 0x21
    push dx
    mov ax, cx
    call lb_dec16
    mov al, '-'
    call lb_char
    pop dx
    push dx
    mov al, dh
    call lb_dec2
    mov al, '-'
    call lb_char
    pop dx
    mov al, dl
    call lb_dec2
    mov si, s_time_eq
    call lb_str
    mov ah, 0x2c
    int 0x21
    push dx
    mov al, ch
    call lb_dec2
    mov al, ':'
    call lb_char
    mov al, cl
    call lb_dec2
    mov al, ':'
    call lb_char
    pop dx
    mov al, dh
    call lb_dec2
    mov si, s_machine_eq
    call lb_str
    mov si, [machine_str]
    call lb_str
    mov al, ','
    call lb_char
    mov ax, [machine_word]
    call lb_hex16
    call lb_emit
    popa
    ret

; ---------------------------------------------------------------------------
; Port probe. On the machines an undriven port may return the last bytes
; on the bus (usually code near the IN); vaeg and M144's VA measurement
; give FFh. Three code patterns, each with the IN at an even and at an
; odd address, so that no prefetched code can give one value six times.

; DX = port. probe_values[0..5], probe_class (0 FF, 1 DRIVEN, 2 FLOAT).
; AL = probe_values[0].
probe_port:
    push cx
    push si
    push di
    mov di, probe_values
    call probe_a_even
    mov [di], al
    call probe_a_odd
    mov [di + 1], al
    call probe_b_even
    mov [di + 2], al
    call probe_b_odd
    mov [di + 3], al
    call probe_c_even
    mov [di + 4], al
    call probe_c_odd
    mov [di + 5], al
    mov si, di
    mov al, [si]
    mov cx, 5
.compare:
    inc si
    cmp al, [si]
    jne .float
    loop .compare
    mov byte [probe_class], 1
    cmp al, 0xff
    jne .done
    mov byte [probe_class], 0
    jmp .done
.float:
    mov byte [probe_class], 2
.done:
    mov al, [probe_values]
    pop di
    pop si
    pop cx
    ret

    align 2
probe_a_even:
    in al, dx
    mov bl, 0x55
    mov bl, 0x55
    mov bl, 0x55
    ret

    align 2
probe_a_odd:
    cld
    in al, dx
    mov bl, 0x55
    mov bl, 0x55
    mov bl, 0x55
    ret

    align 2
probe_b_even:
    in al, dx
    mov cl, 0xaa
    mov cl, 0xaa
    mov cl, 0xaa
    ret

    align 2
probe_b_odd:
    clc
    in al, dx
    mov cl, 0xaa
    mov cl, 0xaa
    mov cl, 0xaa
    ret

    align 2
probe_c_even:
    in al, dx
    mov ah, 0x33
    mov ah, 0x33
    mov ah, 0x33
    ret

    align 2
probe_c_odd:
    stc
    in al, dx
    mov ah, 0x33
    mov ah, 0x33
    mov ah, 0x33
    ret

; Append the last probe: "FF", "D:xx" or "F:xxxxxxxxxxxx".
lb_probe:
    push ax
    push cx
    push si
    cmp byte [probe_class], 0
    jne .not_ff
    mov si, s_ff
    call lb_str
    jmp .done
.not_ff:
    cmp byte [probe_class], 1
    jne .float
    mov si, s_d
    call lb_str
    mov al, [probe_values]
    call lb_hex8
    jmp .done
.float:
    mov si, s_f
    call lb_str
    mov si, probe_values
    mov cx, 6
.f:
    lodsb
    call lb_hex8
    loop .f
.done:
    pop si
    pop cx
    pop ax
    ret

; DX = port: append " pppp=<probe>" and leave the probe in AL/probe_*.
lb_port_probe:
    push ax
    mov al, ' '
    call lb_char
    mov ax, dx
    call lb_hex16
    mov al, '='
    call lb_char
    pop ax
    call probe_port
    call lb_probe
    ret

; ---------------------------------------------------------------------------
; VGA index/data helpers. The incoming index is read back and restored
; when the index port is driven, otherwise index 00h is left.

; DX = index port, AH = index: AL = data (DX+1), one plain IN.
idx_read:
    push bx
    push dx
    in al, dx
    mov bl, al                      ; incoming index
    mov al, ah
    out dx, al
    inc dx
    in al, dx
    dec dx
    xchg al, bl
    out dx, al                      ; restore the index
    mov al, bl
    pop dx
    pop bx
    ret

; ---------------------------------------------------------------------------
; Board control (0FA2h/0FA3h).

; AH = register: AL = data; the index is left at 00h.
board_read:
    push dx
    mov dx, B3_IDX
    mov al, ah
    out dx, al
    inc dx
    in al, dx
    dec dx
    push ax
    xor al, al
    out dx, al
    pop ax
    pop dx
    ret

; AH = register, AL = value: write it; the index is left at 00h.
board_write:
    push dx
    push ax
    mov dx, B3_IDX
    mov al, ah
    out dx, al
    inc dx
    pop ax
    out dx, al
    dec dx
    push ax
    xor al, al
    out dx, al
    pop ax
    pop dx
    ret

; CF clear when reg 00h reads 60h (the board answers).
board_present:
    push ax
    mov ah, 0x00
    call board_read
    cmp al, 0x60
    pop ax
    je .yes
    stc
    ret
.yes:
    clc
    ret

; /MMIO and /RELAY: board reg 03h bit 0 / bit 1 set before the wake-up,
; as the earlier probes wrote reg 03h = 03h first. Emits
; "REG3,in=xx set=xx". No-op without either option.
reg3_apply:
    pusha
    mov al, [opt_mmio]
    or al, [opt_relay]
    jz .done
    mov ah, 0x03
    call board_read
    mov [reg3_in], al
    cmp byte [opt_mmio], 0
    je .relay
    or al, 0x01
.relay:
    cmp byte [opt_relay], 0
    je .write
    or al, 0x02
.write:
    call board_write
    call board_read
    mov [reg3_set], al
    mov byte [reg3_used], 1
    call lb_reset
    mov si, s_reg3
    call lb_tag
    mov si, s_in_eq3
    call lb_str
    mov al, [reg3_in]
    call lb_hex8
    mov si, s_set_eq3
    call lb_str
    mov al, [reg3_set]
    call lb_hex8
    call lb_emit
.done:
    popa
    ret

; Write the incoming reg 03h back; emits "REG3,restored=xx OK|NG".
; CF when it does not read back.
reg3_restore:
    pusha
    cmp byte [reg3_used], 0
    je .ok
    mov ah, 0x03
    mov al, [reg3_in]
    call board_write
    call board_read
    mov [reg3_out], al
    mov byte [reg3_used], 0
    call lb_reset
    mov si, s_reg3
    call lb_tag
    mov si, s_rest_eq3
    call lb_str
    mov al, [reg3_out]
    call lb_hex8
    mov al, [reg3_out]
    cmp al, [reg3_in]
    jne .ng
    mov si, s_ok3
    call lb_str
    call lb_emit
.ok:
    popa
    clc
    ret
.ng:
    mov si, s_ng3
    call lb_str
    call lb_emit
    popa
    stc
    ret

; Wake-up as the earlier probes: 0904h=00h, 0902h=01h, 0904h=20h.
; pos_in = 0902h read while 0904h=00h (before 01h is written).
board_wake:
    push ax
    push dx
    mov dx, B3_SETUP
    xor al, al
    out dx, al
    mov dx, B3_POS102
    in al, dx
    mov [pos_in], al
    mov al, 0x01
    out dx, al
    in al, dx
    mov [pos_set], al
    mov dx, B3_SETUP
    mov al, 0x20
    out dx, al
    mov byte [woken], 1
    pop dx
    pop ax
    ret

; Undo the wake-up: 0904h=00h, 0902h=<pos_in if it was 00h or 01h, else
; 00h>, 0904h=20h. pos_out = 0902h read back in setup.
board_unwake:
    push ax
    push dx
    cmp byte [woken], 0
    je .done
    mov dx, B3_SETUP
    xor al, al
    out dx, al
    mov dx, B3_POS102
    mov al, [pos_in]
    cmp al, 0x01
    jbe .write
    xor al, al
.write:
    out dx, al
    in al, dx
    mov [pos_out], al
    mov dx, B3_SETUP
    mov al, 0x20
    out dx, al
    mov byte [woken], 0
.done:
    pop dx
    pop ax
    ret

; CRTC index port by Misc Output bit 0 (read at 0C5Ch): DX.
crtc_port:
    push ax
    mov dx, VGA_MISC_R
    in al, dx
    mov dx, VGA_CRTC_MONO
    cmp al, 0xff
    je .done
    test al, 0x01
    jz .done
    mov dx, VGA_CRTC_COLOR
.done:
    pop ax
    ret

; SR06 lock test. Fills sr06_in, sr06_12, sr06_00; restores SR06
; (12h if it came in unlocked, else 00h) and the SR index.
; CF clear when 12h reads 12h and 00h reads 0Fh.
sr06_test:
    push ax
    push bx
    push dx
    mov dx, VGA_SEQ_I
    in al, dx
    mov bl, al
    mov al, 0x06
    out dx, al
    inc dx
    in al, dx
    mov [sr06_in], al
    mov al, 0x12
    out dx, al
    in al, dx
    mov [sr06_12], al
    xor al, al
    out dx, al
    in al, dx
    mov [sr06_00], al
    mov al, [sr06_in]
    cmp al, 0x12
    je .restore
    xor al, al
.restore:
    out dx, al
    dec dx
    mov al, bl
    out dx, al
    cmp byte [sr06_12], 0x12
    jne .fail
    cmp byte [sr06_00], 0x0f
    jne .fail
    pop dx
    pop bx
    pop ax
    clc
    ret
.fail:
    pop dx
    pop bx
    pop ax
    stc
    ret

; Append " SR06:in=xx,12>xx,00>xx".
lb_sr06:
    push ax
    push si
    mov si, s_sr06
    call lb_str
    mov al, [sr06_in]
    call lb_hex8
    mov si, s_sr06_12
    call lb_str
    mov al, [sr06_12]
    call lb_hex8
    mov si, s_sr06_00
    call lb_str
    mov al, [sr06_00]
    call lb_hex8
    pop si
    pop ax
    ret

; ---------------------------------------------------------------------------
; Line buffer.

lb_reset:
    mov word [lb_ptr], lb_buf
    ret

; AL -> buffer (keeps room for CR LF).
lb_char:
    push di
    mov di, [lb_ptr]
    cmp di, lb_buf + LB_SIZE - 2
    jae .full
    mov [di], al
    inc di
    mov [lb_ptr], di
.full:
    pop di
    ret

; NUL-terminated SI -> buffer.
lb_str:
    push ax
    push si
.l:
    lodsb
    or al, al
    jz .e
    call lb_char
    jmp .l
.e:
    pop si
    pop ax
    ret

; "<stage>,<prog>,<SI>," as the start of a line.
lb_tag:
    push ax
    push si
    mov si, stage_str
    call lb_str
    mov al, ','
    call lb_char
    mov si, prog_str
    call lb_str
    mov al, ','
    call lb_char
    pop si
    call lb_str
    mov al, ','
    call lb_char
    pop ax
    ret

lb_hex16:
    push ax
    mov al, ah
    call lb_hex8
    pop ax
lb_hex8:
    push ax
    push ax
    shr al, 4
    call lb_nibble
    pop ax
    and al, 0x0f
    call lb_nibble
    pop ax
    ret
lb_nibble:
    add al, '0'
    cmp al, '9'
    jbe .o
    add al, 7
.o:
    jmp lb_char

; AL (0..99) as two decimal digits.
lb_dec2:
    push ax
    push bx
    xor ah, ah
    mov bl, 10
    div bl
    add ax, '00'
    call lb_char
    mov al, ah
    call lb_char
    pop bx
    pop ax
    ret

; AX unsigned decimal.
lb_dec16:
    pusha
    mov bx, 10
    xor cx, cx
.div:
    xor dx, dx
    div bx
    push dx
    inc cx
    or ax, ax
    jnz .div
.out:
    pop ax
    add al, '0'
    call lb_char
    loop .out
    popa
    ret

; Line + CR LF to the screen and the log.
lb_emit:
    pusha
    mov di, [lb_ptr]
    mov byte [di], 13
    mov byte [di + 1], 10
    add di, 2
    mov cx, di
    sub cx, lb_buf
    mov dx, lb_buf
    mov bx, 1
    mov ah, 0x40
    int 0x21
    mov bx, [log_handle]
    or bx, bx
    jz .reset
    mov cx, di
    sub cx, lb_buf
    mov dx, lb_buf
    mov ah, 0x40
    int 0x21
    jc .failed
    cmp ax, cx
    je .reset
.failed:
    mov byte [log_error], 1
.reset:
    call lb_reset
    popa
    ret

; DX = NUL-terminated string to the screen.
print_asciz:
    pusha
    mov si, dx
    xor cx, cx
.len:
    cmp byte [si], 0
    je .write
    inc si
    inc cx
    jmp .len
.write:
    mov bx, 1
    mov ah, 0x40
    int 0x21
    popa
    ret

; ---------------------------------------------------------------------------
; A:\B3LOG.TXT, appended; created when missing.

log_open:
    pusha
    mov dx, s_logname
    mov ax, 0x3d01
    int 0x21
    jnc .opened
    cmp ax, 2
    jne .failed
    mov dx, s_logname
    xor cx, cx
    mov ah, 0x3c
    int 0x21
    jc .failed
    mov [log_handle], ax
    jmp .done
.opened:
    mov [log_handle], ax
    mov bx, ax
    mov ax, 0x4202
    xor cx, cx
    xor dx, dx
    int 0x21
    jnc .done
    mov byte [log_error], 1
    jmp .done
.failed:
    mov word [log_handle], 0
    mov byte [log_error], 1
.done:
    popa
    ret

log_close:
    pusha
    mov bx, [log_handle]
    or bx, bx
    jz .done
    mov ah, 0x3e
    int 0x21
    mov word [log_handle], 0
.done:
    popa
    ret

; ---------------------------------------------------------------------------
; Data.

build_str       db BUILD_ID, 0
s_logname       db 'A:\B3LOG.TXT', 0
s_va            db 'VA', 0
s_va2           db 'VA2/3', 0
s_va91          db 'VA+91', 0
s_unknown       db '?', 0
s_hash          db '#B3E02,', 0
s_build_eq      db ',build=', 0
s_date_eq       db ',date=', 0
s_time_eq       db ',time=', 0
s_machine_eq    db ',machine=', 0
s_machine       db 'MACHINE', 0
s_va_only       db 'not a PC-88VA (F000:FFFE<>FFFF); stage runs on the VA only', 0
s_end           db 'END', 0
s_exit_eq       db 'exit=', 0
s_log_ok        db ' log=A:\B3LOG.TXT', 0
s_log_ng        db ' log=WRITE-FAILED', 0
s_log_warn      db 'WARNING: A:\B3LOG.TXT could not be written', 13, 10, 0
s_ff            db 'FF', 0
s_d             db 'D:', 0
s_f             db 'F:', 0
s_sr06          db ' SR06:in=', 0
s_sr06_12       db ',12>', 0
s_sr06_00       db ',00>', 0
s_opt_mmio      db '/MMIO', 0
s_opt_relay     db '/RELAY', 0
s_reg3          db 'REG3', 0
s_in_eq3        db 'in=', 0
s_set_eq3       db ' set=', 0
s_rest_eq3      db 'restored=', 0
s_ok3           db ' OK', 0
s_ng3           db ' NG', 0
s_pass          db ' PASS', 0
s_fail          db ' FAIL', 0

    align 2
machine_word    dw 0
machine_str     dw s_va
log_handle      dw 0
lb_ptr          dw lb_buf
exit_code       db 0
log_error       db 0
probe_class     db 0
probe_values    times 6 db 0
woken           db 0
opt_mmio        db 0
opt_relay       db 0
reg3_used       db 0
reg3_in         db 0
reg3_set        db 0
reg3_out        db 0
pos_in          db 0
pos_set         db 0
pos_out         db 0
sr06_in         db 0
sr06_12         db 0
sr06_00         db 0
lb_buf          times LB_SIZE db 0
