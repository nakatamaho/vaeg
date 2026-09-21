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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "recording/recording_displayed_capture.h"

static VAEG_RECORDING_DISPLAYED_FRAME make_frame(const uint8_t *pixels,
                                                  uint32_t width, uint32_t height,
                                                  uint32_t pitch, uint64_t size) {
	VAEG_RECORDING_DISPLAYED_FRAME frame;

	memset(&frame, 0, sizeof(frame));
	frame.pixels = pixels;
	frame.width = width;
	frame.height = height;
	frame.pitch_bytes = pitch;
	frame.size_bytes = size;
	frame.pixel_format = VAEG_RECORDING_DISPLAYED_PIXEL_RGBA32;
	frame.row_origin = VAEG_RECORDING_DISPLAYED_ROWS_TOP_DOWN;
	frame.frame_token = 12u;
	frame.descriptor_generation = 3u;
	return frame;
}

int main(void) {
	const uint8_t source[] = {
		10, 20, 30, 255, 40, 50, 60, 255, 70, 80, 90, 255, 0xaa, 0xbb, 0xcc, 0xdd,
		11, 21, 31, 255, 41, 51, 61, 255, 71, 81, 91, 255, 0xee, 0xff, 0x12, 0x34
	};
	const uint8_t expected[] = {
		10, 20, 30, 40, 50, 60, 70, 80, 90, 0xcc, 0xcc, 0xcc,
		11, 21, 31, 41, 51, 61, 71, 81, 91, 0xcc, 0xcc, 0xcc
	};
	uint8_t output[24];
	VAEG_RECORDING_DISPLAYED_FRAME frame;

	frame = make_frame(source, 3u, 2u, 16u, sizeof(source));
	assert(vaeg_recording_displayed_capture_validate(&frame) ==
	       VAEG_RECORDING_DISPLAYED_CAPTURE_OK);
	memset(output, 0xcc, sizeof(output));
	assert(vaeg_recording_displayed_capture_to_rgb24(&frame, output, 12u, sizeof(output)) ==
	       VAEG_RECORDING_DISPLAYED_CAPTURE_OK);
	assert(memcmp(output, expected, sizeof(output)) == 0);

	frame.pitch_bytes = 11u;
	assert(vaeg_recording_displayed_capture_validate(&frame) ==
	       VAEG_RECORDING_DISPLAYED_CAPTURE_SHORT_PITCH);
	frame.pitch_bytes = 16u;
	frame.size_bytes = sizeof(source) - 1u;
	assert(vaeg_recording_displayed_capture_validate(&frame) ==
	       VAEG_RECORDING_DISPLAYED_CAPTURE_BUFFER_TOO_SMALL);
	frame.size_bytes = sizeof(source);
	frame.row_origin = 1u;
	assert(vaeg_recording_displayed_capture_validate(&frame) ==
	       VAEG_RECORDING_DISPLAYED_CAPTURE_INVALID_ROW_ORIGIN);
	frame.row_origin = VAEG_RECORDING_DISPLAYED_ROWS_TOP_DOWN;
	assert(vaeg_recording_displayed_capture_to_rgb24(&frame, output, 8u, sizeof(output)) ==
	       VAEG_RECORDING_DISPLAYED_CAPTURE_SHORT_PITCH);
	assert(strcmp(vaeg_recording_displayed_capture_error_name(
	                 VAEG_RECORDING_DISPLAYED_CAPTURE_GEOMETRY_CHANGED),
	             "geometry_changed") == 0);
	return 0;
}
