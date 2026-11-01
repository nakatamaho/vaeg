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
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef VAEG_GD5428_H
#define VAEG_GD5428_H
#include <stdint.h>
/* W1 only: byte-only wire representation, no VRAM, display or BLT. */
typedef struct {
	uint8_t sr[32], gr[64], cr[64], attr[21], dac[256][3];
	uint8_t sr_index, gr_index, cr_index, attr_index, attr_data;
	uint8_t dac_read, dac_write, dac_read_component, dac_write_component, dac_mode;
	uint8_t pixel_mask, misc, feature, enable;
} GD5428;
#ifdef __cplusplus
extern "C" {
#endif
void gd5428_reset(GD5428 *chip);
uint8_t gd5428_read(GD5428 *chip, uint16_t vga_port);
void gd5428_write(GD5428 *chip, uint16_t vga_port, uint8_t value);
int gd5428_state_valid(const GD5428 *chip);
#ifdef __cplusplus
}
#endif
#endif
