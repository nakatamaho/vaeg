/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "compiler.h"
#include "cpucore.h"
#include "upd9002_trace.h"
#include <errno.h>
#include <stdlib.h>

typedef struct {
	FILE *stream;
	uint32_t remaining;
	uint32_t step;
	uint32_t event;
	int32_t before_clock;
	uint16_t before_cs;
	uint16_t before_ip;
} UPD9002_TRACE_STATE;

static UPD9002_TRACE_STATE trace_state;
typedef struct {
	FILE *stream;
	BOOL active;
	BOOL cmdreq_windows_only;
	UINT16 start_ip;
	UINT8 before_047e;
	uint32_t instruction;
	uint32_t window_count;
	uint32_t current_window;
	uint32_t presentation_instruction;
	uint32_t presentation_clock;
	uint32_t ba_instruction;
	uint32_t ba_clock;
	uint32_t presentation_wait_instruction;
	uint32_t presentation_wait_clock;
	BOOL presentation_wait_seen;
	uint32_t last_wait_instruction;
	uint32_t last_wait_clock;
	BOOL wait_seen;
	BOOL wait_logged;
	BOOL window_active;
	BOOL ba_seen;
	BOOL consumer_seen;
	UINT16 consumer_ip;
	BOOL capture_done;
} UPD9002_GUEST_TRACE_STATE;

static UPD9002_GUEST_TRACE_STATE guest_trace_state;

static BOOL guest_trace_watch_ip(UINT16 ip) {

	switch (ip) {
		case 0x1742:
		case 0x1747:
		case 0x1791:
		case 0x1972:
		case 0x1975:
		case 0x19a7:
		case 0x19bb:
		case 0x19c6:
		case 0x19c8:
		case 0x1b60:
		case 0x1ba1:
		case 0x1bfc:
		case 0x1c05:
		case 0x1c0e:
		case 0x1c14:
		case 0x1c32:
		case 0x1c34:
		case 0x1c37:
		case 0x1cbd:
		case 0x1ccd:
		case 0x1d67:
			return(TRUE);
		default:
			return(FALSE);
	}
}

static const char *guest_trace_label(UINT16 ip) {

	switch (ip) {
		case 0x19bb:
		case 0x19c6:
			return("phase-compare");
		case 0x1b60:
			return("transfer-path-1");
		case 0x1ba1:
			return("transfer-path-2");
		case 0x1c14:
		case 0x1c37:
			return("transfer-setup");
		case 0x1c32:
		case 0x1c34:
			return("transfer-info-command");
		case 0x1742:
		case 0x1747:
		case 0x1791:
		case 0x19a7:
		case 0x1bfc:
		case 0x1c05:
		case 0x1c0e:
		case 0x1cbd:
		case 0x1ccd:
		case 0x1d67:
			return("status-047e");
		default:
			return("handoff");
	}
}

static BOOL guest_trace_047e_writer(UINT16 ip) {

	return (ip == 0x1747) || (ip == 0x1bfc) ||
		(ip == 0x1ccd) || (ip == 0x1d67);
}

static void guest_trace_log(const char *event, UINT16 ip,
		UINT16 next_ip, UINT8 before_047e, UINT8 after_047e) {

	if (guest_trace_state.stream == NULL) {
		return;
	}
	fprintf(guest_trace_state.stream,
		"scsi-guest-trace event=%s label=%s ip=%04x next_ip=%04x "
		"cs=%04x phys=%05x 047e_before=%02x 047e_after=%02x "
		"ax=%04x bx=%04x cx=%04x dx=%04x si=%04x di=%04x "
		"bp=%04x sp=%04x flags=%04x\n",

		event, guest_trace_label(ip), ip, next_ip, CPU_CS,
		(unsigned)((CS_BASE + 0x047e) & CPU_ADRSMASK),
		before_047e, after_047e,
		CPU_AX, CPU_BX, CPU_CX, CPU_DX, CPU_SI, CPU_DI,
		CPU_BP, CPU_SP, CPU_FLAG);
}

static uint32_t guest_trace_clock(void) {

	return (uint32_t)(CPU_CLOCK + CPU_BASECLOCK - CPU_REMCLOCK);
}

static void guest_trace_cmdreq_log(const char *event, UINT16 ip,
		UINT16 next_ip, UINT8 before_047e, UINT8 after_047e) {

	if (guest_trace_state.stream == NULL) {
		return;
	}
	fprintf(guest_trace_state.stream,
		"scsi-cmdreq-window event=%s window=%u step=%08x clock=%08x "
		"label=%s ip=%04x next_ip=%04x cs=%04x phys=%05x "
		"047e_before=%02x 047e_after=%02x "
		"ax=%04x bx=%04x cx=%04x dx=%04x si=%04x di=%04x "
		"bp=%04x sp=%04x flags=%04x\n",
		event, guest_trace_state.current_window,
		guest_trace_state.instruction, guest_trace_clock(),
		guest_trace_label(ip), ip, next_ip, CPU_CS,
		(unsigned)((CS_BASE + 0x047e) & CPU_ADRSMASK),
		before_047e, after_047e,
		CPU_AX, CPU_BX, CPU_CX, CPU_DX, CPU_SI, CPU_DI,
		CPU_BP, CPU_SP, CPU_FLAG);
}

static void guest_trace_cmdreq_summary(const char *consumer,
		UINT16 consumer_ip, UINT16 next_ip) {

	if (guest_trace_state.stream == NULL || !guest_trace_state.ba_seen) {
		return;
	}
	fprintf(guest_trace_state.stream,
		"scsi-cmdreq-window-summary window=%u consumer=%s consumer_ip=%04x "
		"next_ip=%04x presentation_step=%08x presentation_clock=%08x "
		"presentation_wait_seen=%u presentation_wait_step=%08x "
		"presentation_wait_clock=%08x write_step=%08x write_clock=%08x "
		"wait_after_presentation=%u wait_step=%08x wait_clock=%08x "
		"write_to_consume_steps=%08x write_to_consume_clocks=%08x\n",
		guest_trace_state.current_window, consumer, consumer_ip, next_ip,
		guest_trace_state.presentation_instruction,
		guest_trace_state.presentation_clock,
		guest_trace_state.presentation_wait_seen ? 1 : 0,
		guest_trace_state.presentation_wait_instruction,
		guest_trace_state.presentation_wait_clock,
		guest_trace_state.ba_instruction, guest_trace_state.ba_clock,
		guest_trace_state.wait_seen ? 1 : 0,
		guest_trace_state.last_wait_instruction,
		guest_trace_state.last_wait_clock,
		guest_trace_state.instruction - guest_trace_state.ba_instruction,
		guest_trace_clock() - guest_trace_state.ba_clock);
	guest_trace_state.window_active = FALSE;
	if (guest_trace_state.current_window >= 2) {
		guest_trace_state.capture_done = TRUE;
		fprintf(guest_trace_state.stream,
			"scsi-cmdreq-windows-complete windows=%u\n",
			guest_trace_state.current_window);
	}
}

static const char *origin_name(uint32_t origin) {
	switch (origin) {
	case UPD9002_TRACE_ORIGIN_CPU:
		return "cpu";
	case UPD9002_TRACE_ORIGIN_DMA:
		return "dma";
	case UPD9002_TRACE_ORIGIN_DEVICE:
		return "device";
	default:
		return "invalid";
	}
}

void upd9002_trace_start(FILE *stream, uint32_t steps) {

	ZeroMemory(&trace_state, sizeof(trace_state));
	if ((stream != NULL) && (steps != 0)) {
		trace_state.stream = stream;
		trace_state.remaining = steps;
		fprintf(stream, "upd9002-trace-v1\n");
	}
}

void upd9002_trace_stop(void) {

	if (trace_state.stream != NULL) {
		fflush(trace_state.stream);
	}
	ZeroMemory(&trace_state, sizeof(trace_state));
}

void upd9002_guest_trace_start(FILE *stream) {

	ZeroMemory(&guest_trace_state, sizeof(guest_trace_state));
	if (stream != NULL) {
		guest_trace_state.stream = stream;
		fprintf(stream, "scsi-guest-trace-v1\n");
	}
}

void upd9002_guest_trace_start_cmdreq_windows(FILE *stream) {

	ZeroMemory(&guest_trace_state, sizeof(guest_trace_state));
	if (stream != NULL) {
		guest_trace_state.stream = stream;
		guest_trace_state.cmdreq_windows_only = TRUE;
		fprintf(stream, "scsi-cmdreq-windows-v1\n");
	}
}

void upd9002_guest_trace_stop(void) {

	if (guest_trace_state.stream != NULL) {
		fflush(guest_trace_state.stream);
	}
	ZeroMemory(&guest_trace_state, sizeof(guest_trace_state));
}

void upd9002_guest_trace_step_begin(void) {
	UINT8 value;

	guest_trace_state.active = FALSE;
	if (guest_trace_state.cmdreq_windows_only) {
		if ((guest_trace_state.stream == NULL) ||
			guest_trace_state.capture_done) {
			return;
		}
		if (CPU_IP == 0x1cbd) {
			guest_trace_state.last_wait_instruction =
				guest_trace_state.instruction;
			guest_trace_state.last_wait_clock = guest_trace_clock();
			guest_trace_state.wait_seen = TRUE;
			if (guest_trace_state.window_active &&
				!guest_trace_state.wait_logged) {
				guest_trace_cmdreq_log("wait-enter", CPU_IP, CPU_IP,
					mem[(CS_BASE + 0x047e) & CPU_ADRSMASK],
					mem[(CS_BASE + 0x047e) & CPU_ADRSMASK]);
				guest_trace_state.wait_logged = TRUE;
			}
			return;
		}
		if ((CPU_IP != 0x1d67) &&
			(!guest_trace_state.window_active ||
			 !guest_trace_watch_ip(CPU_IP))) {
			return;
		}
		value = mem[(CS_BASE + 0x047e) & CPU_ADRSMASK];
		guest_trace_state.active = TRUE;
		guest_trace_state.start_ip = CPU_IP;
		guest_trace_state.before_047e = value;
		guest_trace_cmdreq_log("entry", CPU_IP, CPU_IP, value, value);
		return;
	}
	if ((guest_trace_state.stream == NULL) ||
		!guest_trace_watch_ip(CPU_IP)) {
		return;
	}
	value = mem[(CS_BASE + 0x047e) & CPU_ADRSMASK];
	guest_trace_state.active = TRUE;
	guest_trace_state.start_ip = CPU_IP;
	guest_trace_state.before_047e = value;
	guest_trace_log("entry", CPU_IP, CPU_IP, value, value);
}

void upd9002_guest_trace_step_end(void) {
	UINT8 after_047e;

	if (guest_trace_state.cmdreq_windows_only) {
		if ((guest_trace_state.stream == NULL) ||
			guest_trace_state.capture_done) {
			return;
		}
		guest_trace_state.instruction++;
		if (!guest_trace_state.active) {
			return;
		}
		after_047e = mem[(CS_BASE + 0x047e) & CPU_ADRSMASK];
		if ((guest_trace_state.start_ip == 0x1d67) &&
			(after_047e == 0xba) && guest_trace_state.window_active) {
			guest_trace_state.ba_seen = TRUE;
			guest_trace_state.ba_instruction = guest_trace_state.instruction;
			guest_trace_state.ba_clock = guest_trace_clock();
			guest_trace_cmdreq_log("047e-write-bah",
				guest_trace_state.start_ip, CPU_IP,
				guest_trace_state.before_047e, after_047e);
			fprintf(guest_trace_state.stream,
				"scsi-cmdreq-window-context window=%u "
				"presentation_to_wait=%u wait_step=%08x wait_clock=%08x\n",
				guest_trace_state.current_window,
				guest_trace_state.wait_seen ? 1 : 0,
				guest_trace_state.last_wait_instruction,
				guest_trace_state.last_wait_clock);
		}
		if ((guest_trace_state.start_ip == 0x1ccd) &&
			guest_trace_state.ba_seen) {
			guest_trace_cmdreq_log("wait-consume",
				guest_trace_state.start_ip, CPU_IP,
				guest_trace_state.before_047e, after_047e);
			guest_trace_cmdreq_summary("1ccd", guest_trace_state.start_ip,
				CPU_IP);
		}
		if ((guest_trace_state.start_ip == 0x1747) &&
			guest_trace_state.ba_seen) {
			guest_trace_state.consumer_seen = TRUE;
			guest_trace_state.consumer_ip = guest_trace_state.start_ip;
			guest_trace_cmdreq_log("main-pump-consume",
				guest_trace_state.start_ip, CPU_IP,
				guest_trace_state.before_047e, after_047e);
		}
		if ((guest_trace_state.start_ip == 0x1791) &&
			guest_trace_state.consumer_seen) {
			guest_trace_cmdreq_log("main-pump-exit",
				guest_trace_state.start_ip, CPU_IP,
				guest_trace_state.before_047e, after_047e);
			guest_trace_cmdreq_summary("1747", guest_trace_state.consumer_ip,
				CPU_IP);
		}
		if (guest_trace_state.window_active) {
			guest_trace_cmdreq_log("exit", guest_trace_state.start_ip, CPU_IP,
				guest_trace_state.before_047e, after_047e);
		}
		guest_trace_state.active = FALSE;
		return;
	}
	if (!guest_trace_state.active) {
		return;
	}
	after_047e = mem[(CS_BASE + 0x047e) & CPU_ADRSMASK];
	guest_trace_log(guest_trace_047e_writer(guest_trace_state.start_ip) ?
		"047e-write" : "exit", guest_trace_state.start_ip, CPU_IP,
		guest_trace_state.before_047e, after_047e);
	guest_trace_state.active = FALSE;
}

void upd9002_guest_trace_scsi_status(uint8_t status) {

	if (!guest_trace_state.cmdreq_windows_only ||
		guest_trace_state.stream == NULL || guest_trace_state.capture_done) {
		return;
	}
	if (status == 0x8a) {
		if (guest_trace_state.window_active) {
			fprintf(guest_trace_state.stream,
				"scsi-cmdreq-window event=window-overlap window=%u "
				"ba_seen=%u step=%08x clock=%08x\n",
				guest_trace_state.current_window,
				guest_trace_state.ba_seen ? 1 : 0,
				guest_trace_state.instruction, guest_trace_clock());
		}
		guest_trace_state.window_count++;
		guest_trace_state.current_window = guest_trace_state.window_count;
		guest_trace_state.window_active = TRUE;
		guest_trace_state.presentation_instruction =
			guest_trace_state.instruction;
		guest_trace_state.presentation_clock = guest_trace_clock();
		guest_trace_state.presentation_wait_instruction =
			guest_trace_state.last_wait_instruction;
		guest_trace_state.presentation_wait_clock =
			guest_trace_state.last_wait_clock;
		guest_trace_state.presentation_wait_seen = guest_trace_state.wait_seen;
		guest_trace_state.wait_seen = FALSE;
		guest_trace_state.wait_logged = FALSE;
		guest_trace_state.ba_seen = FALSE;
		guest_trace_state.consumer_seen = FALSE;
		fprintf(guest_trace_state.stream,
			"scsi-cmdreq-window event=csr-present raw=8a window=%u "
			"step=%08x clock=%08x cs=%04x ip=%04x\n",
			guest_trace_state.current_window,
			guest_trace_state.presentation_instruction,
			guest_trace_state.presentation_clock, CPU_CS, CPU_IP);
	}
	else {
		fprintf(guest_trace_state.stream,
			"scsi-cmdreq-window event=csr-present raw=%02x window=%u "
			"step=%08x clock=%08x cs=%04x ip=%04x\n",
			status, guest_trace_state.current_window,
			guest_trace_state.instruction, guest_trace_clock(), CPU_CS, CPU_IP);
	}
}

int upd9002_trace_active(void) {

	return (trace_state.stream != NULL) && (trace_state.remaining != 0);
}

void upd9002_trace_event(uint32_t origin, const char *kind,
						uint32_t address, uint32_t value, uint32_t width) {

	if (!upd9002_trace_active()) {
		return;
	}
	fprintf(trace_state.stream,
		"event step=%08x seq=%08x origin=%s kind=%s address=%08x value=%08x width=%02x\n",
		trace_state.step, trace_state.event++, origin_name(origin),
		(kind != NULL) ? kind : "invalid", address, value, width);
}

void upd9002_trace_step_begin(void) {

	uint32_t address;
	uint32_t index;

	if (!upd9002_trace_active()) {
		return;
	}
	trace_state.before_clock = CPU_REMCLOCK;
	trace_state.before_cs = CPU_CS;
	trace_state.before_ip = CPU_IP;
	trace_state.event = 0;
	address = (CS_BASE + CPU_IP) & CPU_ADRSMASK;
	fprintf(trace_state.stream,
		"begin step=%08x cs=%04x ip=%04x bytes=",
		trace_state.step, CPU_CS, CPU_IP);
	for (index = 0; index < 8; index++) {
		fprintf(trace_state.stream, "%02x", mem[(address + index) & CPU_ADRSMASK]);
	}
	fputc('\n', trace_state.stream);
	upd9002_trace_event(UPD9002_TRACE_ORIGIN_CPU, "fetch", address,
		mem[address], 1);
	upd9002_trace_event(UPD9002_TRACE_ORIGIN_DMA, "scheduler-checkpoint",
		0, 0, 0);
	upd9002_trace_event(UPD9002_TRACE_ORIGIN_DEVICE, "device-checkpoint",
		0, 0, 0);
}

void upd9002_trace_step_end(void) {

	int32_t consumed;

	if (!upd9002_trace_active()) {
		return;
	}
	consumed = trace_state.before_clock - CPU_REMCLOCK;
	fprintf(trace_state.stream,
		"end step=%08x ax=%04x bx=%04x cx=%04x dx=%04x si=%04x di=%04x bp=%04x sp=%04x es=%04x cs=%04x ss=%04x ds=%04x ip=%04x flags=%04x esbase=%08x csbase=%08x ssbase=%08x dsbase=%08x consumed=%08x remain=%08x\n",
		trace_state.step, CPU_AX, CPU_BX, CPU_CX, CPU_DX, CPU_SI, CPU_DI,
		CPU_BP, CPU_SP, CPU_ES, CPU_CS, CPU_SS, CPU_DS, CPU_IP,
		CPU_FLAG, ES_BASE, CS_BASE, SS_BASE, DS_BASE,
		(uint32_t)consumed, (uint32_t)CPU_REMCLOCK);
	trace_state.step++;
	trace_state.remaining--;
	if (trace_state.remaining == 0) {
		fflush(trace_state.stream);
	}
}

typedef struct {
	uint32_t sequence;
	uint16_t cs;
	uint16_t ip;
	uint16_t ss;
	uint16_t sp;
	uint32_t physical;
	uint32_t stack_physical;
	uint8_t bytes[8];
	uint8_t stack_before[8];
	uint16_t ax;
	uint16_t bx;
	uint16_t cx;
	uint16_t dx;
	uint16_t si;
	uint16_t di;
	uint16_t bp;
	uint16_t flags;
	uint16_t post_cs;
	uint16_t post_ip;
	uint16_t post_ss;
	uint16_t post_sp;
	uint16_t post_flags;
	uint32_t post_cs_base;
	uint32_t post_ss_base;
	uint32_t post_stack_physical;
	uint8_t stack_after[8];
	uint8_t opcode;
	uint8_t io_direction;
	uint16_t io_port;
	uint8_t interrupt_vector;
} UPD9002_M74_TRACE_RECORD;

typedef struct {
	uint32_t sequence;
	uint8_t vector;
	uint8_t external;
} UPD9002_M74_TRACE_INTERRUPT;

typedef struct {
	FILE *stream;
	UPD9002_M74_TRACE_RECORD *records;
	UPD9002_M74_TRACE_INTERRUPT interrupts[256];
	uint32_t limit;
	uint32_t steps;
	uint32_t record_count;
	uint32_t record_head;
	uint32_t interrupt_count;
	uint32_t interrupt_head;
	uint32_t command_number;
	uint32_t arm_command;
	uint32_t current_record;
	BOOL configured;
	BOOL active;
	BOOL dumped;
	BOOL have_previous_record;
	BOOL have_previous_previous_record;
	BOOL stable_loop_reported;
	BOOL stable_loop_candidate_used;
	uint16_t stable_loop_candidate_cs;
	uint16_t stable_loop_candidate_ip;
	UPD9002_M74_TRACE_RECORD history[256];
	uint32_t history_count;
	uint32_t history_head;
	UPD9002_M74_TRACE_RECORD stable_loop_entry_history[256];
	uint32_t stable_loop_entry_history_count;
	UPD9002_M74_TRACE_RECORD previous_record;
	UPD9002_M74_TRACE_RECORD previous_previous_record;
	UPD9002_M74_TRACE_RECORD stable_loop_first;
	UPD9002_M74_TRACE_RECORD stable_loop_predecessor;
	UPD9002_M74_TRACE_RECORD stable_loop_predecessor2;
	BOOL stable_loop_has_predecessor;
	BOOL stable_loop_has_predecessor2;
	uint32_t stable_loop_count;
} UPD9002_M74_TRACE_STATE;

static UPD9002_M74_TRACE_STATE m74_trace_state;

#define UPD9002_M74_TRACE_RECORD_CAPACITY 4096
#define UPD9002_M74_TRACE_INTERRUPT_CAPACITY 256
#define UPD9002_M74_TRACE_STABLE_COUNT 4096
#define UPD9002_M74_TRACE_HISTORY_CAPACITY 256

static const char *m74_trace_control_class(uint8_t opcode) {

	switch (opcode) {
		case 0x9a:
			return("far-call");
		case 0xea:
			return("far-jump");
		case 0xca:
		case 0xcb:
			return("far-return");
		case 0xcf:
			return("iret");
		case 0xc2:
		case 0xc3:
			return("near-return");
		case 0xe8:
			return("near-call");
		case 0xe9:
		case 0xeb:
			return("near-jump");
		case 0xcd:
		case 0xcc:
			return("software-interrupt");
		case 0xff:
			return("indirect-control");
		case 0x64:
			return("repnc-prefix");
		case 0x65:
			return("repc-prefix");
		case 0xf4:
			return("halt");
		default:
			return("sequential-or-conditional");
	}
}

static void m74_trace_io(UPD9002_M74_TRACE_RECORD *record) {

	switch (record->opcode) {
		case 0xe4:
		case 0xe5:
		case 0xe6:
		case 0xe7:
			record->io_port = record->bytes[1];
			record->io_direction = (record->opcode <= 0xe5) ? 'I' : 'O';
			break;
		case 0xec:
		case 0xed:
		case 0xee:
		case 0xef:
			record->io_port = record->dx;
			record->io_direction = (record->opcode <= 0xed) ? 'I' : 'O';
			break;
		default:
			record->io_port = 0;
			record->io_direction = 0;
			break;
	}
	if ((record->opcode == 0xcd) || (record->opcode == 0xcc)) {
		record->interrupt_vector =
			(record->opcode == 0xcc) ? 3 : record->bytes[1];
	}
	else {
		record->interrupt_vector = 0xff;
	}
}

static void m74_trace_stack(uint8_t *destination, uint32_t base) {

	uint32_t index;

	for (index = 0; index < 8; index++) {
		destination[index] = mem[(base + index) & CPU_ADRSMASK];
	}
}

static void m74_trace_print_marker_record(const char *label,
		const UPD9002_M74_TRACE_RECORD *record) {

	fprintf(m74_trace_state.stream,
		"m74-stable-loop-%s seq=%u cs=%04x ip=%04x phys=%05x "
		"opcode=%02x class=%s sp=%04x post_cs=%04x post_ip=%04x "
		"post_sp=%04x flags=%04x\n",
		label, record->sequence, record->cs, record->ip, record->physical,
		record->opcode, m74_trace_control_class(record->opcode),
		record->sp, record->post_cs, record->post_ip, record->post_sp,
		record->flags);
}

static void m74_trace_dump_stable_history(void) {
	uint32_t index;
	uint32_t position;
	UPD9002_M74_TRACE_RECORD *record;

	position = 0;
	for (index = 0; index < m74_trace_state.stable_loop_entry_history_count;
		index++) {
		record = &m74_trace_state.stable_loop_entry_history[position];
		fprintf(m74_trace_state.stream,
			"m74-stable-loop-entry-history index=%u seq=%u cs=%04x "
			"ip=%04x phys=%05x opcode=%02x class=%s sp=%04x "
			"post_cs=%04x post_ip=%04x post_sp=%04x flags=%04x\n",
			index, record->sequence, record->cs, record->ip,
			record->physical, record->opcode,
			m74_trace_control_class(record->opcode), record->sp,
			record->post_cs, record->post_ip, record->post_sp,
			record->flags);
		position = (position + 1) % UPD9002_M74_TRACE_HISTORY_CAPACITY;
	}
}

static void m74_trace_dump_stable_window(uint32_t physical) {
	uint32_t index;
	uint32_t address;
	int32_t before_clock;

	address = (physical + CPU_ADRSMASK - 16) & CPU_ADRSMASK;
	before_clock = CPU_REMCLOCK;
	fprintf(m74_trace_state.stream,
		"m74-stable-loop-window base=%05x bytes=", address);
	for (index = 0; index < 64; index++) {
		fprintf(m74_trace_state.stream, "%02x",
			upd9002_memoryread((address + index) & CPU_ADRSMASK));
	}
	CPU_REMCLOCK = before_clock;
	fprintf(m74_trace_state.stream, "\n");
}

static void m74_trace_dump(const char *reason) {

	uint32_t index;
	uint32_t position;
	uint32_t interrupt_position;
	UPD9002_M74_TRACE_RECORD *record;

	if (!m74_trace_state.configured || m74_trace_state.dumped) {
		return;
	}
	fprintf(m74_trace_state.stream,
		"m74-cpu-trace-v1 reason=%s command=%u steps=%u records=%u "
		"interrupts=%u\n",
		reason, m74_trace_state.command_number, m74_trace_state.steps,
		m74_trace_state.record_count, m74_trace_state.interrupt_count);
	if (m74_trace_state.stable_loop_reported) {
		fprintf(m74_trace_state.stream,
			"m74-stable-loop cs=%04x ip=%04x count=%u\n",
			m74_trace_state.stable_loop_first.cs,
			m74_trace_state.stable_loop_first.ip,
			m74_trace_state.stable_loop_count);
		m74_trace_print_marker_record("first",
			&m74_trace_state.stable_loop_first);
		if (m74_trace_state.stable_loop_has_predecessor) {
			m74_trace_print_marker_record("predecessor",
				&m74_trace_state.stable_loop_predecessor);
		}
		if (m74_trace_state.stable_loop_has_predecessor2) {
			m74_trace_print_marker_record("predecessor2",
				&m74_trace_state.stable_loop_predecessor2);
		}
		m74_trace_dump_stable_history();
		m74_trace_dump_stable_window(
			m74_trace_state.stable_loop_first.physical);
	}
	position = (m74_trace_state.record_head +
		UPD9002_M74_TRACE_RECORD_CAPACITY - m74_trace_state.record_count) %
		UPD9002_M74_TRACE_RECORD_CAPACITY;
	for (index = 0; index < m74_trace_state.record_count; index++) {
		record = &m74_trace_state.records[position];
		fprintf(m74_trace_state.stream,
			"m74-cpu-record seq=%u cs=%04x ip=%04x phys=%05x "
			"bytes=",
			record->sequence, record->cs, record->ip, record->physical);
		for (uint32_t byte = 0; byte < 8; byte++) {
			fprintf(m74_trace_state.stream, "%02x", record->bytes[byte]);
		}
		fprintf(m74_trace_state.stream,
			" opcode=%02x class=%s ax=%04x bx=%04x cx=%04x dx=%04x "
			"si=%04x di=%04x bp=%04x ss=%04x sp=%04x flags=%04x "
			"stack_phys=%05x stack_before=",
			record->opcode, m74_trace_control_class(record->opcode),
			record->ax, record->bx, record->cx, record->dx, record->si,
			record->di, record->bp, record->ss, record->sp, record->flags,
			record->stack_physical);
		for (uint32_t byte = 0; byte < 8; byte++) {
			fprintf(m74_trace_state.stream, "%02x", record->stack_before[byte]);
		}
		fprintf(m74_trace_state.stream,
			" post_cs=%04x post_ip=%04x post_ss=%04x post_sp=%04x "
			"post_cs_base=%05x post_ss_base=%05x post_stack_phys=%05x "
			"post_flags=%04x stack_after=",
			record->post_cs, record->post_ip, record->post_ss,
			record->post_sp, record->post_cs_base, record->post_ss_base,
			record->post_stack_physical, record->post_flags);
		for (uint32_t byte = 0; byte < 8; byte++) {
			fprintf(m74_trace_state.stream, "%02x", record->stack_after[byte]);
		}
		if (record->io_direction != 0) {
			fprintf(m74_trace_state.stream, " io=%c port=%04x",
				record->io_direction, record->io_port);
		}
		if (record->interrupt_vector != 0xff) {
			fprintf(m74_trace_state.stream, " int_vector=%02x",
				record->interrupt_vector);
		}
		fprintf(m74_trace_state.stream, "\n");
		position = (position + 1) % UPD9002_M74_TRACE_RECORD_CAPACITY;
	}
	interrupt_position = (m74_trace_state.interrupt_head +
		UPD9002_M74_TRACE_INTERRUPT_CAPACITY -
		m74_trace_state.interrupt_count) %
		UPD9002_M74_TRACE_INTERRUPT_CAPACITY;
	for (index = 0; index < m74_trace_state.interrupt_count; index++) {
		fprintf(m74_trace_state.stream,
			"m74-interrupt seq=%u vector=%02x kind=%s\n",
			m74_trace_state.interrupts[interrupt_position].sequence,
			m74_trace_state.interrupts[interrupt_position].vector,
			m74_trace_state.interrupts[interrupt_position].external ?
				"external" : "software");
		interrupt_position = (interrupt_position + 1) %
			UPD9002_M74_TRACE_INTERRUPT_CAPACITY;
	}
	fflush(m74_trace_state.stream);
	m74_trace_state.dumped = TRUE;
}

void upd9002_m74_trace_configure(FILE *stream) {

	const char *value;
	const char *arm_value;
	char *end;
	unsigned long limit;
	unsigned long arm_command;

	ZeroMemory(&m74_trace_state, sizeof(m74_trace_state));
	value = getenv("VAEG_M74_CPU_TRACE_LIMIT");
	arm_value = getenv("VAEG_M74_CPU_TRACE_COMMAND");
	if ((stream == NULL) || (value == NULL) || (value[0] == '\0')) {
		return;
	}
	errno = 0;
	end = NULL;
	limit = strtoul(value, &end, 10);
	if ((errno != 0) || (end == value) || (*end != '\0') ||
		(limit == 0) || (limit > 10000000)) {
		fprintf(stream,
			"m74-cpu-trace disabled: VAEG_M74_CPU_TRACE_LIMIT must be "
			"1..10000000\n");
		return;
	}
	m74_trace_state.records = (UPD9002_M74_TRACE_RECORD *)calloc(
		UPD9002_M74_TRACE_RECORD_CAPACITY,
		sizeof(*m74_trace_state.records));
	if (m74_trace_state.records == NULL) {
		fprintf(stream, "m74-cpu-trace disabled: ring allocation failed\n");
		return;
	}
	m74_trace_state.stream = stream;
	m74_trace_state.limit = (uint32_t)limit;
	m74_trace_state.arm_command = 0;
	if ((arm_value != NULL) && (arm_value[0] != '\0')) {
		errno = 0;
		end = NULL;
		arm_command = strtoul(arm_value, &end, 10);
		if ((errno != 0) || (end == arm_value) || (*end != '\0') ||
			(arm_command == 0) || (arm_command > 256)) {
			fprintf(stream,
				"m74-cpu-trace disabled: VAEG_M74_CPU_TRACE_COMMAND "
				"must be 1..256\n");
			free(m74_trace_state.records);
			ZeroMemory(&m74_trace_state, sizeof(m74_trace_state));
			return;
		}
		m74_trace_state.arm_command = (uint32_t)arm_command;
	}
	m74_trace_state.configured = TRUE;
	fprintf(stream, "m74-cpu-trace configured limit=%u ring=%u "
		"arm_command=%u\n", m74_trace_state.limit,
		UPD9002_M74_TRACE_RECORD_CAPACITY, m74_trace_state.arm_command);
}

void upd9002_m74_trace_stop(void) {

	if (m74_trace_state.configured) {
		if (m74_trace_state.active) {
			m74_trace_dump("shutdown");
		}
		free(m74_trace_state.records);
	}
	ZeroMemory(&m74_trace_state, sizeof(m74_trace_state));
}

void upd9002_m74_trace_arm(uint32_t command_number) {

	if (!m74_trace_state.configured ||
		((m74_trace_state.arm_command != 0) &&
		 (m74_trace_state.arm_command != command_number))) {
		return;
	}
	ZeroMemory(m74_trace_state.records,
		UPD9002_M74_TRACE_RECORD_CAPACITY *
		sizeof(*m74_trace_state.records));
	m74_trace_state.record_count = 0;
	m74_trace_state.record_head = 0;
	m74_trace_state.interrupt_count = 0;
	m74_trace_state.interrupt_head = 0;
	m74_trace_state.steps = 0;
	m74_trace_state.command_number = command_number;
	m74_trace_state.current_record = 0;
	m74_trace_state.active = TRUE;
	m74_trace_state.dumped = FALSE;
	m74_trace_state.have_previous_record = FALSE;
	m74_trace_state.have_previous_previous_record = FALSE;
	m74_trace_state.stable_loop_reported = FALSE;
	m74_trace_state.stable_loop_candidate_used = FALSE;
	m74_trace_state.stable_loop_candidate_cs = 0;
	m74_trace_state.stable_loop_candidate_ip = 0;
	m74_trace_state.history_count = 0;
	m74_trace_state.history_head = 0;
	m74_trace_state.stable_loop_entry_history_count = 0;
	m74_trace_state.stable_loop_has_predecessor = FALSE;
	m74_trace_state.stable_loop_has_predecessor2 = FALSE;
	m74_trace_state.stable_loop_count = 0;
	fprintf(m74_trace_state.stream,
		"m74-cpu-trace armed command=%u cs=%04x ip=%04x ss=%04x sp=%04x "
		"stack_phys=%05x stack=", command_number, CPU_CS, CPU_IP,
		CPU_SS, CPU_SP, (SS_BASE + CPU_SP) & CPU_ADRSMASK);
	for (uint32_t byte = 0; byte < 8; byte++) {
		fprintf(m74_trace_state.stream, "%02x",
			upd9002_memoryread(((SS_BASE + CPU_SP) + byte) & CPU_ADRSMASK));
	}
	fprintf(m74_trace_state.stream, " source=");
	for (uint32_t byte = 0; byte < 8; byte++) {
		fprintf(m74_trace_state.stream, "%02x",
			upd9002_memoryread(0x3996d + byte));
	}
	fprintf(m74_trace_state.stream, "\n");
}

void upd9002_m74_trace_step_begin(void) {

	UPD9002_M74_TRACE_RECORD *record;
	uint32_t index;
	uint32_t address;
	int32_t before_clock;

	if (!m74_trace_state.active) {
		return;
	}
	m74_trace_state.current_record = m74_trace_state.record_head;
	record = &m74_trace_state.records[m74_trace_state.current_record];
	ZeroMemory(record, sizeof(*record));
	record->sequence = m74_trace_state.steps;
	record->cs = CPU_CS;
	record->ip = CPU_IP;
	record->ss = CPU_SS;
	record->sp = CPU_SP;
	record->physical = (CS_BASE + CPU_IP) & CPU_ADRSMASK;
	record->stack_physical = (SS_BASE + CPU_SP) & CPU_ADRSMASK;
	record->ax = CPU_AX;
	record->bx = CPU_BX;
	record->cx = CPU_CX;
	record->dx = CPU_DX;
	record->si = CPU_SI;
	record->di = CPU_DI;
	record->bp = CPU_BP;
	record->flags = CPU_FLAG;
	address = record->physical;
	before_clock = CPU_REMCLOCK;
	for (index = 0; index < 8; index++) {
		record->bytes[index] =
			upd9002_memoryread(address + index);
	}
	CPU_REMCLOCK = before_clock;
	record->opcode = record->bytes[0];
	m74_trace_io(record);
	m74_trace_stack(record->stack_before, record->stack_physical);
	m74_trace_state.record_head =
		(m74_trace_state.record_head + 1) %
		UPD9002_M74_TRACE_RECORD_CAPACITY;
	if (m74_trace_state.record_count < UPD9002_M74_TRACE_RECORD_CAPACITY) {
		m74_trace_state.record_count++;
	}
}

static void m74_trace_address_update(
		UPD9002_M74_TRACE_RECORD *record) {
	BOOL self_loop;

	self_loop = (record->post_cs == record->cs) &&
		(record->post_ip == record->ip);
	if (self_loop) {
		if (!m74_trace_state.stable_loop_candidate_used ||
			(m74_trace_state.stable_loop_candidate_cs != record->cs) ||
			(m74_trace_state.stable_loop_candidate_ip != record->ip)) {
			m74_trace_state.stable_loop_candidate_used = TRUE;
			m74_trace_state.stable_loop_candidate_cs = record->cs;
			m74_trace_state.stable_loop_candidate_ip = record->ip;
			m74_trace_state.stable_loop_count = 0;
			m74_trace_state.stable_loop_first = *record;
			m74_trace_state.stable_loop_entry_history_count =
				m74_trace_state.history_count;
			for (uint32_t history_index = 0;
				history_index < m74_trace_state.history_count;
				history_index++) {
				uint32_t history_position =
					(m74_trace_state.history_head +
						UPD9002_M74_TRACE_HISTORY_CAPACITY -
						m74_trace_state.history_count + history_index) %
						UPD9002_M74_TRACE_HISTORY_CAPACITY;
				m74_trace_state.stable_loop_entry_history[history_index] =
					m74_trace_state.history[history_position];
			}
			m74_trace_state.stable_loop_has_predecessor =
				m74_trace_state.have_previous_record;
			m74_trace_state.stable_loop_has_predecessor2 =
				m74_trace_state.have_previous_previous_record;
			if (m74_trace_state.stable_loop_has_predecessor) {
				m74_trace_state.stable_loop_predecessor =
					m74_trace_state.previous_record;
			}
			if (m74_trace_state.stable_loop_has_predecessor2) {
				m74_trace_state.stable_loop_predecessor2 =
					m74_trace_state.previous_previous_record;
			}
		}
		m74_trace_state.stable_loop_count++;
		if ((m74_trace_state.stable_loop_count ==
			UPD9002_M74_TRACE_STABLE_COUNT) &&
			!m74_trace_state.stable_loop_reported) {
			m74_trace_state.stable_loop_reported = TRUE;
			fprintf(m74_trace_state.stream,
				"m74-stable-loop-candidate cs=%04x ip=%04x count=%u\n",
				record->cs, record->ip,
				m74_trace_state.stable_loop_count);
			m74_trace_print_marker_record("candidate-first",
				&m74_trace_state.stable_loop_first);
			if (m74_trace_state.stable_loop_has_predecessor) {
				m74_trace_print_marker_record("candidate-predecessor",
					&m74_trace_state.stable_loop_predecessor);
			}
			m74_trace_dump_stable_window(
				m74_trace_state.stable_loop_first.physical);
			m74_trace_state.active = FALSE;
			m74_trace_dump("stable-loop");
		}
	}
	m74_trace_state.previous_previous_record =
		m74_trace_state.previous_record;
	m74_trace_state.have_previous_previous_record =
		m74_trace_state.have_previous_record;
	m74_trace_state.previous_record = *record;
	m74_trace_state.have_previous_record = TRUE;
	m74_trace_state.history[m74_trace_state.history_head] = *record;
	m74_trace_state.history_head = (m74_trace_state.history_head + 1) %
		UPD9002_M74_TRACE_HISTORY_CAPACITY;
	if (m74_trace_state.history_count <
		UPD9002_M74_TRACE_HISTORY_CAPACITY) {
		m74_trace_state.history_count++;
	}
}

void upd9002_m74_trace_step_end(void) {

	UPD9002_M74_TRACE_RECORD *record;

	if (!m74_trace_state.active) {
		return;
	}
	record = &m74_trace_state.records[m74_trace_state.current_record];
	record->post_cs = CPU_CS;
	record->post_ip = CPU_IP;
	record->post_ss = CPU_SS;
	record->post_sp = CPU_SP;
	record->post_flags = CPU_FLAG;
	record->post_cs_base = CS_BASE;
	record->post_ss_base = SS_BASE;
	record->post_stack_physical =
		(SS_BASE + CPU_SP) & CPU_ADRSMASK;
	m74_trace_stack(record->stack_after, record->post_stack_physical);
	m74_trace_state.steps++;
	if ((record->opcode >= 0xd8) && (record->opcode <= 0xdf)) {
		fprintf(m74_trace_state.stream,
			"m74-fpu-opcode seq=%u cs=%04x ip=%04x phys=%05x "
			"bytes=%02x%02x%02x%02x%02x%02x%02x%02x "
			"ax=%04x bx=%04x cx=%04x dx=%04x si=%04x di=%04x "
			"sp=%04x flags=%04x post_cs=%04x post_ip=%04x post_sp=%04x\n",
			record->sequence, record->cs, record->ip, record->physical,
			record->bytes[0], record->bytes[1], record->bytes[2],
			record->bytes[3], record->bytes[4], record->bytes[5],
			record->bytes[6], record->bytes[7], record->ax, record->bx,
			record->cx, record->dx, record->si, record->di, record->sp,
			record->flags, record->post_cs, record->post_ip, record->post_sp);
	}
	if ((record->opcode == 0x27) || (record->opcode == 0x2f) ||
		(record->opcode == 0x37) || (record->opcode == 0x3f)) {
		fprintf(m74_trace_state.stream,
			"m74-bcd-opcode seq=%u cs=%04x ip=%04x phys=%05x opcode=%02x "
			"ax=%04x flags=%04x post_ax=%04x post_flags=%04x "
			"post_cs=%04x post_ip=%04x\n",
			record->sequence, record->cs, record->ip, record->physical,
			record->opcode, record->ax, record->flags,
			CPU_AX, CPU_FLAG, record->post_cs, record->post_ip);
	}
	if ((record->opcode == 0x0f) &&
		((record->bytes[1] == 0x20) || (record->bytes[1] == 0x22) ||
		 (record->bytes[1] == 0x26))) {
		fprintf(m74_trace_state.stream,
			"m74-bcd4s-opcode seq=%u cs=%04x ip=%04x phys=%05x "
			"bytes=%02x%02x ax=%04x flags=%04x post_ax=%04x "
			"post_flags=%04x post_cs=%04x post_ip=%04x\n",
			record->sequence, record->cs, record->ip, record->physical,
			record->bytes[0], record->bytes[1], record->ax, record->flags,
			CPU_AX, CPU_FLAG, record->post_cs, record->post_ip);
	}
	if (record->post_cs != record->cs) {
		fprintf(m74_trace_state.stream,
			"m74-control-transfer seq=%u from=%04x:%04x to=%04x:%04x "
			"src_phys=%05x target_phys=%05x opcode=%02x class=%s "
			"ss=%04x sp=%04x stack_phys=%05x stack_before=",
			record->sequence, record->cs, record->ip,
			record->post_cs, record->post_ip, record->physical,
			(CS_BASE + record->post_ip) & CPU_ADRSMASK, record->opcode,
			m74_trace_control_class(record->opcode), record->ss, record->sp,
			record->stack_physical);
		for (uint32_t byte = 0; byte < 8; byte++) {
			fprintf(m74_trace_state.stream, "%02x", record->stack_before[byte]);
		}
		fprintf(m74_trace_state.stream, " stack_after=");
		for (uint32_t byte = 0; byte < 8; byte++) {
			fprintf(m74_trace_state.stream, "%02x", record->stack_after[byte]);
		}
		fprintf(m74_trace_state.stream,
			" flags=%04x post_flags=%04x post_ss=%04x post_sp=%04x "
			"post_cs_base=%05x post_ss_base=%05x post_stack_phys=%05x\n",
			record->flags, record->post_flags, record->post_ss,
			record->post_sp, record->post_cs_base, record->post_ss_base,
			record->post_stack_physical);
	}
	m74_trace_address_update(record);
	if (!m74_trace_state.active) {
		return;
	}
	if (m74_trace_state.steps >= m74_trace_state.limit) {
		m74_trace_state.active = FALSE;
		m74_trace_dump("limit");
	}
}

void upd9002_m74_trace_memory_write(uint32_t address, uint16_t value,
		uint8_t width) {
	uint32_t end;

	if (!m74_trace_state.active) {
		return;
	}
	address &= CPU_ADRSMASK;
	end = address + width;
	if ((end > 0x39960) && (address < 0x39980)) {
		fprintf(m74_trace_state.stream,
			"m74-memory-write region=far-call-source step=%u cs=%04x "
			"ip=%04x physical=%05x value=%04x width=%u\n",
			m74_trace_state.steps, CPU_CS, CPU_IP, address, value, width);
	}
	if ((end > 0xa6400) && (address < 0xa6500)) {
		fprintf(m74_trace_state.stream,
			"m74-memory-write region=far-call-target step=%u cs=%04x "
			"ip=%04x physical=%05x value=%04x width=%u\n",
			m74_trace_state.steps, CPU_CS, CPU_IP, address, value, width);
	}
	if ((end > 0x7fff0) && (address < 0x7fffc)) {
		fprintf(m74_trace_state.stream,
			"m74-memory-write region=stack-frame-window step=%u cs=%04x "
			"ip=%04x physical=%05x value=%04x width=%u\n",
			m74_trace_state.steps, CPU_CS, CPU_IP, address, value, width);
	}
}

void upd9002_m74_trace_interrupt(uint8_t vector, uint8_t external) {
	uint32_t position;

	if (!m74_trace_state.active) {
		return;
	}
	position = m74_trace_state.interrupt_head;
	m74_trace_state.interrupts[position].sequence =
		m74_trace_state.steps;
	m74_trace_state.interrupts[position].vector = vector;
	m74_trace_state.interrupts[position].external = external;
	m74_trace_state.interrupt_head =
		(position + 1) % UPD9002_M74_TRACE_INTERRUPT_CAPACITY;
	if (m74_trace_state.interrupt_count <
		UPD9002_M74_TRACE_INTERRUPT_CAPACITY) {
		m74_trace_state.interrupt_count++;
	}
}
