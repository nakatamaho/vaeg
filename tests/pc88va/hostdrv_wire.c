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
#include "io/hostdrv_wire.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void require(int predicate, const char *code) {
	if (!predicate) {
		fprintf(stderr, "%s\n", code);
		exit(1);
	}
}

static void request(uint8_t p[HD_PACKET], unsigned op) {
	memset(p, 0, HD_PACKET);
	memcpy(p, "HDR1", 4);
	p[HD_VERSION] = 1;
	p[HD_OPCODE] = (uint8_t)op;
	hostdrv_put16(p + HD_SEQUENCE, 0xf123);
	if (op != HD_QUERY) {
		hostdrv_put32(p + HD_GENERATION, 0x12345678);
	}
}

static void test_requests(void) {
	uint8_t good[HD_PACKET];
	uint8_t mutant[HD_PACKET];
	unsigned op;

	for (op = HD_QUERY; op <= HD_UNREGISTER; ++op) {
		request(good, op);
		switch (op) {
		case HD_OPEN:
			hostdrv_put16(good + HD_OPTIONS, 0x40);
			hostdrv_put16(good + HD_PATH, 1);
			hostdrv_put16(good + HD_INPUT, 1);
			good[HD_HEADER] = 'A';
			break;
		case HD_FIND_FIRST:
			hostdrv_put16(good + HD_OPTIONS, 0x3f);
			hostdrv_put16(good + HD_INPUT, 11);
			hostdrv_put16(good + HD_CAPACITY, 11);
			memset(good + HD_HEADER, '?', 11);
			break;
		case HD_SPACE:
			hostdrv_put16(good + HD_CAPACITY, 16);
			break;
		case HD_READ:
			hostdrv_put16(good + HD_COUNT, 4096);
			hostdrv_put16(good + HD_CAPACITY, 4096);
			hostdrv_put32(good + HD_OFFSET, 0x7fffffff);
			hostdrv_put32(good + HD_TOKEN, 1);
			break;
		case HD_FIND_NEXT:
			hostdrv_put32(good + HD_OFFSET, 4096);
			hostdrv_put16(good + HD_CAPACITY, 11);
			hostdrv_put32(good + HD_TOKEN, 2);
			break;
		case HD_CLOSE:
		case HD_FSTAT:
		case HD_FIND_RELEASE:
			hostdrv_put32(good + HD_TOKEN, 1);
			break;
		default:
			break;
		}
		require(hostdrv_request_valid(good), "HD_VALID_REQUEST_REJECTED");
		memcpy(mutant, good, sizeof(mutant));
		mutant[63] = 1;
		require(!hostdrv_request_valid(mutant), "HD_RESERVED_ACCEPTED");
		memcpy(mutant, good, sizeof(mutant));
		mutant[HD_INPUT] ^= 1;
		require(!hostdrv_request_valid(mutant), "HD_INPUT_LENGTH_ACCEPTED");
		memcpy(mutant, good, sizeof(mutant));
		mutant[HD_CAPACITY] ^= 1;
		require(!hostdrv_request_valid(mutant), "HD_CAPACITY_ACCEPTED");
		if (op == HD_OPEN || op == HD_FIND_FIRST) {
			memcpy(mutant, good, sizeof(mutant));
			mutant[HD_OPTIONS] ^= 0x80;
			require(!hostdrv_request_valid(mutant), "HD_OPTIONS_ACCEPTED");
		}
	}
	request(good, HD_DETACH);
	require(hostdrv_request_valid(good), "HD_DETACH_FIXTURE_INVALID");
	for (op = HD_UNREGISTER + 1; op <= UINT8_MAX; ++op) {
		memcpy(mutant, good, sizeof(mutant));
		mutant[HD_OPCODE] = (uint8_t)op;
		require(!hostdrv_request_valid(mutant), "HD_UNASSIGNED_OPCODE_ACCEPTED");
	}
}

static void test_responses(void) {
	uint8_t req[HD_PACKET];
	uint8_t reply[HD_PACKET];
	uint8_t mutant[HD_PACKET];
	const uint32_t port = HD_REPLY_BASE | HD_COMPLETED;

	request(req, HD_READ);
	hostdrv_put32(req + HD_TOKEN, 0x12345678);
	hostdrv_put16(req + HD_COUNT, 8);
	hostdrv_put16(req + HD_CAPACITY, 8);
	hostdrv_response_init(reply, req, 0);
	require(hostdrv_response_valid(port, req, reply), "HD_VALID_EOF_REJECTED");
	require(!hostdrv_response_valid(port, req, req), "HD_STALE_SUCCESS_ACCEPTED");
	require(!hostdrv_response_valid(HD_REPLY_BASE | HD_BUSY, req, reply), "HD_BUSY_ACCEPTED");
	memcpy(mutant, reply, sizeof(mutant));
	mutant[3] = '2';
	require(!hostdrv_response_valid(port, req, mutant), "HD_WRONG_MARKER_ACCEPTED");
	memcpy(mutant, reply, sizeof(mutant));
	mutant[HD_SEQUENCE] ^= 1;
	require(!hostdrv_response_valid(port, req, mutant), "HD_WRONG_SEQUENCE_ACCEPTED");
	memcpy(mutant, reply, sizeof(mutant));
	mutant[HD_GENERATION] ^= 1;
	require(!hostdrv_response_valid(port, req, mutant), "HD_WRONG_GENERATION_ACCEPTED");
	memcpy(mutant, reply, sizeof(mutant));
	mutant[HD_OUTPUT] = 1;
	require(!hostdrv_response_valid(port, req, mutant), "HD_INCONSISTENT_COUNT_ACCEPTED");

	hostdrv_response_init(reply, req, 31);
	require(hostdrv_response_valid(port, req, reply), "HD_VALID_READ_ERROR_REJECTED");
	memcpy(mutant, reply, sizeof(mutant));
	mutant[HD_ACTUAL] = 1;
	require(!hostdrv_response_valid(port, req, mutant), "HD_ERROR_OUTPUT_ACCEPTED");

	request(req, HD_OPEN);
	hostdrv_put16(req + HD_PATH, 1);
	hostdrv_put16(req + HD_INPUT, 1);
	req[HD_HEADER] = 'A';
	hostdrv_response_init(reply, req, 0);
	hostdrv_put32(reply + HD_TOKEN, 42);
	hostdrv_put16(reply + HD_SERIAL, 1);
	require(hostdrv_response_valid(port, req, reply), "HD_VALID_OPEN_REJECTED");
	memcpy(mutant, reply, sizeof(mutant));
	mutant[HD_SERIAL] = 0;
	require(!hostdrv_response_valid(port, req, mutant), "HD_ZERO_SERIAL_ACCEPTED");
}

struct exchange_fixture {
	int busy;
	unsigned calls;
	uint8_t ram[HD_PACKET];
};

static unsigned dispatch_query(void *context, const uint8_t req[HD_PACKET],
                               uint8_t reply[HD_PACKET]) {
	struct exchange_fixture *f = context;
	const uint32_t nested = hostdrv_exchange(&f->busy, f->ram, dispatch_query, f);
	f->calls++;
	require(nested == (HD_REPLY_BASE | HD_BUSY), "HD_NESTED_EXCHANGE_NOT_BUSY");
	require(req != f->ram && memcmp(req, f->ram, HD_PACKET) == 0,
	        "HD_REQUEST_NOT_STAGED");
	hostdrv_response_init(reply, req, 0);
	hostdrv_put16(reply + HD_CAPS, 1);
	hostdrv_put32(reply + HD_GENERATION, 42);
	require(memcmp(req, f->ram, HD_PACKET) == 0, "HD_PREMATURE_PUBLICATION");
	return HD_COMPLETED;
}

static void test_exchange(void) {
	struct exchange_fixture f = {0};
	uint8_t req[HD_PACKET];
	uint32_t port;

	request(req, HD_QUERY);
	memcpy(f.ram, req, sizeof(req));
	port = hostdrv_exchange(&f.busy, f.ram, dispatch_query, &f);
	require(hostdrv_response_valid(port, req, f.ram) && f.calls == 1 && f.busy == 0,
	        "HD_EXCHANGE_NOT_COMPLETED");
	memcpy(f.ram, req, sizeof(req));
	port = hostdrv_exchange(&f.busy, f.ram, NULL, &f);
	require(port == (HD_REPLY_BASE | HD_DISABLED) && f.calls == 1 &&
	            memcmp(f.ram, req, HD_PACKET) == 0,
	        "HD_DISABLED_EXCHANGE_SIDE_EFFECT");
	port = hostdrv_exchange(&f.busy, NULL, dispatch_query, &f);
	require(port == (HD_REPLY_BASE | HD_BAD_RANGE) && f.calls == 1 &&
	            memcmp(f.ram, req, HD_PACKET) == 0,
	        "HD_INVALID_RANGE_SIDE_EFFECT");
	req[HD_VERSION] ^= 1;
	memcpy(f.ram, req, sizeof(req));
	port = hostdrv_exchange(&f.busy, f.ram, dispatch_query, &f);
	require(port == (HD_REPLY_BASE | HD_BAD_PACKET) && f.calls == 1 && f.busy == 0 &&
	            memcmp(f.ram, req, HD_PACKET) == 0,
	        "HD_MALFORMED_EXCHANGE_SIDE_EFFECT");
}

static void test_va_range(void) {
	uint32_t address = 0;
	const uint32_t limit = UINT32_C(0xa0000);

	require(hostdrv_bounce_address(UINT32_C(0x2000efc0), limit, &address) &&
	            address == UINT32_C(0x2efc0),
	        "HD_EXACT_SEGMENT_END_REJECTED");
	require(!hostdrv_bounce_address(UINT32_C(0x2000efc1), limit, &address),
	        "HD_SEGMENT_WRAP_ACCEPTED");
	require(hostdrv_bounce_address(UINT32_C(0x7efc0000), limit, &address) &&
	            address == UINT32_C(0x7efc0),
	        "HD_EXACT_RAM_END_REJECTED");
	require(!hostdrv_bounce_address(UINT32_C(0x7efc0001), limit, &address),
	        "HD_BMS_CROSSING_ACCEPTED");
	require(!hostdrv_bounce_address(UINT32_C(0x7efc0000), 0x7ffffU, &address),
	        "HD_UNINSTALLED_RAM_ACCEPTED");
	require(hostdrv_bounce_address(UINT32_C(0x10000000), limit, &address) &&
	            address == UINT32_C(0x10000),
	        "HD_EXACT_RAM_START_REJECTED");
	require(!hostdrv_bounce_address(UINT32_C(0x0fff000f), limit, &address),
	        "HD_LOW_WORK_RAM_ACCEPTED");
	require(!hostdrv_bounce_address(UINT32_C(0xffff0010), limit, &address),
	        "HD_A20_ALIAS_ACCEPTED");
}

int main(void) {
	test_requests();
	test_responses();
	test_exchange();
	test_va_range();
	return 0;
}
