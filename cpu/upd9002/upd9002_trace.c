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
