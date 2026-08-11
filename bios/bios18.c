#include	"compiler.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"bios.h"
#include	"biosmem.h"
#include	"font.h"


typedef struct {
	UINT8	raster;
	UINT8	pl;
	UINT8	bl;
	UINT8	cl;
} CRTDATA;

static const UINT8 modenum[4] = {3, 1, 0, 2};

static const CRTDATA crtdata[7] = {
						{0x09,	0x1f, 0x08, 0x08},		// 200-20
						{0x07,	0x00, 0x07, 0x08},		// 200-25
						{0x13,	0x1e, 0x11, 0x10},		// 400-20
						{0x0f,	0x00, 0x0f, 0x10},		// 400-25
						{0x17,	0x1c, 0x13, 0x10},		// 480-20
						{0x12,	0x1f, 0x11, 0x10},		// 480-25
						{0x0f,	0x00, 0x0f, 0x10}};		// 480-30

static const UINT8 gdcslavesync[6][8] = {
				{0x02,0x26,0x03,0x11,0x86,0x0f,0xc8,0x94},		// 15-L
				{0x02,0x4e,0x4b,0x0c,0x83,0x06,0xe0,0x95},		// 31-H
				{0x02,0x26,0x03,0x11,0x83,0x07,0x90,0x65},		// 24-L
				{0x02,0x4e,0x07,0x25,0x87,0x07,0x90,0x65},		// 24-M
				{0x02,0x26,0x41,0x0c,0x83,0x0d,0x90,0x89},		// 31-L
				{0x02,0x4e,0x47,0x0c,0x87,0x0d,0x90,0x89}};		// 31-M

typedef struct {
	UINT8	lr;
	UINT8	cfi;
} CSRFORM;

static const CSRFORM csrform[4] = {
						{0x07, 0x3b}, {0x09, 0x4b},
						{0x0f, 0x7b}, {0x13, 0x9b}};


// ---- master

void bios0x18_0a(REG8 mode) {

const CRTDATA	*crt;

	gdc_forceready(GDCWORK_MASTER);

	gdc.mode1 &= ~(0x2d);
	mem[MEMB_CRT_STS_FLAG] = mode;
	crt = crtdata;
	if (!(np2cfg.dipsw[0] & 1)) {
		mem[MEMB_CRT_STS_FLAG] |= 0x80;
		gdc.mode1 |= 0x08;
		crt += 2;
	}
	if (!(mode & 0x01)) {
		crt += 1;						// 25行
	}
	if (mode & 0x02) {
		gdc.mode1 |= 0x04;				// 40桁
	}
	if (mode & 0x04) {
		gdc.mode1 |= 0x01;				// アトリビュート
	}
	if (mode & 0x08) {
		gdc.mode1 |= 0x20;				// コードアクセス
	}
	mem[MEMB_CRT_RASTER] = crt->raster;
	crtc.reg.pl = crt->pl;
	crtc.reg.bl = crt->bl;
	crtc.reg.cl = crt->cl;
	crtc.reg.ssl = 0;
	gdc_restorekacmode();
	bios0x18_10(0);
}

void bios0x18_0c(void) {

	if (!(gdcs.textdisp & GDCSCRN_ENABLE)) {
		gdcs.textdisp |= GDCSCRN_ENABLE;
		screenupdate |= 2;
 	}
}

void bios0x18_10(REG8 curdel) {

	UINT8	sts;
	UINT	pos;

	sts = mem[MEMB_CRT_STS_FLAG];
	mem[MEMB_CRT_STS_FLAG] = sts & (~0x40);
	pos = sts & 0x01;
	if (sts & 0x80) {
		pos += 2;
	}
	mem[MEMB_CRT_CNT] = (curdel << 5);
	gdc.m.para[GDC_CSRFORM + 0] = csrform[pos].lr;
	gdc.m.para[GDC_CSRFORM + 1] = curdel << 5;
	gdc.m.para[GDC_CSRFORM + 2] = csrform[pos].cfi;
	gdcs.textdisp |= GDCSCRN_ALLDRAW2 | GDCSCRN_EXT;
}

REG16 bios0x18_14(REG16 seg, REG16 off, REG16 code) {

	UINT16	size;
const BYTE	*p;
	BYTE	buf[32];
	UINT	i;

	switch(code >> 8) {
		case 0x00:			// 8x8
			size = 0x0101;
			MEML_WRITE16(seg, off, 0x0101);
			p = fontrom + 0x82000 + ((code & 0xff) << 4);
			MEML_WRITESTR(seg, off + 2, p, 8);
			break;

//		case 0x28:
		case 0x29:			// 8x16 KANJI
		case 0x2a:
		case 0x2b:
			size = 0x0102;
			MEML_WRITE16(seg, off, 0x0102);
			p = fontrom;
			p += (code & 0x7f) << 12;
			p += (((code >> 8) - 0x20) & 0x7f) << 4;
			MEML_WRITESTR(seg, off + 2, p, 16);
			break;

		case 0x80:			// 8x16 ANK
			size = 0x0102;
			p = fontrom + 0x80000 + ((code & 0xff) << 4);
			MEML_WRITESTR(seg, off + 2, p, 16);
			break;

		default:
			size = 0x0202;
			p = fontrom;
			p += (code & 0x7f) << 12;
			p += (((code >> 8) - 0x20) & 0x7f) << 4;
			for (i=0; i<16; i++, p++) {
				buf[i*2+0] = *p;
				buf[i*2+1] = *(p+0x800);
			}
			MEML_WRITESTR(seg, off + 2, buf, 32);
			break;
	}
	MEML_WRITE16(seg, off, size);
	return(size);
}

void bios0x18_16(REG8 chr, REG8 atr) {

	UINT32	i;

	for (i=0xa0000; i<0xa2000; i+=2) {
		mem[i+0] = chr;
		mem[i+1] = 0;
	}
	for (; i<0xa3fe0; i+=2) {
		mem[i] = atr;
	}
	gdcs.textdisp |= GDCSCRN_ALLDRAW;
}


// ---- 31khz



// ---- slave

void bios0x18_40(void) {

	gdc_forceready(GDCWORK_SLAVE);
	if (!(gdcs.grphdisp & GDCSCRN_ENABLE)) {
		gdcs.grphdisp |= GDCSCRN_ENABLE;
		screenupdate |= 2;
	}
	mem[MEMB_PRXCRT] |= 0x80;
}

void bios0x18_41(void) {

	gdc_forceready(GDCWORK_SLAVE);
	if (gdcs.grphdisp & GDCSCRN_ENABLE) {
		gdcs.grphdisp &= ~(GDCSCRN_ENABLE);
		screenupdate |= 2;
	}
	mem[MEMB_PRXCRT] &= 0x7f;
}

void bios0x18_42(REG8 mode) {

	UINT8	crtmode;
	int		slave;

	gdc_forceready(GDCWORK_MASTER);
	gdc_forceready(GDCWORK_SLAVE);

	crtmode = modenum[mode >> 6];
		ZeroMemory(gdc.s.para + GDC_SCROLL, 4);
		if (crtmode == 2) {							// ALL
			crtmode = 2;
			if ((mem[MEMB_PRXDUPD] & 0x24) == 0x20) {
				mem[MEMB_PRXDUPD] ^= 4;
				gdc.clock |= 3;
				CopyMemory(gdc.s.para + GDC_SYNC, gdcslavesync[3], 8);
				gdc.s.para[GDC_PITCH] = 80;
				gdcs.grphdisp |= GDCSCRN_EXT;
				mem[MEMB_PRXDUPD] |= 0x08;
			}
		}
		else {
			if ((mem[MEMB_PRXDUPD] & 0x24) == 0x24) {
				mem[MEMB_PRXDUPD] ^= 4;
				gdc.clock &= ~3;
				slave = (mem[MEMB_PRXCRT] & 0x40)?2:0;
				CopyMemory(gdc.s.para + GDC_SYNC, gdcslavesync[slave], 8);
				gdc.s.para[GDC_PITCH] = 40;
				gdcs.grphdisp |= GDCSCRN_EXT;
				mem[MEMB_PRXDUPD] |= 0x08;
			}
			if (crtmode & 1) {				// UPPER
				gdc.s.para[GDC_SCROLL+0] = (200*40) & 0xff;
				gdc.s.para[GDC_SCROLL+1] = (200*40) >> 8;
			}
		}
		if (mem[MEMB_PRXDUPD] & 4) {
			gdc.s.para[GDC_SCROLL+3] = 0x40;
		}
		if ((crtmode == 2) || (!(mem[MEMB_PRXCRT] & 0x40))) {
			gdc.mode1 &= ~(0x10);
			gdc.s.para[GDC_CSRFORM] = 0;
		}
		else {
			gdc.mode1 |= 0x10;
			gdc.s.para[GDC_CSRFORM] = 1;
		}
	if (crtmode != 3) {
		gdcs.disp = (mode >> 4) & 1;
	}
	if (!(mode & 0x20)) {
		gdc.mode1 &= ~0x04;
	}
	else {
		gdc.mode2 |= 0x04;
	}
	gdcs.grphdisp |= GDCSCRN_ALLDRAW2;
	screenupdate |= 2;
}
