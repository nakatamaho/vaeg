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
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "compiler.h"
#include "tests/upd9002/semantics_bundle.h"

#include <stdio.h>

typedef struct {
	UINT8 source;
	UINT8 accumulator;
	UINT8 expected_accumulator;
	UINT8 expected_destination;
} PACKED_BCD_CASE;

static int test_evidence_oracles(void) {

	static const PACKED_BCD_CASE ror4_cases[] = {
		{0x12, 0x34, 0x12, 0x41},
		{0xab, 0xcd, 0xab, 0xda},
		{0xf0, 0x0f, 0xf0, 0xff}
	};
	static const PACKED_BCD_CASE rol4_cases[] = {
		{0x12, 0x34, 0x12, 0x24},
		{0xab, 0xcd, 0xab, 0xbd},
		{0xf0, 0x0f, 0xf0, 0x0f}
	};
	UINT index;

	for (index = 0; index < NELEMENTS(ror4_cases); index++) {
		const PACKED_BCD_CASE *const value = &ror4_cases[index];
		if ((value->expected_accumulator != value->source) ||
			(value->expected_destination !=
			 (UINT8)((value->source >> 4) |
					 ((value->accumulator & 0x0f) << 4)))) {
			fprintf(stderr, "upd9002-m62: ROR4 evidence oracle differs\n");
			return FAILURE;
		}
	}
	for (index = 0; index < NELEMENTS(rol4_cases); index++) {
		const PACKED_BCD_CASE *const value = &rol4_cases[index];
		if ((value->expected_accumulator != value->source) ||
			(value->expected_destination !=
			 (UINT8)((value->source << 4) |
					 (value->accumulator & 0x0f)))) {
			fprintf(stderr, "upd9002-m62: ROL4 evidence oracle differs\n");
			return FAILURE;
		}
	}
	return SUCCESS;
}

int upd9002_semantics_bundle_main(void) {

	if (test_evidence_oracles() != SUCCESS) {
		return FAILURE;
	}
	puts("upd9002-m62-semantics-bundle: audit infrastructure passed");
	return SUCCESS;
}
