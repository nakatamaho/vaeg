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
#include "recording/recording_native_tap.h"

#include <stddef.h>

typedef struct {
	int active;
	int failed;
	VAEG_RECORDING_NATIVE_TAP_ERROR error;
	VAEG_RECORDING_NATIVE_TAP_CALLBACK callback;
	void *context;
	uint64_t last_end_ticks;
	uint64_t last_sequence;
	uint64_t accepted_frames;
	uint32_t descriptor_generation;
	int have_sequence;
} VAEG_RECORDING_NATIVE_TAP_STATE;

static VAEG_RECORDING_NATIVE_TAP_STATE tap;

static void tap_fail(VAEG_RECORDING_NATIVE_TAP_ERROR error) {
	tap.active = 0;
	tap.failed = 1;
	tap.error = error;
}

VAEG_RECORDING_NATIVE_TAP_ERROR vaeg_recording_native_tap_start(
	VAEG_RECORDING_NATIVE_TAP_CALLBACK callback, void *context, uint64_t start_ticks,
	uint32_t descriptor_generation) {
	if (callback == NULL) {
		return VAEG_RECORDING_NATIVE_TAP_ERROR_NULL_CALLBACK;
	}
	if (tap.active) {
		return VAEG_RECORDING_NATIVE_TAP_ERROR_ALREADY_ACTIVE;
	}
	tap.active = 1;
	tap.failed = 0;
	tap.error = VAEG_RECORDING_NATIVE_TAP_OK;
	tap.callback = callback;
	tap.context = context;
	tap.last_end_ticks = start_ticks;
	tap.last_sequence = 0u;
	tap.accepted_frames = 0u;
	tap.descriptor_generation = descriptor_generation;
	tap.have_sequence = 0;
	return VAEG_RECORDING_NATIVE_TAP_OK;
}

void vaeg_recording_native_tap_stop(void) {
	tap.active = 0;
	tap.callback = NULL;
	tap.context = NULL;
}

int vaeg_recording_native_tap_is_active(void) {
	return tap.active && !tap.failed;
}

int vaeg_recording_native_tap_requires_guest_composition(int normal_draw) {
	return (normal_draw != 0) || vaeg_recording_native_tap_is_active();
}

VAEG_RECORDING_NATIVE_TAP_ERROR vaeg_recording_native_tap_submit_frame(
	uint64_t sequence, uint64_t end_ticks, uint32_t descriptor_generation,
	const VAEG_RECORDING_NATIVE_FRAME *frame, const VAEG_RECORDING_OSD_SNAPSHOT *osd) {
	VAEG_RECORDING_NATIVE_CAPTURE capture;
	if (!tap.active || tap.failed || tap.callback == NULL) {
		return VAEG_RECORDING_NATIVE_TAP_ERROR_NOT_ACTIVE;
	}
	if ((frame == NULL) || (osd == NULL)) {
		tap_fail(VAEG_RECORDING_NATIVE_TAP_ERROR_NULL_FRAME);
		return tap.error;
	}
	if (descriptor_generation != tap.descriptor_generation) {
		tap_fail(VAEG_RECORDING_NATIVE_TAP_ERROR_DESCRIPTOR_CHANGED);
		return tap.error;
	}
	if ((end_ticks < tap.last_end_ticks) || (end_ticks == tap.last_end_ticks)) {
		tap_fail(VAEG_RECORDING_NATIVE_TAP_ERROR_INVALID_TIME);
		return tap.error;
	}
	if (tap.have_sequence && (sequence <= tap.last_sequence)) {
		tap_fail(VAEG_RECORDING_NATIVE_TAP_ERROR_INVALID_SEQUENCE);
		return tap.error;
	}
	if ((osd->version != VAEG_RECORDING_OSD_SNAPSHOT_VERSION) ||
	    (osd->layer_count > VAEG_RECORDING_OSD_MAX_LAYERS) ||
	    (osd->frame_token != sequence) || (frame->frame_token != sequence) ||
	    (osd->guest_width != frame->width) || (osd->guest_height != frame->height) ||
	    (osd->output_width == 0u) || (osd->output_height == 0u) ||
	    (frame->pixels == NULL) || (frame->width == 0u) || (frame->height == 0u) ||
	    ((uint64_t)frame->pitch_bytes < (uint64_t)frame->width * 3u) ||
	    ((uint64_t)frame->height > UINT64_MAX / frame->pitch_bytes) ||
	    (frame->size_bytes < (uint64_t)frame->height * frame->pitch_bytes)) {
		tap_fail(VAEG_RECORDING_NATIVE_TAP_ERROR_INVALID_CAPTURE);
		return tap.error;
	}
	capture.sequence = sequence;
	capture.begin_ticks = tap.last_end_ticks;
	capture.end_ticks = end_ticks;
	capture.descriptor_generation = descriptor_generation;
	capture.frame = frame;
	capture.osd = osd;
	if (!tap.callback(tap.context, &capture)) {
		tap_fail(VAEG_RECORDING_NATIVE_TAP_ERROR_CALLBACK_REJECTED);
		return tap.error;
	}
	tap.last_end_ticks = end_ticks;
	tap.last_sequence = sequence;
	tap.accepted_frames++;
	tap.have_sequence = 1;
	return VAEG_RECORDING_NATIVE_TAP_OK;
}

void vaeg_recording_native_tap_fail(VAEG_RECORDING_NATIVE_TAP_ERROR error) {
	if (tap.active) {
		tap_fail(error == VAEG_RECORDING_NATIVE_TAP_OK
	                 ? VAEG_RECORDING_NATIVE_TAP_ERROR_CAPTURE_FAILED
	                 : error);
	}
}

VAEG_RECORDING_NATIVE_TAP_ERROR vaeg_recording_native_tap_last_error(void) {
	return tap.error;
}

uint64_t vaeg_recording_native_tap_accepted_frames(void) {
	return tap.accepted_frames;
}

const char *vaeg_recording_native_tap_error_name(VAEG_RECORDING_NATIVE_TAP_ERROR error) {
	switch (error) {
	case VAEG_RECORDING_NATIVE_TAP_OK:
		return "ok";
	case VAEG_RECORDING_NATIVE_TAP_ERROR_NULL_CALLBACK:
		return "null_callback";
	case VAEG_RECORDING_NATIVE_TAP_ERROR_ALREADY_ACTIVE:
		return "already_active";
	case VAEG_RECORDING_NATIVE_TAP_ERROR_NOT_ACTIVE:
		return "not_active";
	case VAEG_RECORDING_NATIVE_TAP_ERROR_NULL_FRAME:
		return "null_frame";
	case VAEG_RECORDING_NATIVE_TAP_ERROR_INVALID_TIME:
		return "invalid_time";
	case VAEG_RECORDING_NATIVE_TAP_ERROR_INVALID_SEQUENCE:
		return "invalid_sequence";
	case VAEG_RECORDING_NATIVE_TAP_ERROR_DESCRIPTOR_CHANGED:
		return "descriptor_changed";
	case VAEG_RECORDING_NATIVE_TAP_ERROR_INVALID_CAPTURE:
		return "invalid_capture";
	case VAEG_RECORDING_NATIVE_TAP_ERROR_CALLBACK_REJECTED:
		return "callback_rejected";
	case VAEG_RECORDING_NATIVE_TAP_ERROR_CAPTURE_FAILED:
		return "capture_failed";
	default:
		return "unknown";
	}
}
