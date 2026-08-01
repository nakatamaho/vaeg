#include	"compiler.h"

#include	"dosio.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"cbuscore.h"
#include	"scsiio.h"
#include	"scsiio.tbl"
#include	"scsicmd.h"

#include	"iocoreva.h"


	_SCSIIO		scsiio;

static const UINT8 scsiirq[] = {0x03, 0x05, 0x06, 0x09, 0x0c, 0x0d, 3, 3};
static BOOL scsi_trace_enabled;
static BOOL scsi_csr_latched;
static BOOL scsi_csr_event_active;
static REG8 scsi_csr_event_status;
static BOOL scsi_csr_pending;
static REG8 scsi_csr_pending_status;

static void scsi_tracef(const char *fmt, ...) {

	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputc('\n', stderr);
}

#define SCSITRACEOUT(arg) do { \
	if (scsi_trace_enabled) { \
		scsi_tracef arg; \
	} \
} while (0)

void scsiio_trace_enable(BOOL enabled) {

	scsi_trace_enabled = enabled;
}

void scsiio_trace_pic_irq(REG8 irq, BOOL asserted) {

	if (scsi_trace_enabled &&
		(irq == scsiirq[(scsiio.resent >> 3) & 7])) {
		SCSITRACEOUT(("scsitrace irq-%s line=%u cs=%04x ip=%04x",
				asserted ? "assert" : "clear", irq, CPU_CS, CPU_IP));
	}
}


void scsiioint(NEVENTITEM item) {

	scsi_csr_event_active = FALSE;
	scsiio.scsistatus = scsi_csr_event_status;
	scsi_csr_latched = TRUE;
	SCSITRACEOUT(("scsitrace event irq=%u cs=%04x ip=%04x aux=%02x "
				"status=%02x phase=%02x membank=%02x",
				scsiirq[(scsiio.resent >> 3) & 7], CPU_CS, CPU_IP,
				scsiio.auxstatus, scsiio.scsistatus, scsiio.phase,
				scsiio.membank));
	TRACEOUT(("scsiioint"));
	if (scsiio.membank & 4) {
		pic_setirq(scsiirq[(scsiio.resent >> 3) & 7]);
		TRACEOUT(("scsi intr"));
	}
	scsiio.auxstatus = 0x80;
	(void)item;

}


static void scsiintr(REG8 status) {

	if (!scsi_csr_event_active && !scsi_csr_latched) {
		scsi_csr_event_active = TRUE;
		scsi_csr_event_status = status;
		nevent_set(NEVENT_SCSIIO, 4000, scsiioint, NEVENT_ABSOLUTE);
	}
	else if (!scsi_csr_pending) {
		scsi_csr_pending = TRUE;
		scsi_csr_pending_status = status;
	}
	else {
		/* The bus layer must back-pressure before reaching this case. */
		return;
	}
	SCSITRACEOUT(("scsitrace request status=%02x phase=%02x cs=%04x ip=%04x",
			status, scsiio.phase, CPU_CS, CPU_IP));
	TRACEOUT(("scsi schedule intr"));
}




static void scsicmd(REG8 cmd) {

	REG8	ret;
	UINT8	id;

	id = scsiio.reg[SCSICTR_DSTID] & 7;
	switch(cmd) {
		case SCSICMD_RESET:
			scsiio.phase = 0;
			scsiio.cmdpos = 0;
			scsiio.rddatpos = 0;
			scsiio.wrdatpos = 0;
			scsiintr(SCSISTAT_RESET);
			break;

		case SCSICMD_NEGATE:
			ret = scsicmd_negate(id);
			scsiintr(ret);
			break;

		case SCSICMD_SEL:
			ret = scsicmd_select(id);
			if (ret & 0x80) {
				scsiintr(0x11);
			}
			else {
				scsiintr(ret);
			}
			break;

		case SCSICMD_SEL_TR:
			ret = scsicmd_transfer(id, scsiio.reg + SCSICTR_CDB);
			if (ret != 0xff) {
				scsiintr(ret);
			}
			break;

		case SCSICMD_TRANS_INFO:
			ret = scsicmd_transinfo(id);
			scsiintr(ret);
			break;

	}
}




// ----

static void IOOUTCALL scsiio_occ0(UINT port, REG8 dat) {

	scsiio.port = dat;
	SCSITRACEOUT(("scsitrace out port=0cc0 ar=%02x cs=%04x ip=%04x",
			dat, CPU_CS, CPU_IP));
	(void)port;
}

static void IOOUTCALL scsiio_occ2(UINT port, REG8 dat) {

	UINT8	bit;

	if (scsiio.port < 0x40) {
		SCSITRACEOUT(("scsitrace out port=0cc2 ar=%02x data=%02x cs=%04x ip=%04x",
				scsiio.port, dat, CPU_CS, CPU_IP));
		TRACEOUT(("scsi ctrl write %s(%.2x) %.2x", scsictr[scsiio.port], scsiio.port, dat));
	}
	if (scsiio.port <= 0x19) {
		scsiio.reg[scsiio.port] = dat;
		if (scsiio.port == SCSICTR_CMD) {
			scsicmd(dat);
		}
	scsiio.port++;
	}
	else {
		SCSITRACEOUT(("scsitrace out port=0cc2 ar=%02x data=%02x cs=%04x ip=%04x",
				scsiio.port, dat, CPU_CS, CPU_IP));
		switch(scsiio.port) {
			case SCSICTR_MEMBANK:
				scsiio.membank = dat;
				break;

			case 0x3f:
				bit = 1 << (dat & 7);
				if (dat & 8) {
					scsiio.datmap |= bit;
				}
				else {
					if (scsiio.datmap & bit) {
						scsiio.datmap &= ~bit;
						if (bit == (1 << 1)) {
							scsiio.wrdatpos = 0;
						}
						else if (bit == (1 << 5)) {
							scsiio.rddatpos = 0;
						}
					}
				}
				break;
		}
	}
	(void)port;
}

static void IOOUTCALL scsiio_occ4(UINT port, REG8 dat) {

	SCSITRACEOUT(("scsitrace out port=0cc4 data=%02x cs=%04x ip=%04x",
			dat, CPU_CS, CPU_IP));
	TRACEOUT(("scsiio_occ4 %.2x", dat));
	(void)port;
	(void)dat;
}

static void IOOUTCALL scsiio_occ6(UINT port, REG8 dat) {

	SCSITRACEOUT(("scsitrace out port=0cc6 data=%02x ar=%02x cs=%04x ip=%04x",
			dat, scsiio.port, CPU_CS, CPU_IP));
	scsiio.data[scsiio.wrdatpos & 0x7fff] = dat;
	scsiio.wrdatpos++;
	if ((scsiio.phase == SCSIPH_DATAOUT) &&
		(scsiio.wrdatpos >= scsiio.cmdpos)) {
		scsiio.phase = SCSIPH_STATUS;
		scsiintr(0x8b);
	}
	(void)port;
}

static REG8 IOINPCALL scsiio_icc0(UINT port) {

	REG8	ret;

	ret = scsiio.auxstatus;
	SCSITRACEOUT(("scsitrace in port=0cc0 aux=%02x ar=%02x cs=%04x ip=%04x",
			ret, scsiio.port, CPU_CS, CPU_IP));
	scsiio.auxstatus = 0;
	(void)port;
	return(ret);
}

static REG8 IOINPCALL scsiio_icc2(UINT port) {

	REG8	ret;

	switch(scsiio.port) {
		case SCSICTR_STATUS:
			if (scsi_csr_latched) {
				scsi_csr_latched = FALSE;
				if (scsi_csr_pending) {
					scsi_csr_event_active = TRUE;
					scsi_csr_event_status = scsi_csr_pending_status;
					scsi_csr_pending = FALSE;
					nevent_set(NEVENT_SCSIIO, 4000, scsiioint,
							NEVENT_ABSOLUTE);
				}
			}
			SCSITRACEOUT(("scsitrace in port=0cc2 ar=%02x status=%02x cs=%04x ip=%04x",
					scsiio.port, scsiio.scsistatus, CPU_CS, CPU_IP));
			scsiio.port++;
			return(scsiio.scsistatus);

		case SCSICTR_MEMBANK:
			SCSITRACEOUT(("scsitrace in port=0cc2 ar=%02x data=%02x cs=%04x ip=%04x",
					scsiio.port, scsiio.membank, CPU_CS, CPU_IP));
			return(scsiio.membank);

		case SCSICTR_MEMWND:
			SCSITRACEOUT(("scsitrace in port=0cc2 ar=%02x data=%02x cs=%04x ip=%04x",
					scsiio.port, scsiio.memwnd, CPU_CS, CPU_IP));
			return(scsiio.memwnd);

		case SCSICTR_RESENT:
			SCSITRACEOUT(("scsitrace in port=0cc2 ar=%02x data=%02x cs=%04x ip=%04x",
					scsiio.port, scsiio.resent, CPU_CS, CPU_IP));
			return(scsiio.resent);

		case 0x36:
			return(0);					// ２枚刺しとか…
	}
	if (scsiio.port <= 0x19) {
		ret = scsiio.reg[scsiio.port];
		SCSITRACEOUT(("scsitrace in port=0cc2 ar=%02x data=%02x cs=%04x ip=%04x",
				scsiio.port, ret, CPU_CS, CPU_IP));
		TRACEOUT(("scsi ctrl read %s %.2x [%.4x:%.4x]",
							scsictr[scsiio.port], ret, CPU_CS, CPU_IP));
		scsiio.port++;
		return(ret);
	}
	SCSITRACEOUT(("scsitrace in port=0cc2 ar=%02x data=ff cs=%04x ip=%04x",
			scsiio.port, CPU_CS, CPU_IP));
	(void)port;
	return(0xff);
}

static REG8 IOINPCALL scsiio_icc4(UINT port) {

	SCSITRACEOUT(("scsitrace in port=0cc4 data=00 cs=%04x ip=%04x",
		CPU_CS, CPU_IP));
	TRACEOUT(("scsiio_icc4"));
	(void)port;
	return(0x00);
}

static REG8 IOINPCALL scsiio_icc6(UINT port) {

	REG8	ret;

	ret = scsiio.data[scsiio.rddatpos & 0x7fff];
	SCSITRACEOUT(("scsitrace in port=0cc6 data=%02x ar=%02x cs=%04x ip=%04x",
			ret, scsiio.port, CPU_CS, CPU_IP));
	scsiio.rddatpos++;
	if ((scsiio.phase == SCSIPH_DATAIN) &&
		(scsiio.rddatpos >= scsiio.cmdpos)) {
		scsiio.phase = SCSIPH_STATUS;
		scsiintr(0x8b);
	}
	(void)port;
	return(ret);
}


// ----

void scsiio_reset(void) {

	ZeroMemory(&scsiio, sizeof(scsiio));
	scsi_csr_latched = FALSE;
	scsi_csr_event_active = FALSE;
	scsi_csr_event_status = 0;
	scsi_csr_pending = FALSE;
	scsi_csr_pending_status = 0;
	if (pccore.hddif & PCHDD_SCSI) {
		/* INT2/IRQ6 is the VA bus choice that does not collide with SASI. */
		scsiio.resent = (2 << 3) + (7 << 0);
		/*
		 * PCPLUS.SYS supplies the $SCSIBIOS service through the board I/O
		 * interface.  The PC-88VA SCSI55 guidance permits the board ROM to
		 * be disconnected, so do not claim a VA system-memory window for it.
		 */
		TRACEOUT(("SCSI board ROM detached; use PCPLUS $SCSIBIOS"));
	}
}

void scsiio_bind(void) {

	if (pccore.hddif & PCHDD_SCSI) {
		iocore_attachout(0x0cc0, scsiio_occ0);
		iocore_attachout(0x0cc2, scsiio_occ2);
		iocore_attachout(0x0cc4, scsiio_occ4);
		iocore_attachout(0x0cc6, scsiio_occ6);
		iocore_attachinp(0x0cc0, scsiio_icc0);
		iocore_attachinp(0x0cc2, scsiio_icc2);
		iocore_attachinp(0x0cc4, scsiio_icc4);
		iocore_attachinp(0x0cc6, scsiio_icc6);
		iocoreva_attachout(0x0cc0, scsiio_occ0);
		iocoreva_attachout(0x0cc2, scsiio_occ2);
		iocoreva_attachout(0x0cc4, scsiio_occ4);
		iocoreva_attachout(0x0cc6, scsiio_occ6);
		iocoreva_attachinp(0x0cc0, scsiio_icc0);
		iocoreva_attachinp(0x0cc2, scsiio_icc2);
		iocoreva_attachinp(0x0cc4, scsiio_icc4);
		iocoreva_attachinp(0x0cc6, scsiio_icc6);
	}
}
