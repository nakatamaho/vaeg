/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/* M144a: independently reimplemented address/pixel rules from public-domain
 * X88000 1.5.3, PC88Z80Main.cpp MemBankRomRamMode_N80,
 * MemBankIERomMode_N80, MemBankGVRamMode_N80/Ex_N80, and
 * X88ScreenDrawer.cpp DrawGraphics N80 colour modes. Archive identity and
 * derivation are recorded in ADR-0016. This is not a VA hardware model. */
#include "pc8001_video.h"

PC8001_ROM_ADDRESS pc8001_rom_address(uint16_t address, uint8_t port31,
                                    uint8_t port71, int sr) {
	PC8001_ROM_ADDRESS result = {PC8001_ROM_RAM, address};
	if (address >= 0x8000 || (port31 & 2)) {
		return result;
	}
	result.source = sr ? PC8001_ROM_SR : PC8001_ROM_N80;
	if (address >= 0x6000) {
		if (sr && !(port71 & 1)) {
			result.source = PC8001_ROM_SR_IE;
			result.offset = address & 0x1fff;
		} else if (!sr && !(port31 & 1)) {
			result.source = PC8001_ROM_4TH;
			result.offset = address & 0x1fff;
		}
	}
	return result;
}

int pc8001_gvram_window(uint16_t address, unsigned plane, int gvam, int gam) {
	return address >= 0x8000 && address < 0xc000 && (gvam ? gam != 0 : plane < 3);
}

unsigned pc8001_mkii_pixel(const uint8_t *vram, unsigned x, unsigned y, int dots320) {
	unsigned offset;
	if (!vram || x >= 640 || y >= 200) {
		return 0;
	}
	offset = y * 80 + x / 8;
	if (dots320) {
		/* First packed bit contributes colour bit 1, second bit 0. */
		return (vram[offset] >> (6 - (x & 6))) & 3;
	}
	return (vram[offset] >> (7 - (x & 7))) & 1;
}

static unsigned planar_pixel(const uint8_t *vram, unsigned offset, unsigned bit) {
	unsigned colour = 0;
	unsigned plane;
	for (plane = 0; plane < 3; plane++) {
		colour |= ((vram[plane * PC8001_PLANE_BYTES + offset] >> bit) & 1U) << plane;
	}
	return colour;
}

unsigned pc8001_sr_pixel(const uint8_t *vram, unsigned x, unsigned y,
                        int dots320, unsigned front_page, uint8_t port53) {
	unsigned front;
	unsigned back;
	unsigned offset;
	unsigned bit;
	if (!vram || x >= 640 || y >= 200 || front_page > 1) {
		return 0;
	}
	if (!dots320) {
		return (port53 & 2) ? 0 : planar_pixel(vram, y * 80 + x / 8, 7 - (x & 7));
	}
	x /= 2;
	offset = y * 40 + x / 8;
	bit = 7 - (x & 7);
	front = (port53 & (2U << front_page)) ? 0 :
	        planar_pixel(vram, offset + front_page * 0x2000, bit);
	back = (port53 & (2U << (1 - front_page))) ? 0 :
	       planar_pixel(vram, offset + (1 - front_page) * 0x2000, bit);
	return front ? front : back;
}
