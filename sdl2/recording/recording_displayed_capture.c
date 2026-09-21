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
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "recording/recording_displayed_capture.h"

#include <stddef.h>

static int vaeg_recording_displayed_checked_size(uint32_t width, uint32_t height,
                                                  uint32_t pitch, uint64_t *size) {
	uint64_t minimum_pitch;
	uint64_t total;

	if (size == NULL) {
		return 0;
	}
	minimum_pitch = (uint64_t)width * 4u;
	if ((uint64_t)pitch < minimum_pitch || (height != 0u && (uint64_t)pitch > UINT64_MAX / height)) {
		return 0;
	}
	total = (uint64_t)pitch * height;
	if (total > VAEG_RECORDING_DISPLAYED_FRAME_MAX_BYTES) {
		return 0;
	}
	*size = total;
	return 1;
}

VAEG_RECORDING_DISPLAYED_CAPTURE_ERROR vaeg_recording_displayed_capture_validate(
	const VAEG_RECORDING_DISPLAYED_FRAME *capture) {
	uint64_t expected_size;

	if (capture == NULL) {
		return VAEG_RECORDING_DISPLAYED_CAPTURE_NULL_INPUT;
	}
	if (capture->pixels == NULL) {
		return VAEG_RECORDING_DISPLAYED_CAPTURE_NULL_PIXELS;
	}
	if ((capture->width == 0u) || (capture->height == 0u)) {
		return VAEG_RECORDING_DISPLAYED_CAPTURE_ZERO_EXTENT;
	}
	if (capture->pixel_format != VAEG_RECORDING_DISPLAYED_PIXEL_RGBA32) {
		return VAEG_RECORDING_DISPLAYED_CAPTURE_UNKNOWN_FORMAT;
	}
	if (capture->row_origin != VAEG_RECORDING_DISPLAYED_ROWS_TOP_DOWN) {
		return VAEG_RECORDING_DISPLAYED_CAPTURE_INVALID_ROW_ORIGIN;
	}
	if (!vaeg_recording_displayed_checked_size(capture->width, capture->height,
	                                            capture->pitch_bytes, &expected_size)) {
		if ((uint64_t)capture->pitch_bytes < (uint64_t)capture->width * 4u) {
			return VAEG_RECORDING_DISPLAYED_CAPTURE_SHORT_PITCH;
		}
		return (capture->height != 0u &&
		        (uint64_t)capture->pitch_bytes > UINT64_MAX / capture->height)
		           ? VAEG_RECORDING_DISPLAYED_CAPTURE_SIZE_OVERFLOW
		           : VAEG_RECORDING_DISPLAYED_CAPTURE_TOO_LARGE;
	}
	if (capture->size_bytes < expected_size) {
		return VAEG_RECORDING_DISPLAYED_CAPTURE_BUFFER_TOO_SMALL;
	}
	return VAEG_RECORDING_DISPLAYED_CAPTURE_OK;
}

VAEG_RECORDING_DISPLAYED_CAPTURE_ERROR vaeg_recording_displayed_capture_to_rgb24(
	const VAEG_RECORDING_DISPLAYED_FRAME *capture, uint8_t *output, uint32_t output_pitch_bytes,
	uint64_t output_size_bytes) {
	uint64_t output_size;
	VAEG_RECORDING_DISPLAYED_CAPTURE_ERROR error;
	uint32_t y;

	if (output == NULL) {
		return VAEG_RECORDING_DISPLAYED_CAPTURE_NULL_PIXELS;
	}
	error = vaeg_recording_displayed_capture_validate(capture);
	if (error != VAEG_RECORDING_DISPLAYED_CAPTURE_OK) {
		return error;
	}
	if ((uint64_t)output_pitch_bytes < (uint64_t)capture->width * 3u ||
	    (capture->height != 0u &&
	     (uint64_t)output_pitch_bytes > UINT64_MAX / capture->height)) {
		return VAEG_RECORDING_DISPLAYED_CAPTURE_SHORT_PITCH;
	}
	output_size = (uint64_t)output_pitch_bytes * capture->height;
	if (output_size > VAEG_RECORDING_DISPLAYED_FRAME_MAX_BYTES) {
		return VAEG_RECORDING_DISPLAYED_CAPTURE_TOO_LARGE;
	}
	if (output_size_bytes < output_size) {
		return VAEG_RECORDING_DISPLAYED_CAPTURE_BUFFER_TOO_SMALL;
	}
	for (y = 0u; y < capture->height; ++y) {
		const uint8_t *source_row = capture->pixels + (size_t)y * capture->pitch_bytes;
		uint8_t *output_row = output + (size_t)y * output_pitch_bytes;
		uint32_t x;
		for (x = 0u; x < capture->width; ++x) {
			output_row[x * 3u] = source_row[x * 4u];
			output_row[x * 3u + 1u] = source_row[x * 4u + 1u];
			output_row[x * 3u + 2u] = source_row[x * 4u + 2u];
		}
	}
	return VAEG_RECORDING_DISPLAYED_CAPTURE_OK;
}

const char *vaeg_recording_displayed_capture_error_name(
	VAEG_RECORDING_DISPLAYED_CAPTURE_ERROR error) {
	switch (error) {
	case VAEG_RECORDING_DISPLAYED_CAPTURE_OK:
		return "ok";
	case VAEG_RECORDING_DISPLAYED_CAPTURE_NULL_INPUT:
		return "null_input";
	case VAEG_RECORDING_DISPLAYED_CAPTURE_NULL_PIXELS:
		return "null_pixels";
	case VAEG_RECORDING_DISPLAYED_CAPTURE_ZERO_EXTENT:
		return "zero_extent";
	case VAEG_RECORDING_DISPLAYED_CAPTURE_UNKNOWN_FORMAT:
		return "unknown_format";
	case VAEG_RECORDING_DISPLAYED_CAPTURE_INVALID_ROW_ORIGIN:
		return "invalid_row_origin";
	case VAEG_RECORDING_DISPLAYED_CAPTURE_SHORT_PITCH:
		return "short_pitch";
	case VAEG_RECORDING_DISPLAYED_CAPTURE_SIZE_OVERFLOW:
		return "size_overflow";
	case VAEG_RECORDING_DISPLAYED_CAPTURE_TOO_LARGE:
		return "too_large";
	case VAEG_RECORDING_DISPLAYED_CAPTURE_BUFFER_TOO_SMALL:
		return "buffer_too_small";
	case VAEG_RECORDING_DISPLAYED_CAPTURE_UNAVAILABLE:
		return "unavailable";
	case VAEG_RECORDING_DISPLAYED_CAPTURE_DUMMY_DRIVER:
		return "dummy_driver";
	case VAEG_RECORDING_DISPLAYED_CAPTURE_BUSY:
		return "busy";
	case VAEG_RECORDING_DISPLAYED_CAPTURE_GEOMETRY_CHANGED:
		return "geometry_changed";
	case VAEG_RECORDING_DISPLAYED_CAPTURE_PRESENTER_CHANGED:
		return "presenter_changed";
	case VAEG_RECORDING_DISPLAYED_CAPTURE_READBACK_FAILED:
		return "readback_failed";
	case VAEG_RECORDING_DISPLAYED_CAPTURE_CALLBACK_REJECTED:
		return "callback_rejected";
	case VAEG_RECORDING_DISPLAYED_CAPTURE_ALLOCATION_FAILED:
		return "allocation_failed";
	default:
		return "unknown";
	}
}
