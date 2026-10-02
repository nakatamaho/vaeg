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
#include "io/sysportva.h"

#include <stdio.h>

#if defined(VAEG_UPD9002_M76_TESTING)
static int boot_inputs_selftest(void) {
	const UINT8 saved_boot = np2cfg.v1v2_boot;
	const UINT8 saved_row = keybrd.keymap[0x0d];
	REG8 baseline, selected, released, pressed;
	int passed;

	iocore_create();
	if (iocore_build() != SUCCESS) {
		iocore_destroy();
		return FAILURE;
	}
	systemportva_bind();
	keyboard_bind();
	np2cfg.v1v2_boot = 0;
	baseline = iocore_inp8(0x40);
	np2cfg.v1v2_boot = 1;
	selected = iocore_inp8(0x40);
	keybrd.keymap[0x0d] = 0xff;
	released = iocore_inp8(0x0d);
	keybrd.keymap[0x0d] = 0xfb;
	pressed = iocore_inp8(0x0d);
	passed = !(baseline & 0x08) && ((baseline ^ selected) == 0x08) &&
	         (released == 0xff) && (pressed == 0xfb);
	np2cfg.v1v2_boot = 0;
	passed = passed && (iocore_inp8(0x40) == baseline) && (iocore_inp8(0x0d) == 0xfb);
	np2cfg.v1v2_boot = saved_boot;
	keybrd.keymap[0x0d] = saved_row;
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
	if (boot_inputs_selftest() != SUCCESS) {
		fprintf(stderr, "upd9002-brkem-upd70008: SW7/PC-key input isolation failed\n");
		return FAILURE;
	}
	fprintf(stderr, "upd9002-brkem-upd70008: SW7/PC-key input isolation passed\n");
	fprintf(stderr,
	        "upd9002-brkem-upd70008: BRKEM/BRKEM2, Z80 JR/IX/IY, CALLN/IRET, LD HL, RETEM passed\n");
	fprintf(stderr, "upd9002-brkem-upd70008: alternate register storage, state authority and "
	                "load-before-enter vector reader passed\n");
	return SUCCESS;
#else
	return FAILURE;
#endif
}
