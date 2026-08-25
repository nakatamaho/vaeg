; Copyright (c) 2026 Nakata Maho
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions are met:
; 1. Redistributions of source code must retain the above copyright notice,
;    this list of conditions and the following disclaimer.
; 2. Redistributions in binary form must reproduce the above copyright notice,
;    this list of conditions and the following disclaimer in the documentation
;    and/or other materials provided with the distribution.
;
; THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS
; OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
; OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
; IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
; INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
; BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
; AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
; OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
; USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

; NEON4 original PEGC palette quantised to VA direct RGB332.
; Source: demos/neon4_1_0/VIDEO4_DATA.INC:pegc_palette_grb.
; Source order is GRB; VA pixel order is ggg rrr bb.
; qg=round(g*7/255), qr=round(r*7/255), qb=round(b*3/255).
align 2
n4_va_rgb332_from_pegc:
        db 00h, 00h, 00h, 24h, 24h, 24h, 25h, 49h, 49h, 49h, 49h, 49h, 6dh, 6dh, 6dh, 6dh
        db 92h, 92h, 92h, 92h, 0b6h, 0b6h, 0b6h, 0b6h, 0b6h, 0dah, 0dbh, 0dbh, 0dbh, 0ffh, 0ffh, 0ffh
        db 00h, 00h, 00h, 00h, 20h, 21h, 21h, 21h, 25h, 25h, 25h, 45h, 45h, 45h, 45h, 46h
        db 46h, 46h, 66h, 66h, 66h, 66h, 66h, 66h, 6ah, 8ah, 8bh, 8bh, 8bh, 8bh, 8bh, 8bh
        db 00h, 00h, 20h, 20h, 20h, 21h, 41h, 41h, 41h, 45h, 45h, 65h, 65h, 65h, 65h, 86h
        db 86h, 86h, 86h, 86h, 0a6h, 0a6h, 0a6h, 0a6h, 0a6h, 0c6h, 0c7h, 0cbh, 0cbh, 0ebh, 0ebh, 0ebh
        db 00h, 00h, 20h, 20h, 20h, 20h, 44h, 44h, 44h, 44h, 44h, 65h, 65h, 65h, 65h, 85h
        db 85h, 85h, 89h, 89h, 0a9h, 0a9h, 0a9h, 0a9h, 0a9h, 0c9h, 0c9h, 0c9h, 0c9h, 0e9h, 0edh, 0eeh
        db 00h, 00h, 24h, 24h, 24h, 24h, 48h, 48h, 48h, 48h, 48h, 6ch, 6ch, 6ch, 6ch, 90h
        db 90h, 90h, 90h, 90h, 0b4h, 0b5h, 0b5h, 0b5h, 0b5h, 0d9h, 0d9h, 0d9h, 0d9h, 0feh, 0feh, 0ffh
        db 00h, 00h, 04h, 04h, 04h, 04h, 28h, 28h, 28h, 28h, 28h, 2ch, 2ch, 2ch, 2ch, 30h
        db 50h, 50h, 50h, 50h, 54h, 54h, 54h, 54h, 54h, 58h, 78h, 78h, 78h, 7ch, 7ch, 7ch
        db 00h, 00h, 04h, 04h, 04h, 05h, 09h, 09h, 09h, 29h, 29h, 2dh, 2dh, 2dh, 2dh, 32h
        db 32h, 32h, 32h, 32h, 36h, 36h, 36h, 36h, 36h, 3ah, 3bh, 5bh, 5bh, 5fh, 5fh, 5fh
        db 00h, 00h, 24h, 24h, 25h, 25h, 49h, 49h, 49h, 49h, 49h, 6dh, 6dh, 6dh, 6dh, 92h
        db 92h, 92h, 92h, 92h, 0b6h, 0b6h, 0b6h, 0b6h, 0b6h, 0dah, 0dbh, 0dbh, 0dbh, 0ffh, 0ffh, 0ffh
n4_va_rgb332_from_pegc_end:
%if n4_va_rgb332_from_pegc_end - n4_va_rgb332_from_pegc != 256
%error "NEON4 VA palette table must contain 256 RGB332 entries"
%endif
