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
}
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

PCCORE pccore{};
namespace {
std::uint64_t clock_ticks, consumed_samples;
std::uint32_t width, height, generation;
bool audio_active;
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
	return vaeg_recording_native_tap_submit_frame(i + 1, clock_ticks, generation, &f, &s);
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
extern "C" SOUND_RECORDING_ERROR sound_recording_start(UINT capacity) {
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
	*epoch = 3;
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
	s->first_sample = 0;
	s->next_sample = stream.generated_sample_count;
	s->epoch = 3;
	s->active = audio_active;
	return SUCCESS;
}
int main() {
	const auto dir = std::filesystem::temp_directory_path() /
	                 ("vaeg-runtime-" +
	                  std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
	check(std::filesystem::create_directory(dir), "RUNTIME_INTEGRATION_EXCLUSIVE_DIRECTORY");
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
