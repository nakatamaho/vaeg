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
MEM_PARAS	equ	2000h
SRC_PARA	equ	1000h		; the sources at CS + 1000h
LIST		equ	0c000h		; SGP list and work area (this segment)
WORK		equ	0e000h

%include "common.inc"
%include "sgp.inc"

; ---------------------------------------------------------------------------
; A depth's sources.
prepare_depth:	call	sgp_sources
		clc
		ret

; ---------------------------------------------------------------------------
; The list at LIST: SET_WORK, then for each sprite SET_SOURCE, SET_DEST and
; BITBLT 0105h, END. Positions from a fixed generator per size, the same at
; every depth and source; kinds in turn.
build_list:	call	begin_list
		mov	al,[loc_i]
		mov	[sgp_vram],al
		call	sgp_begin
		xor	bp,bp			; sprite
.sprite:	call	random_xy		; CX x, DX y
		call	sgp_sprite
		inc	bp
		cmp	bp,[count]
		jb	.sprite
		jmp	sgp_end

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

; ---------------------------------------------------------------------------
s_banner	db	'SGPBENCH: SGP sprites, about 3 minutes. ESC stops.',13,10,'$'
s_memory	db	'Not enough memory (needs 128 KB after the program).',13,10,'$'
s_head2		db	'SPRITES A VRTC, 180 VRTCS A TEST, BITBLT 0105H', 0
s_head3		db	'SCREEN  COL.  SOURCE    8X8  16X16  32X32  64X64', 0
s_loc0		db	'RAM', 0
s_loc1		db	'GVRAM', 0
		align	2
lists		dw	0
prev		db	0
		align	2
image_end:
IMAGE_TOP	equ	image_end - $$ + 100h
%if IMAGE_TOP > LIST
%error the program reaches its SGP list
%endif
