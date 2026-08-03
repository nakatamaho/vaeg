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

static const BYTE hdd_inquiry[36] = {
			0x00,0x00,0x02,0x02,0x1f,0x00,0x00,0x18,
			'N', 'E', 'C', 0x20,0x20,0x20,0x20,0x20,
			'N', 'P', '2', '-', 'H', 'D', 'D', 0x20,
			0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
			'1', '.', '0', '0'};

static const BYTE hdd_inquiry_unsupported_lun[36] = {
			0x7f,0x00,0x02,0x02,0x1f,0x00,0x00,0x00,
			0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
			0x00,0x00,0x00,0x00};

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

static UINT scsicmd_cdb_lun(const BYTE *cdb) {

	return cdb ? ((cdb[1] >> 5) & 7) : 0;
}

static UINT scsicmd_target_lun(void) {

	return scsiio.reg[SCSICTR_TARGETLUN] & 7;
}

static UINT scsicmd_backend_index(UINT id) {

	return 2 + (id & 7);
}

/* The SCSI target backend has one logical unit per target ID: LUN 0. */
static BOOL scsicmd_lun_supported(const BYTE *cdb) {

	return (scsicmd_target_lun() == 0) &&
			(cdb == NULL || scsicmd_cdb_lun(cdb) == 0);
}

static UINT scsicmd_cdb_length(const BYTE *cdb) {

	if (cdb == NULL) {
		return 0;
	}
	switch (cdb[0] >> 5) {
		case 0:
			return 6;
		case 1:
		case 2:
			return 10;
		default:
			return 12;
	}
}

static void scsicmd_trace_cdb_result(UINT id, const BYTE *cdb,
		SXSIDEV sxsi, REG8 status) {
	REG8 inquiry_byte0;
	UINT selected_index;

	inquiry_byte0 = (cdb && cdb[0] == 0x12 && scsiio.cmdpos) ?
			scsiio.data[0] : 0xff;
	selected_index = (sxsi && scsicmd_lun_supported(cdb)) ?
			scsicmd_backend_index(id) : 0xff;
	scsiio_trace_cdb_result(id, scsicmd_target_lun(),
			scsicmd_cdb_lun(cdb),
			selected_index, cdb, scsicmd_cdb_length(cdb), inquiry_byte0,
			scsiio.cmdpos, status, hdd_sense[2], hdd_sense[12], hdd_sense[13]);
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
	UINT	geometry_offset;
	BOOL	dbd;

	ZeroMemory(scsiio.data, sizeof(scsiio.data));
	scsiio.cmdpos = 0;
	if (cdb[0] != 0x03) {
		scsicmd_check_condition = FALSE;
	}
	if (!scsicmd_lun_supported(cdb) && cdb[0] != 0x03 &&
			cdb[0] != 0x12) {
		/* Unsupported LUNs never reach the mounted LUN-0 backend. */
		scsicmd_set_sense(0x05, 0x25, 0x00);
		scsicmd_check_condition = TRUE;
		return 0;
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
			{
				const BYTE *inquiry;

				TRACEOUT(("Inquiry LUN=%u", scsicmd_cdb_lun(cdb)));
				/* Unsupported LUN inquiry is GOOD with peripheral qualifier 011b. */
				inquiry = scsicmd_lun_supported(cdb) ? hdd_inquiry :
						hdd_inquiry_unsupported_lun;
				length = cdb[4];
				copylen = min(length, 36U);
				if (copylen) {
					CopyMemory(scsiio.data, inquiry, copylen);
				}
				scsiio.cmdpos = copylen;
			}
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
			if ((page != 0x00) && (page != 0x04) && (page != 0x3f)) {
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
			 * Page 04h is the rigid-disk geometry page.  Page 00h is
			 * the supported empty page, and page 3fh returns both pages.
			 * The DBD bit selects the optional descriptor.
			 */
			dbd = (cdb[1] & 0x08) != 0;
			page_offset = dbd ? 4 : 12;
			geometry_offset = page_offset;
			if (page == 0x3f) {
				scsiio.data[page_offset + 0] = 0x00;
				scsiio.data[page_offset + 1] = 0x00;
				geometry_offset += 2;
			}
			response_length = geometry_offset +
					((page == 0x00) ? 2 : 24);
			length = cdb[4];
			scsiio.data[1] = 0x00;
			scsiio.data[2] = 0x00;
			scsiio.data[3] = dbd ? 0 : 8;
			if (!dbd) {
				scsiio.data[4] = 0;
				scsicmd_putbe24(scsiio.data + 5,
						(UINT32)sxsi->totals);
				scsicmd_putbe24(scsiio.data + 9,
						(UINT32)sxsi->size);
			}
			if (page != 0x00) {
				scsiio.data[geometry_offset + 0] = 0x04;
				scsiio.data[geometry_offset + 1] = 0x16;
				scsicmd_putbe24(scsiio.data + geometry_offset + 2,
						(UINT32)sxsi->cylinders);
				scsiio.data[geometry_offset + 5] = sxsi->surfaces;
				/* Write-precomp, reduced-current, step-rate, RPL and
				 * rotational fields remain zero for the emulated disk. */
				scsicmd_putbe24(scsiio.data + geometry_offset + 14,
						(UINT32)sxsi->cylinders);
			}
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

	SXSIDEV sxsi;
	REG8 status;
	UINT selected_index;

	TRACEOUT(("scsicmd_select"));
	sxsi = sxsi_getptr((REG8)(0x20 + id));
	selected_index = (sxsi && sxsi->type) ? scsicmd_backend_index(id) : 0xff;
	if (scsicmd_target_lun() != 0) {
		TRACEOUT(("LUN = %d", scsicmd_target_lun()));
		status = 0x42;
	}
	else if ((sxsi) && (sxsi->type)) {
		scsiio.phase = SCSIPH_COMMAND;
		status = 0x8a;
	}
	else {
		status = 0x42;
	}
	scsiio_trace_target_selection(id, scsicmd_target_lun(), selected_index, status);
	return status;
}

REG8 scsicmd_transfer(REG8 id, BYTE *cdb) {

	SXSIDEV sxsi;
	REG8 status;

	if (cdb == NULL) {
		return 0x42;
	}
	sxsi = sxsi_getptr((REG8)(0x20 + id));
	if ((sxsi == NULL) || (sxsi->type == 0)) {
		return 0x42;
	}

	TRACEOUT(("sel ope code = %.2x lun=%u", cdb[0], scsicmd_cdb_lun(cdb)));
	switch(cdb[0]) {
		case 0x00:				// Test Unit Ready
			if (!scsicmd_lun_supported(cdb)) {
				scsicmd_set_sense(0x05, 0x25, 0x00);
				scsiio.reg[SCSICTR_STATUS] = 0x02;
			}
			else {
				scsiio.reg[SCSICTR_STATUS] = 0x00;
			}
			status = 0x16;
			scsicmd_trace_cdb_result(id, cdb,
				scsicmd_lun_supported(cdb) ? sxsi : NULL,
				scsiio.reg[SCSICTR_STATUS]);
			return status;

		case 0x03:				// Request Sense
		case 0x12:				// Inquiry
		case 0x1a:				// Mode Sense (6)
		case 0x25:				// Read Capacity (10)
			scsicmd_datain(sxsi, cdb);
			scsiio.reg[SCSICTR_STATUS] = scsicmd_check_condition ? 0x02 : 0x00;
			scsicmd_trace_cdb_result(id, cdb,
				(scsicmd_lun_supported(cdb) ? sxsi : NULL),
				scsiio.reg[SCSICTR_STATUS]);
			return status = 0x16;
	}

	scsicmd_set_sense(0x05, 0x20, 0x00);
	scsiio.reg[SCSICTR_STATUS] = 0x02;
	scsicmd_trace_cdb_result(id, cdb, NULL, scsiio.reg[SCSICTR_STATUS]);
	return 0x16;
}

REG8 scsicmd_command(REG8 id) {

	SXSIDEV sxsi;
	REG8 service_status;
	BOOL lun_supported;
	SXSIDEV trace_sxsi;

	TRACEOUT(("scsicmd_cmd = %.2x lun=%u", scsiio.cmd[0],
			scsicmd_cdb_lun(scsiio.cmd)));
	sxsi = sxsi_getptr((REG8)(0x20 + id));
	if ((sxsi == NULL) || (sxsi->type == 0)) {
		return 0x42;
	}
	lun_supported = scsicmd_lun_supported(scsiio.cmd);
	scsiio.reg[SCSICTR_STATUS] = 0x00;
	if (scsiio.cmd[0] != 0x03) {
		scsicmd_check_condition = FALSE;
	}

	/* Unsupported LUN INQUIRY is GOOD and is represented by byte 0=7fh. */
	if (!lun_supported && scsiio.cmd[0] != 0x03 && scsiio.cmd[0] != 0x12) {
		scsicmd_set_sense(0x05, 0x25, 0x00);
		scsiio.reg[SCSICTR_STATUS] = 0x02;
		scsiio.data[0] = 0x02;
		scsiio.cmdpos = 1;
		scsiio.rddatpos = 0;
		scsiio.phase = SCSIPH_STATUS;
		service_status = scsicmd_phase_service_status(SCSIPH_STATUS);
		scsicmd_trace_cdb_result(id, scsiio.cmd, NULL,
				scsiio.reg[SCSICTR_STATUS]);
		return service_status;
	}

	switch(scsiio.cmd[0]) {
		case 0x00:
			scsiio.phase = SCSIPH_STATUS;
			service_status = scsicmd_phase_service_status(SCSIPH_STATUS);
			scsicmd_trace_cdb_result(id, scsiio.cmd, sxsi,
					scsiio.reg[SCSICTR_STATUS]);
			return service_status;

		case 0x03:				// Request Sense
		case 0x12:				// Inquiry
		case 0x1a:				// Mode Sense (6)
		case 0x25:				// Read Capacity (10)
			scsicmd_datain(sxsi, scsiio.cmd);
			if (!scsicmd_check_condition) {
				scsiio.phase = SCSIPH_DATAIN;
				scsiio.rddatpos = 0;
				service_status = scsicmd_phase_service_status(SCSIPH_DATAIN);
				trace_sxsi = lun_supported ? sxsi : NULL;
				scsicmd_trace_cdb_result(id, scsiio.cmd, trace_sxsi,
						scsiio.reg[SCSICTR_STATUS]);
				return service_status;
			}
			/* Data-command validation failures complete in STATUS. */
			scsiio.reg[SCSICTR_STATUS] = 0x02;
			scsiio.data[0] = 0x02;
			scsiio.cmdpos = 1;
			scsiio.rddatpos = 0;
			scsiio.phase = SCSIPH_STATUS;
			service_status = scsicmd_phase_service_status(SCSIPH_STATUS);
			scsicmd_trace_cdb_result(id, scsiio.cmd, NULL,
					scsiio.reg[SCSICTR_STATUS]);
			return service_status;
	}

	scsicmd_set_sense(0x05, 0x20, 0x00);
	scsiio.reg[SCSICTR_STATUS] = 0x02;
	scsiio.data[0] = 0x02;
	scsiio.cmdpos = 1;
	scsiio.rddatpos = 0;
	scsiio.phase = SCSIPH_STATUS;
	service_status = scsicmd_phase_service_status(SCSIPH_STATUS);
	scsicmd_trace_cdb_result(id, scsiio.cmd, NULL,
			scsiio.reg[SCSICTR_STATUS]);
	return service_status;
}


BOOL scsicmd_backend_selftest(void) {
	_SXSIDEV saved_slots[SCSIHDD_MAX];
	SXSIDEV slot;
	BYTE saved_reg[0x30];
	BYTE saved_cmd[12];
	BYTE saved_data[36];
	BYTE saved_sense[sizeof(hdd_sense)];
	UINT saved_phase;
	UINT saved_cmdpos;
	UINT saved_rddatpos;
	UINT saved_port;
	REG8 saved_scsistatus;
	BOOL saved_check_condition;
	BYTE cdb[12];
	BYTE expected[36];
	UINT id;
	UINT visible;
	UINT media_operations;
	long saved_remclock;
	REG8 service;
	BOOL ok = TRUE;

#define SCMD_SELFTEST_CHECK(name, expression) do { \
		if (!(expression)) { \
			fprintf(stderr, "selftest: %s FAIL\n", name); \
			ok = FALSE; \
		} \
		else { \
			fprintf(stderr, "selftest: %s PASS\n", name); \
		} \
	} while (0)
#define SCMD_SELFTEST_COMMAND(bytes) do { \
		CopyMemory(scsiio.cmd, (bytes), sizeof(scsiio.cmd)); \
		scsiio.reg[SCSICTR_TARGETLUN] = 0; \
		scsiio.cmdpos = 0; \
		scsiio.rddatpos = 0; \
		service = scsicmd_command(0); \
	} while (0)

	for (id = 0; id < SCSIHDD_MAX; id++) {
		slot = sxsi_getptr((REG8)(0x20 + id));
		saved_slots[id] = *slot;
		ZeroMemory(slot, sizeof(*slot));
		slot->fh = FILEH_INVALID;
	}
	CopyMemory(saved_reg, scsiio.reg, sizeof(saved_reg));
	CopyMemory(saved_cmd, scsiio.cmd, sizeof(saved_cmd));
	CopyMemory(saved_data, scsiio.data, sizeof(saved_data));
	CopyMemory(saved_sense, hdd_sense, sizeof(saved_sense));
	saved_phase = scsiio.phase;
	saved_cmdpos = scsiio.cmdpos;
	saved_rddatpos = scsiio.rddatpos;
	saved_port = scsiio.port;
	saved_scsistatus = scsiio.scsistatus;
	saved_check_condition = scsicmd_check_condition;

	slot = sxsi_getptr(0x20);
	slot->totals = 640L * 32L * 8L;
	slot->cylinders = 640;
	slot->size = 256;
	slot->sectors = 32;
	slot->surfaces = 8;
	slot->type = SXSITYPE_SCSI | SXSITYPE_HDD;

	ZeroMemory(cdb, sizeof(cdb));
	cdb[0] = 0x12;
	cdb[4] = 36;
	SCMD_SELFTEST_COMMAND(cdb);
	ZeroMemory(expected, sizeof(expected));
	expected[0] = 0x00; expected[2] = 0x02; expected[3] = 0x02;
	expected[4] = 0x1f; expected[7] = 0x18;
	CopyMemory(expected + 8, "NEC     ", 8);
	CopyMemory(expected + 16, "NP2-HDD         ", 16);
	CopyMemory(expected + 32, "1.00", 4);
	SCMD_SELFTEST_CHECK("inquiry_lun0_returns_36_bytes",
			service == scsicmd_phase_service_status(SCSIPH_DATAIN) &&
			 scsiio.cmdpos == 36);
	SCMD_SELFTEST_CHECK("inquiry_lun0_additional_length_is_1f",
			scsiio.data[4] == 0x1f);
	SCMD_SELFTEST_CHECK("inquiry_lun0_vendor_product_revision_offsets",
			memcmp(scsiio.data, expected, sizeof(expected)) == 0);

	cdb[4] = 4;
	SCMD_SELFTEST_COMMAND(cdb);
	SCMD_SELFTEST_CHECK("inquiry_allocation_length_truncates_response",
			service == scsicmd_phase_service_status(SCSIPH_DATAIN) &&
			 scsiio.cmdpos == 4 && memcmp(scsiio.data, expected, 4) == 0);

	ZeroMemory(cdb, sizeof(cdb));
	cdb[0] = 0x12; cdb[1] = 0x20; cdb[4] = 36;
	SCMD_SELFTEST_COMMAND(cdb);
	SCMD_SELFTEST_CHECK("inquiry_unsupported_lun_returns_7f",
			service == scsicmd_phase_service_status(SCSIPH_DATAIN) &&
			 scsiio.cmdpos == 36 && scsiio.data[0] == 0x7f &&
			 scsiio.data[8] == 0x00);
	SCMD_SELFTEST_CHECK("inquiry_unsupported_lun_does_not_alias_lun0",
			scsiio.data[0] != expected[0] && scsiio.data[8] != expected[8]);

	ZeroMemory(cdb, sizeof(cdb));
	cdb[0] = 0x00; cdb[1] = 0x20;
	SCMD_SELFTEST_COMMAND(cdb);
	SCMD_SELFTEST_CHECK("tur_unsupported_lun_returns_check_condition",
			service == scsicmd_phase_service_status(SCSIPH_STATUS) &&
			 scsiio.reg[SCSICTR_STATUS] == 0x02 && scsiio.phase == SCSIPH_STATUS);
	ZeroMemory(cdb, sizeof(cdb));
	cdb[0] = 0x03; cdb[1] = 0x20; cdb[4] = sizeof(hdd_sense);
	SCMD_SELFTEST_COMMAND(cdb);
	SCMD_SELFTEST_CHECK("request_sense_unsupported_lun_returns_05_25_00",
			service == scsicmd_phase_service_status(SCSIPH_DATAIN) &&
			 scsiio.data[2] == 0x05 && scsiio.data[12] == 0x25 &&
			 scsiio.data[13] == 0x00);

	media_operations = 0;
	saved_remclock = CPU_REMCLOCK;
	ZeroMemory(cdb, sizeof(cdb));
	cdb[0] = 0x25; cdb[1] = 0x20;
	SCMD_SELFTEST_COMMAND(cdb);
	media_operations = (UINT)(saved_remclock - CPU_REMCLOCK);
	SCMD_SELFTEST_CHECK("read_capacity_unsupported_lun_does_not_access_lun0",
			media_operations == 0 && scsiio.reg[SCSICTR_STATUS] == 0x02);
	ZeroMemory(cdb, sizeof(cdb));
	cdb[0] = 0x1a; cdb[1] = 0x20; cdb[2] = 0x04; cdb[4] = 36;
	SCMD_SELFTEST_COMMAND(cdb);
	media_operations = (UINT)(saved_remclock - CPU_REMCLOCK);
	SCMD_SELFTEST_CHECK("mode_sense_unsupported_lun_does_not_access_lun0",
			media_operations == 0 && scsiio.reg[SCSICTR_STATUS] == 0x02);

	/* Select-and-transfer uses the target-LUN register as well as CDB LUN.
	 * An unsupported LUN must still complete with the protocol-level result,
	 * not be turned into a selection timeout. */
	ZeroMemory(cdb, sizeof(cdb));
	cdb[0] = 0x12; cdb[4] = 36;
	scsiio.reg[SCSICTR_TARGETLUN] = 1;
	service = scsicmd_transfer(0, cdb);
	SCMD_SELFTEST_CHECK("select_transfer_unsupported_lun_inquiry_is_good",
			service == 0x16 && scsiio.reg[SCSICTR_STATUS] == 0x00 &&
			scsiio.data[0] == 0x7f && scsiio.cmdpos == 36);
	ZeroMemory(cdb, sizeof(cdb));
	cdb[0] = 0x00;
	service = scsicmd_transfer(0, cdb);
	SCMD_SELFTEST_CHECK("select_transfer_unsupported_lun_tur_checks",
			service == 0x16 && scsiio.reg[SCSICTR_STATUS] == 0x02 &&
			hdd_sense[2] == 0x05 && hdd_sense[12] == 0x25 &&
			hdd_sense[13] == 0x00);

	scsiio.reg[SCSICTR_TARGETLUN] = 0;
	visible = 0;
	for (id = 0; id < 8; id++) {
		if (scsicmd_select((REG8)id) == 0x8a) {
			visible++;
		}
	}
	SCMD_SELFTEST_CHECK("one_configured_target_enumerates_one_disk", visible == 1);
	SCMD_SELFTEST_CHECK("absent_target_id_times_out",
			scsicmd_select(1) == 0x42 && scsicmd_select(2) == 0x42);
	SCMD_SELFTEST_CHECK("different_luns_do_not_share_device_identity",
			scsiio.reg[SCSICTR_TARGETLUN] == 0);

	for (id = 0; id < SCSIHDD_MAX; id++) {
		slot = sxsi_getptr((REG8)(0x20 + id));
		*slot = saved_slots[id];
	}
	CopyMemory(scsiio.reg, saved_reg, sizeof(saved_reg));
	CopyMemory(scsiio.cmd, saved_cmd, sizeof(saved_cmd));
	CopyMemory(scsiio.data, saved_data, sizeof(saved_data));
	CopyMemory(hdd_sense, saved_sense, sizeof(saved_sense));
	scsiio.phase = saved_phase;
	scsiio.cmdpos = saved_cmdpos;
	scsiio.rddatpos = saved_rddatpos;
	scsiio.port = saved_port;
	scsiio.scsistatus = saved_scsistatus;
	scsicmd_check_condition = saved_check_condition;

#undef SCMD_SELFTEST_COMMAND
#undef SCMD_SELFTEST_CHECK
	return ok ? SUCCESS : FAILURE;
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
