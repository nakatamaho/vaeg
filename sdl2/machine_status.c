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
#include "machine_status.h"
#include "np2.h"
#include "extrarom.h"
#include "frontpanel.h"
#include "machine/pccore.h"
#include "memoryva.h"
#include "sysportva.h"
#include "sgp.h"

#include <stdarg.h>

BOOL machine_status_n_selected(void) {
	/* This is the live N-ROM selector used by the VA memory mapping, not
	 * BOOTMODE_N (an internal startup route, also used for N80-family ROMs). */
	return memoryva_88_mode && memoryva_n80_exist && (memoryva_88_port31 & 0x04);
}

VAEG_MACHINE_MODE machine_status_mode(void) {
	EXTRAROM_PROFILE profile;
	UINT i, count = 0, selected = 0;

	if (machine_status_n_selected()) {
		if (extrarom_profile(np2cfg.extra_machine, &profile) &&
		    (profile.n_role == EXTRAROM_ROLE_N80_2 || profile.n_role == EXTRAROM_ROLE_N80_2SR)) {
			return VAEG_MODE_N_FAMILY;
		}
		return VAEG_MODE_N;
	}
	/* Prefer a valid one-hot hardware lamp latch. During reset, all three
	 * can be asserted; use the running memory domain and system-mode input. */
	for (i = 0; i < 3; i++) {
		if (frontpanel_raw_mode_lit(i)) {
			count++;
			selected = i;
		}
	}
	if (count == 1) {
		return selected == 0 ? VAEG_MODE_V1 : selected == 1 ? VAEG_MODE_V2 : VAEG_MODE_V3;
	}
	if (memoryva_88_mode) {
		return sysportva.modesw == 0xfffe ? VAEG_MODE_V1 : VAEG_MODE_V2;
	}
	return VAEG_MODE_V3;
}

void machine_status_get(VAEG_MACHINE_STATUS *status) {
	EXTRAROM_PROFILE profile;
	const char *name;
	const char *mode = "V3";
	BOOL high = TRUE;

	if (status == NULL)
		return;
	memset(status, 0, sizeof(*status));
	name = pccore.model_va == PCMODEL_VA1 ? "PC-88VA" : "PC-88VA2";
	if (np2cfg.extra_machine[0]) {
		name = extrarom_machine_name(np2cfg.extra_machine);
		if (name == NULL)
			name = "Unknown";
		if (extrarom_profile(np2cfg.extra_machine, &profile))
			high = profile.high;
	}
	snprintf(status->machine, sizeof(status->machine), "%s", name);
	status->has_sgp = strcmp(name, "PC-88VA") == 0 || strcmp(name, "PC-88VA2") == 0;
	status->compatible = memoryva_88_mode != 0;
	status->mode_id = machine_status_mode();
	switch (status->mode_id) {
	case VAEG_MODE_V1:
		mode = "V1";
		break;
	case VAEG_MODE_V2:
		mode = "V2";
		break;
	case VAEG_MODE_N:
		mode = "N";
		break;
	case VAEG_MODE_N80:
		mode = "N80";
		break;
	case VAEG_MODE_N80SR:
		mode = "N80SR";
		break;
	case VAEG_MODE_N_FAMILY:
		mode = extrarom_profile(np2cfg.extra_machine, &profile) &&
		               profile.n_role == EXTRAROM_ROLE_N80_2
		           ? "N/N80"
		           : "N/N80/N80SR";
		break;
	default:
		break;
	}
	snprintf(status->mode, sizeof(status->mode), "%s%s", mode,
	         high && (status->mode_id == VAEG_MODE_V1 || status->mode_id == VAEG_MODE_V2)
	             ? (pccore_88_standard() ? "S" : "H")
	             : "");
	status->cpu_multiple = pccore_cpu_multiple();
	status->cpu_speed = pccore_cpu_speed();
	status->base_hz = pccore.baseclock * PCCORE_STANDARD_MULTIPLE;
	if (status->compatible && pccore_88_standard())
		status->base_hz /= 2;
	status->cpu_hz =
	    (UINT32)((UINT64)(status->compatible ? pccore_compat_cpu_clock() : pccore_cpu_clock()) *
	             status->cpu_speed / 100);
	status->sgp_hz = status->has_sgp ? sgp_effective_clock() : 0;
}

void machine_status_clock(char *text, size_t size, UINT32 hz) {
	if (hz >= 1000000) {
		snprintf(text, size, "%uMHz", (unsigned)((hz + 500000ULL) / 1000000));
	} else {
		/* Do not display zero MHz for intentionally slow configurations. */
		const UINT32 rounded = (hz + 500) / 1000;
		snprintf(text, size, "%u.%03uMHz", (unsigned)(rounded / 1000), (unsigned)(rounded % 1000));
	}
}

static void status_append(char *text, size_t size, const char *format, ...) {
	const size_t used = strlen(text);
	va_list args;

	if (used >= size)
		return;
	va_start(args, format);
	vsnprintf(text + used, size - used, format, args);
	va_end(args);
}

void machine_status_line(char *text, size_t size, const VAEG_MACHINE_STATUS *status, UINT flags,
                         UINT32 fps_tenths, UINT32 frames) {
	char clock[32];

	if (text == NULL || size == 0 || status == NULL)
		return;
	snprintf(text, size, "%s | %s", status->machine, status->mode);
	if (flags & VAEG_DISPINFO_CPU_CLOCK) {
		machine_status_clock(clock, sizeof(clock), status->cpu_hz);
		status_append(text, size, " | CPU %s", clock);
	}
	if ((flags & VAEG_DISPINFO_SGP_CLOCK) && status->has_sgp) {
		machine_status_clock(clock, sizeof(clock), status->sgp_hz);
		status_append(text, size, " | SGP %s", clock);
	}
	if (flags & VAEG_DISPINFO_FPS) {
		status_append(text, size, " | %u.%uFPS", (unsigned)(fps_tenths / 10),
		              (unsigned)(fps_tenths % 10));
	}
	if (flags & VAEG_DISPINFO_FRAME) {
		status_append(text, size, " | FRAME %u", (unsigned)frames);
	}
}
