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
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

#include "compiler.h"
#include "machine/pccore.h"
#include "recording/recording_native_session.h"
#include "recording/recording_native_tap.h"
#include "scrnmng.h"
#include "sound.h"

namespace fs = std::filesystem;

PCCORE pccore{};

namespace {

std::uint64_t fake_clock = 0u;
std::uint64_t fake_next_sample = 0u;
std::uint64_t fake_arm_clock = 0u;
std::uint64_t fake_audio_origin_clock = 0u;
std::uint64_t fake_audio_origin_sample = 0u;
std::uint64_t fake_capture_first_sample = 0u;
bool fake_recording_active = false;
bool fake_limit_audio_to_guest_time = false;
std::uint32_t fake_recording_epoch = 3u;
SOUNDSTREAMSTATE fake_stream{};

void reset_fake_audio() {
	fake_clock = 0u;
	fake_next_sample = 0u;
	fake_arm_clock = 0u;
	fake_audio_origin_clock = 0u;
	fake_audio_origin_sample = 0u;
	fake_capture_first_sample = 0u;
	fake_recording_active = false;
	fake_limit_audio_to_guest_time = false;
	fake_recording_epoch = 3u;
	fake_stream = {};
	fake_stream.rate = 100u;
	fake_stream.lastclock = 0u;
	fake_stream.generation_epoch = 3u;
	pccore.realclock = 1000u;
}

VAEG_RECORDING_NATIVE_FRAME make_frame(std::uint64_t sequence,
                                       std::uint8_t value,
                                       std::uint8_t *pixels) {
	VAEG_RECORDING_NATIVE_FRAME frame{};
	frame.pixels = pixels;
	frame.width = 2u;
	frame.height = 1u;
	frame.pitch_bytes = 6u;
	frame.size_bytes = 6u;
	frame.sample_aspect_width = 1u;
	frame.sample_aspect_height = 1u;
	frame.frame_token = sequence;
	for (std::uint32_t index = 0u; index < 6u; ++index) {
		pixels[index] = static_cast<std::uint8_t>(value + index);
	}
	return frame;
}

VAEG_RECORDING_OSD_SNAPSHOT make_snapshot(std::uint64_t sequence) {
	VAEG_RECORDING_OSD_SNAPSHOT snapshot{};
	snapshot.version = VAEG_RECORDING_OSD_SNAPSHOT_VERSION;
	snapshot.frame_token = sequence;
	snapshot.guest_width = 2u;
	snapshot.guest_height = 1u;
	snapshot.output_width = 2u;
	snapshot.output_height = 1u;
	return snapshot;
}

void submit_frame(std::uint64_t sequence, std::uint64_t end_ticks,
                  std::uint8_t value) {
	if (fake_limit_audio_to_guest_time && end_ticks >= fake_audio_origin_clock) {
		const std::uint64_t elapsed = end_ticks - fake_audio_origin_clock;
		const std::uint64_t generated =
		    (elapsed * fake_stream.rate) / pccore.realclock;
		fake_stream.generated_sample_count = fake_audio_origin_sample + generated;
	}
	std::uint8_t pixels[6]{};
	VAEG_RECORDING_NATIVE_FRAME frame = make_frame(sequence, value, pixels);
	VAEG_RECORDING_OSD_SNAPSHOT snapshot = make_snapshot(sequence);
	assert(vaeg_recording_native_tap_submit_frame(
	           sequence, end_ticks, 77u, &frame, &snapshot) ==
	       VAEG_RECORDING_NATIVE_TAP_OK);
}

void test_native_session_writes_complete_intervals() {
	const fs::path path = fs::temp_directory_path() / "vaeg-rec13-native-session-test.mkv";
	std::error_code error;
	fs::remove(path, error);
	reset_fake_audio();
	if (!vaeg_recording_native_session_start(path.u8string().c_str(), 2u)) {
		std::fprintf(stderr, "native session start failed: %s\n",
		             vaeg_recording_native_session_last_error());
		std::abort();
	}
	assert(vaeg_recording_native_session_is_active());

	fake_clock = 10u;
	submit_frame(1u, fake_clock, 0x10u);
	assert(vaeg_recording_native_session_after_guest_exec());
	assert(fake_next_sample == 0u);

	fake_clock = 20u;
	submit_frame(2u, fake_clock, 0x20u);
	assert(vaeg_recording_native_session_after_guest_exec());

	fake_clock = 30u;
	submit_frame(3u, fake_clock, 0x30u);
	assert(vaeg_recording_native_session_after_guest_exec());
	assert(vaeg_recording_native_session_should_stop());
	assert(fake_next_sample == 2u);
	assert(vaeg_recording_native_session_stop());
	assert(!vaeg_recording_native_session_is_active());
	assert(fs::exists(path));
	assert(fs::file_size(path, error) > 0u && !error);
	fs::remove(path, error);
}

void test_epoch_change_cancels_without_publish() {
	const fs::path path = fs::temp_directory_path() / "vaeg-rec13-native-epoch-test.mkv";
	std::error_code error;
	fs::remove(path, error);
	reset_fake_audio();
	if (!vaeg_recording_native_session_start(path.u8string().c_str(), 0u)) {
		std::fprintf(stderr, "native epoch session start failed: %s\n",
		             vaeg_recording_native_session_last_error());
		std::abort();
	}
	assert(vaeg_recording_native_session_is_active());
	vaeg_recording_native_session_epoch_change();
	assert(!vaeg_recording_native_session_is_active());
	assert(!fake_recording_active);
	assert(!fs::exists(path));
}

void test_start_uses_quiesced_audio_clock_snapshot() {
	const fs::path path = fs::temp_directory_path() / "vaeg-rec14-audio-origin-test.mkv";
	std::error_code error;
	fs::remove(path, error);
	reset_fake_audio();
	/* Simulate the guest/audio watermark advancing while recording is armed. */
	fake_clock = 10u;
	fake_stream.lastclock = 20u;
	fake_arm_clock = 20u;
	assert(vaeg_recording_native_session_start(path.u8string().c_str(), 0u));
	assert(vaeg_recording_native_session_is_active());
	vaeg_recording_native_session_cancel();
	assert(!fs::exists(path));
}

void test_audio_intervals_match_guest_sample_generation() {
	const fs::path path = fs::temp_directory_path() / "vaeg-rec14-audio-grid-test.mkv";
	std::error_code error;
	fs::remove(path, error);
	reset_fake_audio();
	fake_limit_audio_to_guest_time = true;
	assert(vaeg_recording_native_session_start(path.u8string().c_str(), 1u));
	fake_clock = 20u;
	submit_frame(1u, fake_clock, 0x20u);
	assert(vaeg_recording_native_session_after_guest_exec());
	fake_clock = 35u;
	submit_frame(2u, fake_clock, 0x30u);
	assert(vaeg_recording_native_session_after_guest_exec());
	assert(vaeg_recording_native_session_should_stop());
	assert(fake_next_sample == 3u);
	assert(vaeg_recording_native_session_stop());
	assert(fs::exists(path));
	fs::remove(path, error);
}

void test_start_rejects_invalid_and_existing_targets() {
	const fs::path invalid = fs::temp_directory_path() / "vaeg-rec14-invalid.avi";
	const fs::path existing = fs::temp_directory_path() / "vaeg-rec14-existing.mkv";
	const fs::path displayed = fs::temp_directory_path() / "vaeg-rec14-displayed.mkv";
	std::error_code error;
	fs::remove(invalid, error);
	fs::remove(existing, error);
	fs::remove(displayed, error);
	reset_fake_audio();
	assert(!vaeg_recording_session_start(displayed.u8string().c_str(),
	                                     VAEG_RECORDING_SOURCE_DISPLAYED, 0u));
	assert(std::string(vaeg_recording_native_session_last_error()) ==
	       "displayed_source_unavailable");
	assert(!fs::exists(displayed));
	assert(!vaeg_recording_native_session_start(invalid.u8string().c_str(), 0u));
	assert(std::string(vaeg_recording_native_session_last_error()) ==
	       "recording_extension_invalid");
	assert(!fake_recording_active);
	assert(!fs::exists(invalid));
	{
		std::ofstream output(existing, std::ios::binary);
		assert(output.good());
		output << "existing";
	}
	reset_fake_audio();
	assert(!vaeg_recording_native_session_start(existing.u8string().c_str(), 0u));
	assert(std::string(vaeg_recording_native_session_last_error()) ==
	       "recording_target_exists");
	assert(!fake_recording_active);
	fs::remove(existing, error);
}

void test_manual_stop_finalizes_complete_movie() {
	const fs::path path = fs::temp_directory_path() / "vaeg-rec14-manual-stop.mkv";
	std::error_code error;
	fs::remove(path, error);
	reset_fake_audio();
	assert(vaeg_recording_native_session_start(path.u8string().c_str(), 0u));
	fake_clock = 10u;
	submit_frame(1u, fake_clock, 0x10u);
	assert(vaeg_recording_native_session_after_guest_exec());
	fake_clock = 20u;
	submit_frame(2u, fake_clock, 0x20u);
	assert(vaeg_recording_native_session_after_guest_exec());
	assert(vaeg_recording_native_session_accepted_frames() == 1u);
	assert(vaeg_recording_native_session_elapsed_ticks() == 10u);
	assert(vaeg_recording_native_session_request_stop());
	assert(vaeg_recording_native_session_is_finalizing());
	assert(vaeg_recording_native_session_poll_stop());
	assert(!vaeg_recording_native_session_is_active());
	assert(fs::exists(path));
	fs::remove(path, error);
}

void test_double_start_and_unicode_path() {
	const fs::path first = fs::temp_directory_path() / "vaeg-rec14-first.mkv";
	const fs::path second = fs::temp_directory_path() / "vaeg-rec14-second.mkv";
	const fs::path unicode = fs::temp_directory_path() / u8"vaeg-rec14-録画 test.mkv";
	std::error_code error;
	fs::remove(first, error);
	fs::remove(second, error);
	fs::remove(unicode, error);
	reset_fake_audio();
	assert(vaeg_recording_native_session_start(first.u8string().c_str(), 0u));
	assert(!vaeg_recording_native_session_start(second.u8string().c_str(), 0u));
	assert(std::string(vaeg_recording_native_session_last_error()) == "recording_busy");
	vaeg_recording_native_session_cancel();
	assert(!fs::exists(first));

	reset_fake_audio();
	assert(vaeg_recording_native_session_start(unicode.u8string().c_str(), 1u));
	fake_clock = 10u;
	submit_frame(1u, fake_clock, 0x30u);
	assert(vaeg_recording_native_session_after_guest_exec());
	fake_clock = 20u;
	submit_frame(2u, fake_clock, 0x40u);
	assert(vaeg_recording_native_session_after_guest_exec());
	assert(vaeg_recording_native_session_should_stop());
	assert(vaeg_recording_native_session_stop());
	assert(fs::exists(unicode));
	fs::remove(unicode, error);
}

} // namespace

extern "C" UINT64 pccore_guest_clock(void) {
	return fake_clock;
}

extern "C" BOOL scrnmng_get_native_recording_video_descriptor(
    VAEG_RECORDING_VIDEO_DESCRIPTOR *descriptor) {
	if (descriptor == nullptr) {
		return FAILURE;
	}
	*descriptor = {2u, 1u, 6u, VAEG_RECORDING_PIXEL_RGB24};
	return SUCCESS;
}

/* This test deliberately has no Displayed presenter; integration tests own it. */
extern "C" BOOL scrnmng_get_displayed_recording_video_descriptor(
    VAEG_RECORDING_VIDEO_DESCRIPTOR *, UINT32 *) { return FAILURE; }
extern "C" BOOL scrnmng_start_displayed_recording_capture(
    VAEG_RECORDING_DISPLAYED_CAPTURE_CALLBACK, void *) { return FAILURE; }
extern "C" void scrnmng_stop_displayed_recording_capture(void) {}
extern "C" BOOL scrnmng_displayed_recording_capture_active(void) { return FALSE; }
extern "C" VAEG_RECORDING_DISPLAYED_CAPTURE_ERROR scrnmng_displayed_recording_capture_error(void) {
	return VAEG_RECORDING_DISPLAYED_CAPTURE_UNAVAILABLE;
}

extern "C" UINT32 scrnmng_native_recording_descriptor_generation(void) {
	return 77u;
}

extern "C" BOOL sound_get_stream_state(SOUNDSTREAMSTATE *state) {
	if (state == nullptr) {
		return FAILURE;
	}
	*state = fake_stream;
	return SUCCESS;
}

extern "C" void sound_sync(void) {
}

extern "C" SOUND_RECORDING_ERROR sound_recording_start(UINT capacity_frames) {
	assert(capacity_frames != 0u);
	fake_recording_active = true;
	fake_next_sample = 0u;
	if (fake_arm_clock != 0u) {
		fake_clock = fake_arm_clock;
		fake_stream.lastclock = static_cast<std::uint32_t>(fake_arm_clock);
		fake_arm_clock = 0u;
	}
	fake_audio_origin_clock = fake_clock;
	fake_audio_origin_sample = fake_stream.generated_sample_count;
	fake_capture_first_sample = fake_stream.generated_sample_count;
	fake_next_sample = fake_capture_first_sample;
	return SOUND_RECORDING_ERROR_OK;
}

extern "C" void sound_recording_stop(void) {
	fake_recording_active = false;
}

extern "C" void sound_recording_release(void) {
	fake_recording_active = false;
}

extern "C" BOOL sound_recording_is_active(void) {
	return fake_recording_active ? TRUE : FALSE;
}

extern "C" UINT sound_recording_read(SINT16 *destination, UINT capacity_frames,
                                      UINT64 *sample_begin, UINT32 *epoch) {
	assert(fake_recording_active);
	assert(destination != nullptr);
	assert(sample_begin != nullptr);
	assert(epoch != nullptr);
	*sample_begin = fake_next_sample;
	*epoch = fake_recording_epoch;
	UINT count = capacity_frames;
	if (fake_limit_audio_to_guest_time) {
		const std::uint64_t available =
		    (fake_stream.generated_sample_count > fake_next_sample)
		        ? fake_stream.generated_sample_count - fake_next_sample
		        : 0u;
		count = static_cast<UINT>(std::min<std::uint64_t>(count, available));
	}
	for (UINT frame = 0u; frame < count; ++frame) {
		const SINT16 sample = static_cast<SINT16>(fake_next_sample + frame);
		destination[frame * 2u] = sample;
		destination[frame * 2u + 1u] = static_cast<SINT16>(-sample);
	}
	fake_next_sample += count;
	return count;
}

extern "C" BOOL sound_recording_get_state(SOUNDRECORDINGSTATE *state) {
	if (state == nullptr) {
		return FAILURE;
	}
	*state = {};
	state->rate = fake_stream.rate;
	state->first_sample = fake_capture_first_sample;
	state->next_sample = fake_stream.generated_sample_count;
	state->epoch = fake_recording_epoch;
	state->active = fake_recording_active ? TRUE : FALSE;
	return SUCCESS;
}

int main() {
	/* Automatic splitting is explicitly opt-in, never implicit for batch callers. */
	assert(!vaeg_recording_native_session_auto_split_enabled());
	test_native_session_writes_complete_intervals();
	test_epoch_change_cancels_without_publish();
	test_start_uses_quiesced_audio_clock_snapshot();
	test_audio_intervals_match_guest_sample_generation();
	test_start_rejects_invalid_and_existing_targets();
	test_manual_stop_finalizes_complete_movie();
	test_double_start_and_unicode_path();
	return 0;
}
