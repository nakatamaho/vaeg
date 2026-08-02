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
static UINT scsi_trace_completion_limit;
static UINT scsi_trace_completion_count;
static BOOL scsi_trace_stop;
static BOOL scsi_csr_latched;
static BOOL scsi_connected_as_initiator;
static BOOL scsi_csr_event_active;
static REG8 scsi_csr_event_status;
static BOOL scsi_command_phase_pending;
static BOOL scsi_transfer_phase_pending;
static REG8 scsi_transfer_phase_status;
static REG8 scsi_transfer_completion_status;
static UINT scsi_transfer_remaining;
typedef enum {
	SCSI_TRANSFER_IDLE = 0,
	SCSI_TRANSFER_WAIT_FOR_REQ,
	SCSI_TRANSFER_BYTE_PENDING,
	SCSI_TRANSFER_WAIT_FOR_POST_COUNT_REQ,
	SCSI_TRANSFER_COMPLETED_OR_TERMINATED
} SCSITRANSFERSTATE;

static SCSITRANSFERSTATE scsi_transfer_state;
static BOOL scsi_transfer_req_asserted;
static BOOL scsi_transfer_ack_asserted;
static BOOL scsi_target_req_scheduled;
static REG8 scsi_target_req_status;
static UINT scsi_transfer_req_sequence;
static UINT scsi_transfer_bytes;
static UINT scsi_transfer_payload_length;
static UINT scsi_transfer_active_phase;
static BOOL scsi_trace_transfer_active;
static UINT scsi_trace_transfer_phase;
static UINT scsi_trace_transfer_count;
static UINT scsi_trace_transfer_ar19_accesses;
static UINT scsi_trace_transfer_ar19_reads;
static UINT scsi_trace_transfer_ar19_writes;
static UINT scsi_trace_transfer_data_port_accesses;
static UINT scsi_trace_transfer_irq_requests;
static UINT scsi_trace_transfer_irq_assertions;
static BOOL scsi_trace_transfer_result_pending;
static REG8 scsi_trace_transfer_result_status;
static UINT scsi_trace_transfer_cdb_length;
static BYTE scsi_trace_transfer_cdb[12];
static const char *scsi_trace_transfer_source;

static void scsi_tracef(const char *fmt, ...);

/* WD33C93 auxiliary-status bits.  The DATA window is PIO-only in M75. */
#define SCSI_AUX_INT	0x80
#define SCSI_AUX_LCI	0x40
#define SCSI_AUX_BSY	0x20
#define SCSI_AUX_CIP	0x10
#define SCSI_AUX_PE	0x02
#define SCSI_AUX_DBR	0x01

/* 0CC4h uses set/reset strobes for the controller transfer controls. */
#define SCSI_C4_TCMS	0x04
#define SCSI_C4_TCMR	0x08
#define SCSI_C4_TCIR	0x10
#define SCSI_C4_DMER	0x02
#define SCSI_C4_DMES	0x01

static void scsiintr(REG8 status);
static void scsiintr_immediate(REG8 status);
static void scsiintr_transfer_complete(REG8 status);
static void scsiio_target_req(REG8 status);
static void scsiio_schedule_target_req(REG8 status);
static void scsiio_target_req_event(NEVENTITEM item);
static void scsiio_command_write(REG8 command);

static const char *scsi_trace_phase_direction(UINT phase) {

	if (scsicmd_phase_service_status(phase) == 0x42) {
		return "unknown";
	}
	return scsicmd_phase_host_to_spc(phase) ? "host-to-spc" : "spc-to-host";
}

static void scsi_trace_transfer_start(UINT phase, UINT count,
		const char *source) {

	if (!scsi_trace_enabled) {
		return;
	}
	if (scsi_trace_transfer_active) {
		scsi_tracef("scsitrace transfer-abandoned phase=%02x direction=%s "
				"tc=%06x ar19_accesses=%u ar19_reads=%u ar19_writes=%u "
				"data_port_accesses=%u irq_requests=%u irq_assertions=%u "
				"source=%s",
				scsi_trace_transfer_phase,
				scsi_trace_phase_direction(scsi_trace_transfer_phase),
				scsi_trace_transfer_count,
				scsi_trace_transfer_ar19_accesses,
				scsi_trace_transfer_ar19_reads,
				scsi_trace_transfer_ar19_writes,
				scsi_trace_transfer_data_port_accesses,
				scsi_trace_transfer_irq_requests,
				scsi_trace_transfer_irq_assertions,
				scsi_trace_transfer_source);
	}
	scsi_trace_transfer_active = TRUE;
	scsi_trace_transfer_phase = phase;
	scsi_trace_transfer_count = count;
	scsi_trace_transfer_ar19_accesses = 0;
	scsi_trace_transfer_ar19_reads = 0;
	scsi_trace_transfer_ar19_writes = 0;
	scsi_trace_transfer_data_port_accesses = 0;
	scsi_trace_transfer_irq_requests = 0;
	scsi_trace_transfer_irq_assertions = 0;
	scsi_trace_transfer_result_pending = FALSE;
	scsi_trace_transfer_result_status = 0;
	scsi_trace_transfer_cdb_length = 0;
	ZeroMemory(scsi_trace_transfer_cdb, sizeof(scsi_trace_transfer_cdb));
	scsi_trace_transfer_source = source;
	scsi_tracef("scsitrace transfer-start phase=%02x direction=%s tc=%06x "
			"source=%s cs=%04x ip=%04x",
			phase, scsi_trace_phase_direction(phase), count, source,
			CPU_CS, CPU_IP);
}

static void scsi_trace_transfer_ar19_access(BOOL write) {

	if (scsi_trace_transfer_active) {
		scsi_trace_transfer_ar19_accesses++;
		if (write) {
			scsi_trace_transfer_ar19_writes++;
		}
		else {
			scsi_trace_transfer_ar19_reads++;
		}
	}
}

static void scsi_trace_transfer_data_port_access(void) {

	if (scsi_trace_transfer_active) {
		scsi_trace_transfer_data_port_accesses++;
	}
}

static void scsi_trace_transfer_result(REG8 status) {

	if (!scsi_trace_transfer_active) {
		return;
	}
	scsi_trace_transfer_result_pending = TRUE;
	scsi_trace_transfer_result_status = status;
	scsi_trace_transfer_irq_requests++;
	if (scsi_trace_transfer_phase == SCSIPH_COMMAND) {
		scsi_trace_transfer_cdb_length = min(scsiio.wrdatpos,
				(UINT)sizeof(scsi_trace_transfer_cdb));
		if (scsi_trace_transfer_cdb_length) {
			CopyMemory(scsi_trace_transfer_cdb, scsiio.cmd,
					scsi_trace_transfer_cdb_length);
		}
	}
}

static void scsi_trace_transfer_event_result(void) {

	if (!scsi_trace_transfer_active ||
			!scsi_trace_transfer_result_pending) {
		return;
	}
	scsi_tracef("scsitrace transfer-result phase=%02x direction=%s "
			"tc=%06x ar19_accesses=%u ar19_reads=%u ar19_writes=%u "
			"data_port_accesses=%u irq_requests=%u irq_assertions=%u "
			"csr=%02x source=%s cdb_len=%u cdb0=%02x cdb1=%02x "
			"cdb2=%02x cdb3=%02x cdb4=%02x cdb5=%02x cdb6=%02x "
			"cdb7=%02x cdb8=%02x cdb9=%02x cdb10=%02x cdb11=%02x",
			scsi_trace_transfer_phase,
			scsi_trace_phase_direction(scsi_trace_transfer_phase),
			scsi_trace_transfer_count,
			scsi_trace_transfer_ar19_accesses,
			scsi_trace_transfer_ar19_reads,
			scsi_trace_transfer_ar19_writes,
			scsi_trace_transfer_data_port_accesses,
			scsi_trace_transfer_irq_requests,
			scsi_trace_transfer_irq_assertions,
			scsi_trace_transfer_result_status,
			scsi_trace_transfer_source,
			scsi_trace_transfer_cdb_length,
			scsi_trace_transfer_cdb[0], scsi_trace_transfer_cdb[1],
			scsi_trace_transfer_cdb[2], scsi_trace_transfer_cdb[3],
			scsi_trace_transfer_cdb[4], scsi_trace_transfer_cdb[5],
			scsi_trace_transfer_cdb[6], scsi_trace_transfer_cdb[7],
			scsi_trace_transfer_cdb[8], scsi_trace_transfer_cdb[9],
			scsi_trace_transfer_cdb[10], scsi_trace_transfer_cdb[11]);
	scsi_trace_transfer_active = FALSE;
	scsi_trace_transfer_result_pending = FALSE;
	if (scsi_trace_completion_limit != 0) {
		scsi_trace_completion_count++;
		if (scsi_trace_completion_count >= scsi_trace_completion_limit) {
			scsi_trace_stop = TRUE;
		}
	}
}

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

static REG8 scsiio_auxstatus(void) {

	REG8 ret;

	ret = scsiio.auxstatus & (SCSI_AUX_LCI | SCSI_AUX_BSY |
			SCSI_AUX_CIP | SCSI_AUX_PE | SCSI_AUX_DBR);
	if (scsi_csr_latched) {
		ret |= SCSI_AUX_INT;
	}
	return ret;
}

static void scsiio_warn_reserved_register(const char *direction) {

	SCSITRACEOUT(("scsitrace warning reserved register range ar=%02x %s "
			"hardware-pending cs=%04x ip=%04x", scsiio.port, direction,
			CPU_CS, CPU_IP));
}

static UINT scsiio_transfer_count(void) {

	return ((UINT)scsiio.reg[SCSICTR_TRANSCNT + 0] << 16) |
			((UINT)scsiio.reg[SCSICTR_TRANSCNT + 1] << 8) |
			(UINT)scsiio.reg[SCSICTR_TRANSCNT + 2];
}

static void scsiio_decrement_transfer_count(void) {

	if (scsiio.reg[SCSICTR_TRANSCNT + 2]) {
		scsiio.reg[SCSICTR_TRANSCNT + 2]--;
	}
	else if (scsiio.reg[SCSICTR_TRANSCNT + 1]) {
		scsiio.reg[SCSICTR_TRANSCNT + 1]--;
		scsiio.reg[SCSICTR_TRANSCNT + 2] = 0xff;
	}
	else if (scsiio.reg[SCSICTR_TRANSCNT + 0]) {
		scsiio.reg[SCSICTR_TRANSCNT + 0]--;
		scsiio.reg[SCSICTR_TRANSCNT + 1] = 0xff;
		scsiio.reg[SCSICTR_TRANSCNT + 2] = 0xff;
	}
}

static const char *scsi_transfer_state_name(void) {

	switch (scsi_transfer_state) {
		case SCSI_TRANSFER_IDLE: return "idle";
		case SCSI_TRANSFER_WAIT_FOR_REQ: return "wait_for_req";
		case SCSI_TRANSFER_BYTE_PENDING: return "transfer_byte_pending";
		case SCSI_TRANSFER_WAIT_FOR_POST_COUNT_REQ:
			return "wait_for_post_count_req";
		case SCSI_TRANSFER_COMPLETED_OR_TERMINATED:
			return "completed_or_terminated";
	}
	return "unknown";
}

static void scsi_trace_command_lifecycle(const char *event, REG8 command) {
	UINT phase = scsiio.phase;
	UINT msg = (phase == SCSIPH_MSGIN || phase == SCSIPH_MSGOUT) ? 1 : 0;
	UINT cd = (phase == SCSIPH_COMMAND || phase == SCSIPH_STATUS ||
			phase == SCSIPH_INFOOUT || phase == SCSIPH_INFOIN) ? 1 : 0;
	UINT io = scsicmd_phase_host_to_spc(phase) ? 0 : 1;

	SCSITRACEOUT(("scsitrace command-%s command=%02x state=%s "
			"int=%u lci=%u bsy=%u cip=%u dbr=%u csr_pending=%u "
			"req=%u ack=%u msg=%u cd=%u io=%u tc=%06x cs=%04x ip=%04x",
			event, command, scsi_transfer_state_name(),
			(scsi_csr_latched ? 1 : 0),
			(scsiio.auxstatus & SCSI_AUX_LCI) ? 1 : 0,
			(scsiio.auxstatus & SCSI_AUX_BSY) ? 1 : 0,
			(scsiio.auxstatus & SCSI_AUX_CIP) ? 1 : 0,
			(scsiio.auxstatus & SCSI_AUX_DBR) ? 1 : 0,
			(scsi_csr_latched ? 1 : 0),
			(scsi_transfer_req_asserted ? 1 : 0),
			(scsi_transfer_ack_asserted ? 1 : 0), msg, cd, io,
			scsiio_transfer_count(), CPU_CS, CPU_IP));
}

static void scsiio_req_assert(REG8 status, const char *kind) {

	if (scsi_transfer_req_asserted) {
		SCSITRACEOUT(("scsitrace warning req-overlap seq=%u kind=%s "
				"status=%02x phase=%02x state=%s cs=%04x ip=%04x",
				scsi_transfer_req_sequence, kind, status, scsiio.phase,
				scsi_transfer_state_name(), CPU_CS, CPU_IP));
		return;
	}
	scsi_transfer_req_asserted = TRUE;
	scsi_transfer_req_sequence++;
	SCSITRACEOUT(("scsitrace req-assert seq=%u kind=%s status=%02x "
			"phase=%02x state=%s tc=%06x cs=%04x ip=%04x",
			scsi_transfer_req_sequence, kind, status, scsiio.phase,
			scsi_transfer_state_name(), scsiio_transfer_count(), CPU_CS, CPU_IP));
}

static void scsiio_ack_assert(void) {

	scsi_transfer_ack_asserted = TRUE;
	SCSITRACEOUT(("scsitrace ack-assert seq=%u req=%u phase=%02x "
			"tc=%06x cs=%04x ip=%04x",
			scsi_transfer_req_sequence, scsi_transfer_req_asserted ? 1 : 0,
			scsiio.phase, scsiio_transfer_count(), CPU_CS, CPU_IP));
}

static void scsiio_ack_negate(void) {

	scsi_transfer_ack_asserted = FALSE;
	SCSITRACEOUT(("scsitrace ack-negate seq=%u phase=%02x tc=%06x "
			"cs=%04x ip=%04x", scsi_transfer_req_sequence, scsiio.phase,
			scsiio_transfer_count(), CPU_CS, CPU_IP));
}

static void scsiio_begin_req_transfer(void) {

	if (!scsi_transfer_req_asserted ||
			scsi_transfer_state != SCSI_TRANSFER_WAIT_FOR_REQ) {
		return;
	}
	scsi_transfer_active_phase = scsiio.phase;
	scsi_transfer_state = SCSI_TRANSFER_BYTE_PENDING;
	scsiio.auxstatus |= SCSI_AUX_BSY;
	scsiio.auxstatus &= (REG8)~SCSI_AUX_CIP;
	scsiio.auxstatus |= SCSI_AUX_DBR;
	SCSITRACEOUT(("scsitrace command-active state=%s req=%u phase=%02x "
			"tc=%06x", scsi_transfer_state_name(),
			scsi_transfer_req_sequence, scsiio.phase, scsiio_transfer_count()));
}

static void scsiio_wait_for_post_count_req(REG8 completion_status,
		REG8 next_status) {

	/* TC is zero, but WD33C93A waits for a distinct next REQ before
	 * reporting the successful-completion MCI. */
	scsi_transfer_state = SCSI_TRANSFER_WAIT_FOR_POST_COUNT_REQ;
	scsi_transfer_completion_status = completion_status;
	scsi_transfer_phase_pending = (next_status != 0x85);
	scsi_transfer_phase_status = next_status;
	scsiio.auxstatus &= (REG8)~SCSI_AUX_CIP;
	SCSITRACEOUT(("scsitrace post-count-wait completion=%02x next=%02x "
			"state=%s tc=%06x", completion_status, next_status,
			scsi_transfer_state_name(), scsiio_transfer_count()));
	scsiio_schedule_target_req(next_status);
}

static void scsiio_next_byte_req(void) {

	scsi_transfer_state = SCSI_TRANSFER_BYTE_PENDING;
	scsiio_req_assert(0, "byte");
	scsiio.auxstatus |= SCSI_AUX_DBR;
}

static BOOL scsiio_terminate_phase_change(void) {
	REG8 status;

	if (scsi_transfer_state != SCSI_TRANSFER_BYTE_PENDING ||
			scsi_transfer_remaining == 0 ||
			scsi_transfer_active_phase == scsiio.phase) {
		return FALSE;
	}
	status = (REG8)(0x40 | scsiio.phase);
	scsi_transfer_state = SCSI_TRANSFER_COMPLETED_OR_TERMINATED;
	scsi_transfer_req_asserted = FALSE;
	scsi_transfer_ack_asserted = FALSE;
	scsiio.auxstatus &= (REG8)~(SCSI_AUX_BSY | SCSI_AUX_DBR | SCSI_AUX_CIP);
	SCSITRACEOUT(("scsitrace phase-change-terminate status=%02x "
			"remaining=%06x active_phase=%02x current_phase=%02x",
			status, scsi_transfer_remaining,
			(unsigned)scsi_transfer_active_phase, scsiio.phase));
	scsiintr_transfer_complete(status);
	return TRUE;
}

static void scsiio_data_write(REG8 dat) {

	BOOL short_transfer;
	REG8 next_status;
	REG8 completed_phase;

	scsi_trace_transfer_ar19_access(TRUE);
	if (scsiio_terminate_phase_change()) {
		return;
	}
	if (!scsicmd_phase_host_to_spc(scsiio.phase)) {
		SCSITRACEOUT(("scsitrace warning DATA write phase-direction-mismatch "
				"phase=%02x cs=%04x ip=%04x", scsiio.phase, CPU_CS, CPU_IP));
		return;
	}
	if (scsi_transfer_state != SCSI_TRANSFER_BYTE_PENDING ||
			!scsi_transfer_req_asserted ||
			!(scsiio.auxstatus & SCSI_AUX_DBR)) {
		SCSITRACEOUT(("scsitrace warning DATA write without pending REQ "
				"state=%s req=%u dbr=%u data=%02x cs=%04x ip=%04x",
				scsi_transfer_state_name(),
				scsi_transfer_req_asserted ? 1 : 0,
				(scsiio.auxstatus & SCSI_AUX_DBR) ? 1 : 0,
				dat, CPU_CS, CPU_IP));
		return;
	}
	scsiio.auxstatus &= (REG8)~SCSI_AUX_DBR;
	scsiio_ack_assert();
	scsi_transfer_req_asserted = FALSE;
	scsi_transfer_bytes++;
	SCSITRACEOUT(("scsitrace ack seq=%u direction=host-to-spc byte=%02x "
			"phase=%02x tc-before=%06x cs=%04x ip=%04x",
			scsi_transfer_req_sequence, dat, scsiio.phase,
			scsiio_transfer_count(), CPU_CS, CPU_IP));
	scsiio_ack_negate();

	SCSITRACEOUT(("scsitrace data-latched direction=host-to-spc "
			"seq=%u byte=%02x phase=%02x", scsi_transfer_req_sequence,
			dat, scsiio.phase));

	if (scsiio.phase == SCSIPH_COMMAND) {
		if (scsiio.wrdatpos < sizeof(scsiio.cmd)) {
			scsiio.cmd[scsiio.wrdatpos] = dat;
		}
		scsiio.wrdatpos++;
		scsiio_decrement_transfer_count();
		if (scsi_transfer_remaining) {
			scsi_transfer_remaining--;
		}
		if (scsi_transfer_remaining != 0) {
			scsiio_next_byte_req();
			return;
		}
		SCSITRACEOUT(("scsitrace command-transfer-complete bytes=%u",
				scsi_transfer_bytes));
		next_status = scsicmd_command(scsiio.reg[SCSICTR_DSTID] & 7);
		scsiio.auxstatus &= (REG8)~SCSI_AUX_DBR;
		scsiio_wait_for_post_count_req(0x1a, next_status);
		return;
	}

	if (scsiio.wrdatpos < sizeof(scsiio.data)) {
		scsiio.data[scsiio.wrdatpos++] = dat;
	}
	scsiio_decrement_transfer_count();
	if (scsi_transfer_remaining) {
		scsi_transfer_remaining--;
	}
	if (scsi_transfer_remaining != 0) {
		scsiio_next_byte_req();
		return;
	}
	completed_phase = (REG8)scsiio.phase;
	next_status = scsicmd_transinfo(scsiio.reg[SCSICTR_DSTID] & 7);
	short_transfer = (scsiio.phase != SCSIPH_DATAOUT &&
			scsi_transfer_payload_length != 0 &&
			scsiio.wrdatpos < scsi_transfer_payload_length);
	if (short_transfer) {
		/* The target changed phase before the programmed count was exhausted. */
		scsi_transfer_state = SCSI_TRANSFER_COMPLETED_OR_TERMINATED;
		scsiio_req_assert(next_status, "phase-change");
		scsiio.auxstatus &= (REG8)~(SCSI_AUX_BSY | SCSI_AUX_DBR | SCSI_AUX_CIP);
		scsiintr_transfer_complete((REG8)(0x40 | scsiio.phase));
		return;
	}
	scsiio_wait_for_post_count_req((REG8)(0x10 | completed_phase), next_status);
}

static REG8 scsiio_data_read(void) {

	REG8 ret;
	REG8 next_status;
	REG8 completed_phase;
	BOOL short_transfer;

	scsi_trace_transfer_ar19_access(FALSE);
	if (scsiio_terminate_phase_change()) {
		return 0xff;
	}
	if (scsicmd_phase_host_to_spc(scsiio.phase)) {
		SCSITRACEOUT(("scsitrace warning DATA read phase-direction-mismatch "
				"phase=%02x cs=%04x ip=%04x", scsiio.phase, CPU_CS, CPU_IP));
		return 0xff;
	}
	if (scsi_transfer_state != SCSI_TRANSFER_BYTE_PENDING ||
			!scsi_transfer_req_asserted ||
			!(scsiio.auxstatus & SCSI_AUX_DBR)) {
		SCSITRACEOUT(("scsitrace warning DATA read without pending REQ "
				"state=%s req=%u dbr=%u cs=%04x ip=%04x",
				scsi_transfer_state_name(),
				scsi_transfer_req_asserted ? 1 : 0,
				(scsiio.auxstatus & SCSI_AUX_DBR) ? 1 : 0,
				CPU_CS, CPU_IP));
		return 0xff;
	}
	scsiio.auxstatus &= (REG8)~SCSI_AUX_DBR;
	scsiio_ack_assert();
	ret = scsiio.data[scsiio.rddatpos & 0xffff];
	SCSITRACEOUT(("scsitrace data-latched direction=spc-to-host "
			"seq=%u byte=%02x phase=%02x", scsi_transfer_req_sequence,
			ret, scsiio.phase));
	scsiio.rddatpos++;
	scsiio_decrement_transfer_count();
	if (scsi_transfer_remaining) {
		scsi_transfer_remaining--;
	}
	scsi_transfer_req_asserted = FALSE;
	scsi_transfer_bytes++;
	SCSITRACEOUT(("scsitrace ack seq=%u direction=spc-to-host byte=%02x "
			"phase=%02x tc-after=%06x cs=%04x ip=%04x",
			scsi_transfer_req_sequence, ret, scsiio.phase,
			scsiio_transfer_count(), CPU_CS, CPU_IP));
	scsiio_ack_negate();

	short_transfer = (scsiio.phase == SCSIPH_DATAIN &&
			scsi_transfer_remaining != 0 &&
			scsi_transfer_payload_length != 0 &&
			scsiio.rddatpos >= scsi_transfer_payload_length);
	if (short_transfer) {
		next_status = scsicmd_transinfo(scsiio.reg[SCSICTR_DSTID] & 7);
		scsi_transfer_state = SCSI_TRANSFER_COMPLETED_OR_TERMINATED;
		scsiio_req_assert(next_status, "phase-change");
		scsiio.auxstatus &= (REG8)~(SCSI_AUX_BSY | SCSI_AUX_DBR | SCSI_AUX_CIP);
		scsiintr_transfer_complete((REG8)(0x40 | scsiio.phase));
		return ret;
	}
	if (scsi_transfer_remaining != 0) {
		scsiio_next_byte_req();
		return ret;
	}
	completed_phase = (REG8)scsiio.phase;
	next_status = scsicmd_transinfo(scsiio.reg[SCSICTR_DSTID] & 7);
	scsiio_wait_for_post_count_req((REG8)(0x10 | completed_phase), next_status);
	return ret;
}

void scsiio_trace_enable(BOOL enabled) {

	scsi_trace_enabled = enabled;
}

void scsiio_trace_limit(UINT limit) {

	scsi_trace_completion_limit = limit;
	scsi_trace_completion_count = 0;
	scsi_trace_stop = FALSE;
}

BOOL scsiio_trace_stop_requested(void) {

	return scsi_trace_stop;
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
	if (scsi_csr_latched) {
		SCSITRACEOUT(("scsitrace warning csr-overwrite-blocked status=%02x "
				"cs=%04x ip=%04x", scsi_csr_event_status, CPU_CS, CPU_IP));
		(void)item;
		return;
	}
	scsiio.scsistatus = scsi_csr_event_status;
	scsi_csr_latched = TRUE;
	scsiio.auxstatus &= (REG8)~SCSI_AUX_CIP;
	SCSITRACEOUT(("scsitrace csr-latch status=%02x aux=%02x phase=%02x "
			"state=%s req=%u cs=%04x ip=%04x",
			scsiio.scsistatus, scsiio_auxstatus(), scsiio.phase,
			scsi_transfer_state_name(),
			scsi_transfer_req_asserted ? 1 : 0, CPU_CS, CPU_IP));
	TRACEOUT(("scsiioint"));
	if (scsiio.membank & 4) {
		if (scsi_trace_transfer_active &&
				scsi_trace_transfer_result_pending) {
			scsi_trace_transfer_irq_assertions++;
		}
		pic_setirq(scsiirq[(scsiio.resent >> 3) & 7]);
		TRACEOUT(("scsi intr"));
	}
	scsi_trace_transfer_event_result();
	(void)item;
}

static void scsiintr_transfer_complete(REG8 status) {

	scsi_trace_transfer_result(status);
	scsiintr_immediate(status);
}

static void scsiintr_immediate(REG8 status) {

	if (scsi_csr_event_active || scsi_csr_latched) {
		SCSITRACEOUT(("scsitrace csr-drop status=%02x reason=csr-pending "
				"state=%s cs=%04x ip=%04x", status,
				scsi_transfer_state_name(), CPU_CS, CPU_IP));
		return;
	}
	scsi_csr_event_active = TRUE;
	scsi_csr_event_status = status;
	nevent_set(NEVENT_SCSIIO, 100, scsiioint, NEVENT_ABSOLUTE);
}

static void scsiintr(REG8 status) {

	scsi_trace_transfer_result(status);
	if (scsi_csr_event_active || scsi_csr_latched) {
		SCSITRACEOUT(("scsitrace csr-drop status=%02x reason=csr-pending "
				"state=%s cs=%04x ip=%04x", status,
				scsi_transfer_state_name(), CPU_CS, CPU_IP));
		return;
	}
	scsi_csr_event_active = TRUE;
	scsi_csr_event_status = status;
	nevent_set(NEVENT_SCSIIO, 4000, scsiioint, NEVENT_ABSOLUTE);
	SCSITRACEOUT(("scsitrace csr-candidate status=%02x phase=%02x state=%s "
			"cs=%04x ip=%04x", status, scsiio.phase,
			scsi_transfer_state_name(), CPU_CS, CPU_IP));
	TRACEOUT(("scsi schedule intr"));
}


static void scsicmd(REG8 cmd) {

	REG8 ret;
	UINT8 id;
	UINT count;

	id = scsiio.reg[SCSICTR_DSTID] & 7;
	switch(cmd) {
		case SCSICMD_RESET:
			scsiio.phase = 0;
			scsiio.cmdpos = 0;
			scsiio.rddatpos = 0;
			scsiio.wrdatpos = 0;
			scsiio.auxstatus = 0;
			scsi_connected_as_initiator = FALSE;
			scsi_command_phase_pending = FALSE;
			scsi_transfer_phase_pending = FALSE;
			scsi_transfer_req_asserted = FALSE;
			scsi_transfer_ack_asserted = FALSE;
			scsi_transfer_remaining = 0;
			scsi_transfer_payload_length = 0;
			scsi_transfer_state = SCSI_TRANSFER_IDLE;
			scsiintr(SCSISTAT_RESET);
			break;

		case SCSICMD_NEGATE:
			ret = scsicmd_negate(id);
			scsi_connected_as_initiator = FALSE;
			scsi_transfer_req_asserted = FALSE;
			scsi_transfer_ack_asserted = FALSE;
			scsi_transfer_state = SCSI_TRANSFER_COMPLETED_OR_TERMINATED;
			scsiio.auxstatus &= (REG8)~(SCSI_AUX_BSY | SCSI_AUX_DBR |
					SCSI_AUX_CIP);
			scsiintr(ret);
			break;

		case SCSICMD_SEL:
			scsiio.auxstatus |= SCSI_AUX_BSY;
			ret = scsicmd_select(id);
			if (ret & 0x80) {
				/* SELECT is complete before the COMMAND REQ is exposed. */
				scsi_connected_as_initiator = TRUE;
				scsiio.auxstatus &= (REG8)~SCSI_AUX_BSY;
				scsi_command_phase_pending = TRUE;
				scsi_transfer_state = SCSI_TRANSFER_IDLE;
				scsiintr(0x11);
			}
			else {
				scsiio.auxstatus &= (REG8)~SCSI_AUX_BSY;
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
			count = scsiio_transfer_count();
			scsi_trace_transfer_start(scsiio.phase, count,
					scsiio.phase == SCSIPH_COMMAND ?
					"m75c2-ar19-pio" : "level2-transfer-info");
			scsiio.auxstatus |= SCSI_AUX_BSY;
			scsiio.auxstatus &= (REG8)~(SCSI_AUX_CIP | SCSI_AUX_DBR);
		scsi_transfer_remaining = count;
		scsi_transfer_bytes = 0;
		scsi_transfer_payload_length = count;
		if (scsiio.phase == SCSIPH_COMMAND) {
			/* M75c2 accumulates CDB through DATA window. */
			scsiio.wrdatpos = 0;
		}
		else {
			scsiio.rddatpos = 0;
			if (scsiio.phase == SCSIPH_STATUS) {
				/* SCSI status is GOOD for the supported commands. */
				scsiio.data[0] = scsiio.reg[SCSICTR_STATUS];
				scsi_transfer_payload_length = 1;
			}
			else if (scsiio.phase == SCSIPH_MSGIN) {
				/* COMMAND COMPLETE message. */
				scsiio.data[0] = 0x00;
				scsi_transfer_payload_length = 1;
			}
			else if (scsiio.phase == SCSIPH_DATAIN) {
				scsi_transfer_payload_length = scsiio.cmdpos;
			}
		}
		if (count == 0) {
			SCSITRACEOUT(("scsitrace warning Transfer Info with TC=0 "
					"phase=%02x hardware-pending", scsiio.phase));
			scsi_transfer_state = SCSI_TRANSFER_COMPLETED_OR_TERMINATED;
			scsiio.auxstatus &= (REG8)~SCSI_AUX_BSY;
			break;
		}
		scsi_transfer_state = SCSI_TRANSFER_WAIT_FOR_REQ;
		SCSITRACEOUT(("scsitrace command-accepted command=%02x state=%s "
				"tc=%06x req=%u phase=%02x cs=%04x ip=%04x", cmd,
				scsi_transfer_state_name(), count,
				scsi_transfer_req_asserted ? 1 : 0,
				scsiio.phase, CPU_CS, CPU_IP));
		if (scsi_transfer_req_asserted) {
			scsiio_begin_req_transfer();
		}
		break;
	}
}

#define SCSI_TARGET_PROCESSING_CLOCKS 4000

static void scsiio_schedule_target_req(REG8 status) {

	if (scsi_target_req_scheduled) {
		SCSITRACEOUT(("scsitrace warning target-req-overlap status=%02x "
				"state=%s cs=%04x ip=%04x", status,
				scsi_transfer_state_name(), CPU_CS, CPU_IP));
		return;
	}
	scsi_target_req_scheduled = TRUE;
	scsi_target_req_status = status;
	SCSITRACEOUT(("scsitrace target-req-scheduled status=%02x delay=%u "
				"state=%s cs=%04x ip=%04x", status,
				(unsigned)SCSI_TARGET_PROCESSING_CLOCKS,
				scsi_transfer_state_name(), CPU_CS, CPU_IP));
	nevent_set(NEVENT_SCSIIO, SCSI_TARGET_PROCESSING_CLOCKS,
			scsiio_target_req_event, NEVENT_ABSOLUTE);
}

static void scsiio_target_req_event(NEVENTITEM item) {

	REG8 status;

	(void)item;
	if (!scsi_target_req_scheduled) {
		return;
	}
	scsi_target_req_scheduled = FALSE;
	status = scsi_target_req_status;
	SCSITRACEOUT(("scsitrace target-req-ready status=%02x state=%s "
				"cs=%04x ip=%04x", status,
				scsi_transfer_state_name(), CPU_CS, CPU_IP));
	scsiio_target_req(status);
}

static void scsiio_target_req(REG8 status) {

	/* A post-count REQ completes the active Level-II command.  The REQ
	 * is distinct from the byte that reduced TC to zero. */
	if (scsi_transfer_state == SCSI_TRANSFER_WAIT_FOR_POST_COUNT_REQ) {
		scsiio_req_assert(status, "post-count");
		scsi_transfer_req_asserted = FALSE;
		scsi_transfer_state = SCSI_TRANSFER_COMPLETED_OR_TERMINATED;
		scsiio.auxstatus &= (REG8)~(SCSI_AUX_BSY | SCSI_AUX_DBR | SCSI_AUX_CIP);
		SCSITRACEOUT(("scsitrace post-count-req seq=%u status=%02x "
				"completion=%02x", scsi_transfer_req_sequence, status,
				scsi_transfer_completion_status));
		scsiintr_transfer_complete(scsi_transfer_completion_status);
		return;
	}

	/* A target REQ is consumed by an active Level-II command.  Only an
	 * idle target may expose the same bus event as 8MCI Service Required. */
	if (scsi_transfer_state == SCSI_TRANSFER_WAIT_FOR_REQ) {
		scsiio_req_assert(status, "active");
		scsiio_begin_req_transfer();
		return;
	}
	if (scsi_transfer_state == SCSI_TRANSFER_BYTE_PENDING) {
		SCSITRACEOUT(("scsitrace warning req-while-byte-pending status=%02x "
				"req=%u state=%s", status,
				scsi_transfer_req_asserted ? 1 : 0,
				scsi_transfer_state_name()));
		return;
	}
	if (scsi_transfer_state != SCSI_TRANSFER_IDLE ||
			scsi_transfer_req_asserted) {
		SCSITRACEOUT(("scsitrace warning req-without-idle status=%02x "
				"state=%s req=%u", status, scsi_transfer_state_name(),
				scsi_transfer_req_asserted ? 1 : 0));
		return;
	}
	if (scsi_connected_as_initiator && !scsi_csr_latched &&
			!scsi_csr_event_active) {
		/* 8MCI is only an idle-target report. */
		scsiio_req_assert(status, "service");
		SCSITRACEOUT(("scsitrace service-required status=%02x connected=1 "
				"active=0 int=0 req=%u", status,
				scsi_transfer_req_asserted ? 1 : 0));
		scsiintr_immediate(status);
	}
}

static void scsiio_command_write(REG8 command) {

	scsi_trace_command_lifecycle("write", command);
	if (scsi_csr_latched || scsi_csr_event_active) {
		scsiio.auxstatus |= SCSI_AUX_LCI;
		SCSITRACEOUT(("scsitrace command-ignored reason=int-pending "
				"command=%02x lci=1 cs=%04x ip=%04x", command,
				CPU_CS, CPU_IP));
		return;
	}
	if (scsi_transfer_state == SCSI_TRANSFER_BYTE_PENDING ||
			scsi_transfer_state == SCSI_TRANSFER_WAIT_FOR_REQ ||
			scsi_transfer_state == SCSI_TRANSFER_WAIT_FOR_POST_COUNT_REQ) {
		scsiio.auxstatus |= SCSI_AUX_LCI;
		SCSITRACEOUT(("scsitrace command-ignored reason=level2-active "
				"command=%02x lci=1 cs=%04x ip=%04x", command,
				CPU_CS, CPU_IP));
		return;
	}
	scsiio.reg[SCSICTR_CMD] = command;
	scsiio.auxstatus |= SCSI_AUX_CIP;
	scsi_trace_command_lifecycle("accepted", command);
	scsiio.auxstatus &= (REG8)~SCSI_AUX_CIP;
	scsicmd(command);
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
	if (scsiio.port == SCSICTR_DATA) {
		scsiio_data_write(dat);
		return;
	}
	if (scsiio.port == SCSICTR_CMD) {
		scsiio_command_write(dat);
		return;
	}
	if (scsiio.port <= 0x19) {
		scsiio.reg[scsiio.port] = dat;
		/* COMMAND and DATA are fixed windows; other registers advance. */
		scsiio.port++;
	}
	else {
		SCSITRACEOUT(("scsitrace out port=0cc2 ar=%02x data=%02x cs=%04x ip=%04x",
				scsiio.port, dat, CPU_CS, CPU_IP));
		switch(scsiio.port) {
			case SCSICTR_MEMBANK:
				scsiio.membank = dat;
				break;
			case SCSICTR_MEMWND:
				scsiio.memwnd = dat;
				break;
			case SCSICTR_PKGID:
			case SCSICTR_RESENT:
			case SCSICTR_FIFO_CTRL:
			case SCSICTR_FIFO_STATUS:
			SCSITRACEOUT(("scsitrace warning AR=%02x write is hardware-pending "
					"data=%02x cs=%04x ip=%04x", scsiio.port, dat,
					CPU_CS, CPU_IP));
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

			default:
				/* Undefined AR values are held, not auto-incremented. */
				if (scsiio.port >= 0x1a && scsiio.port < 0x30) {
					scsiio_warn_reserved_register("write");
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
	if (dat & SCSI_C4_DMER) {
		/* PCPLUS selects polled I/O; DMA remains deliberately disabled. */
		SCSITRACEOUT(("scsitrace 0cc4 DMER reset (PIO-only)"));
	}
	if (dat & (SCSI_C4_TCMS | SCSI_C4_TCMR | SCSI_C4_TCIR |
			SCSI_C4_DMES)) {
		SCSITRACEOUT(("scsitrace warning 0cc4 bits=%02x hardware-pending",
				dat & (SCSI_C4_TCMS | SCSI_C4_TCMR | SCSI_C4_TCIR |
					SCSI_C4_DMES)));
	}
	(void)port;
}

static void IOOUTCALL scsiio_occ6(UINT port, REG8 dat) {

	scsi_trace_transfer_data_port_access();

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

	ret = scsiio_auxstatus();
	SCSITRACEOUT(("scsitrace in port=0cc0 aux=%02x ar=%02x cs=%04x ip=%04x",
			ret, scsiio.port, CPU_CS, CPU_IP));
	(void)port;
	return(ret);
}

static REG8 IOINPCALL scsiio_icc2(UINT port) {

	REG8	ret;

	switch(scsiio.port) {
		case SCSICTR_STATUS:
			if (scsi_csr_latched) {
				scsi_csr_latched = FALSE;
				scsiio.auxstatus &= (REG8)~SCSI_AUX_INT;
				SCSITRACEOUT(("scsitrace csr-read status=%02x aux=%02x "
						"phase=%02x state=%s cs=%04x ip=%04x",
						scsiio.scsistatus, scsiio_auxstatus(), scsiio.phase,
						scsi_transfer_state_name(), CPU_CS, CPU_IP));
				if (scsi_command_phase_pending) {
					/* The target's COMMAND REQ is held until CSR=11h is read. */
					scsi_command_phase_pending = FALSE;
					scsiio_schedule_target_req(0x8a);
				}
				else if (scsi_transfer_phase_pending) {
					REG8 next_status = scsi_transfer_phase_status;
					scsi_transfer_phase_pending = FALSE;
					scsi_transfer_state = SCSI_TRANSFER_IDLE;
					if (next_status == 0x85) {
						/* MESSAGE IN completion releases the bus. */
						scsiintr_immediate(next_status);
					}
					else {
						scsiio_schedule_target_req(next_status);
					}
				}
			}
			SCSITRACEOUT(("scsitrace in port=0cc2 ar=%02x status=%02x cs=%04x ip=%04x",
					scsiio.port, scsiio.scsistatus, CPU_CS, CPU_IP));
			scsiio.port++;
			return(scsiio.scsistatus);

		case SCSICTR_DATA:
			SCSITRACEOUT(("scsitrace in port=0cc2 ar=%02x data-window "
					"cs=%04x ip=%04x", scsiio.port, CPU_CS, CPU_IP));
			return scsiio_data_read();

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

		case SCSICTR_PKGID:
		case SCSICTR_FIFO_CTRL:
		case SCSICTR_FIFO_STATUS:
			SCSITRACEOUT(("scsitrace warning AR=%02x read is hardware-pending "
					"cs=%04x ip=%04x", scsiio.port, CPU_CS, CPU_IP));
			return(0xff);

		case 0x36:
			return(0);					// ２枚刺しとか…
	}
	if (scsiio.port >= 0x1a && scsiio.port < 0x30) {
		scsiio_warn_reserved_register("read");
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

	scsi_trace_transfer_data_port_access();
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
	scsi_connected_as_initiator = FALSE;
	scsi_csr_event_active = FALSE;
	scsi_csr_event_status = 0;
	scsi_command_phase_pending = FALSE;
	scsi_transfer_phase_pending = FALSE;
	scsi_transfer_phase_status = 0;
	scsi_transfer_completion_status = 0;
	scsi_transfer_state = SCSI_TRANSFER_IDLE;
	scsi_transfer_req_asserted = FALSE;
	scsi_transfer_ack_asserted = FALSE;
	scsi_target_req_scheduled = FALSE;
	scsi_target_req_status = 0;
	scsi_transfer_req_sequence = 0;
	scsi_transfer_bytes = 0;
	scsi_transfer_payload_length = 0;
	scsi_transfer_active_phase = 0;
	scsi_trace_transfer_active = FALSE;
	scsi_trace_transfer_phase = 0;
	scsi_trace_transfer_count = 0;
	scsi_trace_transfer_ar19_accesses = 0;
	scsi_trace_transfer_ar19_reads = 0;
	scsi_trace_transfer_ar19_writes = 0;
	scsi_trace_transfer_data_port_accesses = 0;
	scsi_trace_transfer_irq_requests = 0;
	scsi_trace_transfer_irq_assertions = 0;
	scsi_trace_transfer_result_pending = FALSE;
	scsi_trace_transfer_result_status = 0;
	scsi_trace_transfer_cdb_length = 0;
	ZeroMemory(scsi_trace_transfer_cdb, sizeof(scsi_trace_transfer_cdb));
	scsi_trace_transfer_source = NULL;
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
