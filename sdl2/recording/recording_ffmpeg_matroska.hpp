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
#ifndef VAEG_SDL2_RECORDING_FFMPEG_MATROSKA_HPP
#define VAEG_SDL2_RECORDING_FFMPEG_MATROSKA_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "recording/recording_ffmpeg_pcm_encoder.hpp"
#include "recording/recording_ffmpeg_video_encoder.hpp"

namespace vaeg::recording {

enum class FfmpegMatroskaWriterError {
	None = 0,
	InvalidConfig,
	InvalidInput,
	PathFailure,
	TargetExists,
	AllocationFailure,
	MuxerUnavailable,
	StreamFailure,
	HeaderFailure,
	TimestampFailure,
	InterleaveFailure,
	WriteFailure,
	Incomplete,
	TrailerFailure,
	FlushFailure,
	CloseFailure,
	PublishFailure,
	AlreadyFinalized,
	Canceled,
};

const char *ffmpeg_matroska_writer_error_name(FfmpegMatroskaWriterError error) noexcept;

struct MatroskaWriterConfig {
	std::string final_path;
	std::int32_t video_time_base_num;
	std::int32_t video_time_base_den;
	std::int64_t video_epoch_pts = 0;
	std::uint64_t audio_epoch_sample_index = 0u;
	std::string source;
	std::string build_identity;
	bool osd_included = true;
	/* Required for H.264; empty archive defaults preserve existing metadata.
	 * Public-safe provider/device labels only, never private filesystem paths. */
	std::string encoder_provider;
	std::string encoder_device;
	std::string encoder_options;
#ifdef VAEG_RECORDING_FFMPEG_TESTING
	/* Test-only path injection exercises an existing partial-name collision. */
	std::string partial_path_override;
	enum class TestFault {
		None = 0,
		Create,
		Write,
		Seek,
		Header,
		Trailer,
		Flush,
		Close,
		Publish,
	};
	TestFault test_fault = TestFault::None;
#endif
};

struct MatroskaWriterInfo {
	std::uint32_t width;
	std::uint32_t height;
	std::uint32_t audio_sample_rate_hz;
	std::uint16_t audio_channels;
	std::uint16_t audio_sample_bits;
	std::int32_t video_stream_time_base_num;
	std::int32_t video_stream_time_base_den;
	std::int32_t audio_stream_time_base_num;
	std::int32_t audio_stream_time_base_den;
	std::uint64_t video_packets_written;
	std::uint64_t audio_packets_written;
	std::string final_path;
	std::string partial_path;
};

struct MatroskaWriterCreateResult {
	FfmpegMatroskaWriterError error;
	std::unique_ptr<class FfmpegMatroskaWriter> writer;
	std::string partial_path;

	explicit operator bool() const noexcept {
		return error == FfmpegMatroskaWriterError::None && writer != nullptr;
	}
};

class FfmpegMatroskaWriter {
  public:
	static MatroskaWriterCreateResult create(const MatroskaWriterConfig &config,
	                                         const VideoEncoderInfo &video,
	                                         const PcmEncoderInfo &audio) noexcept;
	~FfmpegMatroskaWriter();

	FfmpegMatroskaWriterError write_interval(
	    const std::vector<EncodedVideoPacket> &video_packets,
	    const std::vector<EncodedPcmPacket> &audio_packets) noexcept;
	FfmpegMatroskaWriterError finish() noexcept;
	FfmpegMatroskaWriterError cancel() noexcept;
	const MatroskaWriterInfo &info() const noexcept;

	FfmpegMatroskaWriter(const FfmpegMatroskaWriter &) = delete;
	FfmpegMatroskaWriter &operator=(const FfmpegMatroskaWriter &) = delete;

  private:
	class Impl;

	explicit FfmpegMatroskaWriter(std::unique_ptr<Impl> impl) noexcept;
	std::unique_ptr<Impl> impl_;
};

} // namespace vaeg::recording

#endif
