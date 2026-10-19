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
#include <assert.h>
#include <string.h>

#include "recording/recording_osd_snapshot.h"

static void initialize_layer(VAEG_RECORDING_OSD_LAYER_INPUT *layer, uint32_t kind,
                             const char *const *lines, uint32_t line_count) {
	memset(layer, 0, sizeof(*layer));
	layer->kind = kind;
	layer->coordinate_space = VAEG_RECORDING_OSD_COORDINATE_OUTPUT;
	layer->anchor = VAEG_RECORDING_OSD_ANCHOR_TOP_RIGHT;
	layer->x = 500;
	layer->y = 22;
	layer->width = 140;
	layer->height = 80;
	layer->clip_x = 0;
	layer->clip_y = 0;
	layer->clip_width = 640;
	layer->clip_height = 400;
	layer->enabled = 1;
	layer->visible = 1;
	layer->scale = 2;
	layer->background_alpha = 190;
	layer->text_alpha = 255;
	layer->line_count = line_count;
	layer->lines = lines;
}

int main(void) {
	const char *video_lines[] = {"TEXT 640x400 4bpp", "G0 ON 640x400 4bpp"};
	const char *framebuffer_lines[] = {"FB0 source 640x400", "FB0 view 640x400"};
	VAEG_RECORDING_OSD_LAYER_INPUT layers[2];
	VAEG_RECORDING_OSD_SNAPSHOT_INPUT input;
	VAEG_RECORDING_OSD_SNAPSHOT snapshot;
	const VAEG_RECORDING_OSD_LAYER *video;
	const VAEG_RECORDING_OSD_LAYER *framebuffer;

	initialize_layer(&layers[0], VAEG_RECORDING_OSD_VIDEO_INFO, video_lines, 2);
	initialize_layer(&layers[1], VAEG_RECORDING_OSD_FRAMEBUFFER_INFO, framebuffer_lines, 2);
	input.frame_token = 77u;
	input.guest_width = 640u;
	input.guest_height = 400u;
	input.output_width = 1280u;
	input.output_height = 822u;
	input.layer_count = 2u;
	input.layers = layers;
	assert(vaeg_recording_osd_snapshot_build(&input, &snapshot) ==
	       VAEG_RECORDING_OSD_ERROR_OK);
	assert(snapshot.version == VAEG_RECORDING_OSD_SNAPSHOT_VERSION);
	assert(snapshot.frame_token == 77u);
	video = vaeg_recording_osd_snapshot_find(&snapshot, VAEG_RECORDING_OSD_VIDEO_INFO);
	framebuffer = vaeg_recording_osd_snapshot_find(&snapshot, VAEG_RECORDING_OSD_FRAMEBUFFER_INFO);
	assert(video != NULL && framebuffer != NULL);
	assert(strcmp(video->lines[0], "TEXT 640x400 4bpp") == 0);
	assert(video->background_alpha == 190u && video->text_alpha == 255u);
	assert(video->anchor == VAEG_RECORDING_OSD_ANCHOR_TOP_RIGHT);
	assert(video->clip_width == 640 && video->clip_height == 400);

	/* The output owns copied strings, not the source arrays. */
	video_lines[0] = "mutated after build";
	framebuffer_lines[0] = "destroyed source";
	assert(strcmp(video->lines[0], "TEXT 640x400 4bpp") == 0);
	assert(strcmp(framebuffer->lines[0], "FB0 source 640x400") == 0);

	/* Disabled and clipped layers remain represented, but are not visible. */
	layers[0].enabled = 0;
	layers[0].visible = 0;
	layers[0].line_count = 0;
	layers[0].lines = NULL;
	layers[1].visible = 0;
	layers[1].background_alpha = 0;
	assert(vaeg_recording_osd_snapshot_build(&input, &snapshot) ==
	       VAEG_RECORDING_OSD_ERROR_OK);
	video = vaeg_recording_osd_snapshot_find(&snapshot, VAEG_RECORDING_OSD_VIDEO_INFO);
	framebuffer = vaeg_recording_osd_snapshot_find(&snapshot, VAEG_RECORDING_OSD_FRAMEBUFFER_INFO);
	assert(video != NULL && framebuffer != NULL);
	assert(video->enabled == 0u && video->visible == 0u && video->line_count == 0u);
	assert(framebuffer->enabled == 1u && framebuffer->visible == 0u &&
	       framebuffer->background_alpha == 0u);

	input.layer_count = VAEG_RECORDING_OSD_MAX_LAYERS + 1u;
	assert(vaeg_recording_osd_snapshot_build(&input, &snapshot) ==
	       VAEG_RECORDING_OSD_ERROR_TOO_MANY_LAYERS);
	input.layer_count = 2u;
	layers[0].kind = layers[1].kind;
	assert(vaeg_recording_osd_snapshot_build(&input, &snapshot) ==
	       VAEG_RECORDING_OSD_ERROR_DUPLICATE_LAYER);
	return 0;
}
