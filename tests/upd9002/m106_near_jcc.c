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

/* M106: the 80386 near conditional jump 0F 8x halts the uPD9002. */

#include "compiler.h"
#include "cpucore.h"
#include "upd9002_ops.h"
#include "memoryva.h"
#include "tests/upd9002/m106_near_jcc.h"

#include <stdio.h>

enum {
	M106_CS = 0x1000,
	M106_IP = 0x0100
};

static void setup_program(const UINT8 *bytes, UINT size) {
	UINT i;

	upd9002_core_reset();
	ZeroMemory(mem, 0x100000);
	CPU_AX = 0x0000;
	CPU_SP = 0x8000;
	CPU_ES = 0x2000;
	CPU_CS = M106_CS;
	CPU_SS = 0x2000;
	CPU_DS = 0x2000;
	CPU_IP = M106_IP;
	CPU_FLAG = 0xf002;
	ES_BASE = (UINT32)CPU_ES << 4;
	CS_BASE = (UINT32)CPU_CS << 4;
	SS_BASE = (UINT32)CPU_SS << 4;
	DS_BASE = (UINT32)CPU_DS << 4;
	upd9002_core_context.s.ss_fix = SS_BASE;
	upd9002_core_context.s.ds_fix = DS_BASE;
	CPU_ADRSMASK = 0x000fffff;
	CPU_REMCLOCK = 5000;
	CPU_BASECLOCK = 10000;
	CPU_CLOCK = 0;
	CPU_ITFBANK = 0;
	upd9002_core_context.s.cpu_type = CPUTYPE_V30;
	for (i = 0; i < size; i++) {
		mem[CS_BASE + M106_IP + i] = bytes[i];
	}
}

/* Each 0F 80h-8Fh stops on its 0Fh byte, ends the time slice, and stays
 * stopped when stepped again; nothing after it runs. */
static int test_near_jcc_halts(void) {
	UINT second;

	for (second = 0x80; second <= 0x8f; second++) {
		/* 0F 8x 00 00; mov ax,1234h */
		const UINT8 program[] = {0x0f, (UINT8)second, 0x00, 0x00, 0xb8, 0x34, 0x12};

		setup_program(program, sizeof(program));
		upd9002_core_step();
		if ((CPU_IP != M106_IP) || (CPU_CS != M106_CS) || (CPU_REMCLOCK >= 0) ||
		    (CPU_AX != 0x0000)) {
			fprintf(stderr,
			        "upd9002-m106-near-jcc: 0F %02X did not halt (ip=%04x clock=%d ax=%04x)\n",
			        second, CPU_IP, CPU_REMCLOCK, CPU_AX);
			return FAILURE;
		}
		CPU_REMCLOCK = 5000;
		upd9002_core_step();
		if ((CPU_IP != M106_IP) || (CPU_AX != 0x0000)) {
			fprintf(stderr, "upd9002-m106-near-jcc: 0F %02X resumed (ip=%04x ax=%04x)\n", second,
			        CPU_IP, CPU_AX);
			return FAILURE;
		}
	}
	return SUCCESS;
}

/* A reserved 0F with a second byte outside 80h-8Fh keeps its one-byte
 * reserved behaviour. */
static int test_other_reserved_unchanged(void) {
	const UINT8 program[] = {0x0f, 0x90, 0x90};

	setup_program(program, sizeof(program));
	upd9002_core_step();
	if ((CPU_IP != M106_IP + 1) || (CPU_REMCLOCK < 0)) {
		fprintf(stderr, "upd9002-m106-near-jcc: 0F 90 changed (ip=%04x clock=%d)\n", CPU_IP,
		        CPU_REMCLOCK);
		return FAILURE;
	}
	return SUCCESS;
}

int upd9002_m106_near_jcc_main(void) {
	int result;

	upd9002_core_initialize();
	result = test_near_jcc_halts();
	if (result == SUCCESS) {
		result = test_other_reserved_unchanged();
	}
	upd9002_core_deinitialize();
	if (result == SUCCESS) {
		puts("upd9002-m106-near-jcc: 0F 80h-8Fh halt check passed");
	}
	return result;
}
