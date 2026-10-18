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
#include <string>
#include <vector>

PCCORE pccore{};
namespace {
std::uint64_t clock_ticks, consumed_samples;
std::uint32_t width, height, generation;
bool audio_active;
unsigned audio_start_calls;
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
                      std::uint64_t first_sample) {
	AVFormatContext *ctx = nullptr;
	check(avformat_open_input(&ctx, path.string().c_str(), nullptr, nullptr) == 0 &&
	          avformat_find_stream_info(ctx, nullptr) >= 0,
	      "SPLIT_DECODE_OPEN");
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
							check(row[x * 4 + c] == (c == 3 ? 255 : 40 + first + videos),
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
void automatic_split(const std::filesystem::path &dir, unsigned kind) {
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
		vaeg_recording_native_session_epoch_change();
		check(vaeg_recording_native_session_is_active() && !audio_active &&
		          vaeg_recording_native_session_elapsed_ticks() == 300000,
		      "SPLIT_EPOCH_WAITING_WITH_REC_TIME");
		reset(640, 400);
		stream.generation_epoch = 4;
		vaeg_recording_native_session_epoch_change(); // Repeated reset before a new frame coalesces.
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
int main() {
	const auto dir = std::filesystem::temp_directory_path() /
	                 ("vaeg-runtime-" +
	                  std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
	check(std::filesystem::create_directory(dir), "RUNTIME_INTEGRATION_EXCLUSIVE_DIRECTORY");
	for (unsigned kind = 0; kind < 4; ++kind)
		automatic_split(dir, kind);
	split_repeated_and_epoch(dir);
	split_edge_cases(dir);
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
