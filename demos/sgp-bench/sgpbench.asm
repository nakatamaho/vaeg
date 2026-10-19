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
; SGPBENCH.COM: SGP sprite benchmark for the PC-88VA/VA2 and vaeg
; (README.md). Synthetic images only.
;
; Uses the screen mode of an arcade-game port for the VA (two screens,
; packed 4 bits per pixel, G0 fronts A 00000h and B 09100h, G1 20000h) and
; its SGP command shapes (SET_WORK, SET_SOURCE/SET_DESTINATION with
; start dot, BITBLT 0105h transparent or 0005h opaque, CLS, END).
;
; 1. Timing: each test is repeated for 30, 120 and 480 VRTCs (windows); the
;    screen shows the microseconds per repetition for each window, from the
;    VRTCs that actually elapsed (16683.4 us a VRTC), so that convergence
;    can be seen.
; 2. Correctness: 48 cases (clipping, start dots, Y flip, transparency over
;    a filled box) are drawn on front A and compared with the expected
;    image (built on the PC by gen_data.py); mismatching bytes
;    are counted per case. SPACE switches between front A (the SGP's
;    result) and front B (the expected image).
; The results are also stored at GVRAM 12400h (README.md).
; Never returns: restart the machine to leave.

		cpu	186
		org	100h

%include "bench_data.inc"		; CASES, N_CASES, SOURCES, EXPECTED, FONT

BENCH_SEG	equ	3000h		; this program, copied (SGP address 3xxxxh)
SCRATCH_SEG	equ	4000h		; 64 KiB of main RAM for CPU tests
MEMORY_TOP	equ	5000h
N_WINDOWS	equ	3		; windows: 30, 120, 480 VRTCs
US_VRTC10_LOW	equ	166834 - 2 * 65536	; tenths of a us a VRTC (59.94 Hz):
						; 2 * 65536 + this
FRONT_B		equ	9100h
GV_A		equ	0a000h		; CPU segments: front A, front B, G1
GV_B		equ	0a000h + FRONT_B / 16
GV_G1		equ	0c000h
GV_SRC		equ	0d000h		; G1 region beyond the picture: sources
RESULT_SEG	equ	0b240h		; GVRAM 12400h
TEXT_TOP	equ	58		; first text line (G1 rows); 19 lines end at
					; row 210: the bottom rows of 224 may be
					; outside a monitor's picture
BOX_W		equ	20
BOX_H		equ	19
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
		mov	ax,FRONT_B		; timing blits are shown on front B
		call	show_front
		mov	si,s_title		; status and the column heads
		mov	byte [colour],YELLOW
		xor	bx,bx
		call	text_line
		call	copy_gv_sources
		; timing tests
		mov	word [test_index],0
		mov	si,tests
.test:		cmp	word [si],0
		je	.timed
		push	si
		call	[si]			; setup
		pop	si
		call	test_name
		mov	word [window],0
.window:	push	si
		mov	bx,[window]
		shl	bx,1
		mov	ax,[windows + bx]
		mov	[window_vrtcs],ax
		mov	bx,[si + 2]
		call	measure
		pop	si
		call	test_report
		inc	word [window]
		cmp	word [window],N_WINDOWS
		jb	.window
		add	si,6
		inc	word [test_index]
		jmp	.test
.timed:		call	correctness
		call	store_results
bench_done:	nop				; the results are complete here
		mov	byte [shown],0
.keys:		mov	ax,[tick]
.vrtc:		cmp	ax,[tick]
		je	.vrtc
		in	al,09h			; SPACE (bit 6, active low)
		not	al
		and	al,40h
		mov	ah,[keys_prev]
		mov	[keys_prev],al
		not	ah
		and	al,ah
		jz	.keys
		xor	byte [shown],1
		xor	ax,ax
		cmp	byte [shown],0
		je	.show
		mov	ax,FRONT_B
.show:		call	show_front
		call	show_which
		jmp	.keys

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
; Video: two packed 4-bpp screens over the BIOS's 320x224 set-up.
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
		cli
		mov	bx,0a000h		; all GVRAM zero
.bank:		mov	es,bx
		xor	di,di
		xor	ax,ax
		mov	cx,8000h
		rep	stosw
		add	bx,1000h
		cmp	bx,0e000h
		jb	.bank
		push	cs
		pop	es
		mov	cx,2			; twice: vaeg settles DAC low bits
.twice:		push	cx
		mov	dx,0300h
		mov	si,palette
		mov	cx,32
.palette:	lodsw
		out	dx,ax
		add	dx,2
		loop	.palette
		pop	cx
		loop	.twice
		mov	dx,0100h
		in	ax,dx
		or	ax,0800h
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
		mov	dx,0200h
		mov	si,frames
		mov	cx,32
.reg:		lodsw
		out	dx,ax
		add	dx,2
		loop	.reg
		mov	si,sync224
		mov	al,10h
		call	tsp_cmd
		mov	cx,14
.sync:		lodsb
		call	tsp_prm
		loop	.sync
		sti
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
; Timing: call BX until [window_vrtcs] VRTCs have passed; reps = calls,
; elapsed = the VRTCs that passed (the last call may overrun the window).
measure:	mov	word [reps],0
		mov	word [reps + 2],0
		mov	ax,[tick]
.sync:		cmp	ax,[tick]
		je	.sync
		mov	ax,[tick]
		mov	[t0],ax
.loop:		push	bx
		call	bx
		pop	bx
		add	word [reps],1
		adc	word [reps + 2],0
		mov	ax,[tick]
		sub	ax,[t0]
		cmp	ax,[window_vrtcs]
		jb	.loop
		mov	[elapsed],ax
		ret

; Start the line of test [test_index] (descriptor SI) with its name.
test_name:	push	si
		mov	di,line
		mov	cx,53
		mov	al,' '
		rep	stosb
		mov	di,line
		mov	ax,[test_index]
		inc	ax
		xor	dx,dx
		mov	cl,2
		call	put_number
		inc	di
		mov	si,[si + 4]
.name:		lodsb
		or	al,al
		jz	.named
		stosb
		jmp	.name
.named:		mov	byte [line + 53],0
		pop	si
		ret

; Window [window] of test [test_index]: store reps and tenths of a us a
; repetition (elapsed * 166834 / reps), add the column, show the line.
test_report:	push	si
		mov	ax,[elapsed]		; DX:AX = elapsed * 166834
		mov	cx,US_VRTC10_LOW
		mul	cx
		mov	cx,[elapsed]
		shl	cx,1
		add	dx,cx
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
		mov	[us10],ax
		mov	[us10 + 2],dx
		mov	bx,[test_index]		; results + 8 + (test * 3 + window) * 8
		imul	bx,bx,N_WINDOWS
		add	bx,[window]
		shl	bx,3
		mov	ax,[reps]
		mov	[results + 8 + bx],ax
		mov	ax,[reps + 2]
		mov	[results + 10 + bx],ax
		mov	ax,[us10]
		mov	[results + 12 + bx],ax
		mov	ax,[us10 + 2]
		mov	[results + 14 + bx],ax
		imul	di,[window],10		; columns 23, 33, 43
		add	di,line + 23
		mov	ax,[us10]
		mov	dx,[us10 + 2]
		mov	cx,10
		call	div32			; DX:AX us, BX tenths
		push	bx
		mov	cl,7
		call	put_number
		mov	al,'.'
		stosb
		pop	ax
		add	al,'0'
		stosb
		mov	byte [colour],WHITE
		mov	si,line
		mov	bx,[test_index]
		inc	bx
		call	text_line
		pop	si
		ret

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
		sub	ax,si			; digits written
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
; SGP: lists in this segment (SGP addresses 3xxxxh).
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
sgp_run:	call	sgp_submit
		jmp	sgp_wait

; ES:DI list start: SET_WORK.
list_begin:	mov	di,list
		mov	ax,3
		stosw
		mov	ax,work
		stosw
		mov	ax,BENCH_SEG >> 12
		stosw
		ret
list_end:	mov	ax,1
		stosw
		ret

; One piece as planes_emit: source e_src (32-bit SGP address of a 16x16
; image, pitch 8 or e_pitch), its part e_left/e_top/e_w/e_h, Y flip e_fy,
; to e_x + e_left, e_y + e_top on the page at e_page (32-bit), mode e_mode.
emit:		mov	ax,4
		stosw
		mov	ax,[e_left]
		and	ax,3
		shl	ax,4
		or	ax,1
		stosw
		mov	ax,[e_w]
		stosw
		mov	ax,[e_h]
		stosw
		mov	ax,[e_pitch]
		cmp	byte [e_fy],0
		je	.pitch
		neg	ax			; read upwards: encoded two bytes
		sub	ax,2			; shorter (FFF6h for -8; undefined on
					; hardware after the first line, M128)
.pitch:		stosw
		mov	ax,[e_top]
		cmp	byte [e_fy],0
		je	.y
		xor	ax,15
.y:		mul	word [e_pitch]
		mov	bx,[e_left]
		shr	bx,1
		add	ax,bx
		xor	dx,dx
		add	ax,[e_src]
		adc	dx,[e_src + 2]
		and	ax,0fffeh
		stosw
		mov	ax,dx
		stosw
		mov	ax,5
		stosw
		mov	ax,[e_x]
		add	ax,[e_left]
		mov	bx,ax
		and	ax,3
		shl	ax,4
		or	ax,1
		stosw
		mov	ax,[e_w]
		stosw
		mov	ax,[e_h]
		stosw
		mov	ax,160
		stosw
		mov	ax,[e_y]
		add	ax,[e_top]
		mov	cx,160
		mul	cx
		shr	bx,1
		add	ax,bx
		adc	dx,0
		add	ax,[e_page]
		adc	dx,[e_page + 2]
		and	ax,0fffeh
		stosw
		mov	ax,dx
		stosw
		mov	ax,7
		stosw
		mov	ax,[e_mode]
		stosw
		ret

; Pieces for the timing tests: 32 pieces of the current e_* shape at the
; grid positions of front B (rows 4-43); CX pieces.
emit_grid:	xor	bp,bp
.piece:		push	cx
		mov	ax,bp
		and	ax,15
		imul	ax,ax,18
		add	ax,16
		add	ax,[grid_dot]
		mov	[e_x],ax
		mov	ax,bp
		shr	ax,4
		imul	ax,ax,20
		add	ax,4
		mov	[e_y],ax
		call	emit
		inc	bp
		pop	cx
		loop	.piece
		ret

; The default timing shape: source 0 from RAM, 16x16, transparent, front B.
shape_default:	mov	ax,SOURCES			; below 10000h: SGP 3:SOURCES
		mov	[e_src],ax
		mov	word [e_src + 2],BENCH_SEG >> 12
		mov	word [e_pitch],8
		mov	word [e_left],0
		mov	word [e_top],0
		mov	word [e_w],16
		mov	word [e_h],16
		mov	byte [e_fy],0
		mov	word [e_mode],0105h
		mov	word [e_page],FRONT_B
		mov	word [e_page + 2],20h
		mov	word [grid_dot],0
		ret

; Sources also in GVRAM, at 230000h (CPU D000:0000), for the GVRAM-source test.
copy_gv_sources:
		mov	ax,GV_SRC
		mov	es,ax
		xor	di,di
		mov	si,SOURCES
		mov	cx,4 * 128 / 2
		rep	movsw
		push	cs
		pop	es
		ret

; ---------------------------------------------------------------------------
; Tests: setup, one repetition, name.
tests:		dw	setup_none, op_loop, n_loop
		dw	setup_none, op_cpu_gv, n_cpu_gv
		dw	setup_none, op_cpu_ram, n_cpu_ram
		dw	setup_none, op_gv_copy, n_gv_copy
		dw	setup_empty, op_list, n_empty
		dw	setup_cls, op_list, n_cls
		dw	setup_cls8, op_list, n_cls8
		dw	setup_one, op_list, n_one
		dw	setup_32, op_list, n_32
		dw	setup_32dot, op_list, n_32dot
		dw	setup_32fy, op_list, n_32fy
		dw	setup_32opaque, op_list, n_32opaque
		dw	setup_32cell, op_list, n_32cell
		dw	setup_32gv, op_list, n_32gv
		dw	setup_32, op_par_gv, n_par_gv
		dw	setup_32, op_par_ram, n_par_ram
		dw	setup_32, op_par_loop, n_par_loop
		dw	0
N_TESTS		equ	17

setup_none:	ret
setup_empty:	call	list_begin
		jmp	list_end
setup_cls:	mov	cx,35840 / 2
		jmp	cls_list
setup_cls8:	mov	cx,1280 / 2
cls_list:	call	list_begin
		mov	ax,6			; SET_COLOR 0
		stosw
		xor	ax,ax
		stosw
		mov	ax,0ah			; CLS front B, CX words
		stosw
		mov	ax,FRONT_B
		stosw
		mov	ax,20h
		stosw
		mov	ax,cx
		stosw
		xor	ax,ax
		stosw
		jmp	list_end
setup_one:	call	shape_default
		call	list_begin
		mov	cx,1
		call	emit_grid
		jmp	list_end
setup_32:	call	shape_default
grid32:		call	list_begin
		mov	cx,32
		call	emit_grid
		jmp	list_end
setup_32dot:	call	shape_default
		mov	word [grid_dot],1
		jmp	grid32
setup_32fy:	call	shape_default
		mov	byte [e_fy],1
		jmp	grid32
setup_32opaque:	call	shape_default
		mov	word [e_mode],0005h
		jmp	grid32
setup_32cell:	call	shape_default
		mov	word [e_left],4
		mov	word [e_top],4
		mov	word [e_w],8
		mov	word [e_h],8
		jmp	grid32
setup_32gv:	call	shape_default
		mov	word [e_src],0
		mov	word [e_src + 2],23h
		jmp	grid32

op_list:	mov	ax,list
		jmp	sgp_run
op_loop:	mov	cx,1000
.l:		loop	.l
		ret
op_cpu_gv:	push	es
		mov	ax,GV_A			; front A is not shown meanwhile
		mov	es,ax
		xor	di,di
		xor	ax,ax
		mov	cx,35840 / 2
		rep	stosw
		pop	es
		ret
op_cpu_ram:	push	es
		mov	ax,SCRATCH_SEG
		mov	es,ax
		xor	di,di
		xor	ax,ax
		mov	cx,35840 / 2
		rep	stosw
		pop	es
		ret
op_gv_copy:	push	ds
		push	es
		mov	ax,GV_A
		mov	ds,ax
		add	ax,8192 / 16
		mov	es,ax
		xor	si,si
		xor	di,di
		mov	cx,8192 / 2
		rep	movsw
		pop	es
		pop	ds
		ret
op_par_gv:	mov	ax,list
		call	sgp_submit
		call	op_cpu_gv
		jmp	sgp_wait
op_par_ram:	mov	ax,list
		call	sgp_submit
		call	op_cpu_ram
		jmp	sgp_wait
op_par_loop:	mov	ax,list
		call	sgp_submit
		mov	cx,4000
.l:		loop	.l
		jmp	sgp_wait

; ---------------------------------------------------------------------------
; Correctness: front A cleared, boxes filled, all cases in one list.
correctness:	mov	ax,GV_A
		call	clear_front
		mov	ax,GV_B
		call	clear_front
		xor	bp,bp			; fill the boxes
.box:		mov	ax,GV_A
		mov	es,ax
		call	box_origin		; DI: first byte
		mov	al,0edh
		test	bp,1
		jz	.fill
		mov	al,0bch
.fill:		mov	dx,BOX_H - 1
.row:		push	di
		mov	cx,BOX_W / 2
		rep	stosb
		pop	di
		add	di,160
		ror	al,4
		dec	dx
		jnz	.row
		inc	bp
		cmp	bp,N_CASES
		jb	.box
		push	cs
		pop	es
		call	list_begin
		mov	word [e_pitch],8
		mov	word [e_page],0
		mov	word [e_page + 2],20h
		mov	si,CASES
		mov	cx,N_CASES
.case:		push	cx
		lodsw				; source image
		xor	ax,5a5ah		; (each word: gen_data.py)
		shl	ax,7
		add	ax,SOURCES			; below 10000h: SGP 3:SOURCES
		mov	[e_src],ax
		mov	word [e_src + 2],BENCH_SEG >> 12
		lodsw
		xor	ax,5a5ah
		mov	[e_left],ax
		lodsw
		xor	ax,5a5ah
		mov	[e_top],ax
		lodsw
		xor	ax,5a5ah
		mov	[e_w],ax
		lodsw
		xor	ax,5a5ah
		mov	[e_h],ax
		lodsw
		xor	ax,5a5ah
		mov	[e_x],ax
		lodsw
		xor	ax,5a5ah
		mov	[e_y],ax
		lodsw				; bit 0 Y flip, bit 1 opaque
		xor	ax,5a5ah
		mov	byte [e_fy],al
		and	byte [e_fy],1
		mov	word [e_mode],0105h
		test	al,2
		jz	.mode
		mov	word [e_mode],0005h
.mode:		push	si
		call	emit
		pop	si
		pop	cx
		loop	.case
		call	list_end
		mov	ax,list
		call	sgp_run
		; count mismatching bytes a box
		mov	ax,GV_A
		mov	es,ax
		xor	bp,bp
		mov	word [failed],0
.compare:	call	box_origin
		mov	si,di
		add	si,EXPECTED
		xor	bx,bx
		mov	dx,BOX_H - 1
.crow:		push	si
		push	di
		mov	cx,BOX_W / 2
.cbyte:		mov	al,[si]
		cmp	al,[es:di]
		je	.same
		inc	bx
.same:		inc	si
		inc	di
		loop	.cbyte
		pop	di
		pop	si
		add	si,160
		add	di,160
		dec	dx
		jnz	.crow
		mov	di,bp
		shl	di,1
		mov	[mismatch + di],bx
		or	bx,bx
		jz	.ok
		inc	word [failed]
.ok:		inc	bp
		cmp	bp,N_CASES
		jb	.compare
		; the expected image on front B
		mov	ax,GV_B
		mov	es,ax
		xor	di,di
		mov	si,EXPECTED
		mov	cx,EXPECTED_BYTES / 2
		rep	movsw
		push	cs
		pop	es
		xor	ax,ax			; show the result
		call	show_front
		call	case_report
		jmp	show_which

; DI = offset of box BP's first byte (16 boxes a row).
box_origin:	mov	ax,bp
		shr	ax,4
		imul	ax,ax,BOX_H * 160
		mov	di,ax
		mov	ax,bp
		and	ax,15
		imul	ax,ax,BOX_W / 2
		add	di,ax
		ret

clear_front:	mov	es,ax
		xor	di,di
		xor	ax,ax
		mov	cx,35840 / 2
		rep	stosw
		push	cs
		pop	es
		ret

; "CASES 48 FAIL n  i:count ..." (first failing cases).
case_report:	mov	di,line
		mov	cx,53
		mov	al,' '
		rep	stosb
		mov	di,line
		mov	si,s_cases
.label:		lodsb
		or	al,al
		jz	.labelled
		stosb
		jmp	.label
.labelled:	mov	ax,[failed]
		xor	dx,dx
		xor	cl,cl
		call	put_number
		mov	al,' '
		stosb
		xor	bp,bp
.each:		cmp	di,line + 46
		jae	.end
		mov	bx,bp
		shl	bx,1
		mov	ax,[mismatch + bx]
		or	ax,ax
		jz	.next
		push	ax
		mov	ax,bp
		xor	dx,dx
		mov	cl,2
		call	put_number
		mov	al,':'
		stosb
		pop	ax
		xor	dx,dx
		xor	cl,cl
		call	put_number
		mov	al,' '
		stosb
.next:		inc	bp
		cmp	bp,N_CASES
		jb	.each
.end:		mov	byte [di],0
		mov	byte [colour],GREEN
		cmp	word [failed],0
		je	.colour
		mov	byte [colour],RED
.colour:	cmp	word [timeouts],0
		je	.print
		mov	si,s_timeout		; overrides: the SGP had to be stopped
		mov	di,line
.t:		lodsb
		stosb
		or	al,al
		jnz	.t
		mov	byte [colour],RED
.print:		mov	si,line
		mov	bx,N_TESTS + 1
		jmp	text_line

show_which:	mov	si,s_actual
		cmp	byte [shown],0
		je	.text
		mov	si,s_expected
.text:		mov	byte [colour],YELLOW
		xor	bx,bx
		jmp	text_line

; Results for the checker at GVRAM 12400h: 'SGPB', tests, cases, timeouts,
; windows, then per test and window reps and tenths of us (32-bit each),
; then per case counts.
store_results:	mov	word [results],'SG'
		mov	word [results + 2],'PB'
		mov	word [results + 4],N_TESTS | (N_CASES << 8)
		mov	byte [results + 7],N_WINDOWS
		mov	al,[timeouts]
		mov	[results + 6],al
		mov	si,mismatch
		mov	di,results + 8 + N_TESTS * N_WINDOWS * 8
		mov	cx,N_CASES
		rep	movsw
		mov	ax,RESULT_SEG
		mov	es,ax
		xor	di,di
		mov	si,results
		mov	cx,(8 + N_TESTS * N_WINDOWS * 8 + N_CASES * 2) / 2
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
		xor	ah,0a5h		; XOR A5h: gen_data.py)
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
s_banner	db	'SGP BENCH: restart the machine to leave.',13,10,'$'
s_24khz		db	'Select 15 kHz (DIP switch SW1 off) before starting.',13,10,'$'
s_memory	db	'Not enough memory (needs 5000h; load below 2000h).',13,10,'$'
s_title		db	'BENCH3 RUNNING  US/REP   30 VRTC  120 VRTC  480 VRTC',0
s_cases		db	'CASES 48 FAIL ',0
s_timeout	db	'SGP TIMEOUT: STOPPED (SEE DOC)',0
s_actual	db	'BENCH3 A=SGP    US/REP   30 VRTC  120 VRTC  480 VRTC',0
s_expected	db	'BENCH3 B=EXPECT US/REP   30 VRTC  120 VRTC  480 VRTC',0
n_loop		db	'CPU LOOP X1000',0
n_cpu_gv	db	'CPU STOSW GV 35840',0
n_cpu_ram	db	'CPU STOSW RAM 35840',0
n_gv_copy	db	'CPU MOVSW GV-GV 8192',0
n_empty		db	'SGP EMPTY LIST',0
n_cls		db	'SGP CLS 35840',0
n_cls8		db	'SGP CLS 1280',0
n_one		db	'SGP 1X16X16',0
n_32		db	'SGP 32X16X16',0
n_32dot		db	'SGP 32X16X16 DOT1',0
n_32fy		db	'SGP 32X16X16 YFLIP',0
n_32opaque	db	'SGP 32X16X16 OPAQUE',0
n_32cell	db	'SGP 32X8X8',0
n_32gv		db	'SGP 32X16X16 GV SRC',0
n_par_gv	db	'09 + CPU STOSW GV',0
n_par_ram	db	'09 + CPU STOSW RAM',0
n_par_loop	db	'09 + CPU LOOP X4000',0

sync224		db	0c1h,57h,1ch,0,9fh,0,10h,0fh,25h,0,0e0h,0,2,4
		align	2
fb_bios		dw	4,320,224,0,0,0
frames		dw	0,0,160,447,0,0,0,0,0,224,0,0,0,0,0,0
		dw	0,0,160,0,0,0,0,0,0,224,0,0,0,0,0,0
palette		dw	PALETTE16, PALETTE16
tick		dw	0
t0		dw	0
reps		dd	0
test_index	dw	0
timeouts	dw	0
failed		dw	0
e_src		dd	0
e_page		dd	0
e_pitch		dw	8
e_left		dw	0
e_top		dw	0
e_w		dw	0
e_h		dw	0
e_x		dw	0
e_y		dw	0
e_mode		dw	0
grid_dot	dw	0
ink		dw	0
e_fy		db	0
colour		db	0
shown		db	0
keys_prev	db	0
digits		times	10 db 0
line		times	54 db 0
		align	2
mismatch	times	N_CASES dw 0
results		times	8 + N_TESTS * N_WINDOWS * 8 + N_CASES * 2 db 0
windows		dw	30, 120, 480
window		dw	0
window_vrtcs	dw	0
elapsed		dw	0
us10		dd	0
work		times	64 db 0
		align	16
list		times	2048 db 0
image_end:					; below F000h (build.sh)
