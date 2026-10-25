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
; Z80TIME 2 (Z80TIME.COM): the uPD9002's compatible (uPD70008) mode on a
; PC-88VA/VA2 and in vaeg, instruction by instruction (README.md). No data
; of any program but its own.
;
; Each row counts the blocks done in [frames] VRTCs (default 1200, 20 s a
; row; a number after the command name changes it); more is faster. A
; compatible-mode block is entered by BRKEM and left by RETEM: EI, LD HL,
; IX = 0100h, DE = 0200h, SP = 0E00h, XOR A (Z set), LD B,0, then 256
; passes of eight copies of one instruction and DJNZ. One instruction is
; (row - BASE) / 8 a pass. LDIR copies 2048 bytes once a block. BRKEM-
; RETEM is 2048 round trips from a native LOOP. The N rows are 256 passes
; of eight V30 instructions and DEC CX, JNZ. Code at 20000h (or at 70000h,
; "CODE HI"); "HI" data at 72800h (the VA inserts one wait from 40000h),
; "LO" data at 30000h. The VRTC handler counts VRTCs. The counts are also
; stored at GVRAM 30000h (README.md). Never returns: restart the machine
; to leave.

		cpu	186
		org	100h

%include "font.inc"			; FONT, PALETTE32

BENCH_SEG	equ	2000h		; this program, copied
HI_CODE_SEG	equ	7000h		; a second copy (code above 40000h)
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
TEXT_TOP	equ	0
ROWS		equ	25		; a column's rows
COLUMN		equ	26		; characters a column
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
		mov	ax,HI_CODE_SEG		; the two copies
		call	.copy
		mov	ax,BENCH_SEG
		call	.copy
		jmp	BENCH_SEG:main
.copy:		mov	es,ax
		xor	si,si
		xor	di,di
		mov	cx,(IMAGE_TOP + 1) / 2
		rep	movsw
		ret
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
		push	cs
		pop	es
		mov	al,0fbh			; only VRTC (IR2)
		mov	dx,18ah
		out	dx,al
		sti
		mov	di,lines		; the screen's lines: spaces
		mov	cx,ROWS * 2 * COLUMN / 2
		mov	ax,2020h
		rep	stosw
		mov	ax,[frames]		; the title: Z80TIME 2 n VRTC
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
		mov	[results + 8 + si],ax
		mov	[results + 10 + si],dx
		push	bx
		call	show
		pop	bx
		add	bx,2
		cmp	bx,N_TESTS * 2
		jb	.test
		mov	word [results],'Z8'
		mov	word [results + 2],'T2'
		mov	word [results + 4],N_TESTS
		mov	ax,[frames]
		mov	[results + 6],ax
		mov	ax,RESULT_SEG
		mov	es,ax
		xor	di,di
		mov	si,results
		mov	cx,(8 + N_TESTS * 4) / 2
		rep	movsw
		push	cs
		pop	es
		mov	si,s_done
		mov	byte [colour],RED
		mov	bx,ROWS + 2
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
; Compatible mode: a block at SI:AX with DS0 = DX.
%macro Z80_JR 2				; opcode, target
		db	%1, ((%2) - ($ + 2)) & 0ffh
%endmacro
%macro Z80_HEAD 0
		db	0fbh			; EI
		db	21h, 00h, 01h		; LD HL,0100h
		db	0ddh, 21h, 00h, 01h	; LD IX,0100h
		db	11h, 00h, 02h		; LD DE,0200h
		db	31h, 00h, 0eh		; LD SP,0E00h
		db	0afh			; XOR A
		db	06h, 00h		; LD B,0
%%pass:
%xdefine Z80_PASS %%pass
%endmacro
%macro Z80_TAIL 0
		Z80_JR	10h, Z80_PASS		; DJNZ
		db	0edh, 0fdh		; RETEM
%endmacro

z_base:		Z80_HEAD
%rep 8
%endrep
		Z80_TAIL
z_nop:		Z80_HEAD
%rep 8
		db	000h
%endrep
		Z80_TAIL
z_ldab:		Z80_HEAD
%rep 8
		db	078h
%endrep
		Z80_TAIL
z_ldan:		Z80_HEAD
%rep 8
		db	03eh, 005h
%endrep
		Z80_TAIL
z_lddenn:		Z80_HEAD
%rep 8
		db	011h, 000h, 002h
%endrep
		Z80_TAIL
z_inca:		Z80_HEAD
%rep 8
		db	03ch
%endrep
		Z80_TAIL
z_incde:		Z80_HEAD
%rep 8
		db	013h
%endrep
		Z80_TAIL
z_addac:		Z80_HEAD
%rep 8
		db	081h
%endrep
		Z80_TAIL
z_addan:		Z80_HEAD
%rep 8
		db	0c6h, 001h
%endrep
		Z80_TAIL
z_addhlde:		Z80_HEAD
%rep 8
		db	019h
%endrep
		Z80_TAIL
z_andn:		Z80_HEAD
%rep 8
		db	0e6h, 00fh
%endrep
		Z80_TAIL
z_rlca:		Z80_HEAD
%rep 8
		db	007h
%endrep
		Z80_TAIL
z_daa:		Z80_HEAD
%rep 8
		db	027h
%endrep
		Z80_TAIL
z_exdehl:		Z80_HEAD
%rep 8
		db	0ebh
%endrep
		Z80_TAIL
z_exx:		Z80_HEAD
%rep 8
		db	0d9h
%endrep
		Z80_TAIL
z_ldahlh:		Z80_HEAD
%rep 8
		db	07eh
%endrep
		Z80_TAIL
z_ldahll:		Z80_HEAD
%rep 8
		db	07eh
%endrep
		Z80_TAIL
z_ldhlah:		Z80_HEAD
%rep 8
		db	077h
%endrep
		Z80_TAIL
z_ldhlal:		Z80_HEAD
%rep 8
		db	077h
%endrep
		Z80_TAIL
z_ldade:		Z80_HEAD
%rep 8
		db	01ah
%endrep
		Z80_TAIL
z_inchl:		Z80_HEAD
%rep 8
		db	034h
%endrep
		Z80_TAIL
z_ldann:		Z80_HEAD
%rep 8
		db	03ah, 000h, 003h
%endrep
		Z80_TAIL
z_ldnna:		Z80_HEAD
%rep 8
		db	032h, 000h, 003h
%endrep
		Z80_TAIL
z_ldhlnn:		Z80_HEAD
%rep 8
		db	02ah, 000h, 003h
%endrep
		Z80_TAIL
z_ldnnhl:		Z80_HEAD
%rep 8
		db	022h, 000h, 003h
%endrep
		Z80_TAIL
z_pushpop:		Z80_HEAD
%rep 8
		db	0d5h, 0d1h
%endrep
		Z80_TAIL
z_callret:		Z80_HEAD
%rep 8
		db	0cdh
		dw	z_ret
%endrep
		Z80_TAIL
z_jr:		Z80_HEAD
%rep 8
		db	018h, 000h
%endrep
		Z80_TAIL
z_jrnz:		Z80_HEAD
%rep 8
		db	020h, 000h
%endrep
		Z80_TAIL
z_jp:		Z80_HEAD
%rep 8
		db	0c3h
		dw	$ + 2
%endrep
		Z80_TAIL
z_jpnz:		Z80_HEAD
%rep 8
		db	0c2h
		dw	$ + 2
%endrep
		Z80_TAIL
z_retnz:		Z80_HEAD
%rep 8
		db	0c0h
%endrep
		Z80_TAIL
z_ldaix:		Z80_HEAD
%rep 8
		db	0ddh, 07eh, 005h
%endrep
		Z80_TAIL
z_ldixa:		Z80_HEAD
%rep 8
		db	0ddh, 077h, 005h
%endrep
		Z80_TAIL
z_incix:		Z80_HEAD
%rep 8
		db	0ddh, 034h, 005h
%endrep
		Z80_TAIL
z_addixde:		Z80_HEAD
%rep 8
		db	0ddh, 019h
%endrep
		Z80_TAIL
z_ldixnn:		Z80_HEAD
%rep 8
		db	0ddh, 021h, 000h, 001h
%endrep
		Z80_TAIL
z_bit0a:		Z80_HEAD
%rep 8
		db	0cbh, 047h
%endrep
		Z80_TAIL
z_srla:		Z80_HEAD
%rep 8
		db	0cbh, 03fh
%endrep
		Z80_TAIL
z_set0hl:		Z80_HEAD
%rep 8
		db	0cbh, 0c6h
%endrep
		Z80_TAIL
z_rlix:		Z80_HEAD
%rep 8
		db	0ddh, 0cbh, 005h, 016h
%endrep
		Z80_TAIL
z_neg:		Z80_HEAD
%rep 8
		db	0edh, 044h
%endrep
		Z80_TAIL
z_calln:		Z80_HEAD
%rep 8
		db	0edh, 0edh, 075h
%endrep
		Z80_TAIL
z_nopch:		Z80_HEAD
%rep 8
		db	000h
%endrep
		Z80_TAIL
z_ldahlch:		Z80_HEAD
%rep 8
		db	07eh
%endrep
		Z80_TAIL
z_ret:		db	0c9h			; RET
z_ldir:		db	0fbh			; EI
		db	21h, 00h, 01h		; LD HL,0100h
		db	11h, 00h, 09h		; LD DE,0900h
		db	01h, 00h, 08h		; LD BC,2048
		db	0edh, 0b0h		; LDIR
		db	0edh, 0fdh		; RETEM
z_retem:	db	0edh, 0fdh		; RETEM

z_enter:	push	ds
		push	es
		push	bp
		xor	bx,bx
		mov	es,bx
		mov	[es:VEC_BRKEM * 4],ax
		mov	[es:VEC_BRKEM * 4 + 2],si
		mov	[cs:z_sp],sp
		mov	ds,dx
		BRKEM	VEC_BRKEM		; (RETEM returns here)
		mov	sp,[cs:z_sp]
		pop	bp
		pop	es
		pop	ds
		ret

t_brkem:	push	ds			; 2048 BRKEM-RETEM round trips
		push	es
		push	bp
		xor	bx,bx
		mov	es,bx
		mov	word [es:VEC_BRKEM * 4],z_retem
		mov	word [es:VEC_BRKEM * 4 + 2],BENCH_SEG
		mov	[cs:z_sp],sp
		mov	bx,HI_SEG
		mov	ds,bx
		mov	cx,2048
.l:		BRKEM	VEC_BRKEM
		loop	.l
		mov	sp,[cs:z_sp]
		pop	bp
		pop	es
		pop	ds
		ret

; The rows' entries.
t_base:		mov	ax,z_base
		mov	dx,HI_SEG
		mov	si,BENCH_SEG
		jmp	z_enter
t_nop:		mov	ax,z_nop
		mov	dx,HI_SEG
		mov	si,BENCH_SEG
		jmp	z_enter
t_ldab:		mov	ax,z_ldab
		mov	dx,HI_SEG
		mov	si,BENCH_SEG
		jmp	z_enter
t_ldan:		mov	ax,z_ldan
		mov	dx,HI_SEG
		mov	si,BENCH_SEG
		jmp	z_enter
t_lddenn:		mov	ax,z_lddenn
		mov	dx,HI_SEG
		mov	si,BENCH_SEG
		jmp	z_enter
t_inca:		mov	ax,z_inca
		mov	dx,HI_SEG
		mov	si,BENCH_SEG
		jmp	z_enter
t_incde:		mov	ax,z_incde
		mov	dx,HI_SEG
		mov	si,BENCH_SEG
		jmp	z_enter
t_addac:		mov	ax,z_addac
		mov	dx,HI_SEG
		mov	si,BENCH_SEG
		jmp	z_enter
t_addan:		mov	ax,z_addan
		mov	dx,HI_SEG
		mov	si,BENCH_SEG
		jmp	z_enter
t_addhlde:		mov	ax,z_addhlde
		mov	dx,HI_SEG
		mov	si,BENCH_SEG
		jmp	z_enter
t_andn:		mov	ax,z_andn
		mov	dx,HI_SEG
		mov	si,BENCH_SEG
		jmp	z_enter
t_rlca:		mov	ax,z_rlca
		mov	dx,HI_SEG
		mov	si,BENCH_SEG
		jmp	z_enter
t_daa:		mov	ax,z_daa
		mov	dx,HI_SEG
		mov	si,BENCH_SEG
		jmp	z_enter
t_exdehl:		mov	ax,z_exdehl
		mov	dx,HI_SEG
		mov	si,BENCH_SEG
		jmp	z_enter
t_exx:		mov	ax,z_exx
		mov	dx,HI_SEG
		mov	si,BENCH_SEG
		jmp	z_enter
t_ldahlh:		mov	ax,z_ldahlh
		mov	dx,HI_SEG
		mov	si,BENCH_SEG
		jmp	z_enter
t_ldahll:		mov	ax,z_ldahll
		mov	dx,LO_SEG
		mov	si,BENCH_SEG
		jmp	z_enter
t_ldhlah:		mov	ax,z_ldhlah
		mov	dx,HI_SEG
		mov	si,BENCH_SEG
		jmp	z_enter
t_ldhlal:		mov	ax,z_ldhlal
		mov	dx,LO_SEG
		mov	si,BENCH_SEG
		jmp	z_enter
t_ldade:		mov	ax,z_ldade
		mov	dx,HI_SEG
		mov	si,BENCH_SEG
		jmp	z_enter
t_inchl:		mov	ax,z_inchl
		mov	dx,HI_SEG
		mov	si,BENCH_SEG
		jmp	z_enter
t_ldann:		mov	ax,z_ldann
		mov	dx,HI_SEG
		mov	si,BENCH_SEG
		jmp	z_enter
t_ldnna:		mov	ax,z_ldnna
		mov	dx,HI_SEG
		mov	si,BENCH_SEG
		jmp	z_enter
t_ldhlnn:		mov	ax,z_ldhlnn
		mov	dx,HI_SEG
		mov	si,BENCH_SEG
		jmp	z_enter
t_ldnnhl:		mov	ax,z_ldnnhl
		mov	dx,HI_SEG
		mov	si,BENCH_SEG
		jmp	z_enter
t_pushpop:		mov	ax,z_pushpop
		mov	dx,HI_SEG
		mov	si,BENCH_SEG
		jmp	z_enter
t_callret:		mov	ax,z_callret
		mov	dx,HI_SEG
		mov	si,BENCH_SEG
		jmp	z_enter
t_jr:		mov	ax,z_jr
		mov	dx,HI_SEG
		mov	si,BENCH_SEG
		jmp	z_enter
t_jrnz:		mov	ax,z_jrnz
		mov	dx,HI_SEG
		mov	si,BENCH_SEG
		jmp	z_enter
t_jp:		mov	ax,z_jp
		mov	dx,HI_SEG
		mov	si,BENCH_SEG
		jmp	z_enter
t_jpnz:		mov	ax,z_jpnz
		mov	dx,HI_SEG
		mov	si,BENCH_SEG
		jmp	z_enter
t_retnz:		mov	ax,z_retnz
		mov	dx,HI_SEG
		mov	si,BENCH_SEG
		jmp	z_enter
t_ldaix:		mov	ax,z_ldaix
		mov	dx,HI_SEG
		mov	si,BENCH_SEG
		jmp	z_enter
t_ldixa:		mov	ax,z_ldixa
		mov	dx,HI_SEG
		mov	si,BENCH_SEG
		jmp	z_enter
t_incix:		mov	ax,z_incix
		mov	dx,HI_SEG
		mov	si,BENCH_SEG
		jmp	z_enter
t_addixde:		mov	ax,z_addixde
		mov	dx,HI_SEG
		mov	si,BENCH_SEG
		jmp	z_enter
t_ldixnn:		mov	ax,z_ldixnn
		mov	dx,HI_SEG
		mov	si,BENCH_SEG
		jmp	z_enter
t_bit0a:		mov	ax,z_bit0a
		mov	dx,HI_SEG
		mov	si,BENCH_SEG
		jmp	z_enter
t_srla:		mov	ax,z_srla
		mov	dx,HI_SEG
		mov	si,BENCH_SEG
		jmp	z_enter
t_set0hl:		mov	ax,z_set0hl
		mov	dx,HI_SEG
		mov	si,BENCH_SEG
		jmp	z_enter
t_rlix:		mov	ax,z_rlix
		mov	dx,HI_SEG
		mov	si,BENCH_SEG
		jmp	z_enter
t_neg:		mov	ax,z_neg
		mov	dx,HI_SEG
		mov	si,BENCH_SEG
		jmp	z_enter
t_ldir:		mov	ax,z_ldir
		mov	dx,HI_SEG
		mov	si,BENCH_SEG
		jmp	z_enter
t_calln:		mov	ax,z_calln
		mov	dx,HI_SEG
		mov	si,BENCH_SEG
		jmp	z_enter
t_nopch:		mov	ax,z_nopch
		mov	dx,HI_SEG
		mov	si,HI_CODE_SEG
		jmp	z_enter
t_ldahlch:		mov	ax,z_ldahlch
		mov	dx,HI_SEG
		mov	si,HI_CODE_SEG
		jmp	z_enter
t_nbase:		push	ds
		mov	bx,HI_SEG
		mov	ds,bx
		mov	bx,0100h
		mov	cx,256
.l:
		dec	cx
		jnz	.l
		pop	ds
		ret
t_nnop:		push	ds
		mov	bx,HI_SEG
		mov	ds,bx
		mov	bx,0100h
		mov	cx,256
.l:
%rep 8
		nop
%endrep
		dec	cx
		jnz	.l
		pop	ds
		ret
t_nmov:		push	ds
		mov	bx,HI_SEG
		mov	ds,bx
		mov	bx,0100h
		mov	cx,256
.l:
%rep 8
		mov al,[bx]
%endrep
		dec	cx
		jnz	.l
		pop	ds
		ret

; ---------------------------------------------------------------------------
; Row BX / 2: its name and the value DX:AX, in column (BX / 2) / ROWS of
; screen line (BX / 2) mod ROWS + 1, then that line drawn.
show:		push	dx
		push	ax
		mov	ax,bx
		shr	ax,1
		mov	cl,ROWS
		div	cl			; AL: column, AH: line
		mov	[s_col],al
		mov	[s_line],ah
		mov	dl,ah
		xor	dh,dh
		imul	di,dx,2 * COLUMN
		add	di,lines
		mov	al,[s_col]
		mov	cl,COLUMN
		mul	cl
		add	di,ax			; DI: the line's column
		mov	si,[names + bx]
		mov	cx,COLUMN - 8
.copy:		lodsb
		or	al,al
		jz	.number
		stosb
		loop	.copy
.number:	add	di,cx			; (the rest is spaces)
		pop	ax
		pop	dx
		push	bx
		mov	cx,7			; seven digits
		add	di,cx
		mov	si,10
.digit:		push	ax			; DX:AX / 10
		mov	ax,dx
		xor	dx,dx
		div	si
		mov	bx,ax			; the high quotient
		pop	ax
		div	si
		add	dl,'0'
		dec	di
		mov	[di],dl
		mov	dx,bx
		loop	.digit
		mov	al,[s_line]		; the line to the screen
		xor	ah,ah
		imul	si,ax,2 * COLUMN
		add	si,lines
		mov	di,linebuf
		mov	cx,2 * COLUMN
		rep	movsb
		mov	byte [di],0
		mov	bl,[s_line]
		xor	bh,bh
		inc	bx
		mov	si,linebuf
		mov	byte [colour],WHITE
		call	text_line
		pop	bx
		ret

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
N_TESTS		equ	50
tests:
		dw	t_base
		dw	t_nop
		dw	t_ldab
		dw	t_ldan
		dw	t_lddenn
		dw	t_inca
		dw	t_incde
		dw	t_addac
		dw	t_addan
		dw	t_addhlde
		dw	t_andn
		dw	t_rlca
		dw	t_daa
		dw	t_exdehl
		dw	t_exx
		dw	t_ldahlh
		dw	t_ldahll
		dw	t_ldhlah
		dw	t_ldhlal
		dw	t_ldade
		dw	t_inchl
		dw	t_ldann
		dw	t_ldnna
		dw	t_ldhlnn
		dw	t_ldnnhl
		dw	t_pushpop
		dw	t_callret
		dw	t_jr
		dw	t_jrnz
		dw	t_jp
		dw	t_jpnz
		dw	t_retnz
		dw	t_ldaix
		dw	t_ldixa
		dw	t_incix
		dw	t_addixde
		dw	t_ldixnn
		dw	t_bit0a
		dw	t_srla
		dw	t_set0hl
		dw	t_rlix
		dw	t_neg
		dw	t_ldir
		dw	t_calln
		dw	t_brkem
		dw	t_nopch
		dw	t_ldahlch
		dw	t_nbase
		dw	t_nnop
		dw	t_nmov
names:
		dw	s_base
		dw	s_nop
		dw	s_ldab
		dw	s_ldan
		dw	s_lddenn
		dw	s_inca
		dw	s_incde
		dw	s_addac
		dw	s_addan
		dw	s_addhlde
		dw	s_andn
		dw	s_rlca
		dw	s_daa
		dw	s_exdehl
		dw	s_exx
		dw	s_ldahlh
		dw	s_ldahll
		dw	s_ldhlah
		dw	s_ldhlal
		dw	s_ldade
		dw	s_inchl
		dw	s_ldann
		dw	s_ldnna
		dw	s_ldhlnn
		dw	s_ldnnhl
		dw	s_pushpop
		dw	s_callret
		dw	s_jr
		dw	s_jrnz
		dw	s_jp
		dw	s_jpnz
		dw	s_retnz
		dw	s_ldaix
		dw	s_ldixa
		dw	s_incix
		dw	s_addixde
		dw	s_ldixnn
		dw	s_bit0a
		dw	s_srla
		dw	s_set0hl
		dw	s_rlix
		dw	s_neg
		dw	s_ldir
		dw	s_calln
		dw	s_brkem
		dw	s_nopch
		dw	s_ldahlch
		dw	s_nbase
		dw	s_nnop
		dw	s_nmov
s_base		db	'BASE DJNZ', 0
s_nop		db	'NOP', 0
s_ldab		db	'LD A,B', 0
s_ldan		db	'LD A,N', 0
s_lddenn		db	'LD DE,NN', 0
s_inca		db	'INC A', 0
s_incde		db	'INC DE', 0
s_addac		db	'ADD A,C', 0
s_addan		db	'ADD A,N', 0
s_addhlde		db	'ADD HL,DE', 0
s_andn		db	'AND N', 0
s_rlca		db	'RLCA', 0
s_daa		db	'DAA', 0
s_exdehl		db	'EX DE,HL', 0
s_exx		db	'EXX', 0
s_ldahlh		db	'LD A,(HL) HI', 0
s_ldahll		db	'LD A,(HL) LO', 0
s_ldhlah		db	'LD (HL),A HI', 0
s_ldhlal		db	'LD (HL),A LO', 0
s_ldade		db	'LD A,(DE) HI', 0
s_inchl		db	'INC (HL) HI', 0
s_ldann		db	'LD A,(NN) HI', 0
s_ldnna		db	'LD (NN),A HI', 0
s_ldhlnn		db	'LD HL,(NN) HI', 0
s_ldnnhl		db	'LD (NN),HL HI', 0
s_pushpop		db	'PUSH+POP HI', 0
s_callret		db	'CALL+RET HI', 0
s_jr		db	'JR TAKEN', 0
s_jrnz		db	'JR NZ NOT', 0
s_jp		db	'JP NN', 0
s_jpnz		db	'JP NZ NOT', 0
s_retnz		db	'RET NZ NOT', 0
s_ldaix		db	'LD A,(IX+D) HI', 0
s_ldixa		db	'LD (IX+D),A HI', 0
s_incix		db	'INC (IX+D) HI', 0
s_addixde		db	'ADD IX,DE', 0
s_ldixnn		db	'LD IX,NN', 0
s_bit0a		db	'BIT 0,A', 0
s_srla		db	'SRL A', 0
s_set0hl		db	'SET 0,(HL) HI', 0
s_rlix		db	'RL (IX+D) HI', 0
s_neg		db	'NEG', 0
s_ldir		db	'LDIR BYTE HI', 0
s_calln		db	'CALLN+IRET', 0
s_brkem		db	'BRKEM+RETEM', 0
s_nopch		db	'NOP CODE HI', 0
s_ldahlch		db	'LD A,(HL) CODE HI', 0
s_nbase		db	'N BASE DEC JNZ', 0
s_nnop		db	'N NOP', 0
s_nmov		db	'N MOV AL,(BX) HI', 0
s_banner	db	'Z80TIME 2: restart the machine to leave.',13,10,'$'
s_24khz		db	'Select 15 kHz (DIP switch SW1 off) before starting.',13,10,'$'
s_memory	db	'Not enough memory (needs 7400h; load below 2000h).',13,10,'$'
s_title		db	'Z80TIME 2  '
s_title_n	db	'00000 VRTC', 0
s_done		db	'DONE: RESTART TO LEAVE', 0

sync224		db	0c1h,57h,1ch,0,9fh,0,10h,0fh,25h,0,0e0h,0,2,4
		align	2
fb_bios		dw	4,320,224,0,0,0
frames4		dw	0,0,160,447,0,0,0,0,0,224,0,0,0,0,0,0
		dw	0,0,160,0,0,0,0,0,0,224,0,0,0,0,0,0
palette		dw	PALETTE32
frames		dw	1200
tick		dw	0
t0		dw	0
count		dd	0
z_sp		dw	0
ink		dw	0
results		times	8 + N_TESTS * 4 db 0
colour		db	0
s_col		db	0
s_line		db	0
lines		times	ROWS * 2 * COLUMN db 0
linebuf		times	2 * COLUMN + 1 db 0
		align	2
image_end:
IMAGE_TOP	equ	image_end - $$ + 100h
%if IMAGE_TOP > 2800h
%error the image reaches HI_SEG in its second copy
%endif
