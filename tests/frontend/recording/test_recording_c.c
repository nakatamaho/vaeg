/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer.
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
#include <stdint.h>

#include "recording/recording.h"

int main(void) {
	VAEG_RECORDING_VIDEO_DESCRIPTOR video = {640u, 400u, 1280u, VAEG_RECORDING_PIXEL_RGB565};
	VAEG_RECORDING_AUDIO_DESCRIPTOR audio = {22050u, 2u, 16u, VAEG_RECORDING_AUDIO_PCM_S16LE};
	VAEG_RECORDING_CAPABILITIES capabilities;
	VAEG_RECORDING_STATUS status;

	assert(vaeg_recording_validate_source(VAEG_RECORDING_SOURCE_NATIVE) == VAEG_RECORDING_ERROR_OK);
	assert(vaeg_recording_validate_source(VAEG_RECORDING_SOURCE_DISPLAYED) ==
	       VAEG_RECORDING_ERROR_OK);
	assert(vaeg_recording_validate_source(2u) == VAEG_RECORDING_ERROR_INVALID_SOURCE);
	assert(vaeg_recording_validate_video_descriptor(&video) == VAEG_RECORDING_ERROR_OK);
	assert(vaeg_recording_validate_video_descriptor(NULL) == VAEG_RECORDING_ERROR_NULL_DESCRIPTOR);

	video.width = 0u;
	assert(vaeg_recording_validate_video_descriptor(&video) ==
	       VAEG_RECORDING_ERROR_INVALID_DIMENSIONS);
	video.width = 640u;
	video.stride_bytes = 1279u;
	assert(vaeg_recording_validate_video_descriptor(&video) == VAEG_RECORDING_ERROR_INVALID_STRIDE);
	video.stride_bytes = 1280u;
	video.pixel_format = 99u;
	assert(vaeg_recording_validate_video_descriptor(&video) ==
	       VAEG_RECORDING_ERROR_INVALID_PIXEL_FORMAT);
	video.pixel_format = VAEG_RECORDING_PIXEL_RGB565;
	video.width = UINT32_MAX;
	video.height = UINT32_MAX;
	video.stride_bytes = UINT64_MAX;
	assert(vaeg_recording_validate_video_descriptor(&video) == VAEG_RECORDING_ERROR_SIZE_OVERFLOW);

	assert(vaeg_recording_validate_audio_descriptor(&audio) == VAEG_RECORDING_ERROR_OK);
	audio.sample_rate_hz = 0u;
	assert(vaeg_recording_validate_audio_descriptor(&audio) ==
	       VAEG_RECORDING_ERROR_INVALID_SAMPLE_RATE);
	audio.sample_rate_hz = 22050u;
	audio.channels = 1u;
	assert(vaeg_recording_validate_audio_descriptor(&audio) ==
	       VAEG_RECORDING_ERROR_INVALID_CHANNELS);
	audio.channels = 2u;
	audio.sample_bits = 8u;
	assert(vaeg_recording_validate_audio_descriptor(&audio) ==
	       VAEG_RECORDING_ERROR_INVALID_SAMPLE_BITS);
	audio.sample_bits = 16u;
	audio.format = 99u;
	assert(vaeg_recording_validate_audio_descriptor(&audio) ==
	       VAEG_RECORDING_ERROR_INVALID_AUDIO_FORMAT);
	assert(vaeg_recording_validate_copy(NULL, 1u) == VAEG_RECORDING_ERROR_NULL_PIXELS);
	assert(vaeg_recording_error_name(VAEG_RECORDING_ERROR_INVALID_SOURCE) != NULL);

	vaeg_recording_get_capabilities(&capabilities);
	assert(capabilities.build_enabled == 0u);
	assert(capabilities.backend_available == 0u);
	assert(capabilities.source_count == 2u);
	assert(capabilities.contract_version == VAEG_RECORDING_CONTRACT_VERSION);
	vaeg_recording_get_status(&status);
	assert(status.state == VAEG_RECORDING_STATE_DISABLED);
	assert(status.source == VAEG_RECORDING_SOURCE_NATIVE);
	assert(status.last_error == VAEG_RECORDING_ERROR_DISABLED);
	return 0;
}
