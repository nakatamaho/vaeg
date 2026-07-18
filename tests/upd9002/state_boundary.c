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
#include "upd9002_state.h"

#include <stdio.h>
#include <string.h>

I286CORE i286core;

static int fail(const char *message) {

	fprintf(stderr, "upd9002-state-boundary: %s\n", message);
	return 1;
}

static void make_noncanonical(Cpu286StateCompat *state, UINT8 padding0,
												UINT8 padding1) {

	memset(state, 0, sizeof(*state));
	state->r.w.ax = 0x1234;
	state->r.w.bx = 0x5678;
	state->r.w.cs = 0x2000;
	state->r.w.ip = 0x0100;
	state->r.w.flag = 0xf246;
	state->cs_base = 0x00020000;
	state->ss_fix = 0x00034560;
	state->ds_fix = 0x00045670;
	state->adrsmask = 0x000fffff;
	state->GDTR.limit = 0xabcd;
	state->GDTR.base = 0x1357;
	state->GDTR.base24 = 0x24;
	state->GDTR.reserved = 0x68;
	state->TRC.base24 = 0x42;
	state->TRC.reserved = 0x86;
	state->padding[0] = padding0;
	state->padding[1] = padding1;
	state->cpu_type = CPUTYPE_V30;
	state->itfbank = 1;
	state->ram_d0 = 0xa55a;
	state->remainclock = -17;
	state->baseclock = 12345;
	state->clock = 0x89abcdef;
}

static int test_import_export(void) {

	Cpu286StateCompat alternate;
	Cpu286StateCompat exported;
	Cpu286StateCompat imported;
	Upd9002RuntimeState first_runtime;

	make_noncanonical(&imported, 0xa5, 0x5a);
	if (upd9002_state_import(&imported, sizeof(imported), NULL, 0) != SUCCESS) {
		return fail("valid import failed");
	}
	upd9002_state_export(&exported);
	if (memcmp(&imported, &exported, sizeof(imported))) {
		return fail("immediate opaque-byte round trip changed");
	}
	if ((i286core.s.padding[0] != 0) || (i286core.s.padding[1] != 0)) {
		return fail("opaque padding entered runtime state");
	}
	first_runtime = i286core.s;

	make_noncanonical(&alternate, 0x12, 0x34);
	if (upd9002_state_import(&alternate, sizeof(alternate), NULL, 0) !=
																SUCCESS) {
		return fail("alternate valid import failed");
	}
	if (memcmp(&first_runtime, &i286core.s, sizeof(first_runtime))) {
		return fail("opaque padding influenced runtime state");
	}
	upd9002_state_export(&exported);
	if (memcmp(&alternate, &exported, sizeof(alternate))) {
		return fail("alternate opaque bytes were not retained");
	}

	i286core.s.r.w.ax = 0xbeef;
	upd9002_state_export(&exported);
	if ((exported.r.w.ax != 0xbeef) ||
		(exported.padding[0] != 0x12) || (exported.padding[1] != 0x34)) {
		return fail("runtime overlay did not preserve opaque padding");
	}
	return 0;
}

static int test_rejected_imports(void) {

	Cpu286StateCompat before_image;
	Cpu286StateCompat invalid;
	Cpu286StateCompat after_image;
	Upd9002RuntimeState before_runtime;
	char error[128];

	make_noncanonical(&invalid, 0xa5, 0x5a);
	if (upd9002_state_import(&invalid, sizeof(invalid), NULL, 0) != SUCCESS) {
		return fail("rejection setup import failed");
	}
	before_runtime = i286core.s;
	upd9002_state_export(&before_image);

	invalid.cpu_type = 0;
	error[0] = '\0';
	if ((upd9002_state_import(&invalid, sizeof(invalid), error,
													sizeof(error)) != FAILURE) ||
		strcmp(error, UPD9002_STATE_ERROR_CPU_TYPE)) {
		return fail("non-V30 cpu_type was not rejected deterministically");
	}
	upd9002_state_export(&after_image);
	if (memcmp(&before_runtime, &i286core.s, sizeof(before_runtime)) ||
		memcmp(&before_image, &after_image, sizeof(before_image))) {
		return fail("non-V30 rejection changed live state");
	}

	error[0] = '\0';
	if ((upd9002_state_import(&invalid, sizeof(invalid) - 1, error,
													sizeof(error)) != FAILURE) ||
		strcmp(error, UPD9002_STATE_ERROR_SIZE)) {
		return fail("malformed payload size was not rejected deterministically");
	}
	upd9002_state_export(&after_image);
	if (memcmp(&before_runtime, &i286core.s, sizeof(before_runtime)) ||
		memcmp(&before_image, &after_image, sizeof(before_image))) {
		return fail("size rejection changed live state");
	}

	invalid = before_image;
	invalid.MSW |= MSW_PE;
	error[0] = '\0';
	if ((upd9002_state_import(&invalid, sizeof(invalid), error,
												sizeof(error)) != FAILURE) ||
		strcmp(error, UPD9002_STATE_ERROR_PROTECTED_MODE)) {
		return fail("MSW.PE state was not rejected deterministically");
	}
	upd9002_state_export(&after_image);
	if (memcmp(&before_runtime, &i286core.s, sizeof(before_runtime)) ||
		memcmp(&before_image, &after_image, sizeof(before_image))) {
		return fail("MSW.PE rejection changed live state");
	}
	return 0;
}

static int test_reset_and_shut(void) {

	Cpu286StateCompat expected;
	Cpu286StateCompat exported;
	Cpu286StateCompat imported;
	UINT8 cpu_type;

	make_noncanonical(&imported, 0xa5, 0x5a);
	if (upd9002_state_import(&imported, sizeof(imported), NULL, 0) != SUCCESS) {
		return fail("lifecycle setup import failed");
	}
	cpu_type = i286core.s.cpu_type;
	memset(&i286core.s, 0, sizeof(i286core.s));
	i286core.s.cpu_type = cpu_type;
	i286core.s.r.w.cs = 0xf000;
	i286core.s.cs_base = 0x000f0000;
	i286core.s.r.w.ip = 0xfff0;
	i286core.s.adrsmask = 0x000fffff;
	i286core.s.r.w.flag = 0xf002;
	upd9002_state_reset();
	upd9002_state_export(&exported);
	memset(&expected, 0, sizeof(expected));
	expected.cpu_type = CPUTYPE_V30;
	expected.r.w.cs = 0xf000;
	expected.cs_base = 0x000f0000;
	expected.r.w.ip = 0xfff0;
	expected.adrsmask = 0x000fffff;
	expected.r.w.flag = 0xf002;
	if (memcmp(&expected, &exported, sizeof(expected))) {
		return fail("reset did not create the canonical legacy image");
	}

	if (upd9002_state_import(&imported, sizeof(imported), NULL, 0) != SUCCESS) {
		return fail("CPU_SHUT setup import failed");
	}
	expected = imported;
	memset(&expected, 0, offsetof(Cpu286StateCompat, cpu_type));
	expected.r.w.cs = 0xf000;
	expected.cs_base = 0x000f0000;
	expected.r.w.ip = 0xfff0;
	expected.adrsmask = 0x000fffff;
	memset(&i286core.s, 0, offsetof(Upd9002RuntimeState, cpu_type));
	i286core.s.r.w.cs = 0xf000;
	i286core.s.cs_base = 0x000f0000;
	i286core.s.r.w.ip = 0xfff0;
	i286core.s.adrsmask = 0x000fffff;
	upd9002_state_shut();
	upd9002_state_export(&exported);
	if (memcmp(&expected, &exported, sizeof(expected))) {
		return fail("CPU_SHUT legacy byte-range transformation changed");
	}
	if (exported.r.w.flag != 0x0000) {
		return fail("CPU_SHUT FLAGS anomaly was normalized");
	}
	return 0;
}

int main(void) {

	upd9002_state_initialize();
	if (test_import_export() || test_rejected_imports() ||
		test_reset_and_shut()) {
		return 1;
	}
	fprintf(stderr,
		"upd9002-state-boundary: ABI, protected-state transaction, opaque bytes, reset, and CPU_SHUT passed\n");
	return 0;
}
