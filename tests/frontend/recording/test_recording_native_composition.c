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
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "recording/recording_native_composition.h"

static uint16_t pack565(uint8_t red, uint8_t green, uint8_t blue) {
	return (uint16_t)(((uint16_t)(red >> 3) << 11) | ((uint16_t)(green >> 2) << 5) |
	                  (uint16_t)(blue >> 3));
}

static void put565(uint8_t *row, uint32_t x, uint16_t value) {
	row[x * 2u] = (uint8_t)value;
	row[x * 2u + 1u] = (uint8_t)(value >> 8);
}

static void init_layer(VAEG_RECORDING_OSD_LAYER *layer, uint32_t kind,
                       const char *const *lines, uint32_t count, uint8_t enabled,
                       uint8_t background_alpha, uint8_t text_alpha) {
	memset(layer, 0, sizeof(*layer));
	layer->kind = kind;
	layer->coordinate_space = VAEG_RECORDING_OSD_COORDINATE_OUTPUT;
	layer->anchor = VAEG_RECORDING_OSD_ANCHOR_TOP_RIGHT;
	layer->enabled = enabled;
	layer->visible = enabled;
	layer->scale = 1;
	layer->background_alpha = background_alpha;
	layer->text_alpha = text_alpha;
	layer->line_count = count;
	if (count != 0u) {
		uint32_t line;
		for (line = 0; line < count; ++line) {
			strncpy(layer->lines[line], lines[line], VAEG_RECORDING_OSD_MAX_LINE_BYTES - 1u);
			layer->lines[line][VAEG_RECORDING_OSD_MAX_LINE_BYTES - 1u] = '\0';
		}
	}
}

static void build_snapshot(VAEG_RECORDING_OSD_SNAPSHOT *snapshot,
                           uint8_t video_enabled, uint8_t framebuffer_enabled) {
	const char *video_lines[] = {"A"};
	const char *framebuffer_lines[] = {"B"};
	memset(snapshot, 0, sizeof(*snapshot));
	snapshot->version = VAEG_RECORDING_OSD_SNAPSHOT_VERSION;
	snapshot->frame_token = 91u;
	snapshot->guest_width = 24u;
	snapshot->guest_height = 32u;
	snapshot->output_width = 24u;
	snapshot->output_height = 32u;
	snapshot->layer_count = VAEG_RECORDING_OSD_MAX_LAYERS;
	init_layer(&snapshot->layers[0], VAEG_RECORDING_OSD_VIDEO_INFO, video_lines, 1,
	           video_enabled, 128u, 255u);
	init_layer(&snapshot->layers[1], VAEG_RECORDING_OSD_FRAMEBUFFER_INFO, framebuffer_lines, 1,
	           framebuffer_enabled, 0u, 255u);
}

int main(void) {
	uint8_t source_storage[32u * 52u + 2u];
	uint8_t glyphs[256u * 8u];
	VAEG_RECORDING_NATIVE_RGB565_SOURCE source;
	VAEG_RECORDING_NATIVE_FRAME frame;
	VAEG_RECORDING_NATIVE_FRAME baseline;
	VAEG_RECORDING_NATIVE_FRAME invalid_frame;
	VAEG_RECORDING_OSD_SNAPSHOT snapshot;
	uint32_t y;

	memset(source_storage, 0xa5, sizeof(source_storage));
	for (y = 0; y < 32u; ++y) {
		uint8_t *row = source_storage + 2u + y * 52u;
		uint32_t x;
		for (x = 0; x < 24u; ++x) {
			put565(row, x, pack565(100u, 120u, 140u));
		}
		put565(row, 0u, pack565(255u, 0u, 0u));
		put565(row, 1u, pack565(0u, 0u, 255u));
		put565(row, 23u, pack565(0u, 255u, 0u));
	}
	memset(&source, 0, sizeof(source));
	source.pixels = source_storage + 2u;
	source.width = 24u;
	source.height = 32u;
	source.pitch_bytes = 52u;
	source.source_size_bytes = sizeof(source_storage) - 2u;
	vaeg_recording_native_frame_initialize(&frame);
	vaeg_recording_native_frame_initialize(&baseline);
	vaeg_recording_native_frame_initialize(&invalid_frame);
	assert(vaeg_recording_native_frame_copy_rgb565(&source, &frame) ==
	       VAEG_RECORDING_NATIVE_FRAME_OK);
	assert(frame.width == 24u && frame.height == 32u && frame.pitch_bytes == 72u);
	assert(frame.sample_aspect_width == 1u && frame.sample_aspect_height == 1u);
	assert(frame.pixels[0] == 255u && frame.pixels[1] == 0u && frame.pixels[2] == 0u);
	assert(frame.pixels[3] == 0u && frame.pixels[4] == 0u && frame.pixels[5] == 255u);
	assert(frame.pixels[69] == 0u && frame.pixels[70] == 255u && frame.pixels[71] == 0u);
	assert(frame.pixels[frame.size_bytes - 1u] != 0xa5u);

	/* The copied frame is independent of both source pixels and source guards. */
	source_storage[2] = 0;
	assert(frame.pixels[0] == 255u);
	put565(source_storage + 2u, 0u, pack565(255u, 0u, 0u));
	assert(vaeg_recording_native_frame_copy_rgb565(&source, &baseline) ==
	       VAEG_RECORDING_NATIVE_FRAME_OK);
	build_snapshot(&snapshot, 0u, 0u);
	memset(glyphs, 0, sizeof(glyphs));
	assert(vaeg_recording_native_frame_apply_osd(&baseline, &snapshot, glyphs) ==
	       VAEG_RECORDING_NATIVE_FRAME_OK);
	assert(memcmp(frame.pixels, baseline.pixels, frame.size_bytes) == 0);

	build_snapshot(&snapshot, 1u, 1u);
	snapshot.layers[1].background_alpha = 64u;
	glyphs['A' * 8u] = 0x80u;
	glyphs['B' * 8u] = 0x80u;
	assert(vaeg_recording_native_frame_apply_osd(&frame, &snapshot, glyphs) ==
	       VAEG_RECORDING_NATIVE_FRAME_OK);
	assert(frame.frame_token == 91u);
	/* The framebuffer panel extends below the 32-pixel frame and is clipped. */
	assert(memcmp(frame.pixels, baseline.pixels, frame.size_bytes) != 0);
	assert(frame.pixels[(6u * frame.pitch_bytes) + (12u * 3u)] == 255u);
	assert(frame.pixels[(6u * frame.pitch_bytes) + (12u * 3u) + 1u] == 255u);
	assert(frame.pixels[(6u * frame.pitch_bytes) + (12u * 3u) + 2u] == 192u);
	assert(frame.pixels[(26u * frame.pitch_bytes) + (12u * 3u)] == 255u);
	/* Background alpha is applied once, and the second panel is clipped at the frame edge. */
	assert(frame.pixels[(2u * frame.pitch_bytes) + (8u * 3u)] == 49u);
	assert(frame.pixels[(2u * frame.pitch_bytes) + (8u * 3u) + 1u] == 60u);
	assert(frame.pixels[(2u * frame.pitch_bytes) + (8u * 3u) + 2u] == 69u);
	assert(frame.pixels[(31u * frame.pitch_bytes) + (8u * 3u)] !=
	       baseline.pixels[(31u * frame.pitch_bytes) + (8u * 3u)]);

	/* Geometry and source extent errors fail before an allocation or read. */
	source.pitch_bytes = 47u;
	assert(vaeg_recording_native_frame_copy_rgb565(&source, &invalid_frame) ==
	       VAEG_RECORDING_NATIVE_FRAME_SHORT_PITCH);
	source.pitch_bytes = 52u;
	source.source_size_bytes -= 5u;
	assert(vaeg_recording_native_frame_copy_rgb565(&source, &invalid_frame) ==
	       VAEG_RECORDING_NATIVE_FRAME_SOURCE_SIZE_OVERFLOW);
	source.source_size_bytes += 5u;
	assert(vaeg_recording_native_frame_copy_rgb565(&source, &frame) ==
	       VAEG_RECORDING_NATIVE_FRAME_OUTPUT_BUSY);
	snapshot.guest_width = 23u;
	assert(vaeg_recording_native_frame_apply_osd(&frame, &snapshot, glyphs) ==
	       VAEG_RECORDING_NATIVE_FRAME_INVALID_SNAPSHOT);

	vaeg_recording_native_frame_destroy(&frame);
	vaeg_recording_native_frame_destroy(&baseline);
	vaeg_recording_native_frame_destroy(&invalid_frame);
	assert(vaeg_recording_native_frame_copy_rgb565(NULL, &frame) ==
	       VAEG_RECORDING_NATIVE_FRAME_NULL_SOURCE);
	return 0;
}
