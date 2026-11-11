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
#include "io/pc88_text_dma.h"
#include "io/upd3301.h"
#include <stdio.h>
#include <string.h>

#define CHECK(condition, code)                                                                     \
	do {                                                                                           \
		if (!(condition)) {                                                                        \
			fprintf(stderr, "%s line=%d\n", code, __LINE__);                                       \
			return 1;                                                                              \
		}                                                                                          \
	} while (0)

static uint8_t main_ram[65536], fast_ram[4096];

int main(void) {
	static const uint32_t offsets[] = {0, 1, 15, 4095, 4096, 65535, 65536, UINT32_MAX};
	PC88_TEXT_DMA a, b;
	UPD3301 crtc;
	PC88_TEXT_MEMORY memory = {main_ram, sizeof(main_ram), fast_ram, sizeof(fast_ram), 0};
	unsigned value, port, active, i;
	uint8_t result;
	for (value = 0; value < 65536; value++) {
		pc88_text_dma_reset(&a, 0x5a5a);
		CHECK(!pc88_text_dma_enabled(&a) && !a.high_next, "PC88_TEXT_DMA_RESET");
		CHECK(pc88_text_dma_write(&a, 0x64, (uint8_t)value) &&
		          a.address == (0x5a00 | (value & 255)) && a.high_next,
		      "PC88_TEXT_DMA_LOW_BYTE");
		b = a;
		pc88_text_dma_write(&a, 0x64, (uint8_t)(value >> 8));
		pc88_text_dma_write(&b, 0x64, (uint8_t)(value >> 8));
		CHECK(a.address == value && !a.high_next && !memcmp(&a, &b, sizeof(a)),
		      "PC88_TEXT_DMA_CONTINUATION");
	}
	for (value = 0; value < 256; value++) {
		pc88_text_dma_reset(&a, 0x5a5a);
		pc88_text_dma_write(&a, 0x64, 0x37);
		CHECK(pc88_text_dma_write(&a, 0x68, (uint8_t)value) && !a.high_next &&
		          a.address == 0x5a37 && a.mode == value &&
		          pc88_text_dma_enabled(&a) == ((value / 128) && ((value / 4) % 2)),
		      "PC88_TEXT_DMA_MODE");
		pc88_text_dma_write(&a, 0x64, 0x12);
		CHECK(a.address == 0x5a12 && a.high_next, "PC88_TEXT_DMA_PHASE_RESET");
		/* Composition of the two APIs is not a machine bus qualification. */
		upd3301_reset(&crtc);
		upd3301_command(&crtc, 0x20);
		crtc.dma_started = (uint8_t)pc88_text_dma_enabled(&a);
		CHECK(upd3301_display_active(&crtc) == pc88_text_dma_enabled(&a),
		      "PC88_TEXT_DMA_DISPLAY_GATE");
	}
	for (port = 0; port < 65536; port++) {
		if (port == 0x64 || port == 0x68)
			continue;
		b = a;
		CHECK(!pc88_text_dma_write(&b, port, 0x84) && !memcmp(&a, &b, sizeof(a)),
		      "PC88_TEXT_DMA_PORT_SCOPE");
	}
	for (i = 0; i < sizeof(main_ram); i++)
		main_ram[i] = (uint8_t)(i ^ (i >> 8));
	for (i = 0; i < sizeof(fast_ram); i++)
		fast_ram[i] = (uint8_t)(i ^ (i >> 8) ^ 0xf0 ^ 0xff);
	for (value = 0; value < 65536; value++) {
		pc88_text_dma_reset(&a, (uint16_t)value);
		for (active = 0; active < 2; active++) {
			memory.fast_active = (int)active;
			for (i = 0; i < sizeof(offsets) / sizeof(offsets[0]); i++) {
				const unsigned address = (unsigned)(((uint64_t)value + offsets[i]) % 65536);
				const int fast = active && address >= 61440;
				const unsigned index = fast ? address - 61440 : address;
				PC88_TEXT_ADDRESS resolved = pc88_text_dma_resolve(&a, offsets[i], (int)active);
				CHECK(resolved.offset == index && resolved.fast == fast, "PC88_TEXT_DMA_ADDRESS");
				CHECK(pc88_text_dma_peek(&a, &memory, offsets[i], &result) &&
				          result == (fast ? fast_ram[index] : main_ram[index]),
				      "PC88_TEXT_DMA_SOURCE");
			}
		}
	}
	pc88_text_dma_reset(&a, 0xffff);
	memory.fast_active = 1;
	CHECK(pc88_text_dma_peek(&a, &memory, 0, &result), "PC88_TEXT_DMA_GUARD_FIXTURE");
	result = 0x7b;
	memory.fast_size = 4095;
	CHECK(!pc88_text_dma_peek(&a, &memory, 0, &result) && result == 0x7b,
	      "PC88_TEXT_DMA_SHORT_SOURCE");
	memory.fast_size = sizeof(fast_ram);
	CHECK(pc88_text_dma_peek(&a, &memory, 0, &result), "PC88_TEXT_DMA_GUARD_FIXTURE");
	result = 0x7b;
	memory.fast_ram = NULL;
	CHECK(!pc88_text_dma_peek(&a, &memory, 0, &result) && result == 0x7b,
	      "PC88_TEXT_DMA_MISSING_SOURCE");
	memory.fast_ram = fast_ram;
	memory.fast_active = 0;
	CHECK(pc88_text_dma_peek(&a, &memory, 0, &result), "PC88_TEXT_DMA_GUARD_FIXTURE");
	result = 0x7b;
	memory.main_size = 65535;
	CHECK(!pc88_text_dma_peek(&a, &memory, 0, &result) && result == 0x7b,
	      "PC88_TEXT_DMA_SHORT_SOURCE");
	memory.main_size = sizeof(main_ram);
	CHECK(!pc88_text_dma_peek(NULL, &memory, 0, &result) &&
	          !pc88_text_dma_peek(&a, NULL, 0, &result) &&
	          !pc88_text_dma_peek(&a, &memory, 0, NULL),
	      "PC88_TEXT_DMA_NULL_GUARD");
	puts(
	    "pc88 text DMA: 65536 address pairs, 256 modes, 65534 rejected ports, 1048576 source lookups PASS");
	return 0;
}
