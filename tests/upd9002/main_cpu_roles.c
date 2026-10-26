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
#include "machine/pccore.h"
#include "cpucore.h"
#include "cpu/upd9002_upd70008.h"
#include "memoryva/memoryva.h"
#include "io/sysportva.h"

/* Independent instruction fixture, never copied from private guest software. */
int upd780_main_role_selftest(void) {
	static const UINT8 code[] = {
	    0xcb, 0x47, /* BIT 0,A: H distinguishes the main CPUs */
	    0x00,       /* NOP: four uPD780 T states, not VA measured cost */
	    0x00, 0x00, /* NOP at S, then S with CPU speed 50% */
	    0xed, 0x20, /* undefined uPD780 instruction, not VA native-vector read */
	    0xed, 0xed, /* not CALLN on the uPD780 */
	    0xed, 0xfd  /* not RETEM on the uPD780 */
	};
	static const UINT8 roles[][2] = {{0, 0}, {1, 0}, {0, 1}, {0, 0}, {1, 0}};
	const BOOL saved_pc8801 = memoryva_pc8801;
	const BOOL saved_pc8001 = memoryva_pc8001;
	const UINT8 saved_mode = memoryva_88_mode;
	const UINT8 saved_port_a = sysportva.a;
	const UINT saved_multiple = np2cfg.multiple;
	const UINT8 saved_speed = np2cfg.cpu_speed;
	UINT i;
	int result = SUCCESS;

	np2cfg.multiple = PCCORE_STANDARD_MULTIPLE;
	np2cfg.cpu_speed = 100;
	pccore_clockrestore();
	memoryva_88_mode = 0;
	for (i = 0; i < NELEMENTS(roles); i++) {
		SINT32 before;
		const BOOL standalone = roles[i][0] || roles[i][1];
		memoryva_pc8801 = roles[i][0];
		memoryva_pc8001 = roles[i][1];
		upd9002_core_initialize();
		upd9002_upd70008_register();
		upd9002_core_reset();
		ZeroMemory(mem, 0x40000);
		STOREINTELWORD(mem + 0xe1 * 4, 0x1000);
		STOREINTELWORD(mem + 0xe1 * 4 + 2, 0x2000);
		mem[0x20100] = 0x0f;
		mem[0x20101] = 0xff;
		mem[0x20102] = 0xe1;
		CopyMemory(mem + 0x21000, code, sizeof(code));
		CPU_CS = CPU_DS = 0x2000;
		CS_BASE = DS_BASE = 0x20000;
		CPU_SS = 0x3000;
		SS_BASE = 0x30000;
		CPU_SP = 0x100;
		CPU_BP = 0x200;
		CPU_IP = 0x100;
		CPU_AL = 1;
		CPU_BX = 0xbeef;
		CPU_FLAG = 0xf000;
		CPU_REMCLOCK = CPU_BASECLOCK = 100000;
		upd9002_core_step(); /* board bootstrap hands off to selected CPU */
		upd9002_core_step(); /* BIT */
		if ((pccore_main_cpu_is_upd780() != standalone) ||
		    ((CPU_FLAG & 0x10) != (standalone ? 0x10 : 0))) {
			fprintf(stderr, "M139_MAIN_CPU_BIT_PROFILE role=%u flags=%04x\n", i, CPU_FLAG);
			result = FAILURE;
			break;
		}
		before = CPU_REMCLOCK;
		upd9002_core_step(); /* NOP */
		if (standalone && before - CPU_REMCLOCK != 4) {
			fprintf(stderr, "M139_MAIN_CPU_TSTATES role=%u elapsed=%d\n", i,
			        (int)(before - CPU_REMCLOCK));
			result = FAILURE;
			break;
		}
		if (standalone) {
			UINT op;
			memoryva_88_mode = 1;
			sysportva.a &= (UINT8)~0x20;
			for (op = 0; op < 2; op++) {
				pccore_set_cpu_speed(op ? 50 : 100);
				before = CPU_REMCLOCK;
				upd9002_core_step();
				if (before - CPU_REMCLOCK != (op ? 16 : 8)) {
					fprintf(stderr, "M139_MAIN_CPU_CLOCK_SCALE role=%u case=%u\n", i, op);
					result = FAILURE;
					break;
				}
			}
			memoryva_88_mode = 0;
			sysportva.a = saved_port_a;
			pccore_set_cpu_speed(100);
			if (result != SUCCESS) {
				break;
			}
			for (op = 0; op < 3; op++) {
				upd9002_core_step();
				if (CPU_COMPAT_MODE != UPD9002_COMPAT_UPD70008 || CPU_COMPAT_RETURN_PENDING ||
				    CPU_IP != 0x1007 + op * 2 || CPU_BX != 0xbeef) {
					fprintf(stderr, "M139_MAIN_CPU_NATIVE_OPCODE role=%u op=%u\n", i, op);
					result = FAILURE;
					break;
				}
			}
		}
		if (result != SUCCESS) {
			break;
		}
	}
	memoryva_pc8801 = saved_pc8801;
	memoryva_pc8001 = saved_pc8001;
	memoryva_88_mode = saved_mode;
	sysportva.a = saved_port_a;
	np2cfg.multiple = saved_multiple;
	np2cfg.cpu_speed = saved_speed;
	pccore_clockrestore();
	upd9002_core_reset();
	if (result == SUCCESS) {
		fprintf(
		    stderr,
		    "main CPU roles: VA/PC-8801/PC-8001 reset selection, BIT, T states, native-opcode isolation PASS\n");
	}
	return result;
}
