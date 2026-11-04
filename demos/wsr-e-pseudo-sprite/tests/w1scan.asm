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
; OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
; ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;
; W1SCAN: no arguments = read only; /P = SR06 signatures at all seven
; legal settings; /W n = provisional GA-98NB-style wake at ONE setting.
; Not an electrical presence test. No IRQ hook, timing loop, memory window,
; relay, GR bank, CRTC or VRAM writes. Wake changes misc/setup and may leave
; those enabled if the prior reads were FF (unknown, not restorable).
bits 16
cpu 186
org 100h
jmp main
%define WSRE_LOG_NAME 'WSRSCAN.TXT'
%include "wsre.inc"
stage_str db 'W1',0
prog_str db 'W1SCAN',0
mode db 0
selected db 0
setting db 0
matches db 0
found db 0
old_index db 0
old_sr db 0
u1 db 0
l1 db 0
u2 db 0
l2 db 0
old_setup db 0
old_misc db 0
idx_port dw 0
data_port dw 0
misc_port dw 0
settings db 0,1,2,3,5,6,7
s_mode db 'MODE',0
s_read db 'READ_ONLY',0
s_probe db 'SR06_ONLY',0
s_wake_mode db 'PROVISIONAL_WAKE_SINGLE',0
s_raw db 'RAW',0
s_sig db 'SR06',0
s_wake db 'WAKE',0
s_restore db 'WAKE_RESTORE',0
s_idx db 'INDEX=',0
s_data db ' DATA=',0
s_before db ' BEFORE=',0
s_u1 db ' U1=',0
s_l1 db ' L1=',0
s_u2 db ' U2=',0
s_l2 db ' L2=',0
s_match db ' MATCH',0
s_nomatch db ' NO_MATCH',0
s_none_scan db 'NONE_INCONCLUSIVE',0
s_multiple db 'MULTIPLE',0
s_found db 'MATCH ',0
s_setup db 'SETUP=',0
s_misc db ' MISC=',0
s_unknown_restore db ' FF_PRIOR_NOT_RESTORED',0
s_known_restore db ' RESTORED_READ_VALUES',0
s_error db 'WSRE_SCAN_USAGE: W1SCAN [/P | /W 0..7 excluding 4]',0
s_target_error db 'WSRE_SCAN_TARGET: V3 VA only',0
s_io_error db 'WSRE_SCAN_LOG_IO',13,10
s_io_error_end:

main:
    push cs
    pop ds
    push cs
    pop es
    cld
    call detect_machine
    call detect_rate
    ; Reject unsupported machines and non-V3 before any OUT.
    cmp word [machine_word],0xfffd
    jae .machine_ok
    jmp target_error
.machine_ok:
    mov dx,0x153
    in al,dx
    test al,0x40
    jnz .v3_ok
    jmp target_error
.v3_ok:
    xor bx,bx
    mov bl,[0x80]
    mov byte [bx+0x81],0
    mov si,0x81
    call skip_spaces
    or al,al
    jz start
    cmp al,'/'
    je .switch
    jmp usage
.switch:
    inc si
    lodsb
    and al,0xdf
    cmp al,'P'
    je .probe
    cmp al,'W'
    je .wake
    jmp usage
.probe:
    mov byte [mode],1
    call skip_spaces
    or al,al
    jz start
    jmp usage
.wake:
    mov byte [mode],2
    call skip_spaces
    sub al,'0'
    cmp al,7
    ja usage
    cmp al,4
    je usage
    mov [selected],al
    inc si
    call skip_spaces
    or al,al
    jnz usage
start:
    call log_open
    cmp byte [log_error],0
    je .opened
    mov al,3
    jmp finish
.opened:
    call emit_banner
    mov word [row_name],s_mode
    mov word [row_param],prog_str
    call check_begin
    mov si,s_read
    cmp byte [mode],0
    je .mode_text
    mov si,s_probe
    cmp byte [mode],1
    je .mode_text
    mov si,s_wake_mode
.mode_text:
    call lb_str
    call lb_emit
    mov bp,settings
    mov cx,7
.loop:
    cmp byte [log_error],0
    jne .io_error
    mov al,[bp]
    inc bp
    cmp byte [mode],2
    jne .do
    cmp al,[selected]
    jne .next
.do:
    mov [setting],al
    add al,'0'
    mov [param_buf],al
    mov byte [param_buf+1],0
    mov word [row_param],param_buf
    mov al,[setting]
    shl al,1
    add al,0xe0
    mov ah,0x44
    mov [idx_port],ax
    inc ah
    mov [data_port],ax
    mov ah,0x42
    mov [misc_port],ax
    call raw_row
    cmp byte [log_error],0
    jne .io_error
    cmp byte [mode],0
    je .next
    cmp byte [mode],2
    jne .signature
    call wake_candidate
.signature:
    call signature
    cmp byte [mode],2
    jne .next
    call restore_wake
.next:
    loop .loop
    mov word [row_name],s_sum
    mov word [row_param],prog_str
    call check_begin
    mov si,s_read
    cmp byte [mode],0
    je .emit_summary
    mov si,s_none_scan
    cmp byte [matches],0
    je .emit_summary
    mov si,s_multiple
    cmp byte [matches],1
    jne .emit_summary
    mov si,s_found
    call lb_str
    mov al,[found]
    add al,'0'
    call lb_char
    jmp .summary_done
.emit_summary:
    call lb_str
.summary_done:
    call lb_emit
    xor al,al
    cmp byte [mode],0
    je finish
    cmp byte [matches],1
    je finish
    mov al,1
    cmp byte [matches],0
    jne finish
    mov al,2
    jmp finish
.io_error:
    mov al,3
finish:
    push ax
    mov bx,[log_handle]
    or bx,bx
    jz .closed
    mov ah,0x3e
    int 0x21
    jnc .closed
    mov byte [log_error],1
.closed:
    pop ax
    cmp byte [log_error],0
    je .exit
    mov bx,1
    mov dx,s_io_error
    mov cx,s_io_error_end-s_io_error
    mov ah,0x40
    int 0x21
    mov al,3
.exit:
    mov ah,0x4c
    int 0x21

target_error:
    mov si,s_target_error
    jmp error_message
usage:
    mov si,s_error
error_message:
    mov word [row_name],s_mode
    mov word [row_param],prog_str
    call check_begin
    call lb_str
    call lb_emit
    mov al,3
    jmp finish
skip_spaces:
    mov al,[si]
    cmp al,' '
    je .skip
    cmp al,9
    jne .done
.skip:
    inc si
    jmp skip_spaces
.done:
    ret

raw_row:
    pusha
    mov word [row_name],s_raw
    call check_begin
    mov si,s_idx
    call lb_str
    mov ax,[idx_port]
    call lb_hex16
    mov si,s_data
    call lb_str
    mov ax,[data_port]
    call lb_hex16
    mov si,s_r_eq
    call lb_str
    mov dx,[data_port]
    in al,dx
    call lb_hex8
    call lb_emit
    popa
    ret

; No DOS calls while the SR06 state is temporarily changed.
signature:
    pusha
    mov dx,[idx_port]
    in al,dx
    mov [old_index],al
    mov al,6
    out dx,al
    mov dx,[data_port]
    in al,dx
    mov [old_sr],al
    mov al,0x12
    out dx,al
    in al,dx
    mov [u1],al
    xor al,al
    out dx,al
    in al,dx
    mov [l1],al
    mov al,0x12
    out dx,al
    in al,dx
    mov [u2],al
    xor al,al
    out dx,al
    in al,dx
    mov [l2],al
    ; Preserve prior unlocked state; otherwise leave SR06 locked.
    cmp byte [old_sr],0x12
    jne .index
    mov al,0x12
    out dx,al
.index:
    mov al,[old_index]
    cmp al,0x1f
    ja .check
    mov dx,[idx_port]
    out dx,al
.check:
    mov word [row_name],s_sig
    call check_begin
    mov si,s_before
    call lb_str
    mov al,[old_sr]
    call lb_hex8
    mov si,s_u1
    call lb_str
    mov al,[u1]
    call lb_hex8
    mov si,s_l1
    call lb_str
    mov al,[l1]
    call lb_hex8
    mov si,s_u2
    call lb_str
    mov al,[u2]
    call lb_hex8
    mov si,s_l2
    call lb_str
    mov al,[l2]
    call lb_hex8
    mov si,s_nomatch
    cmp byte [u1],0x12
    jne .result
    cmp byte [l1],0x0f
    jne .result
    cmp byte [u2],0x12
    jne .result
    cmp byte [l2],0x0f
    jne .result
    inc byte [matches]
    mov al,[setting]
    mov [found],al
    mov si,s_match
.result:
    call lb_str
    call lb_emit
    popa
    ret

wake_candidate:
    pusha
    mov dx,0x46e8
    in al,dx
    mov [old_setup],al
    mov dx,[misc_port]
    add dh,0x0a             ; VGA misc output read: 4CE0 + offset
    in al,dx
    mov [old_misc],al
    mov dx,0x46e8
    mov al,0x18
    out dx,al
    mov dx,[misc_port]
    mov al,1
    out dx,al
    mov dx,0x46e8
    mov al,8
    out dx,al
    mov dx,[misc_port]
    mov al,0xe3
    out dx,al
    mov dx,[idx_port]
    add dh,0x16            ; 5AE0 + offset (same provisional wake as W1PROBE)
    xor al,al
    out dx,al
    mov word [row_name],s_wake
    call wake_row
    popa
    ret
restore_wake:
    pusha
    mov al,[old_misc]
    cmp al,0xff
    je .setup
    mov dx,[misc_port]
    out dx,al
.setup:
    mov al,[old_setup]
    cmp al,0xff
    je .report
    mov dx,0x46e8
    out dx,al
.report:
    mov word [row_name],s_restore
    call wake_row
    popa
    ret
wake_row:
    call check_begin
    mov si,s_setup
    call lb_str
    mov al,[old_setup]
    call lb_hex8
    mov si,s_misc
    call lb_str
    mov al,[old_misc]
    call lb_hex8
    mov si,s_unknown_restore
    cmp byte [old_setup],0xff
    je .text
    cmp byte [old_misc],0xff
    je .text
    mov si,s_known_restore
.text:
    call lb_str
    call lb_emit
    ret
