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
; TSPSPR.COM: the TSP's hardware sprites over the two-screen display an
; arcade-game port for the VA uses (G0 in front of G1, both 320 dots packed
; 4 bpp at 15.98 kHz, 224 lines), on a PC-88VA/VA2 and vaeg (README.md).
; Synthetic images only. Five screens; SPACE goes to the next, after the
; last to the first. Never returns: restart the machine to leave.
;
; Keys: SPACE next screen; arrows (or J L I K) left, right, up, down; D sends
; DSPON once; H changes SPRON's HSPN (31, 15, 7, 3).
;
; TVRAM (CPU offsets in A0000h-A7FFFh, the character side; TSP word
; address = offset / 2): images from 1000h, screen 5's blocks at 5000h, the
; descriptor table at 7E00h (SPRON 7Eh). Sprite 31 (the BIOS's text cursor)
; is not used and CURDEF turns the cursor off. No DSPON unless D is pressed:
; the text display stays off after the SYNC.

		cpu	186
		org	100h

%include "data.inc"			; FONT, ALIEN_IMAGES, SHOT_IMAGE, PALETTE32

BENCH_SEG	equ	2000h		; this program, copied
MEMORY_TOP	equ	3000h
FRAMES		equ	600		; VRTCs a speed test (10 s)
LINE_BYTES	equ	160
LINES		equ	224
GV_G0		equ	0a000h
GV_G1		equ	0c000h
TOP		equ	32		; the sprites' area: lines 32-223
MAX		equ	31		; sprites (31: the cursor's, unused)
BAND		equ	120		; screens 3 and 5: the band's first line
; G0 (set 1) colours
WHITE		equ	1
RED		equ	2
YELLOW		equ	3
GREY		equ	4
; TVRAM images (CPU offsets)
PAT_BOX		equ	1000h		; 16x16 dots, white outline
PAT_CHECK	equ	1100h		; chequered (2x2) grey
PAT_ALIEN	equ	1200h		; 3 aliens x colours 1-14, 256 bytes each
PAT_SHOT	equ	3c00h		; colours 1-14, 8x16 dots, 128 bytes each
PAT_MONO	equ	4400h		; alien 0 in one colour, 64 bytes
SCRATCH		equ	5000h
TABLE		equ	7e00h
; Work area (this segment)
tab		equ	0c000h		; the descriptors (256 bytes)
mx		equ	tab + 256	; screen 4: places and steps
my		equ	mx + MAX * 2
mdx		equ	my + MAX * 2
mdy		equ	mdx + MAX * 2

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
		call	images
		call	none
		call	flush
		mov	al,15h			; CURDEF: sprite 31, cursor off
		call	tsp_cmd
		mov	al,0f8h
		call	tsp_prm
		call	spron
		call	scan_keys
		mov	[keys],al
.screens:	call	position
		call	order_screen
		mov	byte [moving],0
		call	line_screen
		mov	byte [moving],1
		call	line_screen
		call	tvram_screen
		jmp	.screens

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
; transparent) in front of G1 (set 0), all GVRAM cleared; then the sprites:
; colours 1-15 of the TSP the sprite class, all but 0 opaque, the order
; G0 > sprites > G1.
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
		mov	word [order],0
		call	set_order
		mov	dx,0124h		; G0 colour 0 transparent
		mov	ax,1
		out	dx,ax
		mov	dx,0126h		; G1 opaque
		xor	ax,ax
		out	dx,ax
		mov	dx,0110h		; TSP colours 1-15: the sprite class
		mov	ax,0f000h
		out	dx,ax
		mov	dx,012eh		; TSP colours opaque but 0
		mov	ax,1
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

; SPRON: the table at 7E00h, HSPN [hspn].
spron:		mov	al,82h
		call	tsp_cmd
		mov	al,TABLE >> 8
		call	tsp_prm
		xor	al,al
		call	tsp_prm
		mov	al,[hspn]
		shl	al,3
		jmp	tsp_prm

; Port 0106h from [order].
set_order:	mov	bx,[order]
		shl	bx,1
		mov	ax,[orders + bx]
		mov	dx,0106h
		out	dx,ax
		ret

; A0000h-AFFFFh: TVRAM (interrupts disabled), or GVRAM again.
map_tvram:	cli
		push	ax
		push	dx
		mov	dx,0153h
		in	al,dx
		and	al,0f0h
		or	al,01h
		out	dx,al
		pop	dx
		pop	ax
		ret
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
; The images, in TVRAM. In a 16-colour image a byte is two sprite dots:
; one graphics dot.
images:		call	map_tvram
		push	es
		mov	ax,0a000h
		mov	es,ax
		mov	di,PAT_BOX		; white outline
		xor	dx,dx
.box:		mov	al,0ffh
		or	dh,dh
		jz	.b
		cmp	dh,15
		je	.b
		or	dl,dl
		jz	.b
		cmp	dl,15
		je	.b
		xor	al,al
.b:		stosb
		inc	dl
		cmp	dl,16
		jb	.box
		xor	dl,dl
		inc	dh
		cmp	dh,16
		jb	.box
		xor	dx,dx			; chequered (2x2) grey
.check:		mov	al,dh
		xor	al,dl
		and	al,2
		jz	.c
		mov	al,0cch
.c:		stosb
		inc	dl
		cmp	dl,16
		jb	.check
		xor	dl,dl
		inc	dh
		cmp	dh,16
		jb	.check
		mov	si,ALIEN_IMAGES		; the aliens: 3 x colours 1-14
		mov	bx,3
.alien:		mov	al,1
.colour:	push	si
		mov	cx,256
		call	recolour
		pop	si
		inc	al
		cmp	al,15
		jb	.colour
		add	si,256
		dec	bx
		jnz	.alien
		mov	si,SHOT_IMAGE		; the shot: colours 1-14
		mov	al,1
.shot:		push	si
		mov	cx,128
		call	recolour
		pop	si
		inc	al
		cmp	al,15
		jb	.shot
		mov	si,ALIEN_IMAGES		; monochrome: alien 0's shape, each
		mov	di,PAT_MONO		; dot two sprite dots
		mov	cx,16 * 4
.mono:		mov	ah,[si]			; four dots a byte
		call	.pair
		mov	ah,[si + 1]
		call	.pair
		mov	ah,[si + 2]
		call	.pair
		mov	ah,[si + 3]
		call	.pair
		add	si,4
		stosb
		loop	.mono
		pop	es
		jmp	map_gvram
.pair:		shl	al,2
		or	ah,ah
		jz	.p
		or	al,3
.p:		ret

; CX image bytes at SI (0 transparent, 1 the colour AL, 2 white, 3 yellow)
; to ES:DI, a byte each.
recolour:	push	ax
		mov	[ink_body],al
.p:		lodsb
		or	al,al
		jz	.put
		cmp	al,2
		je	.white
		ja	.yellow
		mov	al,[ink_body]
		jmp	.put
.white:		mov	al,15
		jmp	.put
.yellow:	mov	al,5
.put:		mov	ah,al
		shl	ah,4
		or	al,ah
		stosb
		loop	.p
		pop	ax
		ret

; ---------------------------------------------------------------------------
; The descriptors (tab), every one off.
none:		push	di
		mov	di,tab
		xor	ax,ax
		mov	cx,32 * 4
		rep	stosw
		pop	di
		ret

; tab to TVRAM's table.
flush:		call	map_tvram
		push	es
		mov	ax,0a000h
		mov	es,ax
		mov	si,tab
		mov	di,TABLE
		mov	cx,32 * 4
		rep	movsw
		pop	es
		jmp	map_gvram

; Descriptor BX (0-30): 16 lines from line AX, dot DX (both may be
; negative), of [kind]: 0 an alien (C32: 16 colours, 32 sprite dots; image
; [design], colour CL 1-14, or [image] when not 0), 1 a shot (C16: 16
; colours, 16 dots, colour CL), 2 the alien in one colour (M32: 32 dots,
; colour CL). Sprite X = 2 x dot + [xoff], Y = line + [yoff].
sprite:		push	ax
		push	bx
		push	cx
		push	dx
		push	di
		shl	bx,3
		lea	di,[tab + bx]
		add	ax,[yoff]
		and	ax,01ffh
		or	ax,(3 << 10) | (1 << 9)	; 16 lines, shown
		mov	[di],ax
		shl	dx,1
		add	dx,[xoff]
		and	dx,03ffh
		xor	ch,ch
		dec	cx			; CX: colour - 1
		mov	al,[kind]
		cmp	al,1
		je	.shot
		ja	.mono
		or	dx,3 << 11		; 32 dots
		mov	ax,[image]
		or	ax,ax
		jnz	.spda
		mov	ax,[design]		; (design x 14 + colour - 1) x 256
		mov	bx,14
		push	dx
		mul	bx
		pop	dx
		add	ax,cx
		mov	ah,al
		xor	al,al
		add	ax,PAT_ALIEN
		jmp	.spda
.shot:		or	dx,1 << 11		; 16 dots
		mov	ax,cx
		shl	ax,7
		add	ax,PAT_SHOT
		jmp	.spda
.mono:		or	dx,1 << 10		; 32 dots, monochrome
		inc	cx
		shl	cl,4
		mov	[di + 6],cl
		mov	ax,PAT_MONO
.spda:		mov	[di + 2],dx
		shr	ax,1			; TSP word address
		mov	[di + 4],ax
		pop	di
		pop	dx
		pop	cx
		pop	bx
		pop	ax
		ret

; ---------------------------------------------------------------------------
; Keys held (K_*), from the key matrix (active low).
K_LEFT		equ	01h
K_RIGHT		equ	02h
K_UP		equ	04h
K_DOWN		equ	08h
K_NEXT		equ	10h
K_DSPON		equ	20h
K_HSPN		equ	40h
scan_keys:	push	bx
		push	dx
		xor	bl,bl
		in	al,08h			; up (bit 1), right (bit 2)
		not	al
		test	al,02h
		jz	.r
		or	bl,K_UP
.r:		test	al,04h
		jz	.d
		or	bl,K_RIGHT
.d:		in	al,0ah			; down (bit 1), left (bit 2)
		not	al
		test	al,02h
		jz	.l
		or	bl,K_DOWN
.l:		test	al,04h
		jz	.sp
		or	bl,K_LEFT
.sp:		in	al,09h			; SPACE (bit 6)
		not	al
		test	al,40h
		jz	.letters
		or	bl,K_NEXT
.letters:	in	al,03h			; H I J K L (bits 0-4)
		not	al
		test	al,01h
		jz	.i
		or	bl,K_HSPN
.i:		test	al,02h
		jz	.j
		or	bl,K_UP
.j:		test	al,04h
		jz	.k
		or	bl,K_LEFT
.k:		test	al,08h
		jz	.ll
		or	bl,K_DOWN
.ll:		test	al,10h
		jz	.dk
		or	bl,K_RIGHT
.dk:		in	al,02h			; D (bit 4)
		test	al,10h
		jnz	.done
		or	bl,K_DSPON
.done:		mov	al,bl
		pop	dx
		pop	bx
		ret

; A frame: wait for the next VRTC; AL = the keys newly pressed. D and H
; are done here.
frame:		mov	ax,[tick]
.w:		cmp	ax,[tick]
		je	.w
		call	scan_keys
		mov	ah,[keys]
		mov	[keys],al
		not	ah
		and	al,ah
		test	al,K_DSPON
		jz	.h
		cmp	byte [dspon],0
		jne	.h
		push	ax
		mov	byte [dspon],1
		mov	al,12h			; DSPON: the table at 7F00h
		call	tsp_cmd
		mov	al,7fh
		call	tsp_prm
		xor	al,al
		call	tsp_prm
		xor	al,al
		call	tsp_prm
		mov	al,15h			; the cursor off again
		call	tsp_cmd
		mov	al,0f8h
		call	tsp_prm
		pop	ax
.h:		test	al,K_HSPN
		jz	.r
		push	ax
		mov	al,[hspn]		; 31, 15, 7, 3, 31
		shr	al,1
		cmp	al,3
		jae	.set
		mov	al,31
.set:		mov	[hspn],al
		call	spron
		pop	ax
.r:		ret

; ---------------------------------------------------------------------------
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
put_line:	mov	byte [di],0		; text line BX in [colour]
		mov	si,line
		jmp	text_line

; G0 and G1 cleared, the sprites off, the title SI and help DX (lines 0
; and 2).
screen:		push	dx
		push	si
		call	clear_gvram
		call	none
		call	flush
		pop	si
		mov	byte [colour],RED
		xor	bx,bx
		call	text_line
		pop	si
		mov	byte [colour],YELLOW
		mov	bx,2
		jmp	text_line

; G1 dot DX, line BX: colour CL (clipped to the screen).
plot:		cmp	dx,320
		jae	.r
		cmp	bx,LINES
		jae	.r
		push	ax
		push	di
		push	es
		mov	ax,GV_G1
		mov	es,ax
		imul	di,bx,LINE_BYTES
		mov	ax,dx
		shr	ax,1
		add	di,ax
		mov	al,cl
		mov	ah,0f0h			; keep the other dot
		test	dl,1
		jnz	.lo
		shl	al,4
		mov	ah,0fh
.lo:		and	[es:di],ah
		or	[es:di],al
		pop	es
		pop	di
		pop	ax
.r:		ret

; A 16x16 outline in G1 at dot DX, line BX, colour CL (clipped).
g1_box:		push	bx
		push	dx
		push	si
		mov	si,16
.h:		call	plot			; top and bottom
		add	bx,15
		call	plot
		sub	bx,15
		inc	dx
		dec	si
		jnz	.h
		sub	dx,16
		mov	si,16
.v:		call	plot			; sides
		add	dx,15
		call	plot
		sub	dx,15
		inc	bx
		dec	si
		jnz	.v
		pop	si
		pop	dx
		pop	bx
		ret

; G1 rectangle: dot DX (even), line BX, CX bytes wide, SI lines, byte AL.
g1_fill:	push	es
		push	bx
		push	di
		push	ax
		mov	ax,GV_G1
		mov	es,ax
		pop	ax
.l:		push	cx
		imul	di,bx,LINE_BYTES
		push	dx
		shr	dx,1
		add	di,dx
		pop	dx
		rep	stosb
		pop	cx
		inc	bx
		dec	si
		jnz	.l
		pop	di
		pop	bx
		pop	es
		ret

; ---------------------------------------------------------------------------
; 1 POSITION: white sprite outlines over red G1 outlines where they should
; be (sprite X = 2 x dot, Y = line); the arrows move the sprites by a
; sprite dot or a line. Boxes 7-10 cross the left, right and bottom edges
; and (8 lines above the top) the top: box 10 as the manual says (the
; image's address advanced by 8 lines), box 11 without.

N_POS		equ	12
position:	mov	si,s_pos
		mov	dx,s_pos_help
		call	screen
		xor	si,si			; the red G1 outlines
.box:		mov	dx,[pos_dot + si]
		mov	bx,[pos_line + si]
		mov	cl,2
		call	g1_box
		add	si,2
		cmp	si,N_POS * 2
		jb	.box
		mov	byte [kind],0
		mov	word [image],PAT_BOX
.show:		xor	bx,bx
		xor	si,si
.spr:		mov	dx,[pos_dot + si]
		mov	ax,[pos_line + si]
		call	sprite
		inc	bx
		add	si,2
		cmp	si,N_POS * 2
		jb	.spr
		add	word [tab + 10 * 8 + 4],8 * 16 / 2	; box 10: from its
		call	frame			; ninth line
		push	ax
		call	flush
		mov	di,line			; the first box: dot, line, X, Y
		mov	si,s_d
		call	put_str
		mov	ax,[pos_dot]
		call	put_dec
		mov	si,s_l
		call	put_str
		mov	ax,[pos_line]
		call	put_dec
		mov	si,s_x
		call	put_str
		mov	ax,[tab + 2]
		and	ax,03ffh
		call	put_dec
		mov	si,s_y
		call	put_str
		mov	ax,[tab]
		and	ax,01ffh
		call	put_dec
		mov	byte [colour],WHITE
		mov	bx,1
		call	put_line
		pop	ax
		test	al,K_NEXT
		jnz	.done
		test	al,K_LEFT
		jz	.r
		dec	word [xoff]
.r:		test	al,K_RIGHT
		jz	.u
		inc	word [xoff]
.u:		test	al,K_UP
		jz	.d
		dec	word [yoff]
.d:		test	al,K_DOWN
		jz	.show
		inc	word [yoff]
		jmp	.show
.done:		mov	word [image],0
		ret

; ---------------------------------------------------------------------------
; 2 ORDER: aliens and chequered sprites over G1's blue stripes, G0 bars
; across them; UP or DOWN changes port 0106h.

order_screen:	mov	si,s_order
		mov	dx,s_order_help
		call	screen
		xor	dx,dx			; G1: blue stripes 4 dots wide
.stripe:	mov	bx,TOP
		mov	cx,2
		mov	si,LINES - TOP
		mov	al,33h
		call	g1_fill
		add	dx,8
		cmp	dx,320
		jb	.stripe
		push	es			; G0: bars of 4 lines
		mov	ax,GV_G0
		mov	es,ax
		xor	bx,bx
.bar:		mov	ax,[bar_line + bx]
		imul	di,ax,LINE_BYTES
		mov	al,GREY * 11h
		mov	cx,4 * LINE_BYTES
		rep	stosb
		add	bx,2
		cmp	bx,3 * 2
		jb	.bar
		pop	es
		mov	byte [kind],0		; 3 rows of 9: aliens and chequers
		xor	bx,bx
		mov	ax,48
.row:		mov	dx,16
		mov	cx,9
.col:		push	cx
		mov	word [image],PAT_CHECK
		test	bl,1
		jz	.c
		mov	word [image],0
		mov	cx,bx
		shr	cx,1
		mov	[design],cx
		and	word [design],1
		mov	cl,4			; green, yellow in turn
		test	bl,2
		jz	.c
		mov	cl,5
.c:		call	sprite
		pop	cx
		inc	bx
		add	dx,32
		loop	.col
		add	ax,64
		cmp	bx,27
		jb	.row
		mov	word [image],0
		call	flush
.frame:		mov	di,line
		mov	si,s_order_n
		call	put_str
		mov	bx,[order]
		shl	bx,1
		mov	si,[order_names + bx]
		call	put_str
		mov	byte [colour],WHITE
		mov	bx,1
		call	put_line
		call	frame
		test	al,K_NEXT
		jnz	.done
		test	al,K_UP | K_DOWN
		jz	.frame
		mov	bx,[order]
		inc	bx
		cmp	bx,3
		jb	.o
		xor	bx,bx
.o:		mov	[order],bx
		call	set_order
		jmp	.frame
.done:		mov	word [order],0
		jmp	set_order

; ---------------------------------------------------------------------------
; 3 LINE and 4 MOVE ([moving]): N sprites on the lines BAND to BAND + 15,
; each with G1 marks of its colour above and below; or N moving. LEFT and
; RIGHT: N; UP or DOWN: the kind. SPROV and the status's SC each VRTC.

line_screen:	mov	si,s_line
		cmp	byte [moving],0
		je	.t
		mov	si,s_move
.t:		mov	dx,s_line_help
		call	screen
		mov	word [n],6
		cmp	byte [moving],0
		je	.k
		mov	word [n],24
		call	move_init
.k:		mov	byte [kind],0
.layout:	call	layout
.frame:		call	frame
		push	ax
		cmp	byte [moving],0
		je	.still
		call	move_step
		call	flush
.still:		mov	dx,0142h		; the status's SC, SPROV
		in	al,dx
		and	al,20h
		mov	[sc],al
		call	sprov
		mov	[ov],al
		mov	di,line
		mov	si,s_n
		call	put_str
		mov	ax,[n]
		call	put_dec
		mov	al,' '
		stosb
		mov	bl,[kind]
		xor	bh,bh
		shl	bx,1
		mov	si,[kind_names + bx]
		call	put_str
		mov	si,s_hspn
		call	put_str
		mov	al,[hspn]
		xor	ah,ah
		call	put_dec
		mov	si,s_ov
		call	put_str
		mov	al,[ov]
		call	put_hex
		mov	si,s_sc
		call	put_str
		mov	al,[sc]
		call	put_hex
		mov	byte [colour],WHITE
		mov	bx,1
		call	put_line
		pop	ax
		test	al,K_NEXT
		jnz	.done
		test	al,K_UP | K_DOWN
		jz	.n
		inc	byte [kind]
		cmp	byte [kind],3
		jb	.layout
		mov	byte [kind],0
		jmp	.layout
.n:		test	al,K_LEFT
		jz	.n2
		cmp	word [n],1
		jbe	.n2
		dec	word [n]
		jmp	.layout
.n2:		test	al,K_RIGHT
		jz	.frame
		cmp	word [n],MAX
		jae	.frame
		inc	word [n]
		jmp	.layout
.done:		call	none
		jmp	flush

; Sprite i's colour (i mod 14 + 1) to CL and design (i mod 3), for BX = i.
colour_of:	push	ax
		push	dx
		mov	ax,bx
		xor	dx,dx
		mov	cx,3
		div	cx
		mov	[design],dx
		mov	ax,bx
		xor	dx,dx
		mov	cx,14
		div	cx
		mov	cl,dl
		inc	cl
		pop	dx
		pop	ax
		ret

; Standing: sprite i at dot 8 + spacing x i (spacing 296 / N, at most 16,
; even), its marks at lines BAND - 6 to - 3 and BAND + 18 to + 21.
layout:		call	none
		cmp	byte [moving],0
		jne	move_step
		mov	dx,0			; G1's lines BAND - 8 to + 23 cleared
		mov	bx,BAND - 8
		mov	cx,LINE_BYTES
		mov	si,32
		xor	al,al
		call	g1_fill
		mov	ax,296
		xor	dx,dx
		div	word [n]
		cmp	ax,16
		jbe	.s
		mov	ax,16
.s:		and	al,0feh
		mov	[space],ax
		xor	bx,bx
		mov	dx,8
.spr:		call	colour_of
		mov	ax,BAND
		call	sprite
		push	bx
		mov	al,cl			; marks: 2 dots wide
		shl	al,4
		or	al,cl
		push	cx
		mov	cx,1
		mov	bx,BAND - 6
		mov	si,4
		call	g1_fill
		mov	bx,BAND + 18
		mov	si,4
		call	g1_fill
		pop	cx
		pop	bx
		add	dx,[space]
		inc	bx
		cmp	bx,[n]
		jb	.spr
		jmp	flush

; 4 MOVE: the sprites' places (dots 0-303, lines TOP-207) and steps.
move_init:	xor	bx,bx
		mov	ax,1234h
.i:		imul	ax,ax,25173
		add	ax,13849
		mov	dx,ax			; dot
		shr	dx,7
		cmp	dx,304
		jb	.x
		sub	dx,256
.x:		mov	[mx + bx],dx
		mov	dx,ax			; line
		and	dx,0ffh
		add	dx,TOP
		cmp	dx,208
		jb	.y
		sub	dx,128
.y:		mov	[my + bx],dx
		mov	dx,bx			; steps: 1-3 dots, 1-2 lines, signs
		shr	dx,1			; by the number
		mov	si,dx
		and	si,3
		jnz	.sx
		inc	si
.sx:		test	dl,1
		jz	.sx2
		neg	si
.sx2:		mov	[mdx + bx],si
		mov	si,dx
		shr	si,1
		and	si,1
		inc	si
		test	dl,4
		jz	.sy
		neg	si
.sy:		mov	[mdy + bx],si
		add	bx,2
		cmp	bx,MAX * 2
		jb	.i
		ret

move_step:	call	none
		xor	bx,bx
		xor	si,si
.s:		mov	ax,[mx + si]
		add	ax,[mdx + si]
		cmp	ax,0
		jl	.bx
		cmp	ax,304
		jl	.okx
.bx:		neg	word [mdx + si]
		mov	ax,[mx + si]
.okx:		mov	[mx + si],ax
		mov	dx,ax
		mov	ax,[my + si]
		add	ax,[mdy + si]
		cmp	ax,TOP
		jl	.by
		cmp	ax,208
		jl	.oky
.by:		neg	word [mdy + si]
		mov	ax,[my + si]
.oky:		mov	[my + si],ax
		call	colour_of
		call	sprite
		add	si,2
		inc	bx
		cmp	bx,[n]
		jb	.s
		ret

; AL = SPROV's byte (FFh if none comes).
sprov:		mov	al,81h
		call	tsp_cmd
		mov	dx,0142h
		mov	cx,2000
.w:		in	al,dx
		test	al,02h
		jnz	.read
		loop	.w
		mov	al,0ffh
		ret
.read:		mov	dx,0146h
		in	al,dx
		ret

; ---------------------------------------------------------------------------
; 5 TVRAM: 1024-byte blocks in FRAMES VRTCs (more is faster).

N_TESTS		equ	4
tvram_screen:	mov	si,s_tvram
		mov	dx,s_tvram_help
		call	screen
		mov	word [n],MAX		; 31 aliens standing
		mov	byte [kind],0
		mov	byte [moving],0
		call	layout
		xor	bx,bx
.test:		push	bx
		cmp	bx,2 * 2		; the third row: sprites off
		jne	.on
		mov	al,83h
		call	tsp_cmd
.on:		mov	bx,[tests + bx]
		call	run
		pop	bx
		push	bx
		push	ax
		mov	di,line
		mov	si,[test_names + bx]
		call	put_str
		pop	ax
		call	put_dec
		mov	byte [colour],WHITE
		shr	bx,1
		add	bx,4
		call	put_line
		pop	bx
		cmp	bx,2 * 2
		jne	.next
		push	bx
		call	spron
		pop	bx
.next:		add	bx,2
		cmp	bx,N_TESTS * 2
		jb	.test
		mov	si,s_tvram_done
		mov	byte [colour],RED
		mov	bx,9
		call	text_line
.frame:		call	frame
		test	al,K_NEXT
		jz	.frame
		call	none
		jmp	flush

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

%macro LOOP1K 2				; segment, 0 read, 1 write
		push	es
		mov	es,%1
		mov	cx,1024
		xor	di,di
		xor	al,al
%%l:
%if %2 == 0
		mov	al,[es:di]
%else
		mov	[es:di],al
%endif
		inc	di
		dec	cx
		jnz	%%l
		pop	es
		ret
%endmacro
t_tv_write:	call	map_tvram
		call	.loop
		jmp	map_gvram
.loop:		LOOP1K	[cs:tv_seg], 1
t_tv_read:	call	map_tvram
		call	.loop
		jmp	map_gvram
.loop:		LOOP1K	[cs:tv_seg], 0
t_g1_write:	LOOP1K	[cs:g1_seg], 1

; ---------------------------------------------------------------------------
tests		dw	t_tv_write, t_tv_read, t_tv_write, t_g1_write
test_names	dw	s_ta, s_tb, s_tc, s_td
orders		dw	0b9ah, 0ba9h, 09bah
order_names	dw	s_order1, s_order2, s_order3
kind_names	dw	s_c32, s_c16, s_m32
pos_dot		dw	152, 0, 304, 0, 304, 80, 224, -8, 312, 152, 240, 272
pos_line	dw	120, TOP, TOP, 208, 208, 72, 72, 120, 120, 216, -8, -8
bar_line	dw	54, 118, 182

s_banner	db	'TSP SPRITES: restart the machine to leave.',13,10,'$'
s_24khz		db	'Select 15 kHz (DIP switch SW1 off) before starting.',13,10,'$'
s_memory	db	'Not enough memory (needs 3000h; load below 2000h).',13,10,'$'
s_pos		db	'TSPSPR 1 POSITION', 0
s_pos_help	db	'ARROWS MOVE THE SPRITES  SPACE NEXT', 0
s_order		db	'TSPSPR 2 ORDER', 0
s_order_help	db	'UP OR DOWN: ORDER  SPACE NEXT', 0
s_line		db	'TSPSPR 3 LINE', 0
s_move		db	'TSPSPR 4 MOVE', 0
s_line_help	db	'LEFT RIGHT N  UP KIND  H HSPN  SPACE NEXT', 0
s_tvram		db	'TSPSPR 5 TVRAM 600 VRTC', 0
s_tvram_help	db	'1024-BYTE BLOCKS: MORE IS FASTER', 0
s_tvram_done	db	'SPACE: AGAIN FROM 1', 0
s_d		db	'D ', 0
s_l		db	' L ', 0
s_x		db	' X ', 0
s_y		db	' Y ', 0
s_order_n	db	'ORDER ', 0
s_order1	db	'1 G0 > SPRITES > G1', 0
s_order2	db	'2 SPRITES > G0 > G1', 0
s_order3	db	'3 G0 > G1 > SPRITES', 0
s_n		db	'N ', 0
s_hspn		db	' HSPN ', 0
s_ov		db	' OV ', 0
s_sc		db	' SC ', 0
s_c32		db	'C32', 0
s_c16		db	'C16', 0
s_m32		db	'M32', 0
s_ta		db	'TV WRITE 31 ON   ', 0
s_tb		db	'TV READ 31 ON    ', 0
s_tc		db	'TV WRITE OFF     ', 0
s_td		db	'G1 WRITE         ', 0

sync224		db	0c1h,57h,1ch,0,9fh,0,10h,0fh,25h,0,0e0h,0,2,4
		align	2
fb_bios		dw	4,320,224,0,0,0
frames4		dw	0,0,160,447,0,0,0,0,0,224,0,0,0,0,0,0
		dw	0,0,160,0,0,0,0,0,0,224,0,0,0,0,0,0
palette		dw	PALETTE32
tv_seg		dw	0a000h + SCRATCH / 16
g1_seg		dw	GV_G1 + (LINES - 7) * LINE_BYTES / 16	; (G1's last lines)
tick		dw	0
t0		dw	0
count		dw	0
ink		dw	0
order		dw	0
xoff		dw	0
yoff		dw	0
n		dw	0
space		dw	0
image		dw	0
design		dw	0
keys		db	0
kind		db	0
moving		db	0
dspon		db	0
hspn		db	31
ov		db	0
sc		db	0
colour		db	0
ink_body	db	0
line		times	64 db 0
		align	2
image_end:
IMAGE_TOP	equ	image_end - $$ + 100h	; offset of the end, below tab
%if IMAGE_TOP > tab
%error the program reaches its work area
%endif
