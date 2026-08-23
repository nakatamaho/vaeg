/*
 * tsp.c: PC-88VA Text Sprite Processor
 */

#include "compiler.h"
#include "cpucore.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "iocoreva.h"
#include "memoryva.h"
#include "machine/timing.h"

enum {
	// TSP command codes.
	CMD_SYNC = 0x10,
	CMD_DSPON = 0x12,
	CMD_DSPOFF = 0x13,
	CMD_DSPDEF = 0x14,
	CMD_CURDEF = 0x15,
	CMD_SPRON = 0x82,
	CMD_SPROFF = 0x83,
	CMD_SPRDEF = 0x84,
	CMD_EXIT = 0x88,

	// TSP status bits.
	STATUS_BUSY = 0x04,
	STATUS_VB = 0x40,

	// paramfunc
	PARAMFUNC_NOP = 0,
	PARAMFUNC_GENERIC,
	PARAMFUNC_SPRDEF_BEGIN,
	PARAMFUNC_SPRDEF,

	// execfunc
	EXECFUNC_SYNC = 0,
	EXECFUNC_DSPON,
	EXECFUNC_DSPDEF,
	EXECFUNC_CURDEF,
	EXECFUNC_SPRON,
};

_TSP tsp;
BOOL tsp_dirty;

static BYTE *getsprinfo(int no) {
	return textmem + tsp.sprtable + no * 8;
}

/*
Apply the decoded sprite-display state.
*/
static void sprsw(int no, BOOL sw) {
	BYTE *sprinfo;
	WORD d;

	sprinfo = getsprinfo(no);
	d = LOADINTELWORD(sprinfo + 0);
	if (sw) {
		d |= 0x0200;
	} else {
		d &= ~0x0200;
	}
	STOREINTELWORD(sprinfo + 0, d);
}

/*
SYNC
*/
static void exec_sync(void) {
	int i;
	UINT16 newlines;
	//BOOL newhsync15khz;

	for (i = 0; i < 14; i++)
		tsp.syncparam[i] = tsp.parambuf[i];
	tsp.textmg = (tsp.syncparam[0] & 0xc0) == 0x80;
	newlines = tsp.syncparam[0x0a] | ((tsp.syncparam[0x0b] & 0x40) << 2);
	//newhsync15khz = tsp.syncparam[0x02] == 0x1c;
	if (newlines != tsp.screenlines /*|| newhsync15khz != tsp.hsync15khz*/) {
		tsp.screenlines = newlines;
		//tsp.hsync15khz = newhsync15khz;
		tsp.flag |= TSP_F_LINESCHANGED;
	}

	//	TRACEOUT(("tsp: sync: textmg=0x%.2x, screenlines=%d, hsync=%s"
	//		, tsp.textmg
	//		, tsp.screenlines
	//		, (tsp.hsync15khz) ? "15KHz" : "24KHz"));
	TRACEOUT(("tsp: sync: textmg=0x%.2x, screenlines=%d", tsp.textmg, tsp.screenlines));

	tsp.status &= ~STATUS_BUSY;
}

/*
DSPON: start TSP display.
*/
static void exec_dspon(void) {
	TRACEOUT(("tsp: dspon: param=0x%.2x, 0x%.2x, 0x%.2x", tsp.parambuf[0], tsp.parambuf[1],
	          tsp.parambuf[2]));

	tsp.texttable = tsp.parambuf[0] << 8;
	tsp.dspon = TRUE;

	tsp.status &= ~STATUS_BUSY;
}

/*
DSPOFF: stop both the text and sprite display controllers.

The generic uPD72022 command has no parameters.  Retrace/timing continues;
only the two TSP display paths are disabled.  Graphics composition is owned
by the VA display circuitry and is intentionally not changed here.
*/
static void exec_dspoff(void) {
	TRACEOUT(("tsp: dspoff"));

	tsp.dspon = FALSE;
	tsp.spron = FALSE;
	tsp.status &= ~STATUS_BUSY;
}

/*
DSPDEF: define screen composition and display format.
*/
static void exec_dspdef(void) {
	TRACEOUT(("tsp: dspdef: param=0x%.2x, 0x%.2x, 0x%.2x, 0x%.2x, 0x%.2x, 0x%.2x", tsp.parambuf[0],
	          tsp.parambuf[1], tsp.parambuf[2], tsp.parambuf[3], tsp.parambuf[4], tsp.parambuf[5]));

	tsp.attroffset = tsp.parambuf[0] + (WORD)tsp.parambuf[1] * 0x100;
	//tsp.pitch = tsp.parambuf[2] >> 4;
	tsp.lineheight = tsp.parambuf[3] + 1;
	tsp.hlinepos = tsp.parambuf[4];
	tsp.blink =
	    tsp.parambuf[5] >> 3; // TODO: the literal field produces an unexpectedly long blink period;
	                          // verify whether Tekumani contains a transcription error.
	tsp.blinkcnt = tsp.blink;

	tsp.status &= ~STATUS_BUSY;
}

/*
CURDEF: define cursor format.
*/
static void exec_curdef(void) {
	TRACEOUT(("tsp: curdef: param=0x%.2x", tsp.parambuf[0]));

	tsp.curn = tsp.parambuf[0] >> 3;
	tsp.be = tsp.parambuf[0] & 0x01;
	sprsw(tsp.curn, tsp.parambuf[0] & 0x02);

	tsp.status &= ~STATUS_BUSY;
}

/*
SPRON: enable sprite display.
*/
static void exec_spron(void) {
	TRACEOUT(("tsp: spron: param=0x%.2x, 0x%.2x, 0x%.2x", tsp.parambuf[0], tsp.parambuf[1],
	          tsp.parambuf[2]));

	tsp.sprtable = tsp.parambuf[0] << 8;
	tsp.hspn = tsp.parambuf[2] >> 3;
	tsp.mg = tsp.parambuf[2] & 0x02;
	tsp.gr = tsp.parambuf[2] & 0x01;
	tsp.spron = TRUE;

	tsp.status &= ~STATUS_BUSY;
}

/*
SPROFF: stop the sprite display controller and its cursor sprite.
*/
static void exec_sproff(void) {
	TRACEOUT(("tsp: sproff"));

	tsp.spron = FALSE;
	tsp.status &= ~STATUS_BUSY;
}

/*
SPRDEF: write a sprite-control-table entry.
*/
// Parameter bytes after the first.
static void paramfunc_sprdef(REG8 dat) {
	BYTE *mem;

	mem = textmem + tsp.sprtable + tsp.sprdef_offset;
	*mem = dat;
	tsp.sprdef_offset++;
}

// First parameter byte.
static void paramfunc_sprdef_begin(REG8 dat) {
	TRACEOUT(("tsp: sprdef: offset=0x%.2x", dat));

	tsp.sprdef_offset = dat;
	//tsp.paramfunc = paramfunc_sprdef;
	tsp.paramfunc = PARAMFUNC_SPRDEF;
}

/*
EXIT: abort command processing.
*/
static void exec_exit(void) {
	TRACEOUT(("tsp: exit"));

	tsp.status &= ~STATUS_BUSY;
}

/*
Undefined command.
*/
static void exec_unknown(void) {
	TRACEOUT(("tsp: unknown cmd: 0x%.2x", tsp.cmd));
	tsp.status &= ~STATUS_BUSY;
}

/*
static void execcmd(void) {
	switch(tsp.cmd) {
	case CMD_DSPON:
		exec_dspon();
		break;
	case CMD_DSPDEF:
		exec_dspdef();
		break;
	case CMD_CURDEF:
		exec_curdef();
		break;
	case CMD_SPRON:
		exec_spron();
		break;
	default:
		TRACEOUT(("tsp: unknown cmd: 0x%.2x", tsp.cmd));
	}
}
*/

/*
static void paramfunc_nop(REG8 dat) {
}
*/

static void paramfunc_generic(REG8 dat) {
	if (tsp.recvdatacnt) {
		//*(tsp.datap++) = dat;
		tsp.parambuf[tsp.paramindex++] = dat;
		if (--tsp.recvdatacnt == 0) {
			//tsp.paramfunc = paramfunc_nop;
			tsp.paramfunc = PARAMFUNC_NOP;
			//tsp.endparamfunc();
			switch (tsp.execfunc) {
			case EXECFUNC_SYNC:
				exec_sync();
				break;
			case EXECFUNC_DSPON:
				exec_dspon();
				break;
			case EXECFUNC_DSPDEF:
				exec_dspdef();
				break;
			case EXECFUNC_CURDEF:
				exec_curdef();
				break;
			case EXECFUNC_SPRON:
				exec_spron();
				break;
			}
		}
	}
}

// ---- I/O

/*
Read the TSP status register.
*/
static REG8 IOINPCALL tsp_i142(UINT port) {
	REG8 dat;

	dat = tsp.status;
	if (tsp.vsync) {
		dat |= STATUS_VB;
	}
	return dat;
}

/*
Unimplemented TSP port.
*/
static REG8 IOINPCALL tsp_i143(UINT port) {
	return 0xff;
}

/*
Write a TSP command.
*/
static void IOOUTCALL tsp_o142(UINT port, REG8 dat) {
	TRACEOUT(("tsp: command: 0x%.2x", dat));

	tsp.cmd = dat;
	//tsp.datap = tsp.parambuf;
	tsp.paramindex = 0;
	tsp.status |= STATUS_BUSY;
	tsp_dirty = TRUE;

	switch (dat) {
	case CMD_SYNC:
		tsp.recvdatacnt = 14;
		tsp.execfunc = EXECFUNC_SYNC;
		tsp.paramfunc = PARAMFUNC_GENERIC;
		break;
	case CMD_DSPON:
		tsp.recvdatacnt = 3;
		//tsp.endparamfunc = exec_dspon;
		tsp.execfunc = EXECFUNC_DSPON;
		//tsp.paramfunc = paramfunc_generic;
		tsp.paramfunc = PARAMFUNC_GENERIC;
		break;
	case CMD_DSPOFF:
		tsp.recvdatacnt = 0;
		tsp.paramfunc = PARAMFUNC_NOP;
		exec_dspoff();
		break;
	case CMD_DSPDEF:
		tsp.recvdatacnt = 6;
		//tsp.endparamfunc = exec_dspdef;
		tsp.execfunc = EXECFUNC_DSPDEF;
		//tsp.paramfunc = paramfunc_generic;
		tsp.paramfunc = PARAMFUNC_GENERIC;
		break;
	case CMD_CURDEF:
		tsp.recvdatacnt = 1;
		//tsp.endparamfunc = exec_curdef;
		tsp.execfunc = EXECFUNC_CURDEF;
		//tsp.paramfunc = paramfunc_generic;
		tsp.paramfunc = PARAMFUNC_GENERIC;
		break;
	case CMD_SPRON:
		tsp.recvdatacnt = 3;
		//tsp.endparamfunc = exec_spron;
		tsp.execfunc = EXECFUNC_SPRON;
		//tsp.paramfunc = paramfunc_generic;
		tsp.paramfunc = PARAMFUNC_GENERIC;
		break;
	case CMD_SPROFF:
		tsp.recvdatacnt = 0;
		tsp.paramfunc = PARAMFUNC_NOP;
		exec_sproff();
		break;
	case CMD_SPRDEF:
		//tsp.paramfunc = paramfunc_sprdef_begin;
		tsp.paramfunc = PARAMFUNC_SPRDEF_BEGIN;
		break;
	case CMD_EXIT:
		//tsp.paramfunc = paramfunc_nop;
		tsp.paramfunc = PARAMFUNC_NOP;
		exec_exit();
		break;
	default:
		//tsp.paramfunc = paramfunc_nop;
		tsp.paramfunc = PARAMFUNC_NOP;
		exec_unknown();
		break;
	}

	//if (tsp.recvdatacnt == 0) execcmd();
}

/*
Write a TSP parameter byte.
*/
static void IOOUTCALL tsp_o146(UINT port, REG8 dat) {
	TRACEOUT(("tsp: parameter: 0x%.2x", dat));
	/*
	if (tsp.recvdatacnt) {
		*(tsp.datap++) = dat;
		if (--tsp.recvdatacnt == 0) execcmd();
	}
*/
	tsp_dirty = TRUE;
	//tsp.paramfunc(dat);
	switch (tsp.paramfunc) {
	case PARAMFUNC_GENERIC:
		paramfunc_generic(dat);
		break;
	case PARAMFUNC_SPRDEF_BEGIN:
		paramfunc_sprdef_begin(dat);
		break;
	case PARAMFUNC_SPRDEF:
		paramfunc_sprdef(dat);
		break;
	case PARAMFUNC_NOP:
	default:
		break;
	}
}

// ---- I/F

void tsp_reset(void) {
	ZeroMemory(&tsp, sizeof(tsp));
	//tsp.paramfunc = paramfunc_nop;
	tsp.paramfunc = PARAMFUNC_NOP;
	tsp_dirty = TRUE;
	/* Marking tsp_dirty on reset wakes the text renderer
						   from its sleep state. */
}

void tsp_bind(void) {
	tsp_updateclock();
	/*
	iocore_attachout(0x152, memctrlva_o152);
	iocore_attachout(0x153, memctrlva_o153);
	iocore_attachout(0x198, memctrlva_o198);
	iocore_attachout(0x19a, memctrlva_o19a);
	*/
	iocore_attachinp(0x142, tsp_i142);
	iocore_attachinp(0x143, tsp_i143);
	iocore_attachout(0x142, tsp_o142);
	iocore_attachout(0x146, tsp_o146);
}

// ----

void tsp_updateclock(void) {
#if 0
	/*
	The exact uninitialized TSP timing is unknown, so use
	the inherited GDC timing calculation as a fallback.
	*/
	UINT hs = 7;		// Horizontal-sync width in character clocks.
	UINT hfp = 9;		// Right-side horizontal blanking in character clocks.
	UINT hbp = 7;		// Left-side horizontal blanking in character clocks.
	UINT vs = 8;		// Vertical-sync width in lines.
	UINT vfp = 7;		// Bottom vertical blanking in lines.
	UINT vbp = 0x19 -3;	// Top vertical blanking in lines.
	UINT lf = 400 +3;	// Active lines per frame.
	UINT cr = 80;		// Active character clocks per line.
	UINT32 clock = 21052600 / 8;	// Display blocks per second.
						// One block is one raster row of one character cell.
	UINT x;
	UINT y;
	UINT cnt;
	UINT32 hclock;

	x = hfp + hbp + hs + cr + 3;	// Total character clocks per line.
	y = vfp + vbp + vs + lf;		// Total lines per frame.

	hclock = clock / x;				// Lines per second.
	cnt = (pccore.baseclock * y) / hclock;	// Frame duration in base clocks.
	cnt *= pccore.multiple;			// Frame duration in CPU clocks.
	tsp.rasterclock = cnt / y;
//	tsp.hsyncclock = (tsp.rasterclock * cr) / x;
	tsp.dispclock = tsp.rasterclock * lf;
	tsp.vsyncclock = cnt - tsp.dispclock;
	timing_setrate(y, hclock);
#else
	UINT lbl, lbr, had, rbr, rbl, hs, tbl, tbr, vad, bbr, bbl, vs;
	UINT w;
	UINT h;
	UINT cnt;
	UINT32 hclock;
	UINT32 clock;           // Pixel clocks per second.
	UINT sysp4displines;    // System-port 4 VRTC: active-display line count.
	UINT sysp4vsyncexlines; // System-port 4 VRTC: line count for which VRTC remains active
	                        // after the TSP vertical-sync interval ends.
	int hsyncmode;
	//UINT vaddefault, haddefault;

	lbl = tsp.syncparam[2] & 0x3f; // Left blanking.
	lbr = tsp.syncparam[3] & 0x3f; // Left border.
	had = tsp.syncparam[4];        // Horizontal active area.
	rbr = tsp.syncparam[5] & 0x3f; // Right border.
	rbl = tsp.syncparam[6] & 0x3f; // Right blanking.
	hs = tsp.syncparam[7] & 0x3f;  // Horizontal-sync interval.
	tbl = tsp.syncparam[8] & 0x3f; // Top blanking.
	tbr = tsp.syncparam[9] & 0x3f; // Top border.
	vad = tsp.syncparam[10] + ((tsp.syncparam[11] & 0x40) << 2);
	// Vertical active area.
	bbr = tsp.syncparam[11] & 0x3f; // Bottom border.
	bbl = tsp.syncparam[12] & 0x3f; // Bottom blanking.
	vs = tsp.syncparam[13] & 0x3f;  // Vertical-sync interval.

	if (vad == 0 && had == 0) {
		// SYNC parameters have not been initialized.
		// Provide usable display and vertical-sync clocks before the guest
		// programs SYNC parameters.
		// Use a 24.8 kHz, 400-line fallback timing.
		lbl = 0x10;
		lbr = 0;
		had = 0x9f;
		rbl = 0x10;
		rbr = 0;
		hs = 0x0f;
		tbl = 0x19;
		tbr = 0;
		vad = 0x190;
		bbl = 0x07;
		bbr = 0;
		vs = 8;
		hsyncmode = VIDEOVA_24_8KHZ;
	} else {
		hsyncmode = videova_hsyncmode();
	}

	switch (hsyncmode) {
	case VIDEOVA_24_8KHZ:
		clock = 20854022;
		sysp4displines = 402;
		sysp4vsyncexlines = 25;
		//vaddefault = 0x190;
		break;
	case VIDEOVA_15_73KHZ:
		//clock = 14219920;			// 15.73KHz
		clock = 14252364; // Adjusted to approximate the measured 15.766 kHz line rate.
		sysp4displines = 202;
		sysp4vsyncexlines = 36;
		//vaddefault = 0xc8;
		break;
	case VIDEOVA_15_98KHZ:
		clock = 14189837;
		sysp4displines = 202;
		sysp4vsyncexlines = 37;
		//vaddefault = 0xc8;
		break;
	}

	if (vs < 4)
		vs = 4;
	if (vad < 4)
		vad = 4;
	had |= 1;
	if (hs < 4)
		hs = 4;
	if (lbl < 3)
		lbl = 3;

	w = (lbl + 1 + lbr + had + 1 + rbr + rbl + 1 + hs + 1) * 4;
	// Total pixel clocks per line.
	// Unlike the uPD72022 data-sheet description, observed hardware
	// appears not to add one to LBR and RBR.

	h = tbl + tbr + vad + bbr + bbl + vs;
	// Total lines per frame.

	if (sysp4displines + sysp4vsyncexlines >= h) {
		// The original VA bootstrap programs 15.98 kHz/200-line SYNC values
		// even for 24.8 kHz output.  That can make sysp4displines exceed
		// the total line count and leave port 040H VRTC permanently clear;
		// clamp the derived interval to avoid that state.
		sysp4displines = h - sysp4vsyncexlines - 4;
	}

	hclock = clock / w; // Lines per second.
	cnt = (pccore.baseclock * h) / hclock;
	// Frame duration in base clocks.
	cnt *= pccore.multiple; // Frame duration in CPU clocks.
	tsp.rasterclock = cnt / h;
	//tsp.dispclock = tsp.rasterclock * (tbl + tbr + vad);
	//tsp.vsyncclock = cnt - tsp.dispclock;
	tsp.vsyncclock = cnt * (bbr + bbl + vs) / h;
	tsp.dispclock = cnt - tsp.vsyncclock;
	timing_setrate(h, hclock);

	//	tsp.sysp4vsyncextension = tsp.rasterclock * sysp4vsyncexlines;
	//	tsp.sysp4dispclock = tsp.rasterclock * sysp4displines;
	tsp.sysp4vsyncextension = cnt * sysp4vsyncexlines / h;
	tsp.sysp4dispclock = cnt * sysp4displines / h;

#endif
}
