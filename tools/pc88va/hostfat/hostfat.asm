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
; IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
; OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
; IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
; INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
; BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
; USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
; ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
; (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
; THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

bits 16
cpu 186
org 0
[warning -reloc-abs-word]       ; flat SYS image intentionally uses 16-bit offsets

%define NP2_VALUE_PORT          0x07ed
%define NP2_STRING_PORT         0x07ef

%define REQUEST_SIZE            0
%define REQUEST_UNIT            1
%define REQUEST_COMMAND         2
%define REQUEST_STATUS          3
%define REQUEST_MEDIA           13
%define REQUEST_TRANSFER        14
%define REQUEST_COUNT           18
%define REQUEST_START           20

%define COMMAND_INITIALIZE      0
%define COMMAND_MEDIA_CHECK     1
%define COMMAND_BUILD_BPB       2
%define COMMAND_READ            4
%define COMMAND_WRITE           8
%define COMMAND_WRITE_VERIFY    9
%define COMMAND_OPEN            0x0d
%define COMMAND_CLOSE           0x0e
%define COMMAND_REMOVABLE       0x0f

%define STATUS_DONE             0x0100
%define STATUS_BUSY             0x0200
%define STATUS_WRITE_PROTECT    0x8100
%define STATUS_UNKNOWN_COMMAND  0x8103
%define STATUS_SECTOR_NOT_FOUND 0x8108

%define MEDIA_ID                0xf0
; Keep the DOS-visible data-cluster count at 4084, below the FAT16 cutoff.
; The host snapshot backing remains 8 MiB; its final six sectors are hidden.
%define TOTAL_SECTORS           8186

device_header:
	dd 0xffffffff
	dw 0x2000                    ; PC-Engine non-IBM block format
	dw strategy
	dw interrupt
	db 1                         ; units
	dw device_name
	dw 0                         ; device-name segment is this driver

device_name:
	db 'HOSTFAT', 0

align 2
request_pointer:
	dd 0

bpb_pointer_list:
	dw bpb

bpb:
	dw 1024                      ; bytes per sector
	db 2                         ; sectors per cluster
	dw 0                         ; reserved sectors
	db 2                         ; FAT copies
	dw 128                       ; root entries
	dw TOTAL_SECTORS
	db MEDIA_ID
	dw 7                         ; sectors per FAT
	db 0x90                      ; PC-Engine/RDBMS-compatible filler

strategy:
	mov [cs:request_pointer], bx
	mov [cs:request_pointer + 2], es
	retf

interrupt:
	pusha
	push ds
	push es
	push cs
	pop ds
	les bx, [request_pointer]
	mov al, [es:bx + REQUEST_COMMAND]
	cmp al, COMMAND_INITIALIZE
	je initialize
	cmp al, COMMAND_MEDIA_CHECK
	je media_check
	cmp al, COMMAND_BUILD_BPB
	je build_bpb
	cmp al, COMMAND_READ
	je read_sectors
	cmp al, COMMAND_WRITE
	je write_protected
	cmp al, COMMAND_WRITE_VERIFY
	je write_protected
	cmp al, COMMAND_OPEN
	je open_close
	cmp al, COMMAND_CLOSE
	je open_close
	cmp al, COMMAND_REMOVABLE
	je removable
	mov ax, STATUS_UNKNOWN_COMMAND
	jmp finish

media_check:
	mov byte [es:bx + REQUEST_TRANSFER], 1
	mov ax, STATUS_DONE
	jmp finish

build_bpb:
	mov word [es:bx + REQUEST_COUNT], bpb
	mov word [es:bx + REQUEST_COUNT + 2], cs
	mov byte [es:bx + REQUEST_MEDIA], MEDIA_ID
	mov ax, STATUS_DONE
	jmp finish

read_sectors:
	call hostfat_read
	test al, al
	jnz read_error
	mov ax, STATUS_DONE
	jmp finish

read_error:
	mov word [es:bx + REQUEST_COUNT], 0
	mov ax, STATUS_SECTOR_NOT_FOUND
	jmp finish

write_protected:
	mov ax, STATUS_WRITE_PROTECT
	jmp finish

open_close:
	mov ax, STATUS_DONE
	jmp finish

removable:
	mov ax, STATUS_BUSY
	jmp finish

initialize:
	call hostfat_probe
	jc initialize_unavailable
	mov byte [es:bx + REQUEST_MEDIA], 1
	mov word [es:bx + REQUEST_COUNT], bpb_pointer_list
	mov word [es:bx + REQUEST_COUNT + 2], cs
	mov si, ready_message
	call display_message
	jmp initialize_end

initialize_unavailable:
	mov byte [es:bx + REQUEST_MEDIA], 0
	mov word [es:bx + REQUEST_COUNT], 0
	mov word [es:bx + REQUEST_COUNT + 2], 0
	mov si, unavailable_message
	call display_message

initialize_end:
	mov ax, resident_end
	add ax, 15
	mov cl, 4
	shr ax, cl
	mov dx, cs
	add ax, dx
	mov word [es:bx + REQUEST_TRANSFER], 0
	mov word [es:bx + REQUEST_TRANSFER + 2], ax
	mov ax, STATUS_DONE

finish:
	mov [es:bx + REQUEST_STATUS], ax
	pop es
	pop ds
	popa
	retf

hostfat_probe:
	push cx
	push dx
	push si
	mov dx, NP2_STRING_PORT
	mov si, check_command
	mov cx, check_command_end - check_command
	cld
	rep outsb
	mov si, protocol_signature
	mov cx, protocol_signature_end - protocol_signature
.compare:
	in al, dx
	cmp al, [si]
	jne .failed
	inc si
	loop .compare
	clc
	jmp short .done
.failed:
	stc
.done:
	pop si
	pop dx
	pop cx
	ret

hostfat_read:
	push cx
	push dx
	push si
	mov dx, NP2_VALUE_PORT
	mov si, request_pointer
	mov cx, 4
	cld
	rep outsb
	mov dx, NP2_STRING_PORT
	mov si, read_command
	mov cx, read_command_end - read_command
	rep outsb
	mov dx, NP2_VALUE_PORT
	in al, dx
	pop si
	pop dx
	pop cx
	ret

display_message:
	pusha
	push ds
	push es
	mov dx, 0x8000
	mov ah, 2
	int 0x83
	pop es
	pop ds
	popa
	ret

check_command:
	db 'check_hostfat'
check_command_end:

read_command:
	db 'read_hostfat1'
read_command_end:

protocol_signature:
	db 'H1'
protocol_signature_end:

ready_message:
	db 13, 10, 'HOSTFAT read-only drive ready', 13, 10, 0

unavailable_message:
	db 13, 10, 'HOSTFAT unavailable (start vaeg with --hostfat-dir)', 13, 10, 0

resident_end:
