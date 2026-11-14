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
#include "vram/pc88_text.h"
#include <stdio.h>
#include <string.h>
#define CHECK(condition, code)                                                                     \
	do {                                                                                           \
		if (!(condition)) {                                                                        \
			fprintf(stderr, "%s line=%d\n", code, __LINE__);                                       \
			return 1;                                                                              \
		}                                                                                          \
	} while (0)
static uint8_t ram[65536];

int main(void) {
	UPD3301 c;
	PC88_TEXT_DMA dma;
	PC88_TEXT_MEMORY memory = {ram, sizeof(ram), NULL, 0, 0};
	PC88_TEXT_ROW out, saved;
	unsigned pairs, width, mode, row, x, i, position, phase, enabled, reverse, y;
	unsigned glyph, flags, scanline;
	upd3301_reset(&c);
	c.cursor_enabled = 0;
	pc88_text_dma_reset(&dma, 0xff00);
	for (pairs = 1; pairs <= 32; pairs++) {
		const unsigned pitch = 80 + pairs * 2;
		memset(ram, 0, sizeof(ram));
		for (row = 0; row < 25; row++) {
			for (x = 0; x < 80; x++)
				ram[(dma.address + row * pitch + x) % 65536] = (uint8_t)(row + 1);
			for (i = 0; i < pairs; i++) {
				ram[(dma.address + row * pitch + 80 + i * 2) % 65536] = i ? 255 : 0;
				ram[(dma.address + row * pitch + 81 + i * 2) % 65536] = 0;
			}
		}
		for (width = 0; width < 2; width++) {
			c.width80 = (uint8_t)width;
			for (mode = 0; mode <= 2; mode += 2) {
				c.reset[4] = (uint8_t)(mode * 32 + pairs - 1);
				CHECK(pc88_text_row_pitch(&c) == pitch, "PC88_TEXT_PITCH");
				for (row = 0; row < 25; row++) {
					ram[(dma.address + row * pitch + 81) % 65536] =
					    mode ? (uint8_t)((row % 8) * 32 + 8) : 0;
					CHECK(pc88_text_decode_row(&c, &dma, &memory, row, 0xe0, &out) == PC88_TEXT_OK,
					      "PC88_TEXT_ROW_READ");
					CHECK(out.count == (width ? 80 : 40) && out.cell_width == (width ? 8 : 16),
					      "PC88_TEXT_CELL_GEOMETRY");
					for (x = 0; x < out.count; x++)
						CHECK(out.cells[x].character == row + 1 && !out.cells[x].flags &&
						          out.cells[x].color == (mode ? row % 8 : 7),
						      "PC88_TEXT_ROW_PITCH");
				}
			}
		}
	}
	pc88_text_dma_reset(&dma, 0);
	c.reset[4] = 0x41;
	memset(ram, 'A', sizeof(ram));
	/* Equivalent two-colour start-position and end-position lists. */
	for (width = 0; width < 2; width++) {
		c.width80 = (uint8_t)width;
		for (mode = 0; mode < 2; mode++) {
			ram[80] = mode ? 3 : 0;
			ram[81] = 0x28;
			ram[82] = mode ? 80 : 3;
			ram[83] = 0x48;
			CHECK(pc88_text_decode_row(&c, &dma, &memory, 0, 0xe0, &out) == PC88_TEXT_OK,
			      "PC88_TEXT_POSITION_LIST");
			for (x = 0; x < out.count; x++)
				CHECK(out.cells[x].color == (x * (width ? 1 : 2) < 3 ? 1 : 2),
				      "PC88_TEXT_POSITION_LIST");
		}
	}
	/* Duplicate positions consume one pair per displayed cell, not a sorted
	 * or catch-up walk. This also checks the 40-column skipped logical slots. */
	c.reset[4] = 0x42;
	ram[80] = ram[82] = ram[84] = 0;
	ram[81] = 0x28;
	ram[83] = 0x48;
	ram[85] = 0x68;
	for (width = 0; width < 2; width++) {
		c.width80 = (uint8_t)width;
		CHECK(pc88_text_decode_row(&c, &dma, &memory, 0, 0xe0, &out) == PC88_TEXT_OK,
		      "PC88_TEXT_DUPLICATE_POSITIONS");
		for (x = 0; x < out.count; x++)
			CHECK(out.cells[x].color == (x < 2 ? x + 1 : 3), "PC88_TEXT_DUPLICATE_POSITIONS");
	}
	c.reset[4] = 0x41;
	c.width80 = 1;
	ram[80] = 0;
	ram[81] = 0x38;
	ram[82] = 1;
	ram[83] = 0x34;
	ram[164] = 0;
	ram[165] = 0x48;
	ram[166] = 255;
	ram[167] = 0;
	CHECK(pc88_text_decode_row(&c, &dma, &memory, 0, 0xe0, &out) == PC88_TEXT_OK &&
	          out.carry == 0x34 && out.cells[79].flags == 15,
	      "PC88_TEXT_ATTRIBUTE_CARRY");
	CHECK(pc88_text_decode_row(&c, &dma, &memory, 1, out.carry, &out) == PC88_TEXT_OK &&
	          out.carry == 0x44 && out.cells[0].flags == PC88_TEXT_REVERSE,
	      "PC88_TEXT_LINE_DECORATION_RESET");
	/* Secret and attribute blink are distinct from cursor blink. */
	for (i = 0; i < 4; i++) {
		ram[81] = (uint8_t)i;
		ram[82] = 255;
		for (phase = 24; phase <= 25; phase++) {
			c.frames = (uint16_t)phase;
			CHECK(pc88_text_decode_row(&c, &dma, &memory, 0, 0xe0, &out) == PC88_TEXT_OK &&
			          out.cells[0].character == ((i & 1) || ((i & 2) && phase == 25) ? 0 : 'A'),
			      "PC88_TEXT_SECRET_BLINK");
		}
	}
	for (x = 0; x < 80; x++)
		ram[84 + x] = (uint8_t)(x + 1);
	ram[164] = 0;
	ram[165] = 0xe8;
	ram[166] = 255;
	for (width = 0; width < 2; width++)
		for (position = 0; position < 256; position++)
			for (phase = 8; phase <= 9; phase++)
				for (enabled = 0; enabled < 2; enabled++)
					for (reverse = 0; reverse < 2; reverse++)
						for (y = 1; y <= 2; y++) {
							c.width80 = (uint8_t)width;
							c.cursor_x = (uint8_t)position;
							c.cursor_y = (uint8_t)y;
							c.frames = (uint16_t)phase;
							c.cursor_enabled = (uint8_t)enabled;
							c.reverse = (uint8_t)reverse;
							CHECK(pc88_text_decode_row(&c, &dma, &memory, 1, 0xe0, &out) ==
							          PC88_TEXT_OK,
							      "PC88_TEXT_CURSOR_FIXTURE");
							for (x = 0; x < out.count; x++) {
								const unsigned logical = x * (width ? 1 : 2);
								const int inverted = reverse ^ (enabled && phase == 9 && y == 1 &&
								                                position == logical);
								CHECK(out.cells[x].character == logical + 1 &&
								          !!(out.cells[x].flags & PC88_TEXT_REVERSE) == inverted,
								      "PC88_TEXT_CURSOR_POSITION");
								CHECK(
								    pc88_text_raster_bits(&out.cells[x], 0, 4, 8, out.cell_width) ==
								        (inverted ? (width ? 0xff : 0xffff) : 0),
								    "PC88_TEXT_CURSOR_WIDTH");
							}
						}
	for (glyph = 0; glyph < 256; glyph++)
		for (flags = 0; flags < 16; flags++)
			for (width = 8; width <= 16; width += 8)
				for (scanline = 0; scanline < 8; scanline++) {
					PC88_TEXT_CELL cell = {0, 7, (uint8_t)flags};
					const uint16_t bits =
					    pc88_text_raster_bits(&cell, (uint8_t)glyph, scanline, 8, width);
					for (x = 0; x < width; x++) {
						unsigned expected = (glyph >> (7 - x / (width / 8))) & 1;
						if (((flags & PC88_TEXT_OVERLINE) && scanline == 0) ||
						    ((flags & PC88_TEXT_UNDERLINE) && scanline == 7))
							expected = 1;
						expected ^= !!(flags & PC88_TEXT_REVERSE);
						CHECK(((bits >> (width - 1 - x)) & 1) == expected, "PC88_TEXT_RASTER_BITS");
					}
				}
	upd3301_reset(&c);
	c.cursor_enabled = 0;
	c.reset[4] = 0x41;
	memory.main_size = 84;
	CHECK(pc88_text_decode_row(&c, &dma, &memory, 0, 0xe0, &out) == PC88_TEXT_OK,
	      "PC88_TEXT_ATOMIC_FIXTURE");
	saved = out;
	memory.main_size = 83;
	CHECK(pc88_text_decode_row(&c, &dma, &memory, 0, 0xe0, &out) == PC88_TEXT_SOURCE &&
	          !memcmp(&out, &saved, sizeof(out)),
	      "PC88_TEXT_ATOMIC_SOURCE");
	memory.main_size = 84;
	for (mode = 3; mode < 8; mode++) {
		c.reset[4] = 0x41;
		CHECK(pc88_text_decode_row(&c, &dma, &memory, 0, 0xe0, &out) == PC88_TEXT_OK,
		      "PC88_TEXT_ATOMIC_FIXTURE");
		c.reset[4] = (uint8_t)(mode * 32 + 1);
		CHECK(pc88_text_decode_row(&c, &dma, &memory, 0, 0xe0, &out) == PC88_TEXT_UNSUPPORTED &&
		          !memcmp(&out, &saved, sizeof(out)),
		      "PC88_TEXT_UNSUPPORTED_MODE");
	}
	c.reset[4] = 0x41;
	CHECK(pc88_text_decode_row(&c, &dma, &memory, 0, 0xe0, &out) == PC88_TEXT_OK,
	      "PC88_TEXT_ATOMIC_FIXTURE");
	c.reset[0] = 0;
	CHECK(pc88_text_decode_row(&c, &dma, &memory, 0, 0xe0, &out) == PC88_TEXT_GEOMETRY &&
	          !memcmp(&out, &saved, sizeof(out)),
	      "PC88_TEXT_UNSUPPORTED_GEOMETRY");
	c.reset[0] = 78;
	c.reset[4] = 32;
	memory.main_size = 80;
	CHECK(pc88_text_decode_row(&c, &dma, &memory, 0, 0xff, &out) == PC88_TEXT_OK &&
	          out.carry == 0xe0 && out.cells[0].character == 'A' && !out.cells[0].flags,
	      "PC88_TEXT_NO_ATTRIBUTES");
	/* Compose device time, display/DMA gates, row decoding and the 40-column
	 * cursor mask without a host frame callback or a VA cursor sprite. */
	{
		uint64_t crossed;
		upd3301_reset(&c);
		c.reset[4] = 32;
		c.width80 = 0;
		pc88_text_dma_write(&dma, 0x68, 0x84);
		c.dma_started = (uint8_t)pc88_text_dma_enabled(&dma);
		upd3301_command(&c, 0x20);
		CHECK(upd3301_display_active(&c) && upd3301_advance(&c, 577600, &crossed) && crossed == 8 &&
		          pc88_text_decode_row(&c, &dma, &memory, 0, 0xe0, &out) == PC88_TEXT_OK &&
		          pc88_text_raster_bits(&out.cells[0], 0, 4, 8, out.cell_width) == 0,
		      "PC88_TEXT_CLOCK_CURSOR");
		CHECK(upd3301_advance(&c, 72200, &crossed) && crossed == 1 &&
		          pc88_text_decode_row(&c, &dma, &memory, 0, 0xe0, &out) == PC88_TEXT_OK &&
		          pc88_text_raster_bits(&out.cells[0], 0, 4, 8, out.cell_width) == 0xffff &&
		          pc88_text_raster_bits(&out.cells[1], 0, 4, 8, out.cell_width) == 0,
		      "PC88_TEXT_CLOCK_CURSOR");
	}
	puts(
	    "pc88 text: 3200 pitched rows, 8192 cursor cases, 65536 raster cases, clock/cursor composition PASS");
	return 0;
}
