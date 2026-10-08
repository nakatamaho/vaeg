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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef VAEG_RECORDING_H264_ENCODER_HPP
#define VAEG_RECORDING_H264_ENCODER_HPP
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "recording/recording_ffmpeg_video_encoder.hpp"

namespace vaeg::recording {
enum class H264Error {
	None, InvalidConfig, UnsupportedSize, ProviderNotBuilt, ProviderUnavailable,
	OptionFailure, AllocationFailure, InvalidFrame, InflightLimit, EncodeFailure,
	TimestampFailure, DrainFailure, AlreadyDrained
};
const char *h264_error_name(H264Error error) noexcept;
struct H264Config {
	std::uint32_t width = 0, height = 0;
	std::int64_t bitrate = 8000000;
};
H264Error validate_h264_config(const H264Config &config) noexcept;
struct H264Info {
	std::string provider;
	std::string device;
	std::string hardware_evidence;
	std::uint32_t width = 0, height = 0;
	std::int64_t requested_bitrate = 0, actual_bitrate = 0;
	std::vector<std::uint8_t> extradata;
};
/* Worker-only direct libavcodec adapter; no processes, CPU fallback, guest
 * synthesis or SDL rendering. NV12 is BT.709 limited-range SDR, even extent;
 * odd extents fail explicitly until verified padding/crop is implemented.
 * PTS and DTS are retained independently, in 1/60 second units. */
class H264Encoder {
  public:
	static H264Error create(const H264Config &config, std::unique_ptr<H264Encoder> *encoder,
	                        std::string *detail) noexcept;
	~H264Encoder();
	H264Error encode_nv12(const std::uint8_t *y, std::uint32_t y_stride,
	                      const std::uint8_t *uv, std::uint32_t uv_stride,
	                      std::uint64_t slot, std::vector<EncodedVideoPacket> *packets) noexcept;
	H264Error drain(std::vector<EncodedVideoPacket> *packets) noexcept;
	const H264Info &info() const noexcept;
	H264Encoder(const H264Encoder &) = delete;
	H264Encoder &operator=(const H264Encoder &) = delete;
  private:
	class Impl;
	explicit H264Encoder(std::unique_ptr<Impl> impl);
	std::unique_ptr<Impl> impl_;
};
} // namespace vaeg::recording
#endif
