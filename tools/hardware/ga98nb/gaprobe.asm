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
; GAPROBE.COM (M166): an I-O DATA GA-98NBIV (Cirrus Logic CL-GD5434) in a
; PC-88VA bus slot, port set y=0 (readme-disk.txt). Plain 8086/V30 code, no
; PC-98 BIOS. Every card access is logged to GAPROBE.LOG (appended,
; committed line by line) and the screen before it runs, except inside the
; window sections, which run with interrupts off and are logged from
; memory afterwards.
;
;   GAPROBE BASELINE [/BUS]       reads only (no card or window writes)
;   GAPROBE IDENTIFY-WORD         setup, OUT16 44E0h,1206h, SR06, CR27
;   GAPROBE IDENTIFY-BYTE         the same with 8-bit OUTs
;   GAPROBE WINDOW-READ [/SET]    identify; banks A -> B -> A read at E000:OFS
;   GAPROBE WINDOW-TEST [/SET] [/WIN2]  identify; save, write, compare, restore
;   GAPROBE BOARD                 reads only: the odd board ports 40E1h-5FE1h
;   GAPROBE SCAN                  identify; reads E0000h-EFFFFh (port 152h) and
;                                 A0000h-DFFFFh (port 153h) on the bus slot
;
; Options: /BUS (baseline also reads E0000h released to the bus slot),
; /SET (SR07=01h GR05=40h GR06=05h GR0B=2Dh for the window, restored after),
; /WORD (window modes identify with the 16-bit OUT), /WIN2 (window-test
; also checks E4000h with GR0A), /BANK=h (port 152h ROM0 bank for the bus
; slot, default F), /R152=hh (port 152h value restored, default 00),
; /OFS=hhhh (window offset, default 0200), /A=hh /B=hh (banks, 00 and 01).
; Version 2: /INIT (window and scan modes: the full register set of the
; PC-98 initialisation, SR00-SR1F, CR00-CR1B, GR00-GR0B, before the window;
; the card stays initialised; 40E1h is written only by /RELAY),
; /SR0F=hh (SR0F for /INIT, default 3D), /BOARD (identify and window modes:
; the board ports read again at the end), /R153=hh (port 153h restored by
; SCAN, default the value read; without a plausible value A0000h-DFFFFh is
; not scanned). /RELAY=hh (window and scan modes: OUT 40E1h,hh and
; 40E2h,00 as a 16-bit OUT 40E1h,00hh splits, after the registers and before
; the window; 40E1h,00 and 40E2h,00 afterwards. On PC-98 the card's mode
; writes 00B3h (card display) or 00B2h, and 0000h leaving it.)

		cpu	186
		org	100h

		jmp	start

; ---------------------------------------------------------------------------
; Options and state.
opt_bus		db	0
opt_set		db	0
opt_word	db	0
opt_win2	db	0
opt_init	db	0
opt_relay	db	0
relay_given	db	0
opt_board	db	0
opt_sr0f	db	3dh
opt_r153	db	0
r153_given	db	0
r153_ok		db	0
opt_bank	db	0fh
opt_r152	db	00h		; set from port 152h unless /R152= is given
r152_given	db	0
opt_a		db	00h
opt_b		db	01h
opt_ofs		dw	0200h
mode		db	0
cli_mode	db	0		; 1: lines go to MEMLOG
handle		dw	0ffffh
mem_ptr		dw	MEMLOG
verdict		db	0		; 2 I/O PASS, 1 SR06 only, 0 none
sr06_val		db	0
cr27_val		db	0
saved		times	16 db 0		; SR02 SR04 SR07 SR17 GR05 GR06 GR08 GR09 GR0A GR0B MISC
a_save		times	8 db 0
b_save		times	8 db 0
rd1		times	8 db 0
rd2		times	8 db 0
rd3		times	8 db 0
rd4		times	8 db 0
rd5		times	8 db 0
pat_a		db	0a5h, 5ah, 3ch, 0c3h
pat_b		db	96h, 69h, 0fh, 0f0h
failures	db	0
r152_read	db	0
r153_read	db	0
smbc_known	db	00h, 01h, 04h, 08h, 09h, 0ch
		align	2
dump_seg	dw	0

PORT_SETUP	equ	46e8h
PORT_MISC_W	equ	42e0h
PORT_MISC_R	equ	4ce0h
PORT_STAT	equ	5ae0h
PORT_SEQ_I	equ	44e0h
PORT_SEQ_D	equ	45e0h
PORT_CRT_I	equ	54e0h
PORT_CRT_D	equ	55e0h
PORT_GR_I	equ	4ee0h
PORT_GR_D	equ	4fe0h
PORT_MEMMAP	equ	0152h
PORT_SMBC	equ	0153h

; ---------------------------------------------------------------------------
start:		cld
		call	parse
		cmp	byte [mode],0
		jne	.mode_ok
		mov	dx,s_usage
		mov	ah,09h
		int	21h
		mov	ax,4c02h
		int	21h
.mode_ok:	call	open_log
		call	header
		mov	al,[mode]
		cmp	al,1
		jne	.m2
		call	do_baseline
		jmp	.done
.m2:		cmp	al,2
		jne	.m3
		mov	byte [opt_word],1
		call	identify
		jmp	.done
.m3:		cmp	al,3
		jne	.m4
		mov	byte [opt_word],0
		call	identify
		jmp	.done
.m4:		cmp	al,4
		jne	.m5
		call	do_window
		jmp	.done
.m5:		cmp	al,5
		jne	.m6
		call	do_window
		jmp	.done
.m6:		cmp	al,6
		jne	.m7
		call	do_board
		jmp	.done
.m7:		call	do_window
.done:		mov	si,s_end
		call	put_line
		mov	bx,[handle]
		cmp	bx,0ffffh
		je	.exit
		mov	ah,3eh
		int	21h
.exit:		mov	al,[failures]
		mov	ah,4ch
		int	21h

; ---------------------------------------------------------------------------
; The command line: a mode word and options.
parse:		mov	si,81h
		mov	cl,[80h]
		xor	ch,ch
		mov	di,cmdline
		jcxz	.copied
.up:		lodsb
		cmp	al,'a'
		jb	.st
		cmp	al,'z'
		ja	.st
		sub	al,20h
.st:		stosb
		loop	.up
.copied:	mov	byte [di],0
		mov	si,cmdline
.token:		call	skip_blank
		cmp	byte [si],0
		je	.end
		cmp	byte [si],'/'
		je	.option
		mov	bx,modes		; a mode word
		mov	dl,1
.try:		mov	di,[bx]
		or	di,di
		jz	.unknown
		push	si
		call	match_word
		pop	ax
		jnc	.matched
		mov	si,ax
		add	bx,2
		inc	dl
		jmp	.try
.matched:	mov	[mode],dl
		jmp	.token
.unknown:	mov	byte [mode],0
		ret
.option:	inc	si
		mov	di,o_bus
		call	match_flag
		jnc	.bus
		mov	di,o_set
		call	match_flag
		jnc	.set
		mov	di,o_word
		call	match_flag
		jnc	.word
		mov	di,o_win2
		call	match_flag
		jnc	.win2
		mov	di,o_bank
		call	match_flag
		jnc	.bank
		mov	di,o_r152
		call	match_flag
		jnc	.r152
		mov	di,o_ofs
		call	match_flag
		jnc	.ofs
		mov	di,o_a
		call	match_flag
		jnc	.a
		mov	di,o_b
		call	match_flag
		jnc	.b
		mov	di,o_init
		call	match_flag
		jnc	.init
		mov	di,o_board
		call	match_flag
		jnc	.board
		mov	di,o_sr0f
		call	match_flag
		jnc	.sr0f
		mov	di,o_r153
		call	match_flag
		jnc	.r153
		mov	di,o_relay
		call	match_flag
		jnc	.relay
		jmp	.unknown
.relay:		call	hex_value
		mov	[opt_relay],al
		mov	byte [relay_given],1
		jmp	.token
.init:		mov	byte [opt_init],1
		jmp	.token
.board:		mov	byte [opt_board],1
		jmp	.token
.sr0f:		call	hex_value
		mov	[opt_sr0f],al
		jmp	.token
.r153:		call	hex_value
		mov	[opt_r153],al
		mov	byte [r153_given],1
		jmp	.token
.bus:		mov	byte [opt_bus],1
		jmp	.token
.set:		mov	byte [opt_set],1
		jmp	.token
.word:		mov	byte [opt_word],1
		jmp	.token
.win2:		mov	byte [opt_win2],1
		jmp	.token
.bank:		call	hex_value
		and	al,0fh
		mov	[opt_bank],al
		jmp	.token
.r152:		call	hex_value
		mov	[opt_r152],al
		mov	byte [r152_given],1
		jmp	.token
.ofs:		call	hex_value
		and	ax,3ff8h		; inside the 16 KiB page, 8 bytes
		mov	[opt_ofs],ax
		jmp	.token
.a:		call	hex_value
		mov	[opt_a],al
		jmp	.token
.b:		call	hex_value
		mov	[opt_b],al
		jmp	.token
.end:		ret

skip_blank:	cmp	byte [si],' '
		je	.s
		cmp	byte [si],9
		je	.s
		ret
.s:		inc	si
		jmp	skip_blank

; DI word (0-terminated) against SI up to a blank or the end: CF clear and
; SI past it if equal.
match_word:	mov	al,[di]
		or	al,al
		jz	.tail
		cmp	al,[si]
		jne	.no
		inc	si
		inc	di
		jmp	match_word
.tail:		mov	al,[si]
		cmp	al,' '
		je	.yes
		or	al,al
		je	.yes
		cmp	al,'/'
		je	.yes
.no:		stc
		ret
.yes:		clc
		ret

; DI flag name against SI (after '/'): CF clear and SI past it if it
; starts SI ('=' or a blank or the end follows).
match_flag:	push	si
.c:		mov	al,[di]
		or	al,al
		jz	.tail
		cmp	al,[si]
		jne	.no
		inc	si
		inc	di
		jmp	.c
.tail:		mov	al,[si]
		cmp	al,'='
		je	.yes
		cmp	al,' '
		je	.yes
		cmp	al,'/'
		je	.yes
		or	al,al
		je	.yes
.no:		pop	si
		stc
		ret
.yes:		pop	ax
		clc
		ret

; '=hhhh' at SI: AX (SI past it).
hex_value:	xor	ax,ax
		cmp	byte [si],'='
		jne	.done
		inc	si
		xor	dx,dx
.d:		mov	al,[si]
		sub	al,'0'
		jb	.end
		cmp	al,9
		jbe	.add
		sub	al,7
		cmp	al,10
		jb	.end
		cmp	al,15
		ja	.end
.add:		shl	dx,4
		or	dl,al
		inc	si
		jmp	.d
.end:		mov	ax,dx
.done:		ret

; ---------------------------------------------------------------------------
; The log: GAPROBE.LOG opened for appending (created if missing).
open_log:	mov	ax,3d01h
		mov	dx,s_logname
		int	21h
		jnc	.opened
		mov	ah,3ch
		xor	cx,cx
		mov	dx,s_logname
		int	21h
		jc	.none
.opened:	mov	[handle],ax
		mov	bx,ax
		mov	ax,4202h
		xor	cx,cx
		xor	dx,dx
		int	21h
		ret
.none:		mov	word [handle],0ffffh
		mov	dx,s_nolog
		mov	ah,09h
		int	21h
		ret

; The line at LINE (DI its end) with CR LF: to the screen and the log, or
; to MEMLOG while interrupts are off.
line_end:	mov	ax,0a0dh
		stosw
		mov	cx,di
		sub	cx,line
		cmp	byte [cli_mode],0
		je	.now
		mov	si,line			; append to MEMLOG
		mov	di,[mem_ptr]
		cmp	di,MEMLOG_END - 100
		jae	.full
		rep	movsb
		mov	[mem_ptr],di
.full:		ret
.now:		push	cx
		mov	bx,1
		mov	dx,line
		mov	ah,40h
		int	21h
		pop	cx
		mov	bx,[handle]
		cmp	bx,0ffffh
		je	.out
		mov	dx,line
		mov	ah,40h
		int	21h
		mov	bx,[handle]
		mov	ah,68h			; commit: the line is on the disk
		int	21h
.out:		ret

; MEMLOG to the screen and the log (interrupts on again).
flush_mem:	mov	cx,[mem_ptr]
		sub	cx,MEMLOG
		jcxz	.done
		push	cx
		mov	bx,1
		mov	dx,MEMLOG
		mov	ah,40h
		int	21h
		pop	cx
		mov	bx,[handle]
		cmp	bx,0ffffh
		je	.reset
		mov	dx,MEMLOG
		mov	ah,40h
		int	21h
		mov	bx,[handle]
		mov	ah,68h
		int	21h
.reset:		mov	word [mem_ptr],MEMLOG
.done:		ret

; Line helpers: DI in LINE.
put_line:	mov	di,line			; SI: a whole line
		call	put_str
		jmp	line_end
put_str:	lodsb
		or	al,al
		jz	.done
		stosb
		jmp	put_str
.done:		ret
put_hex8:	push	ax
		shr	al,4
		call	.nib
		pop	ax
		push	ax
		and	al,0fh
		call	.nib
		pop	ax
		ret
.nib:		add	al,'0'
		cmp	al,'9'
		jbe	.o
		add	al,7
.o:		stosb
		ret
put_hex16:	push	ax
		mov	al,ah
		call	put_hex8
		pop	ax
		jmp	put_hex8
put_space:	mov	al,' '
		stosb
		ret

; ---------------------------------------------------------------------------
; Logged I/O: the line first, then the access.
; OUT8 DX <- AL
p_out8:		push	ax
		push	dx
		mov	di,line
		mov	si,s_out8
		call	put_str
		mov	ax,dx
		call	put_hex16
		mov	si,s_arrow
		call	put_str
		pop	dx
		pop	ax
		push	ax
		push	dx
		call	put_hex8
		call	line_end
		pop	dx
		pop	ax
		out	dx,al
		ret
; OUT16 DX <- AX
p_out16:	push	ax
		push	dx
		mov	di,line
		mov	si,s_out16
		call	put_str
		mov	ax,dx
		call	put_hex16
		mov	si,s_arrow
		call	put_str
		pop	dx
		pop	ax
		push	ax
		push	dx
		call	put_hex16
		call	line_end
		pop	dx
		pop	ax
		out	dx,ax
		ret
; IN8 DX -> AL (a line before and one after)
p_in8:		push	dx
		mov	di,line
		mov	si,s_in8
		call	put_str
		mov	ax,dx
		call	put_hex16
		call	line_end
		pop	dx
		in	al,dx
		push	ax
		mov	di,line
		mov	si,s_value
		call	put_str			; (PUT_STR uses AL)
		pop	ax
		push	ax
		call	put_hex8
		call	line_end
		pop	ax
		ret

; An indexed register: index AL at DX, data at DX + 100h. read_reg: AL out.
read_reg:	call	p_out8
		add	dh,1
		call	p_in8
		ret
; write_reg: index AL, value AH.
write_reg:	push	ax
		call	p_out8
		pop	ax
		mov	al,ah
		add	dh,1
		jmp	p_out8

; ---------------------------------------------------------------------------
header:		mov	di,line
		mov	si,s_title
		call	put_str
		push	es
		mov	ax,0f000h
		mov	es,ax
		mov	bx,[es:0fffeh]
		pop	es
		mov	si,s_va
		cmp	bx,0ffffh
		je	.m
		mov	si,s_va2
		cmp	bx,0fffeh
		je	.m
		mov	si,s_va91
		cmp	bx,0fffdh
		je	.m
		mov	si,s_unknown
.m:		call	put_str
		mov	si,s_id
		call	put_str
		mov	ax,bx
		call	put_hex16
		call	line_end
		mov	di,line
		mov	si,s_cmd
		call	put_str
		mov	si,cmdline
		call	put_str
		call	line_end
		mov	dx,PORT_MEMMAP		; port 152h as read: the restore
		in	al,dx			; value if it is a plausible bank
		mov	[r152_read],al		; pair (ROM0 0-5, ROM1 0-1), else 00
		cmp	byte [r152_given],0
		jne	.given
		mov	ah,al
		and	ah,0fh
		cmp	ah,5
		ja	.implausible
		mov	ah,al
		shr	ah,4
		cmp	ah,1
		ja	.implausible
		mov	[opt_r152],al
		jmp	.given
.implausible:	mov	byte [opt_r152],0
.given:		mov	di,line
		mov	si,s_mm
		call	put_str
		mov	al,[r152_read]
		call	put_hex8
		mov	si,s_mm2
		call	put_str
		mov	al,[opt_r152]
		call	put_hex8
		mov	si,s_mm3
		call	put_str
		mov	al,[opt_r152]
		and	al,0f0h
		or	al,[opt_bank]
		call	put_hex8
		jmp	line_end

; ---------------------------------------------------------------------------
; BASELINE: the ports read, and E0000h/E4000h read as mapped now (and, with
; /BUS, released to the bus slot).
do_baseline:	mov	si,s_base
		call	put_line
		mov	bx,base_ports
.p:		mov	dx,[bx]
		or	dx,dx
		jz	.mem
		push	bx
		call	p_in8
		pop	bx
		add	bx,2
		jmp	.p
.mem:		mov	si,s_now
		call	put_line
		call	dump_three
		cmp	byte [opt_bus],0
		je	.done
		mov	si,s_bus
		call	put_line
		call	bus_on
		call	dump_three
		call	bus_off
		call	flush_mem
.done:		ret

; 16 bytes at E000:0000, E000:OFS, E400:0000.
dump_three:	mov	ax,0e000h
		xor	bx,bx
		call	dump16
		mov	ax,0e000h
		mov	bx,[opt_ofs]
		call	dump16
		mov	ax,0e400h
		xor	bx,bx
		jmp	dump16

; 16 bytes at AX:BX as a line (ES stays the data segment: the line is
; built with STOSB, so nothing is ever written to AX:BX).
dump16:		mov	[dump_seg],ax
		mov	di,line
		call	put_hex16
		mov	al,':'
		stosb
		mov	ax,bx
		call	put_hex16
		mov	cx,16
.b:		call	put_space
		push	ds
		mov	ds,[dump_seg]
		mov	al,[bx]
		pop	ds
		call	put_hex8
		inc	bx
		loop	.b
		jmp	line_end

; E0000h-EFFFFh to the bus slot (interrupts off, lines to memory), and back.
bus_on:		mov	di,line
		mov	si,s_out8
		call	put_str
		mov	ax,PORT_MEMMAP
		call	put_hex16
		mov	si,s_arrow
		call	put_str
		mov	al,[opt_r152]
		and	al,0f0h
		or	al,[opt_bank]
		call	put_hex8
		call	line_end		; logged while interrupts are on
		cli
		mov	byte [cli_mode],1
		mov	al,[opt_r152]
		and	al,0f0h
		or	al,[opt_bank]
		mov	dx,PORT_MEMMAP
		out	dx,al
		ret
bus_off:	mov	al,[opt_r152]
		mov	dx,PORT_MEMMAP
		out	dx,al
		mov	di,line
		mov	si,s_out8
		call	put_str
		mov	ax,PORT_MEMMAP
		call	put_hex16
		mov	si,s_arrow
		call	put_str
		mov	al,[opt_r152]
		call	put_hex8
		call	line_end
		mov	byte [cli_mode],0
		sti
		ret

; ---------------------------------------------------------------------------
; IDENTIFY: the drivers' detection (procedure 1.2), word or byte, then the
; registers the window uses. Sets VERDICT.
identify:	mov	si,s_ident_w
		cmp	byte [opt_word],0
		jne	.t
		mov	si,s_ident_b
.t:		call	put_line
		mov	dx,PORT_SETUP
		mov	al,18h
		call	p_out8
		mov	dx,PORT_MISC_W
		mov	al,01h
		call	p_out8
		mov	dx,PORT_SETUP
		mov	al,08h
		call	p_out8
		mov	dx,PORT_MISC_W
		mov	al,0e3h
		call	p_out8
		mov	dx,PORT_STAT
		mov	al,00h
		call	p_out8
		cmp	byte [opt_word],0
		je	.byte
		mov	dx,PORT_SEQ_I
		mov	ax,1206h
		call	p_out16
		jmp	.read
.byte:		mov	dx,PORT_SEQ_I
		mov	al,06h
		call	p_out8
		mov	dx,PORT_SEQ_D
		mov	al,12h
		call	p_out8
.read:		mov	dx,PORT_SEQ_D
		call	p_in8
		mov	[sr06_val],al
		mov	dx,PORT_CRT_I
		mov	al,27h
		call	read_reg
		mov	[cr27_val],al
		mov	byte [verdict],0
		cmp	byte [sr06_val],12h
		jne	.say
		mov	byte [verdict],1
		mov	al,[cr27_val]
		and	al,0fch
		cmp	al,0a8h
		jne	.say
		mov	byte [verdict],2
.say:		mov	di,line
		mov	si,s_sr06
		call	put_str
		mov	al,[sr06_val]
		call	put_hex8
		mov	si,s_cr27
		call	put_str
		mov	al,[cr27_val]
		call	put_hex8
		mov	si,s_v_none
		cmp	byte [verdict],1
		jae	.v_resp
		cmp	byte [sr06_val],0fh	; locked: the card answers, the
		jne	.v			; unlock value did not arrive
		mov	si,s_v_locked
		jmp	.v
.v_resp:	cmp	byte [verdict],1
		mov	si,s_v_sr06
		je	.v
		mov	si,s_v_pass
.v:		call	put_str
		call	line_end
		cmp	byte [verdict],0
		jne	.regs
		inc	byte [failures]
		ret
.regs:		mov	si,s_regs		; procedure 2.2.1
		call	put_line
		mov	bx,0
		mov	si,reg_list
.r:		lodsw				; AL index, AH 0 SEQ / 1 GR
		cmp	al,0ffh
		je	.misc
		push	si
		push	bx
		mov	dx,PORT_SEQ_I
		or	ah,ah
		jz	.seq
		mov	dx,PORT_GR_I
.seq:		call	read_reg
		pop	bx
		mov	[saved + bx],al
		inc	bx
		pop	si
		jmp	.r
.misc:		push	bx
		mov	dx,PORT_MISC_R
		call	p_in8
		pop	bx
		mov	[saved + bx],al
		cmp	byte [mode],3		; the identify modes read the board
		ja	.done			; ports here; the window modes later
		cmp	byte [opt_board],0
		je	.done
		call	board_ports
.done:		ret

; ---------------------------------------------------------------------------
; BOARD: the odd ports 40E1h-5FE1h read (no writes).
do_board:	mov	si,s_board
		call	put_line
board_ports:	mov	si,s_board_p
		call	put_line
		mov	dx,40e1h
.p:		push	dx
		call	p_in8
		pop	dx
		add	dh,1
		cmp	dh,60h
		jb	.p
		ret

; 40E1h <- AL, 40E2h <- 00 (a 16-bit OUT 40E1h,00xxh as two bytes).
relay_out:	push	ax
		mov	si,s_relay
		call	put_line
		pop	ax
		mov	dx,40e1h
		call	p_out8
		mov	dx,40e2h
		mov	al,0
		jmp	p_out8

; ---------------------------------------------------------------------------
; /INIT: the full register set (PC-98 initialisation order), logged.
init_full:	mov	si,s_init
		call	put_line
		mov	si,init_seq		; SEQ pairs (index, value), FF ends
		mov	dx,PORT_SEQ_I
		call	write_list
		mov	dx,PORT_SEQ_I		; SR0F as chosen
		mov	al,0fh
		mov	ah,[opt_sr0f]
		call	write_reg
		mov	dx,PORT_CRT_I		; CR11: unprotect CR00-CR07
		mov	ax,0011h
		call	write_reg
		mov	si,init_crt
		mov	dx,PORT_CRT_I
		call	write_list
		mov	si,init_gr
		mov	dx,PORT_GR_I
		call	write_list
		mov	dx,PORT_MISC_R		; MISC as it is now
		jmp	p_in8

; Pairs at SI to index port DX (data DX + 100h), until index FFh.
write_list:	lodsw
		cmp	al,0ffh
		je	.done
		push	si
		push	dx
		call	write_reg
		pop	dx
		pop	si
		jmp	write_list
.done:		ret

; ---------------------------------------------------------------------------
; WINDOW-READ and WINDOW-TEST.
do_window:	call	identify
		cmp	byte [verdict],2
		je	.go
		mov	si,s_blocked
		call	put_line
		inc	byte [failures]
		ret
.go:		cmp	byte [opt_init],0
		je	.noinit
		call	init_full
		cmp	byte [opt_board],0
		je	.plan
		call	board_ports
		jmp	.plan
.noinit:	cmp	byte [opt_board],0
		je	.noboard
		call	board_ports
.noboard:	cmp	byte [opt_set],0
		je	.asis
		mov	si,s_setting
		call	put_line
		mov	dx,PORT_SEQ_I
		mov	ax,0107h
		call	write_reg
		mov	dx,PORT_GR_I
		mov	ax,4005h
		call	write_reg
		mov	dx,PORT_GR_I
		mov	ax,0506h
		call	write_reg
		mov	dx,PORT_GR_I
		mov	ax,2d0bh
		call	write_reg
		jmp	.plan
.asis:		mov	si,s_asis
		call	put_line
.plan:		cmp	byte [relay_given],0
		je	.plan2
		mov	al,[opt_relay]
		call	relay_out
.plan2:		mov	di,line
		mov	si,s_plan
		call	put_str
		mov	ax,[opt_ofs]
		call	put_hex16
		mov	si,s_banks
		call	put_str
		mov	al,[opt_a]
		call	put_hex8
		call	put_space
		mov	al,[opt_b]
		call	put_hex8
		call	line_end
		cmp	byte [mode],7
		jne	.notscan
		call	scan
		jmp	.restore
.notscan:	cmp	byte [mode],5
		je	.test
		call	window_read
		jmp	.restore
.test:		call	window_test
.restore:	cmp	byte [relay_given],0
		je	.r2
		mov	al,0
		call	relay_out
.r2:		cmp	byte [opt_init],0
		je	.setback
		mov	si,s_init_kept
		jmp	put_line
.setback:	cmp	byte [opt_set],0
		je	.done
		mov	si,s_restoring
		call	put_line
		mov	dx,PORT_SEQ_I		; SR07, GR05, GR06, GR0B as found
		mov	al,07h
		mov	ah,[saved + 2]
		call	write_reg
		mov	dx,PORT_GR_I
		mov	al,05h
		mov	ah,[saved + 4]
		call	write_reg
		mov	dx,PORT_GR_I
		mov	al,06h
		mov	ah,[saved + 5]
		call	write_reg
		mov	dx,PORT_GR_I
		mov	al,0bh
		mov	ah,[saved + 9]
		call	write_reg
.done:		ret

; GR09 <- AL without logging (inside the window sections).
gr09:		mov	ah,al
		mov	al,09h
		mov	dx,PORT_GR_I
		out	dx,al
		mov	al,ah
		mov	dx,PORT_GR_D
		out	dx,al
		ret
gr0a:		mov	ah,al
		mov	al,0ah
		mov	dx,PORT_GR_I
		out	dx,al
		mov	al,ah
		mov	dx,PORT_GR_D
		out	dx,al
		ret

; CX bytes from SEG:OFS (AX segment) to DI.
read_win:	push	ds
		push	si
		mov	si,[opt_ofs]
		mov	ds,ax
		rep	movsb
		pop	si
		pop	ds
		ret
; CX bytes from SI to SEG:OFS (AX segment).
write_win:	push	es
		push	di
		mov	di,[opt_ofs]
		mov	es,ax
		rep	movsb
		pop	di
		pop	es
		ret

; A line: text SI, then CX bytes at BX.
show_bytes:	mov	di,line
		call	put_str
.b:		call	put_space
		mov	al,[bx]
		call	put_hex8
		inc	bx
		loop	.b
		jmp	line_end

window_read:	call	bus_on
		mov	al,[opt_a]
		call	gr09
		mov	ax,0e000h
		mov	cx,8
		mov	di,rd1
		call	read_win
		mov	al,[opt_b]
		call	gr09
		mov	ax,0e000h
		mov	cx,8
		mov	di,rd2
		call	read_win
		mov	al,[opt_a]
		call	gr09
		mov	ax,0e000h
		mov	cx,8
		mov	di,rd3
		call	read_win
		mov	al,[saved + 7]		; GR09 as found
		call	gr09
		call	bus_off
		call	flush_mem
		mov	si,s_rd_a
		mov	bx,rd1
		mov	cx,8
		call	show_bytes
		mov	si,s_rd_b
		mov	bx,rd2
		mov	cx,8
		call	show_bytes
		mov	si,s_rd_a2
		mov	bx,rd3
		mov	cx,8
		call	show_bytes
		mov	si,rd1
		mov	di,rd3
		mov	cx,8
		repe	cmpsb
		mov	si,s_ab_stable
		je	.s
		mov	si,s_ab_unstable
.s:		call	put_line
		mov	si,rd1
		mov	di,rd2
		mov	cx,8
		repe	cmpsb
		mov	si,s_ab_same
		je	.d
		mov	si,s_ab_differ
.d:		jmp	put_line

window_test:	call	bus_on
		mov	al,[opt_a]		; save both banks before any write
		call	gr09
		mov	ax,0e000h
		mov	cx,4
		mov	di,a_save
		call	read_win
		mov	al,[opt_b]
		call	gr09
		mov	ax,0e000h
		mov	cx,4
		mov	di,b_save
		call	read_win
		mov	al,[opt_a]		; the patterns
		call	gr09
		mov	ax,0e000h
		mov	cx,4
		mov	si,pat_a
		call	write_win
		mov	al,[opt_b]
		call	gr09
		mov	ax,0e000h
		mov	cx,4
		mov	si,pat_b
		call	write_win
		mov	al,[opt_a]		; A -> B -> A
		call	gr09
		mov	ax,0e000h
		mov	cx,4
		mov	di,rd1
		call	read_win
		mov	al,[opt_b]
		call	gr09
		mov	ax,0e000h
		mov	cx,4
		mov	di,rd2
		call	read_win
		mov	al,[opt_a]
		call	gr09
		mov	ax,0e000h
		mov	cx,4
		mov	di,rd3
		call	read_win
		cmp	byte [opt_win2],0	; the second window, read only
		je	.restore
		mov	al,[opt_a]
		call	gr09
		mov	al,[opt_b]
		call	gr0a
		mov	ax,0e400h
		mov	cx,4
		mov	di,rd4
		call	read_win
		mov	al,[opt_b]
		call	gr09
		mov	al,[opt_a]
		call	gr0a
		mov	ax,0e400h
		mov	cx,4
		mov	di,rd5
		call	read_win
		mov	al,[saved + 8]		; GR0A as found
		call	gr0a
.restore:	mov	al,[opt_b]		; B, then A, back as they were
		call	gr09
		mov	ax,0e000h
		mov	cx,4
		mov	si,b_save
		call	write_win
		mov	al,[opt_a]
		call	gr09
		mov	ax,0e000h
		mov	cx,4
		mov	si,a_save
		call	write_win
		mov	al,[opt_b]		; read the restored bytes back
		call	gr09
		mov	ax,0e000h
		mov	cx,4
		mov	di,rd1 + 4
		call	read_win
		mov	al,[opt_a]
		call	gr09
		mov	ax,0e000h
		mov	cx,4
		mov	di,rd2 + 4
		call	read_win
		mov	al,[saved + 7]
		call	gr09
		call	bus_off
		call	flush_mem
		mov	si,s_save_a
		mov	bx,a_save
		mov	cx,4
		call	show_bytes
		mov	si,s_save_b
		mov	bx,b_save
		mov	cx,4
		call	show_bytes
		mov	si,s_rd_a
		mov	bx,rd1
		mov	cx,4
		call	show_bytes
		mov	si,s_rd_b
		mov	bx,rd2
		mov	cx,4
		call	show_bytes
		mov	si,s_rd_a2
		mov	bx,rd3
		mov	cx,4
		call	show_bytes
		mov	si,rd1			; A, B, A against the patterns
		mov	di,pat_a
		call	cmp4
		jne	.fail
		mov	si,rd2
		mov	di,pat_b
		call	cmp4
		jne	.fail
		mov	si,rd3
		mov	di,pat_a
		call	cmp4
		jne	.fail
		mov	si,s_w1_pass
		call	put_line
		jmp	.win2
.fail:		mov	si,s_w1_fail
		call	put_line
		inc	byte [failures]
.win2:		cmp	byte [opt_win2],0
		je	.restored
		mov	si,s_w2_ab
		mov	bx,rd4
		mov	cx,4
		call	show_bytes
		mov	si,s_w2_ba
		mov	bx,rd5
		mov	cx,4
		call	show_bytes
		mov	si,rd4			; E4000h with GR0A = B shows B
		mov	di,pat_b
		call	cmp4
		jne	.w2fail
		mov	si,rd5
		mov	di,pat_a
		call	cmp4
		jne	.w2fail
		mov	si,s_w2_pass
		call	put_line
		jmp	.restored
.w2fail:	mov	si,s_w2_fail
		call	put_line
		inc	byte [failures]
.restored:	mov	si,s_rest_b
		mov	bx,rd1 + 4
		mov	cx,4
		call	show_bytes
		mov	si,s_rest_a
		mov	bx,rd2 + 4
		mov	cx,4
		call	show_bytes
		mov	si,rd1 + 4
		mov	di,b_save
		call	cmp4
		jne	.rfail
		mov	si,rd2 + 4
		mov	di,a_save
		call	cmp4
		jne	.rfail
		mov	si,s_rest_ok
		jmp	put_line
.rfail:		mov	si,s_rest_fail
		inc	byte [failures]
		jmp	put_line

; ---------------------------------------------------------------------------
; SCAN: 16 bytes at each 16 KiB of E0000h-EFFFFh (port 152h bank) and, if
; port 153h reads plausibly, of A0000h-DFFFFh (153h low nibble 0: the bus
; slot), GR09 = A. Reads only; interrupts off while released.
scan:		mov	dx,PORT_SMBC		; 153h: restore value
		in	al,dx
		mov	[r153_read],al
		cmp	byte [r153_given],0
		jne	.given
		mov	[opt_r153],al
		mov	byte [r153_ok],0
		test	al,40h			; VA mode (bit 6) and a known bank
		jz	.say
		and	al,0fh
		mov	di,smbc_known
		mov	cx,6
		repne	scasb
		jne	.say
.given:		mov	byte [r153_ok],1
.say:		mov	di,line
		mov	si,s_smbc
		call	put_str
		mov	al,[r153_read]
		call	put_hex8
		mov	si,s_mm2
		call	put_str
		mov	al,[opt_r153]
		call	put_hex8
		mov	si,s_smbc_bus
		call	put_str
		mov	al,[opt_r153]
		and	al,0f0h
		call	put_hex8
		mov	si,s_smbc_skip
		cmp	byte [r153_ok],0
		je	.s2
		mov	si,s_close
.s2:		call	put_str
		call	line_end
		mov	si,s_scan_e
		call	put_line
		call	bus_on
		mov	al,[opt_a]
		call	gr09
		mov	ax,0e000h
.e:		push	ax
		xor	bx,bx
		call	dump16
		pop	ax
		add	ax,0400h
		cmp	ax,0f000h
		jb	.e
		mov	al,[saved + 7]
		call	gr09
		call	bus_off
		call	flush_mem
		cmp	byte [r153_ok],0
		je	.done
		mov	si,s_scan_a
		call	put_line
		mov	di,line			; logged before, interrupts on
		mov	si,s_out8
		call	put_str
		mov	ax,PORT_SMBC
		call	put_hex16
		mov	si,s_arrow
		call	put_str
		mov	al,[opt_r153]
		and	al,0f0h
		call	put_hex8
		call	line_end
		cli
		mov	byte [cli_mode],1
		mov	al,[opt_r153]
		and	al,0f0h
		mov	dx,PORT_SMBC
		out	dx,al
		mov	al,[opt_a]
		call	gr09
		mov	ax,0a000h
.a:		push	ax
		xor	bx,bx
		call	dump16
		pop	ax
		add	ax,0400h
		cmp	ax,0e000h
		jb	.a
		mov	al,[saved + 7]
		call	gr09
		mov	al,[opt_r153]
		mov	dx,PORT_SMBC
		out	dx,al
		mov	di,line
		mov	si,s_out8
		call	put_str
		mov	ax,PORT_SMBC
		call	put_hex16
		mov	si,s_arrow
		call	put_str
		mov	al,[opt_r153]
		call	put_hex8
		call	line_end
		mov	byte [cli_mode],0
		sti
		call	flush_mem
.done:		ret

cmp4:		mov	cx,4
		repe	cmpsb
		ret

; ---------------------------------------------------------------------------
modes		dw	m_base, m_idw, m_idb, m_wr, m_wt, m_board, m_scan, 0
m_board		db	'BOARD', 0
m_scan		db	'SCAN', 0
o_init		db	'INIT', 0
o_board		db	'BOARD', 0
o_sr0f		db	'SR0F', 0
o_r153		db	'R153', 0
o_relay		db	'RELAY', 0
m_base		db	'BASELINE', 0
m_idw		db	'IDENTIFY-WORD', 0
m_idb		db	'IDENTIFY-BYTE', 0
m_wr		db	'WINDOW-READ', 0
m_wt		db	'WINDOW-TEST', 0
o_bus		db	'BUS', 0
o_set		db	'SET', 0
o_word		db	'WORD', 0
o_win2		db	'WIN2', 0
o_bank		db	'BANK', 0
o_r152		db	'R152', 0
o_ofs		db	'OFS', 0
o_a		db	'A', 0
o_b		db	'B', 0
base_ports	dw	4ce0h, 5ae0h, 44e0h, 45e0h, 54e0h, 55e0h, 4ee0h, 4fe0h, 40e0h
		dw	41e0h, 0
; SEQ (AH 0) and GR (AH 1) indexes read after identification, in SAVED order
reg_list	db	02h, 0, 04h, 0, 07h, 0, 17h, 0
		db	05h, 1, 06h, 1, 08h, 1, 09h, 1, 0ah, 1, 0bh, 1, 0ffh, 0
; /INIT tables: (index, value) pairs, FFh ends. SR0F comes from /SR0F.
init_seq	db	00h, 02h, 00h, 03h, 01h, 01h, 02h, 0ffh, 03h, 00h, 04h, 0eh
		db	06h, 12h, 07h, 01h, 08h, 00h, 0bh, 4ah, 0ch, 5bh, 0dh, 42h
		db	0eh, 7eh, 10h, 00h, 11h, 00h, 12h, 00h, 13h, 3fh, 14h, 00h
		db	15h, 00h, 16h, 71h, 17h, 35h, 18h, 02h, 19h, 01h, 1ah, 00h
		db	1bh, 2bh, 1ch, 2fh, 1dh, 1fh, 1eh, 33h, 1fh, 1ch, 0ffh, 0
init_crt	db	00h, 5fh, 01h, 4fh, 02h, 50h, 03h, 82h, 04h, 54h, 05h, 80h
		db	06h, 0bh, 07h, 3eh, 08h, 00h, 09h, 40h, 0ah, 00h, 0bh, 00h
		db	0ch, 00h, 0dh, 00h, 0eh, 00h, 0fh, 00h, 10h, 0eah, 11h, 0ch
		db	12h, 0dfh, 13h, 50h, 14h, 00h, 15h, 0e1h, 16h, 0ah, 17h, 0e3h
		db	18h, 0ffh, 19h, 32h, 1ah, 90h, 1bh, 22h, 0ffh, 0
init_gr		db	00h, 00h, 01h, 00h, 02h, 00h, 03h, 00h, 04h, 00h, 05h, 40h
		db	06h, 05h, 07h, 0fh, 08h, 0ffh, 09h, 00h, 0ah, 00h, 0bh, 2dh
		db	0ffh, 0

s_usage		db	'GAPROBE BASELINE [/BUS] | IDENTIFY-WORD | IDENTIFY-BYTE |',13,10
		db	'        WINDOW-READ [/SET] | WINDOW-TEST [/SET] [/WIN2] |',13,10
		db	'        BOARD | SCAN',13,10
		db	'options: /INIT /SR0F=hh /BOARD /WORD /BANK=h /R152=hh',13,10
		db	'         /R153=hh /RELAY=hh /OFS=hhhh /A=hh /B=hh',13,10
		db	'See README.TXT. Logs to GAPROBE.LOG.',13,10,'$'
s_nolog		db	'GAPROBE.LOG could not be opened: screen only.',13,10,'$'
s_logname	db	'GAPROBE.LOG', 0
s_title		db	'GAPROBE M166 v3  ', 0
s_va		db	'PC-88VA', 0
s_va2		db	'PC-88VA2/VA3', 0
s_va91		db	'PC-88VA+VA-91', 0
s_unknown	db	'UNKNOWN MACHINE', 0
s_id		db	'  F000:FFFE=', 0
s_cmd		db	'COMMAND:', 0
s_mm		db	'IN8   0152 -> ', 0
s_mm2		db	' (restore value ', 0
s_mm3		db	', bus-slot value ', 0
s_out8		db	'OUT8  ', 0
s_out16		db	'OUT16 ', 0
s_in8		db	'IN8   ', 0
s_arrow		db	' <- ', 0
s_value		db	'      -> ', 0
s_base		db	'== BASELINE: reads only', 0
s_now		db	'-- E0000h/E4000h as mapped now', 0
s_bus		db	'-- E0000h/E4000h released to the bus slot (interrupts off)', 0
s_ident_w	db	'== IDENTIFY-WORD (OUT16 44E0h,1206h)', 0
s_ident_b	db	'== IDENTIFY-BYTE (OUT8 44E0h,06h; OUT8 45E0h,12h)', 0
s_sr06		db	'SR06=', 0
s_cr27		db	'  CR27=', 0
s_v_none	db	'  RESULT: NO RESPONSE (needs isolation)', 0
s_v_sr06	db	'  RESULT: SR06 ONLY (CR27 not GD5434)', 0
s_v_locked	db	'  RESULT: SR06 LOCKED (0F: card answers, 12h not received)', 0
s_v_pass	db	'  RESULT: I/O PASS (GD5434)', 0
s_regs		db	'-- SR02 SR04 SR07 SR17 GR05 GR06 GR08 GR09 GR0A GR0B MISC', 0
s_blocked	db	'BLOCKED: the window needs I/O PASS first', 0
s_setting	db	'-- /SET: SR07=01 GR05=40 GR06=05 GR0B=2D', 0
s_asis		db	'-- window registers as found (no /SET)', 0
s_restoring	db	'-- restoring SR07 GR05 GR06 GR0B', 0
s_plan		db	'-- E000:', 0
s_banks		db	' banks A B (GR09):', 0
s_rd_a		db	'READ A (first) :', 0
s_rd_b		db	'READ B         :', 0
s_rd_a2		db	'READ A (again) :', 0
s_ab_stable	db	'A read twice: same', 0
s_ab_unstable	db	'A read twice: DIFFERENT (unstable or not memory)', 0
s_ab_same	db	'A and B: same bytes (banks not shown to differ)', 0
s_ab_differ	db	'A and B: different bytes', 0
s_save_a	db	'SAVED A        :', 0
s_save_b	db	'SAVED B        :', 0
s_w1_pass	db	'WINDOW 1: PASS (A5 5A 3C C3 / 96 69 0F F0 / A5 5A 3C C3)', 0
s_w1_fail	db	'WINDOW 1: FAIL', 0
s_w2_ab		db	'E400 GR09=A GR0A=B:', 0
s_w2_ba		db	'E400 GR09=B GR0A=A:', 0
s_w2_pass	db	'WINDOW 2: PASS (E4000h follows GR0A)', 0
s_w2_fail	db	'WINDOW 2: FAIL', 0
s_rest_b	db	'RESTORED B     :', 0
s_rest_a	db	'RESTORED A     :', 0
s_rest_ok	db	'RESTORE: OK', 0
s_rest_fail	db	'RESTORE: FAILED (bytes differ from the saved ones)', 0
s_end		db	'== END', 0
s_board		db	'== BOARD: reads only', 0
s_board_p	db	'-- board ports 40E1h-5FE1h', 0
s_init		db	'-- /INIT: SR00-SR1F, CR00-CR1B, GR00-GR0B (40E1h only by /RELAY)', 0
s_relay		db	'-- /RELAY: 40E1h (16-bit OUT as two bytes)', 0
s_init_kept	db	'-- /INIT: the card stays initialised (power off to reset)', 0
s_smbc		db	'IN8   0153 -> ', 0
s_smbc_bus	db	', bus-slot value ', 0
s_smbc_skip	db	'; A0000h-DFFFFh NOT SCANNED: value not plausible)', 0
s_close		db	')', 0
s_scan_e	db	'-- SCAN E0000h-EFFFFh on the bus slot (port 152h), GR09=A', 0
s_scan_a	db	'-- SCAN A0000h-DFFFFh on the bus slot (port 153h), GR09=A', 0

		align	2
cmdline		times	130 db 0
line		times	160 db 0
MEMLOG:		times	4096 db 0
MEMLOG_END:
