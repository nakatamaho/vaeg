/*
 * sysportva.c: PC-88VA V3 system, calendar, printer, and mode-switch ports
 */


#include	"compiler.h"
#include	"cpucore.h"
#include	"machine/pccore.h"
#include	"iocore.h"
#include	"iocoreva.h"
#include	"sound.h"
#include	"fmboard.h"
#include	"beep.h"
#include	"sysmng.h"

	_SYSPORTVACFG	sysportvacfg = {0xcd};

	_SYSPORTVA		sysportva = {0};


static void modeled_oneventset() {
	BYTE led;
	int i;

	led = (sysportva.c >> 4) & 0x07;
	if (led == 0x07) {
		// Mode-LED latch 111b denotes all LEDs off.
		led = 0x00;		// Preserve the frontend normalization before active-low inversion.
	}
	for (i = 0; i < 3; i++) {
		sysmng_modeled((BYTE)i, (BYTE)(~led & 1));
		led >>= 1;
	}
}

static void calendar_ondataset() {
	upd4990_o20(0,
		(REG8) ((sysportva.port010 & 0x07) |		// Calendar commands C0-C2.
		        ((sysportva.port010 & 0x08) << 2) |	// Calendar serial data output.
		        ((sysportva.port040 & 0x06) << 2))	// Calendar strobe and clock.
	);
}

// ---- I/O

static void IOOUTCALL sysp_o010(UINT port, REG8 dat) {
	sysportva.port010 = dat;
	calendar_ondataset();
	// The printer data output sharing port 010H is not yet modeled.
}

static void IOOUTCALL sysp_o032(UINT port, REG8 dat) {
//	TRACEOUT(("sysp_o032 - %x %x %.4x:%.4x", port, dat, CPU_CS, CPU_IP));
	sysportva.port032 = dat & 0xbf;		// V3 forces legacy GVAM low; port 510H controls native access.
	fmboard_setintmask((BYTE)(dat & 0x80));
}

static REG8 IOINPCALL sysp_i032(UINT port) {
//	TRACEOUT(("sysp_i032 - %x %.4x:%.4x", port, CPU_CS, CPU_IP));
	return (sysportva.port032 & 0x7f) | (fmboard_getintmask() & 0x80);
}

/*
 * Port 040H output latch:
 *   bit7	FBEEP
 *   bit6	JOP1
 *   bit5	BEEP
 *   bit4	1
 *   bit3	0
 *   bit2	CCLK
 *   bit1	CSTB
 *   bit0	XPSTB
 */
static void IOOUTCALL sysp_o040(UINT port, REG8 dat) {
	sysportva.port040 = dat;
	calendar_ondataset();
	mouseifva_outstrobe((UINT8)((dat & 0x40) >> 6));
	// FBEEP, BEEP, and printer-strobe outputs are not modeled here.
}

/*
 * Port 040H input status:
 *   bit7	1
 *   bit6	1
 *   bit5	VRTC
 *   bit4	CDI
 *   bit3	SW7
 *   bit2	DCD
 *   bit1	SW1
 *   bit0	PBSY
 */
static REG8 IOINPCALL sysp_i040(UINT port) {
	UINT8 ret;

	ret =
		0xc0 |							// Bits 7 and 6 read as one.
		//(tsp.vsync & 0x20) |			// Direct VRTC source, retained for comparison.
		(tsp.sysp4vsync & 0x20) |		// VRTC as latched for system port 4.
		((uPD4990.cdat & 0x01) << 4) |	// CDI: calendar serial data input.
		((videova_hsyncmode() == VIDEOVA_24_8KHZ) ? 0 : 0x02) |
										// SW1: 0 for 24.8 kHz, 1 for 15.7 kHz.
		0x01;							// PBSY: printer not busy in the current model.

	return ret;
}

static REG8 IOINPCALL sysp_i150(UINT port) {
	return sysportva.modesw & 0x00ff;
}

static REG8 IOINPCALL sysp_i151(UINT port) {
	return sysportva.modesw >> 8;
}

static void IOOUTCALL sysp_o190(UINT port, REG8 dat) {
	dat &= 0x1d;
	if ((dat ^ sysportva.port190) & 0xfe) {
		TRACEOUT(("o190: unsupported bits are specified: 0x%.2x", dat));
	}
	sysportva.port190 = dat;
}

static REG8 IOINPCALL sysp_i190(UINT port) {
	return sysportva.port190;
}

static void IOOUTCALL sysp_o1c6(UINT port, REG8 dat) {
	sysportva.modesw = (dat & 0x01) ? 0xfffe : 0xfffd;
	if (dat & 0x02) {
		sysportva.a |= 0x20;
	}
	else {
		sysportva.a &= ~0x20;
	}
	if ((dat & 0xfc) != 0x04) {
		TRACEOUT(("o1c6: unsupported bits are specified: 0x%.2x", dat));
	}
}

static void IOOUTCALL sysp_o1cd(UINT port, REG8 dat) {

	if ((sysportva.c ^ dat) & 0x04) {					// ver0.29
		rs232c.send = 1;
	}
	sysportva.c = dat;
	sysport.c = (sysport.c & 0xf0) | (dat & 0x0f);
	beep_oneventset();
	modeled_oneventset();
	(void)port;
}

static void IOOUTCALL sysp_o1cf(UINT port, REG8 dat) {

	REG8	bit;

	if (!(dat & 0xf0)) {
		bit = 1 << (dat >> 1);
		if (dat & 1) {
			sysportva.c |= bit;
		}
		else {
			sysportva.c &= ~bit;
		}
		sysport.c = (sysport.c & 0xf0) | (sysportva.c & 0x0f);
		if (bit == 0x04) {									// ver0.29
			rs232c.send = 1;
		}
		else if (bit == 0x08) {
			beep_oneventset();
		}
		modeled_oneventset();
	}
	(void)port;
}


static REG8 IOINPCALL sysp_i1c9(UINT port) {
	return sysportva.a;
}


static REG8 IOINPCALL sysp_i1cb(UINT port) {

	REG8	ret;

	ret = (videova_hsyncmode() == VIDEOVA_24_8KHZ) ? 0x08 : 0;	// XSW1: one for the active 24.8 kHz output.
/*
	ret = ((~np2cfg.dipsw[0]) & 1) << 3;
 */
	ret |= rs232c_stat();
/*
	ret |= uPD4990.cdat;
	(void)port;
 */
	return(ret);
}


static REG8 IOINPCALL sysp_i1cd(UINT port) {

	(void)port;
	return(sysportva.c);
}


// ---- I/F

void systemportva_reset(void) {
	sysportva.a |= 0xc1;
	sysportva.c = 0xf9;
	sysportva.port010 = 0;
	sysportva.port040 = 0;
	sysportva.port190 &= 0x01;	// Preserve only RSTMD.
	sysportva.port190 |= 0x18;	// Restore FBEN=1 and AVC=10b.
	//beep_oneventset();
	//modeled_oneventset();
}

void systemportva_bind(void) {
	modeled_oneventset();					// State loading occurs between reset and bind;
											// refresh the frontend LEDs here from the
											// restored system-port latch.

	iocore_attachout(0x010, sysp_o010);
	iocore_attachout(0x032, sysp_o032);
	iocore_attachinp(0x032, sysp_i032);
	iocore_attachout(0x040, sysp_o040);
	iocore_attachinp(0x040, sysp_i040);

	iocore_attachinp(0x150, sysp_i150);
	iocore_attachinp(0x151, sysp_i151);

	iocore_attachout(0x190, sysp_o190);
	iocore_attachinp(0x190, sysp_i190);

	iocore_attachout(0x1c6, sysp_o1c6);

	iocore_attachout(0x1cd, sysp_o1cd);
	iocore_attachout(0x1cf, sysp_o1cf);
	iocore_attachinp(0x1c9, sysp_i1c9);
	iocore_attachinp(0x1cb, sysp_i1cb);
	iocore_attachinp(0x1cd, sysp_i1cd);
}

