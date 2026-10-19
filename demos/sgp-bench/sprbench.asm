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
; SPRBENCH.COM: sprite drawing benchmark for the PC-88VA/VA2 and vaeg
; (README.md). Synthetic images only.
;
; Transparent square sprites of 16, 32, 64 and 128 dots, in 16 colours
; (4 bpp: two packed screens, G0 front B at 09100h) and in 256
; colours (8 bpp: one screen, lines 0-203 so that the CPU stays in one
; segment), drawn five ways:
;   CPU     the CPU only: a mask-and-data image, per word AND the
;           destination, OR the data (images pre-shifted: word-aligned X)
;   SGP25, SGP50, SGP75
;           that share of the sprites on the SGP (one list, started first),
;           the rest on the CPU meanwhile, then the wait for the SGP
;   SGP     the SGP only: BITBLT 0105h (source zero transparent) from main
;           RAM, as an arcade-game port for the VA draws
; 32, 16, 8 and 4 sprites a repetition for the four sizes. Each test runs
; twice, each time for WINDOW VRTCs; the time of the repetitions is the
; VRTCs elapsed to the edge after the last one, minus the rest of that VRTC
; measured with a spin loop calibrated against the VRTC (so finer than a
; VRTC). The table shows the mean time a sprite in microseconds and the
; largest difference between the two runs. Before each size and depth one sprite is
; drawn by the SGP and one by the CPU over the same pattern and compared.
; The results are also stored at GVRAM 12400h (README.md).
; Never returns: restart the machine to leave.

		cpu	186
		org	100h

%include "bench_data.inc"		; FONT, PALETTE16

BENCH_SEG	equ	3000h		; this program, copied (SGP address 3xxxxh)
SRC_SEG		equ	4000h		; SGP source image (SGP 40000h), <= 16 KiB
CPU_SEG		equ	4400h		; CPU image: mask, data words, <= 32 KiB
MEMORY_TOP	equ	5000h
WINDOW		equ	240		; VRTCs a run
RUNS		equ	2		; runs a test
US_VRTC10_LOW	equ	166834 - 2 * 65536	; tenths of a us a VRTC (59.94 Hz):
						; 2 * 65536 + this
FRONT_B		equ	9100h
GV_A		equ	0a000h		; CPU segments: front A / 8 bpp screen,
GV_B		equ	0a000h + FRONT_B / 16	; front B, G1
GV_G1		equ	0c000h
RESULT_SEG	equ	0b240h		; GVRAM 12400h
TEXT_TOP	equ	40
N_CFG		equ	8		; 16 colours 16-128, 256 colours 16-128
N_SPLIT		equ	5		; CPU, SGP25, SGP50, SGP75, SGP
CELL		equ	20		; per run reps, tenths of us a rep; sprites, spare
WHITE		equ	15
YELLOW		equ	11
RED		equ	9
GREEN		equ	10

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
		add	ax,1000h
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
		mov	cx,8000h
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
		mov	ax,FRONT_B		; 16-colour sprites are shown on front B
		call	show_front
		mov	si,s_running
		mov	byte [colour],YELLOW
		xor	bx,bx
		call	text_line
		call	calibrate
		mov	word [cfg],0
.cfg:		cmp	word [cfg],N_CFG / 2
		jne	.mode
		call	mode8
		call	calibrate
.mode:		call	config_setup
		call	check_config
		mov	word [split],0
.split:		call	split_setup
		mov	word [run],0
.run:		call	measure
		call	record
		inc	word [run]
		cmp	word [run],RUNS
		jb	.run
		call	finish_cell
		inc	word [split]
		cmp	word [split],N_SPLIT
		jb	.split
		inc	word [cfg]
		cmp	word [cfg],N_CFG
		jb	.cfg
		call	mode4
		call	report
		call	store_results
bench_done:	nop				; the results are complete here
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

; ---------------------------------------------------------------------------
; Video. The BIOS sets up the single-plane 320x224 screen once; mode4 then
; selects two packed 4-bpp screens, mode8 one 8-bpp screen. Both clear
; all GVRAM.
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
		mov	dx,0100h
		in	ax,dx
		mov	[orig_0100],ax
		mov	dx,0102h
		in	ax,dx
		mov	[orig_0102],ax
		call	mode4
		cli
		mov	si,sync224
		mov	al,10h
		call	tsp_cmd
		mov	cx,14
.sync:		lodsb
		call	tsp_prm
		loop	.sync
		sti
		ret

mode4:		call	clear_gvram
		cli
		call	set_palette
		mov	dx,0100h
		mov	ax,[orig_0100]
		or	ax,0800h		; two screens
		out	dx,ax
		mov	dx,0102h
		mov	ax,1111h
		out	dx,ax
		mov	dx,010ch
		mov	ax,01a0h
		out	dx,ax
		mov	dx,0106h
		mov	ax,00bah
		out	dx,ax
		mov	dx,0124h
		mov	ax,1
		out	dx,ax
		add	dx,2
		xor	ax,ax
		out	dx,ax
		mov	si,frames4
		mov	cx,32
		call	set_frames
		sti
		ret

mode8:		call	clear_gvram
		cli
		call	set_palette
		mov	dx,0100h
		mov	ax,[orig_0100]
		and	ax,0f7ffh		; one screen
		out	dx,ax
		mov	dx,0102h
		mov	ax,[orig_0102]
		and	ax,0fffch
		or	ax,2			; G0: 8 bpp, the BIOS's 320 dots
		out	dx,ax
		mov	dx,0106h
		mov	ax,000ah		; G0 only
		out	dx,ax
		mov	dx,0124h		; no transparency
		xor	ax,ax
		out	dx,ax
		mov	dx,010ch
		mov	ax,01c0h		; palette mode 3
		out	dx,ax
		mov	si,frames8
		mov	cx,16
		call	set_frames
		sti
		ret

set_frames:	mov	dx,0200h		; CX word registers from SI
.reg:		lodsw
		out	dx,ax
		add	dx,2
		loop	.reg
		ret

set_palette:	mov	cx,2			; twice: vaeg settles DAC low bits
.twice:		push	cx
		mov	dx,0300h
		mov	si,palette
		mov	cx,32
.entry:		lodsw
		out	dx,ax
		add	dx,2
		loop	.entry
		pop	cx
		loop	.twice
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

; FB0 DSA = AX (word writes only).
show_front:	push	dx
		mov	dx,020eh
		out	dx,ax
		add	dx,2
		xor	ax,ax
		out	dx,ax
		pop	dx
		ret

; ---------------------------------------------------------------------------
; Configuration [cfg]: 0-3 16 colours, 4-7 256 colours; size 16 << (cfg & 3),
; 32 >> (cfg & 3) sprites.
config_setup:	mov	cx,[cfg]
		and	cl,3
		mov	ax,16
		shl	ax,cl
		mov	[n],ax
		mov	ax,32
		shr	ax,cl
		mov	[count],ax
		cmp	word [cfg],N_CFG / 2
		jae	.d8
		mov	word [bpp_mode],1	; SGP pixel mode: 4 bpp
		mov	ax,[n]
		shr	ax,1
		mov	[bytes],ax
		mov	word [pitch],160
		mov	word [rows],224
		mov	word [dest_seg],GV_B
		mov	word [dest_lo],FRONT_B
		mov	word [chk_o1],8		; check regions at x 16 and 160
		mov	word [chk_o2],80
		jmp	.common
.d8:		mov	word [bpp_mode],2	; 8 bpp
		mov	ax,[n]
		mov	[bytes],ax
		mov	word [pitch],320
		mov	word [rows],204
		mov	word [dest_seg],GV_A
		mov	word [dest_lo],0
		mov	word [chk_o1],16
		mov	word [chk_o2],160
.common:	mov	ax,[bytes]
		shr	ax,1
		mov	[words],ax
		mul	word [n]
		mov	[image_words],ax
		mov	ax,[pitch]
		sub	ax,[bytes]
		mov	[skip],ax
		call	gen_images
		jmp	positions

; Pixel (SI = x, BP = y) -> AL; 0 (transparent) where (7x + 3y) mod 11 = 0,
; else (x + 2y) mod 15 + 1 (16 colours) or (3x + 5y) mod 255 + 1.
pixel:		mov	ax,si
		imul	ax,ax,7
		imul	bx,bp,3
		add	ax,bx
		xor	dx,dx
		mov	bx,11
		div	bx
		or	dx,dx
		jz	.zero
		cmp	word [bpp_mode],2
		je	.v8
		mov	ax,bp
		shl	ax,1
		add	ax,si
		mov	bx,15
		jmp	.mod
.v8:		imul	ax,si,3
		imul	bx,bp,5
		add	ax,bx
		mov	bx,255
.mod:		xor	dx,dx
		div	bx
		mov	al,dl
		inc	al
		ret
.zero:		xor	al,al
		ret

; The SGP image at SRC_SEG:0 (rows of [bytes]) and the CPU image at CPU_SEG:0
; (per source word a mask word, then the data word).
gen_images:	push	es
		mov	ax,SRC_SEG
		mov	es,ax
		xor	di,di
		xor	bp,bp
.row:		xor	si,si
.px:		call	pixel
		cmp	word [bpp_mode],2
		je	.store
		shl	al,4
		mov	cl,al
		inc	si
		call	pixel
		or	al,cl
.store:		stosb
		inc	si
		cmp	si,[n]
		jb	.px
		inc	bp
		cmp	bp,[n]
		jb	.row
		push	ds
		mov	cx,[image_words]
		mov	ax,SRC_SEG
		mov	ds,ax
		mov	ax,CPU_SEG
		mov	es,ax
		xor	si,si
		xor	di,di
.word:		lodsw
		mov	bx,ax
		call	mask_byte
		mov	dl,al
		mov	al,bh
		call	mask_byte
		mov	ah,al
		mov	al,dl
		stosw				; mask
		mov	ax,bx
		stosw				; data
		loop	.word
		pop	ds
		pop	es
		ret

; AL pixel byte -> AL mask (FFh where transparent, per pixel).
mask_byte:	cmp	word [cs:bpp_mode],2
		je	.b8
		xor	dh,dh
		test	al,0f0h
		jnz	.low
		or	dh,0f0h
.low:		test	al,0fh
		jnz	.done
		or	dh,0fh
.done:		mov	al,dh
		ret
.b8:		or	al,al
		mov	al,0
		jnz	.r
		mov	al,0ffh
.r:		ret

; Destination offsets of the sprites: x = 16 + ((40 i) mod (288 - N)), on
; 8 dots, y = (24 i) mod (rows - N).
positions:	push	cs
		pop	es
		xor	cx,cx
		mov	di,offsets
.p:		imul	ax,cx,40
		mov	bx,288
		sub	bx,[n]
		xor	dx,dx
		div	bx
		mov	ax,dx
		and	ax,0fff8h
		add	ax,16
		cmp	word [bpp_mode],2
		je	.xb
		shr	ax,1
.xb:		mov	[x_bytes],ax
		imul	ax,cx,24
		mov	bx,[rows]
		sub	bx,[n]
		xor	dx,dx
		div	bx
		mov	ax,dx
		mul	word [pitch]
		add	ax,[x_bytes]
		stosw
		inc	cx
		cmp	cx,[count]
		jb	.p
		ret

; ---------------------------------------------------------------------------
; Drawing.

; One sprite by the CPU at DI in [dest_seg].
cpu_sprite:	push	ds
		push	es
		mov	es,[cs:dest_seg]
		mov	dx,[cs:n]
		mov	bx,[cs:words]
		mov	bp,[cs:skip]
		mov	ax,CPU_SEG
		mov	ds,ax
		xor	si,si
.row:		mov	cx,bx
.word:		lodsw				; mask
		and	ax,[es:di]
		or	ax,[si]			; data
		inc	si
		inc	si
		stosw
		loop	.word
		add	di,bp
		dec	dx
		jnz	.row
		pop	es
		pop	ds
		ret

; The SGP list for sprites 0 .. [k_sgp] - 1 (per sprite: source and
; destination descriptors, BITBLT 0105h).
build_list:	push	es
		push	cs
		pop	es
		mov	di,list
		mov	ax,3			; SET_WORK
		stosw
		mov	ax,work
		stosw
		mov	ax,BENCH_SEG >> 12
		stosw
		xor	bx,bx
.sprite:	cmp	bx,[k_sgp]
		jae	.end
		mov	ax,4			; SET_SOURCE
		stosw
		mov	ax,[bpp_mode]		; dot 0
		stosw
		mov	ax,[n]
		stosw
		stosw
		mov	ax,[bytes]
		stosw
		xor	ax,ax
		stosw
		mov	ax,SRC_SEG >> 12
		stosw
		mov	ax,5			; SET_DESTINATION
		stosw
		mov	ax,[bpp_mode]
		stosw
		mov	ax,[n]
		stosw
		stosw
		mov	ax,[pitch]
		stosw
		mov	si,bx
		shl	si,1
		mov	ax,[offsets + si]
		mov	dx,20h			; GVRAM at SGP 200000h
		add	ax,[dest_lo]
		adc	dx,0
		stosw
		mov	ax,dx
		stosw
		mov	ax,7			; BITBLT, source zero transparent
		stosw
		mov	ax,0105h
		stosw
		inc	bx
		jmp	.sprite
.end:		mov	ax,1			; END
		stosw
		pop	es
		ret

split_setup:	mov	ax,[count]
		mul	word [split]
		shr	ax,2
		mov	[k_sgp],ax
		jmp	build_list

; One repetition: the SGP's share started, the CPU's drawn, the SGP awaited.
op_draw:	cmp	word [k_sgp],0
		je	.cpu
		mov	ax,list
		call	sgp_submit
.cpu:		mov	bx,[k_sgp]
.next:		cmp	bx,[count]
		jae	.wait
		push	bx
		mov	si,bx
		shl	si,1
		mov	di,[offsets + si]
		call	cpu_sprite
		pop	bx
		inc	bx
		jmp	.next
.wait:		cmp	word [k_sgp],0
		je	.ret
		call	sgp_wait
.ret:		ret

sgp_submit:	push	ax			; AX = list offset
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
		inc	word [timeouts]		; stop it and go on
		mov	dx,0504h
		mov	al,2
		out	dx,al
.ready:		pop	cx
		pop	bx
		ret

; ---------------------------------------------------------------------------
; SGP-CPU check: one sprite each over the same pattern, compared.
check_config:	mov	di,[chk_o1]
		call	fill_region
		mov	di,[chk_o2]
		call	fill_region
		push	word [offsets]
		mov	ax,[chk_o1]
		mov	[offsets],ax
		mov	word [k_sgp],1
		call	build_list
		mov	ax,list
		call	sgp_submit
		call	sgp_wait
		pop	word [offsets]
		mov	di,[chk_o2]
		call	cpu_sprite
		push	ds
		push	es
		mov	si,[chk_o1]
		mov	di,[chk_o2]
		mov	dx,[n]
		mov	bx,[bytes]
		mov	bp,[pitch]
		sub	bp,bx
		mov	word [mism],0
		mov	ax,[dest_seg]
		mov	ds,ax
		mov	es,ax
.row:		mov	cx,bx
.byte:		cmpsb
		je	.same
		inc	word [cs:mism]
.same:		loop	.byte
		add	si,bp
		add	di,bp
		dec	dx
		jnz	.row
		pop	es
		pop	ds
		mov	bx,[cfg]
		shl	bx,1
		mov	ax,[mism]
		mov	[checks + bx],ax
		ret

; N rows of [bytes] at DI in [dest_seg]: (13 row + 7 column + 35h) & FFh.
fill_region:	push	es
		mov	es,[dest_seg]
		xor	bp,bp
.row:		push	di
		mov	cx,[bytes]
		xor	si,si
.byte:		imul	ax,bp,13
		imul	dx,si,7
		add	ax,dx
		add	al,35h
		stosb
		inc	si
		loop	.byte
		pop	di
		add	di,[pitch]
		inc	bp
		cmp	bp,[n]
		jb	.row
		pop	es
		ret

; ---------------------------------------------------------------------------
; Timing: op_draw until WINDOW VRTCs have passed; reps = calls,
; elapsed = the VRTCs that passed (the last call may overrun the window).
measure:	mov	word [reps],0
		mov	word [reps + 2],0
		mov	ax,[tick]
.sync:		cmp	ax,[tick]
		je	.sync
		mov	ax,[tick]
		mov	[t0],ax
.loop:		call	op_draw
		add	word [reps],1
		adc	word [reps + 2],0
		mov	ax,[tick]
		sub	ax,[t0]
		cmp	ax,WINDOW
		jb	.loop
		call	spin			; the rest of this VRTC, in spins
		mov	[spins],cx
		mov	ax,[tick]
		sub	ax,[t0]
		mov	[elapsed],ax
		ret

; Count loop passes until the next VRTC: CX.
spin:		xor	cx,cx
		mov	ax,[tick]
.loop:		inc	cx
		cmp	ax,[tick]
		je	.loop
		ret

; Spins in a whole VRTC (mean of 16), with the display mode set: spin_rate.
calibrate:	mov	ax,[tick]
.edge:		cmp	ax,[tick]
		je	.edge
		xor	bx,bx
		xor	si,si
		mov	bp,16
.vrtc:		call	spin
		add	bx,cx
		adc	si,0
		dec	bp
		jnz	.vrtc
		mov	cx,4
.mean:		shr	si,1
		rcr	bx,1
		loop	.mean
		mov	[spin_rate],bx
		mov	bx,[cfg]		; kept: 16 and 256 colours
		cmp	bx,N_CFG / 2
		mov	bx,0
		jb	.store
		mov	bx,2
.store:		mov	ax,[spin_rate]
		mov	[spin_rates + bx],ax
		ret

; Run [run] of cell [cfg] * N_SPLIT + [split]: reps and tenths of us a
; repetition: (elapsed x 166834 - spins x 166834 / spin_rate) / reps.
record:		mov	ax,[spins]		; the unused rest of the last VRTC
		mov	cx,US_VRTC10_LOW
		mul	cx
		mov	cx,[spins]
		shl	cx,1
		add	dx,cx
		mov	cx,[spin_rate]
		cmp	[spins],cx		; at most a VRTC
		jb	.rest
		mov	ax,cx
		dec	ax
		mov	[spins],ax
		jmp	record
.rest:		call	div32
		mov	[rest10],ax
		mov	[rest10 + 2],dx
		mov	ax,[elapsed]
		mov	cx,US_VRTC10_LOW
		mul	cx
		mov	cx,[elapsed]
		shl	cx,1
		add	dx,cx
		sub	ax,[rest10]
		sbb	dx,[rest10 + 2]
		mov	cx,[reps]
		mov	bx,[reps + 2]
.fit:		or	bx,bx			; divisor to 16 bits: halve both
		jz	.divide
		shr	bx,1
		rcr	cx,1
		shr	dx,1
		rcr	ax,1
		jmp	.fit
.divide:	call	div32
		call	cell_offset
		mov	si,[run]
		shl	si,3
		add	bx,si
		mov	cx,[reps]
		mov	[cells + bx],cx
		mov	cx,[reps + 2]
		mov	[cells + 2 + bx],cx
		mov	[cells + 4 + bx],ax
		mov	[cells + 6 + bx],dx
		ret

; BX = the cell's offset in cells, SI = its index x 4.
cell_offset:	imul	bx,[cfg],N_SPLIT
		add	bx,[split]
		mov	si,bx
		shl	si,2
		imul	bx,bx,CELL
		ret

; After both runs: the mean a repetition, a sprite; the difference of the
; runs in 1/100 % of the mean, the largest kept in max_diff.
finish_cell:	call	cell_offset
		mov	cx,[count]
		mov	[cells + 16 + bx],cx
		mov	ax,[cells + 4 + bx]	; mean of the two runs
		mov	dx,[cells + 6 + bx]
		add	ax,[cells + 12 + bx]
		adc	dx,[cells + 14 + bx]
		shr	dx,1
		rcr	ax,1
		mov	[mean10],ax
		mov	[mean10 + 2],dx
		push	bx
		push	si
		call	div32			; / sprites
		pop	si
		pop	bx
		mov	[per_sprite + si],ax
		mov	[per_sprite + 2 + si],dx
		mov	ax,[cells + 4 + bx]	; DX:AX = |a - b|
		mov	dx,[cells + 6 + bx]
		sub	ax,[cells + 12 + bx]
		sbb	dx,[cells + 14 + bx]
		jns	.positive
		not	ax
		not	dx
		add	ax,1
		adc	dx,0
.positive:	mov	cx,[mean10]		; mean and difference to 16 bits
		mov	bx,[mean10 + 2]
.norm:		or	bx,bx
		jz	.ratio
		shr	bx,1
		rcr	cx,1
		shr	dx,1
		rcr	ax,1
		jmp	.norm
.ratio:		or	dx,dx			; difference above the mean
		jnz	.over
		mov	bx,10000		; |a - b| x 10000 / mean
		mul	bx
		call	div32
		or	dx,dx
		jz	.clamp
.over:		mov	ax,0ffffh
.clamp:		cmp	ax,[max_diff]
		jbe	.ret
		mov	[max_diff],ax
.ret:		ret

; DX:AX / CX -> DX:AX quotient, BX remainder.
div32:		push	ax
		mov	ax,dx
		xor	dx,dx
		div	cx
		mov	bx,ax
		pop	ax
		div	cx
		xchg	bx,dx			; BX remainder, DX high quotient
		ret

; Write DX:AX in decimal at DI, right-aligned in CL places (CL=0: as is).
put_number:	push	bx
		push	si
		xor	ch,ch
		push	cx
		mov	si,digits + 10
.digit:		mov	cx,10
		call	div32
		add	bl,'0'
		dec	si
		mov	[si],bl
		mov	bx,ax
		or	bx,dx
		jnz	.digit
		pop	cx
		mov	ax,digits + 10
		sub	ax,si
		sub	cx,ax
		jbe	.copy
		mov	al,' '
		rep	stosb
.copy:		cmp	si,digits + 10
		jae	.done
		movsb
		jmp	.copy
.done:		pop	si
		pop	bx
		ret

; ---------------------------------------------------------------------------
; The table on G1 (after mode4).
report:		xor	ax,ax			; front A: cleared, transparent
		call	show_front
		mov	si,s_title
		mov	byte [colour],YELLOW
		xor	bx,bx
		call	text_line
		mov	si,s_head
		mov	byte [colour],WHITE
		mov	bx,1
		call	text_line
		mov	word [cfg],0
.row:		call	line_clear
		mov	si,s_16c
		cmp	word [cfg],N_CFG / 2
		jb	.label
		mov	si,s_256c
.label:		call	copy_text
		mov	cx,[cfg]
		and	cl,3
		mov	ax,16
		shl	ax,cl
		xor	dx,dx
		mov	di,line + 4
		mov	cl,4
		call	put_number
		mov	word [split],0
.col:		imul	di,[split],9
		add	di,line + 9
		imul	bx,[cfg],N_SPLIT
		add	bx,[split]
		shl	bx,2
		mov	ax,[per_sprite + bx]
		mov	dx,[per_sprite + 2 + bx]
		mov	cx,10
		call	div32			; DX:AX us, BX tenths
		push	bx
		mov	cl,6
		call	put_number
		mov	al,'.'
		stosb
		pop	ax
		add	al,'0'
		stosb
		inc	word [split]
		cmp	word [split],N_SPLIT
		jb	.col
		mov	byte [colour],WHITE
		mov	si,line
		mov	bx,[cfg]
		add	bx,2
		call	text_line
		inc	word [cfg]
		cmp	word [cfg],N_CFG
		jb	.row
		mov	si,s_legend
		mov	byte [colour],YELLOW
		mov	bx,N_CFG + 3
		call	text_line
		call	line_clear		; repeatability
		mov	si,s_repeat
		call	copy_text
		mov	byte [colour],GREEN
		mov	ax,[max_diff]
		cmp	ax,0ffffh
		jne	.diff
		mov	byte [colour],RED
		mov	si,s_over
		call	copy_text
		jmp	.diff_done
.diff:		cmp	ax,50			; above 0.5 %: shown red
		jbe	.diff_ok
		mov	byte [colour],RED
.diff_ok:	xor	dx,dx
		mov	cx,100
		call	div32			; AX percent, BX hundredths
		push	bx
		xor	dx,dx
		xor	cl,cl
		call	put_number
		mov	al,'.'
		stosb
		pop	ax
		aam				; AH tens, AL units
		add	ax,3030h
		xchg	al,ah
		stosw
		mov	si,s_pct
		call	copy_text
.diff_done:	mov	si,line
		mov	bx,N_CFG + 7
		call	text_line
		xor	bp,bp			; check lines: 16, then 256 colours
.check:		call	line_clear
		mov	si,s_check16
		or	bp,bp
		jz	.ctext
		mov	si,s_check256
.ctext:		call	copy_text
		mov	byte [colour],GREEN
		xor	cx,cx			; size index
.size:		push	cx
		mov	ax,16
		shl	ax,cl
		xor	dx,dx
		mov	cl,4
		call	put_number
		mov	al,':'
		stosb
		pop	cx
		mov	bx,bp
		shl	bx,2
		add	bx,cx
		shl	bx,1
		mov	ax,[checks + bx]
		or	ax,ax
		jnz	.bad
		mov	ax,'OK'
		stosw
		jmp	.next_size
.bad:		mov	byte [colour],RED
		cmp	ax,9999			; keep the line within 53 cells
		jbe	.count
		mov	ax,9999
.count:		push	cx
		xor	dx,dx
		xor	cl,cl
		call	put_number
		pop	cx
.next_size:	inc	cx
		cmp	cx,4
		jb	.size
		mov	si,line
		lea	bx,[bp + N_CFG + 4]
		call	text_line
		inc	bp
		cmp	bp,2
		jb	.check
		cmp	word [timeouts],0
		je	.ret
		call	line_clear
		mov	si,s_timeout
		call	copy_text
		mov	ax,[timeouts]
		xor	dx,dx
		xor	cl,cl
		call	put_number
		mov	byte [colour],RED
		mov	si,line
		mov	bx,N_CFG + 8
		call	text_line
.ret:		ret

; line: 53 spaces and the end; DI = line.
line_clear:	push	cs
		pop	es
		mov	di,line
		mov	cx,53
		mov	al,' '
		rep	stosb
		mov	byte [di],0
		mov	di,line
		ret

; Copy the text SI (to its 0) to DI.
copy_text:	lodsb
		or	al,al
		jz	.done
		stosb
		jmp	copy_text
.done:		ret

; Results for the checker at GVRAM 12400h: 'SPRB', configurations,
; splits, SGP timeouts (word), then the cells (CELL bytes each), the times
; a sprite (32-bit tenths of us), the check counts, the largest run
; difference (1/100 %) and the spin rates (16, 256 colours).
store_results:	mov	word [results],'SP'
		mov	word [results + 2],'RB'
		mov	byte [results + 4],N_CFG
		mov	byte [results + 5],N_SPLIT
		mov	ax,[timeouts]
		mov	[results + 6],ax
		mov	ax,RESULT_SEG
		mov	es,ax
		xor	di,di
		mov	si,results
		mov	cx,(8 + N_CFG * N_SPLIT * (CELL + 4) + N_CFG * 2 + 6) / 2
		rep	movsw
		push	cs
		pop	es
		ret

; ---------------------------------------------------------------------------
; Text on G1: line BX (8 rows from TEXT_TOP), string SI, colour [colour].
; 53 cells of 6x8 dots, 3 bytes each; the line is cleared first.
text_line:	push	es
		mov	ax,GV_G1
		mov	es,ax
		imul	di,bx,8 * 160
		add	di,TEXT_TOP * 160
		push	di
		mov	cx,8 * 160 / 2
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
		add	di,160
		inc	bx
		dec	dx
		jnz	.row
		pop	di
		add	di,3
		jmp	.char
.done:		pop	es
		ret

; ---------------------------------------------------------------------------
s_banner	db	'SPRITE BENCH: restart the machine to leave.',13,10,'$'
s_24khz		db	'Select 15 kHz (DIP switch SW1 off) before starting.',13,10,'$'
s_memory	db	'Not enough memory (needs 5000h; load below 2000h).',13,10,'$'
s_running	db	'SPRITE BENCH 2 RUNNING  ABOUT 6 MINUTES',0
s_title		db	'SPRITE BENCH 2  US A SPRITE  MEAN OF 2 X 240 VRTC',0
s_head		db	'COL SIZE      CPU    SGP25    SGP50    SGP75      SGP',0
s_16c		db	'16C',0
s_256c		db	'256C',0
s_legend	db	'SGPNN: NN/100 ON THE SGP  THE REST ON THE CPU AT ONCE',0
s_check16	db	'CHECK SGP=CPU 16C: ',0
s_check256	db	'CHECK SGP=CPU 256C:',0
s_timeout	db	'SGP TIMEOUTS: ',0
s_repeat	db	'2 RUNS: LARGEST DIFFERENCE ',0
s_over		db	'OVER 6 PCT',0
s_pct		db	' PCT',0

sync224		db	0c1h,57h,1ch,0,9fh,0,10h,0fh,25h,0,0e0h,0,2,4
		align	2
fb_bios		dw	4,320,224,0,0,0
frames4		dw	0,0,160,447,0,0,0,0,0,224,0,0,0,0,0,0
		dw	0,0,160,0,0,0,0,0,0,224,0,0,0,0,0,0
frames8		dw	0,0,320,224,0,0,0,0,0,224,0,0,0,0,0,0
palette		dw	PALETTE16, PALETTE16
orig_0100	dw	0
orig_0102	dw	0
tick		dw	0
t0		dw	0
reps		dd	0
elapsed		dw	0
spins		dw	0
spin_rate	dw	0
rest10		dd	0
mean10		dd	0
run		dw	0
timeouts	dw	0
mism		dw	0
cfg		dw	0
split		dw	0
n		dw	0
count		dw	0
bpp_mode	dw	0
bytes		dw	0
words		dw	0
image_words	dw	0
pitch		dw	0
rows		dw	0
skip		dw	0
dest_seg	dw	0
dest_lo		dw	0
chk_o1		dw	0
chk_o2		dw	0
k_sgp		dw	0
x_bytes		dw	0
ink		dw	0
colour		db	0
digits		times	10 db 0
line		times	54 db 0
		align	2
offsets		times	32 dw 0
results		times	8 db 0
cells		times	N_CFG * N_SPLIT * CELL db 0
per_sprite	times	N_CFG * N_SPLIT dd 0
checks		times	N_CFG dw 0
max_diff	dw	0
spin_rates	dw	0, 0
work		times	64 db 0
		align	16
list		times	2048 db 0
image_end:					; below F000h (build.sh)
