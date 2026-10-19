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
#include "recording/recording_ffmpeg_capabilities.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavutil/opt.h>
#include <libavutil/pixfmt.h>
#include <libavutil/samplefmt.h>
}

#include <cstring>

namespace vaeg::recording {
namespace {

constexpr std::uint32_t kMinimumAvutilMajor = 58u;
constexpr std::uint32_t kMinimumAvcodecMajor = 60u;
constexpr std::uint32_t kMinimumAvformatMajor = 60u;

FfmpegVersion decode_version(unsigned version) noexcept {
	return {static_cast<std::uint32_t>((version >> 16) & 0xffu),
	        static_cast<std::uint32_t>((version >> 8) & 0xffu),
	        static_cast<std::uint32_t>(version & 0xffu)};
}

bool version_at_least(const FfmpegVersion &version, std::uint32_t major) noexcept {
	return version.major >= major;
}

bool codec_supports_pixel_format(const AVCodec *codec, enum AVPixelFormat wanted) noexcept {
	if (codec == nullptr) {
		return false;
	}
#if LIBAVCODEC_VERSION_MAJOR >= 61
	const void *configs = nullptr;
	int count = 0;
	if (avcodec_get_supported_config(nullptr, codec, AV_CODEC_CONFIG_PIX_FORMAT, 0, &configs,
	                                 &count) < 0) {
		return false;
	}
	if (configs == nullptr) {
		return true;
	}
	const auto *formats = static_cast<const enum AVPixelFormat *>(configs);
	for (int index = 0; index < count; ++index) {
		if (formats[index] == wanted) {
			return true;
		}
	}
	return false;
#else
	if (codec->pix_fmts == nullptr) {
		return true;
	}
	for (const enum AVPixelFormat *format = codec->pix_fmts; *format != AV_PIX_FMT_NONE; ++format) {
		if (*format == wanted) {
			return true;
		}
	}
	return false;
#endif
}

bool codec_supports_sample_format(const AVCodec *codec, enum AVSampleFormat wanted) noexcept {
	if (codec == nullptr) {
		return false;
	}
#if LIBAVCODEC_VERSION_MAJOR >= 61
	const void *configs = nullptr;
	int count = 0;
	if (avcodec_get_supported_config(nullptr, codec, AV_CODEC_CONFIG_SAMPLE_FORMAT, 0, &configs,
	                                 &count) < 0) {
		return false;
	}
	if (configs == nullptr) {
		return true;
	}
	const auto *formats = static_cast<const enum AVSampleFormat *>(configs);
	for (int index = 0; index < count; ++index) {
		if (formats[index] == wanted) {
			return true;
		}
	}
	return false;
#else
	if (codec->sample_fmts == nullptr) {
		return true;
	}
	for (const enum AVSampleFormat *format = codec->sample_fmts; *format != AV_SAMPLE_FMT_NONE;
	     ++format) {
		if (*format == wanted) {
			return true;
		}
	}
	return false;
#endif
}

bool probe_ffv1_options(const AVCodec *codec, bool *slice_crc, bool *version3) noexcept {
	AVCodecContext *context;
	const AVOption *slice_crc_option;
	AVDictionary *options = nullptr;
	int open_result;

	if (slice_crc == nullptr || version3 == nullptr || codec == nullptr) {
		return false;
	}
	*slice_crc = false;
	*version3 = false;
	context = avcodec_alloc_context3(codec);
	if (context == nullptr) {
		return false;
	}
	context->width = 2;
	context->height = 2;
	context->pix_fmt = AV_PIX_FMT_BGRA;
	context->time_base = AVRational{1, 60};
	context->framerate = AVRational{60, 1};
	context->gop_size = 1;
	context->level = 3;
	slice_crc_option = av_opt_find(context, "slicecrc", nullptr, 0, AV_OPT_SEARCH_CHILDREN);
	if (slice_crc_option != nullptr &&
	    av_opt_set_int(context, "slicecrc", 1, AV_OPT_SEARCH_CHILDREN) >= 0) {
		*slice_crc = true;
	}
	open_result = avcodec_open2(context, codec, &options);
	*version3 = open_result >= 0 && context->level == 3;
	av_dict_free(&options);
	avcodec_free_context(&context);
	return open_result >= 0;
}

} // namespace

const char *ffmpeg_capability_error_name(FfmpegCapabilityError error) noexcept {
	switch (error) {
	case FfmpegCapabilityError::None:
		return "none";
	case FfmpegCapabilityError::UnsupportedVersion:
		return "unsupported_version";
	case FfmpegCapabilityError::MissingFfv1Encoder:
		return "missing_ffv1_encoder";
	case FfmpegCapabilityError::MissingPcmS16leEncoder:
		return "missing_pcm_s16le_encoder";
	case FfmpegCapabilityError::MissingMatroskaMuxer:
		return "missing_matroska_muxer";
	case FfmpegCapabilityError::MissingFfv1PixelFormat:
		return "missing_ffv1_pixel_format";
	case FfmpegCapabilityError::MissingPcmSampleFormat:
		return "missing_pcm_sample_format";
	case FfmpegCapabilityError::MissingFfv1SliceCrc:
		return "missing_ffv1_slice_crc";
	case FfmpegCapabilityError::MissingFfv1Version3:
		return "missing_ffv1_version3";
	case FfmpegCapabilityError::MuxerRejectsFfv1:
		return "muxer_rejects_ffv1";
	case FfmpegCapabilityError::MuxerRejectsPcmS16le:
		return "muxer_rejects_pcm_s16le";
	case FfmpegCapabilityError::ProbeFailure:
		return "probe_failure";
	default:
		return "unknown";
	}
}

FfmpegCapabilityError
evaluate_ffmpeg_capabilities(const FfmpegCapabilityInventory &inventory) noexcept {
	if (!inventory.ffv1_encoder) {
		return FfmpegCapabilityError::MissingFfv1Encoder;
	}
	if (!inventory.pcm_s16le_encoder) {
		return FfmpegCapabilityError::MissingPcmS16leEncoder;
	}
	if (!inventory.matroska_muxer) {
		return FfmpegCapabilityError::MissingMatroskaMuxer;
	}
	if (!inventory.ffv1_bgra) {
		return FfmpegCapabilityError::MissingFfv1PixelFormat;
	}
	if (!inventory.pcm_s16) {
		return FfmpegCapabilityError::MissingPcmSampleFormat;
	}
	if (!inventory.ffv1_slice_crc) {
		return FfmpegCapabilityError::MissingFfv1SliceCrc;
	}
	if (!inventory.ffv1_version3) {
		return FfmpegCapabilityError::MissingFfv1Version3;
	}
	if (!inventory.matroska_accepts_ffv1) {
		return FfmpegCapabilityError::MuxerRejectsFfv1;
	}
	if (!inventory.matroska_accepts_pcm_s16le) {
		return FfmpegCapabilityError::MuxerRejectsPcmS16le;
	}
	return FfmpegCapabilityError::None;
}

FfmpegProbeResult probe_ffmpeg_capabilities() noexcept {
	FfmpegProbeResult result{};
	const AVCodec *ffv1;
	const AVCodec *pcm_s16le;
	const AVOutputFormat *matroska;
	bool options_probe_ok;

	result.error = FfmpegCapabilityError::ProbeFailure;
	result.capabilities.avutil = decode_version(avutil_version());
	result.capabilities.avcodec = decode_version(avcodec_version());
	result.capabilities.avformat = decode_version(avformat_version());
	if (!version_at_least(result.capabilities.avutil, kMinimumAvutilMajor) ||
	    !version_at_least(result.capabilities.avcodec, kMinimumAvcodecMajor) ||
	    !version_at_least(result.capabilities.avformat, kMinimumAvformatMajor)) {
		result.error = FfmpegCapabilityError::UnsupportedVersion;
		return result;
	}

	ffv1 = avcodec_find_encoder(AV_CODEC_ID_FFV1);
	pcm_s16le = avcodec_find_encoder(AV_CODEC_ID_PCM_S16LE);
	matroska = av_guess_format("matroska", nullptr, nullptr);
	result.capabilities.inventory.ffv1_encoder = ffv1 != nullptr;
	result.capabilities.inventory.pcm_s16le_encoder = pcm_s16le != nullptr;
	result.capabilities.inventory.matroska_muxer = matroska != nullptr;
	result.capabilities.inventory.ffv1_bgra = codec_supports_pixel_format(ffv1, AV_PIX_FMT_BGRA);
	result.capabilities.inventory.pcm_s16 =
	    codec_supports_sample_format(pcm_s16le, AV_SAMPLE_FMT_S16);
	result.capabilities.inventory.matroska_accepts_ffv1 =
	    matroska != nullptr &&
	    avformat_query_codec(matroska, AV_CODEC_ID_FFV1, FF_COMPLIANCE_NORMAL) > 0;
	result.capabilities.inventory.matroska_accepts_pcm_s16le =
	    matroska != nullptr &&
	    avformat_query_codec(matroska, AV_CODEC_ID_PCM_S16LE, FF_COMPLIANCE_NORMAL) > 0;
	options_probe_ok = probe_ffv1_options(ffv1, &result.capabilities.inventory.ffv1_slice_crc,
	                                      &result.capabilities.inventory.ffv1_version3);
	if (!options_probe_ok) {
		result.error = FfmpegCapabilityError::ProbeFailure;
		return result;
	}
	result.error = evaluate_ffmpeg_capabilities(result.capabilities.inventory);
	return result;
}

} // namespace vaeg::recording
