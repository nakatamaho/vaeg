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

int main(void) {
	UPD3301 a, b, saved;
	unsigned value, position, i, clock;
	upd3301_reset(&a);
	CHECK(upd3301_columns(&a) == 80 && upd3301_rows(&a) == 25 && upd3301_scanlines(&a) == 16 &&
	          upd3301_attribute_pairs(&a) == 20 && a.reset[3] == 0xe6 && a.width80 &&
	          a.cursor_enabled && !upd3301_display_active(&a) && upd3301_status(&a) == 0,
	      "UPD3301_POWER_ON");
	for (value = 0; value < 256; value++) {
		for (position = 0; position < 5; position++) {
			upd3301_reset(&a);
			upd3301_command(&a, 0);
			for (i = 0; i < position; i++)
				upd3301_parameter_write(&a, (uint8_t)(i + 1));
			b = a;
			upd3301_parameter_write(&a, (uint8_t)value);
			CHECK(a.reset[position] == value && a.parameter == position + 1 &&
			          a.command == (position == 4 ? UPD3301_IDLE : UPD3301_RESET),
			      "UPD3301_RESET_PARAMETERS");
			for (i = 0; i < 5; i++)
				if (i != position)
					CHECK(a.reset[i] == b.reset[i], "UPD3301_PARAMETER_ISOLATION");
			/* Continuation of a copied device, not a machine-SAV test. */
			upd3301_parameter_write(&b, (uint8_t)value);
			CHECK(!memcmp(&a, &b, sizeof(a)), "UPD3301_COPY_CONTINUATION");
		}
		upd3301_reset(&a);
		a.frames = 173;
		a.interrupt_request = 3;
		a.display_started = a.dma_started = 1;
		a.command = UPD3301_RESET;
		a.parameter = 3;
		upd3301_command(&a, (uint8_t)value);
		CHECK(!a.parameter && a.dma_started, "UPD3301_COMMAND_BOUNDARY");
		switch (value / 32) {
		case 0:
			CHECK(a.command == UPD3301_RESET && !a.display_started && !a.interrupt_request,
			      "UPD3301_STOP");
			break;
		case 1:
			CHECK(a.display_started && a.reverse == value % 2 && !a.frames &&
			          !a.interrupt_request && a.last_attribute == 0xe0,
			      "UPD3301_START");
			break;
		case 2:
			CHECK(a.interrupt_mask == value % 4 && a.display_started == (value % 4 == 0) &&
			          a.interrupt_request == (value % 4 == 0 ? 3 : 0),
			      "UPD3301_MASK");
			break;
		case 3:
			CHECK(a.command == UPD3301_LIGHT_PEN, "UPD3301_LIGHT_PEN");
			CHECK(upd3301_parameter_read(&a) == 0 && a.command == UPD3301_LIGHT_PEN &&
			          upd3301_parameter_read(&a) == 0 && a.command == UPD3301_IDLE,
			      "UPD3301_LIGHT_PEN");
			break;
		case 4:
			CHECK(a.command == UPD3301_CURSOR && a.cursor_enabled == value % 2, "UPD3301_CURSOR");
			upd3301_parameter_write(&a, (uint8_t)value);
			b = a;
			upd3301_parameter_write(&a, 24);
			upd3301_parameter_write(&b, 24);
			CHECK(a.cursor_x == value && a.cursor_y == 24 && a.command == UPD3301_IDLE &&
			          !memcmp(&a, &b, sizeof(a)),
			      "UPD3301_CURSOR_CONTINUATION");
			break;
		case 5:
			CHECK(!a.interrupt_request && a.frames == 173, "UPD3301_INTERRUPT_RESET");
			break;
		case 6:
			CHECK(!a.interrupt_request && !a.frames, "UPD3301_COUNTER_RESET");
			break;
		default:
			CHECK(a.command == UPD3301_IDLE && a.display_started && a.interrupt_request == 3 &&
			          a.frames == 173,
			      "UPD3301_RESERVED_COMMAND");
			break;
		}
	}
	for (value = 0; value < 256; value++) {
		upd3301_reset(&a);
		upd3301_command(&a, 0);
		for (i = 0; i < 5; i++)
			upd3301_parameter_write(&a, (uint8_t)value);
		CHECK(upd3301_columns(&a) == value % 128 + 2 && upd3301_rows(&a) == value % 64 + 1 &&
		          upd3301_scanlines(&a) == value % 32 + 1 &&
		          upd3301_attribute_pairs(&a) == value % 32 + 1,
		      "UPD3301_DECODE");
		saved = a;
		upd3301_parameter_write(&a, 123);
		CHECK(!memcmp(&a, &saved, sizeof(a)), "UPD3301_IDLE_DATA");
	}
	for (value = 0; value < 4; value++) {
		const unsigned period = (value + 1) * 16;
		upd3301_reset(&a);
		a.reset[1] = (uint8_t)(value * 64 + 24);
		for (clock = 0; clock < 65536; clock++) {
			CHECK(upd3301_cursor_period(&a) == period &&
			          upd3301_cursor_phase(&a) == (clock % period > period / 2) &&
			          upd3301_attribute_phase(&a) == (clock % (period * 2) > period * 3 / 2),
			      "UPD3301_PHASE");
			upd3301_frame_boundary(&a);
		}
		CHECK(!a.frames && a.interrupt_request == 1, "UPD3301_FRAME_WRAP");
	}
	upd3301_reset(&a);
	saved = a;
	upd3301_reset(&b);
	upd3301_command(&b, 0x21);
	CHECK(upd3301_status(&b) == 0x10 && !upd3301_display_active(&b), "UPD3301_DMA_GATE");
	b.dma_started = 1;
	upd3301_frame_boundary(&b);
	CHECK(upd3301_status(&b) == 0x14 && upd3301_display_active(&b), "UPD3301_STATUS");
	CHECK(!memcmp(&a, &saved, sizeof(a)), "UPD3301_INSTANCE_ISOLATION");
	puts("upd3301: 1280 parameter cases, 256 commands, 256 decodes, 262144 phase samples PASS");
	return 0;
}
