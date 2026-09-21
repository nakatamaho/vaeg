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
#ifndef VAEG_SDL2_RECORDING_FFMPEG_CAPABILITIES_HPP
#define VAEG_SDL2_RECORDING_FFMPEG_CAPABILITIES_HPP

#include <cstdint>

namespace vaeg::recording {

enum class FfmpegCapabilityError {
	None = 0,
	UnsupportedVersion,
	MissingFfv1Encoder,
	MissingPcmS16leEncoder,
	MissingMatroskaMuxer,
	MissingFfv1PixelFormat,
	MissingPcmSampleFormat,
	MissingFfv1SliceCrc,
	MissingFfv1Version3,
	MuxerRejectsFfv1,
	MuxerRejectsPcmS16le,
	ProbeFailure,
};

struct FfmpegVersion {
	std::uint32_t major;
	std::uint32_t minor;
	std::uint32_t micro;
};

struct FfmpegCapabilityInventory {
	bool ffv1_encoder;
	bool pcm_s16le_encoder;
	bool matroska_muxer;
	bool ffv1_bgr0;
	bool pcm_s16;
	bool ffv1_slice_crc;
	bool ffv1_version3;
	bool matroska_accepts_ffv1;
	bool matroska_accepts_pcm_s16le;
};

struct FfmpegCapabilities {
	FfmpegVersion avutil;
	FfmpegVersion avcodec;
	FfmpegVersion avformat;
	FfmpegCapabilityInventory inventory;
};

struct FfmpegProbeResult {
	FfmpegCapabilityError error;
	FfmpegCapabilities capabilities;
};

const char *ffmpeg_capability_error_name(FfmpegCapabilityError error) noexcept;
FfmpegCapabilityError
evaluate_ffmpeg_capabilities(const FfmpegCapabilityInventory &inventory) noexcept;
FfmpegProbeResult probe_ffmpeg_capabilities() noexcept;

} // namespace vaeg::recording

#endif
