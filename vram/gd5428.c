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
#include "gd5428.h"
#include <string.h>

/* [OPEN] Provisional 5428 identity; replace only from VA/VA2 WSRLOG.TXT. */
#define GD5428_CR27 0x98

void gd5428_reset(GD5428 *c) {
	memset(c, 0, sizeof(*c));
	c->cr[0x27] = GD5428_CR27;
	c->pixel_mask = 0xff;
	c->dac_mode = 3;
}

int gd5428_state_valid(const GD5428 *c) {
	return c->attr_data <= 1 && c->dac_read_component < 3 && c->dac_write_component < 3 &&
	       (c->dac_mode == 0 || c->dac_mode == 3) && c->cr[0x27] == GD5428_CR27;
}

uint8_t gd5428_read(GD5428 *c, uint16_t port) {
	uint8_t value;
	switch (port) {
	case 0x3c0:
		return c->attr_index;
	case 0x3c1:
		return (c->attr_index & 0x1f) < 21 ? c->attr[c->attr_index & 0x1f] : 0xff;
	case 0x3c3:
		return c->enable;
	case 0x3c4:
		return c->sr_index;
	case 0x3c5:
		if (c->sr_index == 6)
			return c->sr[6] == 0x12 ? 0x12 : 0x0f;
		return c->sr_index < 32 ? c->sr[c->sr_index] : 0xff;
	case 0x3c6:
		return c->pixel_mask;
	case 0x3c7:
		return c->dac_mode;
	case 0x3c8:
		return c->dac_write;
	case 0x3c9:
		value = c->dac[c->dac_read][c->dac_read_component++];
		if (c->dac_read_component == 3) {
			c->dac_read_component = 0;
			c->dac_read++;
		}
		return value;
	case 0x3ca:
		return c->feature;
	case 0x3cc:
		return c->misc;
	case 0x3ce:
		return c->gr_index;
	case 0x3cf:
		return c->gr_index < 64 ? c->gr[c->gr_index] : 0xff;
	case 0x3d4:
		return c->cr_index;
	case 0x3d5:
		return c->cr_index < 64 ? c->cr[c->cr_index] : 0xff;
	case 0x3da:
		c->attr_data = 0;
		/* No display timing in W1. Retrace belongs to W5. */
		return 0;
	default:
		return 0xff;
	}
}

void gd5428_write(GD5428 *c, uint16_t port, uint8_t value) {
	switch (port) {
	case 0x3c0:
		if (!c->attr_data)
			c->attr_index = value;
		else if ((c->attr_index & 0x1f) < 21)
			c->attr[c->attr_index & 0x1f] = value;
		c->attr_data ^= 1;
		break;
	case 0x3c2:
		c->misc = value;
		break;
	case 0x3c3:
		c->enable = value;
		break;
	case 0x3c4:
		c->sr_index = value;
		break;
	case 0x3c5:
		if (c->sr_index < 32)
			c->sr[c->sr_index] = value;
		break;
	case 0x3c6:
		c->pixel_mask = value;
		break;
	case 0x3c7:
		c->dac_read = value;
		c->dac_read_component = 0;
		c->dac_mode = 3;
		break;
	case 0x3c8:
		c->dac_write = value;
		c->dac_write_component = 0;
		c->dac_mode = 0;
		break;
	case 0x3c9:
		c->dac[c->dac_write][c->dac_write_component++] = value & 0x3f;
		if (c->dac_write_component == 3) {
			c->dac_write_component = 0;
			c->dac_write++;
		}
		break;
	case 0x3ce:
		c->gr_index = value;
		break;
	case 0x3cf:
		if (c->gr_index < 64)
			c->gr[c->gr_index] = value;
		break;
	case 0x3d4:
		c->cr_index = value;
		break;
	case 0x3d5:
		if (c->cr_index < 64 && c->cr_index != 0x27)
			c->cr[c->cr_index] = value;
		break;
	case 0x3da:
		c->feature = value;
		break;
	}
}
