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
#include "recording/recording_h264_encoder.hpp"
#include <algorithm>
#include <cstring>
#include <deque>
#include <limits>
#include <new>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
}
namespace vaeg::recording {
const char *h264_error_name(H264Error error) noexcept {
	switch (error) {
	case H264Error::None: return "OSR_H264_OK";
	case H264Error::InvalidConfig: return "OSR_H264_INVALID_CONFIG";
	case H264Error::UnsupportedSize: return "OSR_H264_UNSUPPORTED_SIZE";
	case H264Error::ProviderNotBuilt: return "OSR_H264_PROVIDER_NOT_BUILT";
	case H264Error::ProviderUnavailable: return "OSR_H264_PROVIDER_UNAVAILABLE";
	case H264Error::OptionFailure: return "OSR_H264_OPTION_FAILURE";
	case H264Error::AllocationFailure: return "OSR_H264_ALLOCATION_FAILURE";
	case H264Error::InvalidFrame: return "OSR_H264_INVALID_FRAME";
	case H264Error::InflightLimit: return "OSR_H264_INFLIGHT_LIMIT";
	case H264Error::EncodeFailure: return "OSR_H264_ENCODE_FAILURE";
	case H264Error::TimestampFailure: return "OSR_H264_TIMESTAMP_FAILURE";
	case H264Error::DrainFailure: return "OSR_H264_DRAIN_FAILURE";
	case H264Error::AlreadyDrained: return "OSR_H264_ALREADY_DRAINED";
	}
	return "OSR_H264_UNKNOWN";
}
class H264Encoder::Impl {
  public:
	AVCodecContext *context = nullptr;
	AVFrame *frame = nullptr;
	AVPacket *packet = nullptr;
	H264Info info;
	std::deque<std::int64_t> pending;
	std::int64_t last_slot = -1;
	bool drained = false;
	~Impl() {
		av_packet_free(&packet);
		av_frame_free(&frame);
		avcodec_free_context(&context);
	}
	H264Error receive(std::vector<EncodedVideoPacket> *packets, bool final) {
		for (;;) {
			const int result = avcodec_receive_packet(context, packet);
			if (result == AVERROR(EAGAIN)) {
				return final ? H264Error::DrainFailure : H264Error::None;
			}
			if (result == AVERROR_EOF) {
				return final && !pending.empty() ? H264Error::DrainFailure : H264Error::None;
			}
			if (result < 0) {
				return H264Error::EncodeFailure;
			}
			const auto found = std::find(pending.begin(), pending.end(), packet->pts);
			if (packet->pts == AV_NOPTS_VALUE || packet->dts == AV_NOPTS_VALUE ||
			    found == pending.end() || packet->dts > packet->pts) {
				av_packet_unref(packet);
				return H264Error::TimestampFailure;
			}
			EncodedVideoPacket owned{};
			owned.sequence = static_cast<std::uint64_t>(packet->pts);
			owned.pts = packet->pts;
			owned.dts = packet->dts;
			owned.duration = 1;
			owned.keyframe = (packet->flags & AV_PKT_FLAG_KEY) != 0;
			owned.data.assign(packet->data, packet->data + packet->size);
			av_packet_unref(packet);
			packets->push_back(std::move(owned));
			pending.erase(found);
		}
	}
};
H264Encoder::H264Encoder(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {}
H264Encoder::~H264Encoder() = default;
const H264Info &H264Encoder::info() const noexcept { return impl_->info; }

H264Error validate_h264_config(const H264Config &config) noexcept {
	if (config.width == 0 || config.height == 0 || config.bitrate <= 0 ||
	    config.bitrate > 1000000000) {
		return H264Error::InvalidConfig;
	}
	if ((config.width & 1) || (config.height & 1) || config.width > 4096 || config.height > 4096) {
		return H264Error::UnsupportedSize;
	}
	return H264Error::None;
}
H264Error H264Encoder::create(const H264Config &config,
                             std::unique_ptr<H264Encoder> *encoder,
                             std::string *detail) noexcept {
	if (encoder == nullptr || detail == nullptr || *encoder) {
		return H264Error::InvalidConfig;
	}
	const auto valid = validate_h264_config(config);
	if (valid != H264Error::None) return valid;
	try {
		detail->clear();
#ifndef __APPLE__
		*detail = "VideoToolbox requires macOS; other hardware adapters are not implemented yet";
		return H264Error::ProviderNotBuilt;
#else
		const AVCodec *codec = avcodec_find_encoder_by_name("h264_videotoolbox");
		if (codec == nullptr) {
			*detail = "FFmpeg was built without h264_videotoolbox";
			return H264Error::ProviderNotBuilt;
		}
		auto impl = std::make_unique<Impl>();
		impl->context = avcodec_alloc_context3(codec);
		impl->frame = av_frame_alloc();
		impl->packet = av_packet_alloc();
		if (!impl->context || !impl->frame || !impl->packet) {
			return H264Error::AllocationFailure;
		}
		auto *c = impl->context;
		c->width = static_cast<int>(config.width);
		c->height = static_cast<int>(config.height);
		c->pix_fmt = AV_PIX_FMT_NV12;
		c->time_base = AVRational{1, 60};
		c->framerate = AVRational{60, 1};
		c->bit_rate = config.bitrate;
		c->gop_size = 120;
		c->max_b_frames = 0;
		c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		c->color_range = AVCOL_RANGE_MPEG;
		c->colorspace = AVCOL_SPC_BT709;
		c->color_primaries = AVCOL_PRI_BT709;
		c->color_trc = AVCOL_TRC_BT709;
		/* FFmpeg 8.1.2 vtenc_configure_encoder maps allow_sw=0/require_sw=0
		 * to RequireHardwareAcceleratedVideoEncoder=true. Fail closed if either
		 * option is absent; never retry allowing software. */
		if (av_opt_set_int(c, "allow_sw", 0, AV_OPT_SEARCH_CHILDREN) < 0 ||
		    av_opt_set_int(c, "require_sw", 0, AV_OPT_SEARCH_CHILDREN) < 0 ||
		    av_opt_set_int(c, "realtime", 1, AV_OPT_SEARCH_CHILDREN) < 0) {
			return H264Error::OptionFailure;
		}
		const int opened = avcodec_open2(c, codec, nullptr);
		if (opened < 0) {
			char message[AV_ERROR_MAX_STRING_SIZE];
			av_strerror(opened, message, sizeof(message));
			*detail = std::string("VideoToolbox hardware-required open failed: ") + message;
			return H264Error::ProviderUnavailable;
		}
		if (c->has_b_frames || c->max_b_frames != 0 || c->pix_fmt != AV_PIX_FMT_NV12 ||
		    c->time_base.num != 1 || c->time_base.den != 60) {
			return H264Error::OptionFailure;
		}
		impl->frame->format = c->pix_fmt;
		impl->frame->width = c->width;
		impl->frame->height = c->height;
		impl->frame->color_range = c->color_range;
		impl->frame->colorspace = c->colorspace;
		impl->frame->color_primaries = c->color_primaries;
		impl->frame->color_trc = c->color_trc;
		if (av_frame_get_buffer(impl->frame, 32) < 0) {
			return H264Error::AllocationFailure;
		}
		impl->info = {"h264_videotoolbox", "OS-selected device (ID not exposed by libavcodec)",
		              "hardware required by allow_sw=0, require_sw=0; output probe pending",
		              config.width, config.height, config.bitrate, c->bit_rate, {}};
		if (c->extradata && c->extradata_size > 0) {
			impl->info.extradata.assign(c->extradata, c->extradata + c->extradata_size);
		}
		*encoder = std::unique_ptr<H264Encoder>(new H264Encoder(std::move(impl)));
		return H264Error::None;
#endif
	} catch (const std::bad_alloc &) {
		return H264Error::AllocationFailure;
	}
}
H264Error H264Encoder::encode_nv12(const std::uint8_t *y, std::uint32_t y_stride,
                                  const std::uint8_t *uv, std::uint32_t uv_stride,
                                  std::uint64_t slot,
                                  std::vector<EncodedVideoPacket> *packets) noexcept {
	if (impl_->drained) {
		return H264Error::AlreadyDrained;
	}
	if (!y || !uv || !packets || y_stride < impl_->info.width || uv_stride < impl_->info.width ||
	    slot > static_cast<std::uint64_t>(INT64_MAX) ||
	    static_cast<std::int64_t>(slot) <= impl_->last_slot) {
		return H264Error::InvalidFrame;
	}
	try {
		const auto received = impl_->receive(packets, false);
		if (received != H264Error::None) return received;
		if (impl_->pending.size() >= 8) return H264Error::InflightLimit;
		if (av_frame_make_writable(impl_->frame) < 0) return H264Error::AllocationFailure;
		for (unsigned row = 0; row < impl_->info.height; ++row) {
			std::memcpy(impl_->frame->data[0] + row * impl_->frame->linesize[0],
			            y + static_cast<std::size_t>(row) * y_stride, impl_->info.width);
		}
		for (unsigned row = 0; row < impl_->info.height / 2; ++row) {
			std::memcpy(impl_->frame->data[1] + row * impl_->frame->linesize[1],
			            uv + static_cast<std::size_t>(row) * uv_stride, impl_->info.width);
		}
		impl_->frame->pts = static_cast<std::int64_t>(slot);
		impl_->pending.push_back(impl_->frame->pts);
		if (avcodec_send_frame(impl_->context, impl_->frame) < 0) {
			impl_->pending.pop_back();
			return H264Error::EncodeFailure;
		}
		impl_->last_slot = impl_->frame->pts;
		return impl_->receive(packets, false);
	} catch (const std::bad_alloc &) { return H264Error::AllocationFailure; }
}
H264Error H264Encoder::drain(std::vector<EncodedVideoPacket> *packets) noexcept {
	if (!packets) return H264Error::InvalidFrame;
	if (impl_->drained) return H264Error::AlreadyDrained;
	impl_->drained = true;
	try {
		const auto received = impl_->receive(packets, false);
		if (received != H264Error::None) return received;
		if (avcodec_send_frame(impl_->context, nullptr) < 0) return H264Error::DrainFailure;
		return impl_->receive(packets, true);
	} catch (const std::bad_alloc &) { return H264Error::AllocationFailure; }
}
} // namespace vaeg::recording
