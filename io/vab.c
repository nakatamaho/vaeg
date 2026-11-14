/* Copyright (c) 2026 Nakata Maho
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

/* M150: NEC PC-8801-17 Video Art Board. See io/vab.h. */

#include "compiler.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "memoryva.h"
#include "vab.h"

_VAB vab;
UINT8 vab_ram[VAB_RAM_SIZE];

static BOOL vab_present(void) {
	return (np2cfg.vab_enable != 0);
}

BOOL vab_card_selected(UINT8 eram_bank) {
	return vab_present() && (((eram_bank >> 2) & 0x03) == VAB_CARD);
}

UINT32 vab_ram_offset(UINT8 eram_bank, UINT32 address) {
	return ((UINT32)(eram_bank & 0x03) << 15) | (address & 0x7fff);
}

BOOL vab_display_active(void) {
	return vab_present() && memoryva_88_mode && ((vab.mode1 & 0x41) == 0x41);
}

static void IOOUTCALL vab_ob4(UINT port, REG8 dat) {
	(void)port;
	if (vab_present()) {
		vab.mode1 = (UINT8)dat;
	}
}

static void IOOUTCALL vab_ob5(UINT port, REG8 dat) {
	(void)port;
	if (vab_present()) {
		vab.mode2 = (UINT8)dat;
	}
}

static REG8 IOINPCALL vab_ib4(UINT port) {
	(void)port;
	if (vab_card_selected(memoryva_88_eram.bank)) {
		return (vab.mode1);
	}
	return (0xff);
}

static REG8 IOINPCALL vab_ib5(UINT port) {
	(void)port;
	if (vab_card_selected(memoryva_88_eram.bank)) {
		return (vab.mode2);
	}
	return (0xff);
}

void vab_reset(void) {
	ZeroMemory(&vab, sizeof(vab));
	ZeroMemory(vab_ram, sizeof(vab_ram));
}

void vab_bind(void) {
	/* Always attached: with the board disabled the handlers behave as an
	 * empty slot (writes ignored, reads FFh), so the setting needs no
	 * reset. Nothing else in vaeg decodes B4h/B5h. */
	iocore_attachout(0x0b4, vab_ob4);
	iocore_attachout(0x0b5, vab_ob5);
	iocore_attachinp(0x0b4, vab_ib4);
	iocore_attachinp(0x0b5, vab_ib5);
}

void vab_compose_rgb565(UINT8 *dst, int pitch, int width, int height) {
	int x;
	int y;
	if ((dst == NULL) || (width <= 0) || (height <= 0)) {
		return;
	}
	for (y = 0; y < height; y++) {
		UINT32 sy = ((UINT32)y * VAB_H) / (UINT32)height;
		const UINT8 *row = vab_ram + sy * VAB_W * 2;
		UINT16 *out = (UINT16 *)(dst + (size_t)y * (size_t)pitch);
		for (x = 0; x < width; x++) {
			UINT32 sx = ((UINT32)x * VAB_W) / (UINT32)width;
			UINT32 pixel = (UINT32)row[sx * 2] | ((UINT32)row[sx * 2 + 1] << 8);
			UINT32 g = (pixel >> 10) & 0x3f;
			UINT32 r = (pixel >> 5) & 0x1f;
			UINT32 b = pixel & 0x1f;
			out[x] = (UINT16)((r << 11) | (g << 5) | b);
		}
	}
}
