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
DAT_PARAS	equ	2000h		; room for a .DAT file, at CS + 1000h
MEM_PARAS	equ	1000h + DAT_PARAS
ENTRY		equ	12		; a sprite: GVRAM segment and offset,
					; code offset (0) and segment, data
					; segment, width (bytes, dots or words)

%include "common.inc"

; ---------------------------------------------------------------------------
; CPUn.DAT read to CS + 1000h; its header (32 entries: code and data
; paragraphs, from the file's start) made segments. CF: not found.
prepare_depth:	mov	ax,cs
		add	ax,1000h
		mov	[dat_seg],ax
		xor	bx,bx
		mov	bl,[depth_i]
		shl	bx,1
		mov	si,[dat_names + bx]			; the name, 0-terminated
		mov	di,file_name
.copy:		lodsb
		cmp	al,'$'
		je	.named
		stosb
		jmp	.copy
.named:		mov	byte [di],0
		mov	dx,file_name
		mov	ax,3d00h		; open, read
		int	21h
		jc	.fail
		mov	bx,ax
		mov	cx,DAT_PARAS / 800h	; 32 KB blocks
		mov	dx,[dat_seg]
.read:		push	cx
		push	dx
		push	ds
		mov	ds,dx
		xor	dx,dx
		mov	cx,8000h
		mov	ah,3fh
		int	21h
		pop	ds
		pop	dx
		pop	cx
		jc	.close_fail
		cmp	ax,8000h
		jb	.close
		add	dx,800h
		loop	.read
.close:		mov	ah,3eh
		int	21h
		push	es
		mov	es,[dat_seg]
		xor	si,si
		xor	di,di
		mov	cx,32
.entry:		mov	ax,[es:si]
		add	ax,[dat_seg]
		mov	[code_segs + di],ax
		mov	ax,[es:si + 2]
		add	ax,[dat_seg]
		mov	[data_segs + di],ax
		add	si,4
		add	di,2
		loop	.entry
		pop	es
		clc
		ret
.close_fail:	mov	ah,3eh
		int	21h
.fail:		mov	word [fail_msg],s_nofile
		stc
		ret

; ---------------------------------------------------------------------------
; The sprites' table: for each, its GVRAM place (segment A000h + address /
; 16, the rest in the offset), its routine and data (image size_i * 4 +
; kind, parity at 4 bpp) and its width. Positions as SGPBENCH's.
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
		mov	di,sprite_tab
		xor	bp,bp
.sprite:	call	random_xy		; CX x, DX y
		mov	ax,dx			; DX:AX = y * pitch
		mul	word [pitch]
		xor	bx,bx			; BX: parity
		mov	si,cx			; SI: x in bytes
		cmp	byte [code],1
		jne	.x8
		and	bx,0			; 4 bpp: x / 2, parity x and 1
		mov	bl,cl
		and	bl,1
		shr	si,1
		jmp	.addr
.x8:		cmp	byte [code],2
		je	.addr
		shl	si,1
.addr:		add	ax,si
		adc	dx,0
		mov	si,ax			; segment and offset
		and	si,0fh
		shr	ax,4
		shl	dx,12
		or	ax,dx
		add	ax,0a000h
		mov	[di],ax
		mov	[di + 2],si
		mov	word [di + 4],0
		xor	ax,ax			; entry (size_i * 4 + kind) * 2
		mov	al,[size_i]		; + parity
		shl	ax,2
		mov	dx,bp
		and	dx,3
		add	ax,dx
		shl	ax,1
		add	ax,bx
		mov	si,ax
		shl	si,1
		mov	ax,[code_segs + si]
		mov	[di + 6],ax
		mov	ax,[data_segs + si]
		mov	[di + 8],ax
		mov	ax,[size]		; width: 4 bpp size / 2 + parity
		cmp	byte [code],1		; bytes, else size dots
		jne	.width
		shr	ax,1
		add	ax,bx
.width:		mov	[di + 10],ax
		add	di,ENTRY
		inc	bp
		cmp	bp,[count]
		jae	.end
		jmp	.sprite
.end:		mov	cl,[code]		; the line's rest in bytes
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
measure:	cli
		push	es
		xor	ax,ax
		mov	es,ax
		mov	ax,[es:0ah * 4]
		mov	[old_vrtc],ax
		mov	ax,[es:0ah * 4 + 2]
		mov	[old_vrtc + 2],ax
		mov	word [es:0ah * 4],vrtc
		mov	[es:0ah * 4 + 2],cs
		pop	es
		mov	dx,18ah
		in	al,dx
		mov	[old_mask],al
		mov	al,0fbh			; only VRTC (IR2)
		out	dx,al
		mov	word [ticks],0
		sti
.edge:		cmp	word [ticks],0
		je	.edge
		mov	ax,[ticks]
		mov	[t0],ax
		mov	word [drawn],0
		mov	word [drawn + 2],0
.list:		mov	si,sprite_tab
		mov	cx,[count]
.sprite:	push	cx
		push	si
		cmp	byte [loc_i],0
		jne	.loop
		mov	di,[si + 2]		; COMPILED: DS:DI the place
		push	ds
		mov	ds,[si]
		call	far [cs:si + 4]
		pop	ds
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
.end:		cli
		push	es
		xor	ax,ax
		mov	es,ax
		mov	ax,[old_vrtc]
		mov	[es:0ah * 4],ax
		mov	ax,[old_vrtc + 2]
		mov	[es:0ah * 4 + 2],ax
		pop	es
		mov	dx,18ah
		mov	al,[old_mask]
		out	dx,al
		sti
		push	cs
		pop	es
		mov	ax,[drawn]
		mov	dx,[drawn + 2]
		ret

vrtc:		push	ax
		push	dx
		inc	word [cs:ticks]
		mov	al,20h
		mov	dx,188h
		out	dx,al
		pop	dx
		pop	ax
		iret

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
s_nofile	db	'CPU4.DAT, CPU8.DAT or CPU16.DAT not found or not read',13,10
		db	'(run CPUBENCH where they are).',13,10,'$'
s_head2		db	'SPRITES A VRTC, 180 VRTCS A TEST, CPU', 0
s_head3		db	'SCREEN  COL.  WAY       8X8  16X16  32X32  64X64', 0
s_loc0		db	'COMPLD', 0
s_loc1		db	'LOOP', 0
dat_names	dw	s_f4, s_f8, s_f16
s_f4		db	'CPU4.DAT$'
s_f8		db	'CPU8.DAT$'
s_f16		db	'CPU16.DAT$'
		align	2
dat_seg		dw	0
span_bytes	dw	0
t0		dw	0
drawn		dd	0
old_vrtc	dd	0
code_segs	times	32 dw 0
data_segs	times	32 dw 0
old_mask	db	0
file_name	times	16 db 0
		align	2
sprite_tab	times	128 * ENTRY db 0
image_end:
IMAGE_TOP	equ	image_end - $$ + 100h
%if IMAGE_TOP > 0fff0h
%error the program is too large
%endif
