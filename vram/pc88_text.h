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
 *
 * Independent text decoding derived from Manuke's public-domain X88000 1.5.3
 * X88ScreenDrawer.cpp (RatchText). See ADR-0016 for provenance and deviations.
 */
#ifndef VAEG_PC88_TEXT_H
#define VAEG_PC88_TEXT_H
#include "io/upd3301.h"
#include "io/pc88_text_dma.h"
#ifdef __cplusplus
extern "C" {
#endif

enum {
	PC88_TEXT_OK = 0,
	PC88_TEXT_BAD_ARGUMENT = 1,
	PC88_TEXT_UNSUPPORTED = 2,
	PC88_TEXT_GEOMETRY = 3,
	PC88_TEXT_SOURCE = 4
};
enum {
	PC88_TEXT_REVERSE = 1,
	PC88_TEXT_GRAPHIC = 2,
	PC88_TEXT_OVERLINE = 4,
	PC88_TEXT_UNDERLINE = 8
};
typedef struct {
	uint8_t character, color, flags;
} PC88_TEXT_CELL;
typedef struct {
	uint8_t count, cell_width, carry;
	PC88_TEXT_CELL cells[80];
} PC88_TEXT_ROW;

/* Only the source model's 80 logical slots, transparent mono/color and
 * no-attribute modes are supported here. Other geometry/modes return zero. */
unsigned pc88_text_row_pitch(const UPD3301 *crtc);
/* Bounded row snapshot, not a DMA transfer or a clocked scanout. The caller
 * owns display blanking, row/frame scheduling and the packed attribute carry.
 * On error neither output nor device state changes. Forty-column output is
 * packed into 40 cells, each consuming two logical slots and 16 pixel columns. */
int pc88_text_decode_row(const UPD3301 *crtc, const PC88_TEXT_DMA *dma,
                         const PC88_TEXT_MEMORY *memory, unsigned row, uint8_t carry,
                         PC88_TEXT_ROW *output);
/* Stateless glyph-row composition. The caller supplies an eight-bit glyph
 * row from its chosen font/PCG/semigraphics source. Scanline/height are unscaled
 * glyph-row coordinates; vertical scaling and palette/priority belong to the
 * caller. Invalid arguments draw zero. Bit 7 (8 pixels) or bit 15 (16 pixels) is the leftmost output pixel. */
uint16_t pc88_text_raster_bits(const PC88_TEXT_CELL *cell, uint8_t glyph, unsigned scanline,
                               unsigned height, unsigned width);
#ifdef __cplusplus
}
#endif
#endif
