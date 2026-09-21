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
#include <limits>
#include <numeric>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
}

#include "recording/recording_ffmpeg_pcm_encoder.hpp"

using vaeg::recording::EncodedPcmPacket;
using vaeg::recording::FfmpegPcmEncoder;
using vaeg::recording::FfmpegPcmEncoderError;
using vaeg::recording::PcmEncoderConfig;
using vaeg::recording::PcmSampleBlock;

namespace {

struct Fixture {
	std::vector<std::int32_t> interleaved_s32;
	std::vector<std::uint8_t> expected_s16le;
};

std::int16_t saturate_s16(std::int32_t value) {
	if (value > 32767) {
		return 32767;
	}
	if (value < -32768) {
		return -32768;
	}
	return static_cast<std::int16_t>(value);
}

Fixture make_fixture() {
	constexpr std::uint32_t frames = 11u;
	constexpr std::int32_t values[frames][2] = {
	    {0, 0},          {1200, -1200},           {32767, -32768},
	    {32768, -32769}, {400000, -400000},       {-1, 1},
	    {16384, -16384}, {-32768, 32767},         {700, 0},
	    {0, -700},       {123456789, -123456789},
	};
	Fixture fixture{std::vector<std::int32_t>(frames * 2u), std::vector<std::uint8_t>(frames * 4u)};
	for (std::uint32_t frame = 0u; frame < frames; ++frame) {
		for (std::uint32_t channel = 0u; channel < 2u; ++channel) {
			const std::int32_t value = values[frame][channel];
			fixture.interleaved_s32[frame * 2u + channel] = value;
			const std::uint16_t packed = static_cast<std::uint16_t>(saturate_s16(value));
			const std::size_t output = static_cast<std::size_t>(frame) * 4u + channel * 2u;
			fixture.expected_s16le[output] = static_cast<std::uint8_t>(packed & 0xffu);
			fixture.expected_s16le[output + 1u] = static_cast<std::uint8_t>(packed >> 8);
		}
	}
	return fixture;
}

struct Decoder {
	AVCodecContext *context = nullptr;
	AVFrame *frame = nullptr;

	~Decoder() {
		av_frame_free(&frame);
		avcodec_free_context(&context);
	}
};

void collect_decoded_frame(Decoder *decoder, std::vector<std::uint8_t> *output) {
	assert(decoder != nullptr && output != nullptr);
	assert(decoder->frame->format == AV_SAMPLE_FMT_S16);
	assert(decoder->frame->ch_layout.nb_channels == 2);
	assert(decoder->frame->nb_samples >= 0);
	const std::size_t bytes = static_cast<std::size_t>(decoder->frame->nb_samples) * 4u;
	const auto *data = decoder->frame->data[0];
	assert(data != nullptr);
	output->insert(output->end(), data, data + bytes);
}

std::vector<std::uint8_t> decode_packets(const std::vector<EncodedPcmPacket> &packets,
                                         std::uint32_t sample_rate_hz) {
	const AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_PCM_S16LE);
	assert(codec != nullptr);
	Decoder decoder;
	decoder.context = avcodec_alloc_context3(codec);
	decoder.frame = av_frame_alloc();
	assert(decoder.context != nullptr && decoder.frame != nullptr);
	decoder.context->sample_rate = static_cast<int>(sample_rate_hz);
	av_channel_layout_default(&decoder.context->ch_layout, 2);
	assert(avcodec_open2(decoder.context, codec, nullptr) >= 0);

	std::vector<std::uint8_t> output;
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
			collect_decoded_frame(&decoder, &output);
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
	return output;
}

PcmEncoderConfig make_config() {
	return {22050u, 1u, 44100u};
}

std::vector<EncodedPcmPacket> encode_partition(const Fixture &fixture,
                                               const std::vector<std::uint64_t> &partition,
                                               std::uint64_t first_sample_index) {
	const auto create = FfmpegPcmEncoder::create(make_config());
	assert(create);
	assert(create.encoder->info().sample_rate_hz == 22050u);
	assert(create.encoder->info().channels == 2u);
	assert(create.encoder->info().sample_bits == 16u);
	assert(create.encoder->info().s16le);
	assert(create.encoder->info().origin_offset_numerator == 1u);
	assert(create.encoder->info().origin_offset_denominator == 44100u);

	std::vector<EncodedPcmPacket> packets;
	std::uint64_t offset = 0u;
	for (const std::uint64_t frames : partition) {
		assert(frames > 0u);
		const std::size_t scalar_offset = static_cast<std::size_t>(offset) * 2u;
		PcmSampleBlock block{fixture.interleaved_s32.data() + scalar_offset, frames,
		                     frames * 2u * sizeof(std::int32_t), first_sample_index + offset};
		assert(create.encoder->encode(block, &packets) == FfmpegPcmEncoderError::None);
		offset += frames;
	}
	assert(offset == fixture.interleaved_s32.size() / 2u);
	assert(create.encoder->flush(&packets) == FfmpegPcmEncoderError::None);
	assert(create.encoder->flush(&packets) == FfmpegPcmEncoderError::AlreadyFlushed);
	return packets;
}

void check_round_trip_and_partitioning() {
	const Fixture fixture = make_fixture();
	const std::vector<std::uint64_t> irregular = {3u, 1u, 4u, 1u, 2u};
	const auto irregular_packets = encode_partition(fixture, irregular, 41u);
	assert(irregular_packets.size() == irregular.size());
	std::uint64_t expected_index = 41u;
	std::uint64_t total_frames = 0u;
	for (std::size_t index = 0u; index < irregular_packets.size(); ++index) {
		assert(irregular_packets[index].first_sample_index == expected_index);
		assert(irregular_packets[index].pts == static_cast<std::int64_t>(expected_index));
		assert(irregular_packets[index].dts == static_cast<std::int64_t>(expected_index));
		assert(irregular_packets[index].duration == static_cast<std::int64_t>(irregular[index]));
		assert(!irregular_packets[index].data.empty());
		expected_index += irregular[index];
		total_frames += irregular[index];
	}
	assert(total_frames == 11u);
	assert(decode_packets(irregular_packets, 22050u) == fixture.expected_s16le);

	const auto whole_packets = encode_partition(fixture, {11u}, 41u);
	assert(whole_packets.size() == 1u);
	assert(whole_packets[0].first_sample_index == 41u);
	assert(whole_packets[0].duration == 11);
	assert(decode_packets(whole_packets, 22050u) == fixture.expected_s16le);
}

void check_invalid_inputs() {
	const Fixture fixture = make_fixture();
	const auto create = FfmpegPcmEncoder::create(make_config());
	assert(create);
	std::vector<EncodedPcmPacket> packets;
	PcmSampleBlock empty{fixture.interleaved_s32.data(), 0u, 0u, 0u};
	assert(create.encoder->encode(empty, &packets) == FfmpegPcmEncoderError::InvalidInput);
	PcmSampleBlock short_block{fixture.interleaved_s32.data(), 2u, sizeof(std::int32_t), 0u};
	assert(create.encoder->encode(short_block, &packets) == FfmpegPcmEncoderError::InvalidInput);
	PcmSampleBlock first{fixture.interleaved_s32.data(), 2u, 2u * 2u * sizeof(std::int32_t), 5u};
	assert(create.encoder->encode(first, &packets) == FfmpegPcmEncoderError::None);
	PcmSampleBlock gap{fixture.interleaved_s32.data() + 4u, 1u, 2u * sizeof(std::int32_t), 8u};
	assert(create.encoder->encode(gap, &packets) == FfmpegPcmEncoderError::SampleIndexMismatch);

	const auto overflow_create = FfmpegPcmEncoder::create(make_config());
	assert(overflow_create);
	PcmSampleBlock too_large{fixture.interleaved_s32.data(),
	                         static_cast<std::uint64_t>(std::numeric_limits<int>::max()) + 1u,
	                         std::numeric_limits<std::uint64_t>::max(), 0u};
	assert(overflow_create.encoder->encode(too_large, &packets) ==
	       FfmpegPcmEncoderError::SizeOverflow);
}

void check_failure_paths() {
	const Fixture fixture = make_fixture();
	const auto make_config_with_fault = [](PcmEncoderConfig::TestFault fault) {
		PcmEncoderConfig config = make_config();
		config.test_fault = fault;
		return config;
	};
	assert(FfmpegPcmEncoder::create(make_config_with_fault(PcmEncoderConfig::TestFault::Allocation))
	           .error == FfmpegPcmEncoderError::AllocationFailure);
	assert(
	    FfmpegPcmEncoder::create(make_config_with_fault(PcmEncoderConfig::TestFault::Open)).error ==
	    FfmpegPcmEncoderError::OpenFailure);
	std::vector<EncodedPcmPacket> packets;
	const PcmSampleBlock block{fixture.interleaved_s32.data(), 1u, 2u * sizeof(std::int32_t), 0u};
	{
		const auto create =
		    FfmpegPcmEncoder::create(make_config_with_fault(PcmEncoderConfig::TestFault::Send));
		assert(create);
		assert(create.encoder->encode(block, &packets) == FfmpegPcmEncoderError::SendFailure);
	}
	{
		const auto create =
		    FfmpegPcmEncoder::create(make_config_with_fault(PcmEncoderConfig::TestFault::Receive));
		assert(create);
		assert(create.encoder->encode(block, &packets) == FfmpegPcmEncoderError::ReceiveFailure);
	}
	{
		const auto create =
		    FfmpegPcmEncoder::create(make_config_with_fault(PcmEncoderConfig::TestFault::Flush));
		assert(create);
		assert(create.encoder->flush(&packets) == FfmpegPcmEncoderError::FlushFailure);
	}
}

} // namespace

int main() {
	assert(FfmpegPcmEncoder::create({0u, 0u, 1u}).error == FfmpegPcmEncoderError::InvalidConfig);
	assert(FfmpegPcmEncoder::create({22050u, 1u, 1u}).error ==
	       FfmpegPcmEncoderError::InvalidConfig);
	check_round_trip_and_partitioning();
	check_invalid_inputs();
	check_failure_paths();
	assert(vaeg::recording::ffmpeg_pcm_encoder_error_name(
	           FfmpegPcmEncoderError::SampleIndexMismatch) != nullptr);
	return 0;
}
