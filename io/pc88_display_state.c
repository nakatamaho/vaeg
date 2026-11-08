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
#include "pc88_display_state.h"
#include <string.h>

int pc88_display_state_valid(const UPD3301 *c, const PC88_TEXT_DMA *d) {
	if (!c || !d)
		return PC88_STATE_ARGUMENT;
	if (c->cursor_enabled > 1 || c->display_started > 1 || c->dma_started > 1 || c->reverse > 1 ||
	    c->width80 > 1 || c->hires > 1)
		return PC88_STATE_FLAGS;
	switch (c->command) {
	case UPD3301_IDLE:
		if (c->parameter != 0 && c->parameter != 1 && c->parameter != 2 && c->parameter != 5)
			return PC88_STATE_COMMAND;
		break;
	case UPD3301_RESET:
		if (c->parameter > 4 || c->display_started)
			return PC88_STATE_COMMAND;
		break;
	case UPD3301_LIGHT_PEN:
	case UPD3301_CURSOR:
		if (c->parameter > 1)
			return PC88_STATE_COMMAND;
		break;
	default:
		return PC88_STATE_COMMAND;
	}
	if (c->interrupt_mask > 3 || c->interrupt_request > 3)
		return PC88_STATE_INTERRUPT;
	if (c->last_attribute & 8)
		return PC88_STATE_ATTRIBUTE;
	if (!upd3301_clock_valid(c))
		return PC88_STATE_CLOCK;
	if (d->high_next > 1 || c->dma_started != pc88_text_dma_enabled(d))
		return PC88_STATE_DMA;
	return PC88_STATE_OK;
}

int pc88_display_clock_matches(const UPD3301 *c, const PC88_DISPLAY_CLOCK_BINDING *binding) {
	if (!c || !binding)
		return PC88_STATE_ARGUMENT;
	return binding->clock_hz && binding->hires <= 1 && c->clock_hz == binding->clock_hz &&
	               c->hires == binding->hires
	           ? PC88_STATE_OK
	           : PC88_STATE_BINDING;
}

static void put_le(uint8_t *bytes, uint64_t value, unsigned count) {
	unsigned i;
	for (i = 0; i < count; i++) {
		bytes[i] = (uint8_t)value;
		value >>= 8;
	}
}

static uint64_t get_le(const uint8_t *bytes, unsigned count) {
	uint64_t value = 0;
	unsigned i;
	for (i = 0; i < count; i++)
		value |= (uint64_t)bytes[i] << (8 * i);
	return value;
}

int pc88_display_state_encode(const UPD3301 *c, const PC88_TEXT_DMA *d,
                              const PC88_DISPLAY_CLOCK_BINDING *binding, uint8_t *output,
                              size_t size) {
	uint8_t bytes[PC88_DISPLAY_STATE_BYTES];
	int status;
	if (!c || !d || !binding || !output)
		return PC88_STATE_ARGUMENT;
	if (size != sizeof(bytes))
		return PC88_STATE_SIZE;
	status = pc88_display_state_valid(c, d);
	if (status != PC88_STATE_OK)
		return status;
	status = pc88_display_clock_matches(c, binding);
	if (status != PC88_STATE_OK)
		return status;
	bytes[0] = PC88_DISPLAY_STATE_VERSION;
	memcpy(bytes + 1, c->reset, 5);
	bytes[6] = c->command;
	bytes[7] = c->parameter;
	bytes[8] = c->cursor_x;
	bytes[9] = c->cursor_y;
	bytes[10] = (uint8_t)(c->cursor_enabled | (c->display_started << 1) | (c->dma_started << 2) |
	                      (c->reverse << 3) | (c->width80 << 4) | (c->hires << 5));
	bytes[11] = c->interrupt_mask;
	bytes[12] = c->interrupt_request;
	bytes[13] = c->last_attribute;
	put_le(bytes + 14, c->frames, 2);
	put_le(bytes + 16, c->clock_hz, 4);
	put_le(bytes + 20, c->clock_remaining, 8);
	put_le(bytes + 28, d->address, 2);
	bytes[30] = d->high_next;
	bytes[31] = d->mode;
	memcpy(output, bytes, sizeof(bytes));
	return PC88_STATE_OK;
}

int pc88_display_state_decode(UPD3301 *c, PC88_TEXT_DMA *d,
                              const PC88_DISPLAY_CLOCK_BINDING *binding, const uint8_t *input,
                              size_t size) {
	UPD3301 candidate;
	PC88_TEXT_DMA dma;
	int status;
	if (!c || !d || !binding || !input)
		return PC88_STATE_ARGUMENT;
	if (size != PC88_DISPLAY_STATE_BYTES)
		return PC88_STATE_SIZE;
	if (input[0] != PC88_DISPLAY_STATE_VERSION)
		return PC88_STATE_VERSION;
	if (input[10] & 0xc0)
		return PC88_STATE_FLAGS;
	memset(&candidate, 0, sizeof(candidate));
	memset(&dma, 0, sizeof(dma));
	memcpy(candidate.reset, input + 1, 5);
	candidate.command = input[6];
	candidate.parameter = input[7];
	candidate.cursor_x = input[8];
	candidate.cursor_y = input[9];
	candidate.cursor_enabled = input[10] & 1;
	candidate.display_started = (input[10] >> 1) & 1;
	candidate.dma_started = (input[10] >> 2) & 1;
	candidate.reverse = (input[10] >> 3) & 1;
	candidate.width80 = (input[10] >> 4) & 1;
	candidate.hires = (input[10] >> 5) & 1;
	candidate.interrupt_mask = input[11];
	candidate.interrupt_request = input[12];
	candidate.last_attribute = input[13];
	candidate.frames = (uint16_t)get_le(input + 14, 2);
	candidate.clock_hz = (uint32_t)get_le(input + 16, 4);
	candidate.clock_remaining = get_le(input + 20, 8);
	dma.address = (uint16_t)get_le(input + 28, 2);
	dma.high_next = input[30];
	dma.mode = input[31];
	status = pc88_display_state_valid(&candidate, &dma);
	if (status != PC88_STATE_OK)
		return status;
	status = pc88_display_clock_matches(&candidate, binding);
	if (status != PC88_STATE_OK)
		return status;
	*c = candidate;
	*d = dma;
	return PC88_STATE_OK;
}
