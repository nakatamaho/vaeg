/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "compiler.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "wsre.h"

WSRE_STATE wsre;
static const UINT board_ports[] = {0x40e1, 0x42e1, 0x51e1, 0x59e1, 0x5be1};

static UINT vga_port(UINT port) {
	if ((port & 0xf0ff) == 0x40e0)
		return 0x3c0 + ((port >> 8) & 15);
	switch (port) {
	case 0x44e1:
		return 0x3c5;
	case 0x4ee1:
		return 0x3cf;
	case 0x54e0:
		return 0x3d4;
	case 0x54e1:
	case 0x55e0:
		return 0x3d5;
	case 0x5ae0:
		return 0x3da;
	default:
		return 0;
	}
}

static REG8 IOINPCALL wsre_inp(UINT port) {
	if (!wsre.enabled)
		return 0xff;
	if (port == 0x46e8)
		return wsre.setup;
	/* [OPEN] Melco board-port reads await VA/VA2 W1PROBE logs. */
	if ((port & 0xff) == 0xe1)
		return 0xff;
	return gd5428_read(&wsre.chip, (uint16_t)vga_port(port));
}

static void IOOUTCALL wsre_out(UINT port, REG8 value) {
	UINT i;
	if (!wsre.enabled)
		return;
	if (port == 0x46e8) {
		wsre.setup = (uint8_t)value;
		return;
	}
	for (i = 0; i < NELEMENTS(board_ports); ++i) {
		if (port == board_ports[i]) {
			wsre.board[i] = (uint8_t)value;
			return;
		}
	}
	/* [OPEN] E1 data aliases model the proposed word split, not measured
	 * VA-slot behavior. The W1 word rows must confirm this on both models. */
	gd5428_write(&wsre.chip, (uint16_t)vga_port(port), (uint8_t)value);
}

void wsre_reset(void) {
	memset(&wsre, 0, sizeof(wsre));
	if (!np2cfg.wsreenable)
		return;
	wsre.enabled = 1;
	gd5428_reset(&wsre.chip);
}

int wsre_state_valid(const WSRE_STATE *state) {
	return state->enabled == 1 && gd5428_state_valid(&state->chip);
}

void wsre_bind(void) {
	UINT i;
	static const UINT extra[] = {0x54e0, 0x55e0, 0x5ae0, 0x46e8};
	static const UINT aliases[] = {0x44e1, 0x4ee1, 0x54e1};
	if (!wsre.enabled)
		return;
	for (i = 0; i < 16; ++i) {
		iocore_attachout(0x40e0 + i * 0x100, wsre_out);
		iocore_attachinp(0x40e0 + i * 0x100, wsre_inp);
	}
	for (i = 0; i < NELEMENTS(extra); ++i) {
		iocore_attachout(extra[i], wsre_out);
		iocore_attachinp(extra[i], wsre_inp);
	}
	for (i = 0; i < NELEMENTS(board_ports); ++i) {
		iocore_attachout(board_ports[i], wsre_out);
		iocore_attachinp(board_ports[i], wsre_inp);
	}
	for (i = 0; i < NELEMENTS(aliases); ++i)
		iocore_attachout(aliases[i], wsre_out);
}
