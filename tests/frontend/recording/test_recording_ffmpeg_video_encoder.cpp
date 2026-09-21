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
#include <cstdint>
#include <cstring>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/error.h>
#include <libavutil/mem.h>
}

#include "recording/recording_ffmpeg_video_encoder.hpp"

using vaeg::recording::EncodedVideoPacket;
using vaeg::recording::FfmpegVideoEncoder;
using vaeg::recording::FfmpegVideoEncoderError;
using vaeg::recording::VideoEncoderConfig;
using vaeg::recording::VideoEncoderFrame;

namespace {

struct DecodedFrame {
	std::int64_t pts;
	std::vector<std::uint8_t> rgb;
};

struct Decoder {
	AVCodecContext *context = nullptr;
	AVFrame *frame = nullptr;

	~Decoder() {
		av_frame_free(&frame);
		avcodec_free_context(&context);
	}
};

std::uint8_t expand5(std::uint32_t value) {
	return static_cast<std::uint8_t>((value * 255u) / 31u);
}

std::uint8_t expand6(std::uint32_t value) {
	return static_cast<std::uint8_t>((value * 255u) / 63u);
}

struct Fixture {
	VAEG_RECORDING_VIDEO_DESCRIPTOR descriptor;
	std::vector<std::uint8_t> bytes;
	std::vector<std::uint8_t> expected_rgb;
};

Fixture make_rgb24_fixture() {
	constexpr std::uint32_t width = 7u;
	constexpr std::uint32_t height = 5u;
	constexpr std::uint64_t stride = width * 3u + 5u;
	Fixture fixture{{width, height, stride, VAEG_RECORDING_PIXEL_RGB24},
	                std::vector<std::uint8_t>(stride * height, 0xa5u),
	                std::vector<std::uint8_t>(width * height * 3u)};
	std::uint32_t random = 0x13579bdfu;
	for (std::uint32_t y = 0; y < height; ++y) {
		for (std::uint32_t x = 0; x < width; ++x) {
			random = random * 1664525u + 1013904223u;
			const std::size_t output = (static_cast<std::size_t>(y) * width + x) * 3u;
			std::uint8_t red = static_cast<std::uint8_t>(random >> 24);
			std::uint8_t green = static_cast<std::uint8_t>(random >> 16);
			std::uint8_t blue = static_cast<std::uint8_t>(random >> 8);
			if (x == 0u && y == 0u) {
				red = 0u;
				green = 0u;
				blue = 0u;
			} else if (x == 1u && y == 0u) {
				red = 255u;
				green = 0u;
				blue = 0u;
			} else if (x == 2u && y == 0u) {
				red = 0u;
				green = 255u;
				blue = 0u;
			} else if (x == 3u && y == 0u) {
				red = 0u;
				green = 0u;
				blue = 255u;
			} else if (x == 4u && y == 0u) {
				red = 255u;
				green = 255u;
				blue = 255u;
			}
			fixture.bytes[static_cast<std::size_t>(y) * stride + x * 3u] = red;
			fixture.bytes[static_cast<std::size_t>(y) * stride + x * 3u + 1u] = green;
			fixture.bytes[static_cast<std::size_t>(y) * stride + x * 3u + 2u] = blue;
			fixture.expected_rgb[output] = red;
			fixture.expected_rgb[output + 1u] = green;
			fixture.expected_rgb[output + 2u] = blue;
		}
	}
	return fixture;
}

Fixture make_rgb565_fixture() {
	constexpr std::uint32_t width = 3u;
	constexpr std::uint32_t height = 2u;
	constexpr std::uint64_t stride = width * 2u + 4u;
	constexpr std::uint16_t values[] = {0xf800u, 0x07e0u, 0x001fu, 0xffffu, 0x0000u, 0x8410u};
	Fixture fixture{{width, height, stride, VAEG_RECORDING_PIXEL_RGB565},
	                std::vector<std::uint8_t>(stride * height, 0x5au),
	                std::vector<std::uint8_t>(width * height * 3u)};
	for (std::uint32_t y = 0; y < height; ++y) {
		for (std::uint32_t x = 0; x < width; ++x) {
			const std::uint16_t value = values[y * width + x];
			const std::size_t source = static_cast<std::size_t>(y) * stride + x * 2u;
			fixture.bytes[source] = static_cast<std::uint8_t>(value & 0xffu);
			fixture.bytes[source + 1u] = static_cast<std::uint8_t>(value >> 8);
			const std::size_t output = (static_cast<std::size_t>(y) * width + x) * 3u;
			fixture.expected_rgb[output] = expand5((value >> 11) & 0x1fu);
			fixture.expected_rgb[output + 1u] = expand6((value >> 5) & 0x3fu);
			fixture.expected_rgb[output + 2u] = expand5(value & 0x1fu);
		}
	}
	return fixture;
}

std::vector<DecodedFrame> decode_packets(const std::vector<EncodedVideoPacket> &packets,
                                         const vaeg::recording::VideoEncoderInfo &info) {
	const std::uint32_t width = info.width;
	const std::uint32_t height = info.height;
	const AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_FFV1);
	assert(codec != nullptr);
	Decoder decoder;
	decoder.context = avcodec_alloc_context3(codec);
	decoder.frame = av_frame_alloc();
	assert(decoder.context != nullptr && decoder.frame != nullptr);
	decoder.context->width = static_cast<int>(width);
	decoder.context->height = static_cast<int>(height);
	decoder.context->thread_count = 1;
	if (!info.codec_extradata.empty()) {
		decoder.context->extradata = static_cast<std::uint8_t *>(
		    av_mallocz(info.codec_extradata.size() + AV_INPUT_BUFFER_PADDING_SIZE));
		assert(decoder.context->extradata != nullptr);
		std::memcpy(decoder.context->extradata, info.codec_extradata.data(),
		            info.codec_extradata.size());
		decoder.context->extradata_size = static_cast<int>(info.codec_extradata.size());
	}
	const int open_result = avcodec_open2(decoder.context, codec, nullptr);
	assert(open_result >= 0);

	std::vector<DecodedFrame> decoded;
	for (const auto &encoded : packets) {
		AVPacket packet{};
		packet.data = const_cast<std::uint8_t *>(encoded.data.data());
		packet.size = static_cast<int>(encoded.data.size());
		packet.pts = encoded.pts;
		packet.dts = encoded.dts;
		assert(avcodec_send_packet(decoder.context, &packet) >= 0);
		for (;;) {
			const int result = avcodec_receive_frame(decoder.context, decoder.frame);
			if (result == AVERROR(EAGAIN)) {
				break;
			}
			assert(result >= 0);
			assert(decoder.frame->format == AV_PIX_FMT_BGR0);
			DecodedFrame output{decoder.frame->pts, std::vector<std::uint8_t>(width * height * 3u)};
			for (std::uint32_t y = 0; y < height; ++y) {
				const auto *source = decoder.frame->data[0] +
				                     static_cast<std::size_t>(y) * decoder.frame->linesize[0];
				for (std::uint32_t x = 0; x < width; ++x) {
					const auto *pixel = source + x * 4u;
					const std::size_t destination = (static_cast<std::size_t>(y) * width + x) * 3u;
					output.rgb[destination] = pixel[2];
					output.rgb[destination + 1u] = pixel[1];
					output.rgb[destination + 2u] = pixel[0];
				}
			}
			decoded.push_back(std::move(output));
		}
	}
	assert(avcodec_send_packet(decoder.context, nullptr) >= 0);
	for (;;) {
		const int result = avcodec_receive_frame(decoder.context, decoder.frame);
		if (result == AVERROR_EOF) {
			break;
		}
		assert(result == AVERROR(EAGAIN));
	}
	return decoded;
}

void check_round_trip(const Fixture &fixture) {
	const VideoEncoderConfig config{fixture.descriptor.width, fixture.descriptor.height, 1, 60};
	const auto create = FfmpegVideoEncoder::create(config);
	assert(create);
	assert(create.encoder->info().gop_size == 1u);
	assert(create.encoder->info().bgr0);
	assert(create.encoder->info().ffv1_version3);
	assert(create.encoder->info().slice_crc);

	std::vector<EncodedVideoPacket> packets;
	for (std::uint64_t index = 0; index < 3u; ++index) {
		VideoEncoderFrame input{fixture.descriptor, fixture.bytes.data(), fixture.bytes.size(),
		                        100u + index, 200 + static_cast<std::int64_t>(index)};
		assert(create.encoder->encode(input, &packets) == FfmpegVideoEncoderError::None);
	}
	assert(create.encoder->flush(&packets) == FfmpegVideoEncoderError::None);
	assert(create.encoder->flush(&packets) == FfmpegVideoEncoderError::AlreadyFlushed);
	assert(packets.size() == 3u);
	for (std::size_t index = 0; index < packets.size(); ++index) {
		assert(packets[index].sequence == 100u + index);
		assert(packets[index].pts == 200 + static_cast<std::int64_t>(index));
		assert(packets[index].keyframe);
		assert(!packets[index].data.empty());
	}
	const auto decoded = decode_packets(packets, create.encoder->info());
	assert(decoded.size() == 3u);
	for (const auto &frame : decoded) {
		assert(frame.rgb == fixture.expected_rgb);
	}
}

void check_invalid_inputs() {
	const VideoEncoderConfig config{3u, 2u, 1, 60};
	const auto create = FfmpegVideoEncoder::create(config);
	assert(create);
	const Fixture fixture = make_rgb565_fixture();
	VideoEncoderFrame input{fixture.descriptor, fixture.bytes.data(), fixture.bytes.size(), 1u, 1};
	input.descriptor.width = 4u;
	std::vector<EncodedVideoPacket> packets;
	assert(create.encoder->encode(input, &packets) == FfmpegVideoEncoderError::InvalidInput);
}

void check_failure_paths() {
	const auto make_config = [](VideoEncoderConfig::TestFault fault) {
		VideoEncoderConfig config{3u, 2u, 1, 60};
		config.test_fault = fault;
		return config;
	};
	assert(
	    FfmpegVideoEncoder::create(make_config(VideoEncoderConfig::TestFault::Allocation)).error ==
	    FfmpegVideoEncoderError::AllocationFailure);
	assert(FfmpegVideoEncoder::create(make_config(VideoEncoderConfig::TestFault::Open)).error ==
	       FfmpegVideoEncoderError::OpenFailure);
	const Fixture fixture = make_rgb565_fixture();
	std::vector<EncodedVideoPacket> packets;
	{
		const auto create =
		    FfmpegVideoEncoder::create(make_config(VideoEncoderConfig::TestFault::Send));
		assert(create);
		VideoEncoderFrame input{fixture.descriptor, fixture.bytes.data(), fixture.bytes.size(), 1u,
		                        1};
		assert(create.encoder->encode(input, &packets) == FfmpegVideoEncoderError::SendFailure);
	}
	{
		const auto create =
		    FfmpegVideoEncoder::create(make_config(VideoEncoderConfig::TestFault::Receive));
		assert(create);
		VideoEncoderFrame input{fixture.descriptor, fixture.bytes.data(), fixture.bytes.size(), 1u,
		                        1};
		assert(create.encoder->encode(input, &packets) == FfmpegVideoEncoderError::ReceiveFailure);
	}
	{
		const auto create =
		    FfmpegVideoEncoder::create(make_config(VideoEncoderConfig::TestFault::Flush));
		assert(create);
		assert(create.encoder->flush(&packets) == FfmpegVideoEncoderError::FlushFailure);
	}
}

} // namespace

int main() {
	assert(FfmpegVideoEncoder::create({0u, 2u, 1, 60}).error ==
	       FfmpegVideoEncoderError::InvalidConfig);
	check_round_trip(make_rgb24_fixture());
	check_round_trip(make_rgb565_fixture());
	check_invalid_inputs();
	check_failure_paths();
	assert(vaeg::recording::ffmpeg_video_encoder_error_name(
	           FfmpegVideoEncoderError::ReceiveFailure) != nullptr);
	return 0;
}
