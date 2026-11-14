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

/* M149: Digital Arts HyPER-FRAME+ board model. See io/hfplus.h for the
 * measured semantics and the limits. */

#include "compiler.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "hfplus.h"

_HFPLUS hfplus;

static UINT8 hfplus_fb[HFPLUS_FB_SIZE];

/* The probe logs show the ports undriven (FF) at power-on and after the
 * viewer's stop sequence D0=00 EE=00, and driven after the initialisation
 * leaves D0=E1 EE=C0. Any non-zero control or EE byte therefore wakes the
 * port decode; zeroing both returns it to undriven. */
static void hfplus_update_driven(void) {
	hfplus.driven = (UINT8)((hfplus.control || hfplus.ee) ? 1 : 0);
}

static BOOL hfplus_in_bounds(void) {
	/* Rows 0-409 fit the 6-bank frame memory; the viewer's special
	 * coordinates (Y=0191/0193, X up to 019F) stay inside. */
	return ((hfplus.y < (UINT16)409) &&
			(hfplus.x < (UINT16)HFPLUS_W) &&
			(((UINT32)hfplus.y * HFPLUS_W + hfplus.x) * 3 + 2 < HFPLUS_FB_SIZE));
}

void IOOUTCALL hfplus_o_d0(UINT port, REG8 dat) {
	hfplus.control = (UINT8)dat;
	hfplus_update_driven();
	(void)port;
}

void IOOUTCALL hfplus_o_d3(UINT port, REG8 dat) {
	hfplus.d3 = (UINT8)dat;
	hfplus_update_driven();
	(void)port;
}

void IOOUTCALL hfplus_o_ee(UINT port, REG8 dat) {
	hfplus.ee = (UINT8)dat;
	hfplus_update_driven();
	(void)port;
}

void IOOUTCALL hfplus_o_xlow(UINT port, REG8 dat) {
	hfplus.x = (UINT16)((hfplus.x & 0xff00) | dat);
	(void)port;
}

void IOOUTCALL hfplus_o_xhigh(UINT port, REG8 dat) {
	hfplus.x = (UINT16)((hfplus.x & 0x00ff) | (dat << 8));
	(void)port;
}

void IOOUTCALL hfplus_o_ylow(UINT port, REG8 dat) {
	hfplus.y = (UINT16)((hfplus.y & 0xff00) | dat);
	(void)port;
}

void IOOUTCALL hfplus_o_yhigh(UINT port, REG8 dat) {
	hfplus.y = (UINT16)((hfplus.y & 0x00ff) | (dat << 8));
	(void)port;
}

static void hfplus_pixel(UINT port, REG8 dat) {
	UINT32 off;
	if (hfplus_in_bounds()) {
		off = ((UINT32)hfplus.y * HFPLUS_W + hfplus.x) * 3;
		if (port == 0x00d8) {
			hfplus_fb[off] = (UINT8)dat;
		} else if (port == 0x00da) {
			hfplus_fb[off + 1] = (UINT8)dat;
		} else {
			hfplus_fb[off + 2] = (UINT8)dat;
		}
	}
}

void IOOUTCALL hfplus_o_r(UINT port, REG8 dat) {
	hfplus_pixel(port, dat);
}

void IOOUTCALL hfplus_o_g(UINT port, REG8 dat) {
	hfplus_pixel(port, dat);
}

void IOOUTCALL hfplus_o_b(UINT port, REG8 dat) {
	hfplus_pixel(port, dat);
}

REG8 IOINPCALL hfplus_i_d0(UINT port) {
	(void)port;
	if (!hfplus.driven) {
		return (0xff);
	}
	return (hfplus.control);
}

REG8 IOINPCALL hfplus_i_d3(UINT port) {
	(void)port;
	if (!hfplus.driven) {
		return (0xff);
	}
	return (hfplus.d3);
}

REG8 IOINPCALL hfplus_i_pixel(UINT port) {
	UINT32 off;
	(void)port;
	if (!hfplus.driven || !hfplus_in_bounds()) {
		return (0xff);
	}
	off = ((UINT32)hfplus.y * HFPLUS_W + hfplus.x) * 3;
	if (port == 0x00d8) {
		return (hfplus_fb[off]);
	} else if (port == 0x00da) {
		return (hfplus_fb[off + 1]);
	}
	return (hfplus_fb[off + 2]);
}

/* IN 00D4-00D7 and IN 00EE are not attached: the VA bus returns FF for
 * them, which is what the probe log shows. */

void hfplus_reset(void) {
	ZeroMemory(&hfplus, sizeof(hfplus));
	ZeroMemory(hfplus_fb, sizeof(hfplus_fb));
}

void hfplus_bind(void) {
	if (!np2cfg.hfplus_enable) {
		return;
	}
	iocore_attachout(0x00d0, hfplus_o_d0);
	iocore_attachout(0x00d3, hfplus_o_d3);
	iocore_attachout(0x00d4, hfplus_o_xlow);
	iocore_attachout(0x00d5, hfplus_o_xhigh);
	iocore_attachout(0x00d6, hfplus_o_ylow);
	iocore_attachout(0x00d7, hfplus_o_yhigh);
	iocore_attachout(0x00d8, hfplus_o_r);
	iocore_attachout(0x00da, hfplus_o_g);
	iocore_attachout(0x00dc, hfplus_o_b);
	iocore_attachout(0x00ee, hfplus_o_ee);
	iocore_attachinp(0x00d0, hfplus_i_d0);
	iocore_attachinp(0x00d3, hfplus_i_d3);
	iocore_attachinp(0x00d8, hfplus_i_pixel);
	iocore_attachinp(0x00da, hfplus_i_pixel);
	iocore_attachinp(0x00dc, hfplus_i_pixel);
}

UINT8 *hfplus_framebuffer(void) {
	return (hfplus_fb);
}

int hfplus_write_ppm(const char *path) {
	FILE *fp;
	UINT16 y;
	fp = fopen(path, "wb");
	if (fp == NULL) {
		return (-1);
	}
	fprintf(fp, "P6\n%u %u\n255\n", (unsigned)HFPLUS_W, (unsigned)HFPLUS_H);
	for (y = 0; y < (UINT16)HFPLUS_H; y++) {
		fwrite(hfplus_fb + (UINT32)y * HFPLUS_W * 3, 1,
				(UINT32)HFPLUS_W * 3, fp);
	}
	fclose(fp);
	return (0);
}
