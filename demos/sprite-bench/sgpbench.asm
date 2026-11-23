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

FRAMES		equ	180		; VRTCs a test (about 3 s)
TOP		equ	10		; lines 0-9: the title
LIST		equ	0c000h		; SGP list and work area (this segment)
WORK		equ	0e000h
VSRC		equ	24000h		; GVRAM offset of the GVRAM sources
VSRC_SEG	equ	0a000h + VSRC / 16
WHITE		equ	2		; colour indices (gen_data.py)
YELLOW		equ	9
N_TESTS		equ	48		; 2 screens x 3 depths x 2 sources x 4 sizes
SKIPPED		equ	0ffffh		; a result's high word: not run

start:		cld
		mov	dx,s_banner
		call	dos_print
		mov	ax,cs			; memory to CS + 2000h: the sources
		add	ax,2000h		; at CS + 1000h
		cmp	[0002h],ax
		jae	.room
		mov	dx,s_memory
		call	dos_print
		mov	ax,4c01h
		int	21h
.room:		push	es			; the machine: F000:FFFE
		mov	ax,0f000h
		mov	es,ax
		mov	ax,[es:0fffeh]
		pop	es
		mov	si,s_va
		cmp	ax,0ffffh
		je	.machine
		mov	si,s_va2
		cmp	ax,0fffeh
		je	.machine
		mov	si,s_va91
		cmp	ax,0fffdh
		je	.machine
		mov	si,s_unknown
.machine:	mov	[machine],si
		mov	dx,01cbh		; port 1CBh bit 3: 24.8 kHz
		in	al,dx
		and	al,08h
		mov	[khz24],al
		mov	ax,cs			; CS physical, the sources' segment
		mov	dx,ax
		shl	ax,4
		shr	dx,12
		mov	[cs_lo],ax
		mov	[cs_hi],dx
		mov	ax,cs
		add	ax,1000h
		mov	[src_seg],ax
		call	save_video
		mov	byte [res_i],0
.res:		mov	byte [depth_i],0
.depth:		call	select_mode
		cmp	byte [res_i],1		; 224 lines at 24.8 kHz: skipped
		jne	.run
		cmp	byte [khz24],0
		je	.run
		call	mark_skipped
		jmp	.next_depth
.run:		call	set_mode
		call	expand_sources
		mov	byte [loc_i],0
.loc:		mov	byte [size_i],0
.size:		call	escape
		jnc	.go
		jmp	finish
.go:		call	run_test
		inc	byte [size_i]
		cmp	byte [size_i],4
		jb	.size
		inc	byte [loc_i]
		cmp	byte [loc_i],2
		jb	.loc
.next_depth:	inc	byte [depth_i]
		cmp	byte [depth_i],3
		jb	.depth
		inc	byte [res_i]
		cmp	byte [res_i],2
		jb	.res
		mov	byte [complete],1
finish:		mov	byte [res_i],0		; the table on a 320x200, 16-colour
		mov	byte [depth_i],0	; screen, until a key
		call	select_mode
		call	set_mode
		call	draw_table
		mov	ah,08h
		int	21h
		call	restore_video
		call	print_results
		mov	ax,4c00h
		int	21h

dos_print:	mov	ah,09h
		int	21h
		ret

; ESC typed (DOS direct console input): CF set.
escape:		mov	ah,06h
		mov	dl,0ffh
		int	21h
		jz	.no
		cmp	al,1bh
		jne	escape
		stc
		ret
.no:		clc
		ret

; ---------------------------------------------------------------------------
; The depth's and screen's values from depth_i and res_i.
select_mode:	xor	bx,bx
		mov	bl,[depth_i]
		mov	al,[bpp_tab + bx]
		mov	[bpp],al
		inc	bl
		mov	[code],bl		; SGP and GRRES: 1, 2, 3
		dec	bl
		mov	[shift],bl		; 4-bpp bytes to this depth's: << 0-2
		mov	al,2
		sub	al,bl
		mov	[dshift],al		; dots a word: 1 << (2 - shift)... x >> 2-0
		shl	bx,1
		mov	ax,[pitch_tab + bx]
		mov	[pitch],ax
		mov	ax,200
		cmp	byte [res_i],0
		je	.h
		mov	ax,224
.h:		mov	[height],ax
		ret

; The BIOS's mode and pixel size, the memory map, restored at the end.
save_video:	push	es
		mov	ax,0338h
		mov	es,ax
		mov	ax,[es:000fh]
		mov	[saved_mode],ax
		mov	ax,[es:0011h]
		mov	[saved_bpp],ax
		pop	es
		mov	dx,0153h
		in	al,dx
		mov	[saved_map],al
		ret

; One screen, G0, single plane, 320 dots, [bpp] at [height] lines; then
; the frame buffer, composition and (16 colours) the palette set
; directly, the 224-line SYNC or the 24.8 kHz line doubling, GVRAM cleared.
set_mode:	call	doubling_off
		xor	ah,ah			; ScnMode: G0 only, 320x200
		mov	bx,0a00eh
		mov	cl,[bpp]
		xor	ch,ch
		xor	dx,dx
		int	8fh
		push	cs
		pop	ds
		push	cs
		pop	es
		xor	ax,ax
		mov	al,[bpp]
		mov	[fbdesc],ax
		mov	ax,[height]
		mov	[fbdesc + 4],ax
		mov	[windesc + 4],ax
		mov	ax,0100h		; DefBuf: screen 0, one buffer
		mov	cx,1
		mov	di,fbdesc
		int	8fh
		push	cs
		pop	es
		mov	ax,0200h		; DefWin
		mov	cx,1
		mov	di,windesc
		int	8fh
		mov	ax,0b01h		; ScnDsp: on
		int	8fh
		push	cs
		pop	ds
		push	cs
		pop	es
		mov	dx,0153h		; GVRAM single plane at A0000h
		mov	al,54h
		out	dx,al
		mov	dx,0580h		; CPU data write
		mov	al,10h
		out	dx,al
		mov	dx,0102h		; GRRES: 320 dots, the depth
		mov	al,[code]
		or	al,10h
		mov	ah,al
		out	dx,ax
		mov	ax,[pitch]		; FB0: pitch, lines - 1, height
		mov	[fb0 + 4],ax
		mov	ax,[height]
		mov	[fb0 + 18],ax
		dec	ax
		mov	[fb0 + 6],ax
		mov	dx,0200h
		mov	si,fb0
		mov	cx,16
.fb:		lodsw
		out	dx,ax
		add	dx,2
		loop	.fb
		cmp	byte [code],1
		jne	.direct
		mov	dx,0106h		; palette screens: G0 only
		mov	ax,000ah
		out	dx,ax
		mov	dx,0108h
		xor	ax,ax
		out	dx,ax
		cli
		mov	cx,2			; twice: vaeg settles DAC low bits
.palette:	push	cx
		mov	dx,0300h
		mov	si,PAL4
		mov	cx,16
.entry:		lodsw
		out	dx,ax
		add	dx,2
		loop	.entry
		pop	cx
		loop	.palette
		sti
		jmp	.composed
.direct:	mov	dx,0106h		; direct colour: G0 only
		xor	ax,ax
		out	dx,ax
		mov	dx,0108h
		mov	ax,0008h
		out	dx,ax
.composed:	mov	dx,0124h		; G0 opaque
		xor	ax,ax
		out	dx,ax
		cmp	word [height],224
		jne	.lines200
		cli
		mov	si,sync224
		call	send_sync
		sti
		mov	byte [sync_sent],1
		jmp	clear_gvram
.lines200:	cmp	byte [sync_sent],0	; after 224 lines: the BIOS's SYNC
		je	.doubling
		cli
		mov	si,sync200
		call	send_sync
		sti
.doubling:	call	doubling_on
		jmp	clear_gvram

send_sync:	mov	al,10h			; SYNC, 14 bytes at SI
		call	tsp_cmd
		mov	cx,14
.b:		lodsb
		call	tsp_prm
		loop	.b
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

; At 24.8 kHz, 200-line graphics with each line on two rasters (port 100h
; RSM = 01), as demos/sgp-pseudo-sprite (M109).
doubling_on:	cmp	byte [khz24],0
		je	.r
		mov	dx,0100h
		in	ax,dx
		test	al,02h
		jz	.r
		test	al,0c0h
		jnz	.r
		or	al,40h
		out	dx,ax
		mov	byte [doubled],1
.r:		ret
doubling_off:	cmp	byte [doubled],0
		je	.r
		mov	dx,0100h
		in	ax,dx
		and	al,3fh
		out	dx,ax
		mov	byte [doubled],0
.r:		ret

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

; The BIOS screen back: the BIOS's 200-line SYNC and DSPON if the 224-line
; one was sent, RSM, the mode, the palette and the memory map.
restore_video:	push	cs
		pop	ds
		call	doubling_off
		cmp	byte [sync_sent],0
		je	.mode
		cli
		mov	si,sync200
		call	send_sync
		mov	al,12h			; DSPON (SYNC stops the text)
		call	tsp_cmd
		mov	al,7fh
		call	tsp_prm
		xor	al,al
		call	tsp_prm
		xor	al,al
		call	tsp_prm
		sti
.mode:		xor	ah,ah
		mov	bx,[saved_mode]
		mov	cx,[saved_bpp]
		xor	dx,dx
		int	8fh
		mov	ax,0a00h
		int	8fh
		push	cs
		pop	ds
		push	cs
		pop	es
		mov	dx,0153h
		mov	al,[saved_map]
		out	dx,al
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
; One test: G0 cleared, the title, the list, FRAMES VRTCs of lists, the
; result stored and shown.
run_test:	call	clear_screen
		call	title
		call	build_list
		call	measure			; DX:AX sprites
		mov	bx,[slot]
		shl	bx,2
		mov	[results + bx],ax
		mov	[results + bx + 2],dx
		call	format_rate		; s_rate: sprites a VRTC
		mov	si,s_rate
		mov	cx,320 - 8 * 6
		mov	dx,1
		mov	al,YELLOW
		jmp	draw_text

; slot = ((res * 3 + depth) * 2 + loc) * 4 + size
slot_index:	mov	al,[res_i]
		mov	ah,3
		mul	ah
		add	al,[depth_i]
		shl	al,1
		add	al,[loc_i]
		shl	al,2
		add	al,[size_i]
		xor	ah,ah
		mov	[slot],ax
		ret

mark_skipped:	mov	byte [loc_i],0
.loc:		mov	byte [size_i],0
.size:		call	slot_index
		mov	bx,[slot]
		shl	bx,2
		mov	word [results + bx],0
		mov	word [results + bx + 2],SKIPPED
		inc	byte [size_i]
		cmp	byte [size_i],4
		jb	.size
		inc	byte [loc_i]
		cmp	byte [loc_i],2
		jb	.loc
		ret

; G0's displayed lines cleared (the sources at VSRC are kept).
clear_screen:	push	es
		mov	ax,[pitch]
		mul	word [height]		; DX:AX bytes
		mov	bx,0a000h
.seg:		mov	es,bx
		xor	di,di
		or	dx,dx
		jz	.last
		push	ax
		xor	ax,ax
		mov	cx,8000h
		rep	stosw
		pop	ax
		dec	dx
		add	bx,1000h
		jmp	.seg
.last:		mov	cx,ax
		shr	cx,1
		xor	ax,ax
		rep	stosw
		pop	es
		ret

; Line 1: "320X224 256 RAM 16X16 N64 PC-88VA2"
title:		mov	di,line
		mov	si,s_320x
		call	append
		mov	ax,[height]
		call	append_dec
		mov	al,' '
		stosb
		xor	bx,bx
		mov	bl,[depth_i]
		shl	bx,1
		mov	si,[depth_names + bx]
		call	append
		mov	al,' '
		stosb
		xor	bx,bx
		mov	bl,[loc_i]
		shl	bx,1
		mov	si,[loc_names + bx]
		call	append
		mov	al,' '
		stosb
		xor	bx,bx
		mov	bl,[size_i]
		shl	bx,1
		mov	si,[size_names + bx]
		call	append
		mov	si,s_n
		call	append
		xor	bx,bx
		mov	bl,[size_i]
		shl	bx,1
		mov	ax,[count_tab + bx]
		call	append_dec
		mov	al,0
		stosb
		mov	si,line
		xor	cx,cx
		mov	dx,1
		mov	al,WHITE
		jmp	draw_text

; String SI appended at DI (without its 0).
append:		lodsb
		or	al,al
		jz	.r
		stosb
		jmp	append
.r:		ret
; AX in decimal at DI.
append_dec:	xor	cx,cx
		mov	bx,10
.div:		xor	dx,dx
		div	bx
		push	dx
		inc	cx
		or	ax,ax
		jnz	.div
.out:		pop	ax
		add	al,'0'
		stosb
		loop	.out
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

; CX = x in 0..320-size, DX = y in TOP..height-size (16-bit LCG).
random_xy:	call	random
		mov	cx,320
		sub	cx,[size]
		inc	cx
		mul	cx			; DX = rand * range >> 16
		push	dx
		call	random
		mov	cx,[height]
		sub	cx,[size]
		sub	cx,TOP - 1
		mul	cx
		add	dx,TOP
		pop	cx
		ret
random:		mov	ax,[seed]
		mov	dx,25173
		mul	dx
		add	ax,13849
		mov	[seed],ax
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

; s_rate = DX:AX * 10 / FRAMES as "nnnn.n" (sprites a VRTC).
format_rate:	push	ax
		mov	ax,dx
		mov	cx,10
		mul	cx			; high word * 10
		mov	bx,ax
		pop	ax
		mul	cx
		add	dx,bx			; DX:AX * 10
		mov	cx,FRAMES
		mov	di,s_rate
		cmp	dx,cx
		jb	.fits
		mov	si,s_over
		call	append
		mov	al,0
		stosb
		ret
.fits:		div	cx			; AX tenths (< 65536 for FRAMES 180)
		xor	dx,dx
		mov	cx,10
		div	cx
		mov	di,s_rate
		push	dx
		call	append_dec
		mov	al,'.'
		stosb
		pop	ax
		add	al,'0'
		stosb
		mov	al,0
		stosb
		ret

; ---------------------------------------------------------------------------
; Text: string SI from dot CX, line DX, colour index AL; 6x8 cells.
draw_text:	mov	[ink],al
.char:		lodsb
		or	al,al
		jz	.r
		cmp	al,'a'
		jb	.upper
		sub	al,20h
.upper:		sub	al,20h
		cmp	al,64
		jb	.glyph
		xor	al,al
.glyph:		xor	ah,ah
		mov	bx,ax
		shl	ax,3
		sub	ax,bx			; * 7
		add	ax,FONT
		mov	bx,ax
		push	si
		xor	si,si			; row
.row:		mov	ah,[bx + si]
		shl	ah,3
		push	cx
		mov	bp,5
.dot:		shl	ah,1
		jnc	.next
		push	ax
		push	dx
		add	dx,si
		mov	al,[ink]
		call	put_dot
		pop	dx
		pop	ax
.next:		inc	cx
		dec	bp
		jnz	.dot
		pop	cx
		inc	si
		cmp	si,7
		jb	.row
		pop	si
		add	cx,6
		jmp	.char
.r:		ret

; Dot CX, line DX, colour index AL, at this depth (all registers kept).
put_dot:	push	es
		push	ax
		push	bx
		push	cx
		push	dx
		push	di
		mov	[pd_x],cx
		mov	[pd_c],al
		mov	ax,dx
		mul	word [pitch]		; DX:AX line offset
		mov	bx,cx
		mov	cl,[code]
		cmp	cl,1
		jne	.not4
		shr	bx,1
		jmp	.addr
.not4:		cmp	cl,2
		je	.addr
		shl	bx,1
.addr:		add	ax,bx
		adc	dx,0
		mov	di,ax			; segment A000h + offset / 16
		and	di,0fh
		shr	ax,4
		shl	dx,12
		or	ax,dx
		add	ax,0a000h
		mov	es,ax
		xor	bh,bh
		mov	bl,[pd_c]
		cmp	cl,1
		jne	.b8
		mov	al,[es:di]		; 4 bpp: the left dot high
		test	byte [pd_x],1
		jnz	.low
		and	al,0fh
		shl	bl,4
		or	al,bl
		jmp	.st
.low:		and	al,0f0h
		or	al,bl
.st:		mov	[es:di],al
		jmp	.done
.b8:		cmp	cl,2
		jne	.b16
		mov	al,[PAL8 + bx]
		mov	[es:di],al
		jmp	.done
.b16:		shl	bx,1
		mov	ax,[PAL16 + bx]
		mov	[es:di],ax
.done:		pop	di
		pop	dx
		pop	cx
		pop	bx
		pop	ax
		pop	es
		ret

; ---------------------------------------------------------------------------
; The table: TABLE_LINES lines, each built at line by table_line (AX =
; its number), 0-terminated; drawn on G0 or printed through DOS.
TABLE_LINES	equ	17

draw_table:	xor	ax,ax
.l:		push	ax
		call	table_line
		pop	ax
		push	ax
		mov	dx,10
		mul	dx
		add	ax,4
		mov	dx,ax
		xor	cx,cx
		mov	si,line
		mov	al,WHITE
		call	draw_text
		pop	ax
		inc	ax
		cmp	ax,TABLE_LINES
		jb	.l
		ret

print_results:	mov	dx,s_crlf
		call	dos_print
		xor	ax,ax
.l:		push	ax
		call	table_line
		cmp	byte [line],0
		je	.next
		mov	al,0			; 0 to CR LF $
		mov	di,line
		mov	cx,96
		repne	scasb
		dec	di
		mov	ax,0a0dh
		stosw
		mov	al,'$'
		stosb
		mov	dx,line
		call	dos_print
.next:		pop	ax
		inc	ax
		cmp	ax,TABLE_LINES
		jb	.l
		ret

table_line:	mov	di,line
		cmp	ax,3
		jae	.body
		or	ax,ax
		jnz	.l1
		mov	si,s_head1		; SGPBENCH  machine  15 KHZ
		call	append
		mov	si,[machine]
		call	append
		mov	si,s_khz15
		cmp	byte [khz24],0
		je	.khz
		mov	si,s_khz24
.khz:		call	append
		jmp	.end
.l1:		mov	si,s_head2
		cmp	ax,1
		je	.text
		mov	si,s_head3
.text:		call	append
		jmp	.end
.body:		sub	ax,3
		cmp	ax,N_TESTS / 4
		jb	.row
		sub	ax,N_TESTS / 4
		mov	si,s_foot
		jz	.text
		mov	si,s_stopped
		cmp	byte [complete],0
		je	.text
		mov	si,s_empty
		jmp	.text
.row:		shl	ax,2			; slot of the row's first size
		push	ax
		mov	si,s_320x		; "320X200 16    RAM   "
		call	append
		pop	bx
		push	bx
		mov	ax,bx
		shr	ax,3			; res * 3 + depth
		mov	cl,3
		div	cl
		push	ax			; AH depth
		mov	bx,200
		or	al,al
		jz	.h
		mov	bx,224
.h:		mov	ax,bx
		call	append_dec
		mov	al,' '
		stosb
		pop	ax
		mov	bl,ah
		xor	bh,bh
		shl	bx,1
		mov	si,[depth_names + bx]
		call	append_pad6
		pop	bx
		push	bx
		mov	ax,bx
		shr	ax,2
		and	ax,1
		mov	si,s_ram
		jz	.loc
		mov	si,s_gvram
.loc:		call	append_pad6
		xor	cx,cx			; sizes
.col:		pop	bx
		push	bx
		add	bx,cx
		shl	bx,2
		mov	ax,[results + bx]
		mov	dx,[results + bx + 2]
		push	cx
		push	di
		mov	si,s_skip
		cmp	dx,SKIPPED
		je	.cell
		call	format_rate
		mov	si,s_rate
.cell:		pop	di
		call	append_right7
		pop	cx
		inc	cx
		cmp	cx,4
		jb	.col
		pop	bx
.end:		mov	al,0
		stosb
		ret

; SI padded with spaces to 6 at DI.
append_pad6:	mov	dx,di
		add	dx,6
		call	append
.sp:		cmp	di,dx
		jae	.r
		mov	al,' '
		stosb
		jmp	.sp
.r:		ret
; SI right-aligned in 7 at DI.
append_right7:	push	si
		xor	cx,cx
.len:		lodsb
		or	al,al
		jz	.pad
		inc	cx
		jmp	.len
.pad:		pop	si
		mov	ax,7
		sub	ax,cx
		mov	cx,ax
		mov	al,' '
		rep	stosb
		jmp	append

; ---------------------------------------------------------------------------
bpp_tab		db	4, 8, 16
		align	2
pitch_tab	dw	160, 320, 640
count_tab	dw	128, 64, 32, 16		; sprites a list: 8x8 ... 64x64
seed_tab	dw	1234, 2345, 3456, 4567
depth_names	dw	s_d16, s_d256, s_d64k
loc_names	dw	s_ram, s_gvram
size_names	dw	s_8, s_16, s_32, s_64
sync224		db	0c1h,57h,1ch,0,9fh,0,10h,0fh,25h,0,0e0h,0,2,4
sync200		db	0c1h,57h,1ch,0,9fh,0,10h,0fh,25h,0,0c8h,0,0fh,8	; the BIOS's
		align	2
fbdesc		dw	0, 320, 200
windesc		dw	0, 0, 200, 0, 0
fb0		dw	0, 0, 160, 199, 0, 0, 0, 0, 0, 200, 0, 0, 0, 0, 0, 0

s_banner	db	'SGPBENCH: SGP sprites, about 3 minutes. ESC stops.',13,10,'$'
s_memory	db	'Not enough memory (needs 128 KB after the program).',13,10,'$'
s_va		db	'PC-88VA', 0
s_va2		db	'PC-88VA2/VA3', 0
s_va91		db	'PC-88VA + VA-91', 0
s_unknown	db	'UNKNOWN MACHINE', 0
s_crlf		db	13,10,'$'
s_head1		db	'SGPBENCH  ', 0
s_khz15		db	'  15 KHZ', 0
s_khz24		db	'  24.8 KHZ', 0
s_head2		db	'SPRITES A VRTC, 180 VRTCS A TEST, BITBLT 0105H', 0
s_head3		db	'SCREEN  COL.  SOURCE    8X8  16X16  32X32  64X64', 0
s_stopped	db	'(STOPPED BY ESC: LATER ROWS ARE 0)', 0
s_foot		db	'SPRITES A LIST: 128, 64, 32, 16', 0
s_empty		db	0
s_320x		db	'320X', 0
s_d16		db	'16', 0
s_d256		db	'256', 0
s_d64k		db	'65536', 0
s_ram		db	'RAM', 0
s_gvram		db	'GVRAM', 0
s_skip		db	'-', 0
s_over		db	'OVER', 0
s_8		db	'8X8', 0
s_16		db	'16X16', 0
s_32		db	'32X32', 0
s_64		db	'64X64', 0
s_n		db	' N', 0

		align	2
machine		dw	0
saved_mode	dw	0
saved_bpp	dw	0
cs_lo		dw	0
cs_hi		dw	0
src_seg		dw	0
pitch		dw	0
height		dw	0
size		dw	0
count		dw	0
seed		dw	0
src_pitch	dw	0
slot		dw	0
lists		dw	0
ticks		dw	0
pd_x		dw	0
results		times	N_TESTS dd 0
saved_map	db	0
khz24		db	0
res_i		db	0
depth_i		db	0
loc_i		db	0
size_i		db	0
bpp		db	0
code		db	0
shift		db	0
dshift		db	0
doubled		db	0
sync_sent	db	0
complete	db	0
prev		db	0
ink		db	0
pd_c		db	0
s_rate		times	12 db 0
line		times	96 db 0
		align	2
image_end:
IMAGE_TOP	equ	image_end - $$ + 100h
%if IMAGE_TOP > LIST
%error the program reaches its SGP list
%endif
