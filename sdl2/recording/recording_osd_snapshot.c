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
#include "recording/recording_osd_snapshot.h"

#include <stddef.h>
#include <string.h>

static VAEG_RECORDING_OSD_ERROR validate_rectangle(const VAEG_RECORDING_OSD_LAYER_INPUT *layer) {
	int64_t right;
	int64_t bottom;
	int64_t clip_right;
	int64_t clip_bottom;

	if ((layer->width < 0) || (layer->height < 0) || (layer->clip_width < 0) ||
	    (layer->clip_height < 0)) {
		return VAEG_RECORDING_OSD_ERROR_INVALID_RECTANGLE;
	}
	right = (int64_t)layer->x + layer->width;
	bottom = (int64_t)layer->y + layer->height;
	clip_right = (int64_t)layer->clip_x + layer->clip_width;
	clip_bottom = (int64_t)layer->clip_y + layer->clip_height;
	if ((right > INT32_MAX) || (right < INT32_MIN) || (bottom > INT32_MAX) ||
	    (bottom < INT32_MIN) || (clip_right > INT32_MAX) || (clip_right < INT32_MIN) ||
	    (clip_bottom > INT32_MAX) || (clip_bottom < INT32_MIN)) {
		return VAEG_RECORDING_OSD_ERROR_INVALID_RECTANGLE;
	}
	return VAEG_RECORDING_OSD_ERROR_OK;
}

static VAEG_RECORDING_OSD_ERROR validate_layer(const VAEG_RECORDING_OSD_LAYER_INPUT *layer) {
	uint32_t line;

	if (layer->kind >= VAEG_RECORDING_OSD_LAYER_COUNT) {
		return VAEG_RECORDING_OSD_ERROR_INVALID_LAYER_KIND;
	}
	if (layer->coordinate_space != VAEG_RECORDING_OSD_COORDINATE_OUTPUT) {
		return VAEG_RECORDING_OSD_ERROR_INVALID_COORDINATE_SPACE;
	}
	if (layer->anchor != VAEG_RECORDING_OSD_ANCHOR_TOP_RIGHT) {
		return VAEG_RECORDING_OSD_ERROR_INVALID_ANCHOR;
	}
	if ((layer->enabled > 1u) || (layer->visible > 1u) || (layer->scale == 0u)) {
		return VAEG_RECORDING_OSD_ERROR_INVALID_FLAG;
	}
	if (validate_rectangle(layer) != VAEG_RECORDING_OSD_ERROR_OK) {
		return VAEG_RECORDING_OSD_ERROR_INVALID_RECTANGLE;
	}
	if (layer->line_count > VAEG_RECORDING_OSD_MAX_LINES) {
		return VAEG_RECORDING_OSD_ERROR_TOO_MANY_LINES;
	}
	if ((layer->line_count != 0u) && (layer->lines == NULL)) {
		return VAEG_RECORDING_OSD_ERROR_NULL_LINE;
	}
	for (line = 0; line < layer->line_count; ++line) {
		if (layer->lines[line] == NULL) {
			return VAEG_RECORDING_OSD_ERROR_NULL_LINE;
		}
		if (strlen(layer->lines[line]) >= VAEG_RECORDING_OSD_MAX_LINE_BYTES) {
			return VAEG_RECORDING_OSD_ERROR_LINE_TOO_LONG;
		}
	}
	return VAEG_RECORDING_OSD_ERROR_OK;
}

VAEG_RECORDING_OSD_ERROR vaeg_recording_osd_snapshot_build(
	const VAEG_RECORDING_OSD_SNAPSHOT_INPUT *input,
	VAEG_RECORDING_OSD_SNAPSHOT *output) {
	uint32_t layer_no;

	if (input == NULL) {
		return VAEG_RECORDING_OSD_ERROR_NULL_INPUT;
	}
	if (output == NULL) {
		return VAEG_RECORDING_OSD_ERROR_NULL_OUTPUT;
	}
	if ((input->guest_width == 0u) || (input->guest_height == 0u) ||
	    (input->output_width == 0u) || (input->output_height == 0u)) {
		return VAEG_RECORDING_OSD_ERROR_INVALID_DIMENSIONS;
	}
	if (input->layer_count > VAEG_RECORDING_OSD_MAX_LAYERS) {
		return VAEG_RECORDING_OSD_ERROR_TOO_MANY_LAYERS;
	}
	if ((input->layer_count != 0u) && (input->layers == NULL)) {
		return VAEG_RECORDING_OSD_ERROR_NULL_INPUT;
	}
	memset(output, 0, sizeof(*output));
	output->version = VAEG_RECORDING_OSD_SNAPSHOT_VERSION;
	output->frame_token = input->frame_token;
	output->guest_width = input->guest_width;
	output->guest_height = input->guest_height;
	output->output_width = input->output_width;
	output->output_height = input->output_height;
	output->layer_count = input->layer_count;
	for (layer_no = 0; layer_no < input->layer_count; ++layer_no) {
		const VAEG_RECORDING_OSD_LAYER_INPUT *source = &input->layers[layer_no];
		VAEG_RECORDING_OSD_LAYER *destination = &output->layers[layer_no];
		uint32_t line;
		VAEG_RECORDING_OSD_ERROR error = validate_layer(source);
		uint32_t previous;

		if (error != VAEG_RECORDING_OSD_ERROR_OK) {
			memset(output, 0, sizeof(*output));
			return error;
		}
		for (previous = 0; previous < layer_no; ++previous) {
			if (output->layers[previous].kind == source->kind) {
				memset(output, 0, sizeof(*output));
				return VAEG_RECORDING_OSD_ERROR_DUPLICATE_LAYER;
			}
		}
		destination->kind = source->kind;
		destination->coordinate_space = source->coordinate_space;
		destination->anchor = source->anchor;
		destination->x = source->x;
		destination->y = source->y;
		destination->width = source->width;
		destination->height = source->height;
		destination->clip_x = source->clip_x;
		destination->clip_y = source->clip_y;
		destination->clip_width = source->clip_width;
		destination->clip_height = source->clip_height;
		destination->enabled = source->enabled;
		destination->visible = source->visible;
		destination->scale = source->scale;
		destination->background_alpha = source->background_alpha;
		destination->text_alpha = source->text_alpha;
		destination->line_count = source->line_count;
		for (line = 0; line < source->line_count; ++line) {
			memcpy(destination->lines[line], source->lines[line],
			       strlen(source->lines[line]) + 1u);
		}
	}
	return VAEG_RECORDING_OSD_ERROR_OK;
}

const VAEG_RECORDING_OSD_LAYER *vaeg_recording_osd_snapshot_find(
	const VAEG_RECORDING_OSD_SNAPSHOT *snapshot, uint32_t kind) {
	uint32_t layer_no;

	if (snapshot == NULL) {
		return NULL;
	}
	for (layer_no = 0; layer_no < snapshot->layer_count; ++layer_no) {
		if (snapshot->layers[layer_no].kind == kind) {
			return &snapshot->layers[layer_no];
		}
	}
	return NULL;
}

const char *vaeg_recording_osd_error_name(VAEG_RECORDING_OSD_ERROR error) {
	switch (error) {
	case VAEG_RECORDING_OSD_ERROR_OK:
		return "ok";
	case VAEG_RECORDING_OSD_ERROR_NULL_INPUT:
		return "null_input";
	case VAEG_RECORDING_OSD_ERROR_NULL_OUTPUT:
		return "null_output";
	case VAEG_RECORDING_OSD_ERROR_INVALID_DIMENSIONS:
		return "invalid_dimensions";
	case VAEG_RECORDING_OSD_ERROR_TOO_MANY_LAYERS:
		return "too_many_layers";
	case VAEG_RECORDING_OSD_ERROR_INVALID_LAYER_KIND:
		return "invalid_layer_kind";
	case VAEG_RECORDING_OSD_ERROR_INVALID_COORDINATE_SPACE:
		return "invalid_coordinate_space";
	case VAEG_RECORDING_OSD_ERROR_INVALID_ANCHOR:
		return "invalid_anchor";
	case VAEG_RECORDING_OSD_ERROR_INVALID_FLAG:
		return "invalid_flag";
	case VAEG_RECORDING_OSD_ERROR_INVALID_RECTANGLE:
		return "invalid_rectangle";
	case VAEG_RECORDING_OSD_ERROR_TOO_MANY_LINES:
		return "too_many_lines";
	case VAEG_RECORDING_OSD_ERROR_NULL_LINE:
		return "null_line";
	case VAEG_RECORDING_OSD_ERROR_LINE_TOO_LONG:
		return "line_too_long";
	case VAEG_RECORDING_OSD_ERROR_DUPLICATE_LAYER:
		return "duplicate_layer";
	default:
		return "unknown";
	}
}
