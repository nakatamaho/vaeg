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
#include "recording/recording_ffmpeg_pcm_encoder.hpp"
#include "compiler.h"
#include "common/parts.h"

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
#include <libavutil/frame.h>
#include <libavutil/intreadwrite.h>
}

#include <algorithm>
#include <cstring>
#include <limits>
#include <new>
#include <utility>

namespace vaeg::recording {
namespace {

struct CodecContextDeleter {
	void operator()(AVCodecContext *context) const noexcept {
		avcodec_free_context(&context);
	}
};

struct FrameDeleter {
	void operator()(AVFrame *frame) const noexcept {
		av_frame_free(&frame);
	}
};

struct PacketDeleter {
	void operator()(AVPacket *packet) const noexcept {
		av_packet_free(&packet);
	}
};

using CodecContextPtr = std::unique_ptr<AVCodecContext, CodecContextDeleter>;
using FramePtr = std::unique_ptr<AVFrame, FrameDeleter>;
using PacketPtr = std::unique_ptr<AVPacket, PacketDeleter>;

bool host_is_little_endian() noexcept {
	const std::uint16_t value = 1u;
	return *reinterpret_cast<const std::uint8_t *>(&value) == 1u;
}

bool multiply_fits(std::uint64_t left, std::uint64_t right) noexcept {
	return right == 0u || left <= std::numeric_limits<std::size_t>::max() / right;
}

} // namespace

class FfmpegPcmEncoder::Impl {
  public:
	PcmEncoderConfig config{};
	PcmEncoderInfo info{};
	CodecContextPtr context;
	FramePtr frame;
	PacketPtr packet;
	struct PendingBlock {
		std::uint64_t first_sample_index;
		std::int64_t pts;
		std::uint64_t sample_frames;
	};
	std::vector<PendingBlock> pending;
	std::uint64_t next_sample_index = 0u;
	bool has_sample_index = false;
	bool flushed = false;
#ifdef VAEG_RECORDING_FFMPEG_TESTING
	PcmEncoderConfig::TestFault test_fault = PcmEncoderConfig::TestFault::None;
#endif

	FfmpegPcmEncoderError drain(std::vector<EncodedPcmPacket> *packets,
	                            bool *end_of_file) noexcept {
		if (packets == nullptr || end_of_file == nullptr) {
			return FfmpegPcmEncoderError::InvalidInput;
		}
		*end_of_file = false;
		for (;;) {
#ifdef VAEG_RECORDING_FFMPEG_TESTING
			if (test_fault == PcmEncoderConfig::TestFault::Receive) {
				return FfmpegPcmEncoderError::ReceiveFailure;
			}
#endif
			const int result = avcodec_receive_packet(context.get(), packet.get());
			if (result == AVERROR(EAGAIN)) {
				return FfmpegPcmEncoderError::None;
			}
			if (result == AVERROR_EOF) {
				*end_of_file = true;
				return FfmpegPcmEncoderError::None;
			}
			if (result < 0) {
				return FfmpegPcmEncoderError::ReceiveFailure;
			}
			if (packet->pts == AV_NOPTS_VALUE || packet->dts == AV_NOPTS_VALUE) {
				av_packet_unref(packet.get());
				return FfmpegPcmEncoderError::PacketTimestampMissing;
			}

			auto pending_block =
			    std::find_if(pending.begin(), pending.end(), [this](const PendingBlock &candidate) {
				    return candidate.pts == packet->pts;
			    });
			if (pending_block == pending.end()) {
				av_packet_unref(packet.get());
				return FfmpegPcmEncoderError::PacketTimestampMismatch;
			}

			EncodedPcmPacket output{};
			output.first_sample_index = pending_block->first_sample_index;
			output.pts = packet->pts;
			output.dts = packet->dts;
			output.duration = packet->duration > 0
			                      ? packet->duration
			                      : static_cast<std::int64_t>(pending_block->sample_frames);
			try {
				output.data.assign(packet->data, packet->data + packet->size);
			} catch (const std::bad_alloc &) {
				av_packet_unref(packet.get());
				return FfmpegPcmEncoderError::PacketCopyFailure;
			}
			try {
				packets->push_back(std::move(output));
			} catch (const std::bad_alloc &) {
				av_packet_unref(packet.get());
				return FfmpegPcmEncoderError::PacketCopyFailure;
			}
			pending.erase(pending_block);
			av_packet_unref(packet.get());
		}
	}

	FfmpegPcmEncoderError prepare_frame(const PcmSampleBlock &block) noexcept {
		if (block.interleaved_s32 == nullptr || block.sample_frames == 0u) {
			return FfmpegPcmEncoderError::InvalidInput;
		}
		if (!multiply_fits(block.sample_frames, 2u) ||
		    !multiply_fits(block.sample_frames * 2u, sizeof(std::int32_t))) {
			return FfmpegPcmEncoderError::SizeOverflow;
		}
		if (block.sample_frames > static_cast<std::uint64_t>(std::numeric_limits<int>::max())) {
			return FfmpegPcmEncoderError::SizeOverflow;
		}
		if (block.sample_frames >
		    static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max()) /
		        (2u * sizeof(SINT16))) {
			return FfmpegPcmEncoderError::SizeOverflow;
		}
		const std::uint64_t input_bytes = block.sample_frames * 2u * sizeof(std::int32_t);
		if (block.bytes < input_bytes ||
		    block.first_sample_index >
		        static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) ||
		    block.sample_frames >
		        std::numeric_limits<std::uint64_t>::max() - block.first_sample_index) {
			return FfmpegPcmEncoderError::InvalidInput;
		}
		if (has_sample_index && block.first_sample_index != next_sample_index) {
			return FfmpegPcmEncoderError::SampleIndexMismatch;
		}
		if (!host_is_little_endian()) {
			return FfmpegPcmEncoderError::UnsupportedSampleFormat;
		}

		av_frame_unref(frame.get());
		frame->format = AV_SAMPLE_FMT_S16;
		frame->sample_rate = static_cast<int>(info.sample_rate_hz);
		frame->nb_samples = static_cast<int>(block.sample_frames);
		if (av_channel_layout_copy(&frame->ch_layout, &context->ch_layout) < 0 ||
		    av_frame_get_buffer(frame.get(), 0) < 0 || frame->data[0] == nullptr ||
		    frame->linesize[0] < 0 ||
		    static_cast<std::uint64_t>(frame->linesize[0]) < block.sample_frames * 4u) {
			return FfmpegPcmEncoderError::FrameFailure;
		}

		std::vector<SINT16> saturated(static_cast<std::size_t>(block.sample_frames) * 2u);
		satuation_s16(saturated.data(), block.interleaved_s32,
		              static_cast<std::uint32_t>(block.sample_frames * 2u * sizeof(SINT16)));
		for (std::uint64_t sample = 0u; sample < block.sample_frames; ++sample) {
			for (std::uint32_t channel = 0u; channel < 2u; ++channel) {
				const std::size_t destination =
				    static_cast<std::size_t>(sample * 4u + channel * 2u);
				AV_WL16(frame->data[0] + destination,
				        static_cast<std::uint16_t>(
				            saturated[static_cast<std::size_t>(sample) * 2u + channel]));
			}
		}
		frame->pts = static_cast<std::int64_t>(block.first_sample_index);
		frame->duration = static_cast<std::int64_t>(block.sample_frames);
		return FfmpegPcmEncoderError::None;
	}
};

const char *ffmpeg_pcm_encoder_error_name(FfmpegPcmEncoderError error) noexcept {
	switch (error) {
	case FfmpegPcmEncoderError::None:
		return "none";
	case FfmpegPcmEncoderError::InvalidConfig:
		return "invalid_config";
	case FfmpegPcmEncoderError::InvalidInput:
		return "invalid_input";
	case FfmpegPcmEncoderError::SizeOverflow:
		return "size_overflow";
	case FfmpegPcmEncoderError::UnsupportedSampleFormat:
		return "unsupported_sample_format";
	case FfmpegPcmEncoderError::CodecUnavailable:
		return "codec_unavailable";
	case FfmpegPcmEncoderError::AllocationFailure:
		return "allocation_failure";
	case FfmpegPcmEncoderError::OpenFailure:
		return "open_failure";
	case FfmpegPcmEncoderError::FrameFailure:
		return "frame_failure";
	case FfmpegPcmEncoderError::SendFailure:
		return "send_failure";
	case FfmpegPcmEncoderError::SendWouldBlock:
		return "send_would_block";
	case FfmpegPcmEncoderError::ReceiveFailure:
		return "receive_failure";
	case FfmpegPcmEncoderError::PacketTimestampMissing:
		return "packet_timestamp_missing";
	case FfmpegPcmEncoderError::PacketTimestampMismatch:
		return "packet_timestamp_mismatch";
	case FfmpegPcmEncoderError::PacketCopyFailure:
		return "packet_copy_failure";
	case FfmpegPcmEncoderError::SampleIndexMismatch:
		return "sample_index_mismatch";
	case FfmpegPcmEncoderError::FlushFailure:
		return "flush_failure";
	case FfmpegPcmEncoderError::AlreadyFlushed:
		return "already_flushed";
	default:
		return "unknown";
	}
}

PcmEncoderCreateResult FfmpegPcmEncoder::create(const PcmEncoderConfig &config) noexcept {
	if (config.sample_rate_hz == 0u ||
	    config.sample_rate_hz > static_cast<std::uint32_t>(std::numeric_limits<int>::max()) ||
	    config.origin_offset_denominator == 0u ||
	    config.origin_offset_numerator >= config.origin_offset_denominator) {
		return {FfmpegPcmEncoderError::InvalidConfig, nullptr};
	}
#ifdef VAEG_RECORDING_FFMPEG_TESTING
	if (config.test_fault == PcmEncoderConfig::TestFault::Allocation) {
		return {FfmpegPcmEncoderError::AllocationFailure, nullptr};
	}
#endif

	try {
		const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_PCM_S16LE);
		if (codec == nullptr) {
			return {FfmpegPcmEncoderError::CodecUnavailable, nullptr};
		}
		CodecContextPtr context(avcodec_alloc_context3(codec));
		if (!context) {
			return {FfmpegPcmEncoderError::AllocationFailure, nullptr};
		}
		context->sample_fmt = AV_SAMPLE_FMT_S16;
		context->sample_rate = static_cast<int>(config.sample_rate_hz);
		context->time_base = AVRational{1, static_cast<int>(config.sample_rate_hz)};
		av_channel_layout_default(&context->ch_layout, 2);
#ifdef VAEG_RECORDING_FFMPEG_TESTING
		if (config.test_fault == PcmEncoderConfig::TestFault::Open) {
			return {FfmpegPcmEncoderError::OpenFailure, nullptr};
		}
#endif
		if (avcodec_open2(context.get(), codec, nullptr) < 0 ||
		    context->sample_fmt != AV_SAMPLE_FMT_S16 ||
		    context->sample_rate != static_cast<int>(config.sample_rate_hz) ||
		    context->ch_layout.nb_channels != 2) {
			return {FfmpegPcmEncoderError::OpenFailure, nullptr};
		}

		FramePtr frame(av_frame_alloc());
		PacketPtr packet(av_packet_alloc());
		if (!frame || !packet) {
			return {FfmpegPcmEncoderError::AllocationFailure, nullptr};
		}
		std::unique_ptr<Impl> impl(new Impl());
		impl->config = config;
		impl->info = {
		    config.sample_rate_hz,           2u, 16u, true, config.origin_offset_numerator,
		    config.origin_offset_denominator};
		impl->context = std::move(context);
		impl->frame = std::move(frame);
		impl->packet = std::move(packet);
#ifdef VAEG_RECORDING_FFMPEG_TESTING
		impl->test_fault = config.test_fault;
#endif
		return {FfmpegPcmEncoderError::None,
		        std::unique_ptr<FfmpegPcmEncoder>(new FfmpegPcmEncoder(std::move(impl)))};
	} catch (const std::bad_alloc &) {
		return {FfmpegPcmEncoderError::AllocationFailure, nullptr};
	}
}

FfmpegPcmEncoder::FfmpegPcmEncoder(std::unique_ptr<Impl> impl) noexcept : impl_(std::move(impl)) {
}

FfmpegPcmEncoder::~FfmpegPcmEncoder() = default;

FfmpegPcmEncoderError FfmpegPcmEncoder::encode(const PcmSampleBlock &block,
                                               std::vector<EncodedPcmPacket> *packets) noexcept {
	if (!impl_ || packets == nullptr || impl_->flushed) {
		return impl_ && impl_->flushed ? FfmpegPcmEncoderError::AlreadyFlushed
		                               : FfmpegPcmEncoderError::InvalidInput;
	}
	try {
		const FfmpegPcmEncoderError prepare_error = impl_->prepare_frame(block);
		if (prepare_error != FfmpegPcmEncoderError::None) {
			return prepare_error;
		}
		impl_->pending.push_back({block.first_sample_index,
		                          static_cast<std::int64_t>(block.first_sample_index),
		                          block.sample_frames});
		for (;;) {
#ifdef VAEG_RECORDING_FFMPEG_TESTING
			if (impl_->test_fault == PcmEncoderConfig::TestFault::Send) {
				impl_->pending.pop_back();
				return FfmpegPcmEncoderError::SendFailure;
			}
#endif
			const int result = avcodec_send_frame(impl_->context.get(), impl_->frame.get());
			if (result == AVERROR(EAGAIN)) {
				const std::size_t packet_count = packets->size();
				bool end_of_file = false;
				const FfmpegPcmEncoderError drain_error = impl_->drain(packets, &end_of_file);
				if (drain_error != FfmpegPcmEncoderError::None) {
					impl_->pending.pop_back();
					return drain_error;
				}
				if (end_of_file || packets->size() == packet_count) {
					impl_->pending.pop_back();
					return FfmpegPcmEncoderError::SendWouldBlock;
				}
				continue;
			}
			if (result < 0) {
				impl_->pending.pop_back();
				return FfmpegPcmEncoderError::SendFailure;
			}
			impl_->next_sample_index = block.first_sample_index + block.sample_frames;
			impl_->has_sample_index = true;
			bool end_of_file = false;
			return impl_->drain(packets, &end_of_file);
		}
	} catch (const std::bad_alloc &) {
		return FfmpegPcmEncoderError::AllocationFailure;
	}
}

FfmpegPcmEncoderError FfmpegPcmEncoder::flush(std::vector<EncodedPcmPacket> *packets) noexcept {
	if (!impl_ || packets == nullptr) {
		return FfmpegPcmEncoderError::InvalidInput;
	}
	if (impl_->flushed) {
		return FfmpegPcmEncoderError::AlreadyFlushed;
	}
	try {
#ifdef VAEG_RECORDING_FFMPEG_TESTING
		if (impl_->test_fault == PcmEncoderConfig::TestFault::Flush) {
			return FfmpegPcmEncoderError::FlushFailure;
		}
#endif
		for (;;) {
			const int result = avcodec_send_frame(impl_->context.get(), nullptr);
			if (result == AVERROR(EAGAIN)) {
				const std::size_t packet_count = packets->size();
				bool end_of_file = false;
				const FfmpegPcmEncoderError drain_error = impl_->drain(packets, &end_of_file);
				if (drain_error != FfmpegPcmEncoderError::None || end_of_file ||
				    packets->size() == packet_count) {
					return FfmpegPcmEncoderError::FlushFailure;
				}
				continue;
			}
			if (result < 0) {
				return FfmpegPcmEncoderError::FlushFailure;
			}
			break;
		}
		for (;;) {
			bool end_of_file = false;
			const FfmpegPcmEncoderError drain_error = impl_->drain(packets, &end_of_file);
			if (drain_error != FfmpegPcmEncoderError::None) {
				return FfmpegPcmEncoderError::FlushFailure;
			}
			if (end_of_file) {
				break;
			}
			return FfmpegPcmEncoderError::FlushFailure;
		}
		if (!impl_->pending.empty()) {
			return FfmpegPcmEncoderError::PacketTimestampMismatch;
		}
		impl_->flushed = true;
		return FfmpegPcmEncoderError::None;
	} catch (const std::bad_alloc &) {
		return FfmpegPcmEncoderError::AllocationFailure;
	}
}

const PcmEncoderInfo &FfmpegPcmEncoder::info() const noexcept {
	return impl_->info;
}

} // namespace vaeg::recording
