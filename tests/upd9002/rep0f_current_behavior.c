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

#include "compiler.h"
#include "cpucore.h"
#include "v30patch.h"
#include "tests/upd9002/rep0f_current_behavior.h"

#include <stdio.h>
#include <stdlib.h>


typedef struct {
	const char *case_id;
	UINT8 bytes[4];
	UINT size;
	UINT16 expected_ip;
	UINT16 expected_msw;
} REP0F_CURRENT_CASE;


static const REP0F_CURRENT_CASE current_cases[] = {
	{"0f01f0", {0x0f, 0x01, 0xf0, 0x90}, 3, 0x2002, 0x0000},
	{"f20f01f0", {0xf2, 0x0f, 0x01, 0xf0}, 4, 0x2004, 0x0001},
	{"f30f01f0", {0xf3, 0x0f, 0x01, 0xf0}, 4, 0x2004, 0x0001}
};


static int run_case(const REP0F_CURRENT_CASE *test_case) {

	ZeroMemory(&CPU_STATSAVE, sizeof(CPU_STATSAVE));
	ZeroMemory(mem, 0x100000);
	i286core.s.cpu_type = CPUTYPE_V30;
	CPU_AX = 0x0001;
	CPU_CS = 0x0000;
	CS_BASE = 0x00000000;
	CPU_IP = 0x2000;
	CPU_FLAG = 0xf002;
	CPU_ADRSMASK = 0x000fffff;
	CPU_REMCLOCK = 1000;
	CPU_BASECLOCK = 1000;
	CopyMemory(mem + 0x2000, test_case->bytes, test_case->size);

	v30c_step();
	fprintf(stderr,
		"upd9002-rep0f-current: case=%s ip=%04x msw=%04x\n",
		test_case->case_id, CPU_IP, CPU_MSW);
	if ((CPU_IP != test_case->expected_ip) ||
		(CPU_MSW != test_case->expected_msw)) {
		return(FAILURE);
	}
	return(SUCCESS);
}


int upd9002_rep0f_current_behavior_main(void) {

	UINT index;

	i286c_initialize();
	for (index = 0; index < NELEMENTS(current_cases); index++) {
		if (run_case(&current_cases[index]) != SUCCESS) {
			fprintf(stderr,
				"upd9002-rep0f-current: accepted behavior changed\n");
			i286c_deinitialize();
			return(EXIT_FAILURE);
		}
	}
	i286c_deinitialize();
	fprintf(stderr,
		"upd9002-rep0f-current: cases=3 current-behavior-only pass\n");
	return(EXIT_SUCCESS);
}
