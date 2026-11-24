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
; sgp.inc: the SGP's part of SGPBENCH.COM and MIXBENCH.COM (README.md): the
; sources at this depth, the list's commands and the list started. The
; program defines SRC_PARA (the sources' segment, from CS), LIST and WORK
; (the list and the SGP's work area, offsets in this segment) and sets
; sgp_vram (0: sources in main RAM, else in GVRAM) before a list. A
; program may also set page_lo and page_hi (added to the destinations) and
; sgp_erase with zero_lo and zero_hi (BITBLT 0005h from zeros there: the
; sprite's rectangle cleared).

VSRC		equ	24000h		; GVRAM offset of the GVRAM sources
VSRC_SEG	equ	0a000h + VSRC / 16

; ---------------------------------------------------------------------------
; SPRITES (4 bpp) to this depth at src_seg:0 (offset of an image: its 4-bpp
; offset << shift), then copied to GVRAM at VSRC.
sgp_sources:	mov	ax,cs
		add	ax,SRC_PARA
		mov	[src_seg],ax
		mov	dx,ax			; physical, for SET_SOURCE
		shl	ax,4
		shr	dx,12
		mov	[src_lo],ax
		mov	[src_hi],dx
		push	es
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
; The list at LIST: sgp_begin (DI to LIST, SET_WORK), sgp_sprite for each
; sprite, sgp_end (END). After begin_list.
sgp_begin:	call	sgp_pitch
		mov	di,LIST
		jmp	sgp_work

; The source pitch: size * bpp / 8.
sgp_pitch:	mov	ax,[size]
		xor	cx,cx
		mov	cl,[bpp]
		mul	cx
		shr	ax,3
		mov	[src_pitch],ax
		ret

; SET_WORK (WORK in this segment) at ES:DI.
sgp_work:	mov	ax,3			; SET_WORK
		stosw
		mov	ax,WORK
		xor	dx,dx
		call	cs_phys
		stosw
		mov	ax,dx
		stosw
		ret

; Sprite BP (its kind BP and 3) at dot CX, line DX: SET_SOURCE, SET_DEST
; and BITBLT 0105h at DI (BP kept).
sgp_sprite:	mov	[sx],cx
		mov	[sy],dx
		mov	ax,4			; SET_SOURCE
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
.shifted:	cmp	byte [sgp_erase],0
		je	.image
		mov	ax,[zero_lo]		; erasing: the zeros
		mov	dx,[zero_hi]
		jmp	.src
.image:		cmp	byte [sgp_vram],0
		jne	.vram
		add	ax,[src_lo]		; main RAM: src_seg
		adc	dx,[src_hi]
		jmp	.src
.vram:		add	ax,VSRC & 0ffffh	; GVRAM: SGP 200000h + VSRC
		adc	dx,20h + (VSRC >> 16)
.src:		stosw
		mov	ax,dx
		stosw
		mov	cx,[sx]
		mov	dx,[sy]
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
		adc	dx,0
		add	ax,[page_lo]
		adc	dx,[page_hi]
		add	dx,20h			; SGP 200000h: GVRAM
		stosw
		mov	ax,dx
		stosw
		mov	ax,7			; BITBLT: copy, source 0 transparent
		stosw				; (erasing: 0005h, an opaque copy)
		mov	ax,0105h
		cmp	byte [sgp_erase],0
		je	.op
		mov	ax,0005h
.op:		stosw
		ret

sgp_end:	mov	ax,1			; END
		stosw
		ret

; DX:AX + CS physical.
cs_phys:	add	ax,[cs_lo]
		adc	dx,[cs_hi]
		ret

; The list at LIST started (SGP address CS:LIST).
submit:		mov	ax,LIST
		xor	dx,dx
		call	cs_phys
; The list at physical DX:AX started.
submit_at:	push	ax
		push	dx
		mov	dx,0580h
		mov	al,10h
		out	dx,al
		pop	dx
		pop	ax
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

; The SGP idle (stopped after a long timeout).
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

		align	2
src_seg		dw	0
src_lo		dw	0
src_hi		dw	0
src_pitch	dw	0
sx		dw	0
sy		dw	0
page_lo		dw	0
page_hi		dw	0
zero_lo		dw	0
zero_hi		dw	0
sgp_vram	db	0
sgp_erase	db	0
		align	2
