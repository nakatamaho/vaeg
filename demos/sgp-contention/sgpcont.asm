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
; SGPCONT.COM: the SGP and the main CPU at the same time, and the two-screen
; display an arcade-game port for the VA uses (G0 in front of G1, both 320
; dots packed 4 bpp, G0 colour 0 transparent), on a PC-88VA/VA2 and vaeg
; (README.md). Synthetic images only.
;
; Each row counts what was done in FRAMES VRTCs (more is faster): CPU loops
; of 1024 iterations in main RAM, G0 and G1; SGP lists of 30 16x16 sprites
; drawn into G0 (BITBLT 0105h, source 0 transparent, from main RAM); the
; same lists while the CPU works in main RAM or in G1 (the CPU's work in
; blocks of 1024 iterations); 30 16x16 rectangles of G0 cleared by the CPU
; and by the SGP. At the end the sprites are drawn over a striped G1 band:
; the stripes must show through the sprites' holes. The loops, segments
; and lists are those of the port's test build measured on the machines.
; Never returns: restart the machine to leave.

		cpu	186
		org	100h

%include "font.inc"			; FONT, PALETTE32

BENCH_SEG	equ	2000h		; this program, copied (SGP address 2xxxxh)
RAM_SEG		equ	7280h		; the CPU's main RAM block
SRC_SEG		equ	7480h		; SGP sources: the sprite (128 bytes),
ZERO_SEG	equ	SRC_SEG + 8	; then zeros
MEMORY_TOP	equ	7500h
LIST		equ	0c000h		; SGP lists and work area (this segment)
CLIST		equ	0c400h
WORK		equ	0c800h
FRAMES		equ	600		; VRTCs a test (10 s)
N_SPRITES	equ	30		; sprites a list
LINE_BYTES	equ	160
LINES		equ	224
GV_G0		equ	0a000h
GV_G1		equ	0c000h
TEXT_TOP	equ	16
WHITE		equ	15
YELLOW		equ	11
RED		equ	9

start:		cld
		mov	dx,s_banner
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
		call	sources
		call	lists
		mov	si,s_title
		mov	byte [colour],RED
		xor	bx,bx
		call	text_line
		xor	bx,bx
.test:		push	bx
		mov	ax,[tests + bx]
		or	ax,ax
		jz	.pair
		mov	bx,ax
		call	run			; AX: blocks or lists
		pop	bx
		push	bx
		call	show			; AX at row BX / 2
		jmp	.next
.pair:		mov	ax,[pair_cpu + bx]
		call	pair			; AX lists, DX chunks of 64
		pop	bx
		push	bx
		push	dx
		call	show
		pop	ax
		shr	ax,4			; chunks of 64 to blocks of 1024
		pop	bx
		add	bx,2
		push	bx
		call	show
.next:		pop	bx
		add	bx,2
		cmp	bx,N_TESTS * 2
		jb	.test
		mov	si,s_done
		mov	byte [colour],RED
		mov	bx,N_TESTS + 3
		call	text_line
		mov	ax,GV_G0		; the final picture: G0 clear, the
		mov	es,ax			; sprites over the striped band
		xor	di,di
		xor	ax,ax
		mov	cx,8000h
		rep	stosw
		mov	ax,GV_G1		; (G1 lines 0-7: the loops' bytes)
		mov	es,ax
		xor	di,di
		mov	cx,4 * LINE_BYTES
		rep	stosw
		push	cs
		pop	es
		call	stripes
		mov	ax,LIST
		call	submit
		call	sgp_wait
bench_done:	nop				; the picture is complete here
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

; ---------------------------------------------------------------------------
; Video (as demos/sgp-bench): the BIOS sets up the single-plane 320x224
; screen; then two packed 4-bpp screens, G0 (palette set 1, colour 0
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

; ---------------------------------------------------------------------------
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

; The SGP drawing list LIST while the CPU repeats the 64-iteration chunk
; AX, for FRAMES VRTCs: AX = lists finished, DX = chunks.
pair:		mov	[chunk],ax
		mov	word [lists_n],0
		mov	word [chunks_n],0
		mov	ax,[tick]
.edge:		cmp	ax,[tick]
		je	.edge
		mov	ax,[tick]
		mov	[t0],ax
.list:		mov	ax,LIST
		call	submit
.chunk:		call	[chunk]
		inc	word [chunks_n]
		mov	dx,0506h
		in	al,dx
		test	al,1
		jnz	.chunk
		inc	word [lists_n]
		mov	ax,[tick]
		sub	ax,[t0]
		cmp	ax,FRAMES
		jb	.list
		mov	ax,[lists_n]
		mov	dx,[chunks_n]
		ret

c_ram_rmw:	push	es
		mov	es,[ram_seg]
		mov	cx,64
		xor	di,di
		mov	dx,0f05h
.l:		mov	al,[es:di]
		and	al,dl
		or	al,dh
		mov	[es:di],al
		inc	di
		dec	cx
		jnz	.l
		pop	es
		ret
c_g1_write:	push	es
		mov	es,[g1_seg]
		mov	cx,64
		xor	di,di
.l:		mov	[es:di],al
		inc	di
		dec	cx
		jnz	.l
		pop	es
		ret

; The tests: one block each.
%macro LOOP1024 2			; segment, 0 read, 1 write, 2 AND-OR
		push	es
		mov	es,%1
		mov	cx,1024
		xor	di,di
		mov	dx,0f05h
%%l:
%if %2 == 0
		mov	al,[es:di]
%elif %2 == 1
		mov	[es:di],al
%else
		mov	al,[es:di]
		and	al,dl
		or	al,dh
		mov	[es:di],al
%endif
		inc	di
		dec	cx
		jnz	%%l
		pop	es
		ret
%endmacro

t_cpu:		mov	cx,1024
.l:		add	ax,bx
		inc	di
		dec	cx
		jnz	.l
		ret
t_ram_rmw:	LOOP1024 [ram_seg], 2
t_g0_read:	LOOP1024 [g0_seg], 0
t_g0_write:	LOOP1024 [g0_seg], 1
t_g0_rmw:	LOOP1024 [g0_seg], 2
t_g1_read:	LOOP1024 [g1_seg], 0
t_g1_write:	LOOP1024 [g1_seg], 1
t_g1_rmw:	LOOP1024 [g1_seg], 2
t_ram_g0:	push	ds
		push	es
		mov	ax,[cs:g0_seg]
		mov	es,ax
		mov	ax,[cs:ram_seg]
		mov	ds,ax
		xor	si,si
		xor	di,di
		mov	cx,512
		rep	movsw
		pop	es
		pop	ds
		ret
t_stosw_g0:	push	es
		mov	es,[g0_seg]
		xor	di,di
		mov	cx,512
		rep	stosw
		pop	es
		ret
t_stosw_g1:	push	es
		mov	es,[g1_seg]
		xor	di,di
		mov	cx,512
		rep	stosw
		pop	es
		ret
t_sgp_draw:	mov	ax,LIST
		call	submit
		jmp	sgp_wait
t_sgp_clear:	mov	ax,CLIST
		call	submit
		jmp	sgp_wait
t_cpu_clear:	push	es			; the 30 rectangles, 8 bytes x 16
		mov	es,[g0_seg]
		xor	bx,bx
		xor	ax,ax
.rect:		mov	di,[offsets + bx]
		mov	dx,16
.line:		mov	cx,4
		rep	stosw
		add	di,LINE_BYTES - 8
		dec	dx
		jnz	.line
		add	bx,2
		cmp	bx,N_SPRITES * 2
		jb	.rect
		pop	es
		ret

; SGP list at CS:AX (SGP address 2:AX) started.
submit:		push	ax
		call	sgp_wait
		mov	dx,0580h
		mov	al,10h
		out	dx,al
		mov	dx,0500h
		pop	ax
		out	dx,ax
		add	dx,2
		mov	ax,BENCH_SEG >> 12
		out	dx,ax
		mov	dx,0504h
		xor	al,al
		out	dx,al
		mov	dx,0506h
		mov	al,1
		out	dx,al
		ret
sgp_wait:	push	bx
		push	cx
		mov	dx,0506h
		mov	bx,64
.outer:		mov	cx,0ffffh
.poll:		in	al,dx
		test	al,1
		jz	.ready
		loop	.poll
		dec	bx
		jnz	.outer
		mov	dx,0504h		; a timeout: stopped
		mov	al,2
		out	dx,al
.ready:		pop	cx
		pop	bx
		ret

; ---------------------------------------------------------------------------
; The sprite (16 x 16, 8 bytes a row): 0 where (x + y) mod 5 = 0, else
; (x + 2y) mod 15 + 1; then 128 zero bytes.
sources:	push	es
		mov	ax,SRC_SEG
		mov	es,ax
		xor	di,di
		xor	bp,bp			; y
.row:		xor	si,si			; x
.byte:		call	.pixel
		shl	al,4
		mov	ah,al
		inc	si
		call	.pixel
		or	al,ah
		stosb
		inc	si
		cmp	si,16
		jb	.byte
		inc	bp
		cmp	bp,16
		jb	.row
		xor	ax,ax
		mov	cx,64
		rep	stosw
		pop	es
		ret
.pixel:		push	dx
		push	bx
		mov	ax,si
		add	ax,bp
		mov	bl,5
		div	bl
		or	ah,ah
		jz	.zero
		mov	ax,bp
		shl	ax,1
		add	ax,si
		mov	bl,15
		div	bl
		mov	al,ah
		inc	al
		jmp	.p
.zero:		xor	al,al
.p:		pop	bx
		pop	dx
		ret

; Offsets of the 30 rectangles in G0 (bytes 104-151, lines 4-199) and the
; two lists: drawing (BITBLT 0105h) and clearing (0005h from zeros).
lists:		mov	di,offsets
		xor	cx,cx
.off:		mov	ax,cx
		mov	bl,5
		div	bl			; AL row, AH column
		mov	bl,ah
		xor	bh,bh
		imul	bx,bx,10
		add	bx,104
		xor	ah,ah
		imul	ax,ax,36
		add	ax,4
		imul	ax,ax,LINE_BYTES
		add	ax,bx
		stosw
		inc	cx
		cmp	cx,N_SPRITES
		jb	.off
		mov	di,LIST
		mov	dx,0105h
		mov	si,SRC_SEG
		call	.list
		mov	di,CLIST
		mov	dx,0005h
		mov	si,ZERO_SEG
		jmp	.list
.list:		mov	ax,3			; SET_WORK
		stosw
		mov	ax,WORK
		stosw
		mov	ax,BENCH_SEG >> 12
		stosw
		xor	bx,bx
.sprite:	mov	ax,4			; SET_SOURCE: 4 bpp, 16 x 16, 8
		stosw				; bytes a line, main RAM
		mov	ax,1
		stosw
		mov	ax,16
		stosw
		stosw
		mov	ax,8
		stosw
		mov	ax,si			; linear address of segment SI
		shl	ax,4
		stosw
		mov	ax,si
		shr	ax,12
		stosw
		mov	ax,5			; SET_DESTINATION: G0 (SGP 200000h)
		stosw
		mov	ax,1
		stosw
		mov	ax,16
		stosw
		stosw
		mov	ax,LINE_BYTES
		stosw
		mov	ax,[offsets + bx]
		stosw
		mov	ax,20h
		stosw
		mov	ax,7			; BITBLT
		stosw
		mov	ax,dx
		stosw
		add	bx,2
		cmp	bx,N_SPRITES * 2
		jb	.sprite
		mov	ax,1			; END
		stosw
		ret

; G1 bytes 100-151 of every line: stripes of set 0's colours 1-15.
stripes:	push	es
		mov	ax,GV_G1
		mov	es,ax
		xor	dx,dx			; line
.line:		imul	di,dx,LINE_BYTES
		add	di,100
		mov	cx,52
		xor	bx,bx
.byte:		mov	ax,dx
		shr	ax,3
		add	ax,bx
		shr	ax,2
		push	dx
		xor	dx,dx
		push	bx
		mov	bx,15
		div	bx
		pop	bx
		mov	al,dl
		inc	al
		mov	ah,al
		shl	ah,4
		or	al,ah
		pop	dx
		stosb
		inc	bx
		loop	.byte
		inc	dx
		cmp	dx,LINES
		jb	.line
		pop	es
		ret

; ---------------------------------------------------------------------------
; Row BX / 2 of the table: its name, then the value AX at column 18.
show:		push	bx
		push	ax
		mov	si,[names + bx]
		mov	di,line
.copy:		lodsb
		or	al,al
		jz	.pad
		stosb
		jmp	.copy
.pad:		mov	al,' '
.space:		cmp	di,line + 18
		jae	.number
		stosb
		jmp	.space
.number:	pop	ax
		add	di,5
		mov	byte [di],0
		mov	cx,5
		mov	si,10
.digit:		xor	dx,dx
		div	si
		add	dl,'0'
		dec	di
		mov	[di],dl
		loop	.digit
		pop	bx
		push	bx
		shr	bx,1
		add	bx,2
		mov	si,line
		mov	byte [colour],WHITE
		call	text_line
		pop	bx
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
tests		dw	t_cpu, t_ram_rmw, t_g0_read, t_g0_write, t_g0_rmw, t_stosw_g0
		dw	t_ram_g0, t_g1_read, t_g1_write, t_g1_rmw, t_stosw_g1
		dw	t_sgp_draw, 0, 0, 0, 0, t_cpu_clear, t_sgp_clear
pair_cpu	dw	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, c_ram_rmw, 0, c_g1_write, 0, 0, 0
names		dw	s_cpu, s_ram_rmw, s_g0_read, s_g0_write, s_g0_rmw, s_stosw_g0
		dw	s_ram_g0, s_g1_read, s_g1_write, s_g1_rmw, s_stosw_g1
		dw	s_sgp, s_sgp_ram, s_ram_sgp, s_sgp_g1, s_g1_sgp, s_cpu_clr
		dw	s_sgp_clr
s_banner	db	'SGP CONTENTION: restart the machine to leave.',13,10,'$'
s_24khz		db	'Select 15 kHz (DIP switch SW1 off) before starting.',13,10,'$'
s_memory	db	'Not enough memory (needs 7500h; load below 2000h).',13,10,'$'
s_title		db	'SGPCONT 2 SCREENS 600 VRTC', 0
s_done		db	'DONE: RESTART TO LEAVE', 0
s_cpu		db	'A CPU', 0
s_ram_rmw	db	'B RAM AND OR', 0
s_g0_read	db	'C G0 READ', 0
s_g0_write	db	'D G0 WRITE', 0
s_g0_rmw	db	'E G0 AND OR', 0
s_stosw_g0	db	'F STOSW G0', 0
s_ram_g0	db	'G MOVSW RAM G0', 0
s_g1_read	db	'H G1 READ', 0
s_g1_write	db	'I G1 WRITE', 0
s_g1_rmw	db	'J G1 AND OR', 0
s_stosw_g1	db	'K STOSW G1', 0
s_sgp		db	'L SGP 30 SPRITES', 0
s_sgp_ram	db	'M SGP WITH RAM', 0
s_ram_sgp	db	'N RAM WITH SGP', 0
s_sgp_g1	db	'O SGP WITH G1', 0
s_g1_sgp	db	'P G1 WITH SGP', 0
s_cpu_clr	db	'Q CPU CLEAR 30', 0
s_sgp_clr	db	'R SGP CLEAR 30', 0

sync224		db	0c1h,57h,1ch,0,9fh,0,10h,0fh,25h,0,0e0h,0,2,4
		align	2
fb_bios		dw	4,320,224,0,0,0
frames4		dw	0,0,160,447,0,0,0,0,0,224,0,0,0,0,0,0
		dw	0,0,160,0,0,0,0,0,0,224,0,0,0,0,0,0
palette		dw	PALETTE32
ram_seg		dw	RAM_SEG
g0_seg		dw	GV_G0
g1_seg		dw	GV_G1
tick		dw	0
t0		dw	0
count		dw	0
chunk		dw	0
lists_n		dw	0
chunks_n	dw	0
ink		dw	0
offsets		times	N_SPRITES dw 0
colour		db	0
line		times	32 db 0
		align	2
image_end:
IMAGE_TOP	equ	image_end - $$ + 100h	; offset of the end, below LIST
%if IMAGE_TOP > LIST
%error the program reaches its SGP lists
%endif
