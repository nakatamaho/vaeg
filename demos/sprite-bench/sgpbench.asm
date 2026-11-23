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
; SGPBENCH.COM: SGP sprite throughput on a PC-88VA, VA2 or VA3 (and vaeg),
; for each screen (320x200 and 320x224), colour depth (16, 256 and 65536
; colours), source (main RAM and GVRAM) and sprite size (8x8, 16x16, 32x32
; and 64x64): the sprites drawn in FRAMES VRTCs by SGP lists of BITBLT
; 0105h (source colour 0 transparent) at fixed scattered positions. The
; characters are generated (gen_data.py). One screen, G0 only, single
; plane. 320x224 needs a 15 kHz display and is skipped at 24.8 kHz. ESC
; between tests stops. At the end the BIOS screen is restored and the
; table is printed through DOS (SGPBENCH > RESULT.TXT keeps it).

		cpu	186
		org	100h

		jmp	start
%include "data.inc"			; PAL4, PAL8, PAL16, FONT, SPR_OFS, SPRITES

%define PROGRAM	'SGPBENCH'
MEM_PARAS	equ	2000h		; the sources at CS + 1000h
LIST		equ	0c000h		; SGP list and work area (this segment)
WORK		equ	0e000h
VSRC		equ	24000h		; GVRAM offset of the GVRAM sources
VSRC_SEG	equ	0a000h + VSRC / 16

%include "common.inc"

; ---------------------------------------------------------------------------
; A depth's sources.
prepare_depth:	mov	ax,cs
		add	ax,1000h
		mov	[src_seg],ax
		call	expand_sources
		clc
		ret

; ---------------------------------------------------------------------------
; SPRITES (4 bpp) to this depth at src_seg:0 (offset of an image: its 4-bpp
; offset << shift), then copied to GVRAM at VSRC.
expand_sources:	push	es
		mov	es,[src_seg]
		mov	si,SPRITES
		xor	di,di
		mov	cx,SPR_BYTES
		mov	bl,[code]
		cmp	bl,1
		jne	.wide
		rep	movsb
		jmp	.copy
.wide:		lodsb
		mov	ah,al
		shr	al,4			; the left dot first
		call	.dot
		mov	al,ah
		and	al,0fh
		call	.dot
		loop	.wide
.copy:		push	ds
		mov	cx,di			; bytes
		shr	cx,1
		mov	ds,[src_seg]
		mov	ax,VSRC_SEG
		mov	es,ax
		xor	si,si
		xor	di,di
		rep	movsw
		pop	ds
		pop	es
		ret
.dot:		push	bx
		xor	bh,bh
		mov	bl,al
		cmp	byte [code],2
		jne	.d16
		mov	al,[PAL8 + bx]
		stosb
		pop	bx
		ret
.d16:		shl	bx,1
		mov	dx,[PAL16 + bx]
		push	ax
		mov	ax,dx
		stosw
		pop	ax
		pop	bx
		ret

; ---------------------------------------------------------------------------
; The list at LIST: SET_WORK, then for each sprite SET_SOURCE, SET_DEST and
; BITBLT 0105h, END. Positions from a fixed generator per size, the same at
; every depth and source; kinds in turn.
build_list:	call	slot_index
		xor	bx,bx
		mov	bl,[size_i]
		mov	cl,bl
		mov	ax,8
		shl	ax,cl
		mov	[size],ax
		shl	bx,1
		mov	ax,[count_tab + bx]
		mov	[count],ax
		mov	ax,[seed_tab + bx]
		mov	[seed],ax
		mov	ax,[size]		; source pitch: size * bpp / 8
		xor	cx,cx
		mov	cl,[bpp]
		mul	cx
		shr	ax,3
		mov	[src_pitch],ax
		mov	di,LIST
		mov	ax,3			; SET_WORK
		stosw
		mov	ax,WORK
		xor	dx,dx
		call	cs_phys
		stosw
		mov	ax,dx
		stosw
		xor	bp,bp			; sprite
.sprite:	mov	ax,4			; SET_SOURCE
		stosw
		xor	ax,ax
		mov	al,[code]
		stosw
		mov	ax,[size]
		stosw
		stosw
		mov	ax,[src_pitch]
		stosw
		xor	bx,bx			; image: size_i * 4 + kind
		mov	bl,[size_i]
		shl	bx,2
		mov	ax,bp
		and	ax,3
		add	bx,ax
		shl	bx,1
		mov	ax,[SPR_OFS + bx]
		xor	dx,dx
		mov	cl,[shift]
		or	cl,cl
		jz	.shifted
.sh:		shl	ax,1
		rcl	dx,1
		dec	cl
		jnz	.sh
.shifted:	cmp	byte [loc_i],0
		jne	.vram
		add	ax,[cs_lo]		; main RAM: CS + 1000h
		adc	dx,[cs_hi]
		inc	dx
		jmp	.src
.vram:		add	ax,VSRC & 0ffffh	; GVRAM: SGP 200000h + VSRC
		adc	dx,20h + (VSRC >> 16)
.src:		stosw
		mov	ax,dx
		stosw
		call	random_xy		; CX x, DX y
		mov	ax,5			; SET_DEST
		stosw
		mov	al,cl			; start dot: x mod dots a word
		mov	ah,1
		push	cx
		mov	cl,[dshift]
		shl	ah,cl
		pop	cx
		dec	ah
		and	al,ah
		shl	al,4
		or	al,[code]
		xor	ah,ah
		stosw
		mov	ax,[size]
		stosw
		stosw
		mov	ax,[pitch]
		stosw
		mov	ax,dx			; y * pitch + (x >> dshift) * 2
		mul	word [pitch]
		push	cx
		mov	bx,cx
		mov	cl,[dshift]
		shr	bx,cl
		shl	bx,1
		pop	cx
		add	ax,bx
		adc	dx,20h			; SGP 200000h: GVRAM
		stosw
		mov	ax,dx
		stosw
		mov	ax,7			; BITBLT: copy, source 0 transparent
		stosw
		mov	ax,0105h
		stosw
		inc	bp
		cmp	bp,[count]
		jae	.end
		jmp	.sprite
.end:		mov	ax,1			; END
		stosw
		ret

; DX:AX + CS physical.
cs_phys:	add	ax,[cs_lo]
		adc	dx,[cs_hi]
		ret

; ---------------------------------------------------------------------------
; Lists for FRAMES VRTCs (TSP status bit 6 polled, interrupts off): DX:AX
; sprites of the lists finished.
measure:	cli
		mov	word [lists],0
		mov	word [ticks],0
		mov	dx,0142h		; a VRTC's start
		mov	cx,0ffffh
.out1:		in	al,dx
		test	al,40h
		jz	.in0
		loop	.out1
.in0:		mov	cx,0ffffh
.in1:		in	al,dx
		test	al,40h
		jnz	.start
		loop	.in1
.start:		mov	byte [prev],40h
		call	submit
.poll:		mov	dx,0506h
		in	al,dx
		test	al,1
		jnz	.vrtc
		inc	word [lists]
		call	submit
.vrtc:		mov	dx,0142h
		in	al,dx
		and	al,40h
		cmp	al,[prev]
		je	.poll
		mov	[prev],al
		or	al,al
		jz	.poll
		inc	word [ticks]
		cmp	word [ticks],FRAMES
		jb	.poll
		call	sgp_wait		; the last list, not counted
		sti
		mov	ax,[lists]
		mul	word [count]
		ret

; The list at LIST started (SGP address CS:LIST).
submit:		mov	dx,0580h
		mov	al,10h
		out	dx,al
		mov	ax,LIST
		xor	dx,dx
		call	cs_phys
		push	dx
		mov	dx,0500h
		out	dx,ax
		pop	ax
		mov	dx,0502h
		out	dx,ax
		mov	dx,0504h
		xor	al,al
		out	dx,al
		mov	dx,0506h
		mov	al,1
		out	dx,al
		ret
sgp_wait:	mov	dx,0506h
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
.ready:		ret


; ---------------------------------------------------------------------------
s_banner	db	'SGPBENCH: SGP sprites, about 3 minutes. ESC stops.',13,10,'$'
s_memory	db	'Not enough memory (needs 128 KB after the program).',13,10,'$'
s_head2		db	'SPRITES A VRTC, 180 VRTCS A TEST, BITBLT 0105H', 0
s_head3		db	'SCREEN  COL.  SOURCE    8X8  16X16  32X32  64X64', 0
s_loc0		db	'RAM', 0
s_loc1		db	'GVRAM', 0
		align	2
src_seg		dw	0
src_pitch	dw	0
lists		dw	0
prev		db	0
		align	2
image_end:
IMAGE_TOP	equ	image_end - $$ + 100h
%if IMAGE_TOP > LIST
%error the program reaches its SGP list
%endif
