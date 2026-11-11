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
; EMSBANK.COM (M146): is the EMS page frame part of the bus slot bank of
; the system memory area (A0000h-DFFFFh, port 153h bits 3-0 = SMBC)?
;
; With an EMM loaded (EMMVA01 + EMM + EMMVA02), one logical page is
; mapped to physical page 0 and four marker bytes are written to the frame
; with SMBC = 0 (bus slot).  The frame offset 0 is then accessed with
; SMBC = 4 (GVRAM; the GVRAM bytes are saved and restored) and SMBC = 8
; (kanji ROM #1, read only), and read again with SMBC = 0.  If the frame
; belongs to the bus slot, the marker survives the GVRAM write, and neither
; the GVRAM bytes read before the write nor the kanji ROM bytes equal it.
; Interrupts are disabled while SMBC is not the caller's value.  Results go
; to the screen and EMSBANK.TXT.
; Synthetic data only.

	cpu	186
	bits	16
	org	100h

PORT_SMBC	equ	153h

start:
	mov	di, outbuf
	mov	si, title
	call	copystr
	call	machine
	; an EMM must hook INT 67h
	push	es
	xor	ax, ax
	mov	es, ax
	mov	ax, [es:67h*4]
	or	ax, [es:67h*4+2]
	pop	es
	jz	.noemm
	mov	byte [emmfn], 40h
	mov	ah, 40h
	int	67h
	or	ah, ah
	jnz	.emmerr
	mov	byte [emmfn], 41h
	mov	ah, 41h
	int	67h
	or	ah, ah
	jnz	.emmerr
	mov	[frame], bx
	mov	byte [emmfn], 43h
	mov	bx, 1
	mov	ah, 43h
	int	67h
	or	ah, ah
	jnz	.emmerr
	mov	[handle], dx
	mov	dx, PORT_SMBC		; the caller's bank, restored at the end
	in	al, dx
	mov	[smbc_orig], al
	mov	dx, [handle]
	mov	byte [emmfn], 44h
	mov	ax, 4400h
	xor	bx, bx
	int	67h
	or	ah, ah
	jnz	.maperr

	call	probe

	mov	si, s_frame
	call	copystr
	mov	ax, [frame]
	call	hexword
	mov	si, s_after
	call	copystr
	mov	al, [smbc_map]
	call	hexbyte
	mov	si, s_r0
	call	copystr
	mov	si, r0
	call	hex4
	mov	si, s_g0
	call	copystr
	mov	si, g0
	call	hex4
	mov	si, s_g
	call	copystr
	mov	si, g1
	call	hex4
	mov	si, s_r1
	call	copystr
	mov	si, r1
	call	hex4
	mov	si, s_k
	call	copystr
	mov	si, kr
	call	hex4
	mov	si, s_crlf
	call	copystr
	; verdict
	mov	si, s_fail
	mov	ax, [r0]
	mov	bx, [r0+2]
	cmp	ax, [marker]
	jne	.verdict
	cmp	bx, [marker+2]
	jne	.verdict
	cmp	ax, [r1]
	jne	.verdict
	cmp	bx, [r1+2]
	jne	.verdict
	cmp	ax, [g0]		; GVRAM must not show the marker
	jne	.g0ok
	cmp	bx, [g0+2]
	je	.verdict
.g0ok:
	cmp	ax, [kr]		; nor may kanji ROM #1
	jne	.pass
	cmp	bx, [kr+2]
	je	.verdict
.pass:
	mov	si, s_pass
.verdict:
	call	copystr
.free:
	mov	dx, [handle]
	mov	ah, 45h
	int	67h
	jmp	.out
.maperr:
	call	emmerror
	jmp	.free
.emmerr:
	call	emmerror
	jmp	.out
.noemm:
	mov	si, s_noemm
	call	copystr
.out:
	mov	byte [di], '$'
	mov	dx, outbuf
	mov	ah, 09h
	int	21h
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

; "EMM ERROR fn=xx status=yy" (AH holds the EMM status)
emmerror:
	push	ax
	mov	si, s_emmerr
	call	copystr
	mov	al, [emmfn]
	call	hexbyte
	mov	si, s_status
	call	copystr
	pop	ax
	mov	al, ah
	call	hexbyte
	mov	si, s_crlf
	jmp	copystr

; The bank sequence; SMBC is restored before interrupts are enabled.
probe:
	push	es
	push	di
	mov	es, [frame]
	mov	dx, PORT_SMBC
	pushf
	cli
	in	al, dx
	mov	[smbc_map], al
	and	al, 0F0h
	mov	[smbc_hi], al
	out	dx, al			; bus slot
	mov	ax, [marker]
	mov	[es:0], ax
	mov	ax, [marker+2]
	mov	[es:2], ax
	mov	ax, [es:0]
	mov	[r0], ax
	mov	ax, [es:2]
	mov	[r0+2], ax
	mov	al, [smbc_hi]
	or	al, 4			; GVRAM
	out	dx, al
	mov	ax, [es:0]
	mov	[g0], ax
	mov	ax, [es:2]
	mov	[g0+2], ax
	mov	word [es:0], 0
	mov	word [es:2], 0
	mov	ax, [es:0]
	mov	[g1], ax
	mov	ax, [es:2]
	mov	[g1+2], ax
	mov	ax, [g0]
	mov	[es:0], ax
	mov	ax, [g0+2]
	mov	[es:2], ax
	mov	al, [smbc_hi]
	or	al, 8			; kanji ROM #1
	out	dx, al
	mov	ax, [es:0]
	mov	[kr], ax
	mov	ax, [es:2]
	mov	[kr+2], ax
	mov	al, [smbc_hi]		; bus slot
	out	dx, al
	mov	ax, [es:0]
	mov	[r1], ax
	mov	ax, [es:2]
	mov	[r1+2], ax
	mov	al, [smbc_orig]
	out	dx, al
	popf
	pop	di
	pop	es
	ret

; F000:FFFE: FFFFh PC-88VA, FFFEh PC-88VA2/VA3, FFFDh PC-88VA + VA-91
machine:
	push	es
	mov	ax, 0F000h
	mov	es, ax
	mov	ax, [es:0FFFEh]
	pop	es
	mov	si, m_va
	cmp	ax, 0FFFFh
	je	.name
	mov	si, m_va2
	cmp	ax, 0FFFEh
	je	.name
	mov	si, m_va91
	cmp	ax, 0FFFDh
	je	.name
	mov	si, m_unknown
.name:
	push	ax
	call	copystr
	pop	ax
	call	hexword
	mov	si, s_crlf
	jmp	copystr

copystr:
	lodsb
	or	al, al
	jz	.done
	stosb
	jmp	copystr
.done:
	ret

hex4:				; four bytes at SI
	mov	cx, 4
.loop:
	lodsb
	call	hexbyte
	loop	.loop
	ret

hexword:
	push	ax
	mov	al, ah
	call	hexbyte
	pop	ax
hexbyte:
	push	ax
	shr	al, 4
	call	.nib
	pop	ax
	and	al, 0Fh
.nib:
	add	al, '0'
	cmp	al, '9'
	jbe	.put
	add	al, 7
.put:
	stosb
	ret

title	db	'EMSBANK (M146): EMS page frame vs system memory bank (153h)', 13, 10, 0
m_va	db	'PC-88VA ', 0
m_va2	db	'PC-88VA2/VA3 ', 0
m_va91	db	'PC-88VA + PC-88VA-91 ', 0
m_unknown db	'unknown machine ', 0
s_frame	db	'frame=', 0
s_after	db	' 153h-after-map=', 0
s_r0	db	13, 10, 'bus=', 0
s_g0	db	' gvram-before=', 0
s_g	db	' gvram-written=', 0
s_r1	db	' bus-again=', 0
s_k	db	' kanji=', 0
s_crlf	db	13, 10, 0
s_pass	db	'PASS frame is in the bus slot bank', 13, 10, 0
s_fail	db	'FAIL frame does not follow 153h', 13, 10, 0
s_noemm	db	'NO EMM', 13, 10, 0
s_emmerr db	'EMM ERROR fn=', 0
s_status db	' status=', 0
emmfn	db	0
fname	db	'EMSBANK.TXT', 0
marker	db	'EMSb'
frame	dw	0
handle	dw	0
smbc_map db	0
smbc_orig db	0
smbc_hi	db	0
r0	times 4 db 0
g0	times 4 db 0
g1	times 4 db 0
kr	times 4 db 0
r1	times 4 db 0
outbuf:
