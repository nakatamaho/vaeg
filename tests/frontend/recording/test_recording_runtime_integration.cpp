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
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "compiler.h"
#include "machine/pccore.h"
#include "recording/recording.h"
#include "recording/recording_native_session.h"
#include "recording/recording_native_tap.h"
#include "recording/recording_runtime.h"
#include "scrnmng.h"
#include "sound.h"
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#ifdef VAEG_RECORDING_METAL_PROBE
#include <SDL.h>
#include "imgui.h"
#include "librashader/metal_presenter.h"
#include "librashader/builtin_shaders.h"
#include <cerrno>
#include <fcntl.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
extern char **environ;
#endif

PCCORE pccore{};
namespace {
std::uint64_t clock_ticks, consumed_samples;
std::uint32_t width, height, generation;
bool audio_active;
unsigned audio_start_calls;
bool displayed_active;
VAEG_RECORDING_DISPLAYED_CAPTURE_CALLBACK displayed_callback;
void *displayed_context;
std::uint32_t displayed_token_bias;
VAEG_RECORDING_DISPLAYED_CAPTURE_ERROR displayed_error;
SOUNDSTREAMSTATE stream{};
void check(bool value, const char *code) {
	if (!value) {
		std::fprintf(stderr, "%s: %s\n", code, vaeg_recording_native_session_last_error());
		std::exit(1);
	}
}
void reset(unsigned w, unsigned h) {
	clock_ticks = consumed_samples = 0;
	width = w;
	height = h;
	generation = 77;
	audio_active = false;
	audio_start_calls = 0;
	displayed_active = false;
	displayed_callback = nullptr;
	displayed_context = nullptr;
	displayed_token_bias = 0;
	displayed_error = VAEG_RECORDING_DISPLAYED_CAPTURE_OK;
	stream = {};
	stream.rate = 48000;
	stream.generation_epoch = 3;
	pccore.realclock = 6000000;
	vaeg_recording_runtime_clear_event();
}
VAEG_RECORDING_NATIVE_TAP_ERROR frame(unsigned i) {
	clock_ticks = 125u + static_cast<std::uint64_t>(i) * 100000u;
	stream.lastclock = static_cast<UINT32>(clock_ticks);
	stream.generated_sample_count = 1u + i * 800u;
	std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width) * height * 3u,
	                                 static_cast<std::uint8_t>(40 + i));
	VAEG_RECORDING_NATIVE_FRAME f{};
	f.pixels = pixels.data();
	f.width = width;
	f.height = height;
	f.pitch_bytes = width * 3;
	f.size_bytes = pixels.size();
	f.sample_aspect_width = f.sample_aspect_height = 1;
	f.frame_token = i + 1;
	VAEG_RECORDING_OSD_SNAPSHOT s{};
	s.version = VAEG_RECORDING_OSD_SNAPSHOT_VERSION;
	s.frame_token = i + 1;
	s.guest_width = s.output_width = width;
	s.guest_height = s.output_height = height;
	return vaeg_recording_native_session_submit_frame(i + 1, clock_ticks, generation, &f, &s);
}
void validate_movie(const std::filesystem::path &path, unsigned w, unsigned h, unsigned frames) {
	AVFormatContext *ctx = nullptr;
	check(avformat_open_input(&ctx, path.string().c_str(), nullptr, nullptr) == 0,
	      "RUNTIME_INTEGRATION_OPEN_MOVIE");
	check(avformat_find_stream_info(ctx, nullptr) >= 0, "RUNTIME_INTEGRATION_STREAM_INFO");
	int video = av_find_best_stream(ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
	check(video >= 0 && ctx->streams[video]->codecpar->width == static_cast<int>(w) &&
	          ctx->streams[video]->codecpar->height == static_cast<int>(h),
	      "RUNTIME_INTEGRATION_FIXED_DESCRIPTOR");
	AVPacket *pkt = av_packet_alloc();
	check(pkt != nullptr, "RUNTIME_INTEGRATION_PACKET_ALLOC");
	unsigned count = 0;
	int status;
	while ((status = av_read_frame(ctx, pkt)) >= 0) {
		if (pkt->stream_index == video)
			++count;
		av_packet_unref(pkt);
	}
	check(status == AVERROR_EOF && count == frames, "RUNTIME_INTEGRATION_COMPLETE_INTERVAL_COUNT");
	av_packet_free(&pkt);
	avformat_close_input(&ctx);
}
/* Independent decode checks the joined identities, not just packet counts. */
void validate_content(const std::filesystem::path &path, unsigned first, unsigned count,
                      std::uint64_t first_sample, bool displayed = false,
                      const std::vector<std::vector<std::uint8_t>> *gpu_frames = nullptr) {
	AVFormatContext *ctx = nullptr;
	check(avformat_open_input(&ctx, path.string().c_str(), nullptr, nullptr) == 0 &&
	          avformat_find_stream_info(ctx, nullptr) >= 0,
	      "SPLIT_DECODE_OPEN");
	const auto *source = av_dict_get(ctx->metadata, "VAEG_RECORDING_SOURCE", nullptr, 0);
	check(source && std::string(source->value) == (displayed ? "displayed" : "native"),
	      "DECODE_SOURCE_METADATA");
	AVCodecContext *decoders[2]{};
	int streams[2];
	for (int k = 0; k < 2; ++k) {
		streams[k] = av_find_best_stream(ctx, k == 0 ? AVMEDIA_TYPE_VIDEO : AVMEDIA_TYPE_AUDIO, -1,
		                                 -1, nullptr, 0);
		check(streams[k] >= 0, "SPLIT_DECODE_STREAM");
		auto *par = ctx->streams[streams[k]]->codecpar;
		const AVCodec *codec = avcodec_find_decoder(par->codec_id);
		decoders[k] = avcodec_alloc_context3(codec);
		check(decoders[k] && avcodec_parameters_to_context(decoders[k], par) >= 0 &&
		          avcodec_open2(decoders[k], codec, nullptr) >= 0,
		      "SPLIT_DECODE_CODEC");
	}
	AVFrame *decoded = av_frame_alloc();
	AVPacket *packet = av_packet_alloc();
	check(decoded && packet, "SPLIT_DECODE_ALLOC");
	unsigned videos = 0;
	std::uint64_t samples = 0;
	auto receive = [&](int k) {
		int status;
		while ((status = avcodec_receive_frame(decoders[k], decoded)) >= 0) {
			if (k == 0) {
				check(decoded->format == AV_PIX_FMT_BGRA, "SPLIT_DECODE_BGRA");
				for (int y = 0; y < decoded->height; ++y) {
					const auto *row = decoded->data[0] + y * decoded->linesize[0];
					for (int x = 0; x < decoded->width; ++x) {
						for (int c = 0; c < 4; ++c) {
							const unsigned expected = c == 3 ? 255 : gpu_frames ?
							    gpu_frames->at(videos).at((y * decoded->width + x) * 4 + 2 - c) :
							    40 + first + videos + (displayed ? c * 20 + (x + 3 * y) % 17 : 0);
							check(row[x * 4 + c] == expected,
							      "SPLIT_DECODE_FRAME_IDENTITY_NO_DROP_DUP_OR_PADDING");
						}
					}
				}
				++videos;
			} else {
				check(decoded->format == AV_SAMPLE_FMT_S16 && decoded->ch_layout.nb_channels == 2,
				      "SPLIT_DECODE_PCM_FORMAT");
				const auto *pcm = reinterpret_cast<const std::int16_t *>(decoded->data[0]);
				for (int i = 0; i < decoded->nb_samples; ++i) {
					const auto value = static_cast<std::int16_t>(first_sample + samples++);
					check(pcm[i * 2] == value && pcm[i * 2 + 1] == -value,
					      "SPLIT_DECODE_AUDIO_SAMPLE_IDENTITY_NO_GAP_OR_DUP");
				}
			}
			av_frame_unref(decoded);
		}
		check(status == AVERROR(EAGAIN) || status == AVERROR_EOF, "SPLIT_DECODE_RECEIVE");
	};
	int status;
	while ((status = av_read_frame(ctx, packet)) >= 0) {
		for (int k = 0; k < 2; ++k) {
			if (packet->stream_index == streams[k]) {
				check(avcodec_send_packet(decoders[k], packet) >= 0, "SPLIT_DECODE_SEND");
				receive(k);
			}
		}
		av_packet_unref(packet);
	}
	check(status == AVERROR_EOF, "SPLIT_DECODE_EOF");
	for (int k = 0; k < 2; ++k) {
		check(avcodec_send_packet(decoders[k], nullptr) >= 0, "SPLIT_DECODE_FLUSH");
		receive(k);
		avcodec_free_context(&decoders[k]);
	}
	check(videos == count && samples == count * 800u, "SPLIT_DECODE_TOTALS");
	av_frame_free(&decoded);
	av_packet_free(&packet);
	avformat_close_input(&ctx);
}
std::vector<std::string> notified_paths;
void split_saved_notice(const char *path) {
	check(std::filesystem::is_regular_file(std::filesystem::u8path(path)),
	      "SPLIT_SAVED_ONLY_AFTER_PUBLISH");
	notified_paths.emplace_back(path);
}
void automatic_split(const std::filesystem::path &dir, unsigned kind) {
	notified_paths.clear();
	vaeg_recording_native_session_set_saved_callback(&split_saved_notice);
	const auto path = dir / ("auto-" + std::to_string(kind) + ".mkv");
	const auto next = dir / ("auto-" + std::to_string(kind) + "-part-002.mkv");
	reset(640, 400);
	check(vaeg_recording_native_session_start(path.string().c_str(), 0), "SPLIT_START");
	vaeg_recording_native_session_enable_auto_split();
	std::uint64_t first_sample = 0;
	for (unsigned i = 0; i < 8; ++i) {
		if (i == 4) {
			if (kind == 0) {
				width = 320;
				height = 224;
			} // DOS -> game geometry.
			if (kind == 2) {
				width = 400;
				height = 640;
			} // Rotation.
			if (kind != 3)
				++generation; // Equal-size timing-only change is kind1.
			else
				vaeg_recording_runtime_screen_changed(); // Explicit presentation request.
			check(vaeg_recording_native_session_is_active(), "SPLIT_REQUEST_DOES_NOT_STOP");
		}
		check(frame(i) == VAEG_RECORDING_NATIVE_TAP_OK, "SPLIT_TAP");
		check(vaeg_recording_runtime_step(0) == VAEG_RECORDING_RUNTIME_CONTINUE &&
		          vaeg_recording_native_session_is_active(),
		      "SPLIT_REC_STAYS_ACTIVE");
		if (i == 0)
			first_sample = consumed_samples;
	}
	check(audio_start_calls == 1 && consumed_samples == first_sample + 5600 &&
	          vaeg_recording_native_session_accepted_frames() == 7 &&
	          vaeg_recording_native_session_elapsed_ticks() == 700000,
	      "SPLIT_CONTINUOUS_AUDIO_CLOCK");
	check(vaeg_recording_native_session_stop(), "SPLIT_STOP");
	validate_movie(path, 640, 400, 4);
	validate_movie(next, width, height, 3);
	validate_content(path, 0, 4, first_sample);
	validate_content(next, 4, 3, first_sample + 3200);
	check(notified_paths == std::vector<std::string>{path.u8string(), next.u8string()},
	      "SPLIT_EACH_FINALIZED_SEGMENT_NOTIFIED_ONCE");
	vaeg_recording_native_session_set_saved_callback(nullptr);
}
void split_repeated_and_epoch(const std::filesystem::path &dir) {
	const auto path = dir / "repeat.mkv";
	reset(640, 400);
	check(vaeg_recording_native_session_start(path.string().c_str(), 0), "SPLIT_REPEAT_START");
	vaeg_recording_native_session_enable_auto_split();
	std::uint64_t first_sample = 0;
	for (unsigned i = 0; i < 12; ++i) {
		if (i == 4) {
			width = 320;
			height = 224;
			++generation;
		}
		if (i == 8) {
			width = 224;
			height = 320;
			++generation;
		}
		check(frame(i) == VAEG_RECORDING_NATIVE_TAP_OK, "SPLIT_REPEAT_FRAME");
		vaeg_recording_runtime_step(0);
		check(vaeg_recording_native_session_is_active(), "SPLIT_REPEAT_ACTIVE");
		if (i == 0)
			first_sample = consumed_samples;
	}
	check(vaeg_recording_native_session_stop() && audio_start_calls == 1, "SPLIT_REPEAT_STOP");
	validate_movie(path, 640, 400, 4);
	validate_movie(dir / "repeat-part-002.mkv", 320, 224, 4);
	validate_movie(dir / "repeat-part-003.mkv", 224, 320, 3);
	validate_content(path, 0, 4, first_sample);
	validate_content(dir / "repeat-part-002.mkv", 4, 4, first_sample + 3200);
	validate_content(dir / "repeat-part-003.mkv", 8, 3, first_sample + 6400);

	// A reset-driven monitor change closes the old clock epoch before reset.
	for (unsigned limit : {0u, 5u}) {
		reset(320, 224);
		const auto base = "epoch-" + std::to_string(limit);
		check(vaeg_recording_native_session_start((dir / (base + ".mkv")).string().c_str(), limit),
		      "SPLIT_EPOCH_START");
		vaeg_recording_native_session_enable_auto_split();
		std::uint64_t old_first = 0, new_first = 0;
		for (unsigned i = 0; i < 4; ++i) {
			check(frame(i) == VAEG_RECORDING_NATIVE_TAP_OK, "SPLIT_EPOCH_OLD_FRAME");
			vaeg_recording_runtime_step(0);
			if (i == 0)
				old_first = consumed_samples;
		}
		vaeg_recording_native_session_before_reset();
		check(vaeg_recording_native_session_is_active() && !audio_active &&
		          vaeg_recording_native_session_elapsed_ticks() == 300000,
		      "SPLIT_EPOCH_WAITING_WITH_REC_TIME");
		reset(640, 400);
		stream.generation_epoch = 4;
		vaeg_recording_native_session_before_reset(); // Repeated reset before a new frame coalesces.
		check(vaeg_recording_runtime_step(0) == VAEG_RECORDING_RUNTIME_CONTINUE &&
		          vaeg_recording_native_session_is_active(),
		      "SPLIT_EPOCH_WAIT_CONTINUES");
		for (unsigned i = 0; i < (limit ? 3u : 4u); ++i) {
			check(frame(i) == VAEG_RECORDING_NATIVE_TAP_OK, "SPLIT_EPOCH_NEW_FRAME");
			vaeg_recording_runtime_step(0);
			if (i == 0)
				new_first = consumed_samples;
		}
		if (limit)
			check(!vaeg_recording_native_session_is_active(), "SPLIT_EPOCH_GLOBAL_LIMIT");
		else {
			check(vaeg_recording_native_session_elapsed_ticks() == 600000,
			      "SPLIT_EPOCH_CUMULATIVE_MOVIE_TIME");
			check(vaeg_recording_native_session_stop(), "SPLIT_EPOCH_STOP");
		}
		validate_movie(dir / (base + ".mkv"), 320, 224, 3);
		validate_movie(dir / (base + "-part-002.mkv"), 640, 400, limit ? 2 : 3);
		validate_content(dir / (base + ".mkv"), 0, 3, old_first);
		validate_content(dir / (base + "-part-002.mkv"), 0, limit ? 2 : 3, new_first);
	}
	reset(320, 224);
	check(vaeg_recording_native_session_start((dir / "early.mkv").string().c_str(), 0),
	      "SPLIT_EARLY_START");
	vaeg_recording_native_session_enable_auto_split();
	vaeg_recording_runtime_screen_changed();
	for (unsigned i = 0; i < 4; ++i) {
		check(frame(i) == VAEG_RECORDING_NATIVE_TAP_OK, "SPLIT_EARLY_FRAME");
		vaeg_recording_runtime_step(0);
		if (i == 0)
			first_sample = consumed_samples;
	}
	check(vaeg_recording_native_session_stop() && !std::filesystem::exists(dir / "early.mkv"),
	      "SPLIT_EARLY_NO_EMPTY_ORIGINAL");
	validate_content(dir / "early-part-002.mkv", 0, 3, first_sample);
}
void split_edge_cases(const std::filesystem::path &dir) {
	for (unsigned kind = 0; kind < 3; ++kind) {
		reset(320, 224);
		const auto name = "edge-" + std::to_string(kind);
		const auto path = dir / (name + ".mkv");
		const auto next = dir / (name + "-part-002.mkv");
		check(vaeg_recording_native_session_start(path.string().c_str(), kind == 0 ? 4 : 0),
		      "SPLIT_EDGE_START");
		vaeg_recording_native_session_enable_auto_split();
		std::uint64_t first_sample = 0;
		for (unsigned i = 0; i < 4; ++i) {
			check(frame(i) == VAEG_RECORDING_NATIVE_TAP_OK, "SPLIT_EDGE_FIXTURE");
			check(vaeg_recording_runtime_step(0) == VAEG_RECORDING_RUNTIME_CONTINUE,
			      "SPLIT_EDGE_FIXTURE_STEP");
			if (i == 0)
				first_sample = consumed_samples;
		}
		if (kind == 1) { // Exactly one mutation: an existing next-file target.
			std::ofstream sentinel(next);
			sentinel << "keep";
		}
		++generation;
		check(frame(4) == VAEG_RECORDING_NATIVE_TAP_OK, "SPLIT_EDGE_BOUNDARY");
		vaeg_recording_runtime_step(0);
		if (kind == 0 || kind == 1) {
			check(!vaeg_recording_native_session_is_active(), "SPLIT_LIMIT_OR_COLLISION_STOPS");
			validate_movie(path, 320, 224, 4);
			validate_content(path, 0, 4, first_sample);
			if (kind == 0)
				check(!std::filesystem::exists(next), "SPLIT_LIMIT_NO_EXTRA_SEGMENT");
			else {
				std::ifstream sentinel(next);
				std::string text;
				sentinel >> text;
				check(text == "keep", "SPLIT_COLLISION_NO_OVERWRITE");
				check(std::string(vaeg_recording_native_session_last_error()) == "target_exists",
				      "SPLIT_COLLISION_EXACT_DIAGNOSTIC");
			}
		} else { // Stopping immediately after rollover does not publish an empty movie.
			check(vaeg_recording_native_session_request_stop() &&
			          vaeg_recording_native_session_poll_stop(),
			      "SPLIT_EMPTY_FINAL_STOP");
			check(!std::filesystem::exists(next), "SPLIT_EMPTY_FINAL_NOT_PUBLISHED");
			validate_movie(path, 320, 224, 4);
		}
	}
}
void geometry(const std::filesystem::path &path, unsigned w, unsigned h, unsigned next_w,
              unsigned next_h, bool batch) {
	reset(w, h);
	check(vaeg_recording_native_session_start(path.string().c_str(), 0),
	      "RUNTIME_INTEGRATION_START");
	for (unsigned i = 0; i < 4; ++i) {
		check(frame(i) == VAEG_RECORDING_NATIVE_TAP_OK, "RUNTIME_INTEGRATION_BASELINE_TAP");
		check(vaeg_recording_runtime_step(batch) == VAEG_RECORDING_RUNTIME_CONTINUE,
		      "RUNTIME_INTEGRATION_BASELINE_STEP");
	}
	check(vaeg_recording_native_session_accepted_frames() == 3,
	      "RUNTIME_INTEGRATION_BASELINE_ACCEPTED");
	width = next_w;
	height = next_h;
	++generation;
	check(frame(4) == VAEG_RECORDING_NATIVE_TAP_ERROR_DESCRIPTOR_CHANGED,
	      "RUNTIME_INTEGRATION_CONTROLLED_DESCRIPTOR_CHANGE");
	const auto result = vaeg_recording_runtime_step(batch);
	check(!vaeg_recording_native_session_is_active(), "RUNTIME_INTEGRATION_RECORDING_RELEASED");
	if (batch) {
		check(result == VAEG_RECORDING_RUNTIME_BATCH_FAILED && !std::filesystem::exists(path),
		      "RUNTIME_INTEGRATION_BATCH_FAIL_CLOSED");
	} else {
		check(result == VAEG_RECORDING_RUNTIME_CONTINUE &&
		          vaeg_recording_runtime_last_event() == VAEG_RECORDING_RUNTIME_SCREEN_SAVED,
		      "RUNTIME_INTEGRATION_GUEST_CONTINUES");
		validate_movie(path, w, h, 3);
		check(vaeg_recording_runtime_step(0) == VAEG_RECORDING_RUNTIME_CONTINUE,
		      "RUNTIME_INTEGRATION_IDLE_AFTER_GEOMETRY");
	}
}
}
extern "C" UINT64 pccore_guest_clock(void) {
	return clock_ticks;
}
extern "C" BOOL scrnmng_get_native_recording_video_descriptor(VAEG_RECORDING_VIDEO_DESCRIPTOR *d) {
	if (!d)
		return FAILURE;
	*d = {width, height, width * 3u, VAEG_RECORDING_PIXEL_RGB24};
	return SUCCESS;
}
extern "C" BOOL scrnmng_get_displayed_recording_video_descriptor(
    VAEG_RECORDING_VIDEO_DESCRIPTOR *d, UINT32 *g) {
	*d = {width, height, width * 3u, VAEG_RECORDING_PIXEL_RGB24};
	*g = generation;
	return SUCCESS;
}
extern "C" BOOL scrnmng_start_displayed_recording_capture(
    VAEG_RECORDING_DISPLAYED_CAPTURE_CALLBACK callback, void *context) {
	if (displayed_active) return FAILURE;
	displayed_active = true;
	displayed_callback = callback;
	displayed_context = context;
	return SUCCESS;
}
extern "C" void scrnmng_stop_displayed_recording_capture(void) {
	displayed_active = false;
	displayed_callback = nullptr;
	displayed_context = nullptr;
	displayed_error = VAEG_RECORDING_DISPLAYED_CAPTURE_OK;
}
extern "C" BOOL scrnmng_displayed_recording_capture_active(void) { return displayed_active; }
extern "C" VAEG_RECORDING_DISPLAYED_CAPTURE_ERROR scrnmng_displayed_recording_capture_error(void) {
	return displayed_error;
}

extern "C" UINT32 scrnmng_native_recording_descriptor_generation(void) {
	return generation;
}
extern "C" BOOL sound_get_stream_state(SOUNDSTREAMSTATE *s) {
	if (!s)
		return FAILURE;
	*s = stream;
	return SUCCESS;
}
extern "C" void sound_sync(void) {
}
extern "C" SOUND_RECORDING_ERROR sound_recording_start(UINT capacity) {
	++audio_start_calls;
	check(capacity != 0, "RUNTIME_INTEGRATION_AUDIO_CAPACITY");
	audio_active = true;
	return SOUND_RECORDING_ERROR_OK;
}
extern "C" void sound_recording_stop(void) {
	audio_active = false;
}
extern "C" void sound_recording_release(void) {
	audio_active = false;
}
extern "C" BOOL sound_recording_is_active(void) {
	return audio_active;
}
extern "C" UINT sound_recording_read(SINT16 *dst, UINT capacity, UINT64 *begin, UINT32 *epoch) {
	check(audio_active && dst && begin && epoch, "RUNTIME_INTEGRATION_AUDIO_READ");
	*begin = consumed_samples;
	*epoch = stream.generation_epoch;
	const UINT count = static_cast<UINT>(
	    std::min<std::uint64_t>(capacity, stream.generated_sample_count - consumed_samples));
	for (UINT i = 0; i < count; ++i) {
		dst[i * 2] = static_cast<SINT16>(consumed_samples + i);
		dst[i * 2 + 1] = -dst[i * 2];
	}
	consumed_samples += count;
	return count;
}
extern "C" BOOL sound_recording_get_state(SOUNDRECORDINGSTATE *s) {
	if (!s)
		return FAILURE;
	*s = {};
	s->rate = stream.rate;
	/* Real FIFO first_sample remains fixed while consumed_frames advances. */
	s->first_sample = 0;
	s->consumed_frames = consumed_samples;
	s->produced_frames = stream.generated_sample_count;
	s->next_sample = stream.generated_sample_count;
	s->epoch = stream.generation_epoch;
	s->active = audio_active;
	return SUCCESS;
}
static bool displayed_frame(unsigned i, std::uint64_t token_delta = 0,
                            bool regress_clock = false) {
	clock_ticks = regress_clock ? 125u : 125u + static_cast<std::uint64_t>(i) * 100000u;
	stream.lastclock = static_cast<UINT32>(clock_ticks);
	stream.generated_sample_count = 1u + i * 800u;
	/* Deliberate row padding, colored pixels, asymmetric rows and nonopaque
	 * input alpha. The archive preserves RGB exactly and makes alpha opaque. */
	const unsigned pitch = width * 4u + 8u;
	std::vector<std::uint8_t> pixels(static_cast<std::size_t>(pitch) * height, 0xcd);
	for (unsigned y = 0; y < height; ++y)
		for (unsigned x = 0; x < width; ++x) {
			auto *p = pixels.data() + y * pitch + x * 4;
			for (unsigned c = 0; c < 3; ++c)
				p[c] = static_cast<std::uint8_t>(40 + i + (2 - c) * 20 + (x + 3 * y) % 17);
			p[3] = 17;
		}
	VAEG_RECORDING_DISPLAYED_FRAME f{};
	f.pixels = pixels.data(); f.width = width; f.height = height;
	f.pitch_bytes = pitch; f.size_bytes = pixels.size();
	f.pixel_format = VAEG_RECORDING_DISPLAYED_PIXEL_RGBA32;
	f.row_origin = VAEG_RECORDING_DISPLAYED_ROWS_TOP_DOWN;
	f.frame_token = static_cast<std::uint32_t>(i + 1 + displayed_token_bias) + token_delta;
	f.descriptor_generation = generation;
	check(displayed_active && displayed_callback, "DISPLAYED_FIXTURE_ACTIVE");
	const bool accepted = displayed_callback(displayed_context, &f) != 0;
	/* Neither staged pixels nor queued slices may borrow this memory. */
	std::fill(pixels.begin(), pixels.end(), 0xee);
	if (!accepted) {
		displayed_active = false;
		displayed_error = VAEG_RECORDING_DISPLAYED_CAPTURE_CALLBACK_REJECTED;
	}
	return accepted;
}
static void displayed_sessions(const std::filesystem::path &dir) {
	// Each negative scenario begins with the same two valid complete intervals.
	for (unsigned kind = 0; kind < 11; ++kind) {
		reset(3, 2);
		const auto path = dir / ("displayed-" + std::to_string(kind) + ".mkv");
		check(vaeg_recording_session_start(path.string().c_str(),
		                                  VAEG_RECORDING_SOURCE_DISPLAYED, kind == 0 ? 2 : 0),
		      "DISPLAYED_SESSION_START");
		vaeg_recording_native_session_enable_auto_split();
		check(vaeg_recording_session_is_displayed() &&
		          !vaeg_recording_native_session_auto_split_enabled() &&
		          !vaeg_recording_native_tap_is_active(), "DISPLAYED_NO_NATIVE_SUBSTITUTION");
		for (unsigned i = 0; i < 3; ++i) {
			check(vaeg_recording_runtime_step(0) == VAEG_RECORDING_RUNTIME_CONTINUE &&
			          vaeg_recording_native_session_is_active(), "DISPLAYED_PRE_PRESENT_STEP");
			check(displayed_frame(i), "DISPLAYED_VALID_CAPTURE");
			check(vaeg_recording_native_session_accepted_frames() == i, "DISPLAYED_INTERVAL_COUNT");
		}
		check(vaeg_recording_native_session_elapsed_ticks() == 200000,
		      "DISPLAYED_GUEST_TIME_NOT_HOST_TIME");
		if (kind == 0) {
			check(vaeg_recording_native_session_should_stop(), "DISPLAYED_FRAME_LIMIT");
			vaeg_recording_runtime_step(0);
		} else if (kind == 1) {
			check(vaeg_recording_native_session_request_stop() &&
			          vaeg_recording_native_session_poll_stop(), "DISPLAYED_MANUAL_STOP");
		} else if (kind == 2) {
			vaeg_recording_native_session_before_reset();
		} else if (kind == 3) {
			vaeg_recording_native_session_epoch_change();
		} else if (kind == 7 || kind == 9 || kind == 10) {
			check(!displayed_frame(3, kind == 7 ? 1u : kind == 9 ? (UINT64_C(1) << 32) : 0u,
			                       kind == 10), "DISPLAYED_BOUNDARY_MUTATION_REJECTED");
			check(std::string(vaeg_recording_native_session_last_error()) == "none",
			      "DISPLAYED_PENDING_FAILURE_NOT_YET_PUBLISHED");
			vaeg_recording_runtime_step(0);
			check(std::string(vaeg_recording_native_session_last_error()) ==
			          "displayed_capture_mismatch", "DISPLAYED_TOKEN_GAP_EXACT_ERROR");
		} else {
			if (kind == 8) {
				clock_ticks += 100000u;
				stream.lastclock = static_cast<UINT32>(clock_ticks);
				stream.generated_sample_count += 800u;
			}
			displayed_active = false;
			displayed_error = kind == 6 ? VAEG_RECORDING_DISPLAYED_CAPTURE_READBACK_FAILED :
			                              VAEG_RECORDING_DISPLAYED_CAPTURE_GEOMETRY_CHANGED;
			const auto result = vaeg_recording_runtime_step(kind == 5);
			check(result == (kind == 5 ? VAEG_RECORDING_RUNTIME_BATCH_FAILED :
			                             VAEG_RECORDING_RUNTIME_CONTINUE), "DISPLAYED_FAILURE_POLICY");
			check(vaeg_recording_runtime_last_event() ==
			          (kind == 4 || kind == 8 ? VAEG_RECORDING_RUNTIME_SCREEN_SAVED :
			                       VAEG_RECORDING_RUNTIME_ERROR_STOPPED), "DISPLAYED_FAILURE_EXACT_EVENT");
			if (kind == 5 || kind == 6)
				check(std::string(vaeg_recording_native_session_last_error()) ==
				          (kind == 5 ? "geometry_changed" : "readback_failed"),
				      "DISPLAYED_FAILURE_EXACT_CAPTURE_ERROR");
		}
		check(!vaeg_recording_native_session_is_active() && !displayed_active && !audio_active,
		      "DISPLAYED_SESSION_RESOURCES_RELEASED");
		if (kind == 0 || kind == 1 || kind == 2 || kind == 4 || kind == 8) {
			const unsigned count = kind == 8 ? 3 : 2;
			validate_movie(path, 3, 2, count);
			validate_content(path, 0, count, 0, true);
		} else check(!std::filesystem::exists(path), "DISPLAYED_NO_FAILED_MOVIE_PUBLISHED");
	}
	reset(3, 2);
	displayed_token_bias = UINT32_MAX - 1u;
	const auto wrapped = dir / "displayed-token-wrap.mkv";
	check(vaeg_recording_session_start(wrapped.string().c_str(), VAEG_RECORDING_SOURCE_DISPLAYED, 0),
	      "DISPLAYED_COUNTER_WRAP_START");
	for (unsigned i = 0; i < 3; ++i) check(displayed_frame(i), "DISPLAYED_COUNTER_WRAP_CAPTURE");
	check(vaeg_recording_native_session_stop(), "DISPLAYED_COUNTER_WRAP_STOP");
	validate_content(wrapped, 0, 2, 0, true);
	reset(3, 2);
	const auto empty = dir / "displayed-empty.mkv";
	check(vaeg_recording_session_start(empty.string().c_str(), VAEG_RECORDING_SOURCE_DISPLAYED, 0),
	      "DISPLAYED_EMPTY_START");
	check(!vaeg_recording_native_session_stop() && !std::filesystem::exists(empty) &&
	          !audio_active && !displayed_active &&
	          std::string(vaeg_recording_native_session_last_error()) == "recording_no_intervals",
	      "DISPLAYED_EMPTY_STOP_NO_PUBLISHED_FILE");
}

#ifdef VAEG_RECORDING_METAL_PROBE
static void frontend_archive_probe(const std::filesystem::path &dir) {
	const auto roms = dir / "empty-roms";
	check(std::filesystem::create_directory(roms), "METAL_FRONTEND_EMPTY_ROM_DIRECTORY");
	check(setenv("VAEG_NATIVE_CRT", "1", 1) == 0 &&
	          setenv("SDL_AUDIODRIVER", "dummy", 1) == 0, "METAL_FRONTEND_PUBLIC_ENVIRONMENT");
	for (const char *source : {"native", "displayed"}) {
		const auto path = dir / (std::string("frontend-") + source + ".mkv");
		const auto log = dir / (std::string("frontend-") + source + ".log");
		std::vector<std::string> command{VAEG_METAL_FRONTEND_BINARY, "--roms", roms.string(),
		    "--model", "va2", "--no-cfg", "--no-bkupmem", "--windowed", "--frameskip", "4",
		    "--record", path.string(), "--record-source", source, "--record-frames", "4"};
		std::vector<char *> argv;
		for (auto &arg : command) argv.push_back(arg.data());
		argv.push_back(nullptr);
		posix_spawn_file_actions_t actions;
		check(posix_spawn_file_actions_init(&actions) == 0, "METAL_FRONTEND_SPAWN_ACTIONS");
		check(posix_spawn_file_actions_addopen(&actions, STDOUT_FILENO, log.string().c_str(),
		                                      O_CREAT | O_WRONLY | O_EXCL, 0600) == 0 &&
		          posix_spawn_file_actions_adddup2(&actions, STDOUT_FILENO, STDERR_FILENO) == 0,
		      "METAL_FRONTEND_EXCLUSIVE_LOG");
		pid_t pid;
		const int spawned = posix_spawn(&pid, command.front().c_str(), &actions, nullptr,
		                                 argv.data(), environ);
		posix_spawn_file_actions_destroy(&actions);
		check(spawned == 0, "METAL_FRONTEND_SPAWN");
		int status;
		pid_t waited;
		do { waited = waitpid(pid, &status, 0); } while (waited < 0 && errno == EINTR);
		check(waited == pid && WIFEXITED(status) && WEXITSTATUS(status) == 0,
		      "METAL_FRONTEND_ARCHIVE_EXIT");
		std::ifstream input(log);
		const std::string text((std::istreambuf_iterator<char>(input)), {});
		check(text.find("Native CRT active: backend=Metal") != std::string::npos,
		      "METAL_FRONTEND_NO_SDL_SUBSTITUTION");
		AVFormatContext *ctx = nullptr;
		check(avformat_open_input(&ctx, path.string().c_str(), nullptr, nullptr) == 0 &&
		          avformat_find_stream_info(ctx, nullptr) >= 0, "METAL_FRONTEND_MOVIE_OPEN");
		const int video = av_find_best_stream(ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
		const int audio = av_find_best_stream(ctx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
		const auto *tag = av_dict_get(ctx->metadata, "VAEG_RECORDING_SOURCE", nullptr, 0);
		check(video >= 0 && audio >= 0 && ctx->nb_streams == 2 && tag &&
		          std::string(tag->value) == source &&
		          ctx->streams[video]->codecpar->codec_id == AV_CODEC_ID_FFV1 &&
		          ctx->streams[audio]->codecpar->codec_id == AV_CODEC_ID_PCM_S16LE,
		      "METAL_FRONTEND_SOURCE_AND_STREAMS");
		const int w = ctx->streams[video]->codecpar->width;
		const int h = ctx->streams[video]->codecpar->height;
		if (std::string(source) == "displayed")
			check(w >= 640 && h > 400, "METAL_FRONTEND_FULL_CLIENT_INCLUDES_MENU");
		avformat_close_input(&ctx);
		validate_movie(path, w, h, 4);
	}
	// Empty ROMs are intentional: this is public frontend/startup/timing evidence,
	// not a commercial ROM identity, guest boot or audio-device runtime test.
	std::puts("METAL_FRONTEND_NATIVE_DISPLAYED_STARTUP_DRAW_SKIP_PASS");
}

static int metal_archive_probe() {
	if (std::getenv("VAEG_RUN_METAL_ARCHIVE_PROBE") == nullptr) {
		std::puts("METAL_ARCHIVE_OPT_IN_REQUIRED");
		return 77;
	}
	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		std::fprintf(stderr, "METAL_ARCHIVE_VIDEO_UNAVAILABLE: %s\n", SDL_GetError());
		return 77;
	}
	auto *window = SDL_CreateWindow("vaeg public Metal archive probe",
	    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 320, 240,
	    SDL_WINDOW_METAL | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_SHOWN);
	if (!window) {
		std::fprintf(stderr, "METAL_ARCHIVE_WINDOW_UNAVAILABLE: %s\n", SDL_GetError());
		SDL_Quit();
		return 77;
	}
	const auto dir = std::filesystem::temp_directory_path() /
	    ("vaeg-metal-archive-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
	check(std::filesystem::create_directory(dir), "METAL_ARCHIVE_EXCLUSIVE_DIRECTORY");
	const auto cache = (dir / "shader-cache").string();
	vaeg_builtin_shader_set_cache_root(cache.c_str());
	const char *preset = vaeg_builtin_shader_preset();
	check(preset != nullptr, "METAL_ARCHIVE_PUBLIC_PRESET");
	ImGui::CreateContext();
	auto &io = ImGui::GetIO();
	io.IniFilename = nullptr;
	std::vector<std::uint8_t> unfiltered;
	for (unsigned filtered = 0; filtered < 2; ++filtered) {
		int w, h, logical_w, logical_h;
		SDL_GetWindowSizeInPixels(window, &w, &h);
		SDL_GetWindowSize(window, &logical_w, &logical_h);
		reset(w, h);
		auto presenter = vaeg::librashader::create_metal_presenter();
		const auto parameters = (dir / "parameters.cfg").string();
		vaeg::librashader::NativePresenterCreateInfo info{window,
		    static_cast<std::uint32_t>(w), static_cast<std::uint32_t>(h),
		    vaeg::librashader::PresenterBackend::Metal, filtered != 0, preset, parameters.c_str()};
		check(presenter->initialize(info) == vaeg::librashader::PresenterResult::Recovered,
		      "METAL_ARCHIVE_INITIALIZE");
		presenter->set_output_viewport(0, h / 8, w, h - h / 8);
		const auto path = dir / (filtered ? "filtered.mkv" : "unfiltered.mkv");
		check(vaeg_recording_session_start(path.string().c_str(),
		                                  VAEG_RECORDING_SOURCE_DISPLAYED, 0), "METAL_ARCHIVE_START");
		std::vector<std::vector<std::uint8_t>> expected;
		for (unsigned i = 0; i < 3; ++i) {
			SDL_PumpEvents();
			check(presenter->gui_prepare(), "METAL_ARCHIVE_GUI_PREPARE");
			io.DisplaySize = ImVec2(static_cast<float>(logical_w), static_cast<float>(logical_h));
			io.DisplayFramebufferScale = ImVec2(static_cast<float>(w) / logical_w,
			                                     static_cast<float>(h) / logical_h);
			io.DeltaTime = 1.0f / 60.0f;
			ImGui::NewFrame();
			ImGui::GetForegroundDrawList()->AddRectFilled(ImVec2(8, 8), ImVec2(36, 24),
			                                               IM_COL32(231, 17, 61, 255));
			ImGui::GetForegroundDrawList()->AddText(ImVec2(40, 8), IM_COL32(255, 255, 255, 255),
			                                         "REC TEST");
			ImGui::Render();
			std::vector<std::uint16_t> guest(64 * 48);
			for (unsigned y = 0; y < 48; ++y)
				for (unsigned x = 0; x < 64; ++x)
					guest[y * 64 + x] = static_cast<std::uint16_t>(
					    (((x + i * 3) % 32) << 11) | (((y * 2 + i) % 64) << 5) | ((x + y) % 32));
			VAEG_FRAME_INPUT frame{};
			vaeg_frame_input_initialize(&frame, guest.data(), 64, 48, 128,
			    VAEG_FRAME_PIXEL_RGB565, VAEG_FRAME_ROWS_TOP_DOWN, 4, 3, 60, 1, i, 16666667);
			expected.emplace_back(static_cast<std::size_t>(w) * h * 4);
			VAEG_OUTPUT_CAPTURE output{expected.back().data(), width, height, width * 4, 0};
			presenter->set_output_capture(&output);
			check(presenter->present(frame) == vaeg::librashader::PresenterResult::Presented &&
			          output.complete, "METAL_ARCHIVE_ONE_LIVE_READBACK");
			presenter->set_output_capture(nullptr);
			const unsigned anchor = (static_cast<unsigned>(12 * io.DisplayFramebufferScale.y) * width +
			                         static_cast<unsigned>(12 * io.DisplayFramebufferScale.x)) * 4;
			check(expected.back()[anchor] == 231 && expected.back()[anchor + 1] == 17 &&
			          expected.back()[anchor + 2] == 61, "METAL_ARCHIVE_FINAL_GUI_RECTANGLE");
			unsigned white = 0;
			for (int y = 8 * io.DisplayFramebufferScale.y; y < 24 * io.DisplayFramebufferScale.y; ++y)
				for (int x = 40 * io.DisplayFramebufferScale.x; x < 120 * io.DisplayFramebufferScale.x; ++x) {
					const auto *p = expected.back().data() + (y * width + x) * 4;
					if (p[0] == 255 && p[1] == 255 && p[2] == 255) ++white;
				}
			check(white > 10, "METAL_ARCHIVE_FIRST_FRAME_FONT_ATLAS");
			clock_ticks = 125u + static_cast<std::uint64_t>(i) * 100000u;
			stream.lastclock = static_cast<UINT32>(clock_ticks);
			stream.generated_sample_count = 1u + i * 800u;
			VAEG_RECORDING_DISPLAYED_FRAME capture{expected.back().data(), width, height,
			    width * 4, expected.back().size(), VAEG_RECORDING_DISPLAYED_PIXEL_RGBA32,
			    VAEG_RECORDING_DISPLAYED_ROWS_TOP_DOWN, i + 1, generation};
			check(displayed_callback(displayed_context, &capture) != 0, "METAL_ARCHIVE_OWNED_CAPTURE");
		}
		check(vaeg_recording_native_session_stop(), "METAL_ARCHIVE_STOP");
		validate_movie(path, w, h, 2);
		validate_content(path, 0, 2, 0, true, &expected);
		if (!filtered) unfiltered = expected.front();
		else check(unfiltered != expected.front(), "METAL_ARCHIVE_ACTUAL_SHADER_NOT_NATIVE_SUBSTITUTION");
		presenter->gui_shutdown();
		presenter->shutdown();
	}
	ImGui::DestroyContext();
	SDL_DestroyWindow(window);
	SDL_Quit();
	std::puts("METAL_ARCHIVE_FINAL_GUI_FFV1_PCM_PARITY_PASS");
	frontend_archive_probe(dir);
	return 0;
}
#endif

int main() {
#ifdef VAEG_RECORDING_METAL_PROBE
	return metal_archive_probe();
#endif
	const auto dir = std::filesystem::temp_directory_path() /
	                 ("vaeg-runtime-" +
	                  std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
	check(std::filesystem::create_directory(dir), "RUNTIME_INTEGRATION_EXCLUSIVE_DIRECTORY");
	displayed_sessions(dir);
	for (unsigned kind = 0; kind < 4; ++kind)
		automatic_split(dir, kind);
	split_repeated_and_epoch(dir);
	split_edge_cases(dir);
	reset(320, 224);
	const auto state_load = dir / "state-load.mkv";
	check(vaeg_recording_native_session_start(state_load.string().c_str(), 0), "SPLIT_STATE_LOAD_START");
	vaeg_recording_native_session_enable_auto_split();
	check(frame(0) == VAEG_RECORDING_NATIVE_TAP_OK, "SPLIT_STATE_LOAD_BASELINE");
	vaeg_recording_runtime_step(0);
	check(vaeg_recording_native_session_is_active(), "SPLIT_STATE_LOAD_VALID_FIXTURE");
	// The state-load epoch notification is NOT an approved reset/geometry rollover.
	vaeg_recording_native_session_epoch_change();
	check(!vaeg_recording_native_session_is_active() && !std::filesystem::exists(state_load),
	      "SPLIT_STATE_LOAD_REMAINS_CANCEL_NOT_RESTART");
	geometry(dir / "right.mkv", 400, 640, 448, 640, false);
	geometry(dir / "none.mkv", 640, 400, 640, 448, false);
	geometry(dir / "small.mkv", 320, 224, 224, 320, false);
	geometry(dir / "batch.mkv", 400, 640, 448, 640, true);
	reset(320, 224);
	const auto path = dir / "limit.mkv";
	check(vaeg_recording_native_session_start(path.string().c_str(), 2),
	      "RUNTIME_INTEGRATION_LIMIT_START");
	for (unsigned i = 0; i < 3; ++i) {
		check(frame(i) == VAEG_RECORDING_NATIVE_TAP_OK, "RUNTIME_INTEGRATION_LIMIT_TAP");
		check(vaeg_recording_runtime_step(0) == VAEG_RECORDING_RUNTIME_CONTINUE,
		      "RUNTIME_INTEGRATION_LIMIT_CONTINUE");
	}
	check(!vaeg_recording_native_session_is_active() &&
	          vaeg_recording_runtime_last_event() == VAEG_RECORDING_RUNTIME_LIMIT_SAVED,
	      "RUNTIME_INTEGRATION_LIMIT_SAVED");
	validate_movie(path, 320, 224, 2);
	reset(320, 224);
	const auto settings_path = dir / "settings.mkv";
	check(vaeg_recording_native_session_start(settings_path.string().c_str(), 0),
	      "RUNTIME_INTEGRATION_SETTINGS_START");
	for (unsigned i = 0; i < 4; ++i) {
		check(frame(i) == VAEG_RECORDING_NATIVE_TAP_OK, "RUNTIME_INTEGRATION_SETTINGS_TAP");
		check(vaeg_recording_runtime_step(0) == VAEG_RECORDING_RUNTIME_CONTINUE,
		      "RUNTIME_INTEGRATION_SETTINGS_STEP");
	}
	vaeg_recording_runtime_screen_changed();
	check(!vaeg_recording_native_session_is_active() &&
	          vaeg_recording_runtime_last_event() == VAEG_RECORDING_RUNTIME_SCREEN_SAVED,
	      "RUNTIME_INTEGRATION_SETTINGS_SAVED");
	validate_movie(settings_path, 320, 224, 3);
	std::puts("recording runtime integration passed");
	return 0;
}
