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
#include "cpucore.h"
#include "tests/upd9002/semantics_bundle.h"

#include <stdio.h>

typedef struct {
	UINT8 source;
	UINT8 accumulator;
	UINT8 expected_accumulator;
	UINT8 expected_destination;
} PACKED_BCD_CASE;

typedef struct {
	UINT8 radix;
	UINT8 initial_al;
	UINT8 expected_ah;
	UINT8 expected_al;
} AAM_CASE;

static UINT16 aam_expected_flags(UINT16 initial_flags, UINT8 value) {

	UINT bits = value;
	UINT parity = 0;
	UINT16 flags = (UINT16)(initial_flags & 0xf700);

	while (bits) {
		parity ^= bits & 1;
		bits >>= 1;
	}
	flags |= 0x0002;
	if (!parity) {
		flags |= 0x0004;
	}
	if (!value) {
		flags |= 0x0040;
	}
	if (value & 0x80) {
		flags |= 0x0080;
	}
	return flags;
}

static void setup_instruction(const UINT8 *instruction, UINT length,
							UINT16 ax, UINT16 flags) {

	UINT index;

	upd9002_core_reset();
	ZeroMemory(mem, 0x100000);
	CPU_AX = ax;
	CPU_CX = 0x2468;
	CPU_DX = 0x369c;
	CPU_BX = 0x48ad;
	CPU_SP = 0x8000;
	CPU_BP = 0x7bcd;
	CPU_SI = 0x55aa;
	CPU_DI = 0xaa55;
	CPU_ES = 0x1111;
	CPU_CS = 0x2000;
	CPU_SS = 0x3000;
	CPU_DS = 0x3333;
	CPU_IP = 0x0100;
	CPU_FLAG = flags;
	ES_BASE = (UINT32)CPU_ES << 4;
	CS_BASE = (UINT32)CPU_CS << 4;
	SS_BASE = (UINT32)CPU_SS << 4;
	DS_BASE = (UINT32)CPU_DS << 4;
	i286core.s.ss_fix = SS_BASE;
	i286core.s.ds_fix = DS_BASE;
	CPU_ADRSMASK = 0x000fffff;
	CPU_REMCLOCK = 1000;
	CPU_BASECLOCK = 1000;
	CPU_CLOCK = 0;
	i286core.s.cpu_type = CPUTYPE_V30;
	for (index = 0; index < length; index++) {
		mem[(CS_BASE + CPU_IP + index) & CPU_ADRSMASK] =
														instruction[index];
	}
}

static int test_aam_semantics(void) {

	static const AAM_CASE cases[] = {
		{0, 0x85, 0xff, 0x85},
		{1, 0xff, 0xff, 0x00},
		{2, 0xff, 0x7f, 0x01},
		{9, 0x5a, 0x0a, 0x00},
		{10, 0x63, 0x09, 0x09},
		{11, 0x79, 0x0b, 0x00},
		{16, 0xab, 0x0a, 0x0b},
		{255, 0xfe, 0x00, 0xfe}
	};
	const UINT16 initial_flags = 0xfc93;
	UINT index;

	for (index = 0; index < NELEMENTS(cases); index++) {
		const AAM_CASE *const value = &cases[index];
		const UINT8 instruction[] = {0xd4, value->radix};
		const UINT16 expected_ax =
					(UINT16)((value->expected_ah << 8) | value->expected_al);

		setup_instruction(instruction, NELEMENTS(instruction),
							(UINT16)(0x5a00 | value->initial_al),
							initial_flags);
		upd9002_core_step();
		if (CPU_AX != expected_ax) {
			fprintf(stderr,
				"upd9002-m62: AAM radix %u produced AX=%04x, expected %04x\n",
				value->radix, CPU_AX, expected_ax);
			return FAILURE;
		}
		if (CPU_FLAG != aam_expected_flags(initial_flags,
											value->expected_al)) {
			fprintf(stderr,
				"upd9002-m62: AAM radix %u produced FLAGS=%04x\n",
				value->radix, CPU_FLAG);
			return FAILURE;
		}
		if ((CPU_IP != 0x0102) || (CPU_CX != 0x2468) ||
			(CPU_DX != 0x369c) || (CPU_BX != 0x48ad) ||
			(CPU_SP != 0x8000) || (CPU_BP != 0x7bcd) ||
			(CPU_SI != 0x55aa) || (CPU_DI != 0xaa55)) {
			fprintf(stderr,
				"upd9002-m62: AAM changed IP or an unrelated register\n");
			return FAILURE;
		}
	}
	return SUCCESS;
}

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

	upd9002_core_initialize();
	if ((test_evidence_oracles() != SUCCESS) ||
		(test_aam_semantics() != SUCCESS)) {
		upd9002_core_deinitialize();
		return FAILURE;
	}
	upd9002_core_deinitialize();
	puts("upd9002-m62-semantics-bundle: audit infrastructure passed");
	return SUCCESS;
}
