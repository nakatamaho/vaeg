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
#ifndef VAEG_SDL2_RECORDING_NATIVE_COMPOSITION_H
#define VAEG_SDL2_RECORDING_NATIVE_COMPOSITION_H

#include <stdint.h>

#include "recording/recording_osd_snapshot.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
	VAEG_RECORDING_NATIVE_FRAME_MAX_BYTES = 256 * 1024 * 1024
};

typedef enum {
	VAEG_RECORDING_NATIVE_FRAME_OK = 0,
	VAEG_RECORDING_NATIVE_FRAME_NULL_SOURCE,
	VAEG_RECORDING_NATIVE_FRAME_NULL_OUTPUT,
	VAEG_RECORDING_NATIVE_FRAME_NULL_GLYPHS,
	VAEG_RECORDING_NATIVE_FRAME_ZERO_EXTENT,
	VAEG_RECORDING_NATIVE_FRAME_SHORT_PITCH,
	VAEG_RECORDING_NATIVE_FRAME_SOURCE_SIZE_OVERFLOW,
	VAEG_RECORDING_NATIVE_FRAME_SIZE_OVERFLOW,
	VAEG_RECORDING_NATIVE_FRAME_TOO_LARGE,
	VAEG_RECORDING_NATIVE_FRAME_OUTPUT_BUSY,
	VAEG_RECORDING_NATIVE_FRAME_INVALID_SNAPSHOT,
	VAEG_RECORDING_NATIVE_FRAME_INVALID_LAYER
} VAEG_RECORDING_NATIVE_FRAME_ERROR;

typedef struct {
	const uint8_t *pixels;
	uint32_t width;
	uint32_t height;
	uint32_t pitch_bytes;
	uint64_t source_size_bytes;
} VAEG_RECORDING_NATIVE_RGB565_SOURCE;

typedef struct {
	uint8_t *pixels;
	uint32_t width;
	uint32_t height;
	uint32_t pitch_bytes;
	uint64_t size_bytes;
	uint32_t sample_aspect_width;
	uint32_t sample_aspect_height;
	uint64_t frame_token;
} VAEG_RECORDING_NATIVE_FRAME;

void vaeg_recording_native_frame_initialize(VAEG_RECORDING_NATIVE_FRAME *frame);
void vaeg_recording_native_frame_destroy(VAEG_RECORDING_NATIVE_FRAME *frame);
VAEG_RECORDING_NATIVE_FRAME_ERROR vaeg_recording_native_frame_copy_rgb565(
	const VAEG_RECORDING_NATIVE_RGB565_SOURCE *source, VAEG_RECORDING_NATIVE_FRAME *frame);
VAEG_RECORDING_NATIVE_FRAME_ERROR vaeg_recording_native_frame_apply_osd(
	VAEG_RECORDING_NATIVE_FRAME *frame, const VAEG_RECORDING_OSD_SNAPSHOT *snapshot,
	const uint8_t *glyphs_8x8);
const char *vaeg_recording_native_frame_error_name(VAEG_RECORDING_NATIVE_FRAME_ERROR error);

#ifdef __cplusplus
}
#endif

#endif
