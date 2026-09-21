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
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <cassert>

#include "recording/recording_ffmpeg_capabilities.hpp"

using vaeg::recording::FfmpegCapabilityError;
using vaeg::recording::FfmpegCapabilityInventory;

static FfmpegCapabilityInventory all_capabilities() {
	return {true, true, true, true, true, true, true, true, true};
}

static void test_distinct_missing_capabilities() {
	FfmpegCapabilityInventory inventory = all_capabilities();
	inventory.ffv1_encoder = false;
	assert(vaeg::recording::evaluate_ffmpeg_capabilities(inventory) ==
	       FfmpegCapabilityError::MissingFfv1Encoder);
	inventory = all_capabilities();
	inventory.pcm_s16le_encoder = false;
	assert(vaeg::recording::evaluate_ffmpeg_capabilities(inventory) ==
	       FfmpegCapabilityError::MissingPcmS16leEncoder);
	inventory = all_capabilities();
	inventory.matroska_muxer = false;
	assert(vaeg::recording::evaluate_ffmpeg_capabilities(inventory) ==
	       FfmpegCapabilityError::MissingMatroskaMuxer);
	inventory = all_capabilities();
	inventory.ffv1_bgr0 = false;
	assert(vaeg::recording::evaluate_ffmpeg_capabilities(inventory) ==
	       FfmpegCapabilityError::MissingFfv1PixelFormat);
	inventory = all_capabilities();
	inventory.pcm_s16 = false;
	assert(vaeg::recording::evaluate_ffmpeg_capabilities(inventory) ==
	       FfmpegCapabilityError::MissingPcmSampleFormat);
	inventory = all_capabilities();
	inventory.ffv1_slice_crc = false;
	assert(vaeg::recording::evaluate_ffmpeg_capabilities(inventory) ==
	       FfmpegCapabilityError::MissingFfv1SliceCrc);
	inventory = all_capabilities();
	inventory.ffv1_version3 = false;
	assert(vaeg::recording::evaluate_ffmpeg_capabilities(inventory) ==
	       FfmpegCapabilityError::MissingFfv1Version3);
	inventory = all_capabilities();
	inventory.matroska_accepts_ffv1 = false;
	assert(vaeg::recording::evaluate_ffmpeg_capabilities(inventory) ==
	       FfmpegCapabilityError::MuxerRejectsFfv1);
	inventory = all_capabilities();
	inventory.matroska_accepts_pcm_s16le = false;
	assert(vaeg::recording::evaluate_ffmpeg_capabilities(inventory) ==
	       FfmpegCapabilityError::MuxerRejectsPcmS16le);
	assert(vaeg::recording::ffmpeg_capability_error_name(
	           FfmpegCapabilityError::MissingFfv1Encoder) != nullptr);
}

static void test_actual_development_libraries() {
	const auto result = vaeg::recording::probe_ffmpeg_capabilities();
	assert(result.error == FfmpegCapabilityError::None);
	assert(result.capabilities.avutil.major >= 58u);
	assert(result.capabilities.avcodec.major >= 60u);
	assert(result.capabilities.avformat.major >= 60u);
	assert(result.capabilities.inventory.ffv1_encoder);
	assert(result.capabilities.inventory.pcm_s16le_encoder);
	assert(result.capabilities.inventory.matroska_muxer);
	assert(result.capabilities.inventory.ffv1_bgr0);
	assert(result.capabilities.inventory.pcm_s16);
	assert(result.capabilities.inventory.ffv1_slice_crc);
	assert(result.capabilities.inventory.ffv1_version3);
	assert(result.capabilities.inventory.matroska_accepts_ffv1);
	assert(result.capabilities.inventory.matroska_accepts_pcm_s16le);
}

int main() {
	test_distinct_missing_capabilities();
	test_actual_development_libraries();
	return 0;
}
