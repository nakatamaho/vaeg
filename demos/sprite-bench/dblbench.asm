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
; DBLBENCH.COM: double-buffered sprites on a PC-88VA, VA2 or VA3 (and
; vaeg): for each screen, colour depth, way and size, the most sprites
; that can be erased and drawn on a hidden page every VRTC (one frame a
; VRTC), found by halving between 0 and NMAX. A trial of N sprites runs
; TRY_FRAMES frames; each frame starts at a VRTC, does its work and must
; end before the next one, where the page it drew is shown (FB0's DSA).
; The ways:
;   CPU  two pages; the CPU erases the N sprites' rectangles, then draws
;        the N sprites (CPUn.DAT's compiled routines, as CPUBENCH's
;        COMPLD);
;   SGP  two pages; one SGP list erases (BITBLT 0005h from zeros) and
;        draws (BITBLT 0105h, sources in main RAM, as SGPBENCH's RAM);
;   MIX  three pages; the SGP erases the page shown last frame while the
;        CPU draws on the page it erased the frame before (needs room for
;        three pages: not at 65536 colours).
; Positions and characters as the other programs'; the positions do not
; move, so a frame erases and redraws the same places. 320x224 at 65536
; colours has no room for two pages and is skipped.

		cpu	186
		org	100h

		jmp	start
%include "data.inc"			; PAL4, PAL8, PAL16, FONT, SPR_OFS, SPRITES

%define PROGRAM	'DBLBENCH'
%define N_WAYS	3			; CPU, SGP, MIX
%define WHOLE_RESULT
%define FOOT_TEXT 'ERASED AND DRAWN EVERY VRTC, 30 FRAMES A TRIAL'
DAT_PARA	equ	1000h		; a .DAT file at CS + 1000h
DAT_PARAS	equ	2000h		; its room
SRC_PARA	equ	3000h		; the SGP's sources at CS + 3000h,
ZERO_OFS	equ	0b000h		; and 8 KB of zeros after them
LIST_PARA	equ	4000h		; the SGP lists, 64 KB at CS + 4000h
MEM_PARAS	equ	5000h
LIST		equ	0c000h		; (unused: the lists are at LIST_PARA)
WORK		equ	0e000h		; the SGP's work area (this segment)
NMAX		equ	500		; sprites at most
TRY_FRAMES	equ	30

%include "common.inc"
%include "sgp.inc"
%include "cpu.inc"

; ---------------------------------------------------------------------------
; A depth's CPUn.DAT and SGP sources, and the zeros. CF: CPUn.DAT not
; found.
prepare_depth:	call	load_dat
		jc	.r
		call	sgp_sources
		push	es
		mov	es,[src_seg]
		mov	di,ZERO_OFS
		xor	ax,ax
		mov	cx,1000h
		rep	stosw
		pop	es
		mov	ax,ZERO_OFS
		xor	dx,dx
		add	ax,[src_lo]
		adc	dx,[src_hi]
		mov	[zero_lo],ax
		mov	[zero_hi],dx
		mov	ax,cs			; the lists' segment, physical
		add	ax,LIST_PARA
		mov	[list_seg],ax
		mov	dx,ax
		shl	ax,4
		shr	dx,12
		mov	[list_lo],ax
		mov	[list_hi],dx
		clc
.r:		ret

; ---------------------------------------------------------------------------
; The size, the pages (their size in paragraphs, two or three of them)
; and the positions of NMAX sprites.
build_list:	call	begin_list
		mov	ax,[pitch]		; page: pitch * height bytes
		mul	word [height]
		mov	cx,4
.para:		shr	dx,1
		rcr	ax,1
		loop	.para
		mov	[page_para],ax
		mov	byte [n_pages],2
		cmp	byte [loc_i],2
		jne	.pages
		mov	byte [n_pages],3
.pages:		mov	di,pos_x
		xor	bp,bp
.pos:		call	random_xy
		mov	[di],cx
		mov	[di + NMAX * 2],dx
		add	di,2
		inc	bp
		cmp	bp,NMAX
		jb	.pos
		ret

; ---------------------------------------------------------------------------
; The most sprites at a frame a VRTC (DX = SKIPPED when the pages do not
; fit GVRAM).
measure:	xor	ax,ax
		mov	al,[n_pages]
		mul	word [page_para]
		or	dx,dx
		jnz	.skip
		cmp	ax,4000h		; 256 KB
		jbe	.fits
.skip:		xor	ax,ax
		mov	dx,SKIPPED
		ret
.fits:		call	copy_title
		mov	word [lo],0
		mov	word [hi],NMAX + 1
.halve:		mov	ax,[hi]
		sub	ax,[lo]
		cmp	ax,1
		jbe	.found
		mov	ax,[hi]
		add	ax,[lo]
		shr	ax,1
		push	ax
		call	trial
		pop	ax
		jc	.fail
		mov	[lo],ax
		jmp	.halve
.fail:		mov	[hi],ax
		jmp	.halve
.found:		xor	ax,ax			; page 0 shown
		call	show_page
		call	clear_pages
		mov	ax,[lo]
		xor	dx,dx
		ret

; The title (lines 0 to TOP - 1 of page 0) copied to the other pages.
copy_title:	mov	ax,[pitch]
		mov	cx,TOP
		mul	cx
		shr	ax,1
		mov	[title_words],ax
		mov	bx,[page_para]
		mov	cl,[n_pages]
		dec	cl
.page:		push	cx
		push	ds
		push	es
		mov	ax,0a000h
		add	ax,bx
		mov	es,ax
		mov	ax,0a000h
		mov	ds,ax
		xor	si,si
		xor	di,di
		mov	cx,[cs:title_words]
		rep	movsw
		pop	es
		pop	ds
		pop	cx
		add	bx,[page_para]
		dec	cl
		jnz	.page
		ret

; Every page cleared below the title.
clear_pages:	mov	ax,[pitch]		; the title's paragraphs
		mov	cx,TOP
		mul	cx
		shr	ax,4
		mov	bx,ax
		mov	ax,[height]		; bytes below it
		sub	ax,TOP
		mul	word [pitch]
		mov	[clr_lo],ax
		mov	[clr_hi],dx
		mov	cl,[n_pages]
		xor	si,si			; the page's paragraph
.page:		push	cx
		push	es
		mov	ax,0a000h
		add	ax,si
		add	ax,bx
		mov	es,ax
		mov	ax,[clr_lo]
		mov	dx,[clr_hi]
.chunk:		or	dx,dx			; 32 KB pieces
		jnz	.big
		cmp	ax,8000h
		jb	.last
.big:		push	ax
		xor	ax,ax
		xor	di,di
		mov	cx,4000h
		rep	stosw
		pop	ax
		sub	ax,8000h
		sbb	dx,0
		mov	cx,es
		add	cx,800h
		mov	es,cx
		jmp	.chunk
.last:		mov	cx,ax
		shr	cx,1
		xor	ax,ax
		xor	di,di
		rep	stosw
		pop	es
		pop	cx
		add	si,[page_para]
		dec	cl
		jnz	.page
		ret

; DSA of FB0 = page AX's start.
show_page:	mul	word [page_para]	; paragraphs to bytes
		mov	cx,4
.sh:		shl	ax,1
		rcl	dx,1
		loop	.sh
		push	dx
		mov	dx,020eh
		out	dx,ax
		pop	ax
		mov	dx,0210h
		out	dx,ax
		ret

; ---------------------------------------------------------------------------
; A trial of AX sprites: the CPU's table, the SGP's lists, the pages
; cleared, TRY_FRAMES frames. CF: a frame did not end inside its VRTC.
trial:		mov	[n_spr],ax
		mov	di,sprite_tab		; the CPU's table, page 0
		xor	bp,bp
.entry:		cmp	bp,[n_spr]
		jae	.lists
		mov	bx,bp
		shl	bx,1
		mov	cx,[pos_x + bx]
		mov	dx,[pos_x + NMAX * 2 + bx]
		call	cpu_entry
		inc	bp
		jmp	.entry
.lists:		call	sgp_pitch
		mov	byte [sgp_vram],0
		push	es
		mov	es,[list_seg]
		xor	cx,cx			; page
.list:		push	cx
		mov	ax,cx			; its start
		mul	word [page_para]
		mov	bx,4
.sh:		shl	ax,1
		rcl	dx,1
		dec	bx
		jnz	.sh
		mov	[page_lo],ax
		mov	[page_hi],dx
		mov	ax,cx			; the list: SGP at 8000h a page,
		mov	cl,15			; MIX at 4000h
		cmp	byte [loc_i],2
		jne	.at
		mov	cl,14
.at:		shl	ax,cl
		mov	di,ax
		call	sgp_work
		mov	byte [sgp_erase],1
		call	sgp_all
		cmp	byte [loc_i],2
		je	.end
		mov	byte [sgp_erase],0
		call	sgp_all
.end:		call	sgp_end
		pop	cx
		inc	cl
		cmp	cl,[n_pages]
		jb	.list
		pop	es
		mov	byte [sgp_erase],0
		xor	ax,ax
		mov	[page_lo],ax
		mov	[page_hi],ax
		call	clear_pages
		xor	ax,ax
		mov	[shown],al		; pages: shown 0, drawn 1, erased 2
		inc	ax
		mov	[drawn_p],al
		inc	ax
		mov	[erased_p],al
		mov	word [frames],TRY_FRAMES
		call	vrtc_on
.frame:		mov	ax,[ticks]
		mov	[t_frame],ax
		mov	al,[loc_i]
		cmp	al,1
		je	.sgp
		jb	.cpu
		call	mix_frame
		jmp	.done
.sgp:		call	sgp_frame
		jmp	.done
.cpu:		call	cpu_frame
.done:		mov	ax,[ticks]
		cmp	ax,[t_frame]
		jne	.late
.wait:		cmp	ax,[ticks]		; the next VRTC
		je	.wait
		xor	ax,ax			; the page drawn shown
		mov	al,[drawn_p]
		call	show_page
		mov	al,[shown]		; turned
		mov	ah,[drawn_p]
		cmp	byte [n_pages],3
		je	.three
		mov	[drawn_p],al
		mov	[shown],ah
		jmp	.turned
.three:		mov	bl,[erased_p]
		mov	[shown],ah
		mov	[drawn_p],bl
		mov	[erased_p],al
.turned:	dec	word [frames]
		jnz	.frame
		call	vrtc_off
		clc
		ret
.late:		call	sgp_wait
		call	vrtc_off
		stc
		ret

; The N sprites at ES:DI from pos_x (erasing or drawing as sgp_erase).
sgp_all:	xor	bp,bp
.s:		cmp	bp,[n_spr]
		jae	.r
		mov	bx,bp
		shl	bx,1
		mov	cx,[pos_x + bx]
		mov	dx,[pos_x + NMAX * 2 + bx]
		call	sgp_sprite
		inc	bp
		jmp	.s
.r:		ret

; The page drawn's paragraph (from A000h) in AX.
drawn_para:	xor	ax,ax
		mov	al,[drawn_p]
		mul	word [page_para]
		ret

; CPU: the N rectangles erased, then the N sprites drawn.
cpu_frame:	call	drawn_para
		mov	[pp],ax
		mov	si,sprite_tab
		mov	cx,[n_spr]
		jcxz	.r
.erase:		push	cx
		push	si
		mov	ax,[si]
		add	ax,[pp]
		mov	es,ax
		mov	di,[si + 2]
		mov	bx,[si + 10]		; bytes a line
		cmp	byte [code],3
		jne	.bytes
		shl	bx,1
.bytes:		mov	bp,[pitch]
		sub	bp,bx
		mov	dx,[size]
		xor	ax,ax
.line:		mov	cx,bx
		shr	cx,1
		rep	stosw
		jnc	.even
		stosb
.even:		add	di,bp
		dec	dx
		jnz	.line
		pop	si
		pop	cx
		add	si,ENTRY
		loop	.erase
		push	cs
		pop	es
		call	cpu_draw
.r:		ret

; The N sprites drawn on page [pp].
cpu_draw:	mov	si,sprite_tab
		mov	cx,[n_spr]
		jcxz	.r
.sprite:	push	cx
		push	si
		mov	di,[si + 2]
		mov	ax,[si]
		add	ax,[pp]
		push	ds
		mov	ds,ax
		call	far [cs:si + 4]
		pop	ds
		pop	si
		pop	cx
		add	si,ENTRY
		loop	.sprite
.r:		ret

; SGP: the page drawn's list (erase, then draw), waited for.
sgp_frame:	xor	ax,ax
		mov	ah,[drawn_p]
		shl	ah,7			; * 8000h
		xor	dx,dx
		add	ax,[list_lo]
		adc	dx,[list_hi]
		call	submit_at
		jmp	sgp_wait

; MIX: the SGP erases the page erased while the CPU draws the page drawn.
mix_frame:	xor	ax,ax
		mov	ah,[erased_p]
		shl	ah,6			; * 4000h
		xor	dx,dx
		add	ax,[list_lo]
		adc	dx,[list_hi]
		call	submit_at
		call	drawn_para
		mov	[pp],ax
		call	cpu_draw
		jmp	sgp_wait

; ---------------------------------------------------------------------------
s_banner	db	'DBLBENCH: double-buffered sprites, about 5 minutes.',13,10
		db	'ESC stops.',13,10,'$'
s_memory	db	'Not enough memory (needs 320 KB after the program).',13,10,'$'
s_head2		db	'MOST SPRITES AT A FRAME A VRTC, DOUBLE BUFFER', 0
s_head3		db	'SCREEN  COL.  WAY       8X8  16X16  32X32  64X64', 0
s_loc0		db	'CPU', 0
s_loc1		db	'SGP', 0
s_loc2		db	'MIX', 0
		align	2
list_seg	dw	0
list_lo		dw	0
list_hi		dw	0
page_para	dw	0
title_words	dw	0
clr_lo		dw	0
clr_hi		dw	0
lo		dw	0
hi		dw	0
n_spr		dw	0
frames		dw	0
t_frame		dw	0
pp		dw	0
n_pages		db	0
shown		db	0
drawn_p		db	0
erased_p	db	0
		align	2
pos_x		times	NMAX * 2 dw 0	; x, then y
sprite_tab	times	NMAX * ENTRY db 0
image_end:
IMAGE_TOP	equ	image_end - $$ + 100h
%if IMAGE_TOP > WORK
%error the program reaches the SGP's work area
%endif
