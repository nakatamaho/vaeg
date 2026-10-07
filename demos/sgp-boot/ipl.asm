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
; IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
; OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
; IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
; INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
; BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
; USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
; ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
; (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
; THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;
; IPL.BIN: boot sector of the self-booting SGP demo disk (vaeg M106); see
; demos/sgp-boot/README.md. Assemble with NASM:
;   nasm -f bin -DLOADER_SECTORS=n -o IPL.BIN ipl.asm
;
; The PC-88VA ROM reads this first sector of drive 1 to 3000:0000 and jumps
; there with SS:SP near 3000:FFFE (technical manual 1.4). Like other
; self-booting VA software, it loads the rest itself with the ROM floppy
; disk BIOS (INT 80h): LOADER_SECTORS sectors from logical sector 1 (track 0
; sector 2) to LOADER_SEG:0000, then jumps there. Disk: 2DD, 80 cylinders x
; 2 heads x 9 sectors x 512 bytes; logical sector n is track n / 9 (cylinder
; x 2 + head), sector n mod 9 + 1.
		cpu	186
		org	0
LOADER_SEG	equ	2000h
SECTORS		equ	9		; per track
RETRIES		equ	4

start:		cli
		mov	ax,cs
		mov	ds,ax
		sti
		cld
		mov	ax,0a12h		; disk mode of drive 1 (as other VA boot disks)
		xor	cx,cx
		int	80h
		mov	ax,LOADER_SEG
		mov	es,ax
		xor	bp,bp
		mov	si,1			; logical sector
		mov	di,LOADER_SECTORS	; sectors left
.next:		or	di,di
		jz	.done
		mov	ax,si			; track = si / 9, sector = si % 9 + 1
		mov	bl,SECTORS
		div	bl
		mov	cl,al
		mov	dh,ah
		inc	dh
		mov	al,SECTORS + 1		; read to the end of the track
		sub	al,dh
		xor	ah,ah
		cmp	ax,di
		jbe	.count
		mov	ax,di
.count:		mov	[count],al		; sectors in this read
		mov	byte [tries],RETRIES
.read:		mov	al,[count]
		mov	ah,01h			; read, with retry
		xor	bx,bx			; BH/BL: ID cylinder/head if BIOSMODE asks
		xor	ch,ch			; drive 1
		mov	dl,02h			; MFM, 512 bytes
		int	80h
		jnc	.ok
		dec	byte [tries]
		jz	error
		mov	ah,06h			; recalibrate, then try again
		xor	ch,ch
		int	80h
		jmp	.read
.ok:		mov	bl,[count]
		xor	bh,bh
		add	si,bx
		sub	di,bx
		shl	bx,9
		add	bp,bx
		jmp	.next
.done:		jmp	LOADER_SEG:0000h

error:		mov	si,message
.p:		lodsb
		or	al,al
		jz	.halt
		xor	ah,ah
		mov	dx,ax
		int	83h			; text BIOS 00h: one character
		jmp	.p
.halt:		hlt
		jmp	.halt

message		db	'SGP demo disk: read error', 13, 10, 0
count		db	0
tries		db	0

		times	510 - ($ - $$) db 0
		dw	0
