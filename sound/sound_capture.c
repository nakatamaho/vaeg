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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "sound_capture.h"

static SINT16 vaeg_sound_capture_s16(SINT32 sample) {
	if (sample > 32767) {
		return 32767;
	}
	if (sample < -32768) {
		return -32768;
	}
	return (SINT16)sample;
}

VAEG_SOUND_CAPTURE_RESULT vaeg_sound_capture_fifo_init(
	VAEG_SOUND_CAPTURE_FIFO *fifo, SINT16 *buffer, UINT capacity_frames,
	UINT64 first_sample, UINT32 epoch) {
	if (fifo == NULL || buffer == NULL || capacity_frames == 0u) {
		return VAEG_SOUND_CAPTURE_INVALID;
	}
	ZeroMemory(fifo, sizeof(*fifo));
	fifo->buffer = buffer;
	fifo->capacity_frames = capacity_frames;
	fifo->first_sample = first_sample;
	fifo->next_sample = first_sample;
	fifo->epoch = epoch;
	fifo->initialized = TRUE;
	return VAEG_SOUND_CAPTURE_OK;
}

BOOL vaeg_sound_capture_fifo_can_append(
	const VAEG_SOUND_CAPTURE_FIFO *fifo, UINT frames) {
	if (fifo == NULL || !fifo->initialized || frames > fifo->capacity_frames - fifo->pending_frames) {
		return FALSE;
	}
	if (UINT64_MAX - fifo->next_sample < frames ||
	    UINT64_MAX - fifo->produced_frames < frames) {
		return FALSE;
	}
	return TRUE;
}

VAEG_SOUND_CAPTURE_RESULT vaeg_sound_capture_fifo_append_s32(
	VAEG_SOUND_CAPTURE_FIFO *fifo, const SINT32 *source, UINT frames, BOOL muted) {
	UINT frame;
	UINT index;

	if (fifo == NULL || !fifo->initialized || (source == NULL && frames != 0u)) {
		return VAEG_SOUND_CAPTURE_INVALID;
	}
	if (!vaeg_sound_capture_fifo_can_append(fifo, frames)) {
		if (frames > fifo->capacity_frames - fifo->pending_frames) {
			return VAEG_SOUND_CAPTURE_CAPACITY;
		}
		return VAEG_SOUND_CAPTURE_OVERFLOW;
	}
	for (frame = 0; frame < frames; ++frame) {
		index = fifo->write_frame * 2u;
		if (muted) {
			fifo->buffer[index] = 0;
			fifo->buffer[index + 1u] = 0;
		} else {
			fifo->buffer[index] = vaeg_sound_capture_s16(source[frame * 2u]);
			fifo->buffer[index + 1u] = vaeg_sound_capture_s16(source[frame * 2u + 1u]);
		}
		fifo->write_frame++;
		if (fifo->write_frame == fifo->capacity_frames) {
			fifo->write_frame = 0;
		}
	}
	fifo->pending_frames += frames;
	fifo->next_sample += frames;
	fifo->produced_frames += frames;
	return VAEG_SOUND_CAPTURE_OK;
}

UINT vaeg_sound_capture_fifo_read(
	VAEG_SOUND_CAPTURE_FIFO *fifo, SINT16 *destination, UINT capacity_frames,
	UINT64 *sample_begin, UINT32 *epoch) {
	UINT count;
	UINT first;
	UINT second;
	UINT index;

	if (fifo == NULL || !fifo->initialized || destination == NULL || capacity_frames == 0u) {
		return 0u;
	}
	count = min(capacity_frames, fifo->pending_frames);
	if (sample_begin != NULL) {
		*sample_begin = fifo->first_sample + fifo->consumed_frames;
	}
	if (epoch != NULL) {
		*epoch = fifo->epoch;
	}
	first = min(count, fifo->capacity_frames - fifo->read_frame);
	CopyMemory(destination, fifo->buffer + fifo->read_frame * 2u,
	           first * 2u * sizeof(*destination));
	second = count - first;
	if (second != 0u) {
		CopyMemory(destination + first * 2u, fifo->buffer,
		           second * 2u * sizeof(*destination));
	}
	index = fifo->read_frame + count;
	if (index >= fifo->capacity_frames) {
		index -= fifo->capacity_frames;
	}
	fifo->read_frame = index;
	fifo->pending_frames -= count;
	fifo->consumed_frames += count;
	return count;
}

void vaeg_sound_capture_fifo_clear(VAEG_SOUND_CAPTURE_FIFO *fifo) {
	if (fifo == NULL) {
		return;
	}
	ZeroMemory(fifo, sizeof(*fifo));
}

UINT vaeg_sound_capture_fifo_pending(const VAEG_SOUND_CAPTURE_FIFO *fifo) {
	if (fifo == NULL || !fifo->initialized) {
		return 0u;
	}
	return fifo->pending_frames;
}
