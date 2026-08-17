#include "compiler.h"
#include "strres.h"
#include "dosio.h"
#include "soundmng.h"
#include "sysmng.h"
#include "timemng.h"
#include "cpucore.h"
#include "machine/pccore.h"
#include "iocore.h"
#include "cbuscore.h"
#include "mpu98ii.h"
#include "romva.h"
#include "biosmem.h"
#include "scrndraw.h"
#include "sound.h"
#include "fmboard.h"
#include "beep.h"
#include "font.h"
#include "diskdrv.h"
#include "fddfile.h"
#include "fdd_mtr.h"
#include "sxsi.h"
#include "np2ver.h"
#include "machine/calendar.h"
#include "machine/timing.h"
#include "machine/keystat.h"
#include "machine/debugsub.h"
#include "upd9002_diagnostic.h"
#include "diagnostics/upd9002_debug.h"

#include "bmsio.h"
#include "emsio.h"

#include "../vram/maketextva.h"
#include "../vram/makesprva.h"
#include "../vram/makegrphva.h"
#include "scrnmng.h"
#include "../vram/scrndrawva.h"
#include "memoryva.h"
#include "tsp.h"
#include "sgp.h"
#include "videova.h"
#include "subsystemmx.h"
#include "va91.h"

const OEMCHAR np2version[] = OEMTEXT(NP2VER_CORE);

#define PCBASEMULTIPLE PCCORE_STANDARD_MULTIPLE

NP2CFG np2cfg = {0,
                 1,
                 0,
                 32,
                 0,
                 0,
                 0x40,
                 0,
                 0,
                 0,
                 0,
                 {0x3e, 0x73, 0x7b},
                 0,
                 0,
                 0,
                 {1, 1, 6, 1, 8, 1},
                 OEMTEXT("88VA2"),
                 PCBASECLOCK40,
                 2,
                 0,
                 1,
                 {0x48, 0x05, 0x04, 0x00, 0x01, 0x00, 0x00, 0x6e},
                 1,
                 EMSIO_DEFAULT_MEGABYTES,
                 2,
                 1,
                 0x000000,
                 0xffffff,
                 22050,
                 500,
                 FMBOARD_VA_OPNA,
                 0,
                 {0, 0, 0},
                 0xd1,
                 0x7f,
                 0xd1,
                 0,
                 0,
                 1,
                 3,
                 {0x0c, 0x0c, 0x08, 0x06, 0x03, 0x0c},
                 64,
                 64,
                 64,
                 64,
                 64,
                 1,
                 0x82,
                 3,
                 1,
                 80,
                 0,
                 0,
                 0,
                 {OEMTEXT(""), OEMTEXT("")},
                 {OEMTEXT(""), OEMTEXT(""), OEMTEXT(""), OEMTEXT("")},
                 OEMTEXT(""),
                 OEMTEXT(""),
                 OEMTEXT("")};

PCCORE pccore = {PCBASECLOCK25,
                 PCBASEMULTIPLE,
                 0,
                 PCMODEL_VA,
                 0,
                 0,
                 {0x3e, 0x73, 0x7b},
                 0,
                 0,
                 0,
                 PCBASECLOCK25 *PCBASEMULTIPLE};
CLOCKSCALE pccore_cpu_scale = {PCCORE_STANDARD_MULTIPLE, PCCORE_STANDARD_MULTIPLE, 0};
static UINT pccore_cpu_multiple_value = PCCORE_STANDARD_MULTIPLE;

UINT8 screenupdate = 3; // Bit 0 requests a partial redraw.
                        // Bit 1 requests a full redraw.
int screendispflag = 1;
int soundrenewal = 0;
BOOL drawframe;
UINT drawcount = 0;
BOOL hardwarereset = FALSE;
static BOOL pccore_debug_resume = FALSE;

// ---------------------------------------------------------------------------

BOOL pccore_cpu_multiple_valid(UINT multiple) {
	return ((multiple >= 1) && (multiple <= PCCORE_CPU_MULTIPLE_MAX));
}

void pccore_clockrestore(void) {
	UINT multiple;

	multiple = np2cfg.multiple;
	if (multiple == 0) {
		multiple = 1;
	} else if (multiple > PCCORE_CPU_MULTIPLE_MAX) {
		multiple = PCCORE_CPU_MULTIPLE_MAX;
	}
	pccore.multiple = PCCORE_STANDARD_MULTIPLE;
	pccore.realclock = pccore.baseclock * pccore.multiple;
	pccore_cpu_multiple_value = multiple;
	clockscale_configure(&pccore_cpu_scale, PCCORE_STANDARD_MULTIPLE, multiple);
}

UINT pccore_cpu_multiple(void) {
	return (pccore_cpu_multiple_value);
}

UINT32 pccore_cpu_clock(void) {
	return (pccore.baseclock * pccore_cpu_multiple_value);
}

// ---------------------------------------------------------------------------

void getbiospath(OEMCHAR *path, const OEMCHAR *fname, int maxlen) {
	const OEMCHAR *p;

	p = np2cfg.biospath;
	if (p[0]) {
		file_cpyname(path, p, maxlen);
		file_setseparator(path, maxlen);
		file_catname(path, fname, maxlen);
	} else {
		file_cpyname(path, file_getcd(fname), maxlen);
	}
}

// ----

static void pccore_set(void) {
	UINT8 model;
	UINT8 extsize;

	ZeroMemory(&pccore, sizeof(pccore));
	model = PCMODEL_VA;

	/* The active tree has only PC-88VA models.  Unknown legacy values
	 * deliberately fall back to the VA2 configuration. */
	pccore.model_va = PCMODEL_VA2;
	if (!milstr_cmp(np2cfg.model, str_VA1)) {
		pccore.model_va = PCMODEL_VA1;
	}
	pccore.model = model;

	if (np2cfg.baseclock >= ((PCBASECLOCK40 + PCBASECLOCK25) / 2)) {
		pccore.baseclock = PCBASECLOCK40; // 4.0MHz
		pccore.cpumode = CPUMODE_BASE4MHZ;
	} else if (np2cfg.baseclock >= ((PCBASECLOCK25 + PCBASECLOCK20) / 2)) {
		pccore.baseclock = PCBASECLOCK25; // 2.5MHz
		pccore.cpumode = 0;
	} else {
		pccore.baseclock = PCBASECLOCK20; // 2.0MHz
		pccore.cpumode = CPUMODE_8MHZ;
	}
	pccore_clockrestore();

	// Configure VA expansion memory.
	extsize = 0;
	if (!(np2cfg.dipsw[2] & 0x80)) {
		extsize = min(np2cfg.EXTMEM, EMSIO_MAX_MEGABYTES);
	}
	pccore.extmem = extsize;
	CopyMemory(pccore.dipsw, np2cfg.dipsw, 3);

	// Select the installed sound board.
	pccore.sound = np2cfg.SOUND_SW;

	// Select other supported expansion-bus devices.
	pccore.device = 0;
	if (np2cfg.mpuenable) {
		pccore.device |= PCCBUS_MPU98;
	}
}

// --------------------------------------------------------------------------

static void sound_init(void) {
	UINT rate;

	rate = np2cfg.samplingrate;
	if ((rate != 11025) && (rate != 22050) && (rate != 44100)) {
		rate = 0;
	}
	sound_create(rate, np2cfg.delayms);
	fddmtrsnd_initialize(rate);
	beep_initialize(rate);
	beep_setvol(np2cfg.BEEP_VOL);
	opngen_initialize(rate);
	opngen_setvol(np2cfg.vol_fm);
	psggen_initialize(rate);
	psggen_setvol(np2cfg.vol_ssg);
	rhythm_initialize(rate);
	rhythm_setvol(np2cfg.vol_rhythm);
	adpcm_initialize(rate);
	adpcm_setvol(np2cfg.vol_adpcm);
}

static void sound_term(void) {
	soundmng_stop();
	rhythm_deinitialize();
	beep_deinitialize();
	fddmtrsnd_deinitialize();
	sound_destroy();
}

void pccore_init(void) {
	CPU_INITIALIZE();

	// VA rendering owns its palette and raster conversion.
	maketextva_initialize();
	makesprva_initialize();
	makegrphva_initialize();
	subsystemmx_initialize();
	sxsi_initialize();

	font_initialize();
	font_load(np2cfg.fontfile, TRUE);
	fddfile_initialize();

	sound_init();

	rs232c_construct();
	mpu98ii_construct();

	iocore_create();
}

void pccore_term(void) {
	sound_term();

	fdd_eject(0);
	fdd_eject(1);
	fdd_eject(2);
	fdd_eject(3);

	iocore_destroy();

	mpu98ii_destruct();
	rs232c_destruct();

	sxsi_trash();

	CPU_DEINITIALIZE();
}

void pccore_cfgupdate(void) {
	BOOL renewal;
	int i;

	renewal = FALSE;
	for (i = 0; i < 8; i++) {
		if (np2cfg.memsw[i] != mem[MEMX_MSW + i * 4]) {
			np2cfg.memsw[i] = mem[MEMX_MSW + i * 4];
			renewal = TRUE;
		}
	}
	{
		UINT8 val;

		val = keystat_getlockedkey();
		if (np2cfg.lockedkey != val) {
			np2cfg.lockedkey = val;
			renewal = TRUE;
		}
	}
	if (renewal) {
		sysmng_update(SYS_UPDATECFG);
	}
}

void pccore_reset(void) {
	int i;

	pccore_debug_resume = FALSE;
	drawcount = 0;
	scrnmng_reset_metrics();
	soundmng_stop();
	if (soundrenewal) {
		soundrenewal = 0;
		sound_term();
		sound_init();
	}
	ZeroMemory(mem, 0x110000);
	ZeroMemory(mem + VRAM1_B, 0x18000);
	ZeroMemory(mem + VRAM1_E, 0x08000);
	ZeroMemory(mem + FONT_ADRS, 0x08000);

	// Copy configured memory-switch bytes into the VA work area.
	for (i = 0; i < 8; i++) {
		mem[0xa3fe2 + i * 4] = np2cfg.memsw[i];
	}

	pccore_set();
	sgp_configure_speed();
	bmsio_set();
	keystat_setlockedkey(np2cfg.lockedkey);
	nevent_allreset();

	// Reset the CPU after deriving the VA model and expansion-memory size.

	CPU_RESET();
	CPU_SETEXTSIZE((UINT32)pccore.extmem);

	// Open configured SASI and SCSI media.
	sxsi_open();
	if (sxsi_issasi()) {
		pccore.hddif |= PCHDD_SASI;
		TRACEOUT(("supported SASI"));
	}
	if (sxsi_isscsi()) {
		pccore.hddif |= PCHDD_SCSI;
		TRACEOUT(("supported SCSI"));
	}

	sound_changeclock();
	beep_changeclock();
	sound_reset();
	fddmtrsnd_bind();

	fddfile_reset2dmode();

	iocore_reset(); // Sound-board reset calls the native PIC interface.
	cbuscore_reset();
	fmboard_reset(pccore.sound);

	upd9002_memorymap_va();
	iocore_build();
	iocore_bind();
	cbuscore_bind();
	fmboard_bind();

	fddmtr_initialize();
	calendar_initialize();

	romva_initialize();
	va91_initialize();
	CS_BASE = 0xf0000;
	CPU_CS = 0xf000;
	CPU_IP = 0xfff0;

	CPU_CLEARPREFETCH();
	sysmng_cpureset();

	timing_reset();
	soundmng_play();
}

static void drawscreenva(void) {
	int y;
	BOOL text200;
	BOOL grph200;
	UINT16 lines;

	//	if (videova.grmode & 0x1000) {
	//		// SYNCEN: horizontal-sync output enable.
	tsp_updateclock();
	//	}

	if (!drawframe) {
		return;
	}

	lines = tsp.screenlines;
	if (videova_hsyncmode() != VIDEOVA_24_8KHZ)
		lines *= 2;
	if (lines > SURFACE_HEIGHT)
		lines = SURFACE_HEIGHT;

	if ((tsp.flag & TSP_F_LINESCHANGED) && (videova.grmode & 0x1000)) {
		// Apply a TSP line-count change only while SYNCEN is enabled.
		scrnmng_setheight(0, lines);
		tsp.flag &= ~TSP_F_LINESCHANGED;
	}

	maketextva_begin(&text200);
	makesprva_begin();
	makegrphva_begin(&grph200);
	scrndrawva_compose_begin();

	if (videova_hsyncmode() != VIDEOVA_24_8KHZ) {
		// 15.7 kHz output.
		switch (videova.grmode & 0x00c0) {
		case 0x00: // Non-interlaced mode 0.
		case 0x40: // Non-interlaced mode 1.
			for (y = 0; y < lines /*SURFACE_HEIGHT*/;) {
				// Even output raster.
				maketextva_raster();
				makesprva_raster();
				makegrphva_raster();
				scrndrawva_compose_raster();
				y++;
				// Odd output raster.
				maketextva_blankraster();
				makesprva_blankraster();
				makegrphva_blankraster();
				scrndrawva_compose_raster();
				y++;
			}
			break;

		case 0x80: // Interlaced mode 0.
			for (y = 0; y < lines /*SURFACE_HEIGHT*/;) {
				// Even output raster.
				maketextva_raster();
				makesprva_raster();
				makegrphva_raster();
				scrndrawva_compose_raster();
				y++;
				// Odd output raster; reuse the previous raster where noted.
				if (text200) {
					// Reuse the previous output raster.
				} else {
					maketextva_raster();
				}
				scrndrawva_compose_raster();
				y++;
			}
			break;
		case 0xc0: // Interlaced mode 1.
			for (y = 0; y < lines /*SURFACE_HEIGHT*/;) {
				// Even output raster.
				maketextva_raster();
				makesprva_raster();
				makegrphva_raster();
				scrndrawva_compose_raster();
				y++;
				// Odd output raster.
				if (text200) {
					// Reuse the previous output raster.
				} else {
					maketextva_raster();
				}
				// Sprites are always 200-line and retain the previous output raster.
				if (grph200) {
					// Reuse the previous output raster.
				} else {
					makegrphva_raster();
				}
				scrndrawva_compose_raster();
				y++;
			}
			break;
		}
	} else {
		// 24.8 kHz output.
		switch (videova.grmode & 0x00c0) {
		case 0x00: // Non-interlaced mode 0.
			for (y = 0; y < lines /*SURFACE_HEIGHT*/;) {
				// Even output raster.
				maketextva_raster();
				makesprva_raster();
				makegrphva_raster();
				scrndrawva_compose_raster();
				y++;
				// Odd output raster.
				maketextva_raster();
				makesprva_raster();
				if (grph200) {
					makegrphva_blankraster();
				} else {
					makegrphva_raster();
				}
				scrndrawva_compose_raster();
				y++;
			}
			break;
		case 0x40: // Non-interlaced mode 1.
			for (y = 0; y < lines /*SURFACE_HEIGHT*/;) {
				// Even output raster.
				maketextva_raster();
				makesprva_raster();
				makegrphva_raster();
				scrndrawva_compose_raster();
				y++;
				// Odd output raster.
				maketextva_raster();
				makesprva_raster();
				if (grph200) {
					// Reuse the previous output raster.
				} else {
					makegrphva_raster();
				}
				scrndrawva_compose_raster();
				y++;
			}
			break;
		case 0x80: // Interlaced mode 0.
		case 0xc0: // Interlaced mode 1.
			// This mode combination is prohibited.
			for (y = 0; y < lines /*SURFACE_HEIGHT*/;) {
				maketextva_blankraster();
				makesprva_blankraster();
				makegrphva_blankraster();
				scrndrawva_compose_raster();
				y++;
				scrndrawva_compose_raster();
				y++;
			}
			break;
		}
	}

	screenupdate |= 2; // The VA renderer currently implements full redraws only.

	if (screenupdate) {
		screenupdate = scrndrawva_draw((BYTE)(screenupdate & 2));
		drawcount++;
	}
}

static void screendispva_setnevent() {
	nevent_set(NEVENT_FLAMES, tsp.dispclock, screenvsyncva, NEVENT_RELATIVE);
	nevent_set(NEVENT_FLAMES2, tsp.sysp4vsyncextension, sysp4vsyncend, NEVENT_RELATIVE);
}

// Begin the active-display period.
void screendispva(NEVENTITEM item) {
	/*
	PICITEM		pi;
*/
	tsp.vsync = 0;
	/*	Moved to sysp4vsyncstart.
	screendispflag = 0;
*/
	drawscreenva();
	/*
	pi = &pic.pi[0];
	if (pi->irr & PIC_CRTV) {
		pi->irr &= ~PIC_CRTV;
	}
*/
	screendispva_setnevent();
}

// Begin the vertical-retrace period.
void screenvsyncva(NEVENTITEM item) {
	//	tsp.vsync = 0x20;
	tsp.vsync = 0x40;

	videova.blinkcnt++;
	if (--tsp.blinkcnt == 0) {
		tsp.blinkcnt = tsp.blink;
		tsp.blinkcnt2++;
	}
#if 0
	nevent_set(NEVENT_FLAMES, tsp.vsyncclock, screendispva, NEVENT_RELATIVE);
#else
	/*
	// Delay the interrupt by six clocks.
	// Software may observe VRTC at port 040H before the interrupt;
	// this ordering avoids a hang in Saishuu Heiki UPO.
	nevent_set(NEVENT_FLAMES, 6, screenvsyncva2, NEVENT_RELATIVE);
*/
	nevent_set(NEVENT_FLAMES, tsp.vsyncclock, screendispva, NEVENT_RELATIVE);
#endif
}

#if 1
/*
void screenvsyncva2(NEVENTITEM item) {
	pic_setirq(2);
	nevent_set(NEVENT_FLAMES, tsp.vsyncclock - 6, screendispva, NEVENT_RELATIVE);

}
*/
#endif

void sysp4vsyncint(NEVENTITEM item) {
	pic_setirq(2);
}

void sysp4vsyncstart(NEVENTITEM item) {
	tsp.sysp4vsync = 0x20;

	// Delay the interrupt by six clocks.
	// Software may observe VRTC at port 040H before the interrupt;
	// this ordering avoids a hang in Saishuu Heiki UPO.
	nevent_set(NEVENT_FLAMES2, 6, sysp4vsyncint, NEVENT_RELATIVE);

	screendispflag = 0;
}

void sysp4vsyncend(NEVENTITEM item) {
	PICITEM pi;

	tsp.sysp4vsync = 0;

	pi = &pic.pi[0];
	if (pi->irr & PIC_CRTV) {
		pi->irr &= ~PIC_CRTV;
	}

	nevent_set(NEVENT_FLAMES2, tsp.sysp4dispclock, sysp4vsyncstart, NEVENT_RELATIVE);
}

// ---------------------------------------------------------------------------

//@@@@@@

// Debug breakpoints.
typedef struct {
	BOOL enabled;
	UINT16 seg;
	UINT16 off;
} BREAKADDR;

enum {
	BREAKADDR_MAX = 16,
};

BOOL stopexec = FALSE;      // Stop execution.
BOOL singlestep = FALSE;    // Execute one instruction.
BOOL breakpointflag = TRUE; //FALSE;				// Enable breakpoint checks.
BREAKADDR breakaddrx[BREAKADDR_MAX] = {
    // Debug breakpoints.
    {FALSE, 0xe000, 0x9213},
    {FALSE, 0xe000, 0xb577},
};

void pccore_debugmem(UINT32 op, UINT32 addr, UINT16 data) {
	/*
	int	x = 0;

    if (addr == 0x2d830+0xb955) {
		x=op+addr+data;
	}
*/
	(void)op;
	(void)addr;
	(void)data;
}

void pccore_debugint(UINT32 no) {
	if (no != 0x82 && !(no == 0x83 && CPU_AX == 0x2e00) && no != 0x96) {
		TRACEOUT((
		    "cpu: int 0x%02x %04x:%04x rom0=%02x AX=%04x BX=%04x CX=%04x DX=%04x SI=%04x DI=%04x BP=%04x SP=%04x DS=%04x ES=%04x SS=%04x",
		    no, CPU_CS, CPU_IP, memoryva.rom0_bank, CPU_AX, CPU_BX, CPU_CX, CPU_DX, CPU_SI, CPU_DI,
		    CPU_BP, CPU_SP, CPU_DS, CPU_ES, CPU_SS));
	}
	/*
	if (no == 0x8b && CPU_AH == 0x17) {
		int i;
		TRACEOUT(("Music BIOS (17h): CH=%x, DL=%x, ES:BP=%.4x:%.4x",CPU_CH, CPU_DL, CPU_ES, CPU_BP));
		for (i = 0; i < CPU_DL; i++) {
			TRACEOUT(("  %.2x",mem[(CPU_ES << 4) + CPU_BP + i]));
		}
	}
	*/
}

//@@@@@@

#if defined(USEIPTRACE) // Shinra
#define IPTRACE (1 << 12)
#endif

#if defined(TRACE) && IPTRACE
static UINT trpos = 0;
static UINT32 treip[IPTRACE];
static BYTE trerom0bank[IPTRACE];
static WORD tredata1[IPTRACE];

int treafter = 0; // Shinra

void iptrace_out(void) {
	FILEH fh;
	UINT s;
	UINT32 eip;
	char buf[32];

	s = trpos;
	if (s > IPTRACE) {
		s -= IPTRACE;
	} else {
		s = 0;
	}
	fh = file_create_c("his.txt");
	while (s < trpos) {
		BYTE bank = trerom0bank[s & (IPTRACE - 1)];
		eip = treip[s & (IPTRACE - 1)];
		//		SPRINTF(buf, "%.4x:%.4x (rom0=%.2x)\r\n", (eip >> 16), eip & 0xffff, bank);
		SPRINTF(buf, "%.4x:%.4x (rom0=%.2x) ES=%.4x\r\n", (eip >> 16), eip & 0xffff, bank,
		        tredata1[s & (IPTRACE - 1)]);
		s++;
		file_write(fh, buf, strlen(buf));
	}
	file_close(fh);
}
#endif

#if defined(TRACE)
static int resetcnt = 0;
static int execcnt = 0;
int piccnt = 0;
int tr = 0;
UINT cflg;
#endif

void pccore_postevent(UINT32 event) { // yet!

	(void)event;
}

static void pccore_process_cpu_reset_request(void) {
	if (CPU_RESETREQ) {
		CPU_RESETREQ = 0;
		CPU_SHUT();
	}
}

#if defined(VAEG_UPD9002_M42_TESTING)
void upd9002_m42_process_cpu_reset_request(void) {
	pccore_process_cpu_reset_request();
}
#endif

void pccore_exec(BOOL draw) {
	if (upd9002_diagnostic_pending() || upd9002_debug_event_pending()) {
		return;
	}
	if (pccore_debug_resume) {
		pccore_debug_resume = FALSE;
		goto debug_resume_cpu;
	}

	drawframe = draw;
	//	keystat_sync();
	soundmng_sync();
	mouseif_sync();

	screendispflag = 1;
	if (!nevent_iswork(NEVENT_FLAMES)) {
		screendispva_setnevent();
	}

	while (screendispflag) {
#if defined(TRACE)
		resetcnt++;
#endif
		pic_irq();
		pccore_process_cpu_reset_request();

		while (CPU_REMCLOCK > 0) {
		debug_resume_cpu:
			if (upd9002_debug_step_begin()) {
				pccore_debug_resume = TRUE;
				return;
			}
#if defined(TRACE) && IPTRACE
			treip[trpos & (IPTRACE - 1)] = (CPU_CS << 16) + CPU_IP;
			trerom0bank[trpos & (IPTRACE - 1)] = memoryva.rom0_bank;
			tredata1[trpos & (IPTRACE - 1)] = CPU_ES;
			trpos++;
#endif
			//@@@@@@
			//@@@@@@

#if defined(TRACE) && defined(IPTRACE) // Shinra
			if (treafter) {
				if (treafter < 0) {
					iptrace_out();
					treafter = 0;
				} else {
					if (--treafter == 0) {
						iptrace_out();
					}
				}
			}
#endif

			//TRACEOUT(("%.4x:%.4x", CPU_CS, CPU_IP));
			upd9002_core_step();
			if (upd9002_diagnostic_pending()) {
				return;
			}
			subsystemmx_exec();
			sgp_step();
		}

		nevent_progress();
	}
	mpu98ii_callback();
	diskdrv_callback();
	calendar_inc();
	sound_sync(); // happy!

	if (hardwarereset) {
		hardwarereset = FALSE;
		pccore_cfgupdate();
		pccore_reset();
	}

#if defined(TRACE)
	execcnt++;
	if (execcnt >= 60) {
		//		TRACEOUT(("resetcnt = %d / pic %d", resetcnt, piccnt));
		execcnt = 0;
		resetcnt = 0;
		piccnt = 0;
	}
#endif
}

void pccore_redraw(void) {
	BOOL saved_drawframe;

	saved_drawframe = drawframe;
	drawframe = TRUE;
	drawscreenva();
	drawframe = saved_drawframe;
}
