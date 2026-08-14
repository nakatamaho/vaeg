/*
 * cgromva.c: PC-88VA V3 CGROM port and hardware character-code decoder
 *
 * Known limitations:
 *   Invalid-code behavior still requires real-hardware verification.
 *   The 8x8 font path is incomplete.
 */

#include "compiler.h"
#include "cpucore.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "iocoreva.h"

#include "memoryva.h"

#define SETLOWBYTE(x, y) (x) = ((x) & 0xff00 | (y))
#define SETHIGHBYTE(x, y) (x) = ((x) & 0x00ff | ((WORD)(y) << 8))

_CGROMVA cgromva;

static BYTE tofu[32] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

/*
 * Return the glyph storage addressed by a VA hardware character code.
 * hccode fields:
 *   bit 15 selects the right half of a double-width glyph.
 *   bits 14-8 contain the second JIS byte.
 *   bit 7 is zero.
 *   bits 6-0 contain the first JIS byte minus 20H.
 */
BYTE *cgromva_font(UINT16 hccode) {
	int lr;
	UINT16 jis1;
	UINT16 jis2;
	BYTE *base;
	unsigned int font;

	lr = hccode >> 15;
	jis1 = (hccode & 0x7f) + 0x20;
	jis2 = (hccode >> 8) & 0x7f;

	if (jis2 == 0 && lr == 0) {
		// ANK is valid only when the right-half flag is clear.
		base = fontmem;
		if (videova.txtmode & 0x04) {
			// 8-dot ANK glyph.
			font = 0x41000 + ((hccode & 0xff) << 3);
			/* Tekumani describes 42000H with a 16-byte stride. The loaded
			 * VA font image instead uses an 8-byte stride from 41000H. */
		} else {
			// 16-dot ANK glyph.
			font = 0x40000 + ((hccode & 0xff) << 4);
		}
	} else {
		if (jis1 < 0x28) {
			// JIS non-kanji block.
			base = fontmem;
			font = lr + ((jis2 & 0x60) << 8) + ((jis1 & 0x07) << 10) + ((jis2 & 0x1f) << 5);
		} else if (jis1 < 0x30) {
			// NEC non-kanji extension.
			base = fontmem;
			font =
			    lr + 0x40000 + ((jis2 & 0x60) << 8) + ((jis1 & 0x07) << 10) + ((jis2 & 0x1f) << 5);
		} else if (jis1 < 0x40) {
			// JIS level-1 block, 3xxx.
			base = fontmem;
			font =
			    lr + (((UINT32)jis2 & 0x60) << 10) + ((jis1 & 0x0f) << 10) + ((jis2 & 0x1f) << 5);
		} else if (jis1 < 0x50) {
			// JIS level-1 block, 4xxx.
			base = fontmem;
			font = lr + 0x4000 + (((UINT32)jis2 & 0x60) << 10) + ((jis1 & 0x0f) << 10) +
			       ((jis2 & 0x1f) << 5);
		} else if (jis1 < 0x60) {
			// JIS level-2 block, 5xxx.
			base = fontmem;
			font = lr + 0x20000 + (((UINT32)jis2 & 0x60) << 10) + ((jis1 & 0x0f) << 10) +
			       ((jis2 & 0x1f) << 5);
		} else if (jis1 < 0x70) {
			// JIS level-2 block, 6xxx.
			base = fontmem;
			font = lr + 0x20000 + 0x4000 + (((UINT32)jis2 & 0x60) << 10) + ((jis1 & 0x0f) << 10) +
			       ((jis2 & 0x1f) << 5);
		} else if (jis1 < 0x76) {
			// JIS level-2 block, 7xxx.
			base = fontmem;
			font =
			    lr + 0x20000 + ((jis2 & 0x60) << 8) + ((jis1 & 0x07) << 10) + ((jis2 & 0x1f) << 5);
		} else if (jis1 < 0x78) {
			// User-defined character block.
			if (jis1 == 0x77 && (jis2 == 0x7e || jis2 == 0x7f)) {
				// Codes 777EH and 777FH use the solid fallback glyph.
				base = tofu;
				font = lr;
			} else {
				base = backupmem;
				font = lr + ((jis2 & 0x60) << 6) + ((jis1 & 0x01) << 10) + ((jis2 & 0x1f) << 5);
			}
		} else {
			// Undefined hardware character code.
			base = fontmem;
			font = 0;
		}
	}
	return base + font;
}

/*
 * Return the number of CGROM bytes used by one glyph raster row.
 * hccode fields:
 *   bit 15 selects the right half of a double-width glyph.
 *   bits 14-8 contain the second JIS byte.
 *   bit 7 is zero.
 *   bits 6-0 contain the first JIS byte minus 20H.
 */
int cgromva_width(UINT16 hccode) {
	return (hccode & 0x7f00) == 0 ? 1 : 2;
}

/*
 * Convert the CGROM port selector and row-side bit to the hardware character
 * code form used by the TVRAM renderer.
 */
static UINT16 curhccode(void) {
	UINT16 hccode;

	hccode = cgromva.cgaddr & 0x7fff | ((cgromva.cgrow & 0x20) ? 0x0000 : 0x8000);
	return hccode;
}

// ---- I/O

static void IOOUTCALL cgromva_o14c(UINT port, REG8 dat) {
	SETLOWBYTE(cgromva.cgaddr, dat);
}

static void IOOUTCALL cgromva_o14d(UINT port, REG8 dat) {
	// Bit 15 is supplied by the 014FH left/right selector, not this byte.
	SETHIGHBYTE(cgromva.cgaddr, dat & 0x7f);
}

static REG8 IOINPCALL cgromva_i14e(UINT port) {
	BYTE *font;
	UINT16 hccode;
	int row;

	hccode = curhccode();
	font = cgromva_font(hccode);

	if (hccode < 0x100 && videova.txtmode & 0x04) {
		// 8-dot ANK glyphs have eight raster rows.
		row = cgromva.cgrow & 0x07;
	} else {
		row = cgromva.cgrow & 0x0f;
	}
	font += cgromva_width(hccode) * row;

	/* Tekumani leaves reads of JIS 777EH and 777FH undefined; the current
	 * fallback glyph returns FFH bytes for those codes. */
	return *font;
}

static void IOOUTCALL cgromva_o14e(UINT port, REG8 dat) {
	int lr;
	UINT16 jis1;
	UINT16 jis2;
	UINT16 hccode;
	int writable = FALSE;
	BYTE *font;

	hccode = curhccode();
	lr = hccode >> 15;
	jis1 = (hccode & 0x7f) + 0x20;
	jis2 = (hccode >> 8) & 0x7f;

	if (jis2 == 0 && lr == 0) {
		// ANK is valid only when the right-half flag is clear.
		writable = FALSE;
	} else {
		if (jis1 < 0x76) {
			writable = FALSE;
		} else if (jis1 < 0x77) {
			// User-defined character block. (76xx)
			writable = TRUE;
		} else if (jis1 < 0x78) {
			// User-defined character block. (77xx)
			if (jis2 == 0x7e || jis2 == 0x7f) {
				// Codes 777EH and 777FH are not writable.
				writable = FALSE;
			} else {
				writable = TRUE;
			}
		} else {
			writable = FALSE;
		}
	}

	if (writable) {
		font = cgromva_font(hccode);
		font += cgromva_width(hccode) * (cgromva.cgrow & 0x0f);
		*font = dat;
	}
}

static void IOOUTCALL cgromva_o14f(UINT port, REG8 dat) {
	cgromva.cgrow = dat;
}

// ---- I/F

void cgromva_reset(void) {
	ZeroMemory(&cgromva, sizeof(cgromva));
}

void cgromva_bind(void) {
	iocore_attachout(0x14c, cgromva_o14c);
	iocore_attachout(0x14d, cgromva_o14d);
	iocore_attachout(0x14e, cgromva_o14e);
	iocore_attachout(0x14f, cgromva_o14f);

	iocore_attachinp(0x14e, cgromva_i14e);
}
