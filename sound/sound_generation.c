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
#include "sound_generation.h"

#include "common/parts.h"

UINT vaeg_sound_generate_s32_stereo(
	SINT32 *pcm, UINT capacity_frames, UINT requested_frames,
	const VAEG_SOUND_GENERATOR_SLOT *slots, UINT slot_count) {
	UINT count;
	UINT slot;

	if ((pcm == NULL) || (capacity_frames > (UINT_MAX / 2u))) {
		return 0;
	}
	count = min(capacity_frames, requested_frames);
	if (count == 0u) {
		return 0;
	}
	ZeroMemory(pcm, count * 2u * sizeof(*pcm));
	if (slots == NULL) {
		return count;
	}
	for (slot = 0; slot < slot_count; ++slot) {
		if (slots[slot].cbfn != NULL) {
			slots[slot].cbfn(slots[slot].hdl, pcm, count);
		}
	}
	return count;
}

UINT vaeg_sound_lookahead_frames(UINT remain_frames, UINT reserve_frames) {
	if (remain_frames <= reserve_frames) {
		return 0;
	}
	return remain_frames - reserve_frames;
}

UINT vaeg_sound_pack_s16_or_silence(
	SINT16 *destination, UINT destination_bytes, const SINT32 *source, UINT source_samples) {
	UINT count;

	if (destination == NULL) {
		return 0;
	}
	if (source == NULL) {
		ZeroMemory(destination, destination_bytes);
		return destination_bytes;
	}
	count = destination_bytes / sizeof(*destination);
	count = min(count, source_samples);
	if (count == 0u) {
		return 0;
	}
	satuation_s16(destination, source, count * sizeof(*destination));
	return count * sizeof(*destination);
}
