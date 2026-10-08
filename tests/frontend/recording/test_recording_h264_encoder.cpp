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
#include <cstdio>
#include <cstdlib>
#include <cstring>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/mem.h>
}
using namespace vaeg::recording;
static void check(bool condition, const char *code) {
	if (!condition) { std::fprintf(stderr, "%s\n", code); std::exit(1); }
}
int main(int argc, char **argv) {
	std::unique_ptr<H264Encoder> encoder;
	std::string detail;
	H264Config valid{640, 400, 8000000};
	check(validate_h264_config(valid) == H264Error::None, "OSR_H264_VALID_CONFIG");
	auto mutated = valid;
	mutated.width = 0;
	check(validate_h264_config(mutated) == H264Error::InvalidConfig, "OSR_H264_INVALID_CONFIG");
	mutated = valid;
	mutated.width = 641;
	check(validate_h264_config(mutated) == H264Error::UnsupportedSize, "OSR_H264_UNSUPPORTED_SIZE");
	mutated = valid;
	mutated.bitrate = 0;
	check(validate_h264_config(mutated) == H264Error::InvalidConfig, "OSR_H264_INVALID_BITRATE");
	if (argc > 1 && std::strcmp(argv[1], "--contracts") == 0) {
		std::puts("OSR_H264_CONTRACT_PASS"); return 0;
	}
	const auto created = H264Encoder::create({640, 400, 8000000}, &encoder, &detail);
	if (created == H264Error::ProviderUnavailable || created == H264Error::ProviderNotBuilt) {
		std::printf("%s: %s\n", h264_error_name(created), detail.c_str());
		return 77; // CTest SKIP, never a hardware PASS.
	}
	check(created == H264Error::None, h264_error_name(created));
	std::printf("provider=%s\ndevice=%s\nevidence=%s\nrequested_bitrate=%lld actual_bitrate=%lld\n",
	            encoder->info().provider.c_str(), encoder->info().device.c_str(),
	            encoder->info().hardware_evidence.c_str(),
	            static_cast<long long>(encoder->info().requested_bitrate),
	            static_cast<long long>(encoder->info().actual_bitrate));
	std::vector<std::uint8_t> y(640 * 400), uv(640 * 200, 128);
	std::vector<EncodedVideoPacket> packets;
	/* Public deterministic SDR fixture: dark top / light bottom (orientation),
	 * scrolling vertical bars, and static high-contrast text-like edges. */
	for (std::uint64_t slot = 0; slot < 180; ++slot) {
		for (unsigned row = 0; row < 400; ++row) {
			for (unsigned col = 0; col < 640; ++col) {
				y[row * 640 + col] = row < 100 ? 32 : row > 299 ? 220 :
				    ((col + slot * 3) % 32 < 16 ? 64 : 180);
			}
		}
		const auto result = encoder->encode_nv12(y.data(), 640, uv.data(), 640, slot, &packets);
		check(result == H264Error::None, h264_error_name(result));
	}
	check(encoder->drain(&packets) == H264Error::None, "OSR_H264_DRAIN");
	check(encoder->drain(&packets) == H264Error::AlreadyDrained, "OSR_H264_DOUBLE_DRAIN");
	check(packets.size() == 180, "OSR_H264_PACKET_COUNT");
	const AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_H264);
	check(codec != nullptr, "OSR_H264_DECODER_MISSING");
	AVCodecContext *decoder = avcodec_alloc_context3(codec);
	check(decoder != nullptr, "OSR_H264_DECODER_ALLOC");
	const auto &extra = encoder->info().extradata;
	decoder->extradata = static_cast<std::uint8_t *>(av_mallocz(extra.size() + AV_INPUT_BUFFER_PADDING_SIZE));
	check(decoder->extradata != nullptr, "OSR_H264_EXTRADATA_ALLOC");
	std::memcpy(decoder->extradata, extra.data(), extra.size());
	decoder->extradata_size = static_cast<int>(extra.size());
	check(avcodec_open2(decoder, codec, nullptr) == 0, "OSR_H264_DECODER_OPEN");
	AVFrame *frame = av_frame_alloc();
	AVPacket *packet = av_packet_alloc();
	check(frame && packet, "OSR_H264_DECODE_ALLOC");
	unsigned count = 0, keys = 0;
	std::uint64_t bytes = 0;
	std::int64_t last_dts = INT64_MIN;
	auto receive = [&]() {
		int result;
		while ((result = avcodec_receive_frame(decoder, frame)) == 0) {
			check(frame->width == 640 && frame->height == 400, "OSR_H264_DECODE_EXTENT");
			check(frame->pts == count, "OSR_H264_DECODE_PTS");
			check(frame->color_range == AVCOL_RANGE_MPEG && frame->colorspace == AVCOL_SPC_BT709,
			      "OSR_H264_COLOR_METADATA");
			check(frame->data[0][20 * frame->linesize[0] + 20] < 50 &&
			      frame->data[0][380 * frame->linesize[0] + 20] > 200, "OSR_H264_ORIENTATION");
			++count; av_frame_unref(frame);
		}
		check(result == AVERROR(EAGAIN) || result == AVERROR_EOF, "OSR_H264_DECODE_RECEIVE");
	};
	for (const auto &owned : packets) {
		check(owned.dts > last_dts && owned.dts <= owned.pts, "OSR_H264_PACKET_TIMESTAMPS");
		last_dts = owned.dts; bytes += owned.data.size(); keys += owned.keyframe;
		check(av_new_packet(packet, static_cast<int>(owned.data.size())) == 0, "OSR_H264_PACKET_ALLOC");
		std::memcpy(packet->data, owned.data.data(), owned.data.size());
		packet->pts = owned.pts; packet->dts = owned.dts;
		check(avcodec_send_packet(decoder, packet) == 0, "OSR_H264_DECODE_SEND");
		av_packet_unref(packet); receive();
	}
	check(avcodec_send_packet(decoder, nullptr) == 0, "OSR_H264_DECODE_FLUSH"); receive();
	check(count == 180 && keys >= 2 && keys < 10, "OSR_H264_DECODE_COUNT_GOP");
	av_packet_free(&packet); av_frame_free(&frame); avcodec_free_context(&decoder);
	std::printf("OSR_H264_HARDWARE_PROBE_PASS frames=%u bytes=%llu keyframes=%u\n", count,
	            static_cast<unsigned long long>(bytes), keys);
}
