/*
 * Copyright (c) 2026 Nakata Maho
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "recording_mp4_sink.hpp"
#if !VAEG_ENABLE_LOCAL_MP4_ENCODERS
#error "MP4_LOCAL_BUILD_ONLY"
#endif
#include <algorithm>
#include <cstring>
#include <deque>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
}
namespace vaeg::recording {
namespace {
void check(int result, const char *code) {
	if (result < 0)
		throw std::runtime_error(code);
}
class LocalSink final : public Mp4Sink {
	Mp4Config config_;
	AVFormatContext *format_ = nullptr;
	AVCodecContext *video_ = nullptr, *audio_ = nullptr;
	AVStream *vs_ = nullptr, *as_ = nullptr;
	AVFrame *vf_ = av_frame_alloc(), *af_ = av_frame_alloc();
	AVPacket *packet_ = av_packet_alloc();
	std::vector<uint8_t> nv12_;
	std::vector<int16_t> pending_;
	std::deque<std::pair<int64_t, int64_t>> video_durations_;
	int64_t audio_pts_ = AV_NOPTS_VALUE;
	void drain(AVCodecContext *codec, AVStream *stream) {
		for (;;) {
			mp4_check_cancel(config_);
			const int result = avcodec_receive_packet(codec, packet_);
			if (result == AVERROR(EAGAIN) || result == AVERROR_EOF)
				return;
			check(result, "MP4_LOCAL_ENCODE");
			if (codec == video_) {
				if (video_durations_.empty() || video_durations_.front().first != packet_->pts)
					throw std::runtime_error("MP4_LOCAL_VIDEO_TIMESTAMP");
				packet_->duration = video_durations_.front().second;
				video_durations_.pop_front();
			}
			av_packet_rescale_ts(packet_, codec->time_base, stream->time_base);
			packet_->stream_index = stream->index;
			check(av_interleaved_write_frame(format_, packet_), "MP4_LOCAL_MUX");
			av_packet_unref(packet_);
		}
	}
	void audio_blocks(bool final) {
		size_t used = 0;
		while (pending_.size() - used >= size_t(audio_->frame_size) * 2 ||
		       (final && used < pending_.size())) {
			mp4_check_cancel(config_);
			const int frames =
			    int((std::min)(size_t(audio_->frame_size), (pending_.size() - used) / 2));
			check(av_frame_make_writable(af_), "MP4_LOCAL_AUDIO_BUFFER");
			af_->nb_samples = frames;
			af_->pts = audio_pts_;
			for (int channel = 0; channel < 2; ++channel) {
				float *p = reinterpret_cast<float *>(af_->data[channel]);
				for (int i = 0; i < frames; ++i)
					p[i] = pending_[used + size_t(i) * 2 + channel] / 32768.0f;
			}
			check(avcodec_send_frame(audio_, af_), "MP4_LOCAL_AUDIO_ENCODE");
			drain(audio_, as_);
			audio_pts_ += frames;
			used += size_t(frames) * 2;
		}
		pending_.erase(pending_.begin(), pending_.begin() + used);
	}

  public:
	explicit LocalSink(const Mp4Config &config) : config_(config) {
	}
	void open() {
		if (!vf_ || !af_ || !packet_)
			throw std::runtime_error("MP4_ALLOCATION");
		const AVCodec *vcodec = avcodec_find_encoder_by_name("libx264");
		const AVCodec *acodec = avcodec_find_encoder_by_name("aac");
		if (!vcodec || !acodec)
			throw std::runtime_error("MP4_LOCAL_ENCODER_UNAVAILABLE");
		check(avformat_alloc_output_context2(&format_, nullptr, "mp4", config_.path.c_str()),
		      "MP4_LOCAL_MUXER_UNAVAILABLE");
		video_ = avcodec_alloc_context3(vcodec);
		audio_ = avcodec_alloc_context3(acodec);
		if (!video_ || !audio_)
			throw std::runtime_error("MP4_ALLOCATION");
		video_->width = config_.width;
		video_->height = config_.height;
		video_->pix_fmt = AV_PIX_FMT_NV12;
		video_->time_base = AVRational{1, 10000000};
		video_->framerate = AVRational{60, 1};
		video_->sample_aspect_ratio = AVRational{1, 1};
		video_->gop_size = 120;
		video_->max_b_frames = 0;
		video_->thread_count = 2;
		video_->color_range = AVCOL_RANGE_MPEG;
		video_->colorspace = AVCOL_SPC_BT709;
		video_->color_primaries = AVCOL_PRI_BT709;
		video_->color_trc = AVCOL_TRC_BT709;
		video_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		check(av_opt_set(video_->priv_data, "preset", "medium", 0), "MP4_LOCAL_OPTIONS");
		check(av_opt_set(video_->priv_data, "crf", "20", 0), "MP4_LOCAL_OPTIONS");
		check(avcodec_open2(video_, vcodec, nullptr), "MP4_LOCAL_VIDEO_OPEN");
		audio_->sample_rate = 48000;
		audio_->sample_fmt = AV_SAMPLE_FMT_FLTP;
		audio_->time_base = AVRational{1, 48000};
		audio_->bit_rate = 128000;
		av_channel_layout_default(&audio_->ch_layout, 2);
		audio_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		check(avcodec_open2(audio_, acodec, nullptr), "MP4_LOCAL_AUDIO_OPEN");
		if (audio_->frame_size <= 0 || audio_->frame_size > 8192)
			throw std::runtime_error("MP4_LOCAL_AUDIO_LAYOUT");
		vs_ = avformat_new_stream(format_, nullptr);
		as_ = avformat_new_stream(format_, nullptr);
		if (!vs_ || !as_)
			throw std::runtime_error("MP4_ALLOCATION");
		vs_->time_base = video_->time_base;
		vs_->avg_frame_rate = video_->framerate;
		as_->time_base = audio_->time_base;
		check(avcodec_parameters_from_context(vs_->codecpar, video_), "MP4_LOCAL_PARAMETERS");
		check(avcodec_parameters_from_context(as_->codecpar, audio_), "MP4_LOCAL_PARAMETERS");
		check(avio_open(&format_->pb, config_.path.c_str(), AVIO_FLAG_WRITE), "MP4_LOCAL_OPEN");
		AVDictionary *options = nullptr;
		av_dict_set(&options, "movflags", "+faststart", 0);
		const int result = avformat_write_header(format_, &options);
		av_dict_free(&options);
		check(result, "MP4_LOCAL_HEADER");
		vf_->format = video_->pix_fmt;
		vf_->width = video_->width;
		vf_->height = video_->height;
		check(av_frame_get_buffer(vf_, 32), "MP4_LOCAL_VIDEO_BUFFER");
		af_->format = audio_->sample_fmt;
		af_->sample_rate = 48000;
		af_->nb_samples = audio_->frame_size;
		check(av_channel_layout_copy(&af_->ch_layout, &audio_->ch_layout), "MP4_ALLOCATION");
		check(av_frame_get_buffer(af_, 0), "MP4_LOCAL_AUDIO_BUFFER");
	}
	~LocalSink() override {
		av_frame_free(&vf_);
		av_frame_free(&af_);
		av_packet_free(&packet_);
		avcodec_free_context(&video_);
		avcodec_free_context(&audio_);
		if (format_) {
			if (format_->pb)
				avio_closep(&format_->pb);
			avformat_free_context(format_);
		}
	}
	void video(const uint8_t *bgra, int64_t pts, int64_t duration) override {
		mp4_check_cancel(config_);
		mp4_bgra_to_nv12(bgra, config_.width, config_.height, nv12_);
		check(av_frame_make_writable(vf_), "MP4_LOCAL_VIDEO_BUFFER");
		for (int y = 0; y < config_.height; ++y)
			std::memcpy(vf_->data[0] + y * vf_->linesize[0],
			            nv12_.data() + size_t(y) * config_.width, config_.width);
		const size_t uv = size_t(config_.width) * config_.height;
		for (int y = 0; y < config_.height / 2; ++y)
			std::memcpy(vf_->data[1] + y * vf_->linesize[1],
			            nv12_.data() + uv + size_t(y) * config_.width, config_.width);
		vf_->pts = pts;
		vf_->duration = duration;
		if (video_durations_.size() >= 256)
			throw std::runtime_error("MP4_LOCAL_VIDEO_QUEUE_BOUNDS");
		video_durations_.emplace_back(pts, duration);
		check(avcodec_send_frame(video_, vf_), "MP4_LOCAL_VIDEO_ENCODE");
		drain(video_, vs_);
	}
	void audio(const int16_t *pcm, size_t frames, int64_t pts) override {
		if (audio_pts_ == AV_NOPTS_VALUE)
			audio_pts_ = av_rescale_q(pts, AVRational{1, 10000000}, audio_->time_base);
		pending_.insert(pending_.end(), pcm, pcm + frames * 2);
		audio_blocks(false);
	}
	void finish() override {
		audio_blocks(true);
		check(avcodec_send_frame(video_, nullptr), "MP4_LOCAL_FLUSH");
		drain(video_, vs_);
		if (!video_durations_.empty())
			throw std::runtime_error("MP4_LOCAL_VIDEO_FLUSH_INCOMPLETE");
		check(avcodec_send_frame(audio_, nullptr), "MP4_LOCAL_FLUSH");
		drain(audio_, as_);
		check(av_write_trailer(format_), "MP4_LOCAL_TRAILER");
		check(avio_closep(&format_->pb), "MP4_LOCAL_CLOSE");
	}
};
}
std::unique_ptr<Mp4Sink> create_local_mp4_sink(const Mp4Config &config) {
	auto sink = std::make_unique<LocalSink>(config);
	sink->open();
	return sink;
}
}
