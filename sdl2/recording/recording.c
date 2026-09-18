/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer.
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
#include "recording/recording.h"

#include <stddef.h>
#include <stdint.h>

#ifndef VAEG_ENABLE_RECORDING
#define VAEG_ENABLE_RECORDING 0
#endif

static uint64_t vaeg_recording_bytes_per_pixel(uint32_t pixel_format) {
	switch (pixel_format) {
	case VAEG_RECORDING_PIXEL_RGB565:
		return 2u;
	case VAEG_RECORDING_PIXEL_RGB24:
		return 3u;
	default:
		return 0u;
	}
}

VAEG_RECORDING_ERROR
vaeg_recording_validate_source(uint32_t source) {
	if (source >= VAEG_RECORDING_SOURCE_COUNT) {
		return VAEG_RECORDING_ERROR_INVALID_SOURCE;
	}
	return VAEG_RECORDING_ERROR_OK;
}

VAEG_RECORDING_ERROR
vaeg_recording_validate_video_descriptor(const VAEG_RECORDING_VIDEO_DESCRIPTOR *descriptor) {
	uint64_t bytes_per_pixel;
	uint64_t minimum_stride;

	if (descriptor == NULL) {
		return VAEG_RECORDING_ERROR_NULL_DESCRIPTOR;
	}
	if (descriptor->width == 0u || descriptor->height == 0u) {
		return VAEG_RECORDING_ERROR_INVALID_DIMENSIONS;
	}

	bytes_per_pixel = vaeg_recording_bytes_per_pixel(descriptor->pixel_format);
	if (bytes_per_pixel == 0u) {
		return VAEG_RECORDING_ERROR_INVALID_PIXEL_FORMAT;
	}
	minimum_stride = (uint64_t)descriptor->width * bytes_per_pixel;
	if (descriptor->stride_bytes < minimum_stride) {
		return VAEG_RECORDING_ERROR_INVALID_STRIDE;
	}
	if (descriptor->height != 0u &&
	    descriptor->stride_bytes > (uint64_t)SIZE_MAX / descriptor->height) {
		return VAEG_RECORDING_ERROR_SIZE_OVERFLOW;
	}
	return VAEG_RECORDING_ERROR_OK;
}

VAEG_RECORDING_ERROR
vaeg_recording_validate_audio_descriptor(const VAEG_RECORDING_AUDIO_DESCRIPTOR *descriptor) {
	if (descriptor == NULL) {
		return VAEG_RECORDING_ERROR_NULL_DESCRIPTOR;
	}
	if (descriptor->sample_rate_hz == 0u) {
		return VAEG_RECORDING_ERROR_INVALID_SAMPLE_RATE;
	}
	if (descriptor->channels != 2u) {
		return VAEG_RECORDING_ERROR_INVALID_CHANNELS;
	}
	if (descriptor->sample_bits != 16u) {
		return VAEG_RECORDING_ERROR_INVALID_SAMPLE_BITS;
	}
	if (descriptor->format != VAEG_RECORDING_AUDIO_PCM_S16LE) {
		return VAEG_RECORDING_ERROR_INVALID_AUDIO_FORMAT;
	}
	return VAEG_RECORDING_ERROR_OK;
}

VAEG_RECORDING_ERROR
vaeg_recording_validate_copy(const void *pixels, uint64_t bytes) {
	if (bytes > (uint64_t)SIZE_MAX) {
		return VAEG_RECORDING_ERROR_SIZE_OVERFLOW;
	}
	if (pixels == NULL && bytes != 0u) {
		return VAEG_RECORDING_ERROR_NULL_PIXELS;
	}
	return VAEG_RECORDING_ERROR_OK;
}

void vaeg_recording_get_capabilities(VAEG_RECORDING_CAPABILITIES *capabilities) {
	if (capabilities == NULL) {
		return;
	}
	capabilities->build_enabled = VAEG_ENABLE_RECORDING ? 1u : 0u;
	capabilities->backend_available = 0u;
	capabilities->source_count = VAEG_RECORDING_SOURCE_COUNT;
	capabilities->contract_version = VAEG_RECORDING_CONTRACT_VERSION;
}

void vaeg_recording_get_status(VAEG_RECORDING_STATUS *status) {
	if (status == NULL) {
		return;
	}
	status->state =
	    VAEG_ENABLE_RECORDING ? VAEG_RECORDING_STATE_UNAVAILABLE : VAEG_RECORDING_STATE_DISABLED;
	status->source = VAEG_RECORDING_SOURCE_NATIVE;
	status->last_error = VAEG_ENABLE_RECORDING ? VAEG_RECORDING_ERROR_BACKEND_UNAVAILABLE
	                                           : VAEG_RECORDING_ERROR_DISABLED;
}

const char *vaeg_recording_error_name(VAEG_RECORDING_ERROR error) {
	switch (error) {
	case VAEG_RECORDING_ERROR_OK:
		return "ok";
	case VAEG_RECORDING_ERROR_NULL_DESCRIPTOR:
		return "null_descriptor";
	case VAEG_RECORDING_ERROR_INVALID_SOURCE:
		return "invalid_source";
	case VAEG_RECORDING_ERROR_INVALID_DIMENSIONS:
		return "invalid_dimensions";
	case VAEG_RECORDING_ERROR_INVALID_PIXEL_FORMAT:
		return "invalid_pixel_format";
	case VAEG_RECORDING_ERROR_INVALID_STRIDE:
		return "invalid_stride";
	case VAEG_RECORDING_ERROR_SIZE_OVERFLOW:
		return "size_overflow";
	case VAEG_RECORDING_ERROR_ALLOCATION_FAILURE:
		return "allocation_failure";
	case VAEG_RECORDING_ERROR_INVALID_SAMPLE_RATE:
		return "invalid_sample_rate";
	case VAEG_RECORDING_ERROR_INVALID_AUDIO_FORMAT:
		return "invalid_audio_format";
	case VAEG_RECORDING_ERROR_INVALID_CHANNELS:
		return "invalid_channels";
	case VAEG_RECORDING_ERROR_INVALID_SAMPLE_BITS:
		return "invalid_sample_bits";
	case VAEG_RECORDING_ERROR_NULL_OUTPUT:
		return "null_output";
	case VAEG_RECORDING_ERROR_NULL_PIXELS:
		return "null_pixels";
	case VAEG_RECORDING_ERROR_BUFFER_TOO_SMALL:
		return "buffer_too_small";
	case VAEG_RECORDING_ERROR_BACKEND_UNAVAILABLE:
		return "backend_unavailable";
	case VAEG_RECORDING_ERROR_DISABLED:
		return "disabled";
	default:
		return "unknown";
	}
}
