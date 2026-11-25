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

/* M166: the GA-98NBIV model (io/ga98nb.c): the PC-98 detection sequence,
 * the 8-bit variant, SR06/CR27, the faults, and the E0000h
 * window banks, through the handlers the model attaches. */

#include <stdio.h>
#include <string.h>
#include "compiler.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "ga98nb.h"

NP2CFG np2cfg;

static IOOUT outs[0x10000];
static IOINP inps[0x10000];
static int failures;
static int checks;

BOOL iocore_attachout(UINT port, IOOUT func) {
	outs[port & 0xffff] = func;
	return (TRUE);
}

BOOL iocore_attachinp(UINT port, IOINP func) {
	inps[port & 0xffff] = func;
	return (TRUE);
}

static void out8(UINT port, UINT8 value) {
	if (outs[port]) {
		outs[port](port, value);
	}
}

/* as vaeg's iocore: a word is two byte accesses, port and port + 1 */
static void out16(UINT port, UINT16 value) {
	out8(port, (UINT8)value);
	out8(port + 1, (UINT8)(value >> 8));
}

static UINT8 in8(UINT port) {
	return (inps[port] ? (UINT8)inps[port](port) : 0xff);
}

static void check(const char *name, long got, long want) {
	checks++;
	if (got != want) {
		failures++;
		printf("FAIL %s: got %lX want %lX\n", name, got, want);
	}
}

static void setup_sequence(void) {
	out8(0x46e8, 0x18);
	out8(0x42e0, 0x01);
	out8(0x46e8, 0x08);
	out8(0x42e0, 0xe3);
	out8(0x5ae0, 0x00);
}

static UINT8 sr06_word(void) {
	out16(0x44e0, 0x1206);
	return (in8(0x45e0));
}

static UINT8 sr06_byte(void) {
	out8(0x44e0, 0x06);
	out8(0x45e0, 0x12);
	return (in8(0x45e0));
}

static UINT8 cr27(void) {
	out8(0x54e0, 0x27);
	return (in8(0x55e0));
}

static void fresh(UINT8 fault) {
	memset(&np2cfg, 0, sizeof(np2cfg));
	np2cfg.ga98nb_enable = 1;
	np2cfg.ga98nb_fault = fault;
	ga98nb_reset();
}

static void gr(UINT8 index, UINT8 value) {
	out16(0x4ee0, (UINT16)(index | (value << 8)));
}

static UINT8 peek(UINT32 address) {
	UINT32 offset;
	return (ga98nb_window(address, &offset) ? ga98nb_window_read(offset) : 0xff);
}

static void poke(UINT32 address, UINT8 value) {
	UINT32 offset;
	if (ga98nb_window(address, &offset)) {
		ga98nb_window_write(offset, value);
	}
}

static void window_mode(void) {
	gr(0x06, 0x05); /* graphics, A0000h-AFFFFh */
	gr(0x0b, 0x2d); /* dual page, 16 KiB offsets */
	gr(0x09, 0x00);
	gr(0x0a, 0x01);
}

int main(void) {
	UINT32 offset;

	memset(&np2cfg, 0, sizeof(np2cfg));
	ga98nb_reset();
	ga98nb_bind();
	check("bound 46E8h", outs[0x46e8] != NULL, 1);
	check("bound 45E0h in", inps[0x45e0] != NULL, 1);
	check("bound 44E1h", outs[0x44e1] != NULL, 1);
	check("bound 4FE1h", outs[0x4fe1] != NULL, 1);
	check("bound 55E0h in", inps[0x55e0] != NULL, 1);
	check("bound 5AE0h in", inps[0x5ae0] != NULL, 1);
	check("40E1h is the relay, no input", inps[0x40e1] == NULL, 1);
	check("56E0h not bound", outs[0x56e0] == NULL, 1);

	/* card off: an empty slot */
	setup_sequence();
	check("off: SR06", sr06_word(), 0xff);
	check("off: CR27", cr27(), 0xff);

	/* card on, not set up: still silent */
	fresh(GA98NB_FAULT_NONE);
	check("before setup: SR06", sr06_word(), 0xff);
	out8(0x46e8, 0x18);
	out8(0x44e0, 0x06);
	check("in setup: SR06", in8(0x45e0), 0xff);

	/* the 16-bit detection: OUT16 44E0h <- 1206h */
	fresh(GA98NB_FAULT_NONE);
	setup_sequence();
	check("enabled", ga98nb_enabled(), TRUE);
	check("misc", in8(0x4ce0), 0xe3);
	check("word: SR06", sr06_word(), 0x12);
	check("CR27", cr27(), 0xa8);
	check("CR27 & FCh", cr27() & 0xfc, 0xa8);
	out8(0x54e0, 0x27);
	out8(0x55e0, 0x00);
	check("CR27 read-only", cr27(), 0xa8);
	out16(0x44e0, 0x0006);
	check("locked: SR06", in8(0x45e0), 0x0f);
	check("byte: SR06", sr06_byte(), 0x12);
	out8(0x44e0, 0x07);
	out8(0x45e0, 0x01);
	check("SR07", in8(0x45e0), 0x01);
	check("SR index", in8(0x44e0), 0x07);

	/* a bus that drops the high byte of word I/O */
	fresh(GA98NB_FAULT_BYTE_BUS);
	setup_sequence();
	check("byte bus: word SR06", sr06_word(), 0x0f);
	check("byte bus: byte SR06", sr06_byte(), 0x12);

	/* the hidden DAC: four reads of 46E0h, then a write */
	fresh(GA98NB_FAULT_NONE);
	setup_sequence();
	in8(0x46e0);
	in8(0x46e0);
	in8(0x46e0);
	in8(0x46e0);
	out8(0x46e0, 0x20);
	check("hidden DAC", ga98nb.hidden_dac, 0x20);
	check("DAC mask kept", ga98nb.dac_mask, 0xff);

	/* attribute flip-flop, reset by 5AE0h */
	out8(0x40e0, 0x10);
	out8(0x40e0, 0x41);
	check("attr 10h", ga98nb.attr[0x10], 0x41);
	out8(0x40e0, 0x11);
	in8(0x5ae0);
	out8(0x40e0, 0x12);
	out8(0x40e0, 0x0f);
	check("attr after 5AE0h reset", ga98nb.attr[0x12], 0x0f);

	/* relay */
	out16(0x40e1, 0x00b3);
	check("relay on", ga98nb.relay, 0xb3);

	/* the window: two banks at one address, A -> B -> A */
	check("window decoded once memory decoding is on", ga98nb_window(0xe0200, &offset), TRUE);
	window_mode();
	poke(0xe0200, 0xa5);
	gr(0x09, 0x01);
	poke(0xe0200, 0x96);
	check("bank B", peek(0xe0200), 0x96);
	gr(0x09, 0x00);
	check("bank A kept", peek(0xe0200), 0xa5);
	check("offset of E0200h, bank 0", ga98nb_window(0xe0200, &offset) ? (long)offset : -1, 0x200);
	gr(0x09, 0x03);
	check("offset of E0200h, bank 3", ga98nb_window(0xe0200, &offset) ? (long)offset : -1, 0xc200);
	check("E4000h uses GR0A", ga98nb_window(0xe4010, &offset) ? (long)offset : -1, 0x4010);
	gr(0x0a, 0xff);
	check("GR0A FFh", ga98nb_window(0xe4000, &offset) ? (long)offset : -1, 0x3fc000);
	check("E8000h outside", ga98nb_window(0xe8000, &offset), FALSE);
	gr(0x0b, 0x00); /* single page, 4 KiB */
	gr(0x09, 0x02);
	check("single page, 4 KiB", ga98nb_window(0xe4010, &offset) ? (long)offset : -1, 0x6010);
	gr(0x06, 0x0d); /* B0000h-B7FFFh: no window */
	check("memory map 3", ga98nb_window(0xe0000, &offset), FALSE);
	gr(0x06, 0x05);
	out8(0x42e0, 0xe1); /* video memory off */
	check("misc bit 1 off", ga98nb_window(0xe0000, &offset), FALSE);

	/* faults: aliasing banks, no window */
	fresh(GA98NB_FAULT_BANK_ALIAS);
	setup_sequence();
	window_mode();
	poke(0xe0200, 0xa5);
	gr(0x09, 0x01);
	poke(0xe0200, 0x96);
	gr(0x09, 0x00);
	check("alias: A overwritten", peek(0xe0200), 0x96);
	fresh(GA98NB_FAULT_NO_WINDOW);
	setup_sequence();
	window_mode();
	check("no window", ga98nb_window(0xe0200, &offset), FALSE);

	printf("ga98nb: %d checks, %d failures\n", checks, failures);
	return (failures != 0);
}
