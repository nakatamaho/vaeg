/*
 * Independent text decoding derived from Manuke's X88000 1.5.3
 * X88ScreenDrawer.cpp (public domain), adapted to C by Nakata Maho.
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
#include "pc88_text.h"
#include <string.h>

unsigned pc88_text_row_pitch(const UPD3301 *c) {
	unsigned mode;
	if (!c || upd3301_columns(c) != 80)
		return 0;
	mode = c->reset[4] >> 5;
	if (mode == 1)
		return 80;
	return (mode == 0 || mode == 2) ? 80 + 2 * upd3301_attribute_pairs(c) : 0;
}

int pc88_text_decode_row(const UPD3301 *c, const PC88_TEXT_DMA *dma, const PC88_TEXT_MEMORY *memory,
                         unsigned row, uint8_t carry, PC88_TEXT_ROW *output) {
	uint8_t bytes[144];
	PC88_TEXT_ROW decoded;
	unsigned mode, pitch, pairs, i, next = 0;
	unsigned color = carry >> 5;
	int secret = carry & 1, blink = carry & 2, reverse = carry & 4;
	int graphic = carry & 16, overline = 0, underline = 0;
	if (!c || !dma || !memory || !output)
		return PC88_TEXT_BAD_ARGUMENT;
	mode = c->reset[4] >> 5;
	if (mode != 0 && mode != 1 && mode != 2)
		return PC88_TEXT_UNSUPPORTED;
	pitch = pc88_text_row_pitch(c);
	if (!pitch || row >= upd3301_rows(c))
		return PC88_TEXT_GEOMETRY;
	for (i = 0; i < pitch; i++)
		if (!pc88_text_dma_peek(dma, memory, row * pitch + i, &bytes[i]))
			return PC88_TEXT_SOURCE;
	memset(&decoded, 0, sizeof(decoded));
	decoded.count = c->width80 ? 80 : 40;
	decoded.cell_width = c->width80 ? 8 : 16;
	pairs = mode == 1 ? 0 : upd3301_attribute_pairs(c);
	if (!pairs) {
		color = 7;
		secret = blink = reverse = graphic = 0;
	}
	for (i = 0; i < decoded.count; i++) {
		const unsigned x = c->width80 ? i : i * 2;
		PC88_TEXT_CELL *cell = &decoded.cells[i];
		if (next < pairs) {
			/* A nonzero first position is an end-position list; zero selects
			 * start positions. Like RatchText, consume at most one pair per
			 * displayed cell, including duplicate/out-of-order positions. */
			unsigned trigger =
			    bytes[80] == 0 ? bytes[80 + next * 2] : (next ? bytes[80 + (next - 1) * 2] : 0);
			if (x >= trigger) {
				const uint8_t attr = bytes[81 + next * 2];
				next++;
				if (mode == 2 && (attr & 8)) {
					color = attr >> 5;
					graphic = attr & 16;
				} else {
					secret = attr & 1;
					blink = attr & 2;
					reverse = attr & 4;
					overline = attr & 16;
					underline = attr & 32;
					if (mode == 0)
						graphic = attr & 128;
				}
			}
		}
		cell->character = (secret || (blink && upd3301_attribute_phase(c))) ? 0 : bytes[x];
		cell->color = (uint8_t)color;
		cell->flags =
		    (uint8_t)((graphic ? PC88_TEXT_GRAPHIC : 0) | (overline ? PC88_TEXT_OVERLINE : 0) |
		              (underline ? PC88_TEXT_UNDERLINE : 0));
		if (!!reverse ^ !!c->reverse ^
		    !!(c->cursor_enabled && upd3301_cursor_phase(c) && x == c->cursor_x &&
		       row == c->cursor_y))
			cell->flags |= PC88_TEXT_REVERSE;
	}
	decoded.carry = (uint8_t)((color << 5) | (secret ? 1 : 0) | (blink ? 2 : 0) |
	                          (reverse ? 4 : 0) | (graphic ? 16 : 0));
	*output = decoded;
	return PC88_TEXT_OK;
}

uint16_t pc88_text_raster_bits(const PC88_TEXT_CELL *cell, uint8_t glyph, unsigned scanline,
                               unsigned height, unsigned width) {
	uint16_t result = 0;
	unsigned bit;
	if (!cell || !height || scanline >= height || (width != 8 && width != 16))
		return 0;
	if (((cell->flags & PC88_TEXT_OVERLINE) && scanline == 0) ||
	    ((cell->flags & PC88_TEXT_UNDERLINE) && scanline == height - 1))
		glyph = 0xff;
	if (cell->flags & PC88_TEXT_REVERSE)
		glyph ^= 0xff;
	if (width == 8)
		return glyph;
	for (bit = 0; bit < 8; bit++)
		if (glyph & (1u << bit))
			result |= (uint16_t)(3u << (bit * 2));
	return result;
}
