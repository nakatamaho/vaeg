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
#ifndef VAEG_SDL2_RECORDING_H
#define VAEG_SDL2_RECORDING_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VAEG_RECORDING_CONTRACT_VERSION 1u

typedef enum {
	VAEG_RECORDING_SOURCE_NATIVE = 0,
	VAEG_RECORDING_SOURCE_DISPLAYED = 1,
	VAEG_RECORDING_SOURCE_COUNT = 2
} VAEG_RECORDING_SOURCE;

typedef enum {
	VAEG_RECORDING_PIXEL_RGB565 = 1,
	VAEG_RECORDING_PIXEL_RGB24 = 2
} VAEG_RECORDING_PIXEL_FORMAT;

typedef enum {
	VAEG_RECORDING_AUDIO_PCM_S16LE = 1
} VAEG_RECORDING_AUDIO_FORMAT;

typedef enum {
	VAEG_RECORDING_ERROR_OK = 0,
	VAEG_RECORDING_ERROR_NULL_DESCRIPTOR,
	VAEG_RECORDING_ERROR_INVALID_SOURCE,
	VAEG_RECORDING_ERROR_INVALID_DIMENSIONS,
	VAEG_RECORDING_ERROR_INVALID_PIXEL_FORMAT,
	VAEG_RECORDING_ERROR_INVALID_STRIDE,
	VAEG_RECORDING_ERROR_SIZE_OVERFLOW,
	VAEG_RECORDING_ERROR_ALLOCATION_FAILURE,
	VAEG_RECORDING_ERROR_INVALID_SAMPLE_RATE,
	VAEG_RECORDING_ERROR_INVALID_AUDIO_FORMAT,
	VAEG_RECORDING_ERROR_INVALID_CHANNELS,
	VAEG_RECORDING_ERROR_INVALID_SAMPLE_BITS,
	VAEG_RECORDING_ERROR_NULL_OUTPUT,
	VAEG_RECORDING_ERROR_NULL_PIXELS,
	VAEG_RECORDING_ERROR_BUFFER_TOO_SMALL,
	VAEG_RECORDING_ERROR_BACKEND_UNAVAILABLE,
	VAEG_RECORDING_ERROR_DISABLED
} VAEG_RECORDING_ERROR;

typedef enum {
	VAEG_RECORDING_STATE_DISABLED = 0,
	VAEG_RECORDING_STATE_UNAVAILABLE = 1
} VAEG_RECORDING_STATE;

typedef struct {
	uint32_t width;
	uint32_t height;
	uint64_t stride_bytes;
	uint32_t pixel_format;
} VAEG_RECORDING_VIDEO_DESCRIPTOR;

typedef struct {
	uint32_t sample_rate_hz;
	uint16_t channels;
	uint16_t sample_bits;
	uint32_t format;
} VAEG_RECORDING_AUDIO_DESCRIPTOR;

typedef struct {
	uint32_t build_enabled;
	uint32_t backend_available;
	uint32_t source_count;
	uint32_t contract_version;
} VAEG_RECORDING_CAPABILITIES;

typedef struct {
	uint32_t state;
	uint32_t source;
	VAEG_RECORDING_ERROR last_error;
} VAEG_RECORDING_STATUS;

VAEG_RECORDING_ERROR vaeg_recording_validate_source(uint32_t source);
VAEG_RECORDING_ERROR
vaeg_recording_validate_video_descriptor(const VAEG_RECORDING_VIDEO_DESCRIPTOR *descriptor);
VAEG_RECORDING_ERROR
vaeg_recording_validate_audio_descriptor(const VAEG_RECORDING_AUDIO_DESCRIPTOR *descriptor);
VAEG_RECORDING_ERROR vaeg_recording_validate_copy(const void *pixels, uint64_t bytes);

void vaeg_recording_get_capabilities(VAEG_RECORDING_CAPABILITIES *capabilities);
void vaeg_recording_get_status(VAEG_RECORDING_STATUS *status);
const char *vaeg_recording_error_name(VAEG_RECORDING_ERROR error);

#ifdef __cplusplus
}
#endif

#endif
