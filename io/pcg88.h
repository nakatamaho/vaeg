/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef VAEG_IO_PCG88_H
#define VAEG_IO_PCG88_H

/*
 * M115: PCG-8100 (PC-8001) and PCG-8800 (PC-8801) programmable character
 * generator with its 8253 three-voice tone generator, in V1/V2 (88) mode.
 * Derived from X88000 1.5.3 PC88Pcg.cpp (public domain), with behaviour
 * checked against QUASI88 0.7.4; see io/pcg88.c.
 */

typedef struct {
	BYTE ram[2][1024]; /* two banks of 128 8x8 characters */
	UINT8 bank[2];     /* bank shown for codes 00h-7Fh and 80h-FFh */
	UINT8 display[2];  /* PCG shown instead of the ROM for that half */
	UINT16 address;    /* port 01h (low) and 02h (high) */
	UINT8 data;        /* port 00h */
	UINT8 setbank;     /* bank written */
	UINT8 sound_on[3]; /* port 02h bits 3, 6, 7: the 8253 gates */
	UINT8 rl[3];       /* 8253 read/load mode per counter */
	UINT8 mode[3];     /* 8253 counter mode 0-5 */
	UINT16 counter[3];
	UINT8 msb[3]; /* the next byte of a 16-bit load is the MSB */
	UINT8 bcd[3];
} _PCG88;

#ifdef __cplusplus
extern "C" {
#endif

extern _PCG88 pcg88;

void pcg88_reset(void);
void pcg88_initfont(void);
void pcg88_bind(void);
const BYTE *pcg88_glyph(BYTE code);
UINT pcg88_frequency(UINT channel);
void pcg88_setvol(UINT vol);
void pcg88_getpcm(void *hdl, SINT32 *pcm, UINT count);

#ifdef __cplusplus
}
#endif

#endif
