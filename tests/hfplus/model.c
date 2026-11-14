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

/* M149: model test of the HyPER-FRAME+ board (io/hfplus.c) against the
 * semantics measured on the real VA (task file sections 8-9). The probes
 * HFV0/HFV1/HFV2/HFV3 run the same sequences; here the port handlers are
 * driven directly. Attached ports are recorded by stubs instead of the
 * real iocore. */

#include <stdio.h>
#include <string.h>
#include "compiler.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "hfplus.h"

/* the port handlers are the model's entry points; the real iocore binds
 * them in hfplus_bind() (recorded here by the stubs above) */
void IOOUTCALL hfplus_o_d0(UINT port, REG8 val);
void IOOUTCALL hfplus_o_d3(UINT port, REG8 val);
void IOOUTCALL hfplus_o_ee(UINT port, REG8 val);
void IOOUTCALL hfplus_o_xlow(UINT port, REG8 val);
void IOOUTCALL hfplus_o_xhigh(UINT port, REG8 val);
void IOOUTCALL hfplus_o_ylow(UINT port, REG8 val);
void IOOUTCALL hfplus_o_yhigh(UINT port, REG8 val);
void IOOUTCALL hfplus_o_r(UINT port, REG8 val);
void IOOUTCALL hfplus_o_g(UINT port, REG8 val);
void IOOUTCALL hfplus_o_b(UINT port, REG8 val);
REG8 IOINPCALL hfplus_i_d0(UINT port);
REG8 IOINPCALL hfplus_i_d3(UINT port);
REG8 IOINPCALL hfplus_i_pixel(UINT port);

NP2CFG np2cfg;

static struct {
	UINT16 ports[32];
	UINT8 kind[32]; /* 1 out, 2 in */
	UINT count;
} attached;

BOOL iocore_attachout(UINT port, IOOUT func) {
	(void)func;
	attached.ports[attached.count] = (UINT16)port;
	attached.kind[attached.count] = 1;
	attached.count++;
	return (TRUE);
}

BOOL iocore_attachinp(UINT port, IOINP func) {
	(void)func;
	attached.ports[attached.count] = (UINT16)port;
	attached.kind[attached.count] = 2;
	attached.count++;
	return (TRUE);
}

static int failures;
static int checks;

static void check(const char *name, long got, long want) {
	checks++;
	if (got != want) {
		failures++;
		printf("FAIL %s: got %ld want %ld\n", name, got, want);
	}
}

/* the JPGVHF viewer's initialisation without its PC-98 BIOS call */
static void init_sequence(void) {
	hfplus_o_ee(0x00ee, 0xc0);
	hfplus_o_d0(0x00d0, 0xa5);
	hfplus_o_d3(0x00d3, 0x00);
	hfplus_o_ylow(0x00d6, 0x91);
	hfplus_o_yhigh(0x00d7, 0x01);
	hfplus_o_xlow(0x00d4, 0x80);
	hfplus_o_xhigh(0x00d5, 0x01);
	hfplus_o_r(0x00d8, 0x00);
	hfplus_o_d0(0x00d0, 0xa1);
	hfplus_o_g(0x00da, 0x00);
	hfplus_o_d0(0x00d0, 0xe1);
}

/* one coordinate in the paper's byte order (D7 D6 D5 D4) */
static void select_xy_byte(UINT16 x, UINT16 y) {
	hfplus_o_yhigh(0x00d7, (UINT8)(y >> 8));
	hfplus_o_ylow(0x00d6, (UINT8)y);
	hfplus_o_xhigh(0x00d5, (UINT8)(x >> 8));
	hfplus_o_xlow(0x00d4, (UINT8)x);
}

/* one coordinate with word OUTs, low byte first (D6, then D4) */
static void select_xy_word(UINT16 x, UINT16 y) {
	hfplus_o_ylow(0x00d6, (UINT8)y);
	hfplus_o_yhigh(0x00d7, (UINT8)(y >> 8));
	hfplus_o_xlow(0x00d4, (UINT8)x);
	hfplus_o_xhigh(0x00d5, (UINT8)(x >> 8));
}

static void write_rgb(UINT8 r, UINT8 g, UINT8 b) {
	hfplus_o_r(0x00d8, r);
	hfplus_o_g(0x00da, g);
	hfplus_o_b(0x00dc, b);
}

static void read_rgb(UINT8 rgb[3]) {
	rgb[0] = hfplus_i_pixel(0x00d8);
	rgb[1] = hfplus_i_pixel(0x00da);
	rgb[2] = hfplus_i_pixel(0x00dc);
}

int main(void) {
	UINT8 rgb[3];
	int i;

	memset(&np2cfg, 0, sizeof(np2cfg));
	np2cfg.hfplus_enable = 1;
	hfplus_reset();
	hfplus_bind();
	check("attached ports", attached.count, 15);

	/* cold: every measured port reads FF */
	check("cold d0", hfplus_i_d0(0x00d0), 0xff);
	check("cold d3", hfplus_i_d3(0x00d3), 0xff);
	check("cold r", hfplus_i_pixel(0x00d8), 0xff);
	check("cold g", hfplus_i_pixel(0x00da), 0xff);
	check("cold b", hfplus_i_pixel(0x00dc), 0xff);

	/* the initialisation leaves the control registers readable */
	init_sequence();
	check("after init d0", hfplus_i_d0(0x00d0), 0xe1);
	check("after init d3", hfplus_i_d3(0x00d3), 0x00);

	/* 12-point readback in both coordinate modes, as HFV1 */
	{
		static const UINT16 pts[12][2] = {
				{40, 10},   {120, 10},  {200, 10},  {280, 10},  {360, 10},  {440, 10},
				{520, 10},  {600, 10},  {0, 350},   {320, 350}, {639, 350}, {639, 399}};
		static const UINT8 pat[12][3] = {
				{0xff, 0xff, 0xff}, {0x5a, 0xa5, 0x3c}, {0x01, 0x02, 0x03},
				{0x00, 0x00, 0x00}, {0xff, 0x00, 0xff}, {0x12, 0x34, 0x56},
				{0x7f, 0x7f, 0x7f}, {0xfe, 0xfd, 0xfc}, {0xc3, 0x81, 0x42},
				{0x99, 0x66, 0x33}, {0x80, 0x40, 0xc0}, {0x11, 0x22, 0x33}};
		for (i = 0; i < 12; i++) {
			char name[64];
			select_xy_byte(pts[i][0], pts[i][1]);
			write_rgb(pat[i][0], pat[i][1], pat[i][2]);
			read_rgb(rgb);
			sprintf(name, "byte readback %d r", i);
			check(name, rgb[0], pat[i][0]);
			sprintf(name, "byte readback %d g", i);
			check(name, rgb[1], pat[i][1]);
			sprintf(name, "byte readback %d b", i);
			check(name, rgb[2], pat[i][2]);
			/* the frame buffer itself holds the same bytes */
			{
				UINT32 off = ((UINT32)pts[i][1] * HFPLUS_W + pts[i][0]) * 3;
				UINT8 *fb = hfplus_framebuffer();
				sprintf(name, "byte framebuffer %d", i);
				check(name, (fb[off] << 16) | (fb[off + 1] << 8) | fb[off + 2],
						(pat[i][0] << 16) | (pat[i][1] << 8) | pat[i][2]);
			}
		}
		for (i = 0; i < 12; i++) {
			char name[64];
			select_xy_word(pts[i][0], pts[i][1]);
			write_rgb((UINT8)(pat[i][0] ^ 0xff), (UINT8)(pat[i][1] ^ 0xff),
					(UINT8)(pat[i][2] ^ 0xff));
			read_rgb(rgb);
			sprintf(name, "word readback %d r", i);
			check(name, rgb[0], (UINT8)(pat[i][0] ^ 0xff));
			sprintf(name, "word readback %d g", i);
			check(name, rgb[1], (UINT8)(pat[i][1] ^ 0xff));
			sprintf(name, "word readback %d b", i);
			check(name, rgb[2], (UINT8)(pat[i][2] ^ 0xff));
		}
	}

	/* the special-coordinate area (Y=0191/0193) stays inside the 6 banks */
	{
		UINT32 off;
		select_xy_word(0x019f, 0x0193);
		write_rgb(0xaa, 0x55, 0xaa);
		read_rgb(rgb);
		check("special y=0193 r", rgb[0], 0xaa);
		check("special y=0193 g", rgb[1], 0x55);
		check("special y=0193 b", rgb[2], 0xaa);
		off = ((UINT32)0x0193 * HFPLUS_W + 0x019f) * 3;
		check("special fb size",
				(off + 2 < HFPLUS_FB_SIZE) ? 1 : 0, 1);
	}

	/* the stop sequence returns the ports to undriven, as HFV2 /STOP */
	hfplus_o_d0(0x00d0, 0x00);
	hfplus_o_ee(0x00ee, 0x00);
	check("stopped d0", hfplus_i_d0(0x00d0), 0xff);
	check("stopped d3", hfplus_i_d3(0x00d3), 0xff);
	check("stopped r", hfplus_i_pixel(0x00d8), 0xff);

	/* the framebuffer keeps its contents across the stop state, and a
	 * fresh wake-up sees them again (state lives in VRAM on the board).
	 * (40,10) holds the word phase's inverted pattern 00/00/00. */
	init_sequence();
	select_xy_byte(40, 10);
	read_rgb(rgb);
	check("restart keeps vram r", rgb[0], 0x00);
	check("restart keeps vram g", rgb[1], 0x00);
	check("restart keeps vram b", rgb[2], 0x00);

	/* hfplus_reset() is power-cycle semantics: the frame buffer clears
	 * and the board wakes undriven; a write wakes it, reads then see
	 * the cleared memory */
	hfplus_reset();
	select_xy_byte(40, 10);
	read_rgb(rgb);
	check("power cycle undriven r", rgb[0], 0xff);
	hfplus_o_d0(0x00d0, 0xe1);
	read_rgb(rgb);
	check("power cycle clears r", rgb[0], 0x00);
	check("power cycle clears g", rgb[1], 0x00);
	check("power cycle clears b", rgb[2], 0x00);

	/* the PPM dump writes the visible raster */
	np2cfg.hfplus_enable = 1;
	init_sequence();
	select_xy_byte(0, 0);
	write_rgb(0x10, 0x20, 0x30);
	{
		FILE *fp;
		check("ppm dump", hfplus_write_ppm("hfplus_test.ppm"), 0);
		fp = fopen("hfplus_test.ppm", "rb");
		if (fp == NULL) {
			failures++;
			printf("FAIL ppm open\n");
		} else {
			char header[32];
			UINT8 px[3];
			fgets(header, sizeof(header), fp);            /* P6 */
			fgets(header, sizeof(header), fp);            /* 640 400 */
			fgets(header, sizeof(header), fp);            /* 255 */
			check("ppm size", fread(px, 1, 3, fp), 3);
			check("ppm row0 r", px[0], 0x10);
			check("ppm row0 g", px[1], 0x20);
			check("ppm row0 b", px[2], 0x30);
			fclose(fp);
			remove("hfplus_test.ppm");
		}
	}

	printf("%d checks, %d failures\n", checks, failures);
	return ((failures == 0) ? 0 : 1);
}
