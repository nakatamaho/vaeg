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

typedef struct {
	UINT phase;
	REG8 service_status;
	BOOL host_to_spc;
} SCSIPHASECONTRACT;

/* One table owns the target phase and the WD33C93 service-request code. */
static const SCSIPHASECONTRACT scsi_phase_contract[] = {
	{SCSIPH_DATAOUT, 0x88, TRUE},
	{SCSIPH_DATAIN, 0x89, FALSE},
	{SCSIPH_COMMAND, 0x8a, TRUE},
	{SCSIPH_STATUS, 0x8b, FALSE},
	{SCSIPH_INFOOUT, 0x8c, TRUE},
	{SCSIPH_INFOIN, 0x8d, FALSE},
	{SCSIPH_MSGOUT, 0x8e, TRUE},
	{SCSIPH_MSGIN, 0x8f, FALSE}
};

REG8 scsicmd_phase_service_status(UINT phase) {
	UINT i;

	for (i = 0; i < (UINT)(sizeof(scsi_phase_contract) /
			sizeof(scsi_phase_contract[0])); i++) {
		if (scsi_phase_contract[i].phase == phase) {
			return scsi_phase_contract[i].service_status;
		}
	}
	return 0x42;
}

REG8 scsicmd_phase_unexpected_status(UINT phase) {

	/* 48h-4Fh reports an information-phase change before TC expires. */
	return (REG8)(0x48 | (phase & 7));
}

BOOL scsicmd_phase_host_to_spc(UINT phase) {
	UINT i;

	for (i = 0; i < (UINT)(sizeof(scsi_phase_contract) /
			sizeof(scsi_phase_contract[0])); i++) {
		if (scsi_phase_contract[i].phase == phase) {
			return scsi_phase_contract[i].host_to_spc;
		}
	}
	return FALSE;
}

static const BYTE hdd_inquiry[0x20] = {
			0x00,0x00,0x02,0x02,0x1b,0x00,0x00,0x18,
			'N', 'E', 'C', 0x20,0x20,0x20,0x20,0x20,
			'N', 'P', '2', '-', 'H', 'D', 'D', 0x20,
			0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20};

static BYTE hdd_sense[18] = {
			0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a,
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			0x00, 0x00};

static BOOL scsicmd_check_condition;


static void scsicmd_putbe32(BYTE *dst, UINT32 value) {

	dst[0] = (BYTE)(value >> 24);
	dst[1] = (BYTE)(value >> 16);
	dst[2] = (BYTE)(value >> 8);
	dst[3] = (BYTE)value;
}

static void scsicmd_putbe24(BYTE *dst, UINT32 value) {

	dst[0] = (BYTE)(value >> 16);
	dst[1] = (BYTE)(value >> 8);
	dst[2] = (BYTE)value;
}

static BOOL scsicmd_geometry_valid(SXSIDEV sxsi) {

	UINT64 expected;

	if ((sxsi == NULL) || (sxsi->totals <= 0) ||
			(sxsi->cylinders == 0) || (sxsi->surfaces == 0) ||
			(sxsi->sectors == 0) || (sxsi->size == 0)) {
		return FALSE;
	}
	expected = (UINT64)sxsi->cylinders * sxsi->surfaces * sxsi->sectors;
	return expected == (UINT64)sxsi->totals;
}

static void scsicmd_set_sense(BYTE key, BYTE asc, BYTE ascq) {

	hdd_sense[2] = key;
	hdd_sense[12] = asc;
	hdd_sense[13] = ascq;
}

static UINT scsicmd_datain(SXSIDEV sxsi, BYTE *cdb) {

	UINT	length;
	UINT	copylen;
	UINT32	last_lba;
	UINT	page;
	UINT	page_offset;
	UINT	response_length;
	BOOL	dbd;

	ZeroMemory(scsiio.data, sizeof(scsiio.data));
	scsiio.cmdpos = 0;
	if (cdb[0] != 0x03) {
		scsicmd_check_condition = FALSE;
	}
	switch(cdb[0]) {
		case 0x03:				// Request Sense
			TRACEOUT(("Request Sense"));
			length = cdb[4];
			copylen = min(length, (UINT)sizeof(hdd_sense));
			if (copylen) {
				CopyMemory(scsiio.data, hdd_sense, copylen);
			}
			scsiio.cmdpos = copylen;
			scsicmd_set_sense(0x00, 0x00, 0x00);
			scsicmd_check_condition = FALSE;
			break;

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
			page = cdb[2] & 0x3f;
			if ((page != 0x04) && (page != 0x3f)) {
				/* Invalid page code: CHECK CONDITION / ILLEGAL REQUEST. */
				scsicmd_set_sense(0x05, 0x24, 0x00);
				scsicmd_check_condition = TRUE;
				break;
			}
			if (!scsicmd_geometry_valid(sxsi)) {
				/* A contradictory image geometry is not a usable target. */
				scsicmd_set_sense(0x05, 0x24, 0x00);
				scsicmd_check_condition = TRUE;
				break;
			}
			/*
			 * Page 04h is the rigid-disk geometry page.  Page 3fh is
			 * the supported-pages request and currently expands to this
			 * single page.  The DBD bit selects the optional descriptor.
			 */
			dbd = (cdb[1] & 0x08) != 0;
			page_offset = dbd ? 4 : 12;
			response_length = page_offset + 24;
			length = cdb[4];
			scsiio.data[1] = 0x00;
			scsiio.data[2] = 0x00;
			scsiio.data[3] = dbd ? 0 : 8;
			if (!dbd) {
				scsiio.data[4] = 0;
				scsicmd_putbe24(scsiio.data + 5,
						(UINT32)sxsi->totals);
				scsicmd_putbe24(scsiio.data + 8,
						(UINT32)sxsi->size);
			}
			scsiio.data[page_offset + 0] = 0x04;
			scsiio.data[page_offset + 1] = 0x16;
			scsicmd_putbe24(scsiio.data + page_offset + 2,
					(UINT32)sxsi->cylinders);
			scsiio.data[page_offset + 5] = sxsi->surfaces;
			/* Write-precomp, reduced-current, step-rate, RPL and
			 * rotational fields remain zero for the emulated disk. */
			scsicmd_putbe24(scsiio.data + page_offset + 14,
					(UINT32)sxsi->cylinders);
			copylen = min(length, response_length);
			scsiio.data[0] = (BYTE)(response_length - 1);
			scsiio.cmdpos = copylen;
			break;

		default:
			scsicmd_set_sense(0x05, 0x20, 0x00);
			scsicmd_check_condition = TRUE;
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


REG8 scsicmd_command(REG8 id) {

	SXSIDEV	sxsi;

	TRACEOUT(("scsicmd_cmd = %.2x", scsiio.cmd[0]));
	if (scsiio.reg[SCSICTR_TARGETLUN] & 7) {
		return(0x42);
	}
	sxsi = sxsi_getptr((REG8)(0x20 + id));
	if ((sxsi == NULL) || (sxsi->type == 0)) {
		return(0x42);
	}
	scsiio.reg[SCSICTR_STATUS] = 0x00;
	if (scsiio.cmd[0] != 0x03) {
		scsicmd_check_condition = FALSE;
	}
	switch(scsiio.cmd[0]) {
		case 0x00:
			scsiio.phase = SCSIPH_STATUS;
			return(scsicmd_phase_service_status(SCSIPH_STATUS));

		case 0x03:				// Request Sense
		case 0x12:				// inquiry
		case 0x1a:				// Mode Sense (6)
		case 0x25:				// Read Capacity (10)
			scsicmd_datain(sxsi, scsiio.cmd);
			if (!scsicmd_check_condition) {
				scsiio.phase = SCSIPH_DATAIN;
				scsiio.rddatpos = 0;
				return(scsicmd_phase_service_status(SCSIPH_DATAIN));
			}
			/* Data-command validation failures complete in STATUS. */
			scsiio.reg[SCSICTR_STATUS] = 0x02;
			scsiio.data[0] = 0x02;
			scsiio.cmdpos = 1;
			scsiio.rddatpos = 0;
			scsiio.phase = SCSIPH_STATUS;
			return(scsicmd_phase_service_status(SCSIPH_STATUS));
	}

	SCSICMD_ERR
	/* Unknown commands use CHECK CONDITION rather than hanging the bus. */
	scsicmd_set_sense(0x05, 0x20, 0x00);
	scsiio.reg[SCSICTR_STATUS] = 0x02;
	scsiio.data[0] = 0x02;
	scsiio.cmdpos = 1;
	scsiio.phase = SCSIPH_STATUS;
	return(scsicmd_phase_service_status(SCSIPH_STATUS));
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
			ret = scsicmd_command(id);
			return(ret);

		case SCSIPH_DATAIN:
			if (scsiio.rddatpos >= scsiio.cmdpos) {
				scsiio.phase = SCSIPH_STATUS;
				/* The next phase starts a fresh PIO data window. */
				scsiio.rddatpos = 0;
				return(scsicmd_phase_service_status(SCSIPH_STATUS));
			}
			return(scsicmd_phase_service_status(SCSIPH_DATAIN));

		case SCSIPH_DATAOUT:
			if (scsiio.cmdpos && (scsiio.wrdatpos >= scsiio.cmdpos)) {
				scsiio.phase = SCSIPH_STATUS;
				/* The next phase starts a fresh PIO data window. */
				scsiio.rddatpos = 0;
				return(scsicmd_phase_service_status(SCSIPH_STATUS));
			}
			return(scsicmd_phase_service_status(SCSIPH_DATAOUT));

		case SCSIPH_STATUS:
			scsiio.reg[SCSICTR_STATUS] = 0x00;
			scsiio.phase = SCSIPH_MSGIN;
			/* The next phase starts a fresh PIO data window. */
			scsiio.rddatpos = 0;
			return(scsicmd_phase_service_status(SCSIPH_MSGIN));

		case SCSIPH_MSGIN:
			scsiio.phase = 0;
			scsiio.rddatpos = 0;
			return((scsiio.reg[SCSICTR_CONTROL] & 0x08) ? 0x85 : 0x80);
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
					stat = scsicmd_command(dstid);
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
