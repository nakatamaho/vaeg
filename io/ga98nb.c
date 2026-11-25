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

/* M166: I-O DATA GA-98NBIV (CL-GD5434) in the bus slot. See io/ga98nb.h
 * for the port and window model and its sources. */

#include "compiler.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "ga98nb.h"

_GA98NB ga98nb;
UINT8 ga98nb_vram[GA98NB_VRAM_SIZE];

enum {
	GA98NB_SETUP = 0x10,  /* 46E8h: setup mode */
	GA98NB_ENABLE = 0x08, /* 46E8h: video subsystem enabled */
	GA98NB_SR06_UNLOCKED = 0x12,
	GA98NB_SR06_LOCKED = 0x0f,
	GA98NB_CR27_GD5434 = 0xa8
};

BOOL ga98nb_enabled(void) {
	return ((np2cfg.ga98nb_enable != 0) && !(ga98nb.setup & GA98NB_SETUP) &&
	        (ga98nb.setup & GA98NB_ENABLE) && (ga98nb.video_on & 1));
}

static void vga_out(UINT port, REG8 dat) {
	UINT index;

	if (port != 0x3c6) {
		ga98nb.dac_reads = 0;
	}
	switch (port) {
	case 0x3c0:
		if (ga98nb.attr_flip == 0) {
			ga98nb.attr_index = (UINT8)(dat & 0x3f);
		} else {
			ga98nb.attr[ga98nb.attr_index & 0x1f] = (UINT8)dat;
		}
		ga98nb.attr_flip ^= 1;
		break;
	case 0x3c2:
		ga98nb.misc = (UINT8)dat;
		break;
	case 0x3c4:
		ga98nb.seq_index = (UINT8)(dat & 0x1f);
		break;
	case 0x3c5:
		index = ga98nb.seq_index;
		if (index == 0x06) {
			/* the extension key: 12h unlocks, anything else locks */
			ga98nb.seq[6] = ((dat & 0x17) == 0x12) ? GA98NB_SR06_UNLOCKED : GA98NB_SR06_LOCKED;
		} else {
			ga98nb.seq[index] = (UINT8)dat;
		}
		break;
	case 0x3c6:
		if (ga98nb.dac_reads >= 4) {
			ga98nb.hidden_dac = (UINT8)dat;
		} else {
			ga98nb.dac_mask = (UINT8)dat;
		}
		ga98nb.dac_reads = 0;
		break;
	case 0x3ce:
		ga98nb.gr_index = (UINT8)(dat & 0x3f);
		break;
	case 0x3cf:
		ga98nb.gr[ga98nb.gr_index] = (UINT8)dat;
		break;
	case 0x3d4:
		ga98nb.crtc_index = (UINT8)(dat & 0x3f);
		break;
	case 0x3d5:
		if (ga98nb.crtc_index != 0x27) { /* the chip ID is read-only */
			ga98nb.crtc[ga98nb.crtc_index] = (UINT8)dat;
		}
		break;
	default: /* palette, feature control: not modelled */
		break;
	}
}

static REG8 vga_in(UINT port) {
	if (port != 0x3c6) {
		ga98nb.dac_reads = 0;
	}
	switch (port) {
	case 0x3c0:
		return (ga98nb.attr_index);
	case 0x3c1:
		return (ga98nb.attr[ga98nb.attr_index & 0x1f]);
	case 0x3c4:
		return (ga98nb.seq_index);
	case 0x3c5:
		return (ga98nb.seq[ga98nb.seq_index]);
	case 0x3c6:
		if (ga98nb.dac_reads >= 4) {
			return (ga98nb.hidden_dac);
		}
		ga98nb.dac_reads++;
		return (ga98nb.dac_mask);
	case 0x3cc:
		return (ga98nb.misc);
	case 0x3ce:
		return (ga98nb.gr_index);
	case 0x3cf:
		return (ga98nb.gr[ga98nb.gr_index]);
	case 0x3d4:
		return (ga98nb.crtc_index);
	case 0x3d5:
		if (ga98nb.crtc_index == 0x27) {
			return (GA98NB_CR27_GD5434);
		}
		return (ga98nb.crtc[ga98nb.crtc_index]);
	case 0x3da:
		ga98nb.attr_flip = 0;
		ga98nb.status ^= 0x09; /* display enable and vertical retrace */
		return (ga98nb.status);
	default:
		return (0xff);
	}
}

/* The VGA port of card port PORT (xxE0h or xxE1h), or 0 if none. */
static UINT vga_port(UINT port) {
	UINT high;
	UINT odd;

	high = (port >> 8) & 0xff;
	odd = port & 1;
	if (odd && (np2cfg.ga98nb_fault == GA98NB_FAULT_BYTE_BUS)) {
		return (0);
	}
	if ((high >= 0x40) && (high <= 0x4f)) {
		return (0x3c0 + (high - 0x40) + odd);
	}
	if (high == 0x54) {
		return (0x3d4 + odd);
	}
	if (high == 0x55) {
		return (odd ? 0 : 0x3d5);
	}
	if (high == 0x5a) {
		return (odd ? 0 : 0x3da);
	}
	return (0);
}

static void IOOUTCALL ga98nb_o_setup(UINT port, REG8 dat) {
	if (np2cfg.ga98nb_enable) {
		ga98nb.setup = (UINT8)dat;
	}
	(void)port;
}

static void IOOUTCALL ga98nb_o_relay(UINT port, REG8 dat) {
	if (np2cfg.ga98nb_enable) {
		ga98nb.relay = (UINT8)dat;
	}
	(void)port;
}

/* 40E1h reads back the value written, before setup too (00h after reset);
 * PC-98 software detects the card by writing 7Ah and reading it back. */
static REG8 IOINPCALL ga98nb_i_relay(UINT port) {
	(void)port;
	return (np2cfg.ga98nb_enable ? ga98nb.relay : 0xff);
}

static void IOOUTCALL ga98nb_o(UINT port, REG8 dat) {
	UINT vga;

	if (!np2cfg.ga98nb_enable) {
		return;
	}
	vga = vga_port(port);
	if (ga98nb.setup & GA98NB_SETUP) {
		/* in setup only the video enable (port 102h) answers, at 42E0h */
		if (vga == 0x3c2) {
			ga98nb.video_on = (UINT8)(dat & 1);
		}
		return;
	}
	if ((vga != 0) && ga98nb_enabled()) {
		vga_out(vga, dat);
	}
}

static REG8 IOINPCALL ga98nb_i(UINT port) {
	UINT vga;

	vga = vga_port(port);
	if ((vga == 0) || !ga98nb_enabled()) {
		return (0xff);
	}
	return (vga_in(vga));
}

BOOL ga98nb_window(UINT32 address, UINT32 *offset) {
	UINT32 within;
	UINT32 bank;
	UINT shift;
	UINT map;

	if (!ga98nb_enabled() || !(ga98nb.misc & 0x02) ||
	    (np2cfg.ga98nb_fault == GA98NB_FAULT_NO_WINDOW) || (address < GA98NB_WINDOW_BASE) ||
	    (address >= (GA98NB_WINDOW_BASE + GA98NB_WINDOW_SIZE))) {
		return (FALSE);
	}
	map = (ga98nb.gr[0x06] >> 2) & 3; /* A0000h-BFFFFh or A0000h-AFFFFh */
	if (map > 1) {
		return (FALSE);
	}
	within = address - GA98NB_WINDOW_BASE;
	if (ga98nb.gr[0x0b] & 0x01) { /* dual page: E4000h uses GR0A */
		bank = (within & 0x4000) ? ga98nb.gr[0x0a] : ga98nb.gr[0x09];
		within &= 0x3fff;
	} else {
		bank = ga98nb.gr[0x09];
	}
	shift = (ga98nb.gr[0x0b] & 0x20) ? 14 : 12;
	if (np2cfg.ga98nb_fault == GA98NB_FAULT_BANK_ALIAS) {
		bank = 0;
	}
	*offset = ((bank << shift) + within) & (GA98NB_VRAM_SIZE - 1);
	return (TRUE);
}

REG8 ga98nb_window_read(UINT32 offset) {
	return (ga98nb_vram[offset & (GA98NB_VRAM_SIZE - 1)]);
}

void ga98nb_window_write(UINT32 offset, REG8 value) {
	ga98nb_vram[offset & (GA98NB_VRAM_SIZE - 1)] = (UINT8)value;
}

void ga98nb_reset(void) {
	ZeroMemory(&ga98nb, sizeof(ga98nb));
	ga98nb.seq[6] = GA98NB_SR06_LOCKED;
	ga98nb.dac_mask = 0xff;
	ZeroMemory(ga98nb_vram, sizeof(ga98nb_vram));
}

void ga98nb_bind(void) {
	UINT high;

	/* Attached whenever compiled in; with the card off (GA98NBIV=0) every
	 * handler behaves as an empty slot, so the GUI toggle needs no reset. */
	iocore_attachout(0x46e8, ga98nb_o_setup);
	for (high = 0x40; high <= 0x5a; high++) {
		if ((high > 0x4f) && (high != 0x54) && (high != 0x55) && (high != 0x5a)) {
			continue;
		}
		iocore_attachout((high << 8) | 0xe0, ga98nb_o);
		iocore_attachinp((high << 8) | 0xe0, ga98nb_i);
		if (high == 0x40) {
			iocore_attachout(0x40e1, ga98nb_o_relay);
			iocore_attachinp(0x40e1, ga98nb_i_relay);
		} else {
			iocore_attachout((high << 8) | 0xe1, ga98nb_o);
			iocore_attachinp((high << 8) | 0xe1, ga98nb_i);
		}
	}
}
