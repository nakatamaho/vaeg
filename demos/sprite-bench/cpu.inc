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
; cpu.inc: the CPU's part of CPUBENCH.COM and MIXBENCH.COM (README.md): a
; depth's CPUn.DAT read, a sprite's table entry, a compiled sprite drawn,
; and the VRTC interrupt that counts VRTCs. The program defines DAT_PARA
; and DAT_PARAS (where the file goes, from CS, and its room).

ENTRY		equ	12		; a sprite: GVRAM segment and offset,
					; code offset (0) and segment, data
					; segment, width (bytes, dots or words)

; ---------------------------------------------------------------------------
; CPUn.DAT read to CS + DAT_PARA; its header (32 entries: code and data
; paragraphs, from the file's start) made segments. CF: not found.
load_dat:	mov	ax,cs
		add	ax,DAT_PARA
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
; Sprite BP (its kind BP and 3) at dot CX, line DX: its entry at DI (DI
; to the next; BP kept): its GVRAM place (segment A000h + address / 16,
; the rest in the offset), its routine and data (image size_i * 4 + kind,
; parity at 4 bpp) and its width. After begin_list.
cpu_entry:	mov	ax,dx			; DX:AX = y * pitch
		mul	word [pitch]
		xor	bx,bx			; BX: parity
		mov	si,cx			; SI: x in bytes
		cmp	byte [code],1
		jne	.x8
		mov	bl,cl			; 4 bpp: x / 2, parity x and 1
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
		ret

; The compiled sprite of the entry at SI (registers but SI and DS lost);
; inline, as a game would call it.
%macro DRAW_COMPILED 0
		mov	di,[si + 2]		; DS:DI the place
		push	ds
		mov	ds,[si]
		call	far [cs:si + 4]
		pop	ds
%endmacro

; ---------------------------------------------------------------------------
; The VRTC interrupt counts ticks; only it is unmasked between vrtc_on
; and vrtc_off. vrtc_on returns at a VRTC's start, with t0 its tick.
vrtc_on:	cli
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
		ret

vrtc_off:	cli
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

s_nofile	db	'CPU4.DAT, CPU8.DAT or CPU16.DAT not found or not read',13,10
		db	'(run ', PROGRAM, ' where they are).',13,10,'$'
dat_names	dw	s_f4, s_f8, s_f16
s_f4		db	'CPU4.DAT$'
s_f8		db	'CPU8.DAT$'
s_f16		db	'CPU16.DAT$'
		align	2
dat_seg		dw	0
t0		dw	0
old_vrtc	dd	0
code_segs	times	32 dw 0
data_segs	times	32 dw 0
old_mask	db	0
file_name	times	16 db 0
		align	2
