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
#ifndef VAEG_SOUND_CAPTURE_H
#define VAEG_SOUND_CAPTURE_H

#include "compiler.h"

typedef enum {
	VAEG_SOUND_CAPTURE_OK = 0,
	VAEG_SOUND_CAPTURE_INVALID,
	VAEG_SOUND_CAPTURE_CAPACITY,
	VAEG_SOUND_CAPTURE_OVERFLOW
} VAEG_SOUND_CAPTURE_RESULT;

typedef struct {
	SINT16 *buffer;
	UINT capacity_frames;
	UINT read_frame;
	UINT write_frame;
	UINT pending_frames;
	UINT64 first_sample;
	UINT64 next_sample;
	UINT64 produced_frames;
	UINT64 consumed_frames;
	UINT32 epoch;
	BOOL initialized;
} VAEG_SOUND_CAPTURE_FIFO;

VAEG_SOUND_CAPTURE_RESULT vaeg_sound_capture_fifo_init(
	VAEG_SOUND_CAPTURE_FIFO *fifo, SINT16 *buffer, UINT capacity_frames,
	UINT64 first_sample, UINT32 epoch);

BOOL vaeg_sound_capture_fifo_can_append(
	const VAEG_SOUND_CAPTURE_FIFO *fifo, UINT frames);

VAEG_SOUND_CAPTURE_RESULT vaeg_sound_capture_fifo_append_s32(
	VAEG_SOUND_CAPTURE_FIFO *fifo, const SINT32 *source, UINT frames, BOOL muted);

UINT vaeg_sound_capture_fifo_read(
	VAEG_SOUND_CAPTURE_FIFO *fifo, SINT16 *destination, UINT capacity_frames,
	UINT64 *sample_begin, UINT32 *epoch);

void vaeg_sound_capture_fifo_clear(VAEG_SOUND_CAPTURE_FIFO *fifo);
UINT vaeg_sound_capture_fifo_pending(const VAEG_SOUND_CAPTURE_FIFO *fifo);

#endif
