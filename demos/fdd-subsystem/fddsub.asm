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
; FDDSUB.COM: the FDD subsystem (a uPD780C at 4 MHz with its own RAM,
; joined to the main CPU by a pair of 8255s) as a coprocessor, on a
; PC-88VA/VA2 and vaeg (README.md): transfer rates through the subsystem
; monitor's commands and through a program of our own run in its RAM, a
; round trip, a frame's exchange (64 bytes out, 32 back) and the
; subsystem CPU's own speed. Synthetic data only; the subsystem program is
; gen_data.py's. Every row is measured [passes] times (8, or FDDSUB P),
; with mean, minimum, maximum and standard deviation. Never returns:
; restart the machine to leave.
;
; Byte transport (both ends 8255 mode 91h; vaeg docs/bios/fdc_monitor.md):
; the main CPU writes data at FDh, reads at FCh; FEh's lower bits are the
; subsystem's DAV (1), RFD (2), DAC (4); FFh sets or clears this side's DAV
; (bit 4), RFD (5), DAC (6), ATN (7). Monitor commands (ATN first): 0Ch
; store bytes, 0Bh return bytes, 0Dh execute; parameters high byte first.
; The subsystem program is stored at 4000h (RAM 4000h-7EFFh is free).

		cpu	186
		org	100h

%include "data.inc"			; FONT, FDC_PROGRAM, PALETTE32

BENCH_SEG	equ	2000h		; this program, copied
MEMORY_TOP	equ	3000h
FRAMES		equ	300		; VRTCs a measurement (5 s)
LINE_BYTES	equ	160
LINES		equ	224
GV_G0		equ	0a000h
GV_G1		equ	0c000h
WHITE		equ	1
RED		equ	2
YELLOW		equ	3
SLOW_MAX	equ	60		; 65536-poll rounds before a timeout
BUF		equ	0c000h		; 256 bytes to send, then received
N_TESTS		equ	10
st_count	equ	0cc00h		; the rows' statistics
st_first	equ	st_count + 2 * N_TESTS
st_sum		equ	st_first + 2 * N_TESTS
st_min		equ	st_sum + 2 * N_TESTS
st_max		equ	st_min + 2 * N_TESTS
st_sq		equ	st_max + 2 * N_TESTS
ST_WORDS	equ	7 * N_TESTS

start:		cld
		mov	si,81h			; FDDSUB [P]: passes (1-99)
		xor	bx,bx
.arg:		lodsb
		cmp	al,' '
		je	.arg
.digit:		sub	al,'0'
		cmp	al,9
		ja	.argend
		imul	bx,bx,10
		xor	ah,ah
		add	bx,ax
		lodsb
		jmp	.digit
.argend:	or	bx,bx
		jz	.banner
		cmp	bx,99
		jbe	.p
		mov	bx,99
.p:		mov	[passes],bx
.banner:	mov	dx,s_banner
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
		push	cs
		pop	es
		mov	al,0fbh			; only VRTC (IR2)
		mov	dx,18ah
		out	dx,al
		sti
		mov	di,BUF			; the data sent: 0, 1, 2, ...
		xor	ax,ax
		mov	cx,256
.fill:		stosb
		inc	al
		loop	.fill
		mov	di,st_count
		xor	ax,ax
		mov	cx,ST_WORDS
		rep	stosw
		mov	si,s_title
		mov	byte [colour],RED
		xor	bx,bx
		call	text_line
		mov	si,s_head
		mov	byte [colour],YELLOW
		mov	bx,1
		call	text_line
		xor	bx,bx			; the rows' names
.name:		push	bx
		call	show_row
		pop	bx
		add	bx,2
		cmp	bx,N_TESTS * 2
		jb	.name
		mov	si,s_notes
		mov	byte [colour],YELLOW
		mov	bx,14
		call	text_line
		mov	si,s_notes2
		mov	bx,15
		call	text_line
		mov	si,s_notes3
		mov	bx,16
		call	text_line
		mov	word [pass],0
.pass:		inc	word [pass]
		call	show_pass
		xor	bx,bx
.test:		cmp	bx,UPLOAD_ROW * 2	; before the program's rows: store
		jne	.measure		; and start it
		push	bx
		call	fdc_upload
		pop	bx
.measure:	push	bx
		call	measure
		pop	bx
		push	bx
		call	add_value
		call	show_row
		pop	bx
		add	bx,2
		cmp	bx,N_TESTS * 2
		jb	.test
		mov	al,5			; back to the monitor
		call	fdc_send
		mov	ax,[pass]
		cmp	ax,[passes]
		jb	.pass
		mov	si,s_done
		mov	byte [colour],RED
		xor	bx,bx
		call	text_line
.halt:		hlt
		jmp	.halt

vrtc:		push	ax
		push	dx
		inc	word [cs:tick]
		mov	al,20h
		mov	dx,188h
		out	dx,al
		pop	dx
		pop	ax
		iret

; ---------------------------------------------------------------------------
; Video (as demos/sgp-contention): the BIOS sets up the single-plane
; 320x224 screen; then two packed 4-bpp screens, G0 (palette set 1, colour 0
; transparent) in front of G1 (set 0), all GVRAM cleared.
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
		call	map_gvram
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
		call	send_sync
		sti
		ret

; SYNC: the 224-line vector.
send_sync:	mov	si,sync224
		mov	al,10h
		call	tsp_cmd
		mov	cx,14
.sync:		lodsb
		call	tsp_prm
		loop	.sync
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

; A0000h-AFFFFh: GVRAM.
map_gvram:	push	ax
		push	dx
		mov	dx,0153h
		in	al,dx
		and	al,0f0h
		or	al,04h
		out	dx,al
		pop	dx
		pop	ax
		sti
		ret


; ---------------------------------------------------------------------------
; The byte transport (main side). Waits poll FEh; SLOW_MAX rounds of 65536
; polls without a change stop the program with a message (the subsystem
; does not answer).
%macro FWAIT 2				; FEh mask, z (until set) or nz (clear)
		xor	cx,cx
%%l:		in	al,0feh
		test	al,%1
		j%2	%%ok
		loop	%%l
		call	fdc_slow
		jmp	%%l
%%ok:
%endmacro

; Send AL.
fdc_send:	push	cx
		mov	ah,al
		mov	byte [slow],0
		FWAIT	2, nz			; its RFD
		mov	al,ah
		out	0fdh,al
		mov	al,09h			; DAV
		out	0ffh,al
		FWAIT	4, nz			; its DAC
		mov	al,08h
		out	0ffh,al
		FWAIT	4, z			; its DAC cleared
		pop	cx
		ret

; AL = the next byte.
fdc_recv:	push	cx
		mov	byte [slow],0
		mov	al,0bh			; RFD
		out	0ffh,al
		FWAIT	1, nz			; its DAV
		mov	al,0ah
		out	0ffh,al
		in	al,0fch
		mov	ah,al
		mov	al,0dh			; DAC
		out	0ffh,al
		FWAIT	1, z			; its DAV cleared
		mov	al,0ch
		out	0ffh,al
		mov	al,ah
		pop	cx
		ret

; A monitor command AL: ATN until its RFD, then the byte.
fdc_command:	push	cx
		push	ax
		mov	byte [slow],0
		mov	al,0fh			; ATN
		out	0ffh,al
		FWAIT	2, nz
		mov	al,0eh
		out	0ffh,al
		pop	ax
		pop	cx
		jmp	fdc_send

; Send the word AX high byte first.
fdc_send_hl:	push	ax
		mov	al,ah
		call	fdc_send
		pop	ax
		jmp	fdc_send

fdc_slow:	inc	byte [slow]
		cmp	byte [slow],SLOW_MAX
		jae	.dead
		ret
.dead:		mov	si,s_timeout
		mov	byte [colour],RED
		mov	bx,17
		call	text_line
.halt:		hlt
		jmp	.halt

; The subsystem program to FDC_ORG (command 0Ch), then started (0Dh).
fdc_upload:	mov	al,0ch
		call	fdc_command
		mov	ax,FDC_ORG
		call	fdc_send_hl
		mov	ax,FDC_BYTES
		call	fdc_send_hl
		mov	si,FDC_PROGRAM
		mov	cx,FDC_BYTES
.b:		lodsb
		call	fdc_send
		loop	.b
		mov	al,0dh
		call	fdc_command
		mov	ax,FDC_ORG
		jmp	fdc_send_hl

; ---------------------------------------------------------------------------
; The rows. A block: 256 bytes, a round trip or a frame's exchange; the
; value is the blocks done in FRAMES VRTCs (more is faster) but row I's.

t_cpu:		mov	cx,1024			; ADD, DEC, JNZ (the CPU's clock)
.l:		add	ax,bx
		dec	cx
		jnz	.l
		ret
t_mon_write:	mov	al,0ch			; monitor: 256 bytes to 6000h
		call	fdc_command
		mov	ax,6000h
		call	fdc_send_hl
		mov	ax,256
		call	fdc_send_hl
		mov	si,BUF
		mov	cx,256
.b:		lodsb
		call	fdc_send
		loop	.b
		ret
t_mon_read:	mov	al,0bh			; monitor: 256 bytes from 6000h
		call	fdc_command
		mov	ax,6000h
		call	fdc_send_hl
		mov	ax,256
		call	fdc_send_hl
		mov	di,BUF + 256
		mov	cx,256
.b:		call	fdc_recv
		stosb
		loop	.b
		ret
t_write:	mov	al,1			; ours: 256 bytes, an answer
		call	fdc_send
		xor	al,al
		call	fdc_send
		mov	al,1
		call	fdc_send
		mov	si,BUF
		mov	cx,256
.b:		lodsb
		call	fdc_send
		loop	.b
		call	fdc_recv
		cmp	al,0aah
		jne	fdc_bad
		ret
t_read:		mov	al,2			; ours: 256 bytes back
		call	fdc_send
		xor	al,al
		call	fdc_send
		mov	al,1
		call	fdc_send
		mov	di,BUF + 256
		mov	cx,256
.b:		call	fdc_recv
		stosb
		loop	.b
		ret
t_ping:		mov	al,3			; a round trip
		call	fdc_send
		call	fdc_recv
		cmp	al,55h
		jne	fdc_bad
		ret
t_frame:	mov	al,6			; 64 bytes out, 32 back
		call	fdc_send
		mov	si,BUF
		mov	cx,64
.o:		lodsb
		call	fdc_send
		loop	.o
		mov	cx,32
.i:		call	fdc_recv
		loop	.i
		ret
; Row H: the subsystem counts 256-step DJNZ loops for FRAMES VRTCs (the
; value: loops; at 4 MHz a loop is 256 x 13 + 37 states).
t_loops:	mov	al,4
		call	fdc_send
		mov	ax,[tick]
		mov	[t0],ax
.w:		mov	ax,[tick]
		sub	ax,[t0]
		cmp	ax,FRAMES
		jb	.w
		mov	al,0fh			; ATN: stop
		out	0ffh,al
		call	fdc_recv
		mov	bl,al
		call	fdc_recv
		mov	bh,al
		mov	al,0eh
		out	0ffh,al
		mov	ax,bx
		ret
; Row I: the received bytes of row E (256 from 5000h, as row D stored
; them): mismatches.
t_verify:	mov	si,BUF
		mov	di,BUF + 256
		mov	cx,256
		xor	ax,ax
.c:		cmpsb
		je	.n
		inc	ax
.n:		loop	.c
		ret

fdc_bad:	mov	si,s_bad
		mov	byte [colour],RED
		mov	bx,17
		call	text_line
.halt:		hlt
		jmp	.halt

; Row BX's value in AX.
measure:	test	byte [test_flags + bx],F_ONCE
		jz	.run
		jmp	[tests + bx]
.run:		push	bx
		mov	bx,[tests + bx]
		call	run
		pop	bx
		ret
F_ONCE		equ	1

; AX = blocks of the test at BX in FRAMES VRTCs.
run:		mov	ax,[tick]
.edge:		cmp	ax,[tick]
		je	.edge
		mov	ax,[tick]
		mov	[t0],ax
		mov	word [count],0
.block:		push	bx
		call	bx
		pop	bx
		inc	word [count]
		mov	ax,[tick]
		sub	ax,[t0]
		cmp	ax,FRAMES
		jb	.block
		mov	ax,[count]
		ret

; Row BX's statistics with value AX: deviations from the first value.
add_value:	cmp	word [st_count + bx],0
		jne	.more
		mov	[st_first + bx],ax
		mov	[st_min + bx],ax
		mov	[st_max + bx],ax
.more:		inc	word [st_count + bx]
		cmp	ax,[st_min + bx]
		jae	.mn
		mov	[st_min + bx],ax
.mn:		cmp	ax,[st_max + bx]
		jbe	.mx
		mov	[st_max + bx],ax
.mx:		sub	ax,[st_first + bx]	; d (signed)
		add	[st_sum + bx],ax
		imul	ax			; d * d
		push	bx
		shl	bx,1
		add	[st_sq + bx],ax
		adc	[st_sq + bx + 2],dx
		pop	bx
		ret

; Row BX's line: name, mean, minimum, maximum, SD (tenths).
show_row:	mov	di,line
		mov	si,[test_names + bx]
		call	put_str
		mov	cx,[st_count + bx]
		jcxz	.out
		mov	ax,[st_sum + bx]	; mean: first + sum / n
		cwd
		idiv	cx
		add	ax,[st_first + bx]
		call	put_dec6
		mov	ax,[st_min + bx]
		call	put_dec6
		mov	ax,[st_max + bx]
		call	put_dec6
		call	row_sd			; AX: SD x 10, CF overflow
		jc	.over
		mov	cx,10
		xor	dx,dx
		div	cx
		push	dx
		mov	byte [di],' '
		inc	di
		push	ax			; the integer part in four columns
		mov	al,' '
		stosb
		stosb
		stosb
		pop	ax
		push	di
		mov	cx,10
.sdd:		xor	dx,dx
		div	cx
		add	dl,'0'
		dec	di
		mov	[di],dl
		or	ax,ax
		jnz	.sdd
		pop	di
		mov	al,'.'
		stosb
		pop	ax
		add	al,'0'
		stosb
		jmp	.out
.over:		mov	si,s_over
		call	put_str
.out:		mov	byte [colour],WHITE
		shr	bx,1
		add	bx,2
		jmp	put_line

; AX in six columns, right-aligned.
put_dec6:	push	cx
		push	dx
		push	si
		push	ax
		mov	al,' '
		mov	cx,6
		rep	stosb
		pop	ax
		push	di
		mov	si,10
.d:		xor	dx,dx
		div	si
		add	dl,'0'
		dec	di
		mov	[di],dl
		or	ax,ax
		jnz	.d
		pop	di
		pop	si
		pop	dx
		pop	cx
		ret

; AX = 10 x the sample SD of row BX: sqrt(100 (n sum d^2 - (sum d)^2) /
; (n (n - 1))); 0 for one value; CF set if too large.
row_sd:		push	bx
		mov	cx,[st_count + bx]
		xor	ax,ax
		cmp	cx,2
		jb	.ok
		mov	si,bx
		shl	si,1
		mov	ax,[st_sq + si]		; n sum d^2: 32 x 16 bits
		mul	cx
		mov	[t32],ax
		mov	[t32 + 2],dx
		mov	ax,[st_sq + si + 2]
		mul	cx
		or	dx,dx
		jnz	.over
		add	[t32 + 2],ax
		jc	.over
		mov	ax,[st_sum + bx]	; - (sum d)^2
		imul	ax
		sub	[t32],ax
		sbb	[t32 + 2],dx
		cmp	word [t32 + 2],0200h	; 100 x that must fit 32 bits
		jae	.over
		mov	ax,[t32 + 2]		; x 100
		mov	dx,100
		mul	dx
		mov	si,ax
		mov	ax,[t32]
		mov	dx,100
		mul	dx
		add	dx,si
		mov	si,cx			; / n (n - 1), 32-bit quotient
		dec	si
		push	ax
		mov	ax,cx
		mul	si
		mov	si,ax
		pop	ax
		push	ax
		mov	ax,dx
		xor	dx,dx
		div	si
		mov	cx,ax			; the high word
		pop	ax
		div	si
		mov	dx,cx
		call	isqrt32
.ok:		clc
		pop	bx
		ret
.over:		stc
		pop	bx
		ret

; AX = floor(sqrt(DX:AX)) (DX:AX below 2^31): Newton's method from above.
isqrt32:	mov	[t32],ax
		mov	[t32 + 2],dx
		or	ax,dx
		jz	.r
		mov	bx,0ffffh
.step:		mov	ax,[t32]
		mov	dx,[t32 + 2]
		div	bx			; n / x (below 46341 from above)
		add	ax,bx
		rcr	ax,1			; (x + n / x) / 2
		cmp	ax,bx
		jae	.done
		mov	bx,ax
		jmp	.step
.done:		mov	ax,bx
.r:		ret

; The title line: the pass.
show_pass:	mov	di,line
		mov	si,s_title
		call	put_str
		mov	si,s_pass
		call	put_str
		mov	ax,[pass]
		call	put_dec
		mov	si,s_of
		call	put_str
		mov	ax,[passes]
		call	put_dec
		mov	byte [colour],RED
		xor	bx,bx
		jmp	put_line

; Text on G0 in 6x8-dot cells (3 bytes), 53 a line: text line BX (lines
; 8 BX to 8 BX + 7, cleared first), string SI, colour [colour].
text_line:	push	es
		mov	ax,GV_G0
		mov	es,ax
		imul	di,bx,8 * LINE_BYTES
		push	di
		mov	cx,8 * LINE_BYTES / 2
		xor	ax,ax
		rep	stosw
		pop	di
		jmp	text_go
; The text at SI from G0 byte DI (no clearing), colour [colour].
text_at:	push	es
		mov	ax,GV_G0
		mov	es,ax
text_go:	mov	al,[colour]
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
		xor	ah,0a5h			; XOR A5h: gen_data.py)
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

; The line buffer: put_* append at DI (from line), end with put_end.
put_str:	lodsb
		or	al,al
		jz	.r
		stosb
		jmp	put_str
.r:		ret
put_dec:	push	cx			; AX: five digits
		push	dx
		push	si
		add	di,5
		push	di
		mov	cx,5
		mov	si,10
.digit:		xor	dx,dx
		div	si
		add	dl,'0'
		dec	di
		mov	[di],dl
		loop	.digit
		pop	di
		pop	si
		pop	dx
		pop	cx
		ret
put_hex:	push	ax			; AL: two digits
		shr	al,4
		call	.d
		pop	ax
		push	ax
		and	al,0fh
		call	.d
		pop	ax
		ret
.d:		add	al,'0'
		cmp	al,'9'
		jbe	.s
		add	al,'A' - '9' - 1
.s:		stosb
		ret
; Text line BX in [colour], overwritten in place, not cleared first
; (cleared, a line redrawn each frame shows only what was drawn before
; the beam passed): padded with spaces to LINE_CELLS cells, short of the
; machine's name at cell 48.
LINE_CELLS	equ	47
put_line:	mov	al,' '
.pad:		cmp	di,line + LINE_CELLS
		jae	.end
		stosb
		jmp	.pad
.end:		mov	byte [di],0
		mov	si,line
		imul	di,bx,8 * LINE_BYTES
		jmp	text_at


; ---------------------------------------------------------------------------
UPLOAD_ROW	equ	3		; rows from D: the program's
tests		dw	t_cpu, t_mon_write, t_mon_read, t_write, t_read, t_ping
		dw	t_frame, t_loops, t_verify, t_cpu
test_flags	dw	0, 0, 0, 0, 0, 0, 0, F_ONCE, F_ONCE, 0
		align	2
test_names	dw	s_ta, s_tb, s_tc, s_td, s_te, s_tf, s_tg, s_th, s_ti, s_tj

s_banner	db	'FDD SUBSYSTEM: restart the machine to leave.',13,10,'$'
s_24khz		db	'Select 15 kHz (DIP switch SW1 off) before starting.',13,10,'$'
s_memory	db	'Not enough memory (needs 3000h; load below 2000h).',13,10,'$'
s_title		db	'FDDSUB', 0
s_head		db	'ROW               MEAN   MIN   MAX     SD', 0
s_pass		db	'  PASS ', 0
s_of		db	' OF ', 0
s_done		db	'FDDSUB DONE: RESTART TO LEAVE', 0
s_timeout	db	'FDD SUBSYSTEM TIMEOUT: RESTART', 0
s_bad		db	'FDD SUBSYSTEM WRONG ANSWER: RESTART', 0
s_over		db	' *****', 0
s_empty		db	0
s_ta		db	'A CPU ADD LOOP   ', 0
s_tb		db	'B MON WRITE 256  ', 0
s_tc		db	'C MON READ 256   ', 0
s_td		db	'D OWN WRITE 256  ', 0
s_te		db	'E OWN READ 256   ', 0
s_tf		db	'F ROUND TRIP     ', 0
s_tg		db	'G FRAME 64+32    ', 0
s_th		db	'H FDC LOOPS      ', 0
s_ti		db	'I READ ERRORS    ', 0
s_tj		db	'J CPU ADD LOOP   ', 0
s_notes		db	'A-G J: BLOCKS IN 300 VRTC (MORE IS FASTER)', 0
s_notes2	db	'H: 256-STEP DJNZ LOOPS OF THE FDD CPU IN 300 VRTC', 0
s_notes3	db	'I: BYTES OF E DIFFERENT FROM D (0 = RIGHT)', 0

sync224		db	0c1h,57h,1ch,0,9fh,0,10h,0fh,25h,0,0e0h,0,2,4
		align	2
fb_bios		dw	4,320,224,0,0,0
frames4		dw	0,0,160,447,0,0,0,0,0,224,0,0,0,0,0,0
		dw	0,0,160,0,0,0,0,0,0,224,0,0,0,0,0,0
palette		dw	PALETTE32
tick		dw	0
t0		dw	0
count		dw	0
ink		dw	0
passes		dw	8
pass		dw	0
t32		dw	0, 0
slow		db	0
colour		db	0
line		times	64 db 0
		align	2
image_end:
IMAGE_TOP	equ	image_end - $$ + 100h	; offset of the end, below BUF
%if IMAGE_TOP > BUF
%error the program reaches its buffer
%endif
