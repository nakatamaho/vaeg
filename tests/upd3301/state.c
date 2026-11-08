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
#include "io/pc88_display_state.h"
#include <stdio.h>
#include <string.h>
#define CHECK(condition, code)                                                                     \
	do {                                                                                           \
		if (!(condition)) {                                                                        \
			fprintf(stderr, "%s line=%d\n", code, __LINE__);                                       \
			return 1;                                                                              \
		}                                                                                          \
	} while (0)

/* Well-formed all-zero payload used as the single-mutation base: reset state
 * at the 4 MHz high-resolution 16-scanline (25-line) profile, idle DMA. */
static const uint8_t zero_payload[PC88_DISPLAY_STATE_BYTES] = {
    0x01, 0x4e, 0x18, 0x6f, 0xe6, 0x13, 0x00, 0x00, 0x00, 0x00, 0x31, 0x03, 0x00, 0xe0, 0x00, 0x00,
    0x00, 0x09, 0x3d, 0x00, 0x00, 0x92, 0x74, 0xcf, 0x10, 0x00, 0x00, 0x00, 0x00, 0xf3, 0x00, 0x00};

int main(void) {
	const PC88_DISPLAY_CLOCK_BINDING binding = {4000000, 1};
	PC88_DISPLAY_CLOCK_BINDING wrong = {4000000, 0};
	UPD3301 a, b;
	PC88_TEXT_DMA d, e;
	uint8_t payload[PC88_DISPLAY_STATE_BYTES];
	uint8_t copy[PC88_DISPLAY_STATE_BYTES];
	unsigned value, i;
	upd3301_reset(&a);
	upd3301_clock_configure(&a, binding.clock_hz, binding.hires);
	pc88_text_dma_reset(&d, 0xf300);
	CHECK(pc88_display_state_valid(&a, &d) == PC88_STATE_OK, "PC88_STATE_RESET_FIXTURE");
	CHECK(pc88_display_state_encode(&a, &d, &binding, payload, sizeof(payload)) == PC88_STATE_OK &&
	          !memcmp(payload, zero_payload, sizeof(payload)),
	      "PC88_STATE_ENCODE");
	CHECK(pc88_display_state_encode(&a, &d, &wrong, payload, sizeof(payload)) == PC88_STATE_BINDING,
	      "PC88_STATE_ENCODE_BINDING");
	b = a;
	e = d;
	CHECK(pc88_display_state_decode(&b, &e, &binding, payload, sizeof(payload)) == PC88_STATE_OK &&
	          !memcmp(&b, &a, sizeof(a)) && !memcmp(&e, &d, sizeof(d)),
	      "PC88_STATE_ROUNDTRIP");
	CHECK(pc88_display_clock_matches(&a, &binding) == PC88_STATE_OK &&
	          pc88_display_clock_matches(&a, &wrong) == PC88_STATE_BINDING &&
	          pc88_display_clock_matches(NULL, &binding) == PC88_STATE_ARGUMENT,
	      "PC88_STATE_BINDING");
	for (value = 0; value < 256; value++) {
		upd3301_reset(&b);
		upd3301_clock_configure(&b, binding.clock_hz, binding.hires);
		pc88_text_dma_reset(&e, 0xf300);
		upd3301_command(&b, (uint8_t)value);
		CHECK(pc88_display_state_valid(&b, &e) == PC88_STATE_OK, "PC88_STATE_COMMAND_FIXTURE");
		switch (value / 32) {
		case 0:
			b.parameter = 5;
			CHECK(pc88_display_state_valid(&b, &e) == PC88_STATE_COMMAND, "PC88_STATE_COMMAND");
			b.parameter = 0;
			b.display_started = 1;
			CHECK(pc88_display_state_valid(&b, &e) == PC88_STATE_COMMAND, "PC88_STATE_COMMAND");
			break;
		case 1:
			CHECK(pc88_display_state_valid(&b, &e) == PC88_STATE_OK, "PC88_STATE_COMMAND");
			b.display_started = 1;
			b.parameter = 3;
			CHECK(pc88_display_state_valid(&b, &e) == PC88_STATE_COMMAND, "PC88_STATE_COMMAND");
			break;
		case 3:
		case 4:
			b.parameter = 2;
			CHECK(pc88_display_state_valid(&b, &e) == PC88_STATE_COMMAND, "PC88_STATE_COMMAND");
			b.parameter = 0;
			b.display_started = 1;
			b.dma_started = 1;
			CHECK(pc88_text_dma_write(&e, 0x68, 0x84), "PC88_STATE_COMMAND_FIXTURE");
			CHECK(pc88_display_state_valid(&b, &e) == PC88_STATE_OK, "PC88_STATE_COMMAND");
			break;
		case 2:
			b.display_started = 0;
			CHECK(pc88_display_state_valid(&b, &e) == PC88_STATE_OK, "PC88_STATE_COMMAND");
			b.interrupt_mask = 4;
			CHECK(pc88_display_state_valid(&b, &e) == PC88_STATE_INTERRUPT, "PC88_STATE_INTERRUPT");
			b.interrupt_mask = value % 4;
			b.interrupt_request = 4;
			CHECK(pc88_display_state_valid(&b, &e) == PC88_STATE_INTERRUPT, "PC88_STATE_INTERRUPT");
			b.interrupt_request = value % 4;
			b.last_attribute = 0xe8;
			CHECK(pc88_display_state_valid(&b, &e) == PC88_STATE_ATTRIBUTE, "PC88_STATE_ATTRIBUTE");
			break;
		default:
			b.command = 7;
			CHECK(pc88_display_state_valid(&b, &e) == PC88_STATE_COMMAND, "PC88_STATE_COMMAND");
			break;
		}
	}
	for (i = 0; i < 6; i++) {
		upd3301_reset(&b);
		upd3301_clock_configure(&b, binding.clock_hz, binding.hires);
		b.hires = 2;
		CHECK(pc88_display_state_valid(&b, &e) == PC88_STATE_FLAGS, "PC88_STATE_FLAGS");
		b = a;
		if (i == 0)
			b.cursor_enabled = 2;
		if (i == 1)
			b.display_started = 3;
		if (i == 2)
			b.dma_started = 2;
		if (i == 3)
			b.reverse = 2;
		if (i == 4)
			b.width80 = 2;
		if (i == 5)
			b.hires = 2;
		CHECK(pc88_display_state_valid(&b, &e) == PC88_STATE_FLAGS, "PC88_STATE_FLAGS");
	}
	for (i = 0; i < 2; i++) {
		upd3301_reset(&b);
		upd3301_clock_configure(&b, binding.clock_hz, binding.hires);
		if (i)
			b.clock_remaining = UINT64_MAX;
		else
			b.clock_remaining = 0;
		CHECK(pc88_display_state_valid(&b, &e) == PC88_STATE_CLOCK, "PC88_STATE_CLOCK");
	}
	e = d;
	e.high_next = 2;
	CHECK(pc88_display_state_valid(&a, &e) == PC88_STATE_DMA, "PC88_STATE_DMA");
	pc88_text_dma_reset(&e, 0xf300);
	pc88_text_dma_write(&e, 0x68, 0x84);
	CHECK(pc88_display_state_valid(&a, &e) == PC88_STATE_DMA, "PC88_STATE_DMA");
	CHECK(pc88_display_state_encode(&a, &e, &binding, payload, sizeof(payload)) == PC88_STATE_DMA,
	      "PC88_STATE_ENCODE_ATOMIC");
	/* Cross-checked mutations beyond the byte sweep. */
	memcpy(copy, zero_payload, sizeof(copy));
	copy[0] = 2;
	CHECK(pc88_display_state_decode(&b, &e, &binding, copy, sizeof(copy)) == PC88_STATE_VERSION,
	      "PC88_STATE_VERSION");
	copy[0] = 1;
	copy[10] |= 0x40;
	CHECK(pc88_display_state_decode(&b, &e, &binding, copy, sizeof(copy)) == PC88_STATE_FLAGS,
	      "PC88_STATE_FLAGS");
	copy[10] = zero_payload[10];
	copy[12] = 5;
	CHECK(pc88_display_state_decode(&b, &e, &binding, copy, sizeof(copy)) == PC88_STATE_INTERRUPT,
	      "PC88_STATE_INTERRUPT");
	copy[12] = zero_payload[12];
	copy[16] = 1; /* wrong clock domain */
	CHECK(pc88_display_state_decode(&b, &e, &binding, copy, sizeof(copy)) == PC88_STATE_BINDING,
	      "PC88_STATE_BINDING");
	copy[16] = zero_payload[16];
	copy[6] = 6;
	CHECK(pc88_display_state_decode(&b, &e, &binding, copy, sizeof(copy)) == PC88_STATE_COMMAND,
	      "PC88_STATE_COMMAND");
	copy[6] = zero_payload[6];
	copy[7] = 3;
	CHECK(pc88_display_state_decode(&b, &e, &binding, copy, sizeof(copy)) == PC88_STATE_COMMAND,
	      "PC88_STATE_COMMAND");
	copy[7] = zero_payload[7];
	copy[13] = 0xe8;
	CHECK(pc88_display_state_decode(&b, &e, &binding, copy, sizeof(copy)) == PC88_STATE_ATTRIBUTE,
	      "PC88_STATE_ATTRIBUTE");
	copy[13] = zero_payload[13];
	copy[20] = 0x80; /* impossible remaining phase */
	CHECK(pc88_display_state_decode(&b, &e, &binding, copy, sizeof(copy)) == PC88_STATE_CLOCK,
	      "PC88_STATE_CLOCK");
	copy[20] = zero_payload[20];
	copy[31] = 0x84;
	CHECK(pc88_display_state_decode(&b, &e, &binding, copy, sizeof(copy)) == PC88_STATE_DMA,
	      "PC88_STATE_DMA");
	copy[31] = zero_payload[31];
	copy[30] = 2;
	CHECK(pc88_display_state_decode(&b, &e, &binding, copy, sizeof(copy)) == PC88_STATE_DMA,
	      "PC88_STATE_DMA");
	copy[30] = zero_payload[30];
	CHECK(pc88_display_state_decode(&b, &e, &binding, copy, sizeof(copy)) == PC88_STATE_OK,
	      "PC88_STATE_OK");
	/* Every payload byte must be enforced or round-trip exactly: a flipped
	 * byte either decodes non-OK, or re-encodes to the flipped payload. */
	for (i = 0; i < sizeof(copy); i++) {
		memcpy(copy, zero_payload, sizeof(copy));
		copy[i] ^= 0x01;
		b = a;
		e = d;
		if (pc88_display_state_decode(&b, &e, &binding, copy, sizeof(copy)) == PC88_STATE_OK)
			CHECK(pc88_display_state_encode(&b, &e, &binding, payload, sizeof(payload)) ==
			              PC88_STATE_OK &&
			          !memcmp(payload, copy, sizeof(copy)),
			      "PC88_STATE_EXHAUSTIVE");
	}
	CHECK(pc88_display_state_decode(&b, &e, &binding, copy, sizeof(copy) - 1) == PC88_STATE_SIZE,
	      "PC88_STATE_SIZE");
	CHECK(pc88_display_state_decode(&b, &e, &binding, copy, sizeof(copy) + 1) == PC88_STATE_SIZE,
	      "PC88_STATE_SIZE");
	CHECK(pc88_display_state_encode(&a, &d, &binding, payload, sizeof(payload) - 1) ==
	          PC88_STATE_SIZE,
	      "PC88_STATE_SIZE");
	CHECK(pc88_display_state_encode(NULL, &d, &binding, payload, sizeof(payload)) ==
	          PC88_STATE_ARGUMENT,
	      "PC88_STATE_ARGUMENT");
	CHECK(pc88_display_state_decode(&b, &e, &binding, NULL, sizeof(copy)) == PC88_STATE_ARGUMENT,
	      "PC88_STATE_ARGUMENT");
	/* Interrupted command continuation and clock phase across round trips. */
	{
		uint64_t crossed = 0;
		upd3301_reset(&b);
		upd3301_clock_configure(&b, binding.clock_hz, binding.hires);
		upd3301_command(&b, 0);
		upd3301_parameter_write(&b, 78);
		upd3301_parameter_write(&b, 40);
		CHECK(upd3301_advance(&b, 1000, &crossed) && crossed == 0, "PC88_STATE_PARTIAL_FIXTURE");
		CHECK(pc88_display_state_encode(&b, &d, &binding, payload, sizeof(payload)) ==
		          PC88_STATE_OK,
		      "PC88_STATE_PARTIAL_FIXTURE");
		b = a;
		e = d;
		CHECK(pc88_display_state_decode(&b, &e, &binding, payload, sizeof(payload)) ==
		              PC88_STATE_OK &&
		          b.command == UPD3301_RESET && b.parameter == 2 && b.reset[1] == 40,
		      "PC88_STATE_PARTIAL_CONTINUATION");
		CHECK(pc88_display_state_decode(&b, &e, &wrong, payload, sizeof(payload)) ==
		          PC88_STATE_BINDING,
		      "PC88_STATE_PARTIAL_BINDING");
	}
	puts(
	    "pc88 display state: 256 commands, 38 flag/clock/dma guards, 32 exhaustive byte mutations PASS");
	return 0;
}
