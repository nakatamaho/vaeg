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
#include	"upd9002_trace.h"


	_SCSIIO		scsiio;

static const UINT8 scsiirq[] = {0x03, 0x05, 0x06, 0x09, 0x0c, 0x0d, 3, 3};
static BOOL scsi_trace_enabled;
static BOOL scsi_trace_compact;
static UINT scsi_trace_completion_limit;
static UINT scsi_trace_completion_count;
static BOOL scsi_trace_stop;
static BOOL scsi_csr_latched;
static BOOL scsi_csr_event_active;
static REG8 scsi_csr_event_status;
static UINT scsi_trace_csr_sequence;
static UINT scsi_csr_event_sequence;
static const char *scsi_csr_event_origin;
static UINT scsi_csr_latched_sequence;
static const char *scsi_csr_latched_origin;
static UINT32 scsi_csr_event_clock;
static UINT32 scsi_csr_latched_clock;
static UINT32 scsi_trace_data_phase_decision_clock;
static BOOL scsi_trace_data_phase_pending;
static BOOL scsi_trace_event_watchdog_reported;
static BOOL scsi_trace_latched_watchdog_reported;
static BOOL scsi_trace_data_phase_watchdog_reported;
static BOOL scsi_trace_watchdog_scheduled;
static BOOL scsi_trace_jitter_enabled;
static UINT32 scsi_trace_jitter_state;
static UINT scsi_trace_jitter_span;
static UINT32 scsi_target_phase_delay_clock;
static BOOL scsi_trace_target_delay_watchdog_reported;
static UINT scsi_trace_data_phase_request_missing_count;
static BOOL scsi_target_selection_pending;
static REG8 scsi_target_selection_status;
static const char *scsi_target_selection_origin;
static BOOL scsi_target_phase_delay_pending;
static BOOL scsi_command_phase_pending;
static BOOL scsi_transfer_phase_pending;
static REG8 scsi_transfer_phase_status;
static BOOL scsi_target_phase_ready;
static UINT scsi_transfer_remaining;
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
static UINT32 scsi_trace_clock(void);
static REG8 scsiio_auxstatus(void);
static void scsi_trace_watchdog(void);
static void scsi_target_publish(void);
static void scsi_target_schedule_after_consume(void);
static void scsi_trace_watchdog_schedule(void);
static UINT scsi_target_processing_clocks(void);

/* WD33C93 auxiliary-status bits.  The DATA window is PIO-only in M75. */
#define SCSI_AUX_INT	0x80
#define SCSI_AUX_LCI	0x40
#define SCSI_AUX_BSY	0x20
#define SCSI_AUX_CIP	0x10
#define SCSI_AUX_PE	0x02
#define SCSI_AUX_DBR	0x01

/* Target processing is a controller event, not a guest-tuned ISR delay. */
#define SCSI_TARGET_PROCESSING_CLOCKS	100
#define SCSI_TRACE_WATCHDOG_CLOCKS 0x40000
#define SCSI_TRACE_JITTER_DEFAULT_SPAN 100

/* 0CC4h uses set/reset strobes for the controller transfer controls. */
#define SCSI_C4_TCMS	0x04
#define SCSI_C4_TCMR	0x08
#define SCSI_C4_TCIR	0x10
#define SCSI_C4_DMER	0x02
#define SCSI_C4_DMES	0x01

static void scsiintr(const char *origin, REG8 status);
static void scsiintr_transfer_complete(REG8 status);
static void scsiintr_enqueue(const char *origin, REG8 status,
		UINT clocks, BOOL record_transfer_result, BOOL target_event);
static void scsiio_target_phase_ready_event(NEVENTITEM item);

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

static void scsiio_schedule_transfer_phase(REG8 status,
		UINT completed_phase) {

	scsi_transfer_phase_status = status;
	if (status == 0x89 && scsi_trace_enabled) {
		scsi_trace_data_phase_pending = TRUE;
		scsi_trace_data_phase_decision_clock = scsi_trace_clock();
		scsi_tracef("scsitrace data-phase-decision status=89 phase=%02x "
				"clock=%u cs=%04x ip=%04x",
				scsiio.phase, scsi_trace_data_phase_decision_clock,
				CPU_CS, CPU_IP);
	}
	/*
	 * Bus free has no following TRANSFER INFO command.  Mark the target
	 * ready when MESSAGE IN completes so that the CSR=1Fh read can release
	 * the pending ending-disconnect status (85h/80h).  All data-bearing
	 * phase changes remain gated by the target-processing event below.
	 */
	if ((status == 0x85) || (status == 0x80)) {
		scsi_transfer_phase_pending = TRUE;
		scsi_target_phase_ready = TRUE;
	}
	else if (scsiio.phase == completed_phase) {
		/* REQ continues within the current phase; DBR may remain available. */
		scsi_transfer_phase_pending = FALSE;
		scsi_target_phase_ready = TRUE;
	}
	else {
		scsi_transfer_phase_pending = TRUE;
		scsi_target_phase_ready = FALSE;
	}
	scsi_trace_watchdog_schedule();
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

static BOOL scsi_trace_compact_line(const char *fmt) {

	if (!scsi_trace_compact) {
		return(TRUE);
	}
	return(strncmp(fmt, "scsitrace transfer-", 19) == 0 ||
			strncmp(fmt, "scsitrace csr-", 14) == 0 ||
			strncmp(fmt, "scsitrace data-read", 19) == 0 ||
			strncmp(fmt, "scsitrace target-phase-wait", 27) == 0 ||
			strncmp(fmt, "scsitrace M75c2", 15) == 0 ||
			strncmp(fmt, "scsitrace warning", 17) == 0 ||
			strncmp(fmt, "scsitrace data-phase-", 21) == 0 ||
			strncmp(fmt, "scsitrace jitter", 16) == 0 ||
			strncmp(fmt, "scsitrace watchdog", 18) == 0 ||
			strncmp(fmt, "scsitrace invariant", 18) == 0);
}

static void scsi_tracef(const char *fmt, ...) {

	va_list ap;

	if (!scsi_trace_compact_line(fmt)) {
		return;
	}
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
static UINT32 scsi_trace_clock(void) {

	return (UINT32)(CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK);
}

static UINT scsi_target_processing_clocks(void) {
	UINT32 sample;
	UINT clocks;

	clocks = SCSI_TARGET_PROCESSING_CLOCKS;
	if (!scsi_trace_jitter_enabled || (scsi_trace_jitter_span == 0)) {
		return(clocks);
	}
	/* Numerical Recipes LCG: deterministic and diagnostic-only. */
	scsi_trace_jitter_state = scsi_trace_jitter_state * 1664525U +
			1013904223U;
	sample = scsi_trace_jitter_state % (scsi_trace_jitter_span + 1);
	clocks += sample;
	SCSITRACEOUT(("scsitrace jitter base=%u span=%u sample=%u clocks=%u seed=%u",
			SCSI_TARGET_PROCESSING_CLOCKS, scsi_trace_jitter_span, sample,
			clocks, scsi_trace_jitter_state));
	return(clocks);
}

static void scsi_trace_watchdog(void) {

	UINT32 now;

	if (!scsi_trace_enabled) {
		return;
	}
	now = scsi_trace_clock();
	if (scsi_csr_event_active && !scsi_trace_event_watchdog_reported &&
			(UINT32)(now - scsi_csr_event_clock) >=
			SCSI_TRACE_WATCHDOG_CLOCKS) {
		scsi_trace_event_watchdog_reported = TRUE;
		SCSITRACEOUT(("scsitrace watchdog scheduled-unpublished status=%02x "
				"seq=%u clocks=%u cs=%04x ip=%04x",
				scsi_csr_event_status, scsi_csr_event_sequence,
				(UINT32)(now - scsi_csr_event_clock), CPU_CS, CPU_IP));
	}
	if (scsi_csr_latched && !scsi_trace_latched_watchdog_reported &&
			(UINT32)(now - scsi_csr_latched_clock) >=
			SCSI_TRACE_WATCHDOG_CLOCKS) {
		scsi_trace_latched_watchdog_reported = TRUE;
		SCSITRACEOUT(("scsitrace watchdog unconsumed-csr status=%02x "
				"seq=%u clocks=%u cs=%04x ip=%04x",
				scsiio.scsistatus, scsi_csr_latched_sequence,
				(UINT32)(now - scsi_csr_latched_clock), CPU_CS, CPU_IP));
	}
	if (scsi_target_phase_delay_pending &&
			!scsi_trace_target_delay_watchdog_reported &&
			(UINT32)(now - scsi_target_phase_delay_clock) >=
			SCSI_TRACE_WATCHDOG_CLOCKS) {
		scsi_trace_target_delay_watchdog_reported = TRUE;
		SCSITRACEOUT(("scsitrace watchdog target-phase-delay clocks=%u "
				"phase=%02x cs=%04x ip=%04x",
				(UINT32)(now - scsi_target_phase_delay_clock),
				scsiio.phase, CPU_CS, CPU_IP));
	}
	if (scsi_trace_data_phase_pending &&
			!scsi_trace_data_phase_watchdog_reported &&
			(UINT32)(now - scsi_trace_data_phase_decision_clock) >=
			SCSI_TRACE_WATCHDOG_CLOCKS) {
		scsi_trace_data_phase_watchdog_reported = TRUE;
		scsi_trace_data_phase_request_missing_count++;
		SCSITRACEOUT(("scsitrace watchdog data-phase-request-missing "
				"status=89 count=%u clocks=%u cs=%04x ip=%04x",
				scsi_trace_data_phase_request_missing_count,
				(UINT32)(now - scsi_trace_data_phase_decision_clock),
				CPU_CS, CPU_IP));
	}
}


static BOOL scsi_trace_watchdog_needed(void) {

	return scsi_trace_enabled && (scsi_csr_event_active ||
			scsi_csr_latched || scsi_trace_data_phase_pending ||
			scsi_target_selection_pending || scsi_command_phase_pending ||
			scsi_transfer_phase_pending || scsi_target_phase_delay_pending);
}

static void scsi_trace_watchdog_schedule(void) {

	if (!scsi_trace_watchdog_needed()) {
		if (scsi_trace_watchdog_scheduled) {
			nevent_reset(NEVENT_SCSIWATCHDOG);
			scsi_trace_watchdog_scheduled = FALSE;
		}
		return;
	}
	nevent_set(NEVENT_SCSIWATCHDOG, SCSI_TRACE_WATCHDOG_CLOCKS,
			scsiio_watchdog_event, NEVENT_ABSOLUTE);
	scsi_trace_watchdog_scheduled = TRUE;
}

void scsiio_watchdog_event(NEVENTITEM item) {

	scsi_trace_watchdog_scheduled = FALSE;
	scsi_trace_watchdog();
	scsi_trace_watchdog_schedule();
	(void)item;
}

static void scsi_trace_csr_record(const char *event, UINT sequence,
		REG8 status, const char *origin) {

	if (!scsi_trace_enabled) {
		return;
	}
	SCSITRACEOUT(("scsitrace csr-%s seq=%u status=%02x origin=%s "
			"phase=%02x ar=%02x aux=%02x membank=%02x "
			"event_active=%u event_status=%02x event_seq=%u event_origin=%s "
			"latched=%u latched_status=%02x latched_seq=%u latched_origin=%s "
			"selection_pending=%u command_pending=%u transfer_pending=%u "
			"transfer_status=%02x target_ready=%u delay_pending=%u "
			"cs=%04x ip=%04x",
			event, sequence, status, origin ? origin : "none",
			scsiio.phase, scsiio.port, scsiio_auxstatus(), scsiio.membank,
			scsi_csr_event_active, scsi_csr_event_status,
			scsi_csr_event_sequence,
			scsi_csr_event_origin ? scsi_csr_event_origin : "none",
			scsi_csr_latched, scsiio.scsistatus, scsi_csr_latched_sequence,
			scsi_csr_latched_origin ? scsi_csr_latched_origin : "none",
			scsi_target_selection_pending, scsi_command_phase_pending,
			scsi_transfer_phase_pending, scsi_transfer_phase_status,
			scsi_target_phase_ready, scsi_target_phase_delay_pending,
			CPU_CS, CPU_IP));
}

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

	/* WD33C93 exposes Transfer Count as high, middle, low (12h-14h). */
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

static void scsiio_data_write(REG8 dat) {

	/*
	 * AR=19h is the WD33C93 DATA register.  It is a fixed window, not a
	 * byte in the auto-incremented register file.  The current M75 path is
	 * polled I/O; the target-side transfer completes immediately after the
	 * host byte is accepted, so DBR is ready again for the next byte.
	 */
	scsi_trace_transfer_ar19_access(TRUE);
	if (!(scsiio.auxstatus & SCSI_AUX_DBR)) {
		SCSITRACEOUT(("scsitrace warning DATA write while DBR=0 data=%02x "
				"cs=%04x ip=%04x", dat, CPU_CS, CPU_IP));
		return;
	}
	scsiio.auxstatus &= (REG8)~SCSI_AUX_DBR;
	if (scsiio.phase == SCSIPH_COMMAND) {
		/* M75c2 accumulates CDB through DATA window. */
		if (scsiio.wrdatpos < sizeof(scsiio.cmd)) {
			scsiio.cmd[scsiio.wrdatpos] = dat;
		}
		scsiio.wrdatpos++;
		scsiio_decrement_transfer_count();
		if (scsi_transfer_remaining) {
			scsi_transfer_remaining--;
		}
		if (scsi_transfer_remaining == 0) {
			REG8 next_status;

			SCSITRACEOUT(("scsitrace M75c2 CDB transfer complete "
					"count=%u", scsiio.wrdatpos));
			next_status = scsicmd_command(scsiio.reg[SCSICTR_DSTID] & 7);
			SCSITRACEOUT(("scsitrace command-response opcode=%02x "
						"response_len=%u rddatpos=%u next_status=%02x "
						"phase=%02x cs=%04x ip=%04x",
						scsiio.cmd[0], scsiio.cmdpos, scsiio.rddatpos,
						next_status, scsiio.phase, CPU_CS, CPU_IP));
			scsiio_schedule_transfer_phase(next_status, SCSIPH_COMMAND);
			scsiintr_transfer_complete(0x1a);
			return;
		}
		scsiio.auxstatus |= SCSI_AUX_DBR;
		return;
	}
	if (scsiio.wrdatpos < sizeof(scsiio.data)) {
		scsiio.data[scsiio.wrdatpos++] = dat;
	}
	scsiio_decrement_transfer_count();
	if (scsi_transfer_remaining) {
		scsi_transfer_remaining--;
	}
	if (scsi_transfer_remaining == 0) {
		REG8 completed_phase;
		REG8 next_status;

		completed_phase = scsiio.phase;
		next_status = scsicmd_transinfo(scsiio.reg[SCSICTR_DSTID] & 7);
		scsiio_schedule_transfer_phase(next_status, completed_phase);
		scsiintr_transfer_complete((REG8)(0x10 | completed_phase));
		return;
	}
	scsiio.auxstatus |= SCSI_AUX_DBR;
}

static REG8 scsiio_data_read(void) {

	REG8 ret;

	scsi_trace_transfer_ar19_access(FALSE);
	if (!(scsiio.auxstatus & SCSI_AUX_DBR)) {
		SCSITRACEOUT(("scsitrace warning DATA read while DBR=0 "
				"cs=%04x ip=%04x", CPU_CS, CPU_IP));
		return 0xff;
	}
	scsiio.auxstatus &= (REG8)~SCSI_AUX_DBR;
	ret = scsiio.data[scsiio.rddatpos & 0xffff];
	SCSITRACEOUT(("scsitrace data-read ar=19 data=%02x index=%u cs=%04x ip=%04x",
			ret, scsiio.rddatpos, CPU_CS, CPU_IP));
	scsiio.rddatpos++;
	SCSITRACEOUT(("scsitrace data-read-state rddatpos=%u cmdpos=%u "
			"remaining=%u phase=%02x cs=%04x ip=%04x",
			scsiio.rddatpos, scsiio.cmdpos, scsi_transfer_remaining,
			scsiio.phase, CPU_CS, CPU_IP));
	scsiio_decrement_transfer_count();
	if (scsi_transfer_remaining) {
		scsi_transfer_remaining--;
	}
	if ((scsiio.phase == SCSIPH_DATAIN) &&
			scsiio.rddatpos >= scsiio.cmdpos &&
			scsi_transfer_remaining) {
		REG8 completed_phase;
		REG8 next_status;
		REG8 short_status;
		UINT transferred;

		/*
		 * The target exhausted its response before the host-programmed
		 * allocation count.  Preserve the residual TC and report the
		 * information-phase change; the host then starts STATUS normally.
		 */
		completed_phase = scsiio.phase;
		transferred = scsiio.rddatpos;
		next_status = scsicmd_transinfo(scsiio.reg[SCSICTR_DSTID] & 7);
		short_status = scsicmd_phase_unexpected_status(scsiio.phase);
		scsiio_schedule_transfer_phase(next_status, completed_phase);
		SCSITRACEOUT(("scsitrace short-data-phase completed=%u residual=%u "
				"status=%02x", transferred, scsi_transfer_remaining,
				short_status));
		scsiintr_transfer_complete(short_status);
		return ret;
	}
	if (scsi_transfer_remaining == 0) {
		REG8 completed_phase;
		REG8 next_status;

		completed_phase = scsiio.phase;
		if ((scsiio.phase == SCSIPH_DATAIN) &&
				scsiio.rddatpos < scsiio.cmdpos) {
			/* The allocation ended first; discard the unrequested suffix. */
			scsiio.phase = SCSIPH_STATUS;
			scsiio.rddatpos = 0;
			next_status = scsicmd_phase_service_status(SCSIPH_STATUS);
		}
		else {
			next_status = scsicmd_transinfo(scsiio.reg[SCSICTR_DSTID] & 7);
		}
		SCSITRACEOUT(("scsitrace data-read-next rddatpos=%u cmdpos=%u "
				"remaining=%u next_status=%02x phase=%02x cs=%04x ip=%04x",
				scsiio.rddatpos, scsiio.cmdpos, scsi_transfer_remaining,
				next_status, scsiio.phase, CPU_CS, CPU_IP));
		scsiio_schedule_transfer_phase(next_status, completed_phase);
		scsiintr_transfer_complete((REG8)(0x10 | completed_phase));
		return ret;
	}
	/* The emulated target produces the next byte without a host-visible wait. */
	scsiio.auxstatus |= SCSI_AUX_DBR;
	return ret;
}

void scsiio_trace_enable(BOOL enabled) {

	scsi_trace_enabled = enabled;
	if (enabled) {
		scsi_trace_watchdog_schedule();
	}
	else {
		nevent_reset(NEVENT_SCSIWATCHDOG);
		scsi_trace_watchdog_scheduled = FALSE;
	}
}

void scsiio_trace_compact(BOOL compact) {

	scsi_trace_compact = compact;
}

void scsiio_trace_limit(UINT limit) {

	scsi_trace_completion_limit = limit;
	scsi_trace_completion_count = 0;
	scsi_trace_data_phase_request_missing_count = 0;
	scsi_trace_stop = FALSE;
}

void scsiio_trace_jitter(BOOL enabled, UINT seed, UINT span) {

	scsi_trace_jitter_enabled = enabled;
	scsi_trace_jitter_state = seed;
	scsi_trace_jitter_span = span ? span : SCSI_TRACE_JITTER_DEFAULT_SPAN;
	if (enabled) {
		SCSITRACEOUT(("scsitrace jitter-config seed=%u span=%u base=%u",
				seed, scsi_trace_jitter_span, SCSI_TARGET_PROCESSING_CLOCKS));
	}
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


static void scsi_target_publish(void) {

	if (scsi_csr_event_active || scsi_csr_latched ||
		scsi_target_phase_delay_pending) {
		return;
	}
	if (scsi_target_selection_pending) {
		const char *origin = scsi_target_selection_origin;
		REG8 status = scsi_target_selection_status;

		scsi_target_selection_pending = FALSE;
		scsiintr_enqueue(origin ? origin : "select-result", status,
			scsi_target_processing_clocks(), FALSE, TRUE);
		return;
	}
	if (scsi_command_phase_pending) {
		scsi_command_phase_pending = FALSE;
		scsiintr_enqueue("select-command-phase", 0x8a,
			scsi_target_processing_clocks(), FALSE, TRUE);
		return;
	}
	if (scsi_transfer_phase_pending && scsi_target_phase_ready) {
		scsi_transfer_phase_pending = FALSE;
		scsi_target_phase_ready = FALSE;
		scsiintr_enqueue("target-phase-ready", scsi_transfer_phase_status,
			scsi_target_processing_clocks(), FALSE, TRUE);
	}
}

static void scsi_target_schedule_after_consume(void) {

	if (scsi_csr_event_active || scsi_csr_latched ||
		scsi_target_phase_delay_pending) {
		return;
	}
	if (scsi_target_selection_pending || scsi_command_phase_pending) {
		scsi_target_phase_delay_pending = TRUE;
		scsi_target_phase_delay_clock = scsi_trace_clock();
		scsi_trace_target_delay_watchdog_reported = FALSE;
		nevent_set(NEVENT_SCSIIO, scsi_target_processing_clocks(),
			scsiio_target_phase_ready_event, NEVENT_ABSOLUTE);
	}
	else {
		scsi_target_publish();
	}
	scsi_trace_watchdog_schedule();
}

void scsiioint(NEVENTITEM item) {

	scsi_csr_event_active = FALSE;
	scsi_trace_watchdog();
	scsiio.scsistatus = scsi_csr_event_status;
	scsi_csr_latched_sequence = scsi_csr_event_sequence;
	scsi_csr_latched_origin = scsi_csr_event_origin;
	upd9002_guest_trace_scsi_status(scsi_csr_event_status);
	scsi_csr_latched = TRUE;
	scsi_csr_latched_clock = scsi_trace_clock();
	scsi_trace_latched_watchdog_reported = FALSE;
	scsi_trace_watchdog_schedule();
	scsi_trace_csr_record("latch", scsi_csr_latched_sequence,
			scsiio.scsistatus, scsi_csr_latched_origin);
	scsiio.auxstatus &= (REG8)~SCSI_AUX_CIP;
	if ((scsi_csr_event_status & 0x80) &&
			!scsicmd_phase_host_to_spc(scsiio.phase)) {
		/* DBR is asserted only after the service request is visible. */
		scsiio.auxstatus |= SCSI_AUX_DBR;
	}
	SCSITRACEOUT(("scsitrace event irq=%u cs=%04x ip=%04x aux=%02x "
				"status=%02x phase=%02x membank=%02x",
				scsiirq[(scsiio.resent >> 3) & 7], CPU_CS, CPU_IP,
				scsiio.auxstatus, scsiio.scsistatus, scsiio.phase,
				scsiio.membank));
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

static void scsiio_target_phase_ready_event(NEVENTITEM item) {

	scsi_target_phase_delay_pending = FALSE;
	scsi_trace_target_delay_watchdog_reported = FALSE;
	scsi_target_phase_ready = TRUE;
	scsi_target_publish();
	(void)item;
}

static void scsiintr_enqueue(const char *origin, REG8 status,
		UINT clocks, BOOL record_transfer_result, BOOL target_event) {
	UINT sequence;

	scsi_trace_watchdog();
	if (status == 0x89 && scsi_trace_data_phase_pending) {
		SCSITRACEOUT(("scsitrace data-phase-request status=89 delta=%u "
				"cs=%04x ip=%04x",
				(UINT32)(scsi_trace_clock() -
					scsi_trace_data_phase_decision_clock),
				CPU_CS, CPU_IP));
		scsi_trace_data_phase_pending = FALSE;
	}

	if (record_transfer_result) {
		scsi_trace_transfer_result(status);
	}
	sequence = scsi_trace_enabled ? ++scsi_trace_csr_sequence : 0;
	scsi_trace_csr_record("request", sequence, status, origin);
	if (scsi_csr_event_active || scsi_csr_latched) {
		scsi_trace_csr_record("overrun", sequence, status, origin);
		SCSITRACEOUT(("scsitrace invariant %s-overlap seq=%u status=%02x "
				"active=%u active_status=%02x latched=%u cs=%04x ip=%04x",
				target_event ? "target" : "host", sequence, status,
				scsi_csr_event_active, scsi_csr_event_status,
				scsi_csr_latched, CPU_CS, CPU_IP));
		scsi_trace_csr_record("drop", sequence, status, origin);
		/* WD33C93 exposes one CSR; a second event is not queued. */
		return;
	}
	scsi_csr_event_active = TRUE;
	scsi_csr_event_status = status;
	scsi_csr_event_sequence = sequence;
	scsi_csr_event_origin = origin;
	scsi_csr_event_clock = scsi_trace_clock();
	scsi_trace_event_watchdog_reported = FALSE;
	nevent_set(NEVENT_SCSIIO, clocks, scsiioint, NEVENT_ABSOLUTE);
	scsi_trace_watchdog_schedule();
	SCSITRACEOUT(("scsitrace request status=%02x phase=%02x cs=%04x ip=%04x",
			status, scsiio.phase, CPU_CS, CPU_IP));
	TRACEOUT(("scsi schedule intr"));
}

static void scsiintr_transfer_complete(REG8 status) {

	scsiintr_enqueue("transfer-complete", status, 100, TRUE, FALSE);
}

static void scsiintr(const char *origin, REG8 status) {

	scsiintr_enqueue(origin, status, 4000, TRUE, FALSE);
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
			scsiio.auxstatus = 0;
			scsi_command_phase_pending = FALSE;
			scsi_transfer_remaining = 0;
			scsiintr("reset", SCSISTAT_RESET);
			break;

		case SCSICMD_NEGATE:
			ret = scsicmd_negate(id);
			scsiio.auxstatus &= (REG8)~(SCSI_AUX_BSY | SCSI_AUX_DBR);
			scsiintr("negate", ret);
			break;

		case SCSICMD_SEL:
			scsiio.auxstatus |= SCSI_AUX_BSY;
			ret = scsicmd_select(id);
			if (ret & 0x80) {
				scsi_command_phase_pending = TRUE;
				scsi_target_selection_origin = "select-complete";
			}
			else {
				scsi_target_selection_origin = "select-error";
			}
			scsi_target_selection_status = (ret & 0x80) ? 0x11 : ret;
			scsi_target_selection_pending = TRUE;
			scsi_target_schedule_after_consume();
			break;

		case SCSICMD_SEL_TR:
			ret = scsicmd_transfer(id, scsiio.reg + SCSICTR_CDB);
			if (ret != 0xff) {
				scsiintr("select-transfer", ret);
			}
			break;

		case SCSICMD_TRANS_INFO:
			scsi_trace_transfer_start(scsiio.phase,
					scsiio_transfer_count(),
				scsiio.phase == SCSIPH_COMMAND ?
					"m75c2-ar19-pio" : "legacy-scsi-phase-engine");
			scsiio.auxstatus |= (SCSI_AUX_BSY | SCSI_AUX_DBR);
			if (scsiio.phase == SCSIPH_COMMAND) {
				scsi_transfer_remaining = scsiio_transfer_count();
				scsiio.wrdatpos = 0;
				if (scsi_transfer_remaining == 0) {
					SCSITRACEOUT(("scsitrace warning M75c2 Transfer Info "
							"with TC=0 hardware-pending"));
					scsiio.auxstatus &= (REG8)~(SCSI_AUX_BSY |
							SCSI_AUX_CIP | SCSI_AUX_DBR);
					break;
				}
				/* M75c2 accumulates CDB through the fixed DATA window. */
				SCSITRACEOUT(("scsitrace M75c2 accumulates CDB through DATA "
						"window count=%u", scsi_transfer_remaining));
				scsiio.auxstatus &= (REG8)~SCSI_AUX_CIP;
				break;
			}
			scsi_transfer_remaining = scsiio_transfer_count();
			/*
			 * Keep the target DATA cursor across repeated one-byte
			 * TRANSFER INFO requests.  PCPLUS may issue TC=1 for each
			 * PIO byte; the cursor is reset when the command enters
			 * DATA IN, not at every request boundary.
			 */
			if (scsi_transfer_phase_pending && !scsi_target_phase_ready) {
				/*
				 * The target has selected the next phase but has not yet
				 * presented its REQ.  Keep DBR low while the host waits for
				 * that service request; do not enter the legacy bulk pump.
				 */
				SCSITRACEOUT(("scsitrace target-phase-wait phase=%02x tc=%06x",
						scsiio.phase, scsi_transfer_remaining));
				scsiio.auxstatus &= (REG8)~SCSI_AUX_DBR;
				if (!scsi_target_phase_delay_pending) {
					scsi_target_phase_delay_pending = TRUE;
					scsi_target_phase_delay_clock = scsi_trace_clock();
					scsi_trace_target_delay_watchdog_reported = FALSE;
					nevent_set(NEVENT_SCSIIO, scsi_target_processing_clocks(),
						scsiio_target_phase_ready_event, NEVENT_ABSOLUTE);
				}
				return;
			}
			if (scsiio.phase == SCSIPH_STATUS) {
				/* SCSI status is GOOD for the supported commands. */
				scsiio.data[0] = scsiio.reg[SCSICTR_STATUS];
			}
			else if (scsiio.phase == SCSIPH_MSGIN) {
				/* COMMAND COMPLETE message. */
				scsiio.data[0] = 0x00;
			}
			if (scsi_transfer_remaining == 0) {
				SCSITRACEOUT(("scsitrace warning Transfer Info with TC=0 "
						"phase=%02x hardware-pending", scsiio.phase));
				scsiio.auxstatus &= (REG8)~(SCSI_AUX_BSY |
						SCSI_AUX_CIP | SCSI_AUX_DBR);
				break;
			}
			/* The target owns the phase; the PIO pump only consumes TC. */
			scsiio.auxstatus &= (REG8)~SCSI_AUX_CIP;
			break;

	}
}




// ----

static void IOOUTCALL scsiio_occ0(UINT port, REG8 dat) {

	scsi_trace_watchdog();
	scsiio.port = dat;
	SCSITRACEOUT(("scsitrace out port=0cc0 ar=%02x cs=%04x ip=%04x",
			dat, CPU_CS, CPU_IP));
	(void)port;
}

static void IOOUTCALL scsiio_occ2(UINT port, REG8 dat) {

	scsi_trace_watchdog();
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
	if (scsiio.port <= 0x19) {
		scsiio.reg[scsiio.port] = dat;
		if (scsiio.port == SCSICTR_CMD) {
			scsiio.auxstatus |= SCSI_AUX_CIP;
			scsicmd(dat);
		}
		/* COMMAND and DATA are fixed windows; all other registers advance. */
		if (scsiio.port != SCSICTR_CMD && scsiio.port != SCSICTR_DATA) {
			scsiio.port++;
		}
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
		scsiintr("legacy-data-complete", 0x8b);
	}
	(void)port;
}

static REG8 IOINPCALL scsiio_icc0(UINT port) {

	scsi_trace_watchdog();
	REG8	ret;

	ret = scsiio_auxstatus();
	SCSITRACEOUT(("scsitrace in port=0cc0 aux=%02x ar=%02x cs=%04x ip=%04x",
			ret, scsiio.port, CPU_CS, CPU_IP));
	(void)port;
	return(ret);
}

static REG8 IOINPCALL scsiio_icc2(UINT port) {

	scsi_trace_watchdog();
	REG8	ret;

	switch(scsiio.port) {
		case SCSICTR_STATUS:
			if (scsi_csr_latched) {
				scsi_trace_csr_record("hostread", scsi_csr_latched_sequence,
						scsiio.scsistatus, scsi_csr_latched_origin);
				scsi_csr_latched = FALSE;
				scsi_trace_latched_watchdog_reported = FALSE;
				scsiio.auxstatus &= (REG8)~SCSI_AUX_INT;
				scsi_target_schedule_after_consume();
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
		scsiintr("legacy-data-complete", 0x8b);
	}
	(void)port;
	return(ret);
}


// ----

void scsiio_reset(void) {

	nevent_reset(NEVENT_SCSIWATCHDOG);
	scsi_trace_watchdog_scheduled = FALSE;
	ZeroMemory(&scsiio, sizeof(scsiio));
	scsi_csr_latched = FALSE;
	scsi_csr_event_active = FALSE;
	scsi_csr_event_status = 0;
	scsi_csr_event_clock = 0;
	scsi_csr_latched_clock = 0;
	scsi_trace_data_phase_decision_clock = 0;
	scsi_trace_data_phase_pending = FALSE;
	scsi_trace_event_watchdog_reported = FALSE;
	scsi_trace_latched_watchdog_reported = FALSE;
	scsi_trace_data_phase_watchdog_reported = FALSE;
	scsi_trace_target_delay_watchdog_reported = FALSE;
	scsi_target_phase_delay_clock = 0;
	scsi_trace_data_phase_request_missing_count = 0;
	scsi_trace_csr_sequence = 0;
	scsi_csr_event_sequence = 0;
	scsi_csr_event_origin = NULL;
	scsi_csr_latched_sequence = 0;
	scsi_csr_latched_origin = NULL;
	scsi_target_selection_pending = FALSE;
	scsi_target_selection_status = 0;
	scsi_target_selection_origin = NULL;
	scsi_target_phase_delay_pending = FALSE;
	scsi_command_phase_pending = FALSE;
	scsi_transfer_phase_pending = FALSE;
	scsi_transfer_phase_status = 0;
	scsi_target_phase_ready = FALSE;
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
