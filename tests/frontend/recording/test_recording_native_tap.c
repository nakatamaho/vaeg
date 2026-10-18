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
#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "recording/recording_native_tap.h"

typedef struct {
	uint32_t calls;
	uint64_t sequence[4];
	uint64_t begin_ticks[4];
	uint64_t end_ticks[4];
	uint32_t descriptor_generation[4];
	uint8_t pixels[4][6];
	uint64_t osd_token[4];
	int reject;
} CALLBACK_STATE;

static int capture_callback(void *opaque, const VAEG_RECORDING_NATIVE_CAPTURE *capture) {
	CALLBACK_STATE *state = (CALLBACK_STATE *)opaque;
	uint32_t index = state->calls++;

	assert(index < 4u);
	assert(capture != NULL);
	assert(capture->frame != NULL);
	assert(capture->osd != NULL);
	state->sequence[index] = capture->sequence;
	state->begin_ticks[index] = capture->begin_ticks;
	state->end_ticks[index] = capture->end_ticks;
	state->descriptor_generation[index] = capture->descriptor_generation;
	memcpy(state->pixels[index], capture->frame->pixels, sizeof(state->pixels[index]));
	state->osd_token[index] = capture->osd->frame_token;
	return state->reject ? 0 : 1;
}

static void make_capture(VAEG_RECORDING_NATIVE_FRAME *frame,
                         VAEG_RECORDING_OSD_SNAPSHOT *osd, uint64_t sequence,
                         const uint8_t *pixels) {
	memset(frame, 0, sizeof(*frame));
	frame->pixels = (uint8_t *)pixels;
	frame->width = 2u;
	frame->height = 1u;
	frame->pitch_bytes = 6u;
	frame->size_bytes = 6u;
	frame->sample_aspect_width = 1u;
	frame->sample_aspect_height = 1u;
	frame->frame_token = sequence;
	memset(osd, 0, sizeof(*osd));
	osd->version = VAEG_RECORDING_OSD_SNAPSHOT_VERSION;
	osd->frame_token = sequence;
	osd->guest_width = 2u;
	osd->guest_height = 1u;
	osd->output_width = 2u;
	osd->output_height = 1u;
}

static void test_intervals_and_ownership(void) {
	static const uint8_t pixels_a[] = {1u, 2u, 3u, 4u, 5u, 6u};
	static const uint8_t pixels_b[] = {7u, 8u, 9u, 10u, 11u, 12u};
	VAEG_RECORDING_NATIVE_FRAME frame;
	VAEG_RECORDING_OSD_SNAPSHOT osd;
	CALLBACK_STATE state;
	memset(&state, 0, sizeof(state));

	assert(vaeg_recording_native_tap_start(NULL, NULL, 100u, 9u) ==
	       VAEG_RECORDING_NATIVE_TAP_ERROR_NULL_CALLBACK);
	assert(vaeg_recording_native_tap_start(capture_callback, &state, 100u, 9u) ==
	       VAEG_RECORDING_NATIVE_TAP_OK);
	assert(vaeg_recording_native_tap_is_active());
	assert(vaeg_recording_native_tap_requires_guest_composition(0));
	assert(vaeg_recording_native_tap_requires_guest_composition(1));
	assert(vaeg_recording_native_tap_start(capture_callback, &state, 100u, 9u) ==
	       VAEG_RECORDING_NATIVE_TAP_ERROR_ALREADY_ACTIVE);

	make_capture(&frame, &osd, 1u, pixels_a);
	assert(vaeg_recording_native_tap_submit_frame(1u, 120u, 9u, &frame, &osd) ==
	       VAEG_RECORDING_NATIVE_TAP_OK);
	make_capture(&frame, &osd, 2u, pixels_b);
	assert(vaeg_recording_native_tap_submit_frame(2u, 150u, 9u, &frame, &osd) ==
	       VAEG_RECORDING_NATIVE_TAP_OK);
	assert(state.calls == 2u);
	assert(state.sequence[0] == 1u && state.begin_ticks[0] == 100u &&
	       state.end_ticks[0] == 120u && state.osd_token[0] == 1u);
	assert(state.sequence[1] == 2u && state.begin_ticks[1] == 120u &&
	       state.end_ticks[1] == 150u && state.osd_token[1] == 2u);
	assert(memcmp(state.pixels[0], pixels_a, sizeof(pixels_a)) == 0);
	assert(memcmp(state.pixels[1], pixels_b, sizeof(pixels_b)) == 0);
	assert(vaeg_recording_native_tap_accepted_frames() == 2u);
	vaeg_recording_native_tap_stop();
}

static void test_descriptor_and_time_fail_closed(void) {
	static const uint8_t pixels[] = {1u, 2u, 3u, 4u, 5u, 6u};
	VAEG_RECORDING_NATIVE_FRAME frame;
	VAEG_RECORDING_OSD_SNAPSHOT osd;
	CALLBACK_STATE state;
	memset(&state, 0, sizeof(state));
	assert(vaeg_recording_native_tap_start(capture_callback, &state, 10u, 7u) ==
	       VAEG_RECORDING_NATIVE_TAP_OK);
	make_capture(&frame, &osd, 1u, pixels);
	assert(vaeg_recording_native_tap_submit_frame(1u, 20u, 8u, &frame, &osd) ==
	       VAEG_RECORDING_NATIVE_TAP_ERROR_DESCRIPTOR_CHANGED);
	assert(!vaeg_recording_native_tap_is_active());
	assert(vaeg_recording_native_tap_last_error() ==
	       VAEG_RECORDING_NATIVE_TAP_ERROR_DESCRIPTOR_CHANGED);

	memset(&state, 0, sizeof(state));
	assert(vaeg_recording_native_tap_start(capture_callback, &state, 10u, 7u) ==
	       VAEG_RECORDING_NATIVE_TAP_OK);
	make_capture(&frame, &osd, 1u, pixels);
	assert(vaeg_recording_native_tap_submit_frame(1u, 9u, 7u, &frame, &osd) ==
	       VAEG_RECORDING_NATIVE_TAP_ERROR_INVALID_TIME);
	assert(!vaeg_recording_native_tap_is_active());
	vaeg_recording_native_tap_stop();
}

static void test_callback_failure(void) {
	static const uint8_t pixels[] = {1u, 2u, 3u, 4u, 5u, 6u};
	VAEG_RECORDING_NATIVE_FRAME frame;
	VAEG_RECORDING_OSD_SNAPSHOT osd;
	CALLBACK_STATE state;
	memset(&state, 0, sizeof(state));
	state.reject = 1;
	assert(vaeg_recording_native_tap_start(capture_callback, &state, 0u, 1u) ==
	       VAEG_RECORDING_NATIVE_TAP_OK);
	make_capture(&frame, &osd, 1u, pixels);
	assert(vaeg_recording_native_tap_submit_frame(1u, 1u, 1u, &frame, &osd) ==
	       VAEG_RECORDING_NATIVE_TAP_ERROR_CALLBACK_REJECTED);
	assert(!vaeg_recording_native_tap_is_active());
	assert(vaeg_recording_native_tap_error_name(
	           VAEG_RECORDING_NATIVE_TAP_ERROR_CALLBACK_REJECTED) != NULL);
	vaeg_recording_native_tap_stop();
}

static void test_rollover_initial_snapshot(void) {
	static const uint8_t pixels[] = {1, 2, 3, 4, 5, 6};
	VAEG_RECORDING_NATIVE_FRAME frame;
	VAEG_RECORDING_OSD_SNAPSHOT osd;
	CALLBACK_STATE state;
	memset(&state, 0, sizeof(state));
	assert(vaeg_recording_native_tap_start(capture_callback, &state, 100, 9) ==
	       VAEG_RECORDING_NATIVE_TAP_OK);
	make_capture(&frame, &osd, 4, pixels);
	assert(vaeg_recording_native_tap_submit_initial_snapshot(4, 100, 9, &frame, &osd) ==
	       VAEG_RECORDING_NATIVE_TAP_OK);
	assert(state.calls == 1 && state.begin_ticks[0] == 100 && state.end_ticks[0] == 100);
	make_capture(&frame, &osd, 5, pixels);
	assert(vaeg_recording_native_tap_submit_frame(5, 120, 9, &frame, &osd) ==
	       VAEG_RECORDING_NATIVE_TAP_OK);
	assert(state.begin_ticks[1] == 100 && state.end_ticks[1] == 120);
	/* Exactly one mutation: an initial snapshot after capture already began. */
	assert(vaeg_recording_native_tap_submit_initial_snapshot(5, 120, 9, &frame, &osd) ==
	       VAEG_RECORDING_NATIVE_TAP_ERROR_INVALID_TIME);
	vaeg_recording_native_tap_stop();
}

int main(void) {
	test_rollover_initial_snapshot();
	test_intervals_and_ownership();
	test_descriptor_and_time_fail_closed();
	test_callback_failure();
	return 0;
}
