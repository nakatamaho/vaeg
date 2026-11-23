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
; SGPMEM.COM: how much of its work the main CPU keeps while the SGP draws,
; by the memory the CPU uses, on a PC-88VA/VA2/VA3 and vaeg (README.md).
; Synthetic images only.
;
; Each CPU loop runs in chunks of 64 iterations: register only, then byte
; reads, byte writes and word writes in main RAM below 40000h (standard
; RAM), main RAM at 60000h (the PC-88VA's extended RAM, 1 wait), GVRAM
; (G1) and TVRAM (port 153h bank 1). A row counts the chunks done in
; FRAMES VRTCs alone, then while the SGP repeats a list of 30 16x16
; sprites (BITBLT 0105h) into G0 from main RAM, then from GVRAM: the
; ratios to the count alone, and the lists the SGP finished meanwhile.
; Two rows give the lists alone. The machine (F000:FFFE) heads the table.
; At the end the table is also written to SGPMEM.TXT; then the program
; stops: restart the machine to leave.

		cpu	186
		org	100h

%include "font.inc"			; FONT, PALETTE32

BENCH_SEG	equ	2000h		; this program, copied (SGP address 2xxxxh)
LO_SEG		equ	3000h		; main RAM below 40000h
SRC_SEG		equ	3800h		; SGP source in main RAM (the sprite)
HI_SEG		equ	6000h		; main RAM at 60000h
MEMORY_TOP	equ	6100h
GV_SRC_SEG	equ	0d000h		; SGP source in GVRAM (offset 30000h,
GV_SRC_SGP	equ	0023h		; SGP address 230000h)
TV_SEG		equ	0a700h		; TVRAM offset 7000h in bank 1
LIST		equ	0c000h		; SGP lists and work area (this segment)
GLIST		equ	0c400h
WORK		equ	0c800h
OUTBUF		equ	0d000h		; the table as text, for SGPMEM.TXT
FRAMES		equ	300		; VRTCs a measurement (5 s)
N_SPRITES	equ	30
LINE_BYTES	equ	160
GV_G0		equ	0a000h
GV_G1		equ	0c000h
TEXT_TOP	equ	16
WHITE		equ	15
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
		call	machine_name
		call	video_init
		cli
		xor	ax,ax
		mov	es,ax
		mov	ax,[es:0ah * 4]
		mov	[old_vrtc],ax
		mov	ax,[es:0ah * 4 + 2]
		mov	[old_vrtc + 2],ax
		mov	word [es:0ah * 4],vrtc
		mov	[es:0ah * 4 + 2],cs
		push	cs
		pop	es
		mov	dx,18ah
		in	al,dx
		mov	[old_mask],al
		mov	al,0fbh			; only VRTC (IR2)
		out	dx,al
		sti
		mov	dx,0153h
		in	al,dx
		mov	[map_gv],al
		and	al,0f0h			; bank 1: TVRAM (GMSP kept)
		or	al,1
		mov	[map_tv],al
		call	sources
		call	lists
		mov	word [out_ptr],OUTBUF
		mov	si,title
		mov	byte [colour],RED
		xor	bx,bx
		call	emit
		mov	si,s_head1
		mov	byte [colour],WHITE
		mov	bx,1
		call	emit
		mov	si,s_head2
		mov	bx,2
		call	emit
		xor	bx,bx			; the CPU rows
.row:		push	bx
		call	cpu_row
		pop	bx
		add	bx,2
		cmp	bx,N_ROWS * 2
		jb	.row
		mov	ax,LIST			; the SGP alone
		mov	si,s_sgp_ram
		mov	bx,N_ROWS + 3
		call	sgp_row
		mov	ax,GLIST
		mov	si,s_sgp_gv
		mov	bx,N_ROWS + 4
		call	sgp_row
		mov	si,s_done
		mov	byte [colour],RED
		mov	bx,N_ROWS + 6
		call	emit
		call	write_file
bench_done:	nop				; the table is complete here
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

; The machine from F000:FFFE into the title.
machine_name:	push	es
		mov	ax,0f000h
		mov	es,ax
		mov	ax,[es:0fffeh]
		pop	es
		mov	si,s_va
		cmp	ax,0ffffh
		je	.copy
		mov	si,s_va2
		cmp	ax,0fffeh
		je	.copy
		mov	si,s_va91
		cmp	ax,0fffdh
		je	.copy
		mov	si,s_unknown
.copy:		mov	di,title_machine
.c:		lodsb
		stosb
		or	al,al
		jnz	.c
		dec	di
		mov	si,s_title_tail
.t:		lodsb
		stosb
		or	al,al
		jnz	.t
		ret

; ---------------------------------------------------------------------------
; Video (as SGPCONT): two packed 4-bpp 320-dot screens, G0 (palette set 1,
; colour 0 transparent) in front of G1 (set 0), 224 lines, GVRAM cleared.
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
; CPU row BX / 2: chunks alone, with the main RAM list, with the GVRAM list.
cpu_row:	mov	ax,[row_chunk + bx]
		mov	[chunk],ax
		mov	ax,[row_seg + bx]
		mov	[target],ax
		mov	al,[map_gv]
		cmp	word [row_tv + bx],0
		je	.map
		mov	al,[map_tv]
.map:		mov	[map_row],al
		push	bx
		call	alone
		mov	[n_alone],ax
		mov	ax,LIST
		call	pair
		mov	[n_ram],dx
		mov	[l_ram],ax
		mov	ax,GLIST
		call	pair
		mov	[n_gv],dx
		mov	[l_gv],ax
		pop	bx
		mov	si,[row_name + bx]	; the line
		mov	di,line
		call	copy_pad14
		mov	ax,[n_alone]
		call	num5
		mov	al,' '
		stosb
		mov	ax,[n_ram]
		call	ratio
		mov	ax,[l_ram]
		call	num5
		mov	al,' '
		stosb
		mov	ax,[n_gv]
		call	ratio
		mov	ax,[l_gv]
		call	num5
		mov	byte [di],0
		shr	bx,1
		add	bx,3
		mov	si,line
		mov	byte [colour],WHITE
		jmp	emit

; The SGP list AX alone: lists in FRAMES VRTCs, line BX, name SI.
sgp_row:	push	bx
		push	si
		mov	[list],ax
		call	edge
.l:		mov	ax,[list]
		call	submit
		call	sgp_wait
		inc	word [count]
		mov	ax,[tick]
		sub	ax,[t0]
		cmp	ax,FRAMES
		jb	.l
		pop	si
		mov	di,line
		call	copy_pad14
		mov	cx,6 + 6		; under the lists columns
		mov	al,' '
		rep	stosb
		mov	ax,[count]
		call	num5
		mov	byte [di],0
		pop	bx
		mov	si,line
		mov	byte [colour],WHITE
		jmp	emit

; Wait for a VRTC edge; t0 and count reset.
edge:		mov	ax,[tick]
.e:		cmp	ax,[tick]
		je	.e
		mov	ax,[tick]
		mov	[t0],ax
		mov	word [count],0
		ret

; Map [map_row] and segment [target] in ES while measuring.
enter_row:	mov	dx,0153h
		mov	al,[map_row]
		out	dx,al
		mov	es,[target]
		ret
leave_row:	push	cs
		pop	es
		mov	dx,0153h
		mov	al,[map_gv]
		out	dx,al
		ret

; Chunks of [chunk] in FRAMES VRTCs, the SGP idle (AX); each chunk reads
; the SGP status as pair does.
alone:		call	edge
		call	enter_row
.c:		call	[chunk]
		inc	word [count]
		mov	dx,0506h
		in	al,dx
		mov	ax,[tick]
		sub	ax,[t0]
		cmp	ax,FRAMES
		jb	.c
		call	leave_row
		mov	ax,[count]
		ret

; The SGP repeating list AX while the CPU repeats [chunk], FRAMES VRTCs:
; AX = lists finished, DX = chunks.
pair:		mov	[list],ax
		mov	word [lists_n],0
		call	edge
		call	enter_row
.list:		mov	ax,[list]
		call	submit
.chunk:		call	[chunk]
		inc	word [count]
		mov	dx,0506h
		in	al,dx
		test	al,1
		jnz	.chunk
		inc	word [lists_n]
		mov	ax,[tick]
		sub	ax,[t0]
		cmp	ax,FRAMES
		jb	.list
		call	leave_row
		mov	ax,[lists_n]
		mov	dx,[count]
		ret

; The chunks: 64 iterations at ES:0.
c_reg:		mov	cx,64
.l:		add	ax,bx
		inc	di
		dec	cx
		jnz	.l
		ret
c_read:		xor	di,di
		mov	cx,64
.l:		mov	al,[es:di]
		inc	di
		dec	cx
		jnz	.l
		ret
c_write:	xor	di,di
		mov	cx,64
.l:		mov	[es:di],al
		inc	di
		dec	cx
		jnz	.l
		ret
c_wordw:	xor	di,di
		mov	cx,64
.l:		mov	[es:di],ax
		add	di,2
		dec	cx
		jnz	.l
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
; The sprite (16 x 16, 8 bytes a row; SGPCONT's): in main RAM at SRC_SEG
; and in GVRAM at GV_SRC_SEG.
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
		push	ds
		mov	ax,SRC_SEG
		mov	ds,ax
		mov	ax,GV_SRC_SEG
		mov	es,ax
		xor	si,si
		xor	di,di
		mov	cx,64
		rep	movsw
		pop	ds
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

; Offsets of the 30 rectangles in G0 (bytes 104-151, lines 228-343, not
; shown) and the two lists (BITBLT 0105h): source in main RAM, in GVRAM.
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
		imul	ax,ax,20
		add	ax,228			; below the 224 lines shown
		imul	ax,ax,LINE_BYTES
		add	ax,bx
		stosw
		inc	cx
		cmp	cx,N_SPRITES
		jb	.off
		mov	di,LIST
		mov	si,SRC_SEG << 4		; SGP address of the main RAM copy
		mov	dx,SRC_SEG >> 12
		call	.list
		mov	di,GLIST
		xor	si,si
		mov	dx,GV_SRC_SGP
.list:		mov	ax,3			; SET_WORK
		stosw
		mov	ax,WORK
		stosw
		mov	ax,BENCH_SEG >> 12
		stosw
		xor	bx,bx
.sprite:	mov	ax,4			; SET_SOURCE: 4 bpp, 16 x 16, 8
		stosw				; bytes a line
		mov	ax,1
		stosw
		mov	ax,16
		stosw
		stosw
		mov	ax,8
		stosw
		mov	ax,si
		stosw
		mov	ax,dx
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
		mov	ax,7			; BITBLT, source 0 transparent
		stosw
		mov	ax,0105h
		stosw
		add	bx,2
		cmp	bx,N_SPRITES * 2
		jb	.sprite
		mov	ax,1			; END
		stosw
		ret

; ---------------------------------------------------------------------------
; Text: SI (0-terminated) to the screen at line BX and to OUTBUF with CR LF.
emit:		push	si
		mov	di,[out_ptr]
.c:		lodsb
		or	al,al
		jz	.end
		stosb
		jmp	.c
.end:		mov	ax,0a0dh
		stosw
		mov	[out_ptr],di
		pop	si
		jmp	text_line

; SI (0-terminated) to DI, padded with spaces to 14 characters.
copy_pad14:	mov	cx,14
.c:		lodsb
		or	al,al
		jz	.pad
		stosb
		loop	.c
		ret
.pad:		mov	al,' '
		rep	stosb
		ret

; AX right-aligned in 5 characters at DI (DI advanced).
num5:		push	bx
		push	cx
		push	dx
		add	di,5
		push	di
		mov	cx,5
		mov	bx,10
.d:		xor	dx,dx
		div	bx
		dec	di
		add	dl,'0'
		mov	[di],dl
		or	ax,ax
		jnz	.n
		dec	cx
		jz	.done
.blank:		dec	di
		mov	byte [di],' '
		loop	.blank
		jmp	.done
.n:		loop	.d
.done:		pop	di
		pop	dx
		pop	cx
		pop	bx
		ret

; AX / [n_alone] as ' d.ddd ' at DI (6 characters, DI advanced).
ratio:		push	bx
		mov	bx,1000
		mul	bx
		mov	bx,[n_alone]
		or	bx,bx
		jz	.zero
		cmp	dx,bx
		jae	.zero
		div	bx
		jmp	.show
.zero:		xor	ax,ax
.show:		mov	byte [di],' '
		inc	di
		xor	dx,dx
		mov	bx,1000
		div	bx			; AX units, DX thousandths
		cmp	ax,9
		jbe	.u
		mov	ax,9
.u:		add	al,'0'
		stosb
		mov	al,'.'
		stosb
		mov	ax,dx
		mov	bl,100
		div	bl
		add	al,'0'
		stosb
		mov	al,ah
		xor	ah,ah
		mov	bl,10
		div	bl
		add	ax,'00'
		stosw
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

; OUTBUF to SGPMEM.TXT in the current directory (DOS, the interrupts as
; MS-DOS had them); a line on the screen if it fails.
write_file:	cli
		xor	ax,ax
		mov	es,ax
		mov	ax,[old_vrtc]
		mov	[es:0ah * 4],ax
		mov	ax,[old_vrtc + 2]
		mov	[es:0ah * 4 + 2],ax
		push	cs
		pop	es
		mov	dx,18ah
		mov	al,[old_mask]
		out	dx,al
		sti
		mov	ah,3ch
		xor	cx,cx
		mov	dx,s_file
		int	21h
		jc	.fail
		mov	bx,ax
		mov	cx,[out_ptr]
		sub	cx,OUTBUF
		mov	dx,OUTBUF
		mov	ah,40h
		int	21h
		jc	.fail
		mov	ah,3eh
		int	21h
		jc	.fail
		mov	si,s_written
		jmp	.show
.fail:		mov	si,s_nowrite
.show:		mov	byte [colour],RED
		mov	bx,N_ROWS + 7
		jmp	text_line

; ---------------------------------------------------------------------------
N_ROWS		equ	13
row_chunk	dw	c_reg
		dw	c_read, c_write, c_wordw
		dw	c_read, c_write, c_wordw
		dw	c_read, c_write, c_wordw
		dw	c_read, c_write, c_wordw
row_seg		dw	LO_SEG
		dw	LO_SEG, LO_SEG, LO_SEG
		dw	HI_SEG, HI_SEG, HI_SEG
		dw	GV_G1, GV_G1, GV_G1
		dw	TV_SEG, TV_SEG, TV_SEG
row_tv		dw	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1
row_name	dw	s_reg
		dw	s_lo_r, s_lo_w, s_lo_ww
		dw	s_hi_r, s_hi_w, s_hi_ww
		dw	s_gv_r, s_gv_w, s_gv_ww
		dw	s_tv_r, s_tv_w, s_tv_ww
s_banner	db	'SGPMEM: restart the machine to leave.',13,10,'$'
s_24khz		db	'Select 15 kHz (DIP switch SW1 off) before starting.',13,10,'$'
s_memory	db	'Not enough memory (needs 6100h; load below 2000h).',13,10,'$'
s_va		db	'PC-88VA', 0
s_va2		db	'PC-88VA2/VA3', 0
s_va91		db	'PC-88VA+VA-91', 0
s_unknown	db	'UNKNOWN MACHINE', 0
s_title_tail	db	'  300 VRTC', 0
s_head1		db	'CPU LOOP      ALONE  SGP SRC RAM  SGP SRC GVRAM', 0
s_head2		db	'64 TIMES     CHUNKS  RATIO LISTS  RATIO LISTS', 0
s_reg		db	'REG ADD', 0
s_lo_r		db	'RAM30 READ', 0
s_lo_w		db	'RAM30 WRITE', 0
s_lo_ww		db	'RAM30 WRITE W', 0
s_hi_r		db	'RAM60 READ', 0
s_hi_w		db	'RAM60 WRITE', 0
s_hi_ww		db	'RAM60 WRITE W', 0
s_gv_r		db	'GVRAM READ', 0
s_gv_w		db	'GVRAM WRITE', 0
s_gv_ww		db	'GVRAM WRITE W', 0
s_tv_r		db	'TVRAM READ', 0
s_tv_w		db	'TVRAM WRITE', 0
s_tv_ww		db	'TVRAM WRITE W', 0
s_sgp_ram	db	'SGP ALONE RAM', 0
s_sgp_gv	db	'SGP ALONE GV', 0
s_done		db	'DONE: RESTART TO LEAVE', 0
s_written	db	'WRITTEN TO SGPMEM.TXT', 0
s_nowrite	db	'SGPMEM.TXT NOT WRITTEN', 0
s_file		db	'SGPMEM.TXT', 0

sync224		db	0c1h,57h,1ch,0,9fh,0,10h,0fh,25h,0,0e0h,0,2,4
		align	2
fb_bios		dw	4,320,224,0,0,0
frames4		dw	0,0,160,447,0,0,0,0,0,224,0,0,0,0,0,0
		dw	0,0,160,0,0,0,0,0,0,224,0,0,0,0,0,0
palette		dw	PALETTE32
old_vrtc	dw	0, 0
tick		dw	0
t0		dw	0
count		dw	0
chunk		dw	0
target		dw	0
list		dw	0
lists_n		dw	0
n_alone		dw	0
n_ram		dw	0
l_ram		dw	0
n_gv		dw	0
l_gv		dw	0
out_ptr		dw	0
ink		dw	0
offsets		times	N_SPRITES dw 0
old_mask	db	0
map_gv		db	0
map_tv		db	0
map_row		db	0
colour		db	0
title		db	'SGPMEM  '
title_machine	times	40 db 0
line		times	56 db 0
		align	2
image_end:
IMAGE_TOP	equ	image_end - $$ + 100h	; offset of the end, below LIST
%if IMAGE_TOP > LIST
%error the program reaches its SGP lists
%endif
