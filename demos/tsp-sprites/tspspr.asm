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
; Keys: SPACE next screen; arrows (or J L I K) left, right, up, down; S the
; SYNC (224 or 200 lines); H changes SPRON's HSPN (31, 15, 7, 3); A screen
; 3's sweep.
;
; TVRAM (CPU offsets in A0000h-A7FFFh, the character side; TSP word
; address = offset / 2): images from 1000h, screen 5's blocks at 5000h, the
; descriptor table at 7E00h (SPRON 7Eh). Sprite 31 (the BIOS's text cursor)
; is not used and CURDEF turns the cursor off. DSPON follows every SYNC:
; without it the VA and VA2 showed no sprite (2026-10-27), where vaeg
; showed them.

		cpu	186
		org	100h

%include "data.inc"			; FONT, ALIEN_IMAGES, SHOT_IMAGE, PALETTE32

BENCH_SEG	equ	2000h		; this program, copied
MEMORY_TOP	equ	3000h
FRAMES		equ	300		; VRTCs a measurement (5 s)
AUTO_FRAMES	equ	120		; screen 3's sweep: VRTCs a step
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
PAT_MONO	equ	4400h		; alien 0 in one colour: 32 dots (64 bytes),
PAT_MONO64	equ	4440h		; twice side by side (64 dots, 128 bytes),
PAT_MONO128	equ	44c0h		; four times (128 dots, 256 bytes)
SCRATCH		equ	5000h
PAT_WIDE	equ	5400h		; alien 0 twice (32x16 dots), colours 1-14,
					; 512 bytes each (to 7000h)
TABLE		equ	7e00h
; Work area (this segment)
tab		equ	0c000h		; the descriptors (256 bytes)
mx		equ	tab + 256	; screen 4: places and steps
my		equ	mx + MAX * 2
mdx		equ	my + MAX * 2
mdy		equ	mdx + MAX * 2
ZEROS		equ	0c400h		; screen 5: 1024 bytes of zeros
RAMBUF		equ	0c800h		; and a main RAM target
st_count	equ	0cc00h		; and the rows' statistics: count,
st_first	equ	st_count + 2 * N_TESTS	; first value, sum of the
st_sum		equ	st_first + 2 * N_TESTS	; deviations, minimum, maximum,
st_min		equ	st_sum + 2 * N_TESTS	; sum of the squared deviations
st_max		equ	st_min + 2 * N_TESTS	; (dwords)
st_sq		equ	st_max + 2 * N_TESTS
ST_WORDS	equ	7 * N_TESTS

start:		cld
		mov	si,81h			; TSPSPR [P]: screen 5's passes (1-99)
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
		call	model_id
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
		call	tsp_start		; DSPON (CURDEF off), SPRON
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
		call	send_sync
		sti
		ret

; SYNC: the 224-line vector, or the BIOS's 200-line one if [sync200].
send_sync:	mov	si,sync224
		cmp	byte [sync200],0
		je	.s
		mov	si,sync200v
.s:		mov	al,10h
		call	tsp_cmd
		mov	cx,14
.sync:		lodsb
		call	tsp_prm
		loop	.sync
		ret

; The TSP started: DSPON (SYNC stops the display, and on the machines the
; sprites with it), the cursor off, SPRON.
tsp_start:	mov	byte [dspon],0
		call	dspon_once
		jmp	spron

; S: the other SYNC, then the TSP started again.
toggle_sync:	xor	byte [sync200],1
		cli
		call	send_sync
		sti
		jmp	tsp_start

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
		mov	di,PAT_WIDE		; alien 0 twice: colours 1-14
		mov	al,1
.wide:		mov	si,ALIEN_IMAGES
		mov	dx,16
.wrow:		mov	cx,16
		call	recolour
		sub	si,16
		mov	cx,16
		call	recolour
		dec	dx
		jnz	.wrow
		inc	al
		cmp	al,15
		jb	.wide
		mov	si,ALIEN_IMAGES		; monochrome: alien 0's shape, each
		mov	bx,monobuf		; dot two sprite dots
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
		mov	[bx],al
		inc	bx
		loop	.mono
		mov	si,monobuf		; once, twice, four times a line
		mov	di,PAT_MONO
		mov	cx,64
		rep	movsb
		mov	si,monobuf
		mov	dx,16
.m64:		movsw
		movsw
		sub	si,4
		movsw
		movsw
		dec	dx
		jnz	.m64
		mov	si,monobuf
		mov	dx,16
.m128:		mov	cx,4
.m128r:		movsw
		movsw
		sub	si,4
		loop	.m128r
		add	si,4
		dec	dx
		jnz	.m128
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
; colour CL), 3 two aliens (C64: 16 colours, 64 dots), 4 and 5 two and four
; aliens in one colour (M64, M128). Sprite X = 2 x dot + [xoff], Y = line +
; [yoff].
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
		cmp	al,2
		je	.mono32
		cmp	al,3
		je	.wide
		cmp	al,4
		je	.mono64
		cmp	al,5
		je	.mono128
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
.wide:		or	dx,7 << 11		; 64 dots
		mov	ax,cx
		shl	ax,9
		add	ax,PAT_WIDE
		jmp	.spda
.mono32:	mov	ax,PAT_MONO		; monochrome: 32, 64, 128 dots
		jmp	.mono
.mono64:	or	dx,1 << 11
		mov	ax,PAT_MONO64
		jmp	.mono
.mono128:	or	dx,3 << 11
		mov	ax,PAT_MONO128
.mono:		or	dx,1 << 10
		inc	cx
		shl	cl,4
		mov	[di + 6],cl
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
K_AUTO		equ	80h
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
.dk:		in	al,02h			; A (bit 1), D (bit 4)
		test	al,02h
		jnz	.dd
		or	bl,K_AUTO
.dd:		test	al,10h
		jnz	.done
		or	bl,K_DSPON
.done:		mov	al,bl
		pop	dx
		pop	bx
		ret

; A frame: wait for the next VRTC; AL = the keys newly pressed. D and H
; are done here.
; The arrows held repeat: after REPEAT_FIRST VRTCs, every REPEAT_NEXT.
REPEAT_FIRST	equ	24
REPEAT_NEXT	equ	4
frame:		mov	ax,[tick]
.w:		cmp	ax,[tick]
		je	.w
		call	scan_keys
		mov	ah,[keys]
		mov	[keys],al
		cmp	al,ah			; the same keys held: count
		jne	.change
		inc	word [held]
		jmp	.edge
.change:	mov	word [held],0
.edge:		not	ah
		and	al,ah
		mov	ah,[keys]		; arrows held long enough: again
		and	ah,K_LEFT | K_RIGHT | K_UP | K_DOWN
		jz	.d
		cmp	word [held],REPEAT_FIRST
		jb	.d
		push	ax
		mov	ax,[held]
		sub	ax,REPEAT_FIRST
		and	ax,REPEAT_NEXT - 1
		pop	ax
		jnz	.d
		or	al,ah
.d:		push	ax			; S (port 04h bit 3): the SYNC
		in	al,04h
		not	al
		and	al,08h
		mov	ah,[s_prev]
		mov	[s_prev],al
		not	ah
		and	al,ah
		jz	.nos
		call	toggle_sync
.nos:		pop	ax
		test	al,K_DSPON
		jz	.h
		push	ax
		call	dspon_once
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

; DSPON (the table at 7F00h, the text cleared by the BIOS) unless sent.
dspon_once:	cmp	byte [dspon],0
		jne	.r
		mov	byte [dspon],1
		mov	al,12h
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
		jmp	tsp_prm
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
		call	show_model
		pop	si
		mov	byte [colour],YELLOW
		mov	bx,2
		jmp	text_line

; The machine from the ROM's word at F000:FFFE (FFFFh PC-88VA, FFFEh
; PC-88VA2 or VA3, FFFDh PC-88VA with the PC-88VA-91 board), as a string.
model_id:	push	es
		mov	ax,0f000h
		mov	es,ax
		mov	ax,[es:0fffeh]
		pop	es
		mov	[model_word],ax
		mov	si,s_va
		cmp	ax,0ffffh
		je	.set
		mov	si,s_va2
		cmp	ax,0fffeh
		je	.set
		mov	si,s_va91
		cmp	ax,0fffdh
		je	.set
		mov	di,s_unknown + 1	; ? and the word in hexadecimal
		mov	al,ah
		call	put_hex
		mov	al,[model_word]
		call	put_hex
		mov	si,s_unknown
.set:		mov	[model_str],si
		ret

; The machine at the right of the title line (cell 48: clear of screen
; 1's top boxes and screen 5's title), in yellow.
show_model:	push	si
		push	di
		push	bx
		mov	si,[model_str]
		mov	byte [colour],YELLOW
		mov	di,48 * 3
		call	text_at
		pop	bx
		pop	di
		pop	si
		ret

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
		mov	si,s_k			; the keys read (K_* bits)
		call	put_str
		mov	al,[keys]
		call	put_hex
		mov	si,s_s224		; the SYNC
		cmp	byte [sync200],0
		je	.s224
		mov	si,s_s200
.s224:		call	put_str
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
		mov	word [n],MAX		; standing: all 31 at first
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
		test	al,K_AUTO		; A: the sweep on or off
		jz	.auto
		xor	byte [auto],1
		mov	word [n],1
		mov	byte [kind],0
		mov	word [auto_cnt],0
		jmp	.layout
.auto:		cmp	byte [auto],0		; the sweep: N 1-31 of each kind,
		je	.keys			; AUTO_FRAMES each
		inc	word [auto_cnt]
		cmp	word [auto_cnt],AUTO_FRAMES
		jb	.keys
		mov	word [auto_cnt],0
		inc	word [n]
		cmp	word [n],MAX
		jbe	.layout
		mov	word [n],1
		inc	byte [kind]
		cmp	byte [kind],N_KINDS
		jb	.layout
		mov	byte [kind],0
		mov	byte [auto],0
		jmp	.layout
.keys:		test	al,K_NEXT
		jnz	.done
		test	al,K_UP | K_DOWN
		jz	.n
		inc	byte [kind]
		cmp	byte [kind],N_KINDS
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
.done:		mov	byte [auto],0
		call	none
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

; Standing: sprite i at dot 8 + spacing x (N - 1 - i) (spacing (304 -
; width) / N, at most 16, even: the sprites inside the screen, each
; showing at least its left spacing dots), its marks at lines BAND - 6 to
; - 3 and BAND + 18 to + 21.
layout:		call	none
		cmp	byte [moving],0
		jne	move_step
		cmp	byte [marks],0		; G1's lines BAND - 8 to + 23 cleared
		je	.clear
		mov	dx,0
		mov	bx,BAND - 8
		mov	cx,LINE_BYTES
		mov	si,32
		xor	al,al
		call	g1_fill
.clear:		cmp	byte [marks],0		; the index ruler's text line
		je	.noruler
		mov	si,s_empty
		mov	bx,(BAND + 24) / 8
		call	text_line
.noruler:
		mov	bl,[kind]
		xor	bh,bh
		shl	bx,1
		mov	ax,304
		sub	ax,[kind_width + bx]
		xor	dx,dx
		div	word [n]
		cmp	ax,16
		jbe	.s
		mov	ax,16
.s:		and	al,0feh
		mov	[space],ax
		mov	ax,[n]			; the first (in front) on the right
		dec	ax
		mul	word [space]
		add	ax,8
		mov	dx,ax
		xor	bx,bx
.spr:		call	colour_of
		mov	ax,[band]
		call	sprite
		cmp	byte [marks],0
		je	.nomark
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
		mov	ax,bx			; every fifth sprite: its number
		push	dx			; under its left edge
		xor	dx,dx
		mov	cx,5
		div	cx
		mov	ax,dx			; (the remainder)
		pop	dx
		or	ax,ax
		jnz	.nomark
		push	bx
		push	dx
		mov	ax,bx
		mov	cl,10
		div	cl
		add	ax,'00'
		mov	[ruler],ax
		mov	byte [colour],WHITE
		mov	si,ruler
		mov	di,dx
		shr	di,1
		add	di,(BAND + 24) * LINE_BYTES
		call	text_at
		pop	dx
		pop	bx
.nomark:	sub	dx,[space]
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
; 5 MEASURE: every row [passes] times (8, or TSPSPR P), the rows in turn
; in each pass, a row's value the blocks done in FRAMES VRTCs (more is
; faster) unless said otherwise; the table gives each row's mean, minimum,
; maximum and standard deviation (tenths) so far. Each row sets up its
; sprites first (layout on the band: OFF SPROFF, 31 or 8 C32, 31 M32, 31
; C64). Rows U-X, with the BIOS's 200-line SYNC, come after the others'
; passes; the SYNC chosen before is restored at the end.

F_VB		equ	1		; the value: [vb_chunks]
F_ONCE		equ	2		; called once, the value in AX
F_TEXT		equ	4		; with the BIOS's 200-line SYNC
N_TESTS		equ	24
tvram_screen:	mov	si,s_tvram
		mov	dx,s_tvram_help
		call	screen
		mov	si,s_tvram_head
		mov	byte [colour],YELLOW
		mov	bx,1
		call	text_line
		mov	word [band],LINES - 16	; the sprites under rows 26-27,
		mov	byte [marks],0		; clear of the table
		mov	di,ZEROS		; the source of the word rows
		xor	ax,ax
		mov	cx,512
		rep	stosw
		mov	di,st_count		; the statistics
		mov	cx,ST_WORDS
		rep	stosw
		xor	bx,bx			; the rows' names
.name:		push	bx
		call	show_row
		pop	bx
		add	bx,2
		cmp	bx,N_TESTS * 2
		jb	.name
		mov	byte [phase],0		; 0 the rows without text, 1 with
.phase:		mov	word [pass],0
.pass:		inc	word [pass]
		call	show_pass
		xor	bx,bx
.test:		mov	al,[test_flags + bx]
		and	al,F_TEXT
		jz	.notext
		mov	al,1
.notext:	cmp	al,[phase]
		jne	.next
		push	bx
		call	measure			; AX: the value of row BX
		pop	bx
		push	bx
		call	add_value
		call	show_row
		pop	bx
.next:		add	bx,2
		cmp	bx,N_TESTS * 2
		jb	.test
		mov	ax,[pass]
		cmp	ax,[passes]
		jb	.pass
		cmp	byte [phase],0
		jne	.end
		mov	byte [phase],1		; the last rows with the BIOS's
		mov	al,[sync200]		; 200-line SYNC
		mov	[sync_saved],al
		or	al,al
		jnz	.phase
		call	toggle_sync
		jmp	.phase
.end:		cmp	byte [sync_saved],0	; the SYNC chosen before
		jne	.kept
		call	toggle_sync
.kept:		mov	si,s_tvram_done
		mov	byte [colour],RED
		xor	bx,bx
		call	text_line
		call	show_model
		call	spron
.frame:		call	frame
		test	al,K_NEXT
		jz	.frame
		mov	word [band],BAND
		mov	byte [marks],1
		call	none
		jmp	flush

; Row BX: its sprites, then its value in AX.
measure:	push	bx
		mov	al,[test_spr + bx]
		call	setup_sprites
		pop	bx
		mov	word [vb_chunks],0
		test	byte [test_flags + bx],F_ONCE
		jz	.run
		jmp	[tests + bx]
.run:		push	bx
		mov	bx,[tests + bx]
		call	run
		pop	bx
		test	byte [test_flags + bx],F_VB
		jz	.r
		mov	ax,[vb_chunks]
.r:		ret

; The sprites of a row: AL 0 none (SPROFF), else an index of spr_n and
; spr_kind, standing on the band.
setup_sprites:	or	al,al
		jnz	.on
		call	none
		call	flush
		mov	al,83h
		jmp	tsp_cmd
.on:		xor	ah,ah
		mov	bx,ax
		mov	al,[spr_n + bx]
		mov	[n],ax
		mov	al,[spr_kind + bx]
		mov	[kind],al
		mov	byte [moving],0
		call	layout
		jmp	spron

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
		mov	si,s_tvram
		call	put_str
		mov	si,s_pass
		call	put_str
		mov	ax,[pass]
		call	put_dec
		mov	si,s_of
		call	put_str
		mov	ax,[passes]
		call	put_dec
		mov	si,s_text_phase
		cmp	byte [phase],0
		jne	.t
		mov	si,s_empty
.t:		call	put_str
		mov	byte [colour],RED
		xor	bx,bx
		call	put_line
		jmp	show_model

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


%macro MOVSW1K 1			; segment: 512 words from ZEROS
		push	es
		mov	es,%1
		xor	di,di
		mov	si,ZEROS
		mov	cx,512
		rep	movsw
		pop	es
%endmacro
t_tv_movsw:	call	map_tvram
		MOVSW1K	[cs:tv_seg]
		jmp	map_gvram
t_g1_movsw:	MOVSW1K	[cs:g1_seg]
		ret

; The next VB, then 128-byte chunks (REP MOVSW) while it lasts, counted
; in vb_chunks (interrupts disabled meanwhile).
%macro VB_CHUNKS 1			; segment
		push	es
		mov	es,%1
		mov	dx,0142h
%%wait:		in	al,dx
		test	al,40h
		jz	%%wait
%%chunk:	xor	di,di
		mov	si,ZEROS
		mov	cx,64
		rep	movsw
		inc	word [vb_chunks]
		in	al,dx
		test	al,40h
		jnz	%%chunk
		pop	es
%endmacro
t_tv_vb:	call	map_tvram
		VB_CHUNKS [cs:tv_seg]
		jmp	map_gvram
t_g1_vb:	cli
		VB_CHUNKS [cs:g1_seg]
		sti
		ret

t_cpu:		mov	cx,1024			; ADD, DEC, JNZ (the CPU's clock)
.l:		add	ax,bx
		dec	cx
		jnz	.l
		ret
t_ram_movsw:	push	es			; main RAM to main RAM
		push	cs
		pop	es
		mov	si,ZEROS
		mov	di,RAMBUF
		mov	cx,512
		rep	movsw
		pop	es
		ret
t_tv_attr:	call	map_tvram		; the attribute side
		MOVSW1K	[cs:tv_attr_seg]
		jmp	map_gvram
t_tv_rdw:	call	map_tvram		; TVRAM to main RAM
		push	ds
		push	es
		push	cs
		pop	es
		mov	ds,[cs:tv_seg]
		xor	si,si
		mov	di,RAMBUF
		mov	cx,512
		rep	movsw
		pop	es
		pop	ds
		jmp	map_gvram
t_tv_stosw:	call	map_tvram
		push	es
		mov	es,[cs:tv_seg]
		xor	di,di
		xor	ax,ax
		mov	cx,512
		rep	stosw
		pop	es
		jmp	map_gvram
t_spron:	jmp	spron			; one SPRON (4 bytes to the TSP)

; AX = the vertical blank's share of a frame in thousandths, the mean of
; VB_FRAMES frames: status polls with VB set against all polls from one
; VB's start to the next (interrupts disabled).
VB_FRAMES	equ	60
t_vb_permille:	cli
		mov	dx,0142h
		mov	word [vb_sum],0
		mov	cx,VB_FRAMES
.w0:		in	al,dx			; the start of a VB
		test	al,40h
		jnz	.w0
.w1:		in	al,dx
		test	al,40h
		jz	.w1
.frame:		xor	si,si			; SI: polls in VB
		xor	di,di			; DI: polls outside
.in:		inc	si
		in	al,dx
		test	al,40h
		jnz	.in
.out:		inc	di
		in	al,dx
		test	al,40h
		jz	.out
		mov	ax,si			; 1000 SI / (SI + DI)
		mov	bx,1000
		mul	bx
		add	di,si
		div	di
		add	[vb_sum],ax
		mov	dx,0142h		; (MUL and DIV took DX)
		loop	.frame
		sti
		mov	ax,[vb_sum]
		xor	dx,dx
		mov	bx,VB_FRAMES
		div	bx
		ret

; ---------------------------------------------------------------------------
; Screen 5's rows: function, flags, sprites (0 off, 1 31 C32, 2 8 C32,
; 3 31 M32, 4 31 C64), name.
tests		dw	t_cpu, t_ram_movsw, t_g1_write, t_g1_movsw
		dw	t_tv_write, t_tv_write, t_tv_read, t_tv_read
		dw	t_tv_movsw, t_tv_movsw, t_tv_attr, t_tv_rdw, t_tv_stosw
		dw	t_tv_movsw, t_tv_movsw, t_tv_movsw
		dw	t_tv_vb, t_g1_vb, t_vb_permille, t_spron
		dw	t_tv_movsw, t_tv_write, t_tv_vb, t_vb_permille
test_flags	dw	0, 0, 0, 0
		dw	0, 0, 0, 0
		dw	0, 0, 0, 0, 0
		dw	0, 0, 0
		dw	F_VB, F_VB, F_ONCE, 0
		dw	F_TEXT, F_TEXT, F_TEXT | F_VB, F_TEXT | F_ONCE
test_spr	dw	0, 0, 0, 0
		dw	0, 1, 0, 1
		dw	0, 1, 1, 1, 1
		dw	2, 3, 4
		dw	1, 1, 1, 1
		dw	1, 1, 1, 1
		align	2
test_names	dw	s_ta, s_tb, s_tc, s_td, s_te, s_tf, s_tg, s_th, s_ti, s_tj
		dw	s_tk, s_tl, s_tm, s_tn, s_to, s_tp, s_tq, s_tr, s_ts, s_tt
		dw	s_tu, s_tv, s_tw, s_tx
spr_n		db	0, MAX, 8, MAX, MAX
spr_kind	db	0, 0, 0, 2, 3
orders		dw	0b9ah, 0ba9h, 09bah
order_names	dw	s_order1, s_order2, s_order3
N_KINDS		equ	6
kind_names	dw	s_c32, s_c16, s_m32, s_c64, s_m64, s_m128
kind_width	dw	16, 8, 16, 32, 32, 64	; dots
pos_dot		dw	152, 0, 304, 0, 304, 80, 224, -8, 312, 152, 240, 272
pos_line	dw	120, TOP, TOP, 208, 208, 72, 72, 120, 120, 216, -8, -8
bar_line	dw	54, 118, 182

s_banner	db	'TSP SPRITES: restart the machine to leave.',13,10,'$'
s_24khz		db	'Select 15 kHz (DIP switch SW1 off) before starting.',13,10,'$'
s_memory	db	'Not enough memory (needs 3000h; load below 2000h).',13,10,'$'
s_pos		db	'TSPSPR 1 POSITION', 0
s_pos_help	db	'ARROWS MOVE THE SPRITES  S SYNC  SPACE NEXT', 0
s_order		db	'TSPSPR 2 ORDER', 0
s_order_help	db	'UP OR DOWN: ORDER  SPACE NEXT', 0
s_line		db	'TSPSPR 3 LINE', 0
s_move		db	'TSPSPR 4 MOVE', 0
s_line_help	db	'LEFT RIGHT N  UP KIND  H HSPN  A SWEEP  SPACE NEXT', 0
s_tvram		db	'TSPSPR 5 MEASURE', 0
s_tvram_help	db	0
s_tvram_done	db	'TSPSPR 5 DONE  SPACE: AGAIN FROM 1', 0
s_d		db	'D ', 0
s_l		db	' L ', 0
s_x		db	' X ', 0
s_y		db	' Y ', 0
s_k		db	'  KEY ', 0
s_s224		db	' S224', 0
s_s200		db	' S200', 0
s_va		db	'VA', 0
s_va2		db	'VA2/3', 0
s_va91		db	'VA+91', 0
s_unknown	db	'?0000', 0
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
s_c64		db	'C64', 0
s_m64		db	'M64', 0
s_m128		db	'M128', 0
s_ta		db	'A CPU ADD LOOP    ', 0
s_tb		db	'B RAM W MOVSW     ', 0
s_tc		db	'C G1 B WRITE      ', 0
s_td		db	'D G1 W MOVSW      ', 0
s_te		db	'E TV B WRITE OFF  ', 0
s_tf		db	'F TV B WRITE 31   ', 0
s_tg		db	'G TV B READ OFF   ', 0
s_th		db	'H TV B READ 31    ', 0
s_ti		db	'I TV W MOVSW OFF  ', 0
s_tj		db	'J TV W MOVSW 31   ', 0
s_tk		db	'K TV W ATTR 31    ', 0
s_tl		db	'L TV W READ 31    ', 0
s_tm		db	'M TV W STOSW 31   ', 0
s_tn		db	'N TV W MOVSW 8    ', 0
s_to		db	'O TV W MOVSW 31M32', 0
s_tp		db	'P TV W MOVSW 31C64', 0
s_tq		db	'Q TV VB 128B 31   ', 0
s_tr		db	'R G1 VB 128B      ', 0
s_ts		db	'S VB 1/1000 FRM 31', 0
s_tt		db	'T SPRON CMDS 31   ', 0
s_tu		db	'U TV W MOVSW S200 ', 0
s_tv		db	'V TV B WRITE S200 ', 0
s_tw		db	'W TV VB 128B S200 ', 0
s_tx		db	'X VB 1/1000 S200  ', 0
s_tvram_head	db	'ROW: 1 KB BLOCKS    MEAN   MIN   MAX     SD', 0
s_pass		db	'  PASS ', 0
s_of		db	' OF ', 0
s_text_phase	db	'  S200', 0
s_empty		db	0
s_over		db	' *****', 0

sync224		db	0c1h,57h,1ch,0,9fh,0,10h,0fh,25h,0,0e0h,0,2,4
sync200v	db	0c1h,57h,1ch,0,9fh,0,10h,0fh,25h,0,0c8h,0,0fh,8	; (the BIOS's)
		align	2
fb_bios		dw	4,320,224,0,0,0
frames4		dw	0,0,160,447,0,0,0,0,0,224,0,0,0,0,0,0
		dw	0,0,160,0,0,0,0,0,0,224,0,0,0,0,0,0
palette		dw	PALETTE32
tv_seg		dw	0a000h + SCRATCH / 16
tv_attr_seg	dw	0a000h + (8000h + SCRATCH) / 16
passes		dw	8
band		dw	BAND
pass		dw	0
vb_sum		dw	0
auto_cnt	dw	0
held		dw	0
model_word	dw	0
model_str	dw	s_va
t32		dw	0, 0
ruler		db	'00', 0
g1_seg		dw	GV_G1 + (LINES - 7) * LINE_BYTES / 16	; (G1's last lines)
tick		dw	0
t0		dw	0
count		dw	0
vb_chunks	dw	0
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
sync200		db	0
sync_saved	db	0
s_prev		db	0
marks		db	1
auto		db	0
phase		db	0
hspn		db	31
ov		db	0
sc		db	0
colour		db	0
ink_body	db	0
monobuf		times	64 db 0
line		times	64 db 0
		align	2
image_end:
IMAGE_TOP	equ	image_end - $$ + 100h	; offset of the end, below tab
%if IMAGE_TOP > tab
%error the program reaches its work area
%endif
