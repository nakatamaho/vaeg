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
#ifndef VAEG_SOUND_GENERATION_H
#define VAEG_SOUND_GENERATION_H

#include "compiler.h"

#ifndef SOUNDCALL
#define SOUNDCALL
#endif

typedef void(SOUNDCALL *VAEG_SOUND_GENERATOR_CALLBACK)(void *hdl, SINT32 *pcm, UINT count);

typedef struct {
	void *hdl;
	VAEG_SOUND_GENERATOR_CALLBACK cbfn;
} VAEG_SOUND_GENERATOR_SLOT;

#ifdef __cplusplus
extern "C" {
#endif

/* Generate interleaved stereo S32 frames without allocating or taking locks. */
UINT vaeg_sound_generate_s32_stereo(
	SINT32 *pcm, UINT capacity_frames, UINT requested_frames,
	const VAEG_SOUND_GENERATOR_SLOT *slots, UINT slot_count);

/* Return the exact legacy look-ahead fill count for a reserved stream tail. */
UINT vaeg_sound_lookahead_frames(UINT remain_frames, UINT reserve_frames);

/* Saturate S32 scalar samples to S16, or clear the requested bytes if source is absent. */
UINT vaeg_sound_pack_s16_or_silence(
	SINT16 *destination, UINT destination_bytes, const SINT32 *source, UINT source_samples);

#ifdef __cplusplus
}
#endif

#endif
