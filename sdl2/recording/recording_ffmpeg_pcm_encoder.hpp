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
#ifndef VAEG_SDL2_RECORDING_FFMPEG_PCM_ENCODER_HPP
#define VAEG_SDL2_RECORDING_FFMPEG_PCM_ENCODER_HPP

#include <cstdint>
#include <memory>
#include <vector>

namespace vaeg::recording {

enum class FfmpegPcmEncoderError {
	None = 0,
	InvalidConfig,
	InvalidInput,
	SizeOverflow,
	UnsupportedSampleFormat,
	CodecUnavailable,
	AllocationFailure,
	OpenFailure,
	FrameFailure,
	SendFailure,
	SendWouldBlock,
	ReceiveFailure,
	PacketTimestampMissing,
	PacketTimestampMismatch,
	PacketCopyFailure,
	SampleIndexMismatch,
	FlushFailure,
	AlreadyFlushed,
};

const char *ffmpeg_pcm_encoder_error_name(FfmpegPcmEncoderError error) noexcept;

struct PcmEncoderConfig {
	std::uint32_t sample_rate_hz;
	std::uint64_t origin_offset_numerator;
	std::uint64_t origin_offset_denominator;
#ifdef VAEG_RECORDING_FFMPEG_TESTING
	enum class TestFault {
		None = 0,
		Allocation,
		Open,
		Send,
		Receive,
		Flush,
	};
	TestFault test_fault = TestFault::None;
#endif
};

struct PcmSampleBlock {
	const std::int32_t *interleaved_s32;
	std::uint64_t sample_frames;
	std::uint64_t bytes;
	std::uint64_t first_sample_index;
};

struct EncodedPcmPacket {
	std::uint64_t first_sample_index;
	std::int64_t pts;
	std::int64_t dts;
	std::int64_t duration;
	std::vector<std::uint8_t> data;
};

struct PcmEncoderInfo {
	std::uint32_t sample_rate_hz;
	std::uint16_t channels;
	std::uint16_t sample_bits;
	bool s16le;
	std::uint64_t origin_offset_numerator;
	std::uint64_t origin_offset_denominator;
};

class FfmpegPcmEncoder;

struct PcmEncoderCreateResult {
	FfmpegPcmEncoderError error;
	std::unique_ptr<FfmpegPcmEncoder> encoder;

	explicit operator bool() const noexcept {
		return error == FfmpegPcmEncoderError::None && encoder != nullptr;
	}
};

class FfmpegPcmEncoder {
  public:
	static PcmEncoderCreateResult create(const PcmEncoderConfig &config) noexcept;
	~FfmpegPcmEncoder();

	FfmpegPcmEncoderError encode(const PcmSampleBlock &block,
	                             std::vector<EncodedPcmPacket> *packets) noexcept;
	FfmpegPcmEncoderError flush(std::vector<EncodedPcmPacket> *packets) noexcept;
	const PcmEncoderInfo &info() const noexcept;

	FfmpegPcmEncoder(const FfmpegPcmEncoder &) = delete;
	FfmpegPcmEncoder &operator=(const FfmpegPcmEncoder &) = delete;

  private:
	class Impl;

	explicit FfmpegPcmEncoder(std::unique_ptr<Impl> impl) noexcept;
	std::unique_ptr<Impl> impl_;
};

} // namespace vaeg::recording

#endif
