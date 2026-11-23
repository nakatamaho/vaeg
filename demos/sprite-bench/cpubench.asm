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
; CPUBENCH.COM: CPU sprite throughput on a PC-88VA, VA2 or VA3 (and vaeg),
; the same tests as SGPBENCH.COM (README.md) with the CPU drawing: for
; each screen (320x200 and 320x224), colour depth, way and size (8x8 to
; 64x64), the sprites drawn in FRAMES VRTCs at the same scattered
; positions. The ways: COMPILED, a far routine per image and (4 bpp) dot
; parity, as the CPU sprites of an arcade-game port for the VA (MOV of an
; immediate for opaque words, read-AND-OR-write for words with
; transparent dots, transparent ones skipped); LOOP, a plain loop over the
; image's data (4 bpp: AND with the mask, OR the dots, a byte at a time;
; 8 and 16 bpp: a dot written unless 0). Both are made by gen_data.py and
; read from CPU4.DAT, CPU8.DAT and CPU16.DAT in the current directory.
; Only the VRTC interrupt is taken during a test (it counts the VRTCs).

		cpu	186
		org	100h

		jmp	start
%include "data.inc"			; PAL4, PAL8, PAL16, FONT, SPR_OFS, SPRITES

%define PROGRAM	'CPUBENCH'
DAT_PARA	equ	1000h		; a .DAT file at CS + 1000h
DAT_PARAS	equ	2000h		; its room
MEM_PARAS	equ	DAT_PARA + DAT_PARAS

%include "common.inc"
%include "cpu.inc"

; ---------------------------------------------------------------------------
; A depth's CPUn.DAT. CF: not found.
prepare_depth:	jmp	load_dat

; ---------------------------------------------------------------------------
; The sprites' table (cpu_entry), positions as SGPBENCH's; the line's
; drawn bytes.
build_list:	call	begin_list
		mov	di,sprite_tab
		xor	bp,bp
.sprite:	call	random_xy		; CX x, DX y
		call	cpu_entry
		inc	bp
		cmp	bp,[count]
		jb	.sprite
		mov	cl,[code]		; the line's rest in bytes
		mov	ax,[size]
		cmp	cl,1
		jne	.bytes8
		shr	ax,1
.bytes8:	cmp	cl,3
		jne	.rest
		shl	ax,1
.rest:		mov	[span_bytes],ax
		ret

; ---------------------------------------------------------------------------
; The table's sprites in turn, again and again, for FRAMES VRTCs (counted
; by the VRTC interrupt, the only one unmasked): DX:AX sprites drawn.
measure:	call	vrtc_on
		mov	word [drawn],0
		mov	word [drawn + 2],0
.list:		mov	si,sprite_tab
		mov	cx,[count]
.sprite:	push	cx
		push	si
		cmp	byte [loc_i],0
		jne	.loop
		DRAW_COMPILED
		jmp	.drawn
.loop:		call	draw_loop
.drawn:		pop	si
		pop	cx
		add	word [drawn],1
		adc	word [drawn + 2],0
		mov	ax,[ticks]
		sub	ax,[t0]
		cmp	ax,FRAMES
		jae	.end
		add	si,ENTRY
		loop	.sprite
		jmp	.list
.end:		call	vrtc_off
		mov	ax,[drawn]
		mov	dx,[drawn + 2]
		ret

; LOOP: the sprite at SI (its table entry) from its data.
draw_loop:	mov	es,[si]
		mov	di,[si + 2]
		mov	bx,[si + 10]		; width
		mov	dx,[size]		; lines
		mov	bp,[pitch]
		sub	bp,[span_bytes]
		mov	al,[code]
		push	ds
		mov	ds,[si + 8]
		xor	si,si
		cmp	al,1
		je	.l4
		cmp	al,2
		je	.l8
.l16row:	mov	cx,bx
.l16:		lodsw
		or	ax,ax
		jz	.s16
		mov	[es:di],ax
.s16:		inc	di
		inc	di
		loop	.l16
		add	di,bp
		dec	dx
		jnz	.l16row
		pop	ds
		ret
.l8:		mov	cx,bx
.b8:		lodsb
		or	al,al
		jz	.s8
		mov	[es:di],al
.s8:		inc	di
		loop	.b8
		add	di,bp
		dec	dx
		jnz	.l8
		pop	ds
		ret
.l4:		sub	bp,bx			; (span_bytes is size / 2: the
		add	bp,[cs:span_bytes]	; parity's byte more)
.l4row:		mov	cx,bx
.b4:		lodsw				; AL keep mask, AH dots
		and	al,[es:di]
		or	al,ah
		stosb
		loop	.b4
		add	di,bp
		dec	dx
		jnz	.l4row
		pop	ds
		ret

; ---------------------------------------------------------------------------
s_banner	db	'CPUBENCH: CPU sprites, about 3 minutes. ESC stops.',13,10,'$'
s_memory	db	'Not enough memory (needs 192 KB after the program).',13,10,'$'
s_head2		db	'SPRITES A VRTC, 180 VRTCS A TEST, CPU', 0
s_head3		db	'SCREEN  COL.  WAY       8X8  16X16  32X32  64X64', 0
s_loc0		db	'COMPLD', 0
s_loc1		db	'LOOP', 0
		align	2
span_bytes	dw	0
drawn		dd	0
		align	2
sprite_tab	times	128 * ENTRY db 0
image_end:
IMAGE_TOP	equ	image_end - $$ + 100h
%if IMAGE_TOP > 0fff0h
%error the program is too large
%endif
