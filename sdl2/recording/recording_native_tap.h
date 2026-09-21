/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef VAEG_SDL2_RECORDING_NATIVE_TAP_H
#define VAEG_SDL2_RECORDING_NATIVE_TAP_H

#include <stdint.h>

#include "recording/recording_native_composition.h"
#include "recording/recording_osd_snapshot.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	VAEG_RECORDING_NATIVE_TAP_OK = 0,
	VAEG_RECORDING_NATIVE_TAP_ERROR_NULL_CALLBACK,
	VAEG_RECORDING_NATIVE_TAP_ERROR_ALREADY_ACTIVE,
	VAEG_RECORDING_NATIVE_TAP_ERROR_NOT_ACTIVE,
	VAEG_RECORDING_NATIVE_TAP_ERROR_NULL_FRAME,
	VAEG_RECORDING_NATIVE_TAP_ERROR_INVALID_TIME,
	VAEG_RECORDING_NATIVE_TAP_ERROR_INVALID_SEQUENCE,
	VAEG_RECORDING_NATIVE_TAP_ERROR_DESCRIPTOR_CHANGED,
	VAEG_RECORDING_NATIVE_TAP_ERROR_INVALID_CAPTURE,
	VAEG_RECORDING_NATIVE_TAP_ERROR_CALLBACK_REJECTED,
	VAEG_RECORDING_NATIVE_TAP_ERROR_CAPTURE_FAILED
} VAEG_RECORDING_NATIVE_TAP_ERROR;

/*
 * The frame and snapshot are owned by the tap producer and are valid only for
 * the duration of the callback. A consumer must copy the frame bytes and any
 * snapshot values it retains before returning. No SDL, GPU or FFmpeg object is
 * exposed by this boundary.
 */
typedef struct {
	uint64_t sequence;
	uint64_t begin_ticks;
	uint64_t end_ticks;
	uint32_t descriptor_generation;
	const VAEG_RECORDING_NATIVE_FRAME *frame;
	const VAEG_RECORDING_OSD_SNAPSHOT *osd;
} VAEG_RECORDING_NATIVE_CAPTURE;

typedef int (*VAEG_RECORDING_NATIVE_TAP_CALLBACK)(
	void *context, const VAEG_RECORDING_NATIVE_CAPTURE *capture);

VAEG_RECORDING_NATIVE_TAP_ERROR vaeg_recording_native_tap_start(
	VAEG_RECORDING_NATIVE_TAP_CALLBACK callback, void *context, uint64_t start_ticks,
	uint32_t descriptor_generation);
void vaeg_recording_native_tap_stop(void);
int vaeg_recording_native_tap_is_active(void);
int vaeg_recording_native_tap_requires_guest_composition(int normal_draw);
VAEG_RECORDING_NATIVE_TAP_ERROR vaeg_recording_native_tap_submit_frame(
	uint64_t sequence, uint64_t end_ticks, uint32_t descriptor_generation,
	const VAEG_RECORDING_NATIVE_FRAME *frame, const VAEG_RECORDING_OSD_SNAPSHOT *osd);
void vaeg_recording_native_tap_fail(VAEG_RECORDING_NATIVE_TAP_ERROR error);
VAEG_RECORDING_NATIVE_TAP_ERROR vaeg_recording_native_tap_last_error(void);
uint64_t vaeg_recording_native_tap_accepted_frames(void);
const char *vaeg_recording_native_tap_error_name(VAEG_RECORDING_NATIVE_TAP_ERROR error);

#ifdef __cplusplus
}
#endif

#endif
