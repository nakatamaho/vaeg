/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "hostdrv_wire.h"

#include <string.h>

uint16_t hostdrv_get16(const uint8_t *p) {
	return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

uint32_t hostdrv_get32(const uint8_t *p) {
	return hostdrv_get16(p) | ((uint32_t)hostdrv_get16(p + 2) << 16);
}

void hostdrv_put16(uint8_t *p, uint16_t value) {
	p[0] = (uint8_t)value;
	p[1] = (uint8_t)(value >> 8);
}

void hostdrv_put32(uint8_t *p, uint32_t value) {
	hostdrv_put16(p, (uint16_t)value);
	hostdrv_put16(p + 2, (uint16_t)(value >> 16));
}

static int zero_bytes(const uint8_t *p, unsigned count) {
	while (count--) {
		if (*p++) {
			return 0;
		}
	}
	return 1;
}

int hostdrv_request_valid(const uint8_t p[HD_PACKET]) {
	const unsigned op = p[HD_OPCODE];
	const unsigned count = hostdrv_get16(p + HD_COUNT);
	const unsigned path = hostdrv_get16(p + HD_PATH);
	const unsigned input = hostdrv_get16(p + HD_INPUT);
	const unsigned capacity = hostdrv_get16(p + HD_CAPACITY);
	const unsigned options = hostdrv_get16(p + HD_OPTIONS);
	const uint32_t token = hostdrv_get32(p + HD_TOKEN);
	const uint32_t offset = hostdrv_get32(p + HD_OFFSET);
	unsigned expected_input = 0;
	unsigned expected_capacity = 0;

	if (memcmp(p, "HDR1", 4) || p[HD_VERSION] != 1 || op > HD_UNREGISTER ||
	    ((hostdrv_get32(p + HD_GENERATION) == 0) != (op == HD_QUERY)) ||
	    hostdrv_get16(p + HD_PATH2) || !zero_bytes(p + HD_STATUS, HD_HEADER - HD_STATUS) ||
	    path > 127 || (path && op != HD_STAT && op != HD_OPEN && op != HD_FIND_FIRST) ||
	    (count && op != HD_READ) || (offset && op != HD_READ && op != HD_FIND_NEXT) ||
	    (options && op != HD_OPEN && op != HD_FIND_FIRST)) {
		return 0;
	}
	if ((token != 0) != (op == HD_READ || op == HD_CLOSE || op == HD_FSTAT ||
	                     op == HD_FIND_NEXT || op == HD_FIND_RELEASE)) {
		return 0;
	}
	switch (op) {
	case HD_STAT:
		expected_input = path;
		break;
	case HD_SPACE:
		expected_capacity = 16;
		break;
	case HD_OPEN:
		if (!path || (options & ~0x77U) || (options & 7) > 2 || (options >> 4) > 4) {
			return 0;
		}
		expected_input = path;
		break;
	case HD_READ:
		if (!count || count > HD_DATA || offset > UINT32_C(0x7fffffff)) {
			return 0;
		}
		expected_capacity = count;
		break;
	case HD_FIND_FIRST:
		if (options & ~0x3fU) {
			return 0;
		}
		expected_input = path + 11;
		expected_capacity = 11;
		break;
	case HD_FIND_NEXT:
		if (!offset || offset > 4096) {
			return 0;
		}
		expected_capacity = 11;
		break;
	default:
		break;
	}
	return input == expected_input && capacity == expected_capacity;
}

void hostdrv_response_init(uint8_t response[HD_PACKET], const uint8_t request[HD_PACKET],
                          uint16_t dos_status) {
	memset(response, 0, HD_PACKET);
	memcpy(response, request, HD_STATUS);
	memcpy(response, "HDS1", 4);
	hostdrv_put16(response + HD_STATUS, dos_status);
}

int hostdrv_response_valid(uint32_t port, const uint8_t request[HD_PACKET],
                           const uint8_t p[HD_PACKET]) {
	uint8_t expected[HD_HEADER];
	unsigned op;
	unsigned output;
	unsigned actual;
	unsigned expected_output = 0;
	unsigned status;

	if (port != (HD_REPLY_BASE | HD_COMPLETED) || !hostdrv_request_valid(request)) {
		return 0;
	}
	op = request[HD_OPCODE];
	status = hostdrv_get16(p + HD_STATUS);
	output = hostdrv_get16(p + HD_OUTPUT);
	actual = hostdrv_get16(p + HD_ACTUAL);
	memcpy(expected, request, HD_HEADER);
	memcpy(expected, "HDS1", 4);
	hostdrv_put16(expected + HD_STATUS, (uint16_t)status);
	if (status) {
		switch (status) {
		case 1: case 2: case 3: case 4: case 5: case 6: case 12: case 17:
		case 18: case 21: case 31: case 32: case 33: case 39: case 80:
			return memcmp(expected, p, HD_HEADER) == 0;
		default:
			return 0;
		}
	}
	if (op == HD_QUERY) {
		if (!hostdrv_get32(p + HD_GENERATION) || hostdrv_get16(p + HD_CAPS) != 1) {
			return 0;
		}
		memcpy(expected + HD_GENERATION, p + HD_GENERATION, 4);
		hostdrv_put16(expected + HD_CAPS, 1);
	}
	if (op == HD_OPEN || op == HD_FIND_FIRST) {
		if (!hostdrv_get32(p + HD_TOKEN)) {
			return 0;
		}
		memcpy(expected + HD_TOKEN, p + HD_TOKEN, 4);
	}
	if (op == HD_OPEN) {
		if (!hostdrv_get16(p + HD_SERIAL)) {
			return 0;
		}
		memcpy(expected + HD_SERIAL, p + HD_SERIAL, 2);
	}
	if (op == HD_OPEN || op == HD_STAT || op == HD_FSTAT ||
	    op == HD_FIND_FIRST || op == HD_FIND_NEXT) {
		const unsigned attr = hostdrv_get16(p + HD_ATTR);
		const uint32_t size = hostdrv_get32(p + HD_SIZE);
		if ((attr & ~0x37U) || size > UINT32_C(0x7fffffff) ||
		    ((attr & 0x10) && (size || op == HD_OPEN || op == HD_FSTAT)) ||
		    (op == HD_STAT && !hostdrv_get16(request + HD_PATH) && !(attr & 0x10))) {
			return 0;
		}
		memcpy(expected + HD_SIZE, p + HD_SIZE, HD_SERIAL - HD_SIZE);
	}
	switch (op) {
	case HD_READ:
		expected_output = actual;
		if (actual > hostdrv_get16(request + HD_COUNT)) {
			return 0;
		}
		hostdrv_put16(expected + HD_ACTUAL, (uint16_t)actual);
		break;
	case HD_SPACE:
		expected_output = 16;
		if (!zero_bytes(p + HD_HEADER + 8, 8)) {
			return 0;
		}
		break;
	case HD_FIND_FIRST:
	case HD_FIND_NEXT:
		expected_output = 11;
		break;
	default:
		break;
	}
	hostdrv_put16(expected + HD_OUTPUT, (uint16_t)expected_output);
	return output == expected_output && memcmp(expected, p, HD_HEADER) == 0;
}

int hostdrv_bounce_address(uint32_t far_pointer, uint32_t mainram_limit, uint32_t *address) {
	const uint32_t offset = far_pointer & UINT32_C(0xffff);
	const uint32_t start = ((far_pointer >> 16) << 4) + offset;
	/* Below 10000h is conservatively reserved for vectors and VA work RAM.
	 * The native byte-map switches to BMS at 80000h, even if bank zero
	 * presently exposes RAM. Never translate or mask the linear address.
	 */
	const uint32_t end = mainram_limit < 0x80000U ? mainram_limit : 0x80000U;

	if (offset > 0x10000U - HD_PACKET || start < 0x10000U || start >= end ||
	    HD_PACKET > end - start) {
		return 0;
	}
	*address = start;
	return 1;
}

uint32_t hostdrv_exchange(int *busy, uint8_t *ram, hostdrv_dispatch dispatch, void *context) {
	uint8_t request[HD_PACKET];
	uint8_t reply[HD_PACKET];
	unsigned result;

	if (dispatch == NULL) {
		return HD_REPLY_BASE | HD_DISABLED;
	}
	if (*busy) {
		return HD_REPLY_BASE | HD_BUSY;
	}
	if (ram == NULL) {
		return HD_REPLY_BASE | HD_BAD_RANGE;
	}
	*busy = 1;
	memcpy(request, ram, HD_PACKET);
	if (hostdrv_request_valid(request)) {
		hostdrv_response_init(reply, request, 31);
		result = dispatch(context, request, reply);
		if (result == HD_COMPLETED) {
			memcpy(ram, reply, HD_PACKET);
		}
	} else {
		result = HD_BAD_PACKET;
	}
	*busy = 0;
	return HD_REPLY_BASE | result;
}
