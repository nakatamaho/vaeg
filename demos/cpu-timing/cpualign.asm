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
; CPUALIGN.COM: main-CPU loops with their first instruction at an even and
; at an odd address, on a PC-88VA/VA2/VA3 and vaeg (README.md). Each row
; repeats a chunk (64 iterations of a loop, or 64 straight instructions)
; for FRAMES VRTCs and counts the chunks, once with the loop at an even
; address and once at an odd one. Only the VRTC interrupt runs meanwhile.
; The table (machine from F000:FFFE, display frequency) goes to the
; screen through DOS and to CPUALIGN.TXT.

		cpu	186
		org	100h

FRAMES		equ	120		; VRTCs a measurement
RAM_PARA	equ	1000h		; main RAM block at CS + 1000h (8 KB)
MEM_PARAS	equ	1200h
GV_SEG		equ	0a000h		; GVRAM (single plane) at A0000h
TV_SEG		equ	0a700h		; TVRAM offset 7000h (bank 1)

		jmp	start

; ---------------------------------------------------------------------------
; Loop bodies. A chunk sets DI = 0 (and SI = 1000h), CX = 64, then runs
; the body until CX is 0, with DS = ES = the row's segment.
%macro B_REG 0
		add	ax,bx
		inc	di
%endmacro
%macro B_RD_ES 0
		mov	al,[es:di]
		inc	di
%endmacro
%macro B_RD 0
		mov	al,[di]
		inc	di
%endmacro
%macro B_RDW 0
		mov	ax,[di]
		add	di,2
%endmacro
%macro B_WR 0
		mov	[di],al
		inc	di
%endmacro
%macro B_WRW 0
		mov	[di],ax
		add	di,2
%endmacro
%macro B_SPR 0				; CPUBENCH's LOOP at 16 colours
		lodsw
		and	al,[es:di]
		or	al,ah
		stosb
%endmacro

; LOOP name, body, parity (0 even, 1 odd): a chunk routine.
%macro LOOP 3
		align	2, db 90h
%1:		xor	di,di
		mov	si,1000h
		mov	cx,64
%if %3
		nop				; the loop at an odd address
%else
		nop
		nop
%endif
%%l:		%2
		dec	cx
		jnz	%%l
		ret
%endmacro

; STRAIGHT name, parity, kind: 64 straight instructions, then RET.
; kind 0: MOV AX,imm16 (3 bytes); 1: MOV WORD [DI+d16],imm16 (6 bytes).
%macro STRAIGHT 3
		align	2, db 90h
%1:		xor	di,di
		jmp	%%go
%if %2
		nop
%else
		nop
		nop
%endif
%%go:
%assign i 0
%rep 64
%if %3 == 0
		mov	ax,1234h + i
%else
		mov	word [di + 0100h + i * 2],1234h + i
%endif
%assign i i + 1
%endrep
		ret
%endmacro

; ---------------------------------------------------------------------------
start:		cld
		mov	dx,s_banner
		call	dos_print
		mov	ax,cs
		add	ax,MEM_PARAS
		cmp	[0002h],ax
		jae	.room
		mov	dx,s_memory
		call	dos_print
		mov	ax,4c01h
		int	21h
.room:		mov	ax,cs
		add	ax,RAM_PARA
		mov	[seg_ram],ax
		call	heading
		mov	dx,0153h		; the memory map, restored at the end
		in	al,dx
		mov	[map_old],al
		cli
		xor	ax,ax
		mov	es,ax
		mov	ax,[es:0ah * 4]
		mov	[old_vrtc],ax
		mov	ax,[es:0ah * 4 + 2]
		mov	[old_vrtc + 2],ax
		mov	word [es:0ah * 4],vrtc
		mov	[es:0ah * 4 + 2],cs
		mov	dx,18ah
		in	al,dx
		mov	[old_mask],al
		mov	al,0fbh			; only VRTC (IR2)
		out	dx,al
		push	cs
		pop	es
		sti
		xor	bx,bx			; the rows
.row:		push	bx
		call	measure_row
		pop	bx
		add	bx,ROW_SIZE
		cmp	bx,N_ROWS * ROW_SIZE
		jb	.row
		cli
		mov	dx,0153h
		mov	al,[map_old]
		out	dx,al
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
		call	print_all
		call	write_file
		mov	ax,4c00h
		int	21h

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
; Row BX: its two chunks measured, its line appended to OUT.
measure_row:	mov	si,[rows + bx + 6]	; segment selector
		mov	ax,[seg_ram]
		mov	cl,[map_old]
		cmp	si,1
		jne	.not_gv
		mov	ax,GV_SEG
		mov	cl,54h			; GVRAM single plane at A0000h
.not_gv:	cmp	si,2
		jne	.not_tv
		mov	ax,TV_SEG
		mov	cl,[map_old]
		and	cl,0f0h
		or	cl,1			; TVRAM (bank 1)
.not_tv:	mov	[target],ax
		mov	[map_row],cl
		push	bx
		mov	ax,[rows + bx + 2]
		call	measure
		pop	bx
		mov	[n_even],ax
		push	bx
		mov	ax,[rows + bx + 4]
		call	measure
		pop	bx
		mov	[n_odd],ax
		mov	si,[rows + bx]		; the line
		mov	di,[out_ptr]
		call	copy_pad
		mov	ax,[n_even]
		call	num6
		mov	ax,[n_odd]
		call	num6
		mov	ax,[n_odd]		; odd / even
		mov	bx,1000
		mul	bx
		mov	bx,[n_even]
		or	bx,bx
		jz	.zero
		cmp	dx,bx
		jae	.zero
		div	bx
		jmp	.ratio
.zero:		xor	ax,ax
.ratio:		call	ratio
		mov	ax,0a0dh
		stosw
		mov	[out_ptr],di
		ret

; Chunks of routine AX in FRAMES VRTCs (AX), DS = ES = [target], port
; 153h = [map_row] meanwhile.
measure:	mov	[chunk],ax
		mov	ax,[tick]
.edge:		cmp	ax,[tick]
		je	.edge
		mov	ax,[tick]
		mov	[t0],ax
		mov	word [count],0
		mov	dx,0153h
		mov	al,[map_row]
		out	dx,al
		test	al,10h			; single plane: CPU data written as
		jz	.mode			; is (as demos/sprite-bench)
		mov	dx,0580h
		mov	al,10h
		out	dx,al
		out	dx,al
.mode:		mov	ax,[target]
		mov	es,ax
.c:		mov	ds,[cs:target]
		call	[cs:chunk]
		push	cs
		pop	ds
		inc	word [count]
		mov	ax,[tick]
		sub	ax,[t0]
		cmp	ax,FRAMES
		jb	.c
		push	cs
		pop	es
		mov	dx,0153h
		mov	al,[map_old]
		out	dx,al
		mov	ax,[count]
		ret

; ---------------------------------------------------------------------------
heading:	push	es
		mov	ax,0f000h
		mov	es,ax
		mov	ax,[es:0fffeh]
		pop	es
		mov	si,s_va
		cmp	ax,0ffffh
		je	.m
		mov	si,s_va2
		cmp	ax,0fffeh
		je	.m
		mov	si,s_va91
		cmp	ax,0fffdh
		je	.m
		mov	si,s_unknown
.m:		mov	di,OUT
		push	si
		mov	si,s_title
		call	copy0
		pop	si
		call	copy0
		mov	si,s_15
		mov	dx,01cbh		; port 1CBh bit 3: 24.8 kHz
		in	al,dx
		test	al,08h
		jz	.k
		mov	si,s_24
.k:		call	copy0
		mov	ax,0a0dh
		stosw
		mov	si,s_head
		call	copy0
		mov	ax,0a0dh
		stosw
		mov	[out_ptr],di
		ret

copy0:		lodsb
		or	al,al
		jz	.done
		stosb
		jmp	copy0
.done:		ret

; SI (0-terminated) at DI, padded to 18 characters.
copy_pad:	mov	cx,18
.c:		lodsb
		or	al,al
		jz	.pad
		stosb
		loop	.c
		ret
.pad:		mov	al,' '
		rep	stosb
		ret

; AX right-aligned in 6 characters at DI.
num6:		push	bx
		push	cx
		push	dx
		add	di,6
		push	di
		mov	cx,6
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

; AX thousandths as '  d.ddd' at DI.
ratio:		push	bx
		mov	word [di],'  '
		add	di,2
		xor	dx,dx
		mov	bx,1000
		div	bx
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

; OUT to the screen (DOS, a character at a time) and to CPUALIGN.TXT.
print_all:	mov	si,OUT
.c:		cmp	si,[out_ptr]
		jae	.done
		lodsb
		mov	dl,al
		mov	ah,02h
		int	21h
		jmp	.c
.done:		ret

write_file:	mov	ah,3ch
		xor	cx,cx
		mov	dx,s_file
		int	21h
		jc	.fail
		mov	bx,ax
		mov	cx,[out_ptr]
		sub	cx,OUT
		mov	dx,OUT
		mov	ah,40h
		int	21h
		jc	.fail
		mov	ah,3eh
		int	21h
		jc	.fail
		ret
.fail:		mov	dx,s_nowrite
		jmp	dos_print

dos_print:	mov	ah,09h
		int	21h
		ret

; ---------------------------------------------------------------------------
; The chunks.
		LOOP	c_reg_e, B_REG, 0
		LOOP	c_reg_o, B_REG, 1
		LOOP	c_rdes_e, B_RD_ES, 0
		LOOP	c_rdes_o, B_RD_ES, 1
		LOOP	c_rd_e, B_RD, 0
		LOOP	c_rd_o, B_RD, 1
		LOOP	c_rdw_e, B_RDW, 0
		LOOP	c_rdw_o, B_RDW, 1
		LOOP	c_wr_e, B_WR, 0
		LOOP	c_wr_o, B_WR, 1
		LOOP	c_wrw_e, B_WRW, 0
		LOOP	c_wrw_o, B_WRW, 1
		LOOP	c_spr_e, B_SPR, 0
		LOOP	c_spr_o, B_SPR, 1
		STRAIGHT c_movr_e, 0, 0
		STRAIGHT c_movr_o, 1, 0
		STRAIGHT c_movm_e, 0, 1
		STRAIGHT c_movm_o, 1, 1

; Rows: name, even chunk, odd chunk, memory (0 main RAM, 1 GVRAM, 2 TVRAM).
ROW_SIZE	equ	8
rows		dw	s_reg, c_reg_e, c_reg_o, 0
		dw	s_rdes, c_rdes_e, c_rdes_o, 0
		dw	s_rd, c_rd_e, c_rd_o, 0
		dw	s_rdw, c_rdw_e, c_rdw_o, 0
		dw	s_wr, c_wr_e, c_wr_o, 0
		dw	s_wrw, c_wrw_e, c_wrw_o, 0
		dw	s_spr, c_spr_e, c_spr_o, 0
		dw	s_gv_rd, c_rd_e, c_rd_o, 1
		dw	s_gv_wr, c_wr_e, c_wr_o, 1
		dw	s_gv_wrw, c_wrw_e, c_wrw_o, 1
		dw	s_gv_spr, c_spr_e, c_spr_o, 1
		dw	s_tv_rd, c_rd_e, c_rd_o, 2
		dw	s_tv_wr, c_wr_e, c_wr_o, 2
		dw	s_movr, c_movr_e, c_movr_o, 0
		dw	s_movm, c_movm_e, c_movm_o, 0
		dw	s_movg, c_movm_e, c_movm_o, 1
N_ROWS		equ	($ - rows) / ROW_SIZE

s_banner	db	'CPUALIGN: about 1 minute.',13,10,'$'
s_memory	db	'Not enough memory (needs 72 KB).',13,10,'$'
s_nowrite	db	'CPUALIGN.TXT could not be written.',13,10,'$'
s_file		db	'CPUALIGN.TXT', 0
s_title		db	'CPUALIGN  ', 0
s_va		db	'PC-88VA', 0
s_va2		db	'PC-88VA2/VA3', 0
s_va91		db	'PC-88VA+VA-91', 0
s_unknown	db	'UNKNOWN MACHINE', 0
s_15		db	'  15 KHZ  CHUNKS IN 120 VRTC', 0
s_24		db	'  24.8 KHZ  CHUNKS IN 120 VRTC', 0
s_head		db	'LOOP (64 TIMES)     EVEN   ODD  ODD/EVEN', 0
s_reg		db	'REG ADD', 0
s_rdes		db	'RAM READ B ES:', 0
s_rd		db	'RAM READ B', 0
s_rdw		db	'RAM READ W', 0
s_wr		db	'RAM WRITE B', 0
s_wrw		db	'RAM WRITE W', 0
s_spr		db	'RAM LODSW AND OR', 0
s_gv_rd		db	'GVRAM READ B', 0
s_gv_wr		db	'GVRAM WRITE B', 0
s_gv_wrw	db	'GVRAM WRITE W', 0
s_gv_spr	db	'GVRAM LODSW AND OR', 0
s_tv_rd		db	'TVRAM READ B', 0
s_tv_wr		db	'TVRAM WRITE B', 0
s_movr		db	'64 MOV AX,IMM', 0
s_movm		db	'64 MOV RAM,IMM', 0
s_movg		db	'64 MOV GVRAM,IMM', 0

		align	2
old_vrtc	dw	0, 0
seg_ram		dw	0
target		dw	0
chunk		dw	0
tick		dw	0
t0		dw	0
count		dw	0
n_even		dw	0
n_odd		dw	0
out_ptr		dw	0
old_mask	db	0
map_old		db	0
map_row		db	0
		align	2
OUT:
