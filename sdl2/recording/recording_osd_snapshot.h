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
#ifndef VAEG_SDL2_RECORDING_OSD_SNAPSHOT_H
#define VAEG_SDL2_RECORDING_OSD_SNAPSHOT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VAEG_RECORDING_OSD_SNAPSHOT_VERSION 1u
#define VAEG_RECORDING_OSD_MAX_LAYERS 2u
#define VAEG_RECORDING_OSD_MAX_LINES 12u
#define VAEG_RECORDING_OSD_MAX_LINE_BYTES 96u

typedef enum {
	VAEG_RECORDING_OSD_VIDEO_INFO = 0,
	VAEG_RECORDING_OSD_FRAMEBUFFER_INFO = 1,
	VAEG_RECORDING_OSD_LAYER_COUNT = 2
} VAEG_RECORDING_OSD_LAYER_KIND;

typedef enum {
	VAEG_RECORDING_OSD_COORDINATE_OUTPUT = 0
} VAEG_RECORDING_OSD_COORDINATE_SPACE;

typedef enum {
	VAEG_RECORDING_OSD_ANCHOR_TOP_RIGHT = 0
} VAEG_RECORDING_OSD_ANCHOR;

typedef enum {
	VAEG_RECORDING_OSD_ERROR_OK = 0,
	VAEG_RECORDING_OSD_ERROR_NULL_INPUT,
	VAEG_RECORDING_OSD_ERROR_NULL_OUTPUT,
	VAEG_RECORDING_OSD_ERROR_INVALID_DIMENSIONS,
	VAEG_RECORDING_OSD_ERROR_TOO_MANY_LAYERS,
	VAEG_RECORDING_OSD_ERROR_INVALID_LAYER_KIND,
	VAEG_RECORDING_OSD_ERROR_INVALID_COORDINATE_SPACE,
	VAEG_RECORDING_OSD_ERROR_INVALID_ANCHOR,
	VAEG_RECORDING_OSD_ERROR_INVALID_FLAG,
	VAEG_RECORDING_OSD_ERROR_INVALID_RECTANGLE,
	VAEG_RECORDING_OSD_ERROR_TOO_MANY_LINES,
	VAEG_RECORDING_OSD_ERROR_NULL_LINE,
	VAEG_RECORDING_OSD_ERROR_LINE_TOO_LONG,
	VAEG_RECORDING_OSD_ERROR_DUPLICATE_LAYER
} VAEG_RECORDING_OSD_ERROR;

typedef struct {
	uint32_t kind;
	uint32_t coordinate_space;
	uint32_t anchor;
	int32_t x;
	int32_t y;
	int32_t width;
	int32_t height;
	int32_t clip_x;
	int32_t clip_y;
	int32_t clip_width;
	int32_t clip_height;
	uint8_t enabled;
	uint8_t visible;
	uint8_t scale;
	uint8_t background_alpha;
	uint8_t text_alpha;
	uint32_t line_count;
	const char *const *lines;
} VAEG_RECORDING_OSD_LAYER_INPUT;

typedef struct {
	uint32_t kind;
	uint32_t coordinate_space;
	uint32_t anchor;
	int32_t x;
	int32_t y;
	int32_t width;
	int32_t height;
	int32_t clip_x;
	int32_t clip_y;
	int32_t clip_width;
	int32_t clip_height;
	uint8_t enabled;
	uint8_t visible;
	uint8_t scale;
	uint8_t background_alpha;
	uint8_t text_alpha;
	uint32_t line_count;
	char lines[VAEG_RECORDING_OSD_MAX_LINES][VAEG_RECORDING_OSD_MAX_LINE_BYTES];
} VAEG_RECORDING_OSD_LAYER;

typedef struct {
	uint64_t frame_token;
	uint32_t guest_width;
	uint32_t guest_height;
	uint32_t output_width;
	uint32_t output_height;
	uint32_t layer_count;
	const VAEG_RECORDING_OSD_LAYER_INPUT *layers;
} VAEG_RECORDING_OSD_SNAPSHOT_INPUT;

typedef struct {
	uint32_t version;
	uint64_t frame_token;
	uint32_t guest_width;
	uint32_t guest_height;
	uint32_t output_width;
	uint32_t output_height;
	uint32_t layer_count;
	VAEG_RECORDING_OSD_LAYER layers[VAEG_RECORDING_OSD_MAX_LAYERS];
} VAEG_RECORDING_OSD_SNAPSHOT;

VAEG_RECORDING_OSD_ERROR vaeg_recording_osd_snapshot_build(
	const VAEG_RECORDING_OSD_SNAPSHOT_INPUT *input,
	VAEG_RECORDING_OSD_SNAPSHOT *output);
const VAEG_RECORDING_OSD_LAYER *vaeg_recording_osd_snapshot_find(
	const VAEG_RECORDING_OSD_SNAPSHOT *snapshot, uint32_t kind);
const char *vaeg_recording_osd_error_name(VAEG_RECORDING_OSD_ERROR error);

#ifdef __cplusplus
}
#endif

#endif
