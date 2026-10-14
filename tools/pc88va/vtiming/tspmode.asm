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
;
; TSPMODE.COM: probe undocumented TSP screen-table MODE values on a
; PC-88VA under PC-Engine (V3 mode), written for vaeg M104; see
; tools/pc88va/vtiming/README.md. Assemble with NASM (8086/80186 only):
; nasm -f bin -o TSPMODE.COM tspmode.asm
;
;   TSPMODE [first]
;
; The PC-88VA documents MODE values 0-5 in bits 4:0 of word 0Ah of each
; screen-table entry; the uPD72022 also has semigraphics and graphics
; display modes whose selection is not documented for the VA. TSPMODE
; writes a 16 x 16 block of character codes 00h-FFh (white, attribute
; F0h) into the main split, then sets split 0's MODE to first (default 0)
; and steps it to 31 on each key (repeats of a held key are dropped); ESC
; stops. The current value is shown
; as "MODE nn" in the function-key row (split 1, left in mode 1). All
; changed TVRAM bytes and both MODE words are restored on exit.
; Timing is not changed.
		cpu	186
		org	0100h
TABLE		equ	7f00h		; screen table (TVRAM offset, PC-Engine)
ROW0		equ	4		; first test row in split 0
COL0		equ	24		; first test column
start:		mov	si,0081h
		xor	bx,bx
.arg:		lodsb
		cmp	al,' '
		je	.arg
		cmp	al,'0'
		jb	.argd
		cmp	al,'9'
		ja	.argd
		sub	al,'0'
		xor	ah,ah
		xchg	ax,bx
		mov	cx,10
		mul	cx
		add	bx,ax
		jmp	.arg
.argd:		and	bx,1fh
		mov	[mode],bl

		cli
		mov	dx,0153h		; system memory area: TVRAM
		in	al,dx
		mov	[bank],al
		and	al,0f0h
		or	al,01h
		out	dx,al
		sti
		mov	ax,0a000h
		mov	es,ax
		; split 0: source address and line width
		mov	ax,[es:TABLE+10h]
		mov	[rsa0],ax
		mov	ax,[es:TABLE+08h]
		and	ax,03ffh
		mov	[vw0],ax
		call	label_row		; split 1, or the last row of split 0
		mov	[rsa1],ax
		mov	ax,[es:TABLE+0ah]
		mov	[save0],ax
		mov	ax,[es:TABLE+2ah]
		mov	[save1],ax

		; save and fill the 16 x 16 test block
		mov	di,savebuf
		xor	bx,bx			; row 0..15
.fr:		call	cell_addr		; SI = cell (row BX, col 0)
		mov	cx,16
		xor	dx,dx
.fc:		mov	ax,[es:si]
		mov	[di],ax
		mov	ax,[es:si+8000h]
		mov	[di+2],ax
		add	di,4
		mov	al,bl			; code = row * 16 + col
		shl	al,4
		or	al,dl
		xor	ah,ah
		mov	[es:si],ax
		mov	word [es:si+8000h],00f0h
		add	si,2
		inc	dl
		loop	.fc
		inc	bx
		cmp	bx,16
		jb	.fr
		; save the label cells of split 1 (10 cells)
		mov	si,[rsa1]
		mov	cx,10
.sl:		mov	ax,[es:si]
		mov	[di],ax
		mov	ax,[es:si+8000h]
		mov	[di+2],ax
		add	di,4
		add	si,2
		loop	.sl

.step:		mov	ax,[save0]
		and	al,0e0h
		or	al,[mode]
		mov	[es:TABLE+0ah],ax
		call	label
		call	settle		; drop key repeats, then wait
		call	getkey
		cmp	al,1bh
		je	.done
		inc	byte [mode]
		cmp	byte [mode],32
		jb	.step

.done:		mov	ax,[save0]		; restore
		mov	[es:TABLE+0ah],ax
		mov	ax,[save1]
		mov	[es:TABLE+2ah],ax
		mov	di,savebuf
		xor	bx,bx
.rr:		call	cell_addr
		mov	cx,16
.rc:		mov	ax,[di]
		mov	[es:si],ax
		mov	ax,[di+2]
		mov	[es:si+8000h],ax
		add	di,4
		add	si,2
		loop	.rc
		inc	bx
		cmp	bx,16
		jb	.rr
		mov	si,[rsa1]
		mov	cx,10
.rl:		mov	ax,[di]
		mov	[es:si],ax
		mov	ax,[di+2]
		mov	[es:si+8000h],ax
		add	di,4
		add	si,2
		loop	.rl
		call	unsplit
		cli
		mov	al,[bank]
		mov	dx,0153h
		out	dx,al
		sti
		mov	ax,4c00h
		int	21h

; M109: wait for a key with the keyboard BIOS's primitive functions 0Ah
; (sense) and 09h (read); functions 00h/01h wait on the Japanese front end,
; which only an OS installs, so they hang on a self-booting disk and under
; MS-DOS. AH = scan code, AL = internal code; ESC (scan 00h) gives AL = 1Bh.
getkey:		mov	ah,0ah
		int	82h
		jc	getkey
		mov	ah,09h
		int	82h
		or	ah,ah
		jnz	.r
		mov	al,1bh
.r:		ret

; wait about 20 frames (VRTC rising edges, port 40h bit 5), then clear
; the keyboard queue so a held key steps only once
settle:		mov	cx,20
.f:		mov	dx,0040h
.lo:		in	al,dx
		test	al,20h
		jnz	.lo
.hi:		in	al,dx
		test	al,20h
		jz	.hi
		loop	.f
		mov	ah,0ch
		int	82h
		ret

; SI = TVRAM offset of row BX, column COL0 of split 0
cell_addr:	push	ax
		push	dx
		mov	ax,bx
		add	ax,ROW0
		mul	word [vw0]
		add	ax,[rsa0]
		add	ax,COL0*2
		mov	si,ax
		pop	dx
		pop	ax
		ret

; "MODE nn" in the function-key row, white, mode-1 attribute
; M109: AX = the TVRAM address of the bottom row for the "MODE" label:
; split 1 (PC-Engine's function-key row) when it is shown (height at +34h).
; Otherwise, as on the self-booting SGP disk, where the ROM text BIOS sets
; up split 0 alone, the last 20-raster row of split 0 is made into split 1
; (a copy of split 0's descriptor, so the label keeps the normal character
; mode while split 0's MODE changes); unsplit puts the table back.
label_row:	mov	ax,[es:TABLE+30h]
		cmp	word [es:TABLE+34h],0
		jne	.r
		push	bx
		push	cx
		push	dx
		push	si
		xor	si,si			; save split 1, copy split 0 to it
.c:		mov	ax,[es:TABLE+20h+si]
		mov	[splitsave+si],ax
		mov	ax,[es:TABLE+si]
		mov	[es:TABLE+20h+si],ax
		add	si,2
		cmp	si,20h
		jb	.c
		mov	ax,[es:TABLE+14h]	; split 0 height in rasters
		mov	[rh0save],ax
		and	ax,01feh
		xor	dx,dx
		mov	cx,20
		div	cx
		or	ax,ax
		jz	.top
		dec	ax
.top:		mov	bx,[es:TABLE+08h]	; line width in bytes
		and	bx,03ffh
		mul	bx
		add	ax,[es:TABLE+10h]
		mov	[es:TABLE+30h],ax	; split 1: that row, 20 rasters
		mov	word [es:TABLE+34h],20
		sub	word [es:TABLE+14h],20
		mov	byte [madesplit],1
		pop	si
		pop	dx
		pop	cx
		pop	bx
.r:		ret

unsplit:	cmp	byte [madesplit],0
		je	.r
		push	ax
		push	si
		xor	si,si
.c:		mov	ax,[splitsave+si]
		mov	[es:TABLE+20h+si],ax
		add	si,2
		cmp	si,20h
		jb	.c
		mov	ax,[rh0save]
		mov	[es:TABLE+14h],ax
		mov	byte [madesplit],0
		pop	si
		pop	ax
.r:		ret

label:		mov	al,[mode]
		xor	ah,ah
		mov	bl,10
		div	bl
		add	ax,3030h
		mov	[labtxt+5],al
		mov	[labtxt+6],ah
		mov	si,[rsa1]
		mov	bx,labtxt
		mov	cx,10
.l:		mov	al,[bx]
		xor	ah,ah
		mov	[es:si],ax
		mov	word [es:si+8000h],00f0h
		inc	bx
		add	si,2
		loop	.l
		ret

labtxt		db	'MODE nn   '
mode		db	0
bank		db	0
		align	2
rsa0		dw	0
rsa1		dw	0
rh0save		dw	0
splitsave	times 16 dw 0
madesplit	db	0
		align	2
vw0		dw	0
save0		dw	0
save1		dw	0
savebuf:
