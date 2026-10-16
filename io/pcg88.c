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
/*
 * M115: PCG-8100 (PC-8001) and PCG-8800 (PC-8801) in V1/V2 (88) mode.
 *
 * Derived from X88000 1.5.3 by Manuke, PC88Pcg.cpp (CPC88Pcg::Reset,
 * WritePcgPort, WriteITimerPort, GetITimerCounterValue) and X88000.cpp
 * (CX88000::PcgSoundOutput), released into the public domain (PDS; source
 * archive x88_1_5_3_src.tar.gz, see ADR-0016 "Derivations"). Rewritten in C
 * for vaeg; one deviation: an 8253 MSB-only load (RL = 10) sets only the
 * MSB, where X88000 falls through into the LSB/MSB load.
 *
 * Ports (OUT only, 88 mode, PCG enabled in the configuration):
 *   00h  data
 *   01h  address bits 7-0
 *   02h  address bits 15-8: bit 4 writes data (bit 5: copies the CG ROM)
 *        into PCG RAM at address bits 9-0; bits 3, 6, 7 enable the sound
 *        of 8253 counters 0, 1, 2
 *   03h  bits 1-0 / 3-2: codes 00h-7Fh / 80h-FFh shown from PCG bank
 *        (bit 1/3) number (bit 0/2); bit 4 selects the bank written
 *   0Ch-0Eh  8253 counters 0-2, 0Fh  8253 mode
 * A counter sounds a square wave of 4 MHz / count (count > 100h; 0 is
 * 65536), as X88000 does.
 */

#include "compiler.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "memoryva.h"
#include "sound.h"
#include "pcg88.h"

_PCG88 pcg88;

static struct {
	UINT32 phase[3]; /* 16.16 */
	SINT32 level;
} pcgsnd;

static BOOL pcg88_active(void) {
	return memoryva_88_mode && np2cfg.pcg_enable;
}

/* The 8x8 character ROM: codes 00h-FFh, 8 bytes each (VA font 41000h). */
static BYTE pcg88_rom(UINT address) {
	return fontmem[0x41000 + (address & 0x7ff)];
}

void pcg88_reset(void) {
	UINT i;

	ZeroMemory(&pcg88, sizeof(pcg88));
	pcg88.display[1] = 1;
	for (i = 0; i < 3; i++) {
		pcg88.counter[i] = 1;
	}
	ZeroMemory(pcgsnd.phase, sizeof(pcgsnd.phase));
}

/* Both banks start as the ROM's codes 80h-FFh (after the ROMs are loaded). */
void pcg88_initfont(void) {
	UINT bank;
	UINT i;

	for (bank = 0; bank < 2; bank++) {
		for (i = 0; i < 1024; i++) {
			pcg88.ram[bank][i] = pcg88_rom(1024 + i);
		}
	}
}

static void IOOUTCALL pcg88_o_pcg(UINT port, REG8 dat) {
	UINT half;

	if (!pcg88_active()) {
		return;
	}
	switch (port & 0xff) {
	case 0x00:
		pcg88.data = (UINT8)dat;
		break;
	case 0x01:
		pcg88.address = (UINT16)((pcg88.address & 0xff00) | dat);
		break;
	case 0x02:
		pcg88.address = (UINT16)((pcg88.address & 0x00ff) | ((UINT16)dat << 8));
		pcg88.sound_on[0] = (dat & 0x08) ? 1 : 0;
		pcg88.sound_on[1] = (dat & 0x40) ? 1 : 0;
		pcg88.sound_on[2] = (dat & 0x80) ? 1 : 0;
		break;
	case 0x03:
		for (half = 0; half < 2; half++) {
			const UINT mode = (UINT)dat >> (2 * half);
			pcg88.display[half] = (mode & 2) ? 1 : 0;
			pcg88.bank[half] = (UINT8)(mode & 1);
		}
		pcg88.setbank = (dat & 0x10) ? 1 : 0;
		return;
	}
	if (pcg88.address & 0x1000) {
		const UINT a = pcg88.address & 0x03ff;
		pcg88.ram[pcg88.setbank][a] = (pcg88.address & 0x2000) ? pcg88_rom(1024 + a) : pcg88.data;
	}
}

static void IOOUTCALL pcg88_o_timer(UINT port, REG8 dat) {
	UINT ch;

	if (!pcg88_active()) {
		return;
	}
	port &= 0x03;
	if (port == 3) {
		ch = (dat >> 6) & 3;
		if (ch < 3) {
			pcg88.mode[ch] = (UINT8)((dat >> 4) & 3);
			pcg88.msb[ch] = 0;
			pcg88.bcd[ch] = (UINT8)(dat & 1);
		}
		return;
	}
	ch = port;
	switch (pcg88.mode[ch]) {
	case 1: /* LSB only */
		pcg88.counter[ch] = dat;
		break;
	case 2: /* MSB only */
		pcg88.counter[ch] = (UINT16)((UINT)dat << 8);
		break;
	case 3: /* LSB, then MSB */
		if (pcg88.msb[ch]) {
			pcg88.counter[ch] = (UINT16)((pcg88.counter[ch] & 0x00ff) | ((UINT)dat << 8));
		} else {
			pcg88.counter[ch] = (UINT16)((pcg88.counter[ch] & 0xff00) | dat);
		}
		pcg88.msb[ch] ^= 1;
		break;
	}
}

void pcg88_bind(void) {
	UINT port;

	for (port = 0x00; port <= 0x03; port++) {
		iocore_attachout(port, pcg88_o_pcg);
	}
	for (port = 0x0c; port <= 0x0f; port++) {
		iocore_attachout(port, pcg88_o_timer);
	}
}

/* The glyph shown for a character code, or NULL when the ROM shows it. */
const BYTE *pcg88_glyph(BYTE code) {
	const UINT half = code >> 7;

	if (!pcg88_active() || !pcg88.display[half]) {
		return NULL;
	}
	return pcg88.ram[pcg88.bank[half]] + ((UINT)(code & 0x7f) << 3);
}

/* The tone of a counter in Hz, 0 when silent. */
UINT pcg88_frequency(UINT channel) {
	UINT count;

	if ((channel >= 3) || !pcg88_active() || !pcg88.sound_on[channel]) {
		return 0;
	}
	count = pcg88.counter[channel];
	if (pcg88.bcd[channel]) {
		count = ((count >> 12) & 0x0f) * 1000 + ((count >> 8) & 0x0f) * 100 +
		        ((count >> 4) & 0x0f) * 10 + (count & 0x0f);
	}
	if (count == 0) {
		return 4000000 / 0x10000;
	}
	return (count > 0x100) ? (4000000 / count) : 0;
}

void pcg88_setvol(UINT vol) {
	if (vol > 128) {
		vol = 128;
	}
	pcgsnd.level = (SINT32)vol * 32;
}

void pcg88_getpcm(void *hdl, SINT32 *pcm, UINT count) {
	UINT32 step[3];
	UINT32 rate = soundcfg.rate;
	UINT ch;
	BOOL any = FALSE;

	(void)hdl;
	if ((rate == 0) || (pcgsnd.level == 0)) {
		return;
	}
	for (ch = 0; ch < 3; ch++) {
		step[ch] = (UINT32)(((UINT64)pcg88_frequency(ch) << 16) / rate);
		any = any || (step[ch] != 0);
	}
	if (!any) {
		return;
	}
	while (count--) {
		SINT32 samp = 0;
		for (ch = 0; ch < 3; ch++) {
			if (step[ch]) {
				pcgsnd.phase[ch] += step[ch];
				samp += (pcgsnd.phase[ch] & 0x8000) ? pcgsnd.level : -pcgsnd.level;
			}
		}
		pcm[0] += samp;
		pcm[1] += samp;
		pcm += 2;
	}
}
