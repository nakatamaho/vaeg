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
#include "recording/recording_native_composition.h"

#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static uint8_t expand5(uint32_t value) {
	return (uint8_t)((value * 255U) / 31U);
}

static uint8_t expand6(uint32_t value) {
	return (uint8_t)((value * 255U) / 63U);
}

static uint8_t alpha_blend(uint8_t destination, uint8_t source, uint8_t alpha) {
	return (uint8_t)(((uint32_t)destination * (255U - alpha) +
	                  (uint32_t)source * alpha + 127U) /
	                 255U);
}

static VAEG_RECORDING_NATIVE_FRAME_ERROR validate_source(
	const VAEG_RECORDING_NATIVE_RGB565_SOURCE *source, uint32_t *output_pitch,
	uint64_t *output_size) {
	uint64_t row_end;
	uint64_t size;

	if (source == NULL) {
		return VAEG_RECORDING_NATIVE_FRAME_NULL_SOURCE;
	}
	if (source->pixels == NULL) {
		return VAEG_RECORDING_NATIVE_FRAME_NULL_SOURCE;
	}
	if ((source->width == 0u) || (source->height == 0u)) {
		return VAEG_RECORDING_NATIVE_FRAME_ZERO_EXTENT;
	}
	if ((uint64_t)source->width * 2u > UINT32_MAX ||
	    source->pitch_bytes < source->width * 2u) {
		return VAEG_RECORDING_NATIVE_FRAME_SHORT_PITCH;
	}
	row_end = (uint64_t)(source->height - 1u) * source->pitch_bytes;
	if (row_end > UINT64_MAX - (uint64_t)source->width * 2u) {
		return VAEG_RECORDING_NATIVE_FRAME_SOURCE_SIZE_OVERFLOW;
	}
	row_end += (uint64_t)source->width * 2u;
	if (source->source_size_bytes < row_end) {
		return VAEG_RECORDING_NATIVE_FRAME_SOURCE_SIZE_OVERFLOW;
	}
	if ((uint64_t)source->width * 3u > UINT32_MAX) {
		return VAEG_RECORDING_NATIVE_FRAME_SIZE_OVERFLOW;
	}
	*output_pitch = source->width * 3u;
	if ((uint64_t)source->height > UINT64_MAX / *output_pitch) {
		return VAEG_RECORDING_NATIVE_FRAME_SIZE_OVERFLOW;
	}
	size = (uint64_t)source->height * *output_pitch;
	if (size > SIZE_MAX) {
		return VAEG_RECORDING_NATIVE_FRAME_SIZE_OVERFLOW;
	}
	*output_size = size;
	if (size > VAEG_RECORDING_NATIVE_FRAME_MAX_BYTES) {
		return VAEG_RECORDING_NATIVE_FRAME_TOO_LARGE;
	}
	return VAEG_RECORDING_NATIVE_FRAME_OK;
}

void vaeg_recording_native_frame_initialize(VAEG_RECORDING_NATIVE_FRAME *frame) {
	if (frame != NULL) {
		memset(frame, 0, sizeof(*frame));
	}
}

void vaeg_recording_native_frame_destroy(VAEG_RECORDING_NATIVE_FRAME *frame) {
	if (frame == NULL) {
		return;
	}
	free(frame->pixels);
	memset(frame, 0, sizeof(*frame));
}

VAEG_RECORDING_NATIVE_FRAME_ERROR vaeg_recording_native_frame_copy_rgb565(
	const VAEG_RECORDING_NATIVE_RGB565_SOURCE *source, VAEG_RECORDING_NATIVE_FRAME *frame) {
	uint32_t output_pitch;
	uint64_t output_size;
	VAEG_RECORDING_NATIVE_FRAME_ERROR error;
	uint32_t y;

	if (frame == NULL) {
		return VAEG_RECORDING_NATIVE_FRAME_NULL_OUTPUT;
	}
	if (frame->pixels != NULL) {
		return VAEG_RECORDING_NATIVE_FRAME_OUTPUT_BUSY;
	}
	error = validate_source(source, &output_pitch, &output_size);
	if (error != VAEG_RECORDING_NATIVE_FRAME_OK) {
		return error;
	}
	frame->pixels = (uint8_t *)malloc((size_t)output_size);
	if (frame->pixels == NULL) {
		return VAEG_RECORDING_NATIVE_FRAME_TOO_LARGE;
	}
	frame->width = source->width;
	frame->height = source->height;
	frame->pitch_bytes = output_pitch;
	frame->size_bytes = output_size;
	frame->sample_aspect_width = 1u;
	frame->sample_aspect_height = 1u;
	for (y = 0; y < source->height; ++y) {
		const uint8_t *source_row = source->pixels + (size_t)y * source->pitch_bytes;
		uint8_t *destination_row = frame->pixels + (size_t)y * output_pitch;
		uint32_t x;
		for (x = 0; x < source->width; ++x) {
			const uint32_t value = source_row[x * 2u] |
			                       ((uint32_t)source_row[x * 2u + 1u] << 8);
			destination_row[x * 3u] = expand5((value >> 11) & 0x1fu);
			destination_row[x * 3u + 1u] = expand6((value >> 5) & 0x3fu);
			destination_row[x * 3u + 2u] = expand5(value & 0x1fu);
		}
	}
	return VAEG_RECORDING_NATIVE_FRAME_OK;
}

static const VAEG_RECORDING_OSD_LAYER *find_layer(
	const VAEG_RECORDING_OSD_SNAPSHOT *snapshot, uint32_t kind) {
	return vaeg_recording_osd_snapshot_find(snapshot, kind);
}

static int bounded_line_length(const char *text) {
	size_t length;

	if (text == NULL) {
		return -1;
	}
	for (length = 0; length < VAEG_RECORDING_OSD_MAX_LINE_BYTES; ++length) {
		if (text[length] == '\0') {
			return (int)length;
		}
	}
	return -1;
}

static int valid_frame(const VAEG_RECORDING_NATIVE_FRAME *frame) {
	uint64_t minimum_pitch;
	uint64_t required_size;

	if ((frame == NULL) || (frame->pixels == NULL) || (frame->width == 0u) ||
	    (frame->height == 0u)) {
		return 0;
	}
	minimum_pitch = (uint64_t)frame->width * 3u;
	if ((minimum_pitch > UINT32_MAX) || ((uint64_t)frame->pitch_bytes < minimum_pitch)) {
		return 0;
	}
	if ((uint64_t)frame->height > UINT64_MAX / frame->pitch_bytes) {
		return 0;
	}
	required_size = (uint64_t)frame->height * frame->pitch_bytes;
	return (frame->size_bytes >= required_size) &&
	       (frame->size_bytes <= VAEG_RECORDING_NATIVE_FRAME_MAX_BYTES);
}

VAEG_RECORDING_NATIVE_FRAME_ERROR vaeg_recording_native_frame_rotate_right(
	VAEG_RECORDING_NATIVE_FRAME *frame) {
	uint8_t *rotated;
	uint32_t x;
	uint32_t y;
	uint32_t old_width;
	uint32_t aspect;
	if (!valid_frame(frame)) {
		return VAEG_RECORDING_NATIVE_FRAME_NULL_SOURCE;
	}
	rotated = (uint8_t *)malloc((size_t)frame->width * frame->height * 3u);
	if (rotated == NULL) {
		return VAEG_RECORDING_NATIVE_FRAME_TOO_LARGE;
	}
	for (y = 0; y < frame->height; ++y) {
		for (x = 0; x < frame->width; ++x) {
			memcpy(rotated + ((size_t)x * frame->height + frame->height - 1u - y) * 3u,
			       frame->pixels + (size_t)y * frame->pitch_bytes + x * 3u, 3u);
		}
	}
	free(frame->pixels);
	frame->pixels = rotated;
	old_width = frame->width;
	frame->width = frame->height;
	frame->height = old_width;
	frame->pitch_bytes = frame->width * 3u;
	frame->size_bytes = (uint64_t)frame->pitch_bytes * frame->height;
	aspect = frame->sample_aspect_width;
	frame->sample_aspect_width = frame->sample_aspect_height;
	frame->sample_aspect_height = aspect;
	return VAEG_RECORDING_NATIVE_FRAME_OK;
}

static int valid_snapshot(const VAEG_RECORDING_NATIVE_FRAME *frame,
	const VAEG_RECORDING_OSD_SNAPSHOT *snapshot) {
	uint32_t layer_no;

	if ((snapshot == NULL) || (snapshot->version != VAEG_RECORDING_OSD_SNAPSHOT_VERSION) ||
	    (snapshot->layer_count > VAEG_RECORDING_OSD_MAX_LAYERS) ||
	    (snapshot->guest_width == 0u) || (snapshot->guest_height == 0u) ||
	    (snapshot->output_width == 0u) || (snapshot->output_height == 0u) ||
	    (frame->width != snapshot->guest_width) ||
	    (frame->height != snapshot->guest_height)) {
		return 0;
	}
	for (layer_no = 0; layer_no < snapshot->layer_count; ++layer_no) {
		const VAEG_RECORDING_OSD_LAYER *layer = &snapshot->layers[layer_no];
		uint32_t line;

		if ((layer->kind >= VAEG_RECORDING_OSD_LAYER_COUNT) ||
		    (layer->coordinate_space != VAEG_RECORDING_OSD_COORDINATE_OUTPUT) ||
		    (layer->anchor != VAEG_RECORDING_OSD_ANCHOR_TOP_RIGHT) ||
		    (layer->enabled > 1u) || (layer->visible > 1u) || (layer->scale == 0u) ||
		    (layer->line_count > VAEG_RECORDING_OSD_MAX_LINES) || (layer->width < 0) ||
		    (layer->height < 0)) {
			return 0;
		}
		for (line = 0; line < layer->line_count; ++line) {
			if (bounded_line_length(layer->lines[line]) < 0) {
				return 0;
			}
		}
	}
	return 1;
}

static int layer_width(const VAEG_RECORDING_OSD_LAYER *layer) {
	int width = 0;
	uint32_t line;

	for (line = 0; line < layer->line_count; ++line) {
		const int length = bounded_line_length(layer->lines[line]);
		if (length > width) {
			width = length;
		}
	}
	return width * 8 + 8;
}

static int layer_height(const VAEG_RECORDING_OSD_LAYER *layer) {
	return (int)layer->line_count * 8 + 8;
}

static void blend_pixel(VAEG_RECORDING_NATIVE_FRAME *frame, int x, int y, uint8_t red,
	uint8_t green, uint8_t blue, uint8_t alpha) {
	uint8_t *pixel;

	if ((x < 0) || (y < 0) || ((uint32_t)x >= frame->width) ||
	    ((uint32_t)y >= frame->height) || (alpha == 0u)) {
		return;
	}
	pixel = frame->pixels + (size_t)y * frame->pitch_bytes + (size_t)x * 3u;
	pixel[0] = alpha_blend(pixel[0], red, alpha);
	pixel[1] = alpha_blend(pixel[1], green, alpha);
	pixel[2] = alpha_blend(pixel[2], blue, alpha);
}

static void draw_layer(VAEG_RECORDING_NATIVE_FRAME *frame,
	const VAEG_RECORDING_OSD_LAYER *layer, const uint8_t *glyphs, int y) {
	const int width = layer_width(layer);
	const int height = layer_height(layer);
	const int x = (int)frame->width - width;
	int row;
	int column;
	uint32_t line;

	for (row = 0; row < height; ++row) {
		for (column = 0; column < width; ++column) {
			blend_pixel(frame, x + column, y + row, 0, 0, 0, layer->background_alpha);
		}
	}
	for (line = 0; line < layer->line_count; ++line) {
		const unsigned char *text = (const unsigned char *)layer->lines[line];
		int text_x = x + 4;
		const int text_y = y + 4 + (int)line * 8;
		while (*text != '\0') {
			const uint8_t *glyph = glyphs + (size_t)(*text) * 8u;
			for (row = 0; row < 8; ++row) {
				for (column = 0; column < 8; ++column) {
					if ((glyph[row] & (uint8_t)(0x80u >> column)) != 0u) {
						blend_pixel(frame, text_x + column, text_y + row, 255, 255, 192,
						            layer->text_alpha);
					}
				}
			}
			text_x += 8;
			++text;
		}
	}
}

VAEG_RECORDING_NATIVE_FRAME_ERROR vaeg_recording_native_frame_apply_osd(
	VAEG_RECORDING_NATIVE_FRAME *frame, const VAEG_RECORDING_OSD_SNAPSHOT *snapshot,
	const uint8_t *glyphs_8x8) {
	const VAEG_RECORDING_OSD_LAYER *video;
	const VAEG_RECORDING_OSD_LAYER *framebuffer;
	int y = 2;

	if (!valid_frame(frame)) {
		return VAEG_RECORDING_NATIVE_FRAME_NULL_OUTPUT;
	}
	if (glyphs_8x8 == NULL) {
		return VAEG_RECORDING_NATIVE_FRAME_NULL_GLYPHS;
	}
	if (!valid_snapshot(frame, snapshot)) {
		return VAEG_RECORDING_NATIVE_FRAME_INVALID_SNAPSHOT;
	}
	video = find_layer(snapshot, VAEG_RECORDING_OSD_VIDEO_INFO);
	framebuffer = find_layer(snapshot, VAEG_RECORDING_OSD_FRAMEBUFFER_INFO);
	if ((video != NULL) && video->enabled && (video->line_count != 0u)) {
		if (video->anchor != VAEG_RECORDING_OSD_ANCHOR_TOP_RIGHT) {
			return VAEG_RECORDING_NATIVE_FRAME_INVALID_LAYER;
		}
		draw_layer(frame, video, glyphs_8x8, y);
		y += layer_height(video) + 4;
	}
	if ((framebuffer != NULL) && framebuffer->enabled && (framebuffer->line_count != 0u)) {
		if (framebuffer->anchor != VAEG_RECORDING_OSD_ANCHOR_TOP_RIGHT) {
			return VAEG_RECORDING_NATIVE_FRAME_INVALID_LAYER;
		}
		draw_layer(frame, framebuffer, glyphs_8x8, y);
	}
	frame->frame_token = snapshot->frame_token;
	return VAEG_RECORDING_NATIVE_FRAME_OK;
}

const char *vaeg_recording_native_frame_error_name(VAEG_RECORDING_NATIVE_FRAME_ERROR error) {
	switch (error) {
	case VAEG_RECORDING_NATIVE_FRAME_OK:
		return "ok";
	case VAEG_RECORDING_NATIVE_FRAME_NULL_SOURCE:
		return "null_source";
	case VAEG_RECORDING_NATIVE_FRAME_NULL_OUTPUT:
		return "null_output";
	case VAEG_RECORDING_NATIVE_FRAME_NULL_GLYPHS:
		return "null_glyphs";
	case VAEG_RECORDING_NATIVE_FRAME_ZERO_EXTENT:
		return "zero_extent";
	case VAEG_RECORDING_NATIVE_FRAME_SHORT_PITCH:
		return "short_pitch";
	case VAEG_RECORDING_NATIVE_FRAME_SOURCE_SIZE_OVERFLOW:
		return "source_size_overflow";
	case VAEG_RECORDING_NATIVE_FRAME_SIZE_OVERFLOW:
		return "size_overflow";
	case VAEG_RECORDING_NATIVE_FRAME_TOO_LARGE:
		return "too_large";
	case VAEG_RECORDING_NATIVE_FRAME_OUTPUT_BUSY:
		return "output_busy";
	case VAEG_RECORDING_NATIVE_FRAME_INVALID_SNAPSHOT:
		return "invalid_snapshot";
	case VAEG_RECORDING_NATIVE_FRAME_INVALID_LAYER:
		return "invalid_layer";
	default:
		return "unknown";
	}
}
