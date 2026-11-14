/*
 * PC-8801 CRTC Emulator
 * Written by Manuke (X88000 1.5.3, public domain).
 * Ported command/state subset to C by Nakata Maho, 2026.
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
#include "upd3301.h"
#include <string.h>

/* X88000 1.5.3 PC88Crtc.h UpdateTimings/PassClock/IsVSync. These are
 * source-model microsecond tables, not hardware measurements. Preserve the
 * remaining countdown across R2 writes, as UpdateTimings does. */
unsigned upd3301_cycle_us(const UPD3301 *c) {
	const int height25 = upd3301_scanlines(c) % 8 == 0;
	return c->hires ? (height25 ? 18050 : 17720) : (height25 ? 16020 : 16270);
}

unsigned upd3301_sync_us(const UPD3301 *c) {
	const int height25 = upd3301_scanlines(c) % 8 == 0;
	return c->hires ? (height25 ? 1933 : 1611) : (height25 ? 3505 : 3755);
}

static void reset_counter(UPD3301 *c) {
	c->frames = 0;
	c->clock_remaining = (uint64_t)c->clock_hz * upd3301_cycle_us(c);
}

int upd3301_clock_configure(UPD3301 *c, uint32_t clock_hz, int hires) {
	if (!c || !clock_hz || (hires != 0 && hires != 1))
		return 0;
	c->clock_hz = clock_hz;
	c->hires = (uint8_t)hires;
	reset_counter(c);
	return 1;
}

int upd3301_clock_valid(const UPD3301 *c) {
	/* A profile change may leave a countdown longer than the new period. */
	return c && c->clock_hz && c->hires <= 1 && c->clock_remaining &&
	       c->clock_remaining <= (uint64_t)c->clock_hz * 18050;
}

int upd3301_in_vsync(const UPD3301 *c) {
	return upd3301_clock_valid(c) &&
	       c->clock_remaining > (uint64_t)c->clock_hz * (upd3301_cycle_us(c) - upd3301_sync_us(c));
}

int upd3301_advance(UPD3301 *c, uint32_t ticks, uint64_t *frames) {
	uint64_t delta = (uint64_t)ticks * 1000000;
	uint64_t count = 0;
	if (!frames || !upd3301_clock_valid(c))
		return 0;
	if (delta < c->clock_remaining) {
		c->clock_remaining -= delta;
	} else {
		const uint64_t cycle = (uint64_t)c->clock_hz * upd3301_cycle_us(c);
		delta -= c->clock_remaining;
		count = 1 + delta / cycle;
		c->clock_remaining = cycle - delta % cycle;
		c->frames = (uint16_t)(c->frames + (uint16_t)count);
		c->interrupt_request |= 1;
	}
	*frames = count;
	return 1;
}

void upd3301_reset(UPD3301 *c) {
	memset(c, 0, sizeof(*c));
	c->reset[0] = 78;
	c->reset[1] = 24;
	c->reset[2] = 0x6f;
	c->reset[3] = 0xe6;
	c->reset[4] = 19;
	c->cursor_enabled = 1;
	c->width80 = 1;
	c->interrupt_mask = 3;
	c->last_attribute = 0xe0;
	upd3301_clock_configure(c, 4000000, 1);
}

void upd3301_command(UPD3301 *c, uint8_t value) {
	c->command = UPD3301_IDLE;
	c->parameter = 0;
	switch (value & 0xe0) {
	case 0x00:
		c->command = UPD3301_RESET;
		c->display_started = 0;
		c->interrupt_request = 0;
		break;
	case 0x20:
		c->display_started = 1;
		c->reverse = value & 1;
		reset_counter(c);
		c->last_attribute = 0xe0;
		c->interrupt_request = 0;
		break;
	case 0x40:
		c->interrupt_mask = value & 3;
		if (value & 3) {
			c->display_started = 0;
			c->interrupt_request = 0;
		}
		break;
	case 0x60:
		c->command = UPD3301_LIGHT_PEN;
		break;
	case 0x80:
		c->command = UPD3301_CURSOR;
		c->cursor_enabled = value & 1;
		break;
	case 0xa0:
		c->interrupt_request = 0;
		break;
	case 0xc0:
		reset_counter(c);
		c->interrupt_request = 0;
		break;
	default:
		break;
	}
}

void upd3301_parameter_write(UPD3301 *c, uint8_t value) {
	if (c->command == UPD3301_RESET) {
		/* X88000 applies each parameter as it arrives, not all five at once. */
		c->reset[c->parameter++] = value;
		if (c->parameter == 5)
			c->command = UPD3301_IDLE;
	} else if (c->command == UPD3301_CURSOR) {
		if (c->parameter == 0) {
			c->cursor_x = value;
			c->parameter = 1;
		} else {
			c->cursor_y = value;
			c->command = UPD3301_IDLE;
		}
	}
}

uint8_t upd3301_parameter_read(UPD3301 *c) {
	if (c->command == UPD3301_LIGHT_PEN && ++c->parameter == 2)
		c->command = UPD3301_IDLE;
	return 0;
}

uint8_t upd3301_status(const UPD3301 *c) {
	/* Preserve X88000's shared 04h indication for either interrupt request.
	 * This is a source-model contract, not a newly verified hardware bit map. */
	return (uint8_t)((c->display_started ? 0x10 : 0) | (c->interrupt_request ? 4 : 0));
}

void upd3301_frame_boundary(UPD3301 *c) {
	c->clock_remaining = (uint64_t)c->clock_hz * upd3301_cycle_us(c);
	c->frames = (uint16_t)(c->frames + 1);
	c->interrupt_request |= 1;
}

unsigned upd3301_columns(const UPD3301 *c) {
	return (c->reset[0] & 127) + 2;
}
unsigned upd3301_rows(const UPD3301 *c) {
	return (c->reset[1] & 63) + 1;
}
unsigned upd3301_scanlines(const UPD3301 *c) {
	return (c->reset[2] & 31) + 1;
}
unsigned upd3301_attribute_pairs(const UPD3301 *c) {
	return (c->reset[4] & 31) + 1;
}
unsigned upd3301_cursor_period(const UPD3301 *c) {
	return ((c->reset[1] >> 6) + 1) * 16;
}
int upd3301_display_active(const UPD3301 *c) {
	return c->display_started && c->dma_started;
}
int upd3301_cursor_phase(const UPD3301 *c) {
	const unsigned period = upd3301_cursor_period(c);
	return c->frames % period > period / 2;
}
int upd3301_attribute_phase(const UPD3301 *c) {
	const unsigned period = upd3301_cursor_period(c) * 2;
	return c->frames % period > period * 3 / 4;
}
