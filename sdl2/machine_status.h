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
#ifndef VAEG_MACHINE_STATUS_H
#define VAEG_MACHINE_STATUS_H

#include <stddef.h>

typedef enum {
	VAEG_MODE_V3,
	VAEG_MODE_V1,
	VAEG_MODE_V2,
	VAEG_MODE_N,
	VAEG_MODE_N80,
	VAEG_MODE_N80SR,
	VAEG_MODE_N_FAMILY
} VAEG_MACHINE_MODE;

typedef struct {
	char machine[64];
	char mode[16];
	VAEG_MACHINE_MODE mode_id;
	BOOL compatible;
	BOOL has_sgp;
	UINT32 cpu_hz;
	UINT32 base_hz;
	UINT32 sgp_hz;
	UINT cpu_multiple;
	UINT cpu_speed;
} VAEG_MACHINE_STATUS;

#ifdef __cplusplus
extern "C" {
#endif

/* Shared by status and lamps. Subtypes require runtime evidence, not banners
 * found in ROMs. N80/N80SR enum slots permit future verified detectors. */
VAEG_MACHINE_MODE machine_status_mode(void);
BOOL machine_status_n_selected(void);
void machine_status_get(VAEG_MACHINE_STATUS *status);
void machine_status_clock(char *text, size_t size, UINT32 hz);
void machine_status_line(char *text, size_t size, const VAEG_MACHINE_STATUS *status, UINT flags,
                         UINT32 fps_tenths, UINT32 frames);

#ifdef __cplusplus
}
#endif
#endif
