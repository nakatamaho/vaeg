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
; Z80TIME.COM: compatible (uPD70008) mode against native V30 code on a
; PC-88VA/VA2 and in vaeg (README.md). No data of any program but its own.
;
; Each row counts the blocks done in [frames] VRTCs (3000, about 50 s; a
; number after the command name changes it); more is faster. A
; compatible-mode block is entered by BRKEM and left by RETEM: EI, then 4
; x 256 passes (DJNZ, DEC C, JR NZ) of one instruction, 1024 passes in
; all; LDIR copies 1024 bytes once. CALLN calls a native IRET. BRKEM-
; RETEM is 1024 round trips a block from a native LOOP. The native rows
; are 1024 passes of DEC CX, JNZ and one instruction; REP MOVSB copies
; 1024 bytes. The code is at 20000h; "HI" data at 72800h (the VA inserts
; one wait from 40000h), "LO" data at 30000h. The VRTC handler counts
; VRTCs. The counts are also stored at GVRAM 30000h (README.md).
; Never returns: restart the machine to leave.

		cpu	186
		org	100h

%include "font.inc"			; FONT, PALETTE32

BENCH_SEG	equ	2000h		; this program, copied
HI_SEG		equ	7280h		; data above 40000h
LO_SEG		equ	3000h		; data below
MEMORY_TOP	equ	7400h
RESULT_SEG	equ	0d000h		; GVRAM 30000h (not shown)
VEC_BRKEM	equ	70h
VEC_CALLN	equ	75h
LINE_BYTES	equ	160
LINES		equ	224
GV_G0		equ	0a000h
GV_G1		equ	0c000h
TEXT_TOP	equ	16
WHITE		equ	15
YELLOW		equ	11
RED		equ	9

%macro BRKEM 1
		db	0fh, 0ffh, %1
%endmacro

start:		cld
		mov	si,81h			; [frames] from the command line
		xor	bx,bx
.arg:		lodsb
		cmp	al,' '
		je	.arg
		sub	al,'0'
		cmp	al,9
		ja	.argend
		imul	bx,bx,10
		xor	ah,ah
		add	bx,ax
		jmp	.arg
.argend:	or	bx,bx
		jz	.default
		mov	[frames],bx
.default:	mov	dx,s_banner
		mov	ah,09h
		int	21h
		mov	dx,01cbh		; 15 kHz only (port 1CBh bit 3 is 1
		in	al,dx			; at 24.8 kHz)
		test	al,08h
		jz	.khz15
		mov	dx,s_24khz
		jmp	dos_fail
.khz15:		mov	ax,cs			; the copy must not overlap this one
		add	ax,(IMAGE_TOP + 15) / 16
		cmp	ax,BENCH_SEG
		ja	.memory
		cmp	word [0002h],MEMORY_TOP
		jae	.room
.memory:	mov	dx,s_memory
		jmp	dos_fail
.room:		cli
		mov	ax,BENCH_SEG
		mov	es,ax
		xor	si,si
		xor	di,di
		mov	cx,(IMAGE_TOP + 1) / 2
		rep	movsw
		jmp	BENCH_SEG:main
dos_fail:	mov	ah,09h
		int	21h
		mov	ax,4c01h
		int	21h

; ---------------------------------------------------------------------------
main:		mov	ax,cs
		mov	ds,ax
		mov	es,ax
		mov	ss,ax
		mov	sp,0fffeh
		sti
		call	video_init
		cli
		xor	ax,ax
		mov	es,ax
		mov	word [es:0ah * 4],vrtc
		mov	[es:0ah * 4 + 2],cs
		mov	word [es:VEC_CALLN * 4],n_iret
		mov	[es:VEC_CALLN * 4 + 2],cs
		mov	[es:VEC_BRKEM * 4 + 2],cs
		push	cs
		pop	es
		mov	al,0fbh			; only VRTC (IR2)
		mov	dx,18ah
		out	dx,al
		sti
		mov	ax,[frames]		; the title: Z80TIME n VRTC
		mov	di,s_title_n
		call	decimal
		mov	si,s_title
		mov	byte [colour],RED
		xor	bx,bx
		call	text_line
		xor	bx,bx
.test:		push	bx
		mov	bx,[tests + bx]
		call	run			; DX:AX: blocks
		pop	bx
		mov	si,bx			; stored
		shl	si,1
		mov	[results + 4 + si],ax
		mov	[results + 6 + si],dx
		push	bx
		call	show
		pop	bx
		add	bx,2
		cmp	bx,N_TESTS * 2
		jb	.test
		mov	word [results],'Z8'
		mov	word [results + 2],'0T'
		mov	ax,[frames]
		mov	[results + 4 + N_TESTS * 4],ax
		mov	ax,RESULT_SEG
		mov	es,ax
		xor	di,di
		mov	si,results
		mov	cx,(4 + N_TESTS * 4 + 4) / 2
		rep	movsw
		push	cs
		pop	es
		mov	si,s_done
		mov	byte [colour],RED
		mov	bx,N_TESTS + 3
		call	text_line
bench_done:	nop				; the results are complete here
.halt:		jmp	.halt

vrtc:		push	ax
		push	dx
		inc	word [cs:tick]
		mov	al,20h
		mov	dx,188h
		out	dx,al
		pop	dx
		pop	ax
		iret

n_iret:		iret				; CALLN VEC_CALLN

; ---------------------------------------------------------------------------
; DX:AX = blocks of the test at BX in [frames] VRTCs.
run:		mov	ax,[tick]
.edge:		cmp	ax,[tick]
		je	.edge
		mov	ax,[tick]
		mov	[t0],ax
		mov	word [count],0
		mov	word [count + 2],0
.block:		push	bx
		call	bx
		pop	bx
		add	word [count],1
		adc	word [count + 2],0
		mov	ax,[tick]
		sub	ax,[t0]
		cmp	ax,[frames]
		jb	.block
		mov	ax,[count]
		mov	dx,[count + 2]
		ret

; ---------------------------------------------------------------------------
; Compatible mode. A block at CS:AX with DS0 = DX.
%macro Z80_JR 2				; opcode, target
		db	%1, ((%2) - ($ + 2)) & 0ffh
%endmacro
%macro Z80_BLOCK 1			; body macro: EI, 4 x 256 passes, RETEM
		db	0fbh			; EI
		db	21h, 00h, 01h		; LD HL,0100h
		db	0ddh, 21h, 00h, 01h	; LD IX,0100h
		db	31h, 00h, 0eh		; LD SP,0E00h
		db	0eh, 04h		; LD C,4
%%outer:	db	06h, 00h		; LD B,0
%%inner:	%1
		Z80_JR	10h, %%inner		; DJNZ
		db	0dh			; DEC C
		Z80_JR	20h, %%outer		; JR NZ
		db	0edh, 0fdh		; RETEM
%endmacro
%macro ZB_NONE 0
%endmacro
%macro ZB_LDAHL 0
		db	7eh			; LD A,(HL)
%endmacro
%macro ZB_LDHLA 0
		db	77h			; LD (HL),A
%endmacro
%macro ZB_ADD 0
		db	80h			; ADD A,B
%endmacro
%macro ZB_LDAIX 0
		db	0ddh, 7eh, 05h		; LD A,(IX+5)
%endmacro
%macro ZB_CALL 0
		db	0cdh			; CALL z_ret
		dw	z_ret
%endmacro
%macro ZB_CALLN 0
		db	0edh, 0edh, VEC_CALLN	; CALLN
%endmacro

z_none:		Z80_BLOCK ZB_NONE
z_ldahl:	Z80_BLOCK ZB_LDAHL
z_ldhla:	Z80_BLOCK ZB_LDHLA
z_add:		Z80_BLOCK ZB_ADD
z_ldaix:	Z80_BLOCK ZB_LDAIX
z_call:		Z80_BLOCK ZB_CALL
z_calln:	Z80_BLOCK ZB_CALLN
z_ret:		db	0c9h			; RET
z_ldir:		db	0fbh			; EI
		db	21h, 00h, 01h		; LD HL,0100h
		db	11h, 00h, 05h		; LD DE,0500h
		db	01h, 00h, 04h		; LD BC,1024
		db	0edh, 0b0h		; LDIR
		db	0edh, 0fdh		; RETEM
z_retem:	db	0edh, 0fdh		; RETEM

z_enter:	push	ds
		push	es
		push	bp
		xor	bx,bx
		mov	es,bx
		mov	[es:VEC_BRKEM * 4],ax
		mov	[cs:z_sp],sp
		mov	ds,dx
		BRKEM	VEC_BRKEM		; (RETEM returns here)
		mov	sp,[cs:z_sp]
		pop	bp
		pop	es
		pop	ds
		ret

%macro ZTEST 3				; name, block, data segment
%1:		mov	ax,%2
		mov	dx,%3
		jmp	z_enter
%endmacro
		ZTEST	t_z_none, z_none, HI_SEG
		ZTEST	t_z_ldahl, z_ldahl, HI_SEG
		ZTEST	t_z_ldhla, z_ldhla, HI_SEG
		ZTEST	t_z_ldahl_lo, z_ldahl, LO_SEG
		ZTEST	t_z_ldhla_lo, z_ldhla, LO_SEG
		ZTEST	t_z_add, z_add, HI_SEG
		ZTEST	t_z_ldaix, z_ldaix, HI_SEG
		ZTEST	t_z_call, z_call, HI_SEG
		ZTEST	t_z_ldir, z_ldir, HI_SEG
		ZTEST	t_z_calln, z_calln, HI_SEG
t_brkem:	push	ds			; 1024 BRKEM-RETEM round trips
		push	es
		push	bp
		xor	bx,bx
		mov	es,bx
		mov	word [es:VEC_BRKEM * 4],z_retem
		mov	[cs:z_sp],sp
		mov	bx,HI_SEG
		mov	ds,bx
		mov	cx,1024
.l:		BRKEM	VEC_BRKEM
		loop	.l
		mov	sp,[cs:z_sp]
		pop	bp
		pop	es
		pop	ds
		ret

; Native code.
%macro NATIVE 2				; data segment, instruction
		push	ds
		mov	bx,%1
		mov	ds,bx
		mov	bx,0100h
		mov	cx,1024
%%l:		%2
		dec	cx
		jnz	%%l
		pop	ds
		ret
%endmacro
t_n_none:	NATIVE	HI_SEG, nop
t_n_read:	NATIVE	HI_SEG, {mov al,[bx]}
t_n_write:	NATIVE	HI_SEG, {mov [bx],al}
t_n_read_lo:	NATIVE	LO_SEG, {mov al,[bx]}
t_n_add:	NATIVE	HI_SEG, {add al,bl}
t_n_call:	NATIVE	HI_SEG, {call n_ret}
n_ret:		ret
t_n_movsb:	push	ds
		push	es
		mov	ax,HI_SEG
		mov	ds,ax
		mov	es,ax
		mov	si,0100h
		mov	di,0500h
		mov	cx,1024
		rep	movsb
		pop	es
		pop	ds
		ret

; ---------------------------------------------------------------------------
; Row BX / 2: its name, then the value DX:AX at column 26.
show:		push	dx
		push	ax
		mov	si,[names + bx]
		mov	di,line
.copy:		lodsb
		or	al,al
		jz	.pad
		stosb
		jmp	.copy
.pad:		mov	al,' '
.space:		cmp	di,line + 26
		jae	.number
		stosb
		jmp	.space
.number:	pop	ax
		pop	dx
		push	bx
		mov	cx,7			; seven digits
		add	di,cx
		mov	byte [di],0
		mov	si,10
.digit:		push	ax			; DX:AX / 10
		mov	ax,dx
		xor	dx,dx
		div	si
		mov	bx,ax			; high quotient
		pop	ax
		div	si
		add	dl,'0'
		dec	di
		mov	[di],dl
		mov	dx,bx
		loop	.digit
		pop	bx
		shr	bx,1
		add	bx,2
		mov	si,line
		mov	byte [colour],WHITE
		jmp	text_line

; AX in decimal at DI (5 digits, leading zeros as spaces then removed).
decimal:	mov	cx,5
		add	di,cx
		mov	si,10
.d:		xor	dx,dx
		div	si
		add	dl,'0'
		dec	di
		mov	[di],dl
		loop	.d
		ret

video_init:	mov	ax,1702h
		int	83h
		mov	ah,0
		mov	bh,0a0h
		mov	bl,0ah
		mov	cx,0404h
		xor	dx,dx
		int	8fh
		mov	di,fb_bios
		mov	ax,0100h
		mov	cx,1
		int	8fh
		mov	ax,0b01h
		int	8fh
		mov	ah,16h
		int	84h
		mov	ax,0500h
		xor	bx,bx
		xor	cx,cx
		xor	dx,dx
		int	8fh
		mov	ax,0b01h
		int	8fh
		push	cs
		pop	ds
		push	cs
		pop	es
		mov	dx,0153h
		in	al,dx
		and	al,0f0h
		or	al,4
		out	dx,al
		mov	dx,0580h
		mov	al,10h
		out	dx,al
		mov	dx,0108h
		xor	ax,ax
		out	dx,ax
		add	dx,2
		out	dx,ax
		call	clear_gvram
		cli
		mov	cx,2			; twice: vaeg settles DAC low bits
.palette:	push	cx
		mov	dx,0300h
		mov	si,palette
		mov	cx,32
.entry:		lodsw
		out	dx,ax
		add	dx,2
		loop	.entry
		pop	cx
		loop	.palette
		mov	dx,0100h		; two screens
		in	ax,dx
		or	ax,0800h
		out	dx,ax
		mov	dx,0102h		; both 320 dots, packed 4 bpp
		mov	ax,1111h
		out	dx,ax
		mov	dx,010ch		; palette mode 2: set 1 for G0
		mov	ax,01a0h
		out	dx,ax
		mov	dx,0106h		; G0 in front of G1
		mov	ax,00bah
		out	dx,ax
		mov	dx,0124h		; G0 colour 0 transparent
		mov	ax,1
		out	dx,ax
		mov	dx,0126h		; G1 opaque
		xor	ax,ax
		out	dx,ax
		mov	dx,0200h		; frame buffers
		mov	si,frames4
		mov	cx,32
.fb:		lodsw
		out	dx,ax
		add	dx,2
		loop	.fb
		mov	si,sync224
		mov	al,10h
		call	tsp_cmd
		mov	cx,14
.sync:		lodsb
		call	tsp_prm
		loop	.sync
		sti
		ret

clear_gvram:	push	es
		mov	bx,0a000h
.bank:		mov	es,bx
		xor	di,di
		xor	ax,ax
		mov	cx,8000h
		rep	stosw
		add	bx,1000h
		cmp	bx,0e000h
		jb	.bank
		pop	es
		ret

tsp_cmd:	mov	ah,al
		mov	dx,0142h
.wait:		in	al,dx
		test	al,5
		jnz	.wait
		mov	al,ah
		out	dx,al
		ret
tsp_prm:	mov	ah,al
		mov	dx,0142h
.wait:		in	al,dx
		test	al,1
		jnz	.wait
		mov	al,ah
		mov	dx,0146h
		out	dx,al
		ret

; Text on G1: line BX (8 rows from TEXT_TOP), string SI, colour [colour].
; 6x8-dot cells, 3 bytes each; the line is cleared first.
text_line:	push	es
		mov	ax,GV_G1
		mov	es,ax
		imul	di,bx,8 * LINE_BYTES
		add	di,TEXT_TOP * LINE_BYTES
		push	di
		mov	cx,8 * LINE_BYTES / 2
		xor	ax,ax
		rep	stosw
		pop	di
		mov	al,[colour]
		mov	ah,al
		shl	ah,4
		mov	[ink],ax		; AL low nibble, AH high nibble
.char:		lodsb
		or	al,al
		jz	.done
		cmp	al,'a'
		jb	.upper
		sub	al,20h
.upper:		sub	al,20h
		cmp	al,64
		jb	.glyph
		xor	al,al
.glyph:		xor	ah,ah
		imul	bx,ax,7
		add	bx,FONT
		push	di
		mov	dx,7
.row:		mov	ah,[bx]			; bits 4-0, bit 4 leftmost (stored
		xor	ah,0a5h			; XOR A5h: gen_font.py)
		shl	ah,3
		mov	cx,3
		push	di
.pair:		xor	al,al
		shl	ah,1
		jnc	.left
		or	al,[ink + 1]
.left:		shl	ah,1
		jnc	.right
		or	al,[ink]
.right:		stosb
		loop	.pair
		pop	di
		add	di,LINE_BYTES
		inc	bx
		dec	dx
		jnz	.row
		pop	di
		add	di,3
		jmp	.char
.done:		pop	es
		ret

; ---------------------------------------------------------------------------
N_TESTS		equ	18
tests		dw	t_z_none, t_z_ldahl, t_z_ldhla, t_z_ldahl_lo, t_z_ldhla_lo, t_z_add
		dw	t_z_ldaix, t_z_call, t_z_ldir, t_z_calln, t_brkem
		dw	t_n_none, t_n_read, t_n_write, t_n_read_lo, t_n_add, t_n_call, t_n_movsb
names		dw	s_a, s_b, s_c, s_d, s_e, s_f, s_g, s_h, s_i, s_j, s_k
		dw	s_l, s_m, s_n, s_o, s_p, s_q, s_r
s_banner	db	'Z80TIME: restart the machine to leave.',13,10,'$'
s_24khz		db	'Select 15 kHz (DIP switch SW1 off) before starting.',13,10,'$'
s_memory	db	'Not enough memory (needs 7400h; load below 2000h).',13,10,'$'
s_title		db	'Z80TIME '
s_title_n	db	'00000 VRTC', 0
s_done		db	'DONE: RESTART TO LEAVE', 0
s_a		db	'A Z DJNZ', 0
s_b		db	'B Z LD A,(HL) HI', 0
s_c		db	'C Z LD (HL),A HI', 0
s_d		db	'D Z LD A,(HL) LO', 0
s_e		db	'E Z LD (HL),A LO', 0
s_f		db	'F Z ADD A,B', 0
s_g		db	'G Z LD A,(IX+5) HI', 0
s_h		db	'H Z CALL RET HI', 0
s_i		db	'I Z LDIR 1K HI', 0
s_j		db	'J Z CALLN IRET', 0
s_k		db	'K BRKEM RETEM', 0
s_l		db	'L N DEC JNZ', 0
s_m		db	'M N MOV AL,(BX) HI', 0
s_n		db	'N N MOV (BX),AL HI', 0
s_o		db	'O N MOV AL,(BX) LO', 0
s_p		db	'P N ADD AL,BL', 0
s_q		db	'Q N CALL RET', 0
s_r		db	'R N MOVSB 1K HI', 0

sync224		db	0c1h,57h,1ch,0,9fh,0,10h,0fh,25h,0,0e0h,0,2,4
		align	2
fb_bios		dw	4,320,224,0,0,0
frames4		dw	0,0,160,447,0,0,0,0,0,224,0,0,0,0,0,0
		dw	0,0,160,0,0,0,0,0,0,224,0,0,0,0,0,0
palette		dw	PALETTE32
frames		dw	3000
tick		dw	0
t0		dw	0
count		dd	0
z_sp		dw	0
ink		dw	0
results		times	4 + N_TESTS * 4 db 0
		dw	0, 0
colour		db	0
line		times	40 db 0
		align	2
image_end:
IMAGE_TOP	equ	image_end - $$ + 100h
