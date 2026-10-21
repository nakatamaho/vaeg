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
#include "recording_mp4.h"
#include "recording_mp4_sink.hpp"
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstring>
#include <deque>
#include <filesystem>
#include <mutex>
#include <thread>
#ifdef _WIN32
#include <windows.h>
#else
#include <cerrno>
#include <sys/stat.h>
#endif
#if VAEG_ENABLE_RECORDING
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libswresample/swresample.h>
}
#endif

namespace vaeg::recording {
void mp4_bgra_to_nv12(const uint8_t *bgra, int width, int height, std::vector<uint8_t> &nv12) {
	const size_t pixels = static_cast<size_t>(width) * height;
	nv12.resize(pixels + pixels / 2);
	for (int y = 0; y < height; y += 2) {
		for (int x = 0; x < width; x += 2) {
			int r = 0, g = 0, b = 0;
			for (int dy = 0; dy != 2; ++dy) {
				for (int dx = 0; dx != 2; ++dx) {
					const size_t i = static_cast<size_t>(y + dy) * width + x + dx;
					const uint8_t *p = bgra + i * 4;
					nv12[i] = static_cast<uint8_t>(std::clamp(
					    ((47 * p[2] + 157 * p[1] + 16 * p[0] + 128) >> 8) + 16, 16, 235));
					r += p[2];
					g += p[1];
					b += p[0];
				}
			}
			r = (r + 2) / 4;
			g = (g + 2) / 4;
			b = (b + 2) / 4;
			const size_t uv = pixels + static_cast<size_t>(y / 2) * width + x;
			nv12[uv] = static_cast<uint8_t>(
			    std::clamp(((-26 * r - 86 * g + 112 * b + 128) >> 8) + 128, 16, 240));
			nv12[uv + 1] = static_cast<uint8_t>(
			    std::clamp(((112 * r - 102 * g - 10 * b + 128) >> 8) + 128, 16, 240));
		}
	}
}
#if !defined(__APPLE__) && !defined(_WIN32)
std::unique_ptr<Mp4Sink> create_native_mp4_sink(const Mp4Config &) {
	return nullptr;
}
#endif
}

namespace {
namespace fs = std::filesystem;
using namespace vaeg::recording;
template <size_t N> void text(char (&out)[N], const std::string &value) {
	std::snprintf(out, N, "%s", value.c_str());
}
[[noreturn]] void fail(const char *code) {
	throw std::runtime_error(code);
}

#if VAEG_ENABLE_RECORDING
struct Input {
	AVFormatContext *format = nullptr;
	AVCodecContext *video = nullptr, *audio = nullptr;
	AVPacket *packet = av_packet_alloc();
	AVFrame *frame = av_frame_alloc();
	SwrContext *resampler = nullptr;
	~Input() {
		swr_free(&resampler);
		av_frame_free(&frame);
		av_packet_free(&packet);
		avcodec_free_context(&video);
		avcodec_free_context(&audio);
		avformat_close_input(&format);
	}
};
class TempDirectory {
	fs::path dir_;

  public:
	~TempDirectory() {
		if (!dir_.empty()) {
			std::error_code ec;
			fs::remove(dir_ / "output.mp4", ec);
			fs::remove(dir_, ec); // Never recursively delete unexpected files.
		}
	}
	fs::path create(const fs::path &target) {
		static std::atomic<unsigned> serial{0};
		for (unsigned tries = 0; tries < 128; ++tries) {
			fs::path candidate = target;
			candidate +=
			    ".vaeg-tmp-" +
			    std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "-" +
			    std::to_string(serial.fetch_add(1));
			std::error_code ec;
#ifdef _WIN32
			const bool created = fs::create_directory(candidate, ec);
#else
			// Owner-only permissions at creation, not a permissive mkdir/chmod window.
			const bool created = ::mkdir(candidate.c_str(), 0700) == 0;
			if (!created)
				ec = std::error_code(errno, std::generic_category());
#endif
			if (created) {
				dir_ = candidate;
#ifndef _WIN32
				fs::permissions(dir_, fs::perms::owner_all, fs::perm_options::replace, ec);
				if (ec)
					fail("MP4_TEMP_PERMISSIONS");
#endif
				return dir_ / "output.mp4";
			}
			if (ec && ec != std::errc::file_exists)
				fail("MP4_TEMP_CREATE");
		}
		fail("MP4_TEMP_CREATE");
	}
};
void publish(const fs::path &temp, const fs::path &target) {
#ifdef _WIN32
	if (!MoveFileExW(temp.c_str(), target.c_str(), MOVEFILE_WRITE_THROUGH)) {
		if (GetLastError() == ERROR_ALREADY_EXISTS || GetLastError() == ERROR_FILE_EXISTS)
			fail("MP4_OUTPUT_EXISTS");
		fail("MP4_PUBLISH_FAILED");
	}
#else
	// Atomic no-replacement publication. No unsafe rename/copy fallback.
	std::error_code ec;
	fs::create_hard_link(temp, target, ec);
	if (ec == std::errc::file_exists)
		fail("MP4_OUTPUT_EXISTS");
	if (ec)
		fail("MP4_PUBLISH_FAILED");
#endif
}
AVCodecContext *decoder(AVStream *stream, AVCodecID expected) {
	if (stream->codecpar->codec_id != expected)
		fail("MP4_INPUT_CODEC");
	const AVCodec *codec = avcodec_find_decoder(expected);
	if (!codec)
		fail("MP4_DECODER_UNAVAILABLE");
	AVCodecContext *context = avcodec_alloc_context3(codec);
	if (!context)
		fail("MP4_ALLOCATION");
	if (avcodec_parameters_to_context(context, stream->codecpar) < 0) {
		avcodec_free_context(&context);
		fail("MP4_INPUT_PARAMETERS");
	}
	context->thread_count = 1;
	context->max_pixels = 16 * 1024 * 1024;
	if (avcodec_open2(context, codec, nullptr) < 0) {
		avcodec_free_context(&context);
		fail("MP4_DECODER_OPEN");
	}
	return context;
}
int64_t time100ns(int64_t pts, AVRational base) {
	if (pts == AV_NOPTS_VALUE || pts < 0 || base.num <= 0 || base.den <= 0)
		fail("MP4_INPUT_TIMESTAMP");
	const auto result = av_rescale_q(pts, base, AVRational{1, 10000000});
	if (result < 0 || result > INT64_MAX / 2)
		fail("MP4_INPUT_TIMESTAMP");
	return result;
}
#endif

class Exporter {
	std::mutex mutex_;
	std::condition_variable wake_;
	std::deque<std::string> queue_;
	std::thread thread_;
	bool stopping_ = false, working_ = false;
	std::atomic<bool> cancel_{false};
	VAEG_MP4_STATUS status_{};
	int64_t video_progress_ = 0, audio_progress_ = 0;
	void progress(int64_t end, int64_t duration, bool video) {
		std::lock_guard<std::mutex> lock(mutex_);
		int64_t &track = video ? video_progress_ : audio_progress_;
		track = (std::max)(track, end);
		status_.encoded_ms = (std::min)(video_progress_, audio_progress_) / 10000;
		status_.progress =
		    duration > 0 ? (std::min)(0.99, status_.encoded_ms * 10000.0 / duration) : -1.0;
	}
	void convert(const std::string &source) {
#if VAEG_ENABLE_RECORDING
		if (!vaeg_recording_mp4_available())
			fail("MP4_BACKEND_UNAVAILABLE");
		if (source.empty() || source.find("://") != std::string::npos)
			fail("MP4_INPUT_PATH");
		const fs::path input_path = fs::u8path(source);
		std::string ext = input_path.extension().u8string();
		for (char &c : ext)
			if (c >= 'A' && c <= 'Z')
				c += 'a' - 'A';
		if (ext != ".mkv" || !fs::is_regular_file(input_path))
			fail("MP4_INPUT_PATH");
		fs::path target = input_path;
		target.replace_extension(".mp4");
		{
			std::lock_guard<std::mutex> lock(mutex_);
			text(status_.output, target.u8string());
		}
		if (fs::symlink_status(target).type() != fs::file_type::not_found)
			fail("MP4_OUTPUT_EXISTS");
		Input in;
		if (!in.packet || !in.frame)
			fail("MP4_ALLOCATION");
		AVDictionary *options = nullptr;
		av_dict_set(&options, "protocol_whitelist", "file", 0);
		av_dict_set(&options, "format_whitelist", "matroska,webm", 0);
		const int opened = avformat_open_input(&in.format, source.c_str(), nullptr, &options);
		av_dict_free(&options);
		if (opened < 0)
			fail("MP4_INPUT_OPEN");
		if (in.format->nb_streams != 2)
			fail("MP4_INPUT_STREAMS");
		int vi = -1, ai = -1;
		for (unsigned i = 0; i < in.format->nb_streams; ++i) {
			const AVCodecParameters *p = in.format->streams[i]->codecpar;
			if (p->codec_type == AVMEDIA_TYPE_VIDEO && vi < 0) {
				if (p->codec_id != AV_CODEC_ID_FFV1)
					fail("MP4_INPUT_CODEC");
				if (p->width <= 0 || p->height <= 0 || p->width > 8192 || p->height > 8192 ||
				    int64_t(p->width) * p->height > 16 * 1024 * 1024)
					fail("MP4_INPUT_PARAMETERS");
				vi = static_cast<int>(i);
			} else if (p->codec_type == AVMEDIA_TYPE_AUDIO && ai < 0) {
				if (p->codec_id != AV_CODEC_ID_PCM_S16LE)
					fail("MP4_INPUT_CODEC");
				ai = static_cast<int>(i);
			} else
				fail("MP4_INPUT_STREAMS");
		}
		if (vi < 0 || ai < 0)
			fail("MP4_INPUT_STREAMS");
		if (avformat_find_stream_info(in.format, nullptr) < 0)
			fail("MP4_INPUT_OPEN");
		AVStream *vs = in.format->streams[vi], *as = in.format->streams[ai];
		in.video = decoder(vs, AV_CODEC_ID_FFV1);
		in.audio = decoder(as, AV_CODEC_ID_PCM_S16LE);
		const int width = in.video->width, height = in.video->height;
		if (width <= 0 || height <= 0 || width > 8192 || height > 8192 ||
		    int64_t(width) * height > 16 * 1024 * 1024 || in.audio->ch_layout.nb_channels != 2 ||
		    in.audio->sample_rate < 8000 || in.audio->sample_rate > 192000)
			fail("MP4_INPUT_PARAMETERS");
		const double scale = (std::min)({1.0, 1280.0 / width, 720.0 / height});
		const int out_width = (std::max)(2, int(width * scale) & ~1);
		const int out_height = (std::max)(2, int(height * scale) & ~1);
		TempDirectory temporary;
		const fs::path temp = temporary.create(target);
		const Mp4Config config{temp.u8string(), out_width, out_height, &cancel_};
		mp4_check_cancel(config);
		auto sink = create_native_mp4_sink(config);
#if VAEG_ENABLE_LOCAL_MP4_ENCODERS
		if (!sink)
			sink = create_local_mp4_sink(config);
#endif
		if (!sink)
			fail("MP4_BACKEND_UNAVAILABLE");
		AVChannelLayout stereo = AV_CHANNEL_LAYOUT_STEREO;
		if (swr_alloc_set_opts2(&in.resampler, &stereo, AV_SAMPLE_FMT_S16, 48000,
		                        &in.audio->ch_layout, AV_SAMPLE_FMT_S16, in.audio->sample_rate, 0,
		                        nullptr) < 0 ||
		    swr_init(in.resampler) < 0)
			fail("MP4_RESAMPLER");
		std::vector<uint8_t> bgra(static_cast<size_t>(out_width) * out_height * 4);
		std::vector<int16_t> pcm;
		int64_t audio_origin = -1, audio_frames = 0, video_end = -1, audio_input_end = -1;
		uint64_t video_count = 0, audio_count = 0;
		const int64_t duration =
		    in.format->duration > 0
		        ? av_rescale_q(in.format->duration, AV_TIME_BASE_Q, AVRational{1, 10000000})
		        : -1;
		auto output_audio = [&](const AVFrame *frame) {
			const int n = frame ? frame->nb_samples : 0;
			const int capacity = swr_get_out_samples(in.resampler, n);
			if (capacity < 0 || capacity > 1024 * 1024)
				fail("MP4_AUDIO_BOUNDS");
			pcm.resize(static_cast<size_t>(capacity) * 2);
			uint8_t *out[] = {reinterpret_cast<uint8_t *>(pcm.data())};
			const uint8_t *data[] = {frame ? frame->data[0] : nullptr};
			const int got = swr_convert(in.resampler, out, capacity, frame ? data : nullptr, n);
			if (got < 0)
				fail("MP4_RESAMPLER");
			if (got > 0) {
				const int64_t pts = audio_origin + av_rescale_q(audio_frames, AVRational{1, 48000},
				                                                AVRational{1, 10000000});
				sink->audio(pcm.data(), static_cast<size_t>(got), pts);
				audio_frames += got;
				progress(audio_origin + av_rescale_q(audio_frames, AVRational{1, 48000},
				                                     AVRational{1, 10000000}),
				         duration, false);
			}
			return got;
		};
		auto drain = [&](AVCodecContext *context, bool is_video, AVFrame *decoded) {
			for (;;) {
				mp4_check_cancel(config);
				const int result = avcodec_receive_frame(context, decoded);
				if (result == AVERROR(EAGAIN) || result == AVERROR_EOF)
					break;
				if (result < 0)
					fail("MP4_DECODE");
				const AVFrame *f = decoded;
				const int64_t pts =
				    time100ns(f->best_effort_timestamp, is_video ? vs->time_base : as->time_base);
				if (is_video) {
					if (f->width != width || f->height != height ||
					    (f->format != AV_PIX_FMT_BGRA && f->format != AV_PIX_FMT_BGR0) ||
					    std::abs(f->linesize[0]) < width * 4 || pts < video_end)
						fail("MP4_VIDEO_LAYOUT");
					for (int y = 0; y < out_height; ++y) {
						const uint8_t *row =
						    f->data[0] + (int64_t(y) * height / out_height) * f->linesize[0];
						for (int x = 0; x < out_width; ++x) {
							uint8_t *p = bgra.data() + (static_cast<size_t>(y) * out_width + x) * 4;
							std::memcpy(p, row + (int64_t(x) * width / out_width) * 4, 3);
							p[3] = 255;
						}
					}
					const int64_t interval =
					    f->duration > 0 ? time100ns(f->duration, vs->time_base) : 10000000 / 60;
					sink->video(bgra.data(), pts, interval);
					video_end = pts + interval;
					++video_count;
					progress(video_end, duration, true);
				} else {
					if (f->format != AV_SAMPLE_FMT_S16 || f->ch_layout.nb_channels != 2 ||
					    f->sample_rate != in.audio->sample_rate || f->nb_samples <= 0 ||
					    (audio_input_end >= 0 && std::abs(pts - audio_input_end) > 20000))
						fail("MP4_AUDIO_LAYOUT");
					if (audio_origin < 0)
						audio_origin = pts;
					audio_input_end = pts + int64_t(f->nb_samples) * 10000000 / f->sample_rate;
					output_audio(f);
					++audio_count;
				}
				av_frame_unref(decoded);
			}
		};
		auto read_tracks = [&](Input &reader, bool video, bool audio) {
			int result;
			while ((result = av_read_frame(reader.format, reader.packet)) >= 0) {
				mp4_check_cancel(config);
				const int index = reader.packet->stream_index;
				if ((video && index == vi) || (audio && index == ai)) {
					AVCodecContext *context = index == vi ? in.video : in.audio;
					if (avcodec_send_packet(context, reader.packet) < 0)
						fail("MP4_DECODE");
					drain(context, index == vi, reader.frame);
				}
				av_packet_unref(reader.packet);
			}
			if (result != AVERROR_EOF)
				fail("MP4_INPUT_READ");
			if (video) {
				if (avcodec_send_packet(in.video, nullptr) < 0)
					fail("MP4_DECODE_FLUSH");
				drain(in.video, true, reader.frame);
				sink->end_video();
			}
			if (audio) {
				if (avcodec_send_packet(in.audio, nullptr) < 0)
					fail("MP4_DECODE_FLUSH");
				drain(in.audio, false, reader.frame);
				while (output_audio(nullptr) > 0)
					mp4_check_cancel(config);
				sink->end_audio();
			}
		};
		if (sink->parallel_inputs()) {
			// AVAssetWriter needs independently scheduled producers: waiting for
			// audio readiness on an interleaved single producer can starve video.
			// Read the same finalized file twice instead of unbounded packet queues.
			Input audio_reader;
			if (!audio_reader.packet || !audio_reader.frame)
				fail("MP4_ALLOCATION");
			AVDictionary *opts = nullptr;
			av_dict_set(&opts, "protocol_whitelist", "file", 0);
			av_dict_set(&opts, "format_whitelist", "matroska,webm", 0);
			const int opened_audio =
			    avformat_open_input(&audio_reader.format, source.c_str(), nullptr, &opts);
			av_dict_free(&opts);
			if (opened_audio < 0)
				fail("MP4_INPUT_OPEN");
			std::mutex error_mutex;
			std::exception_ptr first_error;
			auto track = [&](Input &reader, bool video) {
				try {
					read_tracks(reader, video, !video);
				} catch (...) {
					std::lock_guard<std::mutex> lock(error_mutex);
					if (!first_error)
						first_error = std::current_exception();
					cancel_.store(true);
				}
			};
			std::thread audio_thread([&] {
				track(audio_reader, false);
			});
			track(in, true);
			audio_thread.join();
			if (first_error)
				std::rethrow_exception(first_error);
		} else {
			read_tracks(in, true, true);
		}
		if (!video_count || !audio_count)
			fail("MP4_INPUT_EMPTY");
		mp4_check_cancel(config);
		sink->finish();
		sink.reset();
		mp4_check_cancel(config);
		publish(temp, target);
#else
		(void)source;
		fail("MP4_BACKEND_UNAVAILABLE");
#endif
	}
	void run() {
		for (;;) {
			std::string source;
			{
				std::unique_lock<std::mutex> lock(mutex_);
				wake_.wait(lock, [&] {
					return stopping_ || !queue_.empty();
				});
				if (stopping_)
					return;
				source = std::move(queue_.front());
				queue_.pop_front();
				working_ = true;
				cancel_.store(false);
				status_.state = VAEG_MP4_RUNNING;
				status_.progress = 0;
				status_.encoded_ms = 0;
				video_progress_ = audio_progress_ = 0;
				status_.error[0] = status_.output[0] = '\0';
				text(status_.source, source);
			}
			std::string error;
			try {
				convert(source);
			} catch (const std::exception &e) {
				error = e.what();
			} catch (...) {
				error = "MP4_INTERNAL";
			}
			{
				std::lock_guard<std::mutex> lock(mutex_);
				working_ = false;
				status_.state = error.empty()              ? VAEG_MP4_SUCCEEDED
				                : error == "MP4_CANCELLED" ? VAEG_MP4_CANCELLED
				                                           : VAEG_MP4_FAILED;
				if (error.empty())
					status_.progress = 1;
				text(status_.error, error);
			}
		}
	}

  public:
	~Exporter() {
		shutdown();
	}
	int enqueue(const char *source) {
		std::lock_guard<std::mutex> lock(mutex_);
		if (!vaeg_recording_mp4_available()) {
			text(status_.notice, "MP4_BACKEND_UNAVAILABLE");
			return 0;
		}
		if (!source || !*source) {
			text(status_.notice, "MP4_INPUT_PATH");
			return 0;
		}
		size_t length = 0;
		while (length < 4096 && source[length])
			++length;
		if (length == 4096) {
			text(status_.notice, "MP4_INPUT_PATH_TOO_LONG");
			return 0;
		}
		if (stopping_) {
			text(status_.notice, "MP4_SHUTDOWN");
			return 0;
		}
		if (queue_.size() >= 4) {
			text(status_.notice, "MP4_QUEUE_FULL: original MKV retained");
			return 0;
		}
		if ((working_ && source == std::string(status_.source)) ||
		    std::find(queue_.begin(), queue_.end(), source) != queue_.end()) {
			text(status_.notice, "MP4_ALREADY_QUEUED");
			return 0;
		}
		if (!thread_.joinable())
			thread_ = std::thread(&Exporter::run, this);
		queue_.emplace_back(source);
		status_.notice[0] = '\0';
		wake_.notify_one();
		return 1;
	}
	void get(VAEG_MP4_STATUS *out) {
		std::lock_guard<std::mutex> lock(mutex_);
		*out = status_;
		out->pending = static_cast<unsigned>(queue_.size());
	}
	void cancel() {
		std::lock_guard<std::mutex> lock(mutex_);
		queue_.clear();
		cancel_.store(true);
		if (!working_) {
			status_.state = VAEG_MP4_CANCELLED;
			text(status_.error, "MP4_CANCELLED");
		}
	}
	void shutdown() {
		{
			std::lock_guard<std::mutex> lock(mutex_);
			stopping_ = true;
			cancel_.store(true);
			queue_.clear();
			wake_.notify_all();
		}
		if (thread_.joinable())
			thread_.join();
	}
};
Exporter &exporter() {
	static Exporter value;
	return value;
}
}

extern "C" int vaeg_recording_mp4_available(void) {
#if VAEG_ENABLE_RECORDING &&                                                                       \
    (defined(__APPLE__) || defined(_WIN32) || VAEG_ENABLE_LOCAL_MP4_ENCODERS)
	return 1;
#else
	return 0;
#endif
}
extern "C" const char *vaeg_recording_mp4_backend(void) {
#if !VAEG_ENABLE_RECORDING
	return "unavailable (recording OFF)";
#elif defined(__APPLE__)
	return "macOS AVFoundation / VideoToolbox";
#elif defined(_WIN32)
	return "Windows Media Foundation";
#elif VAEG_ENABLE_LOCAL_MP4_ENCODERS
	return "LOCAL ONLY: static FFmpeg / libx264 / AAC (not for distribution)";
#else
	return "unavailable (Linux local-only encoders OFF)";
#endif
}
extern "C" int vaeg_recording_mp4_enqueue(const char *source) {
	try {
		return exporter().enqueue(source);
	} catch (...) {
		return 0;
	}
}
extern "C" void vaeg_recording_mp4_get_status(VAEG_MP4_STATUS *out) {
	if (out) {
		try {
			exporter().get(out);
		} catch (...) {
			*out = {};
			out->state = VAEG_MP4_FAILED;
			text(out->error, "MP4_INTERNAL");
		}
	}
}
extern "C" void vaeg_recording_mp4_cancel(void) {
	try {
		exporter().cancel();
	} catch (...) {
	}
}
extern "C" void vaeg_recording_mp4_shutdown(void) {
	try {
		exporter().shutdown();
	} catch (...) {
	}
}
