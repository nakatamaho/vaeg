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
; V1V2PORT.COM (M144): in V3 mode, which I/O ports does the main unit
; drive?  Each port is read four times with "MOV BL,55h" (B3 55) after
; the IN and four times with "MOV CL,0AAh" (B1 AA).  An undriven port
; returns what the bus last carried, usually prefetched code, so its value
; follows the trailing bytes ("FLOAT").  A driven port reads the same both
; ways ("DRIVEN xx").  Read only: nothing is written to any port.
; The table goes to the screen and to V1V2PORT.TXT.

	cpu	186
	bits	16
	org	100h

start:
	mov	di, outbuf
	mov	si, title
	call	copystr
	mov	si, ports
.next:
	lodsw
	cmp	ax, 0FFFFh
	je	.done
	mov	[curport], ax
	push	si
	call	probe
	pop	si
	jmp	.next
.done:
	cmp	byte [column], 0
	je	.even
	mov	al, 13
	stosb
	mov	al, 10
	stosb
.even:
	mov	si, footer
	call	copystr
	mov	byte [di], '$'
	; screen
	mov	dx, outbuf
	mov	ah, 09h
	int	21h
	; file
	mov	dx, fname
	xor	cx, cx
	mov	ah, 3Ch
	int	21h
	jc	.exit
	mov	bx, ax
	mov	dx, outbuf
	mov	cx, di
	sub	cx, outbuf
	mov	ah, 40h
	int	21h
	mov	ah, 3Eh
	int	21h
.exit:
	mov	ax, 4C00h
	int	21h

; ---- read [curport] 4x each way, append one line to [di]
probe:
	mov	dx, [curport]
	mov	cx, 4
	mov	bx, vala
.a:	cli
	in	al, dx
	mov	bl, 55h		; B3 55
	mov	bl, 55h
	mov	bl, 55h
	mov	bl, 55h
	sti
	mov	bx, vala
	add	bx, 4
	sub	bx, cx
	mov	[bx], al
	loop	.a
	mov	cx, 4
.b:	cli
	in	al, dx
	mov	cl, 0AAh	; B1 AA (CX is restored below)
	mov	cl, 0AAh
	mov	cl, 0AAh
	mov	cl, 0AAh
	sti
	mov	cl, [bcount]
	dec	byte [bcount]
	mov	ch, 0
	mov	bx, valb
	add	bx, 4
	sub	bx, cx
	mov	[bx], al
	dec	cx
	jnz	.b
	mov	byte [bcount], 4
	; "PPPPh A:aaaaaaaa B:bbbbbbbb verdict", two to a line
	mov	[linestart], di
	mov	ax, [curport]
	call	hex16
	mov	al, 'h'
	stosb
	mov	si, s_a
	call	copystr
	mov	si, vala
	call	hex4
	mov	si, s_b
	call	copystr
	mov	si, valb
	call	hex4
	mov	si, s_sp
	call	copystr
	; verdict
	mov	si, vala
	mov	al, [si]
	mov	cx, 8
.same:	cmp	[si], al
	jne	.diff
	inc	si
	loop	.same
	mov	si, s_driven
	call	copystr
	mov	al, [vala]
	call	hex8
	jmp	.eol
.diff:
	; all A equal, all B equal, A != B -> follows the trailing code
	mov	si, vala
	call	allsame4
	jne	.unst
	mov	si, valb
	call	allsame4
	jne	.unst
	mov	si, s_float
	call	copystr
	jmp	.eol
.unst:
	mov	si, s_unst
	call	copystr
.eol:
	xor	byte [column], 1
	jz	.crlf
	mov	cx, [linestart]
	add	cx, 40
.pad:	cmp	di, cx
	jae	.padded
	mov	al, ' '
	stosb
	jmp	.pad
.padded:
	ret
.crlf:
	mov	al, 13
	stosb
	mov	al, 10
	stosb
	ret

allsame4:		; ZF=1 when the four bytes at si are equal
	mov	al, [si]
	cmp	al, [si+1]
	jne	.r
	cmp	al, [si+2]
	jne	.r
	cmp	al, [si+3]
.r:	ret

hex4:			; four bytes at si as "xxxxxxxx"
	mov	cx, 4
.l:	lodsb
	call	hex8
	loop	.l
	ret

hex16:
	push	ax
	mov	al, ah
	call	hex8
	pop	ax
hex8:
	push	ax
	shr	al, 4
	call	hexd
	pop	ax
	and	al, 0Fh
hexd:
	add	al, '0'
	cmp	al, '9'
	jbe	.o
	add	al, 7
.o:	stosb
	ret

copystr:		; NUL-terminated si -> di
	lodsb
	or	al, al
	jz	.e
	stosb
	jmp	copystr
.e:	ret

title	db	'V1V2PORT (M144) read-only probe. FLOAT=undriven, DRV xx=answered', 13, 10, 0
footer	db	'Saved to V1V2PORT.TXT', 13, 10, 0
s_a	db	' A:', 0
s_b	db	' B:', 0
s_sp	db	' ', 0
s_driven db	'DRV ', 0
s_float	db	'FLOAT', 0
s_unst	db	'UNSTABLE', 0
fname	db	'V1V2PORT.TXT', 0

; V1/V2-only ports (manual), the WSR-E low bytes E0h/E1h, WSR-E's own
; ports for aliasing, and 153h as a known-driven control.
ports	dw	0153h
	dw	00E0h, 00E1h, 00E2h, 00E3h, 00E8h, 00E9h, 00ECh, 00EDh
	dw	0031h, 0034h, 0035h, 005Ch, 0070h, 0071h, 00F0h, 00F1h
	dw	46E8h, 4EE0h, 4FE0h, 40E1h, 5AE0h
	dw	0FFFFh

curport	dw	0
linestart dw	0
column	db	0
bcount	db	4
vala	times 4 db 0
valb	times 4 db 0
outbuf:
