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
#include <assert.h>
#include <string.h>

#include "sound/sound_capture.h"

static void test_fifo_accounting(void) {
	SINT16 storage[8];
	SINT16 output[8];
	const SINT32 source_a[] = {
		-40000, 40000, -32768, 32767, 123, -456, 777, -888};
	const SINT32 source_b[] = {1000, -1000, 2000, -2000};
	VAEG_SOUND_CAPTURE_FIFO fifo;
	UINT64 sample_begin;
	UINT32 epoch;

	assert(vaeg_sound_capture_fifo_init(&fifo, storage, 4u, 100u, 7u) ==
	       VAEG_SOUND_CAPTURE_OK);
	assert(vaeg_sound_capture_fifo_can_append(&fifo, 4u));
	assert(vaeg_sound_capture_fifo_append_s32(&fifo, source_a, 2u, FALSE) ==
	       VAEG_SOUND_CAPTURE_OK);
	assert(vaeg_sound_capture_fifo_pending(&fifo) == 2u);
	assert(vaeg_sound_capture_fifo_read(&fifo, output, 1u, &sample_begin, &epoch) == 1u);
	assert(sample_begin == 100u && epoch == 7u);
	assert(output[0] == -32768 && output[1] == 32767);
	memset(output, 0xa5, sizeof(output));
	assert(vaeg_sound_capture_fifo_append_s32(&fifo, source_b, 2u, TRUE) ==
	       VAEG_SOUND_CAPTURE_OK);
	assert(vaeg_sound_capture_fifo_pending(&fifo) == 3u);
	assert(vaeg_sound_capture_fifo_read(&fifo, output, 3u, &sample_begin, &epoch) == 3u);
	assert(sample_begin == 101u && epoch == 7u);
	assert(output[0] == -32768 && output[1] == 32767);
	assert(output[2] == 0 && output[3] == 0 && output[4] == 0 && output[5] == 0);
	assert(output[6] == (SINT16)0xa5a5 && output[7] == (SINT16)0xa5a5);
	assert(vaeg_sound_capture_fifo_pending(&fifo) == 0u);
	assert(vaeg_sound_capture_fifo_append_s32(&fifo, source_b, 2u, FALSE) ==
	       VAEG_SOUND_CAPTURE_OK);
	assert(vaeg_sound_capture_fifo_read(&fifo, output, 2u, &sample_begin, &epoch) == 2u);
	assert(sample_begin == 104u);
	assert(output[0] == 1000 && output[1] == -1000);
	assert(output[2] == 2000 && output[3] == -2000);
}

static void test_capacity_is_atomic(void) {
	SINT16 storage[4];
	SINT16 output[4] = {11, 22, 33, 44};
	const SINT32 source[] = {1, 2, 3, 4, 5, 6};
	VAEG_SOUND_CAPTURE_FIFO fifo;

	assert(vaeg_sound_capture_fifo_init(&fifo, storage, 2u, 0u, 1u) ==
	       VAEG_SOUND_CAPTURE_OK);
	assert(vaeg_sound_capture_fifo_append_s32(&fifo, source, 3u, FALSE) ==
	       VAEG_SOUND_CAPTURE_CAPACITY);
	assert(vaeg_sound_capture_fifo_pending(&fifo) == 0u);
	assert(vaeg_sound_capture_fifo_read(&fifo, output, 2u, NULL, NULL) == 0u);
	assert(output[0] == 11 && output[1] == 22 && output[2] == 33 && output[3] == 44);
	assert(!vaeg_sound_capture_fifo_can_append(&fifo, UINT_MAX));
}

int main(void) {
	test_fifo_accounting();
	test_capacity_is_atomic();
	return 0;
}
