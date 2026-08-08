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
	BOOL first_touch_seen;
	BOOL first_live_seen;
	BOOL dirty;
	uint16_t writer_cs;
	uint16_t writer_ip;
	uint32_t writer_physical;
	uint16_t writer_value;
	uint8_t writer_width;
	uint8_t writer_instruction[8];
} UPD9002_M74_VECTOR_WRITER;

typedef struct {
	uint16_t ds;
	uint16_t si;
	uint16_t es;
	uint16_t di;
	uint16_t cx;
	uint32_t source_physical;
	uint32_t destination_physical;
	uint8_t source[8];
	uint8_t destination[8];
} UPD9002_M74_INSTALLER_RECORD;

typedef struct {
	BOOL active;
	BOOL source_captured;
	BOOL scratch_captured;
	BOOL nonzero_path;
	BOOL returned_from_2730;
	BOOL reached_01e4;
	uint16_t source_ds;
	uint16_t source_di;
	uint16_t retf_ip;
	uint16_t retf_cs;
	uint8_t source[8];
	uint8_t scratch[8];
} UPD9002_M74_SERVICE_EVENT;

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
	uint32_t lifecycle_sequence;
	BOOL lifecycle_memory_initialized;
	BOOL lifecycle_memory_watch;
	BOOL vector_watch;
	BOOL cf_probe_active;
	uint32_t lifecycle_memory_step;
	UPD9002_M74_VECTOR_WRITER vector_writers[166];
	uint8_t lifecycle_target[0x100];
	uint8_t lifecycle_source[0x20];
	BOOL thunk_active;
	uint32_t thunk_entry_sequence;
	uint16_t thunk_caller_cs;
	uint16_t thunk_caller_ip;
	uint16_t thunk_return_ip;
	uint16_t thunk_dx;
	uint16_t thunk_si;
	uint16_t thunk_sp;
	uint32_t thunk_stack_physical;
	uint8_t thunk_stack[8];
	BOOL reachability;
	BOOL reachability_armed;
	BOOL free_boundary;
	BOOL free_boundary_captured;
	BOOL allocation_capture;
	BOOL reset_arm;
	BOOL reset_first_ok_captured;
	BOOL reset_injection_captured;
	BOOL reset_free_conversion_active;
	BOOL reset_free_conversion_captured;
	BOOL reset_wrapper_source_captured;
	BOOL allocation_entry_captured;
	BOOL allocation_01f7_captured;
	BOOL allocation_2730_captured;
	BOOL allocation_020d_captured;
	BOOL allocation_reset_page_captured;
	BOOL allocation_first_ok_page_captured;
	BOOL allocation_entry_page_captured;
	BOOL allocation_pre_2730_page_captured;
	BOOL allocation_post_2730_page_captured;
	BOOL allocation_pre_01e4_page_captured;
	BOOL installer_capture;
	BOOL installer_code_captured;
	uint16_t free_boundary_cs;
	uint16_t free_boundary_ds;
	uint16_t free_boundary_bx;
	uint16_t free_boundary_caller_ip;
	uint16_t free_boundary_offset;
	uint16_t free_boundary_base_segment;
	uint16_t free_boundary_top_segment;
	uint16_t free_boundary_writer_ip;
	uint16_t free_boundary_writer_ax;
	uint16_t free_boundary_writer_ds;
	uint16_t allocation_ds;
	uint16_t allocation_entry_ss;
	uint16_t allocation_entry_sp;
	uint16_t allocation_entry_ds;
	uint16_t allocation_entry_es;
	uint16_t allocation_entry_di;
	uint8_t allocation_entry_source[8];
	uint16_t allocation_01f7_ss;
	uint16_t allocation_01f7_sp;
	uint16_t allocation_01f7_ds;
	uint16_t allocation_01f7_si;
	uint16_t allocation_01f7_es;
	uint16_t allocation_01f7_di;
	uint16_t allocation_01f7_far_ip;
	uint16_t allocation_01f7_far_cs;
	uint16_t allocation_2730_ss;
	uint16_t allocation_2730_sp;
	uint16_t allocation_2730_ds;
	uint16_t allocation_2730_es;
	uint16_t allocation_2730_di;
	uint16_t allocation_2730_far_ip;
	uint16_t allocation_2730_far_cs;
	uint8_t allocation_2730_control;
	uint8_t allocation_2730_scratch[8];
	uint16_t allocation_020d_ss;
	uint16_t allocation_020d_sp;
	uint16_t allocation_020d_ds;
	uint16_t allocation_020d_si;
	uint16_t allocation_020d_es;
	uint16_t allocation_020d_di;
	uint16_t allocation_020d_far_ip;
	uint16_t allocation_020d_far_cs;
	uint32_t allocation_01f7_count;
	uint32_t allocation_2730_count;
	uint32_t allocation_020d_count;
	uint32_t allocation_2751_count;
	uint32_t allocation_0338_count;
	uint32_t allocation_03c2_count;
	uint32_t allocation_0191_count;
	uint32_t allocation_writer_03fd_count;
	uint32_t allocation_writer_1309_count;
	uint32_t allocation_writer_1350_count;
	uint32_t allocation_writer_273d_count;
	uint32_t allocation_writer_2748_count;
	uint32_t allocation_writer_286d_count;
	uint32_t reset_0180_count;
	uint32_t reset_2730_count;
	uint32_t reset_2751_count;
	uint32_t reset_0191_count;
	uint32_t reset_01e4_count;
	uint32_t reset_first_ok_0180;
	uint32_t reset_first_ok_2730;
	uint32_t reset_first_ok_2751;
	uint32_t reset_first_ok_0191;
	uint32_t reset_first_ok_01e4;
	uint32_t reset_injection_0180;
	uint32_t reset_injection_2730;
	uint32_t reset_injection_2751;
	uint32_t reset_injection_0191;
	uint32_t reset_injection_01e4;
	uint32_t reset_2d92_count;
	uint32_t reset_5a60_count;
	uint32_t reset_5c19_count;
	uint32_t reset_5a7f_count;
	uint32_t reset_33f7_count;
	uint32_t reset_34a7_count;
	uint32_t reset_34b0_count;
	uint32_t reset_34b4_count;
	uint32_t reset_34b8_count;
	uint32_t reset_34bd_count;
	uint16_t reset_free_ax;
	uint16_t reset_free_dx;
	uint16_t reset_free_ds;
	uint8_t reset_free_0652[8];
	uint8_t reset_free_065a[8];
	uint8_t reset_free_scratch[8];
	uint16_t reset_wrapper_ds;
	uint16_t reset_wrapper_di;
	uint8_t reset_wrapper_source[8];
	uint32_t reset_scratch_writer_counts[31];
	uint32_t reset_first_ok_scratch_writer_counts[31];
	uint32_t reset_injection_scratch_writer_counts[31];
	uint32_t reset_copy_total;
	int32_t reset_copy_active;
	uint16_t reset_copy_ds[2];
	uint16_t reset_copy_es[2];
	uint16_t reset_copy_di[2];
	uint16_t reset_copy_return_ip[2];
	uint8_t reset_copy_source[2][8];
	uint8_t reset_copy_before[2][8];
	uint8_t reset_copy_after[2][8];
	BOOL reset_copy_after_captured[2];
	uint8_t allocation_reset_page[0x100];
	uint8_t allocation_first_ok_page[0x100];
	uint8_t allocation_entry_page[0x100];
	uint8_t allocation_pre_2730_page[0x100];
	uint8_t allocation_post_2730_page[0x100];
	uint8_t allocation_pre_01e4_page[0x100];
	uint8_t allocation_work_area[0x100];
	uint8_t allocation_band[16][0x100];
	UPD9002_M74_SERVICE_EVENT startup_service[3];
	uint32_t startup_service_count;
	int32_t startup_service_active;
	UPD9002_M74_INSTALLER_RECORD installer_records[512];
	uint32_t installer_record_count;
	uint32_t installer_record_total;
	uint16_t installer_code_cs;
	uint16_t installer_code_ip;
	uint8_t installer_code[0x100];
	uint32_t reach_391d;
	uint32_t reach_3983;
	uint32_t reach_3985;
	uint32_t reach_3988;
	uint32_t reach_002a;
	uint32_t reach_01e4;
	uint32_t reach_int97;
	uint32_t reach_3816;
	uint32_t reach_3818;
	uint32_t reach_3821;
	uint32_t reach_3831;
	uint32_t reach_3835;
	uint32_t reach_3837;
	uint32_t reach_3976;
	uint32_t reach_397a;
	uint32_t reach_call_34bd, reach_call_34c0, reach_call_43b2, reach_call_49f9;
	uint32_t reach_call_75a8, reach_call_7f2a;
	uint32_t reach_0021, reach_0024, reach_0027, reach_002d;
	uint32_t reach_0030, reach_0033, reach_0036, reach_0180;
	uint16_t reach_entry_dx;
	uint16_t reach_entry_si;
	uint16_t reach_entry_ds;
	uint16_t reach_entry_ss;
	uint16_t reach_entry_sp;
	uint16_t reach_entry_caller_cs;
	uint16_t reach_entry_caller_ip;
	uint16_t reach_exit_ss;
	uint16_t reach_exit_sp;
	uint16_t reach_exit_word0;
	uint16_t reach_exit_word1;
	uint16_t reach_exit_word2;
	uint16_t reach_retf_ip;
	uint16_t reach_retf_cs;
	uint16_t reach_retf_ss;
	uint16_t reach_retf_sp;
	uint8_t reach_retf_memmode_va;
	uint8_t reach_retf_bytes[256];
	uint8_t reach_34c0_base[256];
	uint8_t reach_windows[4][256];
	uint16_t reach_stack_words[8];
	BOOL reach_391d_captured;
	BOOL reach_3983_captured;
	BOOL reach_3988_captured;
	uint16_t reach_3988_ss, reach_3988_sp;
	uint16_t reach_3988_word0, reach_3988_word1, reach_3988_word2;
	BOOL reach_01e4_captured;
} UPD9002_M74_TRACE_STATE;

static UPD9002_M74_TRACE_STATE m74_trace_state;

#define UPD9002_M74_TRACE_RECORD_CAPACITY 4096
#define UPD9002_M74_TRACE_INTERRUPT_CAPACITY 256
#define UPD9002_M74_TRACE_STABLE_COUNT 4096
#define UPD9002_M74_TRACE_HISTORY_CAPACITY 256
#define UPD9002_M74_VECTOR_SLOT_COUNT 166
#define UPD9002_M74_ALLOCATION_BAND_COUNT 16
#define UPD9002_M74_INSTALLER_RECORD_CAPACITY 512

static const uint32_t m74_trace_allocation_bases[
		UPD9002_M74_ALLOCATION_BAND_COUNT] = {
	0x2e800U, 0x30000U, 0x31000U, 0x32000U,
	0x33000U, 0x34000U, 0x34c00U, 0x35000U,
	0x36000U, 0x38000U, 0x3a000U, 0x3c000U,
	0x3e000U, 0x40000U, 0x41000U, 0x415a8U
};

static const uint16_t m74_trace_scratch_writer_ips[31] = {
	0x0200, 0x03fd, 0x091f, 0x092b, 0x0932, 0x1309, 0x1312,
	0x1350, 0x1388, 0x13de, 0x1633, 0x16ab, 0x16d0, 0x1774,
	0x17c7, 0x17d4, 0x1888, 0x188e, 0x1a84, 0x1aa6, 0x1ad1,
	0x223b, 0x2340, 0x241b, 0x2446, 0x2466, 0x2479, 0x248e,
	0x273d, 0x2748, 0x286d
};

static uint16_t m74_trace_vector_offset(uint32_t slot) {

	if (slot < 10) {
		return (uint16_t)(0x0280 + slot * 5);
	}
	return (uint16_t)(0x0a00 + (slot - 10) * 5);
}

static const char *m74_trace_vector_table(uint32_t slot) {

	return (slot < 10) ? "aux" : "main";
}

static uint32_t m74_trace_vector_index(uint32_t slot) {

	return (slot < 10) ? slot : slot - 10;
}

static void m74_trace_read_bytes(uint32_t address, uint8_t *bytes,
		uint32_t length) {
	int32_t before_clock;

	before_clock = CPU_REMCLOCK;
	for (uint32_t index = 0; index < length; index++) {
		bytes[index] = (uint8_t)upd9002_memoryread(
			(address + index) & CPU_ADRSMASK);
	}
	CPU_REMCLOCK = before_clock;
}

static uint16_t m74_trace_read_word(uint32_t address) {
	uint8_t bytes[2];

	m74_trace_read_bytes(address & CPU_ADRSMASK, bytes, sizeof(bytes));
	return (uint16_t)(bytes[0] | ((uint16_t)bytes[1] << 8));
}

static void m74_trace_capture_service_page(uint8_t *bytes) {

	m74_trace_read_bytes(0x03740U, bytes, 0x100);
}

static void m74_trace_startup_service_entry(void) {
	UPD9002_M74_SERVICE_EVENT *event;
	uint32_t source;

	if (!m74_trace_state.allocation_capture ||
		m74_trace_state.reachability_armed ||
		(m74_trace_state.startup_service_active >= 0) ||
		(m74_trace_state.startup_service_count >=
			(sizeof(m74_trace_state.startup_service) /
			 sizeof(m74_trace_state.startup_service[0])))) {
		return;
	}
	event = &m74_trace_state.startup_service[
		m74_trace_state.startup_service_count];
	event->active = TRUE;
	event->source_captured = TRUE;
	event->source_ds = CPU_DS;
	event->source_di = CPU_DI;
	source = (DS_BASE + CPU_DI) & CPU_ADRSMASK;
	m74_trace_read_bytes(source, event->source, sizeof(event->source));
	m74_trace_state.startup_service_active = (int32_t)
		m74_trace_state.startup_service_count;
	m74_trace_state.startup_service_count++;
}

static void m74_trace_current_instruction(uint8_t *bytes) {

	m74_trace_read_bytes((CS_BASE + CPU_IP) & CPU_ADRSMASK, bytes, 8);
}

static BOOL m74_trace_installer_destination(uint32_t physical) {

	return ((physical >= 0x10680U) && (physical < 0x106b2U)) ||
		((physical >= 0x10e00U) && (physical < 0x1110cU));
}

static void m74_trace_installer_event(void) {
	UPD9002_M74_INSTALLER_RECORD *record;
	uint32_t source;
	uint32_t destination;

	if (!m74_trace_state.configured || !m74_trace_state.installer_capture ||
		!(((CPU_CS == 0x19e3) && (CPU_IP == 0xc7eb)) ||
		  ((CPU_CS == 0x1cc5) && (CPU_IP == 0xc6bb)))) {
		return;
	}
	source = (DS_BASE + CPU_SI) & CPU_ADRSMASK;
	destination = (ES_BASE + CPU_DI) & CPU_ADRSMASK;
	m74_trace_state.installer_record_total++;
	if (!m74_trace_state.installer_code_captured) {
		m74_trace_state.installer_code_captured = TRUE;
		m74_trace_state.installer_code_cs = CPU_CS;
		m74_trace_state.installer_code_ip = (uint16_t)(CPU_IP - 0x80);
		m74_trace_read_bytes((CS_BASE + m74_trace_state.installer_code_ip) &
			CPU_ADRSMASK, m74_trace_state.installer_code,
			sizeof(m74_trace_state.installer_code));
	}
	if (!m74_trace_installer_destination(destination) ||
		(m74_trace_state.installer_record_count >=
		 UPD9002_M74_INSTALLER_RECORD_CAPACITY)) {
		return;
	}
	record = &m74_trace_state.installer_records[
		m74_trace_state.installer_record_count++];
	record->ds = CPU_DS;
	record->si = CPU_SI;
	record->es = CPU_ES;
	record->di = CPU_DI;
	record->cx = CPU_CX;
	record->source_physical = source;
	record->destination_physical = destination;
	m74_trace_read_bytes(source, record->source, sizeof(record->source));
	m74_trace_read_bytes(destination, record->destination,
		sizeof(record->destination));
}

static const char *m74_trace_vector_class(const uint8_t *bytes) {

	if (bytes[0] == 0xcb) {
		return "STUB";
	}
	if (bytes[0] == 0xea) {
		return "LIVE";
	}
	return "OTHER";
}

static void m74_trace_vector_snapshot(const char *label) {
	uint32_t live;
	uint32_t stub;
	uint32_t other;

	if (!m74_trace_state.configured || !m74_trace_state.vector_watch ||
		(m74_trace_state.stream == NULL)) {
		return;
	}
	live = 0;
	stub = 0;
	other = 0;
	for (uint32_t slot = 0; slot < UPD9002_M74_VECTOR_SLOT_COUNT; slot++) {
		uint8_t bytes[5];
		uint16_t offset;
		uint32_t physical;
		const char *classification;

		offset = m74_trace_vector_offset(slot);
		physical = 0x10400U + offset;
		m74_trace_read_bytes(physical, bytes, sizeof(bytes));
		classification = m74_trace_vector_class(bytes);
		if (bytes[0] == 0xea) {
			live++;
		}
		else if (bytes[0] == 0xcb) {
			stub++;
		}
		else {
			other++;
		}
		fprintf(m74_trace_state.stream,
			"m74-vector-slot stage=%s table=%s index=%u offset=%04x "
			"physical=%05x class=%s bytes=%02x%02x%02x%02x%02x",
			(label != NULL) ? label : "unknown",
			m74_trace_vector_table(slot), m74_trace_vector_index(slot),
			offset, physical, classification, bytes[0], bytes[1], bytes[2],
			bytes[3], bytes[4]);
		if (bytes[0] == 0xea) {
			fprintf(m74_trace_state.stream, " target=%04x:%04x",
				(uint16_t)(bytes[3] | ((uint16_t)bytes[4] << 8)),
				(uint16_t)(bytes[1] | ((uint16_t)bytes[2] << 8)));
		}
		fputc('\n', m74_trace_state.stream);
	}
	fprintf(m74_trace_state.stream,
		"m74-vector-summary stage=%s slots=%u live=%u stub=%u other=%u\n",
		(label != NULL) ? label : "unknown",
		UPD9002_M74_VECTOR_SLOT_COUNT, live, stub, other);
	fflush(m74_trace_state.stream);
}

static void m74_trace_ea_write(uint32_t address, uint16_t value,
		uint8_t width) {
	uint8_t instruction[8];
	uint32_t first_start;

	first_start = (address + CPU_ADRSMASK + 1U - 4U) & CPU_ADRSMASK;
	m74_trace_current_instruction(instruction);
	for (uint32_t candidate_index = 0; candidate_index < width + 4U;
		candidate_index++) {
		uint8_t bytes[5];
		uint32_t start;

		start = (first_start + candidate_index) & CPU_ADRSMASK;
		m74_trace_read_bytes(start, bytes, sizeof(bytes));
		for (uint32_t changed = 0; changed < width; changed++) {
			uint32_t changed_address;

			changed_address = (address + changed) & CPU_ADRSMASK;
			for (uint32_t byte = 0; byte < sizeof(bytes); byte++) {
				if (((start + byte) & CPU_ADRSMASK) == changed_address) {
					bytes[byte] = (uint8_t)(value >> (changed * 8));
				}
			}
		}
		if (bytes[0] != 0xea) {
			continue;
		}
		fprintf(m74_trace_state.stream,
			"m74-ea-sequence-write physical=%05x bytes="
			"%02x%02x%02x%02x%02x target=%04x:%04x writer=%04x:%04x "
			"instruction=%02x%02x%02x%02x%02x%02x%02x%02x\n",
			start, bytes[0], bytes[1], bytes[2], bytes[3], bytes[4],
			(uint16_t)(bytes[3] | ((uint16_t)bytes[4] << 8)),
			(uint16_t)(bytes[1] | ((uint16_t)bytes[2] << 8)),
			CPU_CS, CPU_IP, instruction[0], instruction[1], instruction[2],
			instruction[3], instruction[4], instruction[5], instruction[6],
			instruction[7]);
	}
}

static void m74_trace_vector_write_prepare(uint32_t address, uint16_t value,
		uint8_t width) {
	uint32_t end;

	if (!m74_trace_state.configured || !m74_trace_state.vector_watch) {
		return;
	}
	address &= CPU_ADRSMASK;
	end = address + width;
	for (uint32_t slot = 0; slot < UPD9002_M74_VECTOR_SLOT_COUNT; slot++) {
		UPD9002_M74_VECTOR_WRITER *writer;
		uint32_t slot_start;
		uint32_t slot_end;

		slot_start = 0x10400U + m74_trace_vector_offset(slot);
		slot_end = slot_start + 5;
		if ((end <= slot_start) || (address >= slot_end)) {
			continue;
		}
		writer = &m74_trace_state.vector_writers[slot];
		writer->dirty = TRUE;
		writer->writer_cs = CPU_CS;
		writer->writer_ip = CPU_IP;
		writer->writer_physical = address;
		writer->writer_value = value;
		writer->writer_width = width;
		m74_trace_current_instruction(writer->writer_instruction);
		if (!writer->first_touch_seen) {
			writer->first_touch_seen = TRUE;
			fprintf(m74_trace_state.stream,
				"m74-vector-first-touch table=%s index=%u offset=%04x "
				"slot_physical=%05x writer=%04x:%04x instruction="
				"%02x%02x%02x%02x%02x%02x%02x%02x write_physical=%05x "
				"value=%04x width=%u\n",
				m74_trace_vector_table(slot), m74_trace_vector_index(slot),
				m74_trace_vector_offset(slot), slot_start, CPU_CS, CPU_IP,
				writer->writer_instruction[0], writer->writer_instruction[1],
				writer->writer_instruction[2], writer->writer_instruction[3],
				writer->writer_instruction[4], writer->writer_instruction[5],
				writer->writer_instruction[6], writer->writer_instruction[7],
				address, value, width);
		}
	}
	m74_trace_ea_write(address, value, width);
}

static void m74_trace_vector_commit(void) {

	if (!m74_trace_state.configured || !m74_trace_state.vector_watch) {
		return;
	}
	for (uint32_t slot = 0; slot < UPD9002_M74_VECTOR_SLOT_COUNT; slot++) {
		UPD9002_M74_VECTOR_WRITER *writer;
		uint8_t bytes[5];
		uint32_t physical;

		writer = &m74_trace_state.vector_writers[slot];
		if (!writer->dirty) {
			continue;
		}
		writer->dirty = FALSE;
		physical = 0x10400U + m74_trace_vector_offset(slot);
		m74_trace_read_bytes(physical, bytes, sizeof(bytes));
		if ((bytes[0] == 0xea) && !writer->first_live_seen) {
			writer->first_live_seen = TRUE;
			fprintf(m74_trace_state.stream,
				"m74-vector-first-live table=%s index=%u offset=%04x "
				"physical=%05x target=%04x:%04x bytes="
				"%02x%02x%02x%02x%02x writer=%04x:%04x instruction="
				"%02x%02x%02x%02x%02x%02x%02x%02x write_physical=%05x "
				"value=%04x width=%u\n",
				m74_trace_vector_table(slot), m74_trace_vector_index(slot),
				m74_trace_vector_offset(slot), physical,
				(uint16_t)(bytes[3] | ((uint16_t)bytes[4] << 8)),
				(uint16_t)(bytes[1] | ((uint16_t)bytes[2] << 8)),
				bytes[0], bytes[1], bytes[2], bytes[3], bytes[4],
				writer->writer_cs, writer->writer_ip,
				writer->writer_instruction[0], writer->writer_instruction[1],
				writer->writer_instruction[2], writer->writer_instruction[3],
				writer->writer_instruction[4], writer->writer_instruction[5],
				writer->writer_instruction[6], writer->writer_instruction[7],
				writer->writer_physical, writer->writer_value,
				writer->writer_width);
		}
	}

}

static void m74_trace_vector_call(void) {
	uint8_t bytes[8];
	uint16_t offset;
	uint8_t slot_bytes[5];

	if (!m74_trace_state.configured || !m74_trace_state.vector_watch) {
		return;
	}
	m74_trace_current_instruction(bytes);
	if ((bytes[0] != 0x9a) || (bytes[3] != 0x40) ||
		(bytes[4] != 0x10)) {
		return;
	}
	offset = (uint16_t)(bytes[1] | ((uint16_t)bytes[2] << 8));
	m74_trace_read_bytes(0x10400U + offset, slot_bytes, sizeof(slot_bytes));
	fprintf(m74_trace_state.stream,
		"m74-vector-call cs=%04x ip=%04x instruction="
		"%02x%02x%02x%02x%02x offset=%04x slot_class=%s slot_bytes="
		"%02x%02x%02x%02x%02x dx=%04x si=%04x sp=%04x flags=%04x\n",
		CPU_CS, CPU_IP, bytes[0], bytes[1], bytes[2], bytes[3], bytes[4],
		offset, m74_trace_vector_class(slot_bytes), slot_bytes[0],
		slot_bytes[1], slot_bytes[2], slot_bytes[3], slot_bytes[4],
		CPU_DX, CPU_SI, CPU_SP, CPU_FLAG);
}

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

static void m74_trace_print_bytes(const char *label, BOOL captured,
		const uint8_t *bytes, uint32_t length) {

	fprintf(m74_trace_state.stream, "%s captured=%u bytes=", label,
		captured ? 1U : 0U);
	if (captured) {
		for (uint32_t index = 0; index < length; index++) {
			fprintf(m74_trace_state.stream, "%02x", bytes[index]);
		}
	}
	fputc('\n', m74_trace_state.stream);
}

void upd9002_m74_trace_configure(FILE *stream) {

	const char *value;
	const char *arm_value;
	const char *watch_value;
	const char *vector_value;
	const char *reachability_value;
	const char *free_boundary_value;
	const char *allocation_value;
	const char *installer_value;
	const char *reset_arm_value;
	char *end;
	unsigned long limit;
	unsigned long arm_command;

	ZeroMemory(&m74_trace_state, sizeof(m74_trace_state));
	value = getenv("VAEG_M74_CPU_TRACE_LIMIT");
	arm_value = getenv("VAEG_M74_CPU_TRACE_COMMAND");
	watch_value = getenv("VAEG_M74_LIFECYCLE_WATCH");
	vector_value = getenv("VAEG_M74_VECTOR_WATCH");
	reachability_value = getenv("VAEG_M74_REACHABILITY");
	free_boundary_value = getenv("VAEG_M74_FREE_BOUNDARY");
	allocation_value = getenv("VAEG_M74_ALLOCATION_CAPTURE");
	installer_value = getenv("VAEG_M74_INSTALLER_CAPTURE");
	reset_arm_value = getenv("VAEG_M74_RESET_ARM");
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
	m74_trace_state.lifecycle_memory_watch =
		(watch_value != NULL) && (watch_value[0] != '\0') &&
		(strcmp(watch_value, "0") != 0);
	m74_trace_state.vector_watch =
		(vector_value != NULL) && (vector_value[0] != '\0') &&
		(strcmp(vector_value, "0") != 0);
	m74_trace_state.reachability =
		(reachability_value != NULL) && (reachability_value[0] != '\0') &&
		(strcmp(reachability_value, "0") != 0);
	m74_trace_state.free_boundary =
		(free_boundary_value != NULL) && (free_boundary_value[0] != '\0') &&
		(strcmp(free_boundary_value, "0") != 0);
	m74_trace_state.allocation_capture =
		(allocation_value != NULL) && (allocation_value[0] != '\0') &&
		(strcmp(allocation_value, "0") != 0);
	m74_trace_state.installer_capture =
		(installer_value != NULL) && (installer_value[0] != '\0') &&
		(strcmp(installer_value, "0") != 0);
	m74_trace_state.reset_arm =
		(reset_arm_value != NULL) && (reset_arm_value[0] != '\0') &&
		(strcmp(reset_arm_value, "0") != 0);
	m74_trace_state.startup_service_active = -1;
	m74_trace_state.reset_copy_active = -1;
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
		"arm_command=%u vector_watch=%u allocation_capture=%u "
		"installer_capture=%u reset_arm=%u\n", m74_trace_state.limit,
		UPD9002_M74_TRACE_RECORD_CAPACITY, m74_trace_state.arm_command,
		m74_trace_state.vector_watch, m74_trace_state.allocation_capture,
		m74_trace_state.installer_capture,
		m74_trace_state.reset_arm);
}

void upd9002_m74_trace_stop(void) {

	if (m74_trace_state.configured && m74_trace_state.reset_arm) {
		fprintf(m74_trace_state.stream,
			"m74-reset-service totals=0180:%u,2730:%u,2751:%u,0191:%u,01e4:%u "
			"first_ok_captured=%u first_ok=0180:%u,2730:%u,2751:%u,0191:%u,01e4:%u "
			"injection_captured=%u injection=0180:%u,2730:%u,2751:%u,0191:%u,01e4:%u\n",
			m74_trace_state.reset_0180_count,
			m74_trace_state.reset_2730_count,
			m74_trace_state.reset_2751_count,
			m74_trace_state.reset_0191_count,
			m74_trace_state.reset_01e4_count,
			m74_trace_state.reset_first_ok_captured ? 1U : 0U,
			m74_trace_state.reset_first_ok_0180,
			m74_trace_state.reset_first_ok_2730,
			m74_trace_state.reset_first_ok_2751,
			m74_trace_state.reset_first_ok_0191,
			m74_trace_state.reset_first_ok_01e4,
			m74_trace_state.reset_injection_captured ? 1U : 0U,
			m74_trace_state.reset_injection_0180,
			m74_trace_state.reset_injection_2730,
			m74_trace_state.reset_injection_2751,
			m74_trace_state.reset_injection_0191,
			m74_trace_state.reset_injection_01e4);
		fprintf(m74_trace_state.stream,
			"m74-reset-residue counts=2d92:%u,5a60:%u,5c19:%u,5a7f:%u "
			"free_capture=%u free_dx_ax=%04x:%04x free_ds=%04x "
			"source0652=",
			m74_trace_state.reset_2d92_count,
			m74_trace_state.reset_5a60_count,
			m74_trace_state.reset_5c19_count,
			m74_trace_state.reset_5a7f_count,
			m74_trace_state.reset_free_conversion_captured ? 1U : 0U,
			m74_trace_state.reset_free_dx,
			m74_trace_state.reset_free_ax,
			m74_trace_state.reset_free_ds);
		for (uint32_t index = 0; index < 8; index++)
			fprintf(m74_trace_state.stream, "%02x",
				m74_trace_state.reset_free_0652[index]);
		fputs(" source065a=", m74_trace_state.stream);
		for (uint32_t index = 0; index < 8; index++)
			fprintf(m74_trace_state.stream, "%02x",
				m74_trace_state.reset_free_065a[index]);
		fputs(" scratch=", m74_trace_state.stream);
		for (uint32_t index = 0; index < 8; index++)
			fprintf(m74_trace_state.stream, "%02x",
				m74_trace_state.reset_free_scratch[index]);
		fprintf(m74_trace_state.stream,
			" setup=33f7:%u,34a7:%u,34b0:%u,34b4:%u,34b8:%u,34bd:%u "
			"wrapper_capture=%u wrapper_source=%04x:%04x:",
			m74_trace_state.reset_33f7_count,
			m74_trace_state.reset_34a7_count,
			m74_trace_state.reset_34b0_count,
			m74_trace_state.reset_34b4_count,
			m74_trace_state.reset_34b8_count,
			m74_trace_state.reset_34bd_count,
			m74_trace_state.reset_wrapper_source_captured ? 1U : 0U,
			m74_trace_state.reset_wrapper_ds,
			m74_trace_state.reset_wrapper_di);
		for (uint32_t index = 0; index < 8; index++)
			fprintf(m74_trace_state.stream, "%02x",
				m74_trace_state.reset_wrapper_source[index]);
		fputc('\n', m74_trace_state.stream);
		fputs("m74-reset-scratch-writers", m74_trace_state.stream);
		for (uint32_t index = 0; index < 31; index++) {
			fprintf(m74_trace_state.stream, " %04x=%u/%u/%u",
				m74_trace_scratch_writer_ips[index],
				m74_trace_state.reset_first_ok_scratch_writer_counts[index],
				m74_trace_state.reset_injection_scratch_writer_counts[index],
				m74_trace_state.reset_scratch_writer_counts[index]);
		}
		fputc('\n', m74_trace_state.stream);
		for (uint32_t event = 0; event < 2; event++) {
			fprintf(m74_trace_state.stream,
				"m74-reset-copy ordinal=%u total=%u source=%04x:%04x "
				"es=%04x return_ip=%04x bytes=",
				event + 1, m74_trace_state.reset_copy_total,
				m74_trace_state.reset_copy_ds[event],
				m74_trace_state.reset_copy_di[event],
				m74_trace_state.reset_copy_es[event],
				m74_trace_state.reset_copy_return_ip[event]);
			for (uint32_t index = 0; index < 8; index++)
				fprintf(m74_trace_state.stream, "%02x",
					m74_trace_state.reset_copy_source[event][index]);
			fputs(" before=", m74_trace_state.stream);
			for (uint32_t index = 0; index < 8; index++)
				fprintf(m74_trace_state.stream, "%02x",
					m74_trace_state.reset_copy_before[event][index]);
			fputs(" after=", m74_trace_state.stream);
			for (uint32_t index = 0; index < 8; index++)
				fprintf(m74_trace_state.stream, "%02x",
					m74_trace_state.reset_copy_after[event][index]);
			fprintf(m74_trace_state.stream, " after_captured=%u\n",
				m74_trace_state.reset_copy_after_captured[event] ? 1U : 0U);
		}
	}

	if (m74_trace_state.configured && m74_trace_state.free_boundary &&
		m74_trace_state.free_boundary_captured) {
		uint32_t lower = ((uint32_t)m74_trace_state.free_boundary_base_segment << 4) +
			m74_trace_state.free_boundary_offset + 1;
		uint32_t upper = ((uint32_t)m74_trace_state.free_boundary_top_segment << 4) +
			0x10000U;
		fprintf(m74_trace_state.stream,
			"m74-free-boundary print_at=%04x:f7b0 caller_ip=%04x "
			"string_at=%04x:%04x ds=%04x offset=%04x "
			"base_segment=%04x top_segment=%04x lower=%05x upper=%05x "
			"free=%u offset_writer=%04x writer_ax=%04x writer_ds=%04x\n",
			m74_trace_state.free_boundary_cs,
			m74_trace_state.free_boundary_caller_ip,
			m74_trace_state.free_boundary_cs,
			m74_trace_state.free_boundary_bx,
			m74_trace_state.free_boundary_ds,
			m74_trace_state.free_boundary_offset,
			m74_trace_state.free_boundary_base_segment,
			m74_trace_state.free_boundary_top_segment,
			lower & CPU_ADRSMASK, upper & CPU_ADRSMASK,
			(unsigned)((upper - lower) & CPU_ADRSMASK),
			m74_trace_state.free_boundary_writer_ip,
			m74_trace_state.free_boundary_writer_ax,
			m74_trace_state.free_boundary_writer_ds);
	}

	if (m74_trace_state.configured && m74_trace_state.allocation_capture &&
		(m74_trace_state.allocation_entry_captured ||
		 m74_trace_state.allocation_reset_page_captured ||
		 (m74_trace_state.startup_service_count != 0))) {
		fprintf(m74_trace_state.stream,
			"m74-allocation-context entry_ss=%04x entry_sp=%04x "
			"entry_ds=%04x entry_es=%04x entry_di=%04x entry_source=",
			m74_trace_state.allocation_entry_ss,
			m74_trace_state.allocation_entry_sp,
			m74_trace_state.allocation_entry_ds,
			m74_trace_state.allocation_entry_es,
			m74_trace_state.allocation_entry_di);
		for (uint32_t index = 0;
			index < sizeof(m74_trace_state.allocation_entry_source); index++)
			fprintf(m74_trace_state.stream, "%02x",
				m74_trace_state.allocation_entry_source[index]);
		fputs(" scratch=", m74_trace_state.stream);
		for (uint32_t index = 0;
			index < sizeof(m74_trace_state.allocation_2730_scratch); index++) {
			fprintf(m74_trace_state.stream, "%02x",
				m74_trace_state.allocation_2730_scratch[index]);
		}
		fprintf(m74_trace_state.stream,
			" 01f7_count=%u 01f7_ss=%04x 01f7_sp=%04x "
			"01f7_ds_si=%04x:%04x 01f7_es_di=%04x:%04x "
			"01f7_far=%04x,%04x 2730_count=%u 2730_ss=%04x "
			"2730_sp=%04x 2730_ds=%04x 2730_es=%04x 2730_di=%04x "
			"2730_far=%04x,%04x 2730_control=%02x 2751_count=%u "
			"0338_count=%u 03c2_count=%u returned_0191_count=%u "
			"writers=03fd:%u,1309:%u,1350:%u,273d:%u,2748:%u,286d:%u "
			"020d_count=%u 020d_ss=%04x "
			"020d_sp=%04x 020d_ds_si=%04x:%04x "
			"020d_es_di=%04x:%04x 020d_far=%04x,%04x ds_at_retf=%04x\n",
			m74_trace_state.allocation_01f7_count,
			m74_trace_state.allocation_01f7_ss,
			m74_trace_state.allocation_01f7_sp,
			m74_trace_state.allocation_01f7_ds,
			m74_trace_state.allocation_01f7_si,
			m74_trace_state.allocation_01f7_es,
			m74_trace_state.allocation_01f7_di,
			m74_trace_state.allocation_01f7_far_ip,
			m74_trace_state.allocation_01f7_far_cs,
			m74_trace_state.allocation_2730_count,
			m74_trace_state.allocation_2730_ss,
			m74_trace_state.allocation_2730_sp,
			m74_trace_state.allocation_2730_ds,
			m74_trace_state.allocation_2730_es,
			m74_trace_state.allocation_2730_di,
			m74_trace_state.allocation_2730_far_ip,
			m74_trace_state.allocation_2730_far_cs,
			m74_trace_state.allocation_2730_control,
			m74_trace_state.allocation_2751_count,
			m74_trace_state.allocation_0338_count,
			m74_trace_state.allocation_03c2_count,
			m74_trace_state.allocation_0191_count,
			m74_trace_state.allocation_writer_03fd_count,
			m74_trace_state.allocation_writer_1309_count,
			m74_trace_state.allocation_writer_1350_count,
			m74_trace_state.allocation_writer_273d_count,
			m74_trace_state.allocation_writer_2748_count,
			m74_trace_state.allocation_writer_286d_count,
			m74_trace_state.allocation_020d_count,
			m74_trace_state.allocation_020d_ss,
			m74_trace_state.allocation_020d_sp,
			m74_trace_state.allocation_020d_ds,
			m74_trace_state.allocation_020d_si,
			m74_trace_state.allocation_020d_es,
			m74_trace_state.allocation_020d_di,
			m74_trace_state.allocation_020d_far_ip,
			m74_trace_state.allocation_020d_far_cs,
			m74_trace_state.allocation_ds);
		m74_trace_print_bytes("m74-service-page checkpoint=reset",
			m74_trace_state.allocation_reset_page_captured,
			m74_trace_state.allocation_reset_page,
			sizeof(m74_trace_state.allocation_reset_page));
		m74_trace_print_bytes("m74-service-page checkpoint=first-ok",
			m74_trace_state.allocation_first_ok_page_captured,
			m74_trace_state.allocation_first_ok_page,
			sizeof(m74_trace_state.allocation_first_ok_page));
		m74_trace_print_bytes("m74-service-page checkpoint=pre-copy",
			m74_trace_state.allocation_entry_page_captured,
			m74_trace_state.allocation_entry_page,
			sizeof(m74_trace_state.allocation_entry_page));
		m74_trace_print_bytes("m74-service-page checkpoint=pre-2730",
			m74_trace_state.allocation_pre_2730_page_captured,
			m74_trace_state.allocation_pre_2730_page,
			sizeof(m74_trace_state.allocation_pre_2730_page));
		m74_trace_print_bytes("m74-service-page checkpoint=post-2730",
			m74_trace_state.allocation_post_2730_page_captured,
			m74_trace_state.allocation_post_2730_page,
			sizeof(m74_trace_state.allocation_post_2730_page));
		m74_trace_print_bytes("m74-service-page checkpoint=pre-01e4",
			m74_trace_state.allocation_pre_01e4_page_captured,
			m74_trace_state.allocation_pre_01e4_page,
			sizeof(m74_trace_state.allocation_pre_01e4_page));
		for (uint32_t event_index = 0;
			event_index < m74_trace_state.startup_service_count; event_index++) {
			UPD9002_M74_SERVICE_EVENT *event =
				&m74_trace_state.startup_service[event_index];
			fprintf(m74_trace_state.stream,
				"m74-startup-service index=%u source=%04x:%04x bytes=",
				event_index, event->source_ds, event->source_di);
			for (uint32_t index = 0; index < sizeof(event->source); index++) {
				fprintf(m74_trace_state.stream, "%02x", event->source[index]);
			}
			fputs(" scratch=", m74_trace_state.stream);
			if (event->scratch_captured) {
				for (uint32_t index = 0; index < sizeof(event->scratch); index++) {
					fprintf(m74_trace_state.stream, "%02x", event->scratch[index]);
				}
			}
			fprintf(m74_trace_state.stream,
				" scratch_captured=%u nonzero_path=%u returned_2730=%u "
				"reached_01e4=%u retf=%04x:%04x\n",
				event->scratch_captured ? 1U : 0U,
				event->nonzero_path ? 1U : 0U,
				event->returned_from_2730 ? 1U : 0U,
				event->reached_01e4 ? 1U : 0U,
				event->retf_cs, event->retf_ip);
		}
		fprintf(m74_trace_state.stream, "m74-allocation-workarea ds=%04x bytes=",
			m74_trace_state.allocation_ds);
		for (uint32_t index = 0;
			index < sizeof(m74_trace_state.allocation_work_area); index++)
			fprintf(m74_trace_state.stream, "%02x",
				m74_trace_state.allocation_work_area[index]);
		fputc('\n', m74_trace_state.stream);
		for (uint32_t range = 0;
			range < UPD9002_M74_ALLOCATION_BAND_COUNT; range++) {
			fprintf(m74_trace_state.stream,
				"m74-allocation-band address=%05x bytes=",
				m74_trace_allocation_bases[range]);
			for (uint32_t index = 0;
				index < sizeof(m74_trace_state.allocation_band[range]); index++)
				fprintf(m74_trace_state.stream, "%02x",
					m74_trace_state.allocation_band[range][index]);
			fputc('\n', m74_trace_state.stream);
		}
	}

	if (m74_trace_state.configured && m74_trace_state.installer_capture) {
		fprintf(m74_trace_state.stream,
			"m74-installer-summary total_hits=%u table_hits=%u "
			"capacity=%u code_captured=%u\n",
			m74_trace_state.installer_record_total,
			m74_trace_state.installer_record_count,
			UPD9002_M74_INSTALLER_RECORD_CAPACITY,
			m74_trace_state.installer_code_captured);
		if (m74_trace_state.installer_code_captured) {
			fprintf(m74_trace_state.stream,
				"m74-installer-code cs=%04x ip=%04x bytes=",
				m74_trace_state.installer_code_cs,
				m74_trace_state.installer_code_ip);
			for (uint32_t index = 0;
				index < sizeof(m74_trace_state.installer_code); index++)
				fprintf(m74_trace_state.stream, "%02x",
					m74_trace_state.installer_code[index]);
			fputc('\n', m74_trace_state.stream);
		}
		for (uint32_t index = 0;
			index < m74_trace_state.installer_record_count; index++) {
			UPD9002_M74_INSTALLER_RECORD *record =
				&m74_trace_state.installer_records[index];
			fprintf(m74_trace_state.stream,
				"m74-installer-record index=%u ds_si=%04x:%04x "
				"es_di=%04x:%04x cx=%04x source_phys=%05x "
				"destination_phys=%05x source=", index, record->ds,
				record->si, record->es, record->di, record->cx,
				record->source_physical, record->destination_physical);
			for (uint32_t byte = 0; byte < sizeof(record->source); byte++)
				fprintf(m74_trace_state.stream, "%02x", record->source[byte]);
			fputs(" destination=", m74_trace_state.stream);
			for (uint32_t byte = 0; byte < sizeof(record->destination); byte++)
				fprintf(m74_trace_state.stream, "%02x",
					record->destination[byte]);
			fputc('\n', m74_trace_state.stream);
		}
	}

	if (m74_trace_state.configured && m74_trace_state.reachability) {
		fprintf(m74_trace_state.stream,
			"m74-reachability counts 391d=%u 3983=%u 3985=%u 3988=%u 002a=%u 01e4=%u int97=%u 3816=%u 3818=%u 3821=%u 3831=%u 3835=%u 3837=%u 3976=%u 397a=%u call34bd=%u call34c0=%u call43b2=%u call49f9=%u call75a8=%u call7f2a=%u j0021=%u j0024=%u j0027=%u j002d=%u j0030=%u j0033=%u j0036=%u e0180=%u\n",
			m74_trace_state.reach_391d, m74_trace_state.reach_3983,
			m74_trace_state.reach_3985, m74_trace_state.reach_3988, m74_trace_state.reach_002a,
			m74_trace_state.reach_01e4, m74_trace_state.reach_int97,
			m74_trace_state.reach_3816, m74_trace_state.reach_3818,
			m74_trace_state.reach_3821, m74_trace_state.reach_3831,
			m74_trace_state.reach_3835, m74_trace_state.reach_3837,
			m74_trace_state.reach_3976, m74_trace_state.reach_397a,
			m74_trace_state.reach_call_34bd, m74_trace_state.reach_call_34c0,
			m74_trace_state.reach_call_43b2,
			m74_trace_state.reach_call_49f9, m74_trace_state.reach_call_75a8,
			m74_trace_state.reach_call_7f2a, m74_trace_state.reach_0021,
			m74_trace_state.reach_0024, m74_trace_state.reach_0027,
			m74_trace_state.reach_002d, m74_trace_state.reach_0030,
			m74_trace_state.reach_0033, m74_trace_state.reach_0036,
			m74_trace_state.reach_0180);
		fprintf(m74_trace_state.stream, "m74-reachability stack8=");
		for (uint32_t index = 0; index < 8; index++) fprintf(m74_trace_state.stream, "%s%04x", index ? "," : "", m74_trace_state.reach_stack_words[index]);
		fprintf(m74_trace_state.stream, " target256=");
		for (uint32_t index = 0; index < 256; index++) fprintf(m74_trace_state.stream, "%02x", m74_trace_state.reach_retf_bytes[index]);
		fprintf(m74_trace_state.stream, "\n");
		fprintf(m74_trace_state.stream, "m74-reachability base34c0=");
		for (uint32_t index = 0; index < 256; index++) fprintf(m74_trace_state.stream, "%02x", m74_trace_state.reach_34c0_base[index]);
		fprintf(m74_trace_state.stream, "\n");
		for (uint32_t window = 0; window < 4; window++) {
			fprintf(m74_trace_state.stream, "m74-reachability window%u=", window);
			for (uint32_t index = 0; index < 256; index++) fprintf(m74_trace_state.stream, "%02x", m74_trace_state.reach_windows[window][index]);
			fprintf(m74_trace_state.stream, "\n");
		}
		fprintf(m74_trace_state.stream, "m74-reachability ret3988 ss=%04x sp=%04x words=%04x,%04x,%04x\n",
			m74_trace_state.reach_3988_ss, m74_trace_state.reach_3988_sp,
			m74_trace_state.reach_3988_word0, m74_trace_state.reach_3988_word1,
			m74_trace_state.reach_3988_word2);
		fprintf(m74_trace_state.stream,
			"m74-reachability entry dx=%04x si=%04x ds=%04x ss=%04x sp=%04x "
			"caller=%04x:%04x exit_ss=%04x exit_sp=%04x exit_words=%04x,%04x,%04x "
			"retf_at=%04x:%04x retf_ss=%04x retf_sp=%04x retf=%04x:%04x "
			"memmode_va=%u mapper_path=%s backing=mem bytes=",
			m74_trace_state.reach_entry_dx, m74_trace_state.reach_entry_si,
			m74_trace_state.reach_entry_ds, m74_trace_state.reach_entry_ss,
			m74_trace_state.reach_entry_sp,
			m74_trace_state.reach_entry_caller_cs,
			m74_trace_state.reach_entry_caller_ip,
			m74_trace_state.reach_exit_ss,
			m74_trace_state.reach_exit_sp,
			m74_trace_state.reach_exit_word0,
			m74_trace_state.reach_exit_word1,
			m74_trace_state.reach_exit_word2,
			0xe000, 0x01e4,
			m74_trace_state.reach_retf_ss,
			m74_trace_state.reach_retf_sp,
			m74_trace_state.reach_retf_cs,
			m74_trace_state.reach_retf_ip,
			m74_trace_state.reach_retf_memmode_va,
			m74_trace_state.reach_retf_memmode_va ?
				"va-low-direct" : "generic-low-direct");
		for (uint32_t index = 0; index < sizeof(m74_trace_state.reach_retf_bytes); index++)
			fprintf(m74_trace_state.stream, "%02x", m74_trace_state.reach_retf_bytes[index]);
		fputc('\n', m74_trace_state.stream);
	}
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
	m74_trace_state.thunk_active = FALSE;
	if (m74_trace_state.reachability) {
		m74_trace_state.reachability_armed = TRUE;
		m74_trace_state.reach_391d = 0;
		m74_trace_state.reach_3983 = 0;
		m74_trace_state.reach_3988 = 0;
		m74_trace_state.reach_3985 = 0;
		m74_trace_state.reach_002a = 0;
		m74_trace_state.reach_01e4 = 0;
		m74_trace_state.reach_call_34bd = 0;
		m74_trace_state.reach_call_34c0 = 0;
		m74_trace_state.reach_call_43b2 = 0;
		m74_trace_state.reach_call_49f9 = 0;
		m74_trace_state.reach_call_75a8 = 0;
		m74_trace_state.reach_call_7f2a = 0;
		m74_trace_state.reach_0021 = 0;
		m74_trace_state.reach_0024 = 0;
		m74_trace_state.reach_0027 = 0;
		m74_trace_state.reach_002d = 0;
		m74_trace_state.reach_0030 = 0;
		m74_trace_state.reach_0033 = 0;
		m74_trace_state.reach_0036 = 0;
		m74_trace_state.reach_0180 = 0;
		m74_trace_state.allocation_entry_captured = FALSE;
		m74_trace_state.allocation_01f7_captured = FALSE;
		m74_trace_state.allocation_2730_captured = FALSE;
		m74_trace_state.allocation_020d_captured = FALSE;
		m74_trace_state.allocation_01f7_count = 0;
		m74_trace_state.allocation_2730_count = 0;
		m74_trace_state.allocation_020d_count = 0;
		m74_trace_state.allocation_2751_count = 0;
		m74_trace_state.allocation_0338_count = 0;
		m74_trace_state.allocation_03c2_count = 0;
		m74_trace_state.allocation_0191_count = 0;
		m74_trace_state.allocation_writer_03fd_count = 0;
		m74_trace_state.allocation_writer_1309_count = 0;
		m74_trace_state.allocation_writer_1350_count = 0;
		m74_trace_state.allocation_writer_273d_count = 0;
		m74_trace_state.allocation_writer_2748_count = 0;
		m74_trace_state.allocation_writer_286d_count = 0;
		m74_trace_state.allocation_entry_page_captured = FALSE;
		m74_trace_state.allocation_pre_2730_page_captured = FALSE;
		m74_trace_state.allocation_post_2730_page_captured = FALSE;
		m74_trace_state.allocation_pre_01e4_page_captured = FALSE;
		m74_trace_state.reach_391d_captured = FALSE;
		m74_trace_state.reach_3983_captured = FALSE;
		m74_trace_state.reach_3988_captured = FALSE;
		m74_trace_state.reach_01e4_captured = FALSE;
		ZeroMemory(m74_trace_state.reach_retf_bytes,
			sizeof(m74_trace_state.reach_retf_bytes));
		ZeroMemory(m74_trace_state.reach_34c0_base,
			sizeof(m74_trace_state.reach_34c0_base));
		ZeroMemory(m74_trace_state.reach_windows, sizeof(m74_trace_state.reach_windows));
		ZeroMemory(m74_trace_state.reach_stack_words, sizeof(m74_trace_state.reach_stack_words));
	}
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

	m74_trace_installer_event();

	if (m74_trace_state.configured && m74_trace_state.reset_arm &&
		(CPU_CS == 0xe000)) {
		if (CPU_IP == 0x0180) m74_trace_state.reset_0180_count++;
		if (CPU_IP == 0x2730) m74_trace_state.reset_2730_count++;
		if (CPU_IP == 0x2751) m74_trace_state.reset_2751_count++;
		if (CPU_IP == 0x0191) m74_trace_state.reset_0191_count++;
		if (CPU_IP == 0x01e4) m74_trace_state.reset_01e4_count++;
		if (CPU_IP == 0x2d92) {
			m74_trace_state.reset_2d92_count++;
			if (!m74_trace_state.reset_free_conversion_captured) {
				m74_trace_state.reset_free_conversion_active = TRUE;
				m74_trace_state.reset_free_ax = CPU_AX;
				m74_trace_state.reset_free_dx = CPU_DX;
				m74_trace_state.reset_free_ds = CPU_DS;
			}
		}
		if (CPU_IP == 0x5a60) m74_trace_state.reset_5a60_count++;
		if (CPU_IP == 0x5c19) {
			m74_trace_state.reset_5c19_count++;
			if (m74_trace_state.reset_free_conversion_active) {
				m74_trace_read_bytes((DS_BASE + 0x0652) & CPU_ADRSMASK,
					m74_trace_state.reset_free_0652, 8);
				m74_trace_read_bytes((DS_BASE + 0x065a) & CPU_ADRSMASK,
					m74_trace_state.reset_free_065a, 8);
			}
		}
		if (CPU_IP == 0x5a7f) {
			m74_trace_state.reset_5a7f_count++;
			if (m74_trace_state.reset_free_conversion_active) {
				m74_trace_read_bytes(0x037cfU,
					m74_trace_state.reset_free_scratch, 8);
				m74_trace_state.reset_free_conversion_active = FALSE;
				m74_trace_state.reset_free_conversion_captured = TRUE;
			}
		}
		if (CPU_IP == 0x33f7) m74_trace_state.reset_33f7_count++;
		if (CPU_IP == 0x34a7) m74_trace_state.reset_34a7_count++;
		if (CPU_IP == 0x34b0) m74_trace_state.reset_34b0_count++;
		if (CPU_IP == 0x34b4) m74_trace_state.reset_34b4_count++;
		if (CPU_IP == 0x34b8) m74_trace_state.reset_34b8_count++;
		if (CPU_IP == 0x34bd) {
			m74_trace_state.reset_34bd_count++;
			if (m74_trace_state.reachability_armed &&
				!m74_trace_state.reset_wrapper_source_captured) {
				m74_trace_state.reset_wrapper_source_captured = TRUE;
				m74_trace_state.reset_wrapper_ds = CPU_DS;
				m74_trace_state.reset_wrapper_di = CPU_DI;
				m74_trace_read_bytes((DS_BASE + CPU_DI) & CPU_ADRSMASK,
					m74_trace_state.reset_wrapper_source, 8);
			}
		}
		if (CPU_IP == 0x01f7) {
			uint32_t event = m74_trace_state.reset_copy_total++;
			if (event < 2) {
				uint32_t stack = (SS_BASE + CPU_SP) & CPU_ADRSMASK;
				m74_trace_state.reset_copy_active = (int32_t)event;
				m74_trace_state.reset_copy_ds[event] = CPU_DS;
				m74_trace_state.reset_copy_es[event] = CPU_ES;
				m74_trace_state.reset_copy_di[event] = CPU_DI;
				m74_trace_state.reset_copy_return_ip[event] =
					m74_trace_read_word(stack);
				m74_trace_read_bytes((DS_BASE + CPU_DI) & CPU_ADRSMASK,
					m74_trace_state.reset_copy_source[event], 8);
				m74_trace_read_bytes(0x037cfU,
					m74_trace_state.reset_copy_before[event], 8);
			}
		}
		if ((CPU_IP == 0x0202) &&
			(m74_trace_state.reset_copy_active >= 0)) {
			uint32_t event = (uint32_t)m74_trace_state.reset_copy_active;
			m74_trace_read_bytes(0x037cfU,
				m74_trace_state.reset_copy_after[event], 8);
			m74_trace_state.reset_copy_after_captured[event] = TRUE;
			m74_trace_state.reset_copy_active = -1;
		}
		for (uint32_t writer = 0; writer < 31; writer++) {
			if (CPU_IP == m74_trace_scratch_writer_ips[writer])
				m74_trace_state.reset_scratch_writer_counts[writer]++;
		}
	}

	if (m74_trace_state.configured && m74_trace_state.free_boundary &&
		(CPU_CS == 0xe000) &&
		((CPU_IP == 0x804b) || (CPU_IP == 0xe992))) {
		m74_trace_state.free_boundary_writer_ip = CPU_IP;
		m74_trace_state.free_boundary_writer_ax = CPU_AX;
		m74_trace_state.free_boundary_writer_ds = CPU_DS;
	}

	if (m74_trace_state.configured && m74_trace_state.free_boundary &&
		!m74_trace_state.free_boundary_captured &&
		(CPU_IP == 0xf7b0)) {
		static const uint8_t marker[] = " bytes free";
		uint8_t candidate[sizeof(marker) - 1];
		uint32_t base = DS_BASE & CPU_ADRSMASK;
		uint32_t stack = (SS_BASE + CPU_SP) & CPU_ADRSMASK;
		m74_trace_read_bytes((CS_BASE + CPU_BX) & CPU_ADRSMASK, candidate,
			sizeof(candidate));
		if (memcmp(candidate, marker, sizeof(candidate)) == 0) {
			m74_trace_state.free_boundary_captured = TRUE;
			m74_trace_state.free_boundary_cs = CPU_CS;
			m74_trace_state.free_boundary_ds = CPU_DS;
			m74_trace_state.free_boundary_bx = CPU_BX;
			m74_trace_state.free_boundary_caller_ip = (uint16_t)(
				upd9002_memoryread(stack) |
				(upd9002_memoryread(stack + 1) << 8));
		m74_trace_state.free_boundary_offset = (uint16_t)(
			upd9002_memoryread(base + 0x0004) |
			(upd9002_memoryread(base + 0x0005) << 8));
		m74_trace_state.free_boundary_base_segment = (uint16_t)(
			upd9002_memoryread(base + 0x0010) |
			(upd9002_memoryread(base + 0x0011) << 8));
			m74_trace_state.free_boundary_top_segment = (uint16_t)(
				upd9002_memoryread(base + 0x001a) |
				(upd9002_memoryread(base + 0x001b) << 8));
		}
	}

	if (m74_trace_state.configured && m74_trace_state.allocation_capture &&
		!m74_trace_state.reachability_armed && (CPU_CS == 0xe000)) {
		UPD9002_M74_SERVICE_EVENT *event = NULL;

		if (CPU_IP == 0x0180) {
			m74_trace_startup_service_entry();
		}
		if ((m74_trace_state.startup_service_active >= 0) &&
			((uint32_t)m74_trace_state.startup_service_active <
			 m74_trace_state.startup_service_count)) {
			event = &m74_trace_state.startup_service[
				m74_trace_state.startup_service_active];
			if ((CPU_IP == 0x2730) && !event->scratch_captured) {
				event->scratch_captured = TRUE;
				m74_trace_read_bytes((DS_BASE + 0x008f) & CPU_ADRSMASK,
					event->scratch, sizeof(event->scratch));
			}
			if (CPU_IP == 0x2751) {
				event->nonzero_path = TRUE;
			}
			if (CPU_IP == 0x0191) {
				event->returned_from_2730 = TRUE;
			}
			if (CPU_IP == 0x01e4) {
				uint32_t stack = (SS_BASE + CPU_SP) & CPU_ADRSMASK;
				event->reached_01e4 = TRUE;
				event->retf_ip = m74_trace_read_word(stack);
				event->retf_cs = m74_trace_read_word(stack + 2);
				event->active = FALSE;
				m74_trace_state.startup_service_active = -1;
			}
		}
	}

	if (m74_trace_state.configured && m74_trace_state.reachability &&
		m74_trace_state.reachability_armed) {
		if ((CPU_CS == 0xe000) && (CPU_IP == 0x3816)) m74_trace_state.reach_3816++;
		if ((CPU_CS == 0xe000) && (CPU_IP == 0x3818)) m74_trace_state.reach_3818++;
		if ((CPU_CS == 0xe000) && (CPU_IP == 0x3821)) m74_trace_state.reach_3821++;
		if ((CPU_CS == 0xe000) && (CPU_IP == 0x3831)) m74_trace_state.reach_3831++;
		if ((CPU_CS == 0xe000) && (CPU_IP == 0x3835)) m74_trace_state.reach_3835++;
		if ((CPU_CS == 0xe000) && (CPU_IP == 0x3837)) m74_trace_state.reach_3837++;
		if ((CPU_CS == 0xe000) && (CPU_IP == 0x3976)) m74_trace_state.reach_3976++;
		if ((CPU_CS == 0xe000) && (CPU_IP == 0x397a)) m74_trace_state.reach_397a++;
		if ((CPU_CS == 0xe000) && (CPU_IP == 0x34bd)) m74_trace_state.reach_call_34bd++;
		if ((CPU_CS == 0xe000) && (CPU_IP == 0x34c0)) m74_trace_state.reach_call_34c0++;
		if ((CPU_CS == 0xe000) && (CPU_IP == 0x43b2)) m74_trace_state.reach_call_43b2++;
		if ((CPU_CS == 0xe000) && (CPU_IP == 0x49f9)) m74_trace_state.reach_call_49f9++;
		if ((CPU_CS == 0xe000) && (CPU_IP == 0x75a8)) m74_trace_state.reach_call_75a8++;
		if ((CPU_CS == 0xe000) && (CPU_IP == 0x7f2a)) m74_trace_state.reach_call_7f2a++;
		if ((CPU_CS == 0xe000) && (CPU_IP == 0x0021)) m74_trace_state.reach_0021++;
		if ((CPU_CS == 0xe000) && (CPU_IP == 0x0024)) m74_trace_state.reach_0024++;
		if ((CPU_CS == 0xe000) && (CPU_IP == 0x0027)) m74_trace_state.reach_0027++;
		if ((CPU_CS == 0xe000) && (CPU_IP == 0x002d)) m74_trace_state.reach_002d++;
		if ((CPU_CS == 0xe000) && (CPU_IP == 0x0030)) m74_trace_state.reach_0030++;
		if ((CPU_CS == 0xe000) && (CPU_IP == 0x0033)) m74_trace_state.reach_0033++;
		if ((CPU_CS == 0xe000) && (CPU_IP == 0x0036)) m74_trace_state.reach_0036++;
		if ((CPU_CS == 0xe000) && (CPU_IP == 0x0180)) {
			m74_trace_state.reach_0180++;
			if (m74_trace_state.allocation_capture &&
				!m74_trace_state.allocation_entry_captured) {
				if (!m74_trace_state.allocation_entry_page_captured) {
					m74_trace_state.allocation_entry_page_captured = TRUE;
					m74_trace_capture_service_page(
						m74_trace_state.allocation_entry_page);
				}
				uint32_t source = (DS_BASE + CPU_DI) & CPU_ADRSMASK;
				m74_trace_state.allocation_entry_captured = TRUE;
				m74_trace_state.allocation_entry_ss = CPU_SS;
				m74_trace_state.allocation_entry_sp = CPU_SP;
				m74_trace_state.allocation_entry_ds = CPU_DS;
				m74_trace_state.allocation_entry_es = CPU_ES;
				m74_trace_state.allocation_entry_di = CPU_DI;
				m74_trace_read_bytes(source,
					m74_trace_state.allocation_entry_source,
					sizeof(m74_trace_state.allocation_entry_source));
			}
		}
		if (m74_trace_state.allocation_capture &&
			m74_trace_state.allocation_entry_captured &&
			(CPU_CS == 0xe000)) {
			uint32_t stack = (SS_BASE + CPU_SP) & CPU_ADRSMASK;

			if (CPU_IP == 0x01f7) {
				m74_trace_state.allocation_01f7_count++;
				if (!m74_trace_state.allocation_01f7_captured) {
					m74_trace_state.allocation_01f7_captured = TRUE;
					m74_trace_state.allocation_01f7_ss = CPU_SS;
					m74_trace_state.allocation_01f7_sp = CPU_SP;
					m74_trace_state.allocation_01f7_ds = CPU_DS;
					m74_trace_state.allocation_01f7_si = CPU_SI;
					m74_trace_state.allocation_01f7_es = CPU_ES;
					m74_trace_state.allocation_01f7_di = CPU_DI;
					m74_trace_state.allocation_01f7_far_ip =
						m74_trace_read_word(stack + 8);
					m74_trace_state.allocation_01f7_far_cs =
						m74_trace_read_word(stack + 10);
				}
			}
			if (CPU_IP == 0x2730) {
				m74_trace_state.allocation_2730_count++;
				if (!m74_trace_state.allocation_2730_captured) {
					m74_trace_state.allocation_2730_captured = TRUE;
					m74_trace_state.allocation_2730_ss = CPU_SS;
					m74_trace_state.allocation_2730_sp = CPU_SP;
					m74_trace_state.allocation_2730_ds = CPU_DS;
					m74_trace_state.allocation_2730_es = CPU_ES;
					m74_trace_state.allocation_2730_di = CPU_DI;
					m74_trace_state.allocation_2730_far_ip =
						m74_trace_read_word(stack + 8);
					m74_trace_state.allocation_2730_far_cs =
						m74_trace_read_word(stack + 10);
					m74_trace_read_bytes((DS_BASE + 0x96) & CPU_ADRSMASK,
						&m74_trace_state.allocation_2730_control, 1);
					m74_trace_read_bytes((DS_BASE + 0x008f) & CPU_ADRSMASK,
						m74_trace_state.allocation_2730_scratch,
						sizeof(m74_trace_state.allocation_2730_scratch));
					if (!m74_trace_state.allocation_pre_2730_page_captured) {
						m74_trace_state.allocation_pre_2730_page_captured = TRUE;
						m74_trace_capture_service_page(
							m74_trace_state.allocation_pre_2730_page);
					}
				}
			}
			if (CPU_IP == 0x0191) {
				m74_trace_state.allocation_0191_count++;
				if (!m74_trace_state.allocation_post_2730_page_captured) {
					m74_trace_state.allocation_post_2730_page_captured = TRUE;
					m74_trace_capture_service_page(
						m74_trace_state.allocation_post_2730_page);
				}
			}
			if (CPU_IP == 0x020d) {
				m74_trace_state.allocation_020d_count++;
				if (!m74_trace_state.allocation_020d_captured) {
					m74_trace_state.allocation_020d_captured = TRUE;
					m74_trace_state.allocation_020d_ss = CPU_SS;
					m74_trace_state.allocation_020d_sp = CPU_SP;
					m74_trace_state.allocation_020d_ds = CPU_DS;
					m74_trace_state.allocation_020d_si = CPU_SI;
					m74_trace_state.allocation_020d_es = CPU_ES;
					m74_trace_state.allocation_020d_di = CPU_DI;
					m74_trace_state.allocation_020d_far_ip =
						m74_trace_read_word(stack + 4);
					m74_trace_state.allocation_020d_far_cs =
						m74_trace_read_word(stack + 6);
				}
			}
			if (CPU_IP == 0x03fd) m74_trace_state.allocation_writer_03fd_count++;
			if (CPU_IP == 0x1309) m74_trace_state.allocation_writer_1309_count++;
			if (CPU_IP == 0x1350) m74_trace_state.allocation_writer_1350_count++;
			if (CPU_IP == 0x273d) m74_trace_state.allocation_writer_273d_count++;
			if (CPU_IP == 0x2748) m74_trace_state.allocation_writer_2748_count++;
			if (CPU_IP == 0x286d) m74_trace_state.allocation_writer_286d_count++;
			if (CPU_IP == 0x2751) m74_trace_state.allocation_2751_count++;
			if (CPU_IP == 0x0338) m74_trace_state.allocation_0338_count++;
			if (CPU_IP == 0x03c2) m74_trace_state.allocation_03c2_count++;
		}
		if ((CPU_CS == 0xe000) && (CPU_IP == 0x3823))
			m74_trace_state.reach_int97++;
		if ((CPU_CS == 0xe000) && (CPU_IP == 0x391d)) {
			m74_trace_state.reach_391d++;
			if (!m74_trace_state.reach_391d_captured) {
				m74_trace_state.reach_391d_captured = TRUE;
				m74_trace_state.reach_entry_dx = CPU_DX;
				m74_trace_state.reach_entry_si = CPU_SI;
				m74_trace_state.reach_entry_ds = CPU_DS;
				m74_trace_state.reach_entry_ss = CPU_SS;
				m74_trace_state.reach_entry_sp = CPU_SP;
				m74_trace_state.reach_entry_caller_cs = CPU_CS;
				m74_trace_state.reach_entry_caller_ip =
					(uint16_t)(upd9002_memoryread(
						(SS_BASE + CPU_SP) & CPU_ADRSMASK) |
					(upd9002_memoryread(
						((SS_BASE + CPU_SP) + 1) & CPU_ADRSMASK) << 8));
			}
		}
		if ((CPU_CS == 0xe000) && (CPU_IP == 0x3983)) {
			m74_trace_state.reach_3983++;
			if (!m74_trace_state.reach_3983_captured) {
				m74_trace_state.reach_3983_captured = TRUE;
				m74_trace_state.reach_exit_ss = CPU_SS;
				m74_trace_state.reach_exit_sp = CPU_SP;
				m74_trace_state.reach_exit_word0 =
					(uint16_t)(upd9002_memoryread(
						(SS_BASE + CPU_SP) & CPU_ADRSMASK) |
					(upd9002_memoryread(
						((SS_BASE + CPU_SP) + 1) & CPU_ADRSMASK) << 8));
				m74_trace_state.reach_exit_word1 =
					(uint16_t)(upd9002_memoryread(
						((SS_BASE + CPU_SP) + 2) & CPU_ADRSMASK) |
					(upd9002_memoryread(
						((SS_BASE + CPU_SP) + 3) & CPU_ADRSMASK) << 8));
				m74_trace_state.reach_exit_word2 =
					(uint16_t)(upd9002_memoryread(
						((SS_BASE + CPU_SP) + 4) & CPU_ADRSMASK) |
					(upd9002_memoryread(
						((SS_BASE + CPU_SP) + 5) & CPU_ADRSMASK) << 8));
			}
		}
		if ((CPU_CS == 0xe000) && (CPU_IP == 0x3985)) m74_trace_state.reach_3985++;
		if ((CPU_CS == 0xe000) && (CPU_IP == 0x3988)) {
			m74_trace_state.reach_3988++;
			if (!m74_trace_state.reach_3988_captured) {
				uint32_t physical = (SS_BASE + CPU_SP) & CPU_ADRSMASK;
				m74_trace_state.reach_3988_captured = TRUE;
				m74_trace_state.reach_3988_ss = CPU_SS;
				m74_trace_state.reach_3988_sp = CPU_SP;
				m74_trace_state.reach_3988_word0 = (uint16_t)(upd9002_memoryread(physical) | (upd9002_memoryread(physical + 1) << 8));
				m74_trace_state.reach_3988_word1 = (uint16_t)(upd9002_memoryread(physical + 2) | (upd9002_memoryread(physical + 3) << 8));
				m74_trace_state.reach_3988_word2 = (uint16_t)(upd9002_memoryread(physical + 4) | (upd9002_memoryread(physical + 5) << 8));
			}
		}
		if ((CPU_CS == 0xe000) && (CPU_IP == 0x002a)) m74_trace_state.reach_002a++;
		if ((CPU_CS == 0xe000) && (CPU_IP == 0x01e4)) {
			m74_trace_state.reach_01e4++;
			if (!m74_trace_state.reach_01e4_captured) {
				uint32_t physical = (SS_BASE + CPU_SP) & CPU_ADRSMASK;
				m74_trace_state.reach_01e4_captured = TRUE;
				m74_trace_state.reach_retf_ss = CPU_SS;
				m74_trace_state.reach_retf_sp = CPU_SP;
				m74_trace_state.reach_retf_memmode_va = memmode_va;
				m74_trace_state.reach_retf_ip = (uint16_t)(upd9002_memoryread(physical) | (upd9002_memoryread(physical + 1) << 8));
				m74_trace_state.reach_retf_cs = (uint16_t)(upd9002_memoryread(physical + 2) | (upd9002_memoryread(physical + 3) << 8));
				for (uint32_t index = 0; index < sizeof(m74_trace_state.reach_retf_bytes); index++)
					m74_trace_state.reach_retf_bytes[index] = (uint8_t)upd9002_memoryread((((uint32_t)m74_trace_state.reach_retf_cs << 4) + m74_trace_state.reach_retf_ip + index) & CPU_ADRSMASK);
				for (uint32_t index = 0; index < sizeof(m74_trace_state.reach_34c0_base); index++)
					m74_trace_state.reach_34c0_base[index] = (uint8_t)upd9002_memoryread(0x34c00 + index);
				for (uint32_t index = 0; index < 8; index++)
					m74_trace_state.reach_stack_words[index] = (uint16_t)(upd9002_memoryread((physical + index * 2) & CPU_ADRSMASK) | (upd9002_memoryread((physical + index * 2 + 1) & CPU_ADRSMASK) << 8));
				const uint16_t window_segments[4] = {0x43b5, 0x49fc, 0x75ab, 0x7f2d};
				for (uint32_t window = 0; window < 4; window++)
					for (uint32_t index = 0; index < 256; index++)
						m74_trace_state.reach_windows[window][index] = (uint8_t)upd9002_memoryread((((uint32_t)window_segments[window] << 4) + index) & CPU_ADRSMASK);
				if (m74_trace_state.allocation_capture) {
					m74_trace_state.allocation_ds = CPU_DS;
					m74_trace_state.allocation_pre_01e4_page_captured = TRUE;
					m74_trace_capture_service_page(
						m74_trace_state.allocation_pre_01e4_page);
					m74_trace_read_bytes(DS_BASE & CPU_ADRSMASK,
						m74_trace_state.allocation_work_area,
						sizeof(m74_trace_state.allocation_work_area));
					for (uint32_t range = 0;
						range < UPD9002_M74_ALLOCATION_BAND_COUNT; range++) {
						m74_trace_read_bytes(m74_trace_allocation_bases[range],
							m74_trace_state.allocation_band[range],
							sizeof(m74_trace_state.allocation_band[range]));
					}
				}
			}
		}
	}

	if (m74_trace_state.configured &&
		m74_trace_state.lifecycle_memory_watch &&
		!m74_trace_state.lifecycle_memory_initialized) {
		for (uint32_t index = 0; index < sizeof(m74_trace_state.lifecycle_target);
			index++) {
			m74_trace_state.lifecycle_target[index] =
				(uint8_t)upd9002_memoryread(0x34c00 + index);
		}
		for (uint32_t index = 0; index < sizeof(m74_trace_state.lifecycle_source);
			index++) {
			m74_trace_state.lifecycle_source[index] =
				(uint8_t)upd9002_memoryread(0x39960 + index);
		}
		m74_trace_state.lifecycle_memory_initialized = TRUE;
	}

	m74_trace_vector_call();
	if (m74_trace_state.active && (CPU_CS == 0xe000) &&
		(CPU_IP == 0x383a)) {
		m74_trace_state.cf_probe_active = TRUE;
	}
	if (m74_trace_state.active && (CPU_CS == 0xe000) &&
		(CPU_IP == 0x391d)) {
		uint8_t stack[8];
		uint32_t index;

		upd9002_m74_trace_lifecycle("before-391d");
		m74_trace_stack(stack, (SS_BASE + CPU_SP) & CPU_ADRSMASK);
		m74_trace_state.thunk_active = TRUE;
		m74_trace_state.thunk_entry_sequence = m74_trace_state.steps;
		m74_trace_state.thunk_caller_cs =
			m74_trace_state.have_previous_record ?
			m74_trace_state.previous_record.cs : 0;
		m74_trace_state.thunk_caller_ip =
			m74_trace_state.have_previous_record ?
			m74_trace_state.previous_record.ip : 0;
		m74_trace_state.thunk_return_ip =
			(uint16_t)(stack[0] | ((uint16_t)stack[1] << 8));
		m74_trace_state.thunk_dx = CPU_DX;
		m74_trace_state.thunk_si = CPU_SI;
		m74_trace_state.thunk_sp = CPU_SP;
		m74_trace_state.thunk_stack_physical =
			(SS_BASE + CPU_SP) & CPU_ADRSMASK;
		for (index = 0; index < sizeof(stack); index++) {
			m74_trace_state.thunk_stack[index] = stack[index];
		}
		fprintf(m74_trace_state.stream,
			"m74-thunk-entry seq=%u caller=%04x:%04x return_ip=%04x "
			"dx=%04x si=%04x ss=%04x sp=%04x stack_phys=%05x stack=",
			m74_trace_state.thunk_entry_sequence,
			m74_trace_state.thunk_caller_cs,
			m74_trace_state.thunk_caller_ip,
			m74_trace_state.thunk_return_ip,
			m74_trace_state.thunk_dx, m74_trace_state.thunk_si,
			CPU_SS, m74_trace_state.thunk_sp,
			m74_trace_state.thunk_stack_physical);
		for (index = 0; index < sizeof(stack); index++) {
			fprintf(m74_trace_state.stream, "%02x", stack[index]);
		}
		fputc('\n', m74_trace_state.stream);
	}
	if (m74_trace_state.active && (CPU_CS == 0xe000) &&
		(CPU_IP == 0x01e4)) {
		upd9002_m74_trace_lifecycle("before-01e4");
	}
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

static void m74_trace_lifecycle_memory_watch(void) {
	uint32_t lifecycle_step = m74_trace_state.lifecycle_memory_step++;

	if (m74_trace_state.configured &&
		m74_trace_state.lifecycle_memory_watch &&
		m74_trace_state.lifecycle_memory_initialized) {
		const uint32_t bases[] = {0x34c00, 0x39960};
		uint8_t *previous[] = {m74_trace_state.lifecycle_target,
			m74_trace_state.lifecycle_source};
		const uint32_t lengths[] = {sizeof(m74_trace_state.lifecycle_target),
			sizeof(m74_trace_state.lifecycle_source)};
		int32_t before_clock = CPU_REMCLOCK;

		for (uint32_t range = 0; range < 2; range++) {
			uint32_t index = 0;
			while (index < lengths[range]) {
				uint8_t current = (uint8_t)upd9002_memoryread(
					 bases[range] + index);
				if (current == previous[range][index]) {
					index++;
					continue;
				}
				uint32_t start = index;
				while (index < lengths[range]) {
					current = (uint8_t)upd9002_memoryread(
						bases[range] + index);
					if (current == previous[range][index]) {
						break;
					}
					index++;
				}
				fprintf(m74_trace_state.stream,
					"m74-step-memory-change sequence=%u cs=%04x ip=%04x "
					"physical=%05x length=%u old=",
					lifecycle_step, CPU_CS, CPU_IP,
					bases[range] + start, index - start);
				for (uint32_t position = start; position < index; position++) {
					fprintf(m74_trace_state.stream, "%02x",
						previous[range][position]);
				}
				fputs(" new=", m74_trace_state.stream);
				for (uint32_t position = start; position < index; position++) {
					fprintf(m74_trace_state.stream, "%02x",
						(uint8_t)upd9002_memoryread(
							bases[range] + position));
				}
				fputc('\n', m74_trace_state.stream);
			}
			for (uint32_t position = 0; position < lengths[range]; position++) {
				previous[range][position] = (uint8_t)upd9002_memoryread(
					bases[range] + position);
			}
		}
		CPU_REMCLOCK = before_clock;
	}
}

void upd9002_m74_trace_step_end(void) {
	m74_trace_lifecycle_memory_watch();
	m74_trace_vector_commit();

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

	if (m74_trace_state.cf_probe_active) {
		fprintf(m74_trace_state.stream,
			"m74-cf-probe seq=%u cs=%04x ip=%04x bytes="
			"%02x%02x%02x%02x%02x%02x%02x%02x ax=%04x bx=%04x "
			"cx=%04x dx=%04x si=%04x di=%04x sp=%04x flags=%04x "
			"post_cs=%04x post_ip=%04x post_sp=%04x post_flags=%04x\n",
			record->sequence, record->cs, record->ip,
			record->bytes[0], record->bytes[1], record->bytes[2],
			record->bytes[3], record->bytes[4], record->bytes[5],
			record->bytes[6], record->bytes[7], record->ax, record->bx,
			record->cx, record->dx, record->si, record->di, record->sp,
			record->flags, record->post_cs, record->post_ip,
			record->post_sp, record->post_flags);
		if ((record->cs == 0xe000) && (record->ip == 0x3861)) {
			m74_trace_state.cf_probe_active = FALSE;
		}
	}
	if ((record->cs == 0xe000) && (record->ip == 0x397a)) {
		fprintf(m74_trace_state.stream,
			"m74-cf-decision seq=%u flags=%04x cf=%u post=%04x:%04x\n",
			record->sequence, record->flags,
			(record->flags & C_FLAG) ? 1U : 0U,
			record->post_cs, record->post_ip);
	}

	if (m74_trace_state.thunk_active && (record->cs == 0xe000) &&
		((record->ip == 0x3922) || (record->ip == 0x3923) ||
		 (record->ip == 0x3973) || (record->ip == 0x3979) ||
		 (record->ip == 0x3983))) {
		fprintf(m74_trace_state.stream,
			"m74-thunk-stack-step seq=%u ip=%04x sp=%04x post_sp=%04x "
			"post=%04x:%04x stack_before=", record->sequence,
			record->ip, record->sp, record->post_sp,
			record->post_cs, record->post_ip);
		for (uint32_t byte = 0; byte < sizeof(record->stack_before); byte++) {
			fprintf(m74_trace_state.stream, "%02x", record->stack_before[byte]);
		}
		fputs(" stack_after=", m74_trace_state.stream);
		for (uint32_t byte = 0; byte < sizeof(record->stack_after); byte++) {
			fprintf(m74_trace_state.stream, "%02x", record->stack_after[byte]);
		}
		fputc('\n', m74_trace_state.stream);
	}

	if (m74_trace_state.thunk_active && (record->cs == 0xe000) &&
		((record->ip == 0x3983) || (record->ip == 0x3988))) {
		fprintf(m74_trace_state.stream,
			"m74-thunk-helper-return seq=%u path=%s post=%04x:%04x "
			"post_sp=%04x flags=%04x\n", record->sequence,
			record->ip == 0x3983 ? "success" : "failure",
			record->post_cs, record->post_ip, record->post_sp,
			record->post_flags);
		if (record->ip == 0x3988) {
			m74_trace_state.thunk_active = FALSE;
		}
	}
	if ((record->post_cs == 0x34c0) && (record->post_ip == 0x0005)) {
		upd9002_m74_trace_lifecycle("after-34c0-0005");
	}
	if (m74_trace_state.thunk_active && (record->cs == 0xe000) &&
		(record->ip == 0x01e4)) {
		uint8_t target_bytes[16];
		uint32_t target_phys;
		uint32_t byte;
		int32_t before_clock;

		target_phys = (record->post_cs_base + record->post_ip) &
			CPU_ADRSMASK;
		before_clock = CPU_REMCLOCK;
		for (byte = 0; byte < sizeof(target_bytes); byte++) {
			target_bytes[byte] =
				(uint8_t)upd9002_memoryread(target_phys + byte);
		}
		CPU_REMCLOCK = before_clock;
		fprintf(m74_trace_state.stream,
			"m74-thunk-retf seq=%u entry_seq=%u from=%04x:%04x "
			"to=%04x:%04x target_phys=%05x entry_caller=%04x:%04x "
			"return_ip=%04x dx=%04x si=%04x entry_sp=%04x "
			"entry_stack_phys=%05x stack_before=",
			record->sequence, m74_trace_state.thunk_entry_sequence,
			record->cs, record->ip, record->post_cs, record->post_ip,
			target_phys, m74_trace_state.thunk_caller_cs,
			m74_trace_state.thunk_caller_ip,
			m74_trace_state.thunk_return_ip, m74_trace_state.thunk_dx,
			m74_trace_state.thunk_si, m74_trace_state.thunk_sp,
			m74_trace_state.thunk_stack_physical);
		for (byte = 0; byte < sizeof(record->stack_before); byte++) {
			fprintf(m74_trace_state.stream, "%02x", record->stack_before[byte]);
		}
		fprintf(m74_trace_state.stream, " target_bytes=");
		for (byte = 0; byte < sizeof(target_bytes); byte++) {
			fprintf(m74_trace_state.stream, "%02x", target_bytes[byte]);
		}
		fputc('\n', m74_trace_state.stream);
		m74_trace_state.thunk_active = FALSE;
	}
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
	uint16_t old_value;
	int32_t before_clock;

	m74_trace_vector_write_prepare(address, value, width);
	address &= CPU_ADRSMASK;
	end = address + width;
	if (m74_trace_state.configured && m74_trace_state.lifecycle_memory_watch &&
		(((end > 0x34c00) && (address < 0x34d00)) ||
		 ((end > 0x39960) && (address < 0x39980)))) {
		before_clock = CPU_REMCLOCK;
		old_value = (width == 1) ?
			(uint16_t)upd9002_memoryread(address) :
			(uint16_t)upd9002_memoryread_w(address);
		CPU_REMCLOCK = before_clock;
		fprintf(m74_trace_state.stream,
			"m74-lifecycle-write cs=%04x ip=%04x physical=%05x "
			"old=%04x new=%04x width=%u\n", CPU_CS, CPU_IP,
			address, old_value, value, width);
	}

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


void upd9002_m74_trace_host_write(uint32_t address, const void *data,
		uint32_t length, const char *kind) {
	const uint8_t *bytes;
	uint32_t end;
	uint32_t overlap_start;
	uint32_t overlap_end;
	uint32_t offset;
	uint32_t count;
	int32_t before_clock;

	if (!m74_trace_state.configured ||
		(m74_trace_state.stream == NULL) || (data == NULL) ||
		(length == 0)) {
		return;
	}
	end = address + length;
	if ((end < address) || (address >= CPU_ADRSMASK + 1U)) {
		return;
	}
	if (end > (CPU_ADRSMASK + 1U)) {
		end = CPU_ADRSMASK + 1U;
	}
	bytes = (const uint8_t *)data;
	if (m74_trace_state.vector_watch && (length >= 5)) {
		uint32_t available;

		available = end - address;
		for (uint32_t index = 0; index + 5 <= available; index++) {
			if (bytes[index] != 0xea) {
				continue;
			}
			fprintf(m74_trace_state.stream,
				"m74-ea-sequence-host-write kind=%s physical=%05x "
				"bytes=%02x%02x%02x%02x%02x target=%04x:%04x "
				"writer=%04x:%04x\n",
				(kind != NULL) ? kind : "unknown", address + index,
				bytes[index], bytes[index + 1], bytes[index + 2],
				bytes[index + 3], bytes[index + 4],
				(uint16_t)(bytes[index + 3] |
					((uint16_t)bytes[index + 4] << 8)),
				(uint16_t)(bytes[index + 1] |
					((uint16_t)bytes[index + 2] << 8)),
				CPU_CS, CPU_IP);
		}
	}
	overlap_start = max(address, 0x34c00U);
	overlap_end = min(end, 0x34d00U);
	if (overlap_start >= overlap_end) {
		overlap_start = max(address, 0x39960U);
		overlap_end = min(end, 0x39980U);
	}
	if (overlap_start >= overlap_end) {
		return;
	}
	offset = overlap_start - address;
	count = overlap_end - overlap_start;
	if (count > 32) {
		count = 32;
	}
	before_clock = CPU_REMCLOCK;
	fprintf(m74_trace_state.stream,
		"m74-host-write kind=%s cs=%04x ip=%04x physical=%05x "
		"length=%u overlap=%05x-%05x old=",
		(kind != NULL) ? kind : "unknown", CPU_CS, CPU_IP, address,
		length, overlap_start, overlap_end - 1);
	for (uint32_t index = 0; index < count; index++) {
		fprintf(m74_trace_state.stream, "%02x",
			(uint8_t)upd9002_memoryread(overlap_start + index));
	}
	fputs(" new=", m74_trace_state.stream);
	for (uint32_t index = 0; index < count; index++) {
		fprintf(m74_trace_state.stream, "%02x", bytes[offset + index]);
	}
	fputc('\n', m74_trace_state.stream);
	CPU_REMCLOCK = before_clock;
	fflush(m74_trace_state.stream);
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

static uint64_t m74_trace_fnv1a_update(uint64_t hash, uint8_t value) {

	hash ^= value;
	return hash * UINT64_C(1099511628211);
}

static void m74_trace_lifecycle_snapshot(const char *label) {
	uint64_t hash;
	uint32_t offset;
	uint32_t nonzero_bytes;
	uint32_t range_count;
	uint32_t range_start[1024];
	uint32_t range_end[1024];
	uint32_t current_start;
	uint8_t prefix[64];
	uint8_t target_window[16];
	BOOL in_range;
	BOOL range_overflow;
	int32_t before_clock;
	uint8_t value;

	if (!m74_trace_state.configured ||
		m74_trace_state.stream == NULL) {
		return;
	}
	before_clock = CPU_REMCLOCK;
	hash = UINT64_C(14695981039346656037);
	nonzero_bytes = 0;
	range_count = 0;
	in_range = FALSE;
	range_overflow = FALSE;
	current_start = 0;
	for (offset = 0; offset < 0x10000; offset++) {
		value = (uint8_t)upd9002_memoryread(0x34c00 + offset);
		hash = m74_trace_fnv1a_update(hash, value);
		if (offset < sizeof(prefix)) {
			prefix[offset] = value;
		}
		if ((offset >= 0x4d60) && (offset < 0x4d70)) {
			target_window[offset - 0x4d60] = value;
		}
		if (value != 0) {
			nonzero_bytes++;
			if (!in_range) {
				current_start = offset;
				in_range = TRUE;
			}
		}
		else if (in_range) {
			if (range_count < 1024) {
				range_start[range_count] = current_start;
				range_end[range_count] = offset - 1;
				range_count++;
			}
			else {
				range_overflow = TRUE;
			}
			in_range = FALSE;
		}
	}
	if (in_range) {
		if (range_count < 1024) {
			range_start[range_count] = current_start;
			range_end[range_count] = 0xffff;
			range_count++;
		}
		else {
			range_overflow = TRUE;
		}
	}
	CPU_REMCLOCK = before_clock;
	fprintf(m74_trace_state.stream,
		"m74-lifecycle label=%s sequence=%u cs=%04x ip=%04x ss=%04x "
		"sp=%04x base=34c00 size=10000 fnv1a=%016llx nonzero_bytes=%u "
		"prefix=",
		(label != NULL) ? label : "unknown", m74_trace_state.lifecycle_sequence++,
		CPU_CS, CPU_IP, CPU_SS, CPU_SP, (unsigned long long)hash,
		nonzero_bytes);
	for (offset = 0; offset < sizeof(prefix); offset++) {
		fprintf(m74_trace_state.stream, "%02x", prefix[offset]);
	}
	fprintf(m74_trace_state.stream, " target_4d60=");
	for (offset = 0; offset < sizeof(target_window); offset++) {
		fprintf(m74_trace_state.stream, "%02x", target_window[offset]);
	}
	fprintf(m74_trace_state.stream, " nonzero_ranges=");
	for (offset = 0; offset < range_count; offset++) {
		if (offset != 0) {
			fputc(',', m74_trace_state.stream);
		}
		fprintf(m74_trace_state.stream, "%04x-%04x",
			range_start[offset], range_end[offset]);
	}
	if (range_overflow) {
		fputs(",OVERFLOW", m74_trace_state.stream);
	}
	fputc('\n', m74_trace_state.stream);
	fflush(m74_trace_state.stream);
}

void upd9002_m74_trace_lifecycle(const char *label) {

	if (m74_trace_state.configured && m74_trace_state.reset_arm &&
		(label != NULL)) {
		if (!m74_trace_state.reset_first_ok_captured &&
			(strcmp(label, "headless-prompt-observed") == 0)) {
			m74_trace_state.reset_first_ok_captured = TRUE;
			m74_trace_state.reset_first_ok_0180 =
				m74_trace_state.reset_0180_count;
			m74_trace_state.reset_first_ok_2730 =
				m74_trace_state.reset_2730_count;
			m74_trace_state.reset_first_ok_2751 =
				m74_trace_state.reset_2751_count;
			m74_trace_state.reset_first_ok_0191 =
				m74_trace_state.reset_0191_count;
			m74_trace_state.reset_first_ok_01e4 =
				m74_trace_state.reset_01e4_count;
			memcpy(m74_trace_state.reset_first_ok_scratch_writer_counts,
				m74_trace_state.reset_scratch_writer_counts,
				sizeof(m74_trace_state.reset_scratch_writer_counts));
		}
		if (!m74_trace_state.reset_injection_captured &&
			(strncmp(label, "headless-before-command-", 24) == 0)) {
			char expected[48];

			snprintf(expected, sizeof(expected), "headless-before-command-%u",
				m74_trace_state.arm_command);
			if (strcmp(label, expected) == 0) {
				m74_trace_state.reset_injection_captured = TRUE;
				m74_trace_state.reset_injection_0180 =
					m74_trace_state.reset_0180_count;
				m74_trace_state.reset_injection_2730 =
					m74_trace_state.reset_2730_count;
				m74_trace_state.reset_injection_2751 =
					m74_trace_state.reset_2751_count;
				m74_trace_state.reset_injection_0191 =
					m74_trace_state.reset_0191_count;
				m74_trace_state.reset_injection_01e4 =
					m74_trace_state.reset_01e4_count;
				memcpy(m74_trace_state.reset_injection_scratch_writer_counts,
					m74_trace_state.reset_scratch_writer_counts,
					sizeof(m74_trace_state.reset_scratch_writer_counts));
			}
		}
	}

	if (m74_trace_state.configured && m74_trace_state.allocation_capture &&
		(label != NULL)) {
		if (!m74_trace_state.allocation_reset_page_captured &&
			(strcmp(label, "reset") == 0)) {
			m74_trace_state.allocation_reset_page_captured = TRUE;
			m74_trace_capture_service_page(
				m74_trace_state.allocation_reset_page);
		}
		if (!m74_trace_state.allocation_first_ok_page_captured &&
			(strcmp(label, "headless-prompt-observed") == 0)) {
			m74_trace_state.allocation_first_ok_page_captured = TRUE;
			m74_trace_capture_service_page(
				m74_trace_state.allocation_first_ok_page);
		}
	}
	if (!m74_trace_state.lifecycle_memory_watch &&
		!m74_trace_state.vector_watch &&
		!m74_trace_state.allocation_capture) {
		return;
	}
	if (m74_trace_state.lifecycle_memory_watch) {
		m74_trace_lifecycle_snapshot(label);
	}
	if (m74_trace_state.vector_watch) {
		m74_trace_vector_snapshot(label);
	}
	if (m74_trace_state.configured && (m74_trace_state.stream != NULL)) {
		fprintf(m74_trace_state.stream,
			"m74-lifecycle-checkpoint label=%s active=%u trace_steps=%u "
			"cs=%04x ip=%04x\n",
			(label != NULL) ? label : "unknown",
			m74_trace_state.active ? 1U : 0U, m74_trace_state.steps,
			CPU_CS, CPU_IP);
		fflush(m74_trace_state.stream);
	}
}
