/* Copyright (c) 2026 Nakata Maho
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

/* M150: model test of the Video Art Board (io/vab.c) against the X88000
 * 1.5.3 behaviour described in io/vab.h. The port handlers are reached
 * through stub iocore bindings. */

#include <stdio.h>
#include <string.h>
#include "compiler.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "memoryva.h"
#include "vab.h"

NP2CFG np2cfg;
UINT8 memoryva_88_mode;
_MEMORYVA88ERAM memoryva_88_eram;

static IOOUT outs[2];
static IOINP ins[2];

BOOL iocore_attachout(UINT port, IOOUT func) {
	outs[port - 0xb4] = func;
	return (TRUE);
}

BOOL iocore_attachinp(UINT port, IOINP func) {
	ins[port - 0xb4] = func;
	return (TRUE);
}

static int checks;
static int failures;

static void check(const char *name, long got, long want) {
	checks++;
	if (got != want) {
		failures++;
		printf("FAIL %s: got %ld want %ld\n", name, got, want);
	}
}

int main(void) {
	static UINT8 surf[(640 + 4) * 2 * 400];
	const int pitch = (640 + 4) * 2;
	UINT16 *row;

	memset(&np2cfg, 0, sizeof(np2cfg));
	vab_reset();
	vab_bind();
	check("bound b4 out", outs[0] != NULL, 1);
	check("bound b5 in", ins[1] != NULL, 1);

	/* disabled: an empty slot */
	memoryva_88_mode = TRUE;
	memoryva_88_eram.bank = 0x04;
	outs[0](0xb4, 0x41);
	check("disabled b4 write ignored", vab.mode1, 0);
	check("disabled b4 read", ins[0](0xb4), 0xff);
	check("disabled card", vab_card_selected(0x04), 0);
	check("disabled display", vab_display_active(), 0);

	/* enabled: B4h/B5h read back only while E3h selects card 1 */
	np2cfg.vab_enable = 1;
	outs[0](0xb4, 0x41);
	outs[1](0xb5, 0x5a);
	memoryva_88_eram.bank = 0x05; /* card 1, bank 1 */
	check("b4 read card1", ins[0](0xb4), 0x41);
	check("b5 read card1", ins[1](0xb5), 0x5a);
	memoryva_88_eram.bank = 0x01; /* card 0 */
	check("b4 read card0", ins[0](0xb4), 0xff);
	check("b5 read card0", ins[1](0xb5), 0xff);
	memoryva_88_eram.bank = 0x0b; /* card 2 */
	check("b4 read card2", ins[0](0xb4), 0xff);

	/* card selection and bank offsets */
	check("card 1 bank 0", vab_card_selected(0x04), 1);
	check("card 1 bank 3", vab_card_selected(0x07), 1);
	check("card 0", vab_card_selected(0x03), 0);
	check("card 3", vab_card_selected(0x0c), 0);
	check("offset bank 0", (long)vab_ram_offset(0x04, 0x10000), 0x00000);
	check("offset bank 2", (long)vab_ram_offset(0x06, 0x17fff), 0x17fff);
	check("offset bank 3", (long)vab_ram_offset(0x07, 0x10123), 0x18123);

	/* display condition: (B4h & 41h) == 41h in an 88-mode session */
	check("display 41", vab_display_active(), 1);
	outs[0](0xb4, 0x40);
	check("display 40", vab_display_active(), 0);
	outs[0](0xb4, 0x01);
	check("display 01", vab_display_active(), 0);
	outs[0](0xb4, 0xff);
	check("display ff", vab_display_active(), 1);
	memoryva_88_mode = FALSE;
	check("display v3", vab_display_active(), 0);
	memoryva_88_mode = TRUE;

	/* pixel format: little-endian word, G 15-10, R 9-5, B 4-0 */
	vab_ram[0] = 0xe0; vab_ram[1] = 0x03;      /* (0,0): red max   = 03E0h */
	vab_ram[2] = 0x00; vab_ram[3] = 0xfc;      /* (1,0): green max = FC00h */
	vab_ram[4] = 0x1f; vab_ram[5] = 0x00;      /* (2,0): blue max  = 001Fh */
	vab_ram[(199 * 320 + 319) * 2] = 0xff;     /* (319,199): white */
	vab_ram[(199 * 320 + 319) * 2 + 1] = 0xff;
	memset(surf, 0x55, sizeof(surf));
	vab_compose_rgb565(surf, pitch, 640, 400);
	row = (UINT16 *)surf;
	check("red", row[0], 0xf800);
	check("red doubled", row[1], 0xf800);
	check("green", row[2], 0x07e0);
	check("blue", row[4], 0x001f);
	row = (UINT16 *)(surf + (size_t)1 * pitch);
	check("row doubled", row[0], 0xf800);
	row = (UINT16 *)(surf + (size_t)399 * pitch);
	check("bottom right white", row[639], 0xffff);
	check("pitch padding untouched", surf[640 * 2], 0x55);

	vab_reset();
	check("reset mode1", vab.mode1, 0);
	check("reset ram", vab_ram[0], 0);

	printf("%d checks, %d failures\n", checks, failures);
	return ((failures == 0) ? 0 : 1);
}
