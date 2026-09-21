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

#include "sound/sound_generation.h"
#include "common/parts.h"

typedef struct {
	char order[4];
	UINT order_count;
	UINT calls;
	UINT frames;
	SINT32 left;
	SINT32 right;
} GENERATOR_CONTEXT;

static void generator_a(void *hdl, SINT32 *pcm, UINT count) {
	GENERATOR_CONTEXT *context = (GENERATOR_CONTEXT *)hdl;
	UINT frame;

	context->order[context->order_count++] = 'A';
	context->calls++;
	context->frames = count;
	for (frame = 0; frame < count; ++frame) {
		pcm[frame * 2u] += context->left;
		pcm[frame * 2u + 1u] += context->right;
	}
}

static void generator_b(void *hdl, SINT32 *pcm, UINT count) {
	GENERATOR_CONTEXT *context = (GENERATOR_CONTEXT *)hdl;
	UINT frame;

	context->order[context->order_count++] = 'B';
	context->calls++;
	context->frames = count;
	for (frame = 0; frame < count; ++frame) {
		pcm[frame * 2u] += context->left / 10;
		pcm[frame * 2u + 1u] += context->right / 10;
	}
}

static UINT legacy_generate_s32_stereo(
	SINT32 *pcm, UINT capacity_frames, UINT requested_frames,
	const VAEG_SOUND_GENERATOR_SLOT *slots, UINT slot_count) {
	UINT count = min(capacity_frames, requested_frames);
	UINT slot;

	if (count == 0u) {
		return 0u;
	}
	ZeroMemory(pcm, count * 2u * sizeof(*pcm));
	for (slot = 0; slot < slot_count; ++slot) {
		slots[slot].cbfn(slots[slot].hdl, pcm, count);
	}
	return count;
}

static UINT legacy_pack_s16_or_silence(
	SINT16 *destination, UINT destination_bytes, const SINT32 *source) {
	if (source != NULL) {
		satuation_s16(destination, source, destination_bytes);
	} else {
		ZeroMemory(destination, destination_bytes);
	}
	return destination_bytes;
}

static void test_generation(void) {
	SINT32 pcm[8];
	SINT32 legacy_pcm[8];
	GENERATOR_CONTEXT context = {{0}, 0, 0, 0, 100, 200};
	GENERATOR_CONTEXT legacy_context = {{0}, 0, 0, 0, 100, 200};
	VAEG_SOUND_GENERATOR_SLOT slots[2];
	VAEG_SOUND_GENERATOR_SLOT legacy_slots[2];

	memset(pcm, 0xa5, sizeof(pcm));
	slots[0].hdl = &context;
	slots[0].cbfn = generator_a;
	slots[1].hdl = &context;
	slots[1].cbfn = generator_b;
	legacy_slots[0].hdl = &legacy_context;
	legacy_slots[0].cbfn = generator_a;
	legacy_slots[1].hdl = &legacy_context;
	legacy_slots[1].cbfn = generator_b;
	assert(vaeg_sound_generate_s32_stereo(pcm, 3u, 5u, slots, 2u) == 3u);
	memset(legacy_pcm, 0xa5, sizeof(legacy_pcm));
	assert(legacy_generate_s32_stereo(legacy_pcm, 3u, 5u, legacy_slots, 2u) == 3u);
	assert(memcmp(pcm, legacy_pcm, sizeof(pcm)) == 0);
	assert(context.calls == 2u && context.frames == 3u);
	assert(context.order_count == 2u && context.order[0] == 'A' && context.order[1] == 'B');
	assert(legacy_context.calls == context.calls &&
	       legacy_context.order[0] == context.order[0] &&
	       legacy_context.order[1] == context.order[1]);
	assert(pcm[0] == 110 && pcm[1] == 220 && pcm[4] == 110 && pcm[5] == 220);
	assert(pcm[6] == (SINT32)0xa5a5a5a5 && pcm[7] == (SINT32)0xa5a5a5a5);
	assert(vaeg_sound_generate_s32_stereo(pcm, 3u, 0u, slots, 2u) == 0u);
	assert(context.calls == 2u);
	assert(vaeg_sound_generate_s32_stereo(NULL, 3u, 1u, slots, 2u) == 0u);
	assert(vaeg_sound_generate_s32_stereo(NULL, UINT_MAX, 1u, NULL, 0u) == 0u);
}

static void test_reserved_lookahead(void) {
	assert(vaeg_sound_lookahead_frames(0u, 0u) == 0u);
	assert(vaeg_sound_lookahead_frames(8u, 8u) == 0u);
	assert(vaeg_sound_lookahead_frames(7u, 8u) == 0u);
	assert(vaeg_sound_lookahead_frames(20u, 8u) == 12u);
	assert(vaeg_sound_lookahead_frames(UINT_MAX, 1u) == UINT_MAX - 1u);
}

static void test_pack_and_no_source_silence(void) {
	const SINT32 source[] = {-40000, -32768, 32767, 40000, 123};
	SINT16 destination[5] = {111, 222, 333, 444, 555};
	SINT16 legacy_destination[5] = {111, 222, 333, 444, 555};
	SINT16 silence[4] = {111, 222, 333, 444};
	SINT16 legacy_silence[4] = {111, 222, 333, 444};

	assert(vaeg_sound_pack_s16_or_silence(destination, 7u, source, 5u) == 6u);
	assert(legacy_pack_s16_or_silence(legacy_destination, 7u, source) == 7u);
	assert(memcmp(destination, legacy_destination, sizeof(destination)) == 0);
	assert(destination[0] == -32768 && destination[1] == -32768);
	assert(destination[2] == 32767 && destination[3] == 444);
	assert(vaeg_sound_pack_s16_or_silence(silence, sizeof(silence), NULL, 0u) ==
	       sizeof(silence));
	assert(legacy_pack_s16_or_silence(legacy_silence, sizeof(legacy_silence), NULL) ==
	       sizeof(legacy_silence));
	assert(memcmp(silence, legacy_silence, sizeof(silence)) == 0);
	assert(silence[0] == 0 && silence[1] == 0 && silence[2] == 0 && silence[3] == 0);
	assert(vaeg_sound_pack_s16_or_silence(destination, sizeof(destination), source, 0u) == 0u);
	assert(vaeg_sound_pack_s16_or_silence(NULL, sizeof(destination), source, 5u) == 0u);
}

int main(void) {
	test_generation();
	test_reserved_lookahead();
	test_pack_and_no_source_silence();
	return 0;
}
