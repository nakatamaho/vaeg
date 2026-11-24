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
; MIXBENCH.COM: the SGP and the CPU drawing sprites together on a
; PC-88VA, VA2 or VA3 (and vaeg), the same tests as SGPBENCH.COM and
; CPUBENCH.COM (README.md) with each list shared: for each screen, colour
; depth, SGP share (a quarter, a half, three quarters) and size, the
; sprites drawn in FRAMES VRTCs at the same scattered positions. A round
; starts the SGP on its share (BITBLT 0105h, sources in main RAM), the
; CPU then draws the rest with its compiled routines (CPUn.DAT, as
; CPUBENCH's COMPLD), and the round ends when both have finished; only
; rounds that end inside the FRAMES VRTCs count. Sprites go to the SGP
; in blocks of four (one of each character): block b when b mod 4 is
; below the share in quarters. Only the VRTC interrupt is taken during a
; test (it counts the VRTCs).

		cpu	186
		org	100h

		jmp	start
%include "data.inc"			; PAL4, PAL8, PAL16, FONT, SPR_OFS, SPRITES

%define PROGRAM	'MIXBENCH'
%define N_WAYS	3		; SGP shares: 1/4, 1/2, 3/4
DAT_PARA	equ	1000h		; a .DAT file at CS + 1000h
DAT_PARAS	equ	2000h		; its room
SRC_PARA	equ	3000h		; the SGP's sources at CS + 3000h
MEM_PARAS	equ	4000h
LIST		equ	0c000h		; SGP list and work area (this segment)
WORK		equ	0e000h

%include "common.inc"
%include "sgp.inc"
%include "cpu.inc"

; ---------------------------------------------------------------------------
; A depth's CPUn.DAT and SGP sources. CF: CPUn.DAT not found.
prepare_depth:	call	load_dat
		jc	.r
		call	sgp_sources
		clc
.r:		ret

; ---------------------------------------------------------------------------
; The sprites shared: the SGP's at LIST, the CPU's in sprite_tab.
; Positions and kinds as SGPBENCH's and CPUBENCH's.
build_list:	call	begin_list
		mov	byte [sgp_vram],0
		call	sgp_begin
		mov	[list_ptr],di
		mov	word [tab_ptr],sprite_tab
		mov	word [sgp_n],0
		mov	word [cpu_n],0
		xor	bp,bp
.sprite:	call	random_xy		; CX x, DX y
		mov	ax,bp			; block (BP / 4) mod 4 <= loc_i:
		shr	ax,2			; the SGP's
		and	al,3
		cmp	al,[loc_i]
		ja	.cpu
		mov	di,[list_ptr]
		call	sgp_sprite
		mov	[list_ptr],di
		inc	word [sgp_n]
		jmp	.next
.cpu:		mov	di,[tab_ptr]
		call	cpu_entry
		mov	[tab_ptr],di
		inc	word [cpu_n]
.next:		inc	bp
		cmp	bp,[count]
		jb	.sprite
		mov	di,[list_ptr]
		jmp	sgp_end

; ---------------------------------------------------------------------------
; Rounds for FRAMES VRTCs (counted by the VRTC interrupt, the only one
; unmasked): the SGP started on its list, the CPU's sprites drawn, the
; SGP waited for. DX:AX sprites of the rounds that ended inside.
measure:	call	vrtc_on
		mov	word [drawn],0
		mov	word [drawn + 2],0
.round:		call	submit
		mov	si,sprite_tab
		mov	cx,[cpu_n]
.sprite:	push	cx
		push	si
		DRAW_COMPILED
		pop	si
		pop	cx
		add	si,ENTRY
		loop	.sprite
		call	sgp_wait
		mov	ax,[ticks]
		sub	ax,[t0]
		cmp	ax,FRAMES
		jae	.end
		mov	ax,[count]
		add	[drawn],ax
		adc	word [drawn + 2],0
		jmp	.round
.end:		call	vrtc_off
		mov	ax,[drawn]
		mov	dx,[drawn + 2]
		ret

; ---------------------------------------------------------------------------
s_banner	db	'MIXBENCH: SGP and CPU sprites together, about 4 minutes.',13,10
		db	'ESC stops.',13,10,'$'
s_memory	db	'Not enough memory (needs 256 KB after the program).',13,10,'$'
s_head2		db	'SPRITES A VRTC, 180 VRTCS A TEST, SGP + CPU COMPLD', 0
s_head3		db	'SCREEN  COL.  SHARE     8X8  16X16  32X32  64X64', 0
s_loc0		db	'SGP1/4', 0
s_loc1		db	'SGP1/2', 0
s_loc2		db	'SGP3/4', 0
		align	2
list_ptr	dw	0
tab_ptr		dw	0
sgp_n		dw	0
cpu_n		dw	0
drawn		dd	0
		align	2
sprite_tab	times	128 * ENTRY db 0
image_end:
IMAGE_TOP	equ	image_end - $$ + 100h
%if IMAGE_TOP > LIST
%error the program reaches its SGP list
%endif
