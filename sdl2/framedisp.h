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
#ifndef VAEG_SDL2_FRAMEDISP_H
#define VAEG_SDL2_FRAMEDISP_H

typedef struct {
	UINT32 tick;
	UINT32 draws;
	UINT32 fps_tenths;
} VAEG_FRAMEDISP;

/* Guest-time speed is independent of frame frequency and draw skipping. */
typedef struct {
	UINT32 tick;
	UINT64 guest_ticks;
	UINT32 clock_hz;
	UINT32 cpu_hz;
	UINT32 policy;
	BOOL started;
	BOOL valid;
	double speed;
	double equivalent_mhz;
} VAEG_RUNTIME_SPEED;

#ifdef __cplusplus
extern "C" {
#endif

void vaeg_framedisp_reset(VAEG_FRAMEDISP *state, UINT32 tick, UINT32 draws);
BOOL vaeg_framedisp_update(VAEG_FRAMEDISP *state, UINT32 tick, UINT32 draws);
void vaeg_runtime_speed_reset(VAEG_RUNTIME_SPEED *state);
BOOL vaeg_runtime_speed_update(VAEG_RUNTIME_SPEED *state, UINT32 tick, UINT64 guest_ticks,
                               UINT32 clock_hz, UINT32 cpu_hz, UINT32 policy);

#ifdef __cplusplus
}
#endif

#endif
