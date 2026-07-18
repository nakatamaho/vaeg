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
#include "upd9002_state.h"

#include <stddef.h>
#include <string.h>

#define STATE_ALIGNOF(type) offsetof(struct { char byte; type value; }, value)
#define STATE_ASSERT(name, condition) \
	typedef char state_assert_##name[(condition) ? 1 : -1]

STATE_ASSERT(compat_size,
	sizeof(Cpu286StateCompat) == UPD9002_CPU286_PAYLOAD_SIZE);
STATE_ASSERT(compat_align, STATE_ALIGNOF(Cpu286StateCompat) == 4);
STATE_ASSERT(compat_r, offsetof(Cpu286StateCompat, r) == 0);
STATE_ASSERT(compat_es_base, offsetof(Cpu286StateCompat, es_base) == 28);
STATE_ASSERT(compat_cs_base, offsetof(Cpu286StateCompat, cs_base) == 32);
STATE_ASSERT(compat_ss_base, offsetof(Cpu286StateCompat, ss_base) == 36);
STATE_ASSERT(compat_ds_base, offsetof(Cpu286StateCompat, ds_base) == 40);
STATE_ASSERT(compat_ss_fix, offsetof(Cpu286StateCompat, ss_fix) == 44);
STATE_ASSERT(compat_ds_fix, offsetof(Cpu286StateCompat, ds_fix) == 48);
STATE_ASSERT(compat_adrsmask, offsetof(Cpu286StateCompat, adrsmask) == 52);
STATE_ASSERT(compat_prefix, offsetof(Cpu286StateCompat, prefix) == 56);
STATE_ASSERT(compat_trap, offsetof(Cpu286StateCompat, trap) == 58);
STATE_ASSERT(compat_resetreq, offsetof(Cpu286StateCompat, resetreq) == 59);
STATE_ASSERT(compat_ovflag, offsetof(Cpu286StateCompat, ovflag) == 60);
STATE_ASSERT(compat_gdtr, offsetof(Cpu286StateCompat, GDTR) == 64);
STATE_ASSERT(compat_msw, offsetof(Cpu286StateCompat, MSW) == 70);
STATE_ASSERT(compat_idtr, offsetof(Cpu286StateCompat, IDTR) == 72);
STATE_ASSERT(compat_ldtr, offsetof(Cpu286StateCompat, LDTR) == 78);
STATE_ASSERT(compat_ldtrc, offsetof(Cpu286StateCompat, LDTRC) == 80);
STATE_ASSERT(compat_tr, offsetof(Cpu286StateCompat, TR) == 86);
STATE_ASSERT(compat_trc, offsetof(Cpu286StateCompat, TRC) == 88);
STATE_ASSERT(compat_padding, offsetof(Cpu286StateCompat, padding) == 94);
STATE_ASSERT(compat_cpu_type, offsetof(Cpu286StateCompat, cpu_type) == 96);
STATE_ASSERT(compat_itfbank, offsetof(Cpu286StateCompat, itfbank) == 97);
STATE_ASSERT(compat_ram_d0, offsetof(Cpu286StateCompat, ram_d0) == 98);
STATE_ASSERT(compat_remainclock,
	offsetof(Cpu286StateCompat, remainclock) == 100);
STATE_ASSERT(compat_baseclock,
	offsetof(Cpu286StateCompat, baseclock) == 104);
STATE_ASSERT(compat_clock, offsetof(Cpu286StateCompat, clock) == 108);
STATE_ASSERT(runtime_size,
	sizeof(Upd9002RuntimeState) == sizeof(Cpu286StateCompat));
STATE_ASSERT(runtime_align, STATE_ALIGNOF(Upd9002RuntimeState) == 4);
STATE_ASSERT(runtime_padding,
	offsetof(Upd9002RuntimeState, padding) ==
	offsetof(Cpu286StateCompat, padding));
STATE_ASSERT(runtime_cpu_type,
	offsetof(Upd9002RuntimeState, cpu_type) ==
	offsetof(Cpu286StateCompat, cpu_type));
STATE_ASSERT(image_size,
	sizeof(Cpu286CompatImage) == sizeof(Cpu286StateCompat));

static Cpu286CompatImage cpu286_compat_image;

static void state_set_error(char *error, size_t error_size,
									const char *message) {

	size_t length;

	if ((error == NULL) || (error_size == 0)) {
		return;
	}
	length = strlen(message);
	if (length >= error_size) {
		length = error_size - 1;
	}
	memcpy(error, message, length);
	error[length] = '\0';
}

static void state_overlay_runtime(Cpu286StateCompat *state,
									const Upd9002RuntimeState *runtime) {

	const size_t padding = offsetof(Cpu286StateCompat, padding);
	const size_t tail = offsetof(Cpu286StateCompat, cpu_type);

	memcpy(state, runtime, padding);
	memcpy((UINT8 *)state + tail, (const UINT8 *)runtime + tail,
		sizeof(*state) - tail);
}

static void state_construct_runtime(Upd9002RuntimeState *runtime,
									const Cpu286StateCompat *state) {

	const size_t padding = offsetof(Cpu286StateCompat, padding);
	const size_t tail = offsetof(Cpu286StateCompat, cpu_type);

	memset(runtime, 0, sizeof(*runtime));
	memcpy(runtime, state, padding);
	memcpy((UINT8 *)runtime + tail, (const UINT8 *)state + tail,
		sizeof(*runtime) - tail);
}

void upd9002_state_initialize(void) {

	memset(&cpu286_compat_image, 0, sizeof(cpu286_compat_image));
}

void upd9002_state_reset(void) {

	memset(&cpu286_compat_image, 0, sizeof(cpu286_compat_image));
}

void upd9002_state_shut(void) {

	memset(&cpu286_compat_image, 0,
		offsetof(Cpu286StateCompat, cpu_type));
}

int upd9002_state_validate(const void *payload, size_t size,
										char *error, size_t error_size) {

	Cpu286StateCompat state;

	if ((payload == NULL) || (size != sizeof(state))) {
		state_set_error(error, error_size, UPD9002_STATE_ERROR_SIZE);
		return FAILURE;
	}
	memcpy(&state, payload, sizeof(state));
	if (state.cpu_type != CPUTYPE_V30) {
		state_set_error(error, error_size, UPD9002_STATE_ERROR_CPU_TYPE);
		return FAILURE;
	}
	if (state.MSW & MSW_PE) {
		state_set_error(error, error_size,
			UPD9002_STATE_ERROR_PROTECTED_MODE);
		return FAILURE;
	}
	if ((error != NULL) && (error_size != 0)) {
		error[0] = '\0';
	}
	return SUCCESS;
}

int upd9002_state_import(const void *payload, size_t size,
									char *error, size_t error_size) {

	Cpu286CompatImage next_image;
	Cpu286StateCompat state;
	Upd9002RuntimeState next_runtime;

	if (upd9002_state_validate(payload, size, error, error_size) != SUCCESS) {
		return FAILURE;
	}
	memcpy(&state, payload, sizeof(state));
	memcpy(next_image.bytes, payload, sizeof(next_image.bytes));
	state_construct_runtime(&next_runtime, &state);

	/* No CPU callback can run while statsave commits these two objects. */
	cpu286_compat_image = next_image;
	i286core.s = next_runtime;
	return SUCCESS;
}

void upd9002_state_export(Cpu286StateCompat *state) {

	if (state == NULL) {
		return;
	}
	memcpy(state, cpu286_compat_image.bytes, sizeof(*state));
	state_overlay_runtime(state, &i286core.s);
}
