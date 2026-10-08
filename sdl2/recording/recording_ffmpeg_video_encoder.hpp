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
#ifndef VAEG_SDL2_RECORDING_FFMPEG_VIDEO_ENCODER_HPP
#define VAEG_SDL2_RECORDING_FFMPEG_VIDEO_ENCODER_HPP

#include <cstdint>
#include <memory>
#include <vector>

#include "recording/recording.h"

namespace vaeg::recording {

enum class FfmpegVideoEncoderError {
	None = 0,
	InvalidConfig,
	InvalidInput,
	UnsupportedPixelFormat,
	SizeOverflow,
	CodecUnavailable,
	AllocationFailure,
	OptionFailure,
	OpenFailure,
	FrameFailure,
	SendFailure,
	SendWouldBlock,
	ReceiveFailure,
	PacketTimestampMissing,
	PacketTimestampMismatch,
	PacketCopyFailure,
	FlushFailure,
	AlreadyFlushed,
};

const char *ffmpeg_video_encoder_error_name(FfmpegVideoEncoderError error) noexcept;

struct VideoEncoderConfig {
	std::uint32_t width;
	std::uint32_t height;
	std::int32_t time_base_num;
	std::int32_t time_base_den;
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

struct VideoEncoderFrame {
	VAEG_RECORDING_VIDEO_DESCRIPTOR descriptor;
	const void *pixels;
	std::uint64_t bytes;
	std::uint64_t sequence;
	std::int64_t pts;
};

struct EncodedVideoPacket {
	std::uint64_t sequence;
	std::int64_t pts;
	std::int64_t dts;
	std::int64_t duration;
	bool keyframe;
	std::vector<std::uint8_t> data;
};

enum class RecordingVideoCodec { Ffv1, H264 };
struct VideoEncoderInfo {
	std::uint32_t width;
	std::uint32_t height;
	std::uint32_t gop_size;
	bool bgr0;
	bool ffv1_version3;
	bool slice_crc;
	std::vector<std::uint8_t> codec_extradata;
	RecordingVideoCodec codec = RecordingVideoCodec::Ffv1;
};

class FfmpegVideoEncoder;

struct VideoEncoderCreateResult {
	FfmpegVideoEncoderError error;
	std::unique_ptr<FfmpegVideoEncoder> encoder;

	explicit operator bool() const noexcept {
		return error == FfmpegVideoEncoderError::None && encoder != nullptr;
	}
};

class FfmpegVideoEncoder {
  public:
	static VideoEncoderCreateResult create(const VideoEncoderConfig &config) noexcept;
	~FfmpegVideoEncoder();

	FfmpegVideoEncoderError encode(const VideoEncoderFrame &frame,
	                               std::vector<EncodedVideoPacket> *packets) noexcept;
	FfmpegVideoEncoderError flush(std::vector<EncodedVideoPacket> *packets) noexcept;
	const VideoEncoderInfo &info() const noexcept;

	FfmpegVideoEncoder(const FfmpegVideoEncoder &) = delete;
	FfmpegVideoEncoder &operator=(const FfmpegVideoEncoder &) = delete;

  private:
	class Impl;

	explicit FfmpegVideoEncoder(std::unique_ptr<Impl> impl) noexcept;
	std::unique_ptr<Impl> impl_;
};

} // namespace vaeg::recording

#endif
