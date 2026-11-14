/*
 * PC-8801 board text DMA subset.
 * Adapted from Manuke's X88000 1.5.3 (public domain) by Nakata Maho, 2026.
 *
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
#include "pc88_text_dma.h"
#include <string.h>

void pc88_text_dma_reset(PC88_TEXT_DMA *dma, uint16_t initial_address) {
	memset(dma, 0, sizeof(*dma));
	dma->address = initial_address;
}

int pc88_text_dma_write(PC88_TEXT_DMA *dma, unsigned port, uint8_t value) {
	if (port == 0x64) {
		if (dma->high_next)
			dma->address = (uint16_t)((dma->address & 0xff) | ((uint16_t)value << 8));
		else
			dma->address = (uint16_t)((dma->address & 0xff00) | value);
		dma->high_next ^= 1;
		return 1;
	}
	if (port == 0x68) {
		dma->mode = value;
		dma->high_next = 0;
		return 1;
	}
	return 0;
}

int pc88_text_dma_enabled(const PC88_TEXT_DMA *dma) {
	return (dma->mode & 0x84) == 0x84;
}

PC88_TEXT_ADDRESS pc88_text_dma_resolve(const PC88_TEXT_DMA *dma, uint32_t offset,
                                        int fast_active) {
	PC88_TEXT_ADDRESS result;
	const uint16_t address = (uint16_t)(dma->address + offset);
	result.fast = (uint8_t)(fast_active && address >= 0xf000);
	result.offset = result.fast ? address & 0x0fff : address;
	return result;
}

int pc88_text_dma_peek(const PC88_TEXT_DMA *dma, const PC88_TEXT_MEMORY *memory, uint32_t offset,
                       uint8_t *value) {
	PC88_TEXT_ADDRESS address;
	const uint8_t *source;
	size_t size;
	if (!dma || !memory || !value)
		return 0;
	if (memory->peek) {
		*value = memory->peek(memory->peek_opaque, (uint32_t)((dma->address + offset) & 0xffff));
		return 1;
	}
	address = pc88_text_dma_resolve(dma, offset, memory->fast_active);
	source = address.fast ? memory->fast_ram : memory->main_ram;
	size = address.fast ? memory->fast_size : memory->main_size;
	if (!source || address.offset >= size)
		return 0;
	*value = source[address.offset];
	return 1;
}
