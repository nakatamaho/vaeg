#include	"compiler.h"

#include	"dosio.h"
#include	"cpucore.h"
#include	"pccore.h"
#include	"iocore.h"
#include	"cbuscore.h"
#include	"scsiio.h"
#include	"scsiio.tbl"
#include	"scsicmd.h"
#include	"sxsi.h"

#if defined(_WIN32) && defined(TRACE)
extern void iptrace_out(void);
#define	SCSICMD_ERR		MessageBox(NULL, "SCSI error", "?", MB_OK);	\
						exit(1);
#else
#define	SCSICMD_ERR
#endif


static const BYTE hdd_inquiry[0x20] = {
			0x00,0x00,0x02,0x02,0x1c,0x00,0x00,0x18,
			'N', 'E', 'C', 0x20,0x20,0x20,0x20,0x20,
			'N', 'P', '2', '-', 'H', 'D', 'D', 0x20,
			0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20};


static void scsicmd_putbe32(BYTE *dst, UINT32 value) {

	dst[0] = (BYTE)(value >> 24);
	dst[1] = (BYTE)(value >> 16);
	dst[2] = (BYTE)(value >> 8);
	dst[3] = (BYTE)value;
}

static UINT scsicmd_datain(SXSIDEV sxsi, BYTE *cdb) {

	UINT	length;
	UINT	copylen;
	UINT32	last_lba;

	ZeroMemory(scsiio.data, sizeof(scsiio.data));
	scsiio.cmdpos = 0;
	switch(cdb[0]) {
		case 0x12:				// Inquiry
			TRACEOUT(("Inquiry"));
			// Logical unit number = cdb[1] >> 5.
			// EVPD = cdb[1] & 1.
			// Page code = cdb[2].
			length = cdb[4];
			copylen = min(length, (UINT)sizeof(hdd_inquiry));
			if (copylen) {
				CopyMemory(scsiio.data, hdd_inquiry, copylen);
			}
			scsiio.cmdpos = copylen;
			break;

		case 0x25:				// Read Capacity (10)
			TRACEOUT(("Read Capacity"));
			last_lba = (sxsi->totals > 0) ? (UINT32)(sxsi->totals - 1) : 0;
			scsicmd_putbe32(scsiio.data + 0, last_lba);
			scsicmd_putbe32(scsiio.data + 4, sxsi->size);
			scsiio.cmdpos = 8;
			break;

		case 0x1a:				// Mode Sense (6)
			TRACEOUT(("Mode Sense (6)"));
			/*
			 * Return one direct-access block descriptor.  The device
			 * geometry comes from the mounted SCSI image, while the
			 * caller's allocation length controls the visible prefix.
			 */
			length = cdb[4];
			scsiio.data[1] = 0x00;
			scsiio.data[2] = 0x00;
			scsiio.data[3] = 0x08;
			scsiio.data[4] = 0;
			scsiio.data[5] = (BYTE)(sxsi->totals >> 16);
			scsiio.data[6] = (BYTE)(sxsi->totals >> 8);
			scsiio.data[7] = (BYTE)sxsi->totals;
			scsiio.data[8] = (BYTE)(sxsi->size >> 16);
			scsiio.data[9] = (BYTE)(sxsi->size >> 8);
			scsiio.data[10] = (BYTE)sxsi->size;
			copylen = min(length, (UINT)12);
			scsiio.data[0] = (copylen > 0) ? (BYTE)(copylen - 1) : 0;
			scsiio.cmdpos = copylen;
			break;

		default:
			break;
	}
	return(scsiio.cmdpos);
}






// ----

REG8 scsicmd_negate(REG8 id) {

	scsiio.phase = 0;
	(void)id;
	return(0x85);			// disconnect
}

REG8 scsicmd_select(REG8 id) {

	SXSIDEV	sxsi;

	TRACEOUT(("scsicmd_select"));
	if (scsiio.reg[SCSICTR_TARGETLUN] & 7) {
		TRACEOUT(("LUN = %d", scsiio.reg[SCSICTR_TARGETLUN] & 7));
		return(0x42);
	}
	sxsi = sxsi_getptr((REG8)(0x20 + id));
	if ((sxsi) && (sxsi->type)) {
		scsiio.phase = SCSIPH_COMMAND;
		return(0x8a);			// Transfer Command要求
	}
	return(0x42);				// Timeout
}

REG8 scsicmd_transfer(REG8 id, BYTE *cdb) {

	SXSIDEV	sxsi;

	if (scsiio.reg[SCSICTR_TARGETLUN] & 7) {
		return(0x42);
	}

	sxsi = sxsi_getptr((REG8)(0x20 + id));
	if ((sxsi == NULL) || (sxsi->type == 0)) {
		return(0x42);
	}

	TRACEOUT(("sel ope code = %.2x", cdb[0]));
	switch(cdb[0]) {
		case 0x00:				// Test Unit Ready
			return(0x16);		// Succeed

		case 0x12:				// Inquiry
		case 0x1a:				// Mode Sense (6)
		case 0x25:				// Read Capacity (10)
			scsicmd_datain(sxsi, cdb);
			return(0x16);			// Succeed
	}

	SCSICMD_ERR
	return(0xff);
}


static REG8 scsicmd_cmd(REG8 id) {

	SXSIDEV	sxsi;

	TRACEOUT(("scsicmd_cmd = %.2x", scsiio.cmd[0]));
	if (scsiio.reg[SCSICTR_TARGETLUN] & 7) {
		return(0x42);
	}
	sxsi = sxsi_getptr((REG8)(0x20 + id));
	if ((sxsi == NULL) || (sxsi->type == 0)) {
		return(0x42);
	}
	switch(scsiio.cmd[0]) {
		case 0x00:
			scsiio.phase = SCSIPH_STATUS;
			return(0x8b);		// Transfer Status要求

		case 0x12:				// inquiry
		case 0x1a:				// Mode Sense (6)
		case 0x25:				// Read Capacity (10)
			scsicmd_datain(sxsi, scsiio.cmd);
			scsiio.phase = SCSIPH_DATAIN;
			scsiio.rddatpos = 0;
			return(0x89);		// Transfer Data要求
	}

	SCSICMD_ERR
	return(0xff);
}

BOOL scsicmd_send(void) {

	switch(scsiio.phase) {
		case SCSIPH_COMMAND:
			scsiio.cmdpos = 0;
			return(SUCCESS);
	}
	return(FAILURE);
}

REG8 scsicmd_transinfo(REG8 id) {

	REG8	ret;

	switch(scsiio.phase) {
		case SCSIPH_COMMAND:
			CopyMemory(scsiio.cmd, scsiio.reg + SCSICTR_CDB,
																	 sizeof(scsiio.cmd));
			ret = scsicmd_cmd(id);
			return(ret);

		case SCSIPH_DATAIN:
			if (scsiio.rddatpos >= scsiio.cmdpos) {
				scsiio.phase = SCSIPH_STATUS;
				return(0x8b);			}
			return(0x89);			// Transfer Data request remains active.

		case SCSIPH_DATAOUT:
			if (scsiio.cmdpos && (scsiio.wrdatpos >= scsiio.cmdpos)) {
				scsiio.phase = SCSIPH_STATUS;
				return(0x8b);
			}
			return(0x88);			// Transfer Data Out request.

		case SCSIPH_STATUS:
			scsiio.reg[SCSICTR_STATUS] = 0x00;
			scsiio.phase = SCSIPH_MSGIN;
			return(0x8f);			// Transfer Message request.

		case SCSIPH_MSGIN:
			scsiio.phase = 0;
			return(0x80);			// Disconnect.
	}
	return(0x42);
}


// ---- BIOS から

static const UINT8 stat2ret[16] = {
				0x40, 0x00, 0x10, 0x00,
				0x20, 0x00, 0x10, 0x00,
				0x30, 0x00, 0x10, 0x00,
				0x20, 0x00, 0x10, 0x00};

static REG8 bios1bc_seltrans(REG8 id) {

	BYTE	cdb[16];
	REG8	ret;

	MEML_READSTR(CPU_DS, CPU_DX, cdb, 16);
	scsiio.reg[SCSICTR_TARGETLUN] = cdb[0];
	if ((cdb[1] & 0x0c) == 0x08) {			// OUT
		MEML_READSTR(CPU_ES, CPU_BX, scsiio.data, CPU_CX);
	}
	ret = scsicmd_transfer(id, cdb + 4);
	if ((cdb[1] & 0x0c) == 0x04) {			// IN
		MEML_WRITESTR(CPU_ES, CPU_BX, scsiio.data, CPU_CX);
	}
	return(ret);
}

void scsicmd_bios(void) {

	UINT8	flag;
	UINT8	ret;
	REG8	stat;
	UINT	cmd;
	REG8	dstid;

	TRACEOUT(("BIOS 1B-C* CPU_AH %.2x", CPU_AH));

	if (CPU_AH & 0x80) {		// エラーぽ
		return;
	}

	flag = MEML_READ8(CPU_SS, CPU_SP+4) & 0xbe;
	ret = mem[0x0483];
	cmd = CPU_AH & 0x1f;
	dstid = CPU_AL & 7;
	if (ret & 0x80) {
		mem[0x0483] &= 0x7f;
	}
	else if (cmd < 0x18) {
		switch(cmd) {
			case 0x00:		// reset
				stat = 0x00;
				break;

			case 0x03:		// Negate ACK
				stat = scsicmd_negate(dstid);
				break;

			case 0x07:		// Select Without AMN
				stat = scsicmd_select(dstid);
				break;

			case 0x09:		// Select Without AMN and Transfer
				stat = bios1bc_seltrans(dstid);
				break;

			default:
				TRACEOUT(("cmd = %.2x", CPU_AH));
				SCSICMD_ERR
				stat = 0x42;
				break;
		}
		ret = stat2ret[stat >> 4] + (stat & 0x0f);
		TRACEOUT(("BIOS 1B-C* CPU_AH %.2x ret = %.2x", CPU_AH, ret));
		mem[0x0483] = ret;
	}
	else {
		if ((ret ^ cmd) & 0x0f) {
			ret = cmd | 0x80;
		}
		else {
			switch(cmd) {
				case 0x19:		// Data In
					MEML_WRITESTR(CPU_ES, CPU_BX, scsiio.data, CPU_CX);
					scsiio.phase = SCSIPH_STATUS;
					stat = 0x8b;
					break;

				case 0x1a:		// Transfer command
					MEML_READSTR(CPU_ES, CPU_BX, scsiio.cmd, 12);
					stat = scsicmd_cmd(dstid);
					break;

				case 0x1b:		// Status In
					scsiio.phase = SCSIPH_MSGIN;
					stat = 0x8f;
					break;

				case 0x1f:		// Message In
					scsiio.phase = 0;
					stat = 0x80;
					break;

				default:
					TRACEOUT(("cmd = %.2x", CPU_AH));
					SCSICMD_ERR
					stat = 0x42;
					break;
			}
			ret = stat2ret[stat >> 4] + (stat & 0x0f);
		}
		TRACEOUT(("BIOS 1B-C* CPU_AH %.2x ret = %.2x", CPU_AH, ret));
		mem[0x0483] = ret;
	}
	flag |= ret & Z_FLAG;
	if (ret & 0x80) {
		flag |= C_FLAG;
		ret &= 0x7f;
	}
	CPU_AH = ret;
	MEML_WRITE8(CPU_SS, CPU_SP + 4, flag);
}
