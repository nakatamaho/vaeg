/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
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
#include "tests/upd9002/brkem_upd70008.h"
#include "cpu/upd9002_upd70008.h"
#include "machine/pccore.h"
#include "io/iocore.h"
#include "io/memctrlva.h"
#include "io/upd9002_regs.h"
#include "memoryva/memoryva.h"

#include <stdio.h>

#if defined(VAEG_UPD9002_M76_TESTING)
static int iotrap_register_selftest(void) {
	static const BYTE ranges[8] = {0x50, 0, 0x5b, 0, 0x60, 0, 0x6f, 0};
	UPD9002_IOTRAP byte_state;
	int passed;
	UINT i;

	iocore_create();
	if (iocore_build() != SUCCESS) {
		iocore_destroy();
		return FAILURE;
	}
	upd9002_regs_reset();
	upd9002_regs_bind();
	/* The firmware installs ranges as descending byte writes. */
	for (i = 8; i > 0; i--) {
		iocore_out8(0xffe0 + i - 1, ranges[i - 1]);
	}
	passed = !memcmp(upd9002_iotrap.ranges, ranges, sizeof(ranges)) &&
	         upd9002_iotrap.control == 0;
	iocore_out8(0xffef, 3);
	byte_state = upd9002_iotrap;
	upd9002_regs_reset();
	passed = passed && upd9002_iotrap.control == 0;
	for (i = 0; i < 8; i++) passed = passed && upd9002_iotrap.ranges[i] == 0;
	for (i = 0; i < 8; i += 2) {
		iocore_out16(0xffe0 + i, ranges[i]);
	}
	iocore_out8(0xffef, 3);
	passed = passed && !memcmp(&byte_state, &upd9002_iotrap, sizeof(byte_state));
	iocore_out8(0xffef, 0);
	passed = passed && upd9002_iotrap.control == 0 &&
	         !memcmp(upd9002_iotrap.ranges, ranges, sizeof(ranges));
	upd9002_regs_reset();
	iocore_destroy();
	return passed ? SUCCESS : FAILURE;
}

static int memory_mode_selftest(void) {
	int passed;

	iocore_create();
	if (iocore_build() != SUCCESS) {
		iocore_destroy();
		return FAILURE;
	}
	memctrlva_reset();
	memctrlva_bind();
	passed = (memoryva_88_mode == 0) && (iocore_inp8(0x153) == 0x41);
	iocore_out8(0x153, 0x03);
	passed = passed && (memoryva_88_mode == 1) && (iocore_inp8(0x153) == 0x03);
	iocore_out8(0x153, 0x43);
	passed = passed && (memoryva_88_mode == 0) && (iocore_inp8(0x153) == 0x43);
	/* The ROM switches modes with a word access starting at port 152h. */
	iocore_out16(0x152, 0x0100);
	passed = passed && (memoryva_88_mode == 1) && (iocore_inp16(0x152) == 0x0100);
	iocore_out16(0x152, 0x4100);
	passed = passed && (memoryva_88_mode == 0) && (iocore_inp16(0x152) == 0x4100);
	/* Synthetic ROM/RAM sentinels test both edges and RAM under the ROM. */
	{
		const BYTE saved_first = rom1mem[0x10000];
		const BYTE saved_last = rom1mem[0x17fff];
		rom1mem[0x10000] = 0xa5;
		rom1mem[0x17fff] = 0x5a;
		upd9002_memorywrite_va(0x0ffff, 0x11);
		upd9002_memorywrite_va(0x10000, 0x22);
		upd9002_memorywrite_va(0x17fff, 0x33);
		upd9002_memorywrite_va(0x18000, 0x44);
		passed = passed && (upd9002_memoryread_va(0x10000) == 0x22);
		iocore_out8(0x153, 0x01);
		passed = passed && (upd9002_memoryread_va(0x10000) == 0xa5) &&
		         (upd9002_memoryread_va_w(0x0ffff) == 0xa511) &&
		         (upd9002_memoryread_va_w(0x17fff) == 0x445a);
		upd9002_memorywrite_va_w(0x17fff, 0x6677);
		passed = passed && (upd9002_memoryread_va_w(0x17fff) == 0x665a) &&
		         (rom1mem[0x17fff] == 0x5a);
		iocore_out8(0x031, 0x02); /* MMODE: all RAM. */
		passed = passed && (upd9002_memoryread_va(0x10000) == 0x22) &&
		         (upd9002_memoryread_va_w(0x17fff) == 0x6677);
		iocore_out8(0x031, 0x00);
		passed = passed && (upd9002_memoryread_va(0x10000) == 0xa5);
		iocore_out8(0x153, 0x41);
		passed = passed && (upd9002_memoryread_va(0x10000) == 0x22);
		rom1mem[0x10000] = saved_first;
		rom1mem[0x17fff] = saved_last;
	}
	iocore_out8(0x153, 0x01);
	memctrlva_reset();
	passed = passed && (memoryva_88_mode == 0) && (iocore_inp8(0x153) == 0x41);
	iocore_destroy();
	return passed ? SUCCESS : FAILURE;
}
#endif

int upd9002_brkem_upd70008_main(void) {
#if defined(VAEG_UPD9002_M76_TESTING)
	if (upd9002_upd70008_compat_selftest() != SUCCESS) {
		fprintf(stderr, "upd9002-brkem-upd70008: production uPD70008-compatible bridge failed\n");
		return FAILURE;
	}
	if (upd9002_upd70008_alt_regs_selftest() != SUCCESS) {
		fprintf(stderr, "upd9002-brkem-upd70008: alternate register storage failed\n");
		return FAILURE;
	}
	if (iotrap_register_selftest() != SUCCESS) {
		fprintf(stderr, "upd9002-brkem-upd70008: I/O trap register byte/word writes failed\n");
		return FAILURE;
	}
	if (memory_mode_selftest() != SUCCESS) {
		fprintf(stderr, "upd9002-brkem-upd70008: memory-mode latch failed\n");
		return FAILURE;
	}
	fprintf(stderr, "upd9002-brkem-upd70008: memory-mode byte/word I/O and reset passed\n");
	fprintf(stderr,
	        "upd9002-brkem-upd70008: BRKEM/BRKEM2, Z80 JR/IX/IY, CALLN/IRET, LD HL, RETEM passed\n");
	fprintf(stderr, "upd9002-brkem-upd70008: alternate register storage, state authority and "
	                "load-before-enter vector reader passed\n");
	return SUCCESS;
#else
	return FAILURE;
#endif
}
