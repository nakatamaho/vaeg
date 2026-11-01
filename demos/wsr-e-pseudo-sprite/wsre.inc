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
; wsre.inc: code shared by the WSR-E track programs
; (docs/modernization/wsr-e-pseudo-sprite-plan.md, sections 5 and 7).
;
; The including program starts with "org 0x100", "jmp main", then
; includes this file. It must define:
;   stage_str   db 'W1', 0          ; stage written in every CSV line
;   prog_str    db 'W1PORT', 0      ; program name for comment lines
; and may change board_str / cr27_str before emitting rows.
;
; Everything here is 80186 code (cpu 186): conditional jumps stay short.

%ifndef BUILD_ID
%define BUILD_ID 'nogit000'
%endif

FRAMES              equ 600         ; VRTCs a timing row (about 10 s)
RUNS                equ 3           ; runs of every timing row

PIC_CMD             equ 0x0188      ; master 8259: command / EOI
PIC_MASK            equ 0x018a      ; master 8259: interrupt mask
VRTC_VECTOR         equ 0x0a        ; IRQ2
VRTC_MASK_ONLY      equ 0xfb        ; mask value: only IRQ2 enabled
PORT_DISPLAY_RATE   equ 0x01cb      ; bit 3 set at 24.8 kHz
PORT_ROM_BANK       equ 0x0152      ; ROM area 0 (bits 3-0), area 1 (7-4)

; WSR-E: VGA port 3C0h + n is 4nE0h (plan, section 11).
WSRE_ATTR           equ 0x40e0
WSRE_ATTR_READ      equ 0x41e0
WSRE_MISC           equ 0x42e0
WSRE_SEQ_INDEX      equ 0x44e0
WSRE_SEQ_DATA       equ 0x45e0
WSRE_PEL_MASK       equ 0x46e0
WSRE_DAC_WRITE      equ 0x48e0
WSRE_DAC_DATA       equ 0x49e0
WSRE_GC_INDEX       equ 0x4ee0
WSRE_GC_DATA        equ 0x4fe0
WSRE_CRTC_INDEX     equ 0x54e0
WSRE_CRTC_DATA      equ 0x55e0
WSRE_STATUS         equ 0x5ae0
WSRE_SETUP          equ 0x46e8
WSRE_BOARD_40E1     equ 0x40e1
WSRE_BOARD_42E1     equ 0x42e1
WSRE_ID_51E1        equ 0x51e1
WSRE_ID_59E1        equ 0x59e1
WSRE_ID_5BE1        equ 0x5be1

LB_SIZE             equ 160

; ---------------------------------------------------------------------------
; Machine and display rate (plan, section 5).

; machine_str -> 'VA', 'VA2/3', 'VA+91' or '?XXXX' from F000:FFFE.
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
    mov di, s_unknown + 1
    call hex16_to_mem
    mov si, s_unknown
.set:
    mov [machine_str], si
    popa
    ret

; rate_str -> '15' or '24' (port 1CBh bit 3 is set at 24.8 kHz).
detect_rate:
    push ax
    push dx
    mov dx, PORT_DISPLAY_RATE
    in al, dx
    mov word [rate_str], s_15
    test al, 0x08
    jz .done
    mov word [rate_str], s_24
.done:
    pop dx
    pop ax
    ret

; ---------------------------------------------------------------------------
; VRTC counting on IRQ2 (as demos/sgp-contention).

vrtc_isr:
    push ax
    push dx
    inc word [cs:tick]
    mov al, 0x20
    mov dx, PIC_CMD
    out dx, al
    pop dx
    pop ax
    iret

; Install the handler and unmask IRQ2; other IRQs keep their mask.
vrtc_install:
    pusha
    push es
    mov ax, 0x3500 + VRTC_VECTOR
    int 0x21
    mov [old_vector], bx
    mov [old_vector + 2], es
    pop es
    mov dx, PIC_MASK
    in al, dx
    mov [old_mask], al
    mov dx, vrtc_isr
    mov ax, 0x2500 + VRTC_VECTOR
    int 0x21
    cli
    mov dx, PIC_MASK
    mov al, [old_mask]
    and al, 0xff - 0x04
    out dx, al
    sti
    mov byte [vrtc_installed], 1
    popa
    ret

; Restore the mask and the vector (safe to call twice).
vrtc_remove:
    pusha
    cmp byte [vrtc_installed], 0
    je .done
    cli
    mov dx, PIC_MASK
    mov al, [old_mask]
    out dx, al
    sti
    push ds
    lds dx, [old_vector]
    mov ax, 0x2500 + VRTC_VECTOR
    int 0x21
    pop ds
    mov byte [vrtc_installed], 0
.done:
    popa
    ret

; CF set if no VRTC arrives in about 2 s of polling.
vrtc_check:
    pusha
    mov ax, [tick]
    mov bx, 16
.outer:
    mov cx, 0xffff
.inner:
    cmp ax, [tick]
    jne .ok
    loop .inner
    dec bx
    jnz .outer
    popa
    stc
    ret
.ok:
    popa
    clc
    ret

; Wait for the next VRTC edge.
vrtc_wait_edge:
    push ax
    mov ax, [tick]
.wait:
    cmp ax, [tick]
    je .wait
    pop ax
    ret

; DX:AX = how many times the near routine BX completed in FRAMES VRTCs.
; Counting starts at a VRTC edge; every IRQ except IRQ2 is masked during
; the row; one VRTC is left after the row. BX survives; the routine may
; change AX, CX, DX, SI, DI, BP.
measure:
    push si
    push di
    push bp
    mov dx, PIC_MASK
    in al, dx
    mov [row_mask], al
    cli
    mov al, VRTC_MASK_ONLY
    out dx, al
    sti
    call vrtc_wait_edge
    mov ax, [tick]
    mov [t0], ax
    xor si, si
    xor di, di
.block:
    push si
    push di
    push bx
    call bx
    pop bx
    pop di
    pop si
    add si, 1
    adc di, 0
    mov ax, [tick]
    sub ax, [t0]
    cmp ax, FRAMES
    jb .block
    mov dx, PIC_MASK
    cli
    mov al, [row_mask]
    out dx, al
    sti
    call vrtc_wait_edge
    mov ax, si
    mov dx, di
    pop bp
    pop di
    pop si
    ret

; ---------------------------------------------------------------------------
; Calibration row R0: LOOP 1024 times in main RAM (M130: 18.1 clocks a
; pass on the VA and VA2).

r0_block:
    mov cx, 1024
.loop:
    loop .loop
    ret

run_r0:
    pusha
    mov word [row_name], s_r0
    mov word [row_param], s_loop1024
    mov byte [row_run], 1
.run:
    mov bx, r0_block
    call measure
    mov [count_lo], ax
    mov [count_hi], dx
    call emit_timing
    inc byte [row_run]
    cmp byte [row_run], RUNS
    jbe .run
    popa
    ret

; ---------------------------------------------------------------------------
; Port probe (plan, section 7). On the machines a port that nothing
; drives returns the last bytes on the bus, usually code bytes near the
; IN; vaeg returns FFh for an unconnected port. Both mean "not driven".
; Three code patterns (A, B, C) differ in the bytes after the IN and in
; the byte before it, and each runs with the IN at an even and at an odd
; address, so neither the IN's own word nor the following words can give
; the same value six times.

; DX = port. Fills probe_values[0..5] (A even, A odd, B even, B odd, C
; even, C odd) and probe_class (0 FF, 1 DRIVEN, 2 FLOAT).
probe_port:
    pusha
    mov di, probe_values
    call probe_a_even
    stosb
    call probe_a_odd
    stosb
    call probe_b_even
    stosb
    call probe_b_odd
    stosb
    call probe_c_even
    stosb
    call probe_c_odd
    stosb
    mov si, probe_values
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
    popa
    ret

; IN at an even address (the word is EC xx), or after a one-byte
; instruction so that it is at an odd address (the word is yy EC). The
; one-byte instructions (CLD, CLC, STC) change nothing the caller uses.
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

; Append the last probe's class to the line: FF / DRIVEN xx /
; FLOAT A=xxxx B=xxxx C=xxxx.
lb_probe_result:
    pusha
    cmp byte [probe_class], 0
    jne .not_ff
    mov si, s_ff
    call lb_str
    jmp .done
.not_ff:
    cmp byte [probe_class], 1
    jne .float
    mov si, s_driven
    call lb_str
    mov al, [probe_values]
    call lb_hex8
    jmp .done
.float:
    mov si, s_float_a
    call lb_str
    mov al, [probe_values]
    call lb_hex8
    mov al, [probe_values + 1]
    call lb_hex8
    mov si, s_float_b
    call lb_str
    mov al, [probe_values + 2]
    call lb_hex8
    mov al, [probe_values + 3]
    call lb_hex8
    mov si, s_float_c
    call lb_str
    mov al, [probe_values + 4]
    call lb_hex8
    mov al, [probe_values + 5]
    call lb_hex8
.done:
    popa
    ret

; DX = port: probe it and emit a check row P,<port>,...,<class>.
; Returns the class in AL.
probe_and_emit:
    push si
    push di
    call probe_port
    mov ax, dx
    mov di, param_buf
    call hex16_to_mem
    mov word [row_name], s_p
    mov word [row_param], param_buf
    call check_begin
    call lb_probe_result
    call lb_emit
    mov al, [probe_class]
    pop di
    pop si
    ret

; ---------------------------------------------------------------------------
; CSV lines (plan, section 7):
; stage,build,machine,khz,board,cr27,row,param,run,count,vrtc,result

; Prefix up to and including the comma after param.
emit_prefix:
    pusha
    call lb_reset
    mov si, stage_str
    call lb_field
    mov si, build_str
    call lb_field
    mov si, [machine_str]
    call lb_field
    mov si, [rate_str]
    call lb_field
    mov si, [board_str]
    call lb_field
    mov si, cr27_str
    call lb_field
    mov si, [row_name]
    call lb_field
    mov si, [row_param]
    call lb_field
    popa
    ret

; Timing row: row_name, row_param, row_run, count_hi:count_lo.
emit_timing:
    pusha
    call emit_prefix
    mov al, [row_run]
    xor ah, ah
    call lb_dec16
    call lb_comma
    mov ax, [count_lo]
    mov dx, [count_hi]
    call lb_dec32
    call lb_comma
    mov ax, FRAMES
    call lb_dec16
    call lb_comma
    call lb_emit
    popa
    ret

; Check row: prefix and three empty fields; the caller appends the
; result (no commas) and calls lb_emit.
check_begin:
    push ax
    call emit_prefix
    mov al, ','
    call lb_char
    call lb_char
    call lb_char
    pop ax
    ret

; AL = value read, AH = expected: append "R=xx E=xx PASS|FAIL" and count
; a failure.
lb_expect:
    push ax
    push si
    push ax
    mov si, s_r_eq
    call lb_str
    call lb_hex8
    mov si, s_e_eq
    call lb_str
    mov al, ah
    call lb_hex8
    pop ax
    mov si, s_pass_sp
    cmp al, ah
    je .ok
    mov si, s_fail_sp
    inc word [fail_count]
.ok:
    call lb_str
    pop si
    pop ax
    ret

; Comment line "# PROG machine-word build rate" to screen and log.
emit_banner:
    pusha
    call lb_reset
    mov al, '#'
    call lb_char
    mov al, ' '
    call lb_char
    mov si, prog_str
    call lb_str
    mov al, ' '
    call lb_char
    mov si, [machine_str]
    call lb_str
    mov al, ' '
    call lb_char
    mov ax, [machine_word]
    call lb_hex16
    mov al, ' '
    call lb_char
    mov si, build_str
    call lb_str
    mov al, ' '
    call lb_char
    mov si, [rate_str]
    call lb_str
    mov si, s_khz
    call lb_str
    call lb_emit
    popa
    ret

; Summary row SUM,<prog>: PASS when fail_count is 0, else FAIL n; SI =
; extra text (zero-terminated, may be empty).
emit_summary:
    pusha
    mov word [row_name], s_sum
    mov word [row_param], prog_str
    call check_begin
    cmp word [fail_count], 0
    jne .fail
    mov si, s_pass
    call lb_str
    jmp .done
.fail:
    mov si, s_fail_n
    call lb_str
    mov ax, [fail_count]
    call lb_dec16
.done:
    call lb_emit
    popa
    ret

; ---------------------------------------------------------------------------
; Line buffer and number formatting.

lb_reset:
    mov word [lb_ptr], lb_buf
    ret

; AL -> line (dropped when full).
lb_char:
    push di
    mov di, [lb_ptr]
    cmp di, lb_buf + LB_SIZE - 3
    jae .full
    mov [di], al
    inc di
    mov [lb_ptr], di
.full:
    pop di
    ret

lb_comma:
    push ax
    mov al, ','
    call lb_char
    pop ax
    ret

; SI -> zero-terminated string.
lb_str:
    push ax
    push si
.next:
    lodsb
    or al, al
    jz .done
    call lb_char
    jmp .next
.done:
    pop si
    pop ax
    ret

lb_field:
    call lb_str
    jmp lb_comma

lb_hex8:
    push ax
    push ax
    shr al, 4
    call lb_hex_nibble
    pop ax
    call lb_hex_nibble
    pop ax
    ret

lb_hex_nibble:
    push ax
    and al, 0x0f
    add al, '0'
    cmp al, '9'
    jbe .digit
    add al, 'A' - '0' - 10
.digit:
    call lb_char
    pop ax
    ret

lb_hex16:
    push ax
    xchg al, ah
    call lb_hex8
    xchg al, ah
    call lb_hex8
    pop ax
    ret

lb_dec16:
    push dx
    xor dx, dx
    call lb_dec32
    pop dx
    ret

; DX:AX unsigned -> decimal.
lb_dec32:
    pusha
    mov di, dec_buf_end
    mov cx, 10
.digit:
    mov bx, ax
    mov ax, dx
    xor dx, dx
    div cx
    xchg ax, bx
    div cx
    add dl, '0'
    dec di
    mov [di], dl
    mov dx, bx
    or bx, ax
    jnz .digit
.copy:
    mov al, [di]
    call lb_char
    inc di
    cmp di, dec_buf_end
    jb .copy
    popa
    ret

; AX -> 4 hex digits and a zero at DI.
hex16_to_mem:
    push ax
    push bx
    push cx
    mov cx, 4
.digit:
    rol ax, 4
    mov bl, al
    and bl, 0x0f
    add bl, '0'
    cmp bl, '9'
    jbe .store
    add bl, 'A' - '0' - 10
.store:
    mov [di], bl
    inc di
    loop .digit
    mov byte [di], 0
    pop cx
    pop bx
    pop ax
    ret

; Write the line plus CR LF to the screen (handle 1) and to the log.
lb_emit:
    pusha
    mov al, 13
    call lb_char_force
    mov al, 10
    call lb_char_force
    mov cx, [lb_ptr]
    sub cx, lb_buf
    mov dx, lb_buf
    mov bx, 1
    mov ah, 0x40
    int 0x21
    mov bx, [log_handle]
    or bx, bx
    jz .reset
    mov cx, [lb_ptr]
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

lb_char_force:
    push di
    mov di, [lb_ptr]
    mov [di], al
    inc di
    mov [lb_ptr], di
    pop di
    ret

; ---------------------------------------------------------------------------
; WSRLOG.TXT in the current directory: appended; a header line when new.

log_open:
    pusha
    mov dx, s_logname
    mov ax, 0x3d02
    int 0x21
    jnc .opened
    cmp ax, 2                 ; create only when missing, never truncate on denied access
    jne .failed
    mov dx, s_logname
    xor cx, cx
    mov ah, 0x3c
    int 0x21
    jc .failed
    mov [log_handle], ax
    mov bx, ax
    mov dx, s_header
    mov cx, s_header_end - s_header
    mov ah, 0x40
    int 0x21
    jc .error
    cmp ax, cx
    jne .error
    jmp .done
.opened:
    mov [log_handle], ax
    mov bx, ax
    mov ax, 0x4202
    xor cx, cx
    xor dx, dx
    int 0x21
    jnc .done
.error:
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
; Common start and exit.

; Machine, rate, log, VRTC handler, banner. CF set (message printed) if
; no VRTC arrives.
common_start:
    call detect_machine
    call detect_rate
    call log_open
    call vrtc_install
    call emit_banner
    call vrtc_check
    jnc .ok
    mov word [row_name], s_vrtc
    mov word [row_param], s_irq2
    call check_begin
    mov si, s_no_vrtc
    call lb_str
    call lb_emit
    stc
.ok:
    ret

; AL = exit code. Restores the VRTC vector and mask, closes the log.
common_exit:
    push ax
%ifdef WSRE_PROBE
    call probe_cleanup
%endif
    call vrtc_remove
    call log_close
    pop ax
    cmp byte [log_error], 0
    je .exit
    mov al, 2
.exit:
    mov ah, 0x4c
    int 0x21

; ---------------------------------------------------------------------------
; Data.

build_str       db BUILD_ID, 0
s_va            db 'VA', 0
s_va2           db 'VA2/3', 0
s_va91          db 'VA+91', 0
s_unknown       db '?0000', 0
s_15            db '15', 0
s_24            db '24', 0
s_khz           db 'kHz', 0
s_dash          db '-', 0
s_none          db 'none', 0
s_yes           db 'yes', 0
s_r0            db 'R0', 0
s_loop1024      db 'LOOP1024', 0
s_p             db 'P', 0
s_sum           db 'SUM', 0
s_vrtc          db 'VRTC', 0
s_irq2          db 'IRQ2', 0
s_no_vrtc       db 'FAIL NO-VRTC', 0
s_ff            db 'FF', 0
s_driven        db 'DRIVEN ', 0
s_float_a       db 'FLOAT A=', 0
s_float_b       db ' B=', 0
s_float_c       db ' C=', 0
s_r_eq          db 'R=', 0
s_e_eq          db ' E=', 0
s_pass          db 'PASS', 0
s_pass_sp       db ' PASS', 0
s_fail_sp       db ' FAIL', 0
s_fail_n        db 'FAIL ', 0
s_logname       db 'WSRLOG.TXT', 0
s_header        db 'stage,build,machine,khz,board,cr27,row,param,run,count,vrtc,result', 13, 10
s_header_end:

    align 2
tick            dw 0
t0              dw 0
old_vector      dw 0, 0
machine_word    dw 0
machine_str     dw s_va
rate_str        dw s_15
board_str       dw s_dash
row_name        dw s_r0
row_param       dw s_dash
count_lo        dw 0
count_hi        dw 0
fail_count      dw 0
log_handle      dw 0
lb_ptr          dw lb_buf
old_mask        db 0
row_mask        db 0
row_run         db 0
vrtc_installed  db 0
log_error       db 0
probe_class     db 0
probe_values    times 6 db 0
cr27_str        db '--', 0
param_buf       times 8 db 0
dec_buf         times 10 db 0
dec_buf_end:
lb_buf          times LB_SIZE db 0
