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
#include "recording/recording_ffmpeg_video_encoder.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/error.h>
#include <libavutil/frame.h>
#include <libavutil/opt.h>
}

#include <algorithm>
#include <cerrno>
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

std::uint8_t expand5(std::uint32_t value) noexcept {
	return static_cast<std::uint8_t>((value * 255u) / 31u);
}

std::uint8_t expand6(std::uint32_t value) noexcept {
	return static_cast<std::uint8_t>((value * 255u) / 63u);
}

bool multiply_fits(std::uint64_t left, std::uint64_t right) noexcept {
	return right == 0u || left <= std::numeric_limits<std::size_t>::max() / right;
}

} // namespace

class FfmpegVideoEncoder::Impl {
  public:
	VideoEncoderConfig config{};
	VideoEncoderInfo info{};
	CodecContextPtr context;
	FramePtr frame;
	PacketPtr packet;
	struct PendingFrame {
		std::uint64_t sequence;
		std::int64_t pts;
	};
	std::vector<PendingFrame> pending;
	bool flushed = false;
#ifdef VAEG_RECORDING_FFMPEG_TESTING
	VideoEncoderConfig::TestFault test_fault = VideoEncoderConfig::TestFault::None;
#endif

	FfmpegVideoEncoderError drain(std::vector<EncodedVideoPacket> *packets) noexcept {
		if (packets == nullptr) {
			return FfmpegVideoEncoderError::InvalidInput;
		}
		for (;;) {
#ifdef VAEG_RECORDING_FFMPEG_TESTING
			if (test_fault == VideoEncoderConfig::TestFault::Receive) {
				return FfmpegVideoEncoderError::ReceiveFailure;
			}
#endif
			const int result = avcodec_receive_packet(context.get(), packet.get());
			if (result == AVERROR(EAGAIN)) {
				return FfmpegVideoEncoderError::None;
			}
			if (result == AVERROR_EOF) {
				return FfmpegVideoEncoderError::None;
			}
			if (result < 0) {
				return FfmpegVideoEncoderError::ReceiveFailure;
			}
			if (packet->pts == AV_NOPTS_VALUE || packet->dts == AV_NOPTS_VALUE) {
				av_packet_unref(packet.get());
				return FfmpegVideoEncoderError::PacketTimestampMissing;
			}

			auto pending_frame =
			    std::find_if(pending.begin(), pending.end(), [this](const PendingFrame &candidate) {
				    return candidate.pts == packet->pts;
			    });
			if (pending_frame == pending.end()) {
				av_packet_unref(packet.get());
				return FfmpegVideoEncoderError::PacketTimestampMismatch;
			}

			EncodedVideoPacket output{};
			output.sequence = pending_frame->sequence;
			output.pts = packet->pts;
			output.dts = packet->dts;
			output.duration = packet->duration > 0 ? packet->duration : 1;
			output.keyframe = (packet->flags & AV_PKT_FLAG_KEY) != 0;
			try {
				output.data.assign(packet->data, packet->data + packet->size);
			} catch (const std::bad_alloc &) {
				av_packet_unref(packet.get());
				return FfmpegVideoEncoderError::PacketCopyFailure;
			}
			try {
				packets->push_back(std::move(output));
			} catch (const std::bad_alloc &) {
				av_packet_unref(packet.get());
				return FfmpegVideoEncoderError::PacketCopyFailure;
			}
			pending.erase(pending_frame);
			av_packet_unref(packet.get());
		}
	}

	FfmpegVideoEncoderError copy_input(const VideoEncoderFrame &input) noexcept {
		if (input.pixels == nullptr || input.descriptor.width != info.width ||
		    input.descriptor.height != info.height || input.descriptor.stride_bytes == 0u) {
			return FfmpegVideoEncoderError::InvalidInput;
		}
		if (input.descriptor.pixel_format != VAEG_RECORDING_PIXEL_RGB565 &&
		    input.descriptor.pixel_format != VAEG_RECORDING_PIXEL_RGB24) {
			return FfmpegVideoEncoderError::UnsupportedPixelFormat;
		}
		if (vaeg_recording_validate_video_descriptor(&input.descriptor) !=
		    VAEG_RECORDING_ERROR_OK) {
			return FfmpegVideoEncoderError::InvalidInput;
		}
		if (!multiply_fits(input.descriptor.stride_bytes, input.descriptor.height)) {
			return FfmpegVideoEncoderError::SizeOverflow;
		}
		const std::uint64_t storage_bytes = input.descriptor.stride_bytes * input.descriptor.height;
		if (input.bytes < storage_bytes) {
			return FfmpegVideoEncoderError::InvalidInput;
		}
		if (av_frame_make_writable(frame.get()) < 0) {
			return FfmpegVideoEncoderError::FrameFailure;
		}

		const auto *source = static_cast<const std::uint8_t *>(input.pixels);
		for (std::uint32_t y = 0; y < info.height; ++y) {
			const auto *source_row =
			    source + static_cast<std::size_t>(y) *
			                 static_cast<std::size_t>(input.descriptor.stride_bytes);
			auto *destination_row =
			    frame->data[0] +
			    static_cast<std::size_t>(y) * static_cast<std::size_t>(frame->linesize[0]);
			std::memset(destination_row, 0, static_cast<std::size_t>(frame->linesize[0]));
			for (std::uint32_t x = 0; x < info.width; ++x) {
				auto *destination = destination_row + (static_cast<std::size_t>(x) * 4u);
				if (input.descriptor.pixel_format == VAEG_RECORDING_PIXEL_RGB565) {
					const std::size_t offset = static_cast<std::size_t>(x) * 2u;
					const std::uint32_t value =
					    source_row[offset] |
					    (static_cast<std::uint32_t>(source_row[offset + 1u]) << 8);
					const std::uint8_t red = expand5((value >> 11) & 0x1fu);
					const std::uint8_t green = expand6((value >> 5) & 0x3fu);
					const std::uint8_t blue = expand5(value & 0x1fu);
					destination[0] = blue;
					destination[1] = green;
					destination[2] = red;
				} else {
					const std::size_t offset = static_cast<std::size_t>(x) * 3u;
					destination[0] = source_row[offset + 2u];
					destination[1] = source_row[offset + 1u];
					destination[2] = source_row[offset];
				}
			}
		}
		frame->pts = input.pts;
		frame->duration = 1;
		frame->pict_type = AV_PICTURE_TYPE_I;
		return FfmpegVideoEncoderError::None;
	}
};

const char *ffmpeg_video_encoder_error_name(FfmpegVideoEncoderError error) noexcept {
	switch (error) {
	case FfmpegVideoEncoderError::None:
		return "none";
	case FfmpegVideoEncoderError::InvalidConfig:
		return "invalid_config";
	case FfmpegVideoEncoderError::InvalidInput:
		return "invalid_input";
	case FfmpegVideoEncoderError::UnsupportedPixelFormat:
		return "unsupported_pixel_format";
	case FfmpegVideoEncoderError::SizeOverflow:
		return "size_overflow";
	case FfmpegVideoEncoderError::CodecUnavailable:
		return "codec_unavailable";
	case FfmpegVideoEncoderError::AllocationFailure:
		return "allocation_failure";
	case FfmpegVideoEncoderError::OptionFailure:
		return "option_failure";
	case FfmpegVideoEncoderError::OpenFailure:
		return "open_failure";
	case FfmpegVideoEncoderError::FrameFailure:
		return "frame_failure";
	case FfmpegVideoEncoderError::SendFailure:
		return "send_failure";
	case FfmpegVideoEncoderError::SendWouldBlock:
		return "send_would_block";
	case FfmpegVideoEncoderError::ReceiveFailure:
		return "receive_failure";
	case FfmpegVideoEncoderError::PacketTimestampMissing:
		return "packet_timestamp_missing";
	case FfmpegVideoEncoderError::PacketTimestampMismatch:
		return "packet_timestamp_mismatch";
	case FfmpegVideoEncoderError::PacketCopyFailure:
		return "packet_copy_failure";
	case FfmpegVideoEncoderError::FlushFailure:
		return "flush_failure";
	case FfmpegVideoEncoderError::AlreadyFlushed:
		return "already_flushed";
	default:
		return "unknown";
	}
}

VideoEncoderCreateResult FfmpegVideoEncoder::create(const VideoEncoderConfig &config) noexcept {
	if (config.width == 0u || config.height == 0u ||
	    config.width > static_cast<std::uint32_t>(std::numeric_limits<int>::max()) ||
	    config.height > static_cast<std::uint32_t>(std::numeric_limits<int>::max()) ||
	    config.time_base_num <= 0 || config.time_base_den <= 0) {
		return {FfmpegVideoEncoderError::InvalidConfig, nullptr};
	}
#ifdef VAEG_RECORDING_FFMPEG_TESTING
	if (config.test_fault == VideoEncoderConfig::TestFault::Allocation) {
		return {FfmpegVideoEncoderError::AllocationFailure, nullptr};
	}
#endif

	try {
		const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_FFV1);
		if (codec == nullptr) {
			return {FfmpegVideoEncoderError::CodecUnavailable, nullptr};
		}
		CodecContextPtr context(avcodec_alloc_context3(codec));
		if (!context) {
			return {FfmpegVideoEncoderError::AllocationFailure, nullptr};
		}
		context->width = static_cast<int>(config.width);
		context->height = static_cast<int>(config.height);
		context->pix_fmt = AV_PIX_FMT_BGR0;
		context->time_base = AVRational{config.time_base_num, config.time_base_den};
		context->framerate = AVRational{config.time_base_den, config.time_base_num};
		context->gop_size = 1;
		context->max_b_frames = 0;
		context->level = 3;
		if (av_opt_set_int(context.get(), "slicecrc", 1, AV_OPT_SEARCH_CHILDREN) < 0) {
			return {FfmpegVideoEncoderError::OptionFailure, nullptr};
		}
#ifdef VAEG_RECORDING_FFMPEG_TESTING
		if (config.test_fault == VideoEncoderConfig::TestFault::Open) {
			return {FfmpegVideoEncoderError::OpenFailure, nullptr};
		}
#endif
		if (avcodec_open2(context.get(), codec, nullptr) < 0) {
			return {FfmpegVideoEncoderError::OpenFailure, nullptr};
		}
		std::int64_t slice_crc = 0;
		if (context->level != 3 || context->gop_size != 1 || context->pix_fmt != AV_PIX_FMT_BGR0 ||
		    av_opt_get_int(context.get(), "slicecrc", AV_OPT_SEARCH_CHILDREN, &slice_crc) < 0 ||
		    slice_crc != 1) {
			return {FfmpegVideoEncoderError::OptionFailure, nullptr};
		}

		FramePtr frame(av_frame_alloc());
		PacketPtr packet(av_packet_alloc());
		if (!frame || !packet) {
			return {FfmpegVideoEncoderError::AllocationFailure, nullptr};
		}
		frame->format = AV_PIX_FMT_BGR0;
		frame->width = static_cast<int>(config.width);
		frame->height = static_cast<int>(config.height);
		if (av_frame_get_buffer(frame.get(), 32) < 0) {
			return {FfmpegVideoEncoderError::AllocationFailure, nullptr};
		}

		std::unique_ptr<Impl> impl(new Impl());
		impl->config = config;
		impl->info = {config.width, config.height, 1u, true, true, true, {}};
		try {
			if (context->extradata != nullptr && context->extradata_size > 0) {
				impl->info.codec_extradata.assign(context->extradata,
				                                  context->extradata + context->extradata_size);
			}
		} catch (const std::bad_alloc &) {
			return {FfmpegVideoEncoderError::AllocationFailure, nullptr};
		}
		impl->context = std::move(context);
		impl->frame = std::move(frame);
		impl->packet = std::move(packet);
#ifdef VAEG_RECORDING_FFMPEG_TESTING
		impl->test_fault = config.test_fault;
#endif
		return {FfmpegVideoEncoderError::None,
		        std::unique_ptr<FfmpegVideoEncoder>(new FfmpegVideoEncoder(std::move(impl)))};
	} catch (const std::bad_alloc &) {
		return {FfmpegVideoEncoderError::AllocationFailure, nullptr};
	}
}

FfmpegVideoEncoder::FfmpegVideoEncoder(std::unique_ptr<Impl> impl) noexcept
    : impl_(std::move(impl)) {
}

FfmpegVideoEncoder::~FfmpegVideoEncoder() = default;

FfmpegVideoEncoderError
FfmpegVideoEncoder::encode(const VideoEncoderFrame &input,
                           std::vector<EncodedVideoPacket> *packets) noexcept {
	if (!impl_ || packets == nullptr || impl_->flushed) {
		return impl_ && impl_->flushed ? FfmpegVideoEncoderError::AlreadyFlushed
		                               : FfmpegVideoEncoderError::InvalidInput;
	}
	try {
		const FfmpegVideoEncoderError copy_error = impl_->copy_input(input);
		if (copy_error != FfmpegVideoEncoderError::None) {
			return copy_error;
		}
		impl_->pending.push_back({input.sequence, input.pts});
		for (;;) {
#ifdef VAEG_RECORDING_FFMPEG_TESTING
			if (impl_->test_fault == VideoEncoderConfig::TestFault::Send) {
				impl_->pending.pop_back();
				return FfmpegVideoEncoderError::SendFailure;
			}
#endif
			const int result = avcodec_send_frame(impl_->context.get(), impl_->frame.get());
			if (result == AVERROR(EAGAIN)) {
				const std::size_t packet_count = packets->size();
				const FfmpegVideoEncoderError drain_error = impl_->drain(packets);
				if (drain_error != FfmpegVideoEncoderError::None) {
					impl_->pending.pop_back();
					return drain_error;
				}
				if (packets->size() == packet_count) {
					impl_->pending.pop_back();
					return FfmpegVideoEncoderError::SendWouldBlock;
				}
				continue;
			}
			if (result < 0) {
				impl_->pending.pop_back();
				return FfmpegVideoEncoderError::SendFailure;
			}
			return impl_->drain(packets);
		}
	} catch (const std::bad_alloc &) {
		return FfmpegVideoEncoderError::AllocationFailure;
	}
}

FfmpegVideoEncoderError
FfmpegVideoEncoder::flush(std::vector<EncodedVideoPacket> *packets) noexcept {
	if (!impl_ || packets == nullptr) {
		return FfmpegVideoEncoderError::InvalidInput;
	}
	if (impl_->flushed) {
		return FfmpegVideoEncoderError::AlreadyFlushed;
	}
	try {
#ifdef VAEG_RECORDING_FFMPEG_TESTING
		if (impl_->test_fault == VideoEncoderConfig::TestFault::Flush) {
			return FfmpegVideoEncoderError::FlushFailure;
		}
#endif
		for (;;) {
			const int result = avcodec_send_frame(impl_->context.get(), nullptr);
			if (result == AVERROR(EAGAIN)) {
				const std::size_t packet_count = packets->size();
				const FfmpegVideoEncoderError drain_error = impl_->drain(packets);
				if (drain_error != FfmpegVideoEncoderError::None) {
					return drain_error;
				}
				if (packets->size() == packet_count) {
					return FfmpegVideoEncoderError::FlushFailure;
				}
				continue;
			}
			if (result < 0) {
				return FfmpegVideoEncoderError::FlushFailure;
			}
			break;
		}
		for (;;) {
			const int result = avcodec_receive_packet(impl_->context.get(), impl_->packet.get());
			if (result == AVERROR_EOF) {
				break;
			}
			if (result == AVERROR(EAGAIN) || result < 0) {
				return FfmpegVideoEncoderError::FlushFailure;
			}
			if (impl_->packet->pts == AV_NOPTS_VALUE || impl_->packet->dts == AV_NOPTS_VALUE) {
				av_packet_unref(impl_->packet.get());
				return FfmpegVideoEncoderError::PacketTimestampMissing;
			}
			auto pending_frame = std::find_if(impl_->pending.begin(), impl_->pending.end(),
			                                  [this](const Impl::PendingFrame &candidate) {
				                                  return candidate.pts == impl_->packet->pts;
			                                  });
			if (pending_frame == impl_->pending.end()) {
				av_packet_unref(impl_->packet.get());
				return FfmpegVideoEncoderError::PacketTimestampMismatch;
			}
			EncodedVideoPacket output{};
			output.sequence = pending_frame->sequence;
			output.pts = impl_->packet->pts;
			output.dts = impl_->packet->dts;
			output.duration = impl_->packet->duration > 0 ? impl_->packet->duration : 1;
			output.keyframe = (impl_->packet->flags & AV_PKT_FLAG_KEY) != 0;
			try {
				output.data.assign(impl_->packet->data, impl_->packet->data + impl_->packet->size);
			} catch (const std::bad_alloc &) {
				av_packet_unref(impl_->packet.get());
				return FfmpegVideoEncoderError::PacketCopyFailure;
			}
			try {
				packets->push_back(std::move(output));
			} catch (const std::bad_alloc &) {
				av_packet_unref(impl_->packet.get());
				return FfmpegVideoEncoderError::PacketCopyFailure;
			}
			impl_->pending.erase(pending_frame);
			av_packet_unref(impl_->packet.get());
		}
		if (!impl_->pending.empty()) {
			return FfmpegVideoEncoderError::PacketTimestampMismatch;
		}
		impl_->flushed = true;
		return FfmpegVideoEncoderError::None;
	} catch (const std::bad_alloc &) {
		return FfmpegVideoEncoderError::AllocationFailure;
	}
}

const VideoEncoderInfo &FfmpegVideoEncoder::info() const noexcept {
	return impl_->info;
}

} // namespace vaeg::recording
