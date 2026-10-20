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
#include "recording/recording_native_session.h"

#ifndef VAEG_ENABLE_RECORDING
#define VAEG_ENABLE_RECORDING 0
#endif

#if !VAEG_ENABLE_RECORDING

int vaeg_recording_session_start(const char *path, uint32_t source, uint64_t frame_limit) {
	(void)path;
	(void)source;
	(void)frame_limit;
	return 0;
}

int vaeg_recording_native_session_start(const char *path, uint64_t frame_limit) {
	(void)path;
	(void)frame_limit;
	return 0;
}

int vaeg_recording_native_session_start_from_environment(void) {
	return 1;
}
void vaeg_recording_native_session_enable_auto_split(void) {
}
int vaeg_recording_native_session_auto_split_enabled(void) {
	return 0;
}
void vaeg_recording_native_session_request_split(void) {
}
const char *vaeg_recording_native_session_output_path(void) {
	return "";
}
VAEG_RECORDING_NATIVE_TAP_ERROR vaeg_recording_native_session_submit_frame(
    uint64_t sequence, uint64_t end_ticks, uint32_t generation,
    const VAEG_RECORDING_NATIVE_FRAME *frame, const VAEG_RECORDING_OSD_SNAPSHOT *osd) {
	return vaeg_recording_native_tap_submit_frame(sequence, end_ticks, generation, frame, osd);
}

int vaeg_recording_native_session_after_guest_exec(void) {
	return 1;
}

int vaeg_recording_native_session_is_active(void) {
	return 0;
}

int vaeg_recording_session_is_displayed(void) {
	return 0;
}
int vaeg_recording_session_displayed_capture_changed(void) {
	return 0;
}
int vaeg_recording_native_session_should_stop(void) {
	return 0;
}

int vaeg_recording_native_session_request_stop(void) {
	return 0;
}

int vaeg_recording_native_session_poll_stop(void) {
	return 1;
}

int vaeg_recording_native_session_is_finalizing(void) {
	return 0;
}

uint64_t vaeg_recording_native_session_elapsed_ticks(void) {
	return 0u;
}

uint64_t vaeg_recording_native_session_accepted_frames(void) {
	return 0u;
}

int vaeg_recording_native_session_stop(void) {
	return 1;
}

void vaeg_recording_native_session_cancel(void) {
}

void vaeg_recording_native_session_before_reset(void) {
}
void vaeg_recording_native_session_epoch_change(void) {
}

const char *vaeg_recording_native_session_last_error(void) {
	return "recording_disabled";
}

#else

#include "compiler.h"
#include "machine/pccore.h"
#include "recording/recording.h"
#include "recording/recording_clock.hpp"
#include "recording/recording_ffmpeg_matroska.hpp"
#include "recording/recording_ffmpeg_pcm_encoder.hpp"
#include "recording/recording_ffmpeg_video_encoder.hpp"
#include "recording/recording_native_tap.h"
#include "recording/recording_queue.hpp"
#include "scrnmng.h"
#include "sound.h"
#include "vaeg_build_info.h"

#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <limits>
#include <memory>
#include <new>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace vaeg::recording {
namespace {

using Wide = unsigned __int128;
using SignedWide = __int128;
namespace fs = std::filesystem;

bool wide_mul(Wide left, Wide right, Wide *result) noexcept {
	if (result == nullptr || (right != 0u && left > (~static_cast<Wide>(0u)) / right)) {
		return false;
	}
	*result = left * right;
	return true;
}

bool wide_add(Wide left, Wide right, Wide *result) noexcept {
	if (result == nullptr || left > (~static_cast<Wide>(0u)) - right) {
		return false;
	}
	*result = left + right;
	return true;
}

std::uint64_t gcd_u64(std::uint64_t left, std::uint64_t right) noexcept {
	while (right != 0u) {
		const std::uint64_t remainder = left % right;
		left = right;
		right = remainder;
	}
	return left;
}

bool reduce_fraction(std::uint64_t numerator, std::uint64_t denominator,
                     std::uint64_t *reduced_numerator,
                     std::uint64_t *reduced_denominator) noexcept {
	std::uint64_t divisor;
	if (reduced_numerator == nullptr || reduced_denominator == nullptr || denominator == 0u) {
		return false;
	}
	if (numerator == 0u) {
		*reduced_numerator = 0u;
		*reduced_denominator = 1u;
		return true;
	}
	divisor = gcd_u64(numerator, denominator);
	*reduced_numerator = numerator / divisor;
	*reduced_denominator = denominator / divisor;
	return true;
}

const char *queue_failure_name(QueueError error) noexcept {
	return queue_error_name(error);
}

class NativeRecordingSink final : public SliceSink {
  public:
	static std::unique_ptr<NativeRecordingSink> create(
	    const std::string &path, const VAEG_RECORDING_VIDEO_DESCRIPTOR &video,
	    const VAEG_RECORDING_AUDIO_DESCRIPTOR &audio, std::uint64_t movie_start_ticks,
	    std::uint64_t audio_epoch_sample, std::uint64_t audio_offset_numerator,
	    std::uint64_t audio_offset_denominator, std::uint32_t clock_rate,
	    bool displayed, std::string *error) noexcept {
	try {
		VideoEncoderConfig video_config{};
		video_config.width = video.width;
		video_config.height = video.height;
		video_config.time_base_num = 1;
		video_config.time_base_den = static_cast<std::int32_t>(clock_rate);
		VideoEncoderCreateResult video_create = FfmpegVideoEncoder::create(video_config);
		if (!video_create) {
			if (error != nullptr) {
				*error = ffmpeg_video_encoder_error_name(video_create.error);
			}
			return nullptr;
		}

		PcmEncoderConfig audio_config{};
		audio_config.sample_rate_hz = audio.sample_rate_hz;
		audio_config.origin_offset_numerator = audio_offset_numerator;
		audio_config.origin_offset_denominator = audio_offset_denominator;
		PcmEncoderCreateResult audio_create = FfmpegPcmEncoder::create(audio_config);
		if (!audio_create) {
			if (error != nullptr) {
				*error = ffmpeg_pcm_encoder_error_name(audio_create.error);
			}
			return nullptr;
		}

		MatroskaWriterConfig writer_config{};
		writer_config.final_path = path;
		writer_config.video_time_base_num = 1;
		writer_config.video_time_base_den = static_cast<std::int32_t>(clock_rate);
		writer_config.video_epoch_pts = static_cast<std::int64_t>(movie_start_ticks);
		writer_config.audio_epoch_sample_index = audio_epoch_sample;
		writer_config.source = displayed ? "displayed" : "native";
		writer_config.build_identity = VAEG_BUILD_COMMIT;
		writer_config.osd_included = true;
		MatroskaWriterCreateResult writer_create = FfmpegMatroskaWriter::create(
		    writer_config, video_create.encoder->info(), audio_create.encoder->info());
		if (!writer_create) {
			if (error != nullptr) {
				*error = ffmpeg_matroska_writer_error_name(writer_create.error);
			}
			return nullptr;
		}

		return std::unique_ptr<NativeRecordingSink>(new NativeRecordingSink(
		    std::move(video_create.encoder), std::move(audio_create.encoder),
		    std::move(writer_create.writer)));
	} catch (const std::bad_alloc &) {
		if (error != nullptr) {
			*error = "allocation_failure";
		}
		return nullptr;
	}
	}

	QueueError write(const CaptureSlice &slice) noexcept override {
		try {
			std::vector<EncodedVideoPacket> video_packets;
			std::vector<EncodedPcmPacket> audio_packets;
			const VAEG_RECORDING_VIDEO_DESCRIPTOR &video = slice.video_descriptor();
			if (slice.begin_ticks() > static_cast<std::uint64_t>(INT64_MAX) ||
			    slice.end_ticks() > static_cast<std::uint64_t>(INT64_MAX)) {
				last_error_ = "video_time_overflow";
				return QueueError::SinkFailure;
			}
			const VideoEncoderFrame video_frame{
			    video, slice.video_bytes().data(), slice.video_bytes().size(), slice.sequence(),
			    static_cast<std::int64_t>(slice.begin_ticks())};
			if (video_encoder_->encode(video_frame, &video_packets) !=
			    FfmpegVideoEncoderError::None) {
				last_error_ = "video_encode_failure";
				return QueueError::SinkFailure;
			}
			if (video_packets.size() != 1u ||
			    slice.end_ticks() - slice.begin_ticks() >
			        static_cast<std::uint64_t>(INT64_MAX)) {
				last_error_ = "video_packet_interval_mismatch";
				return QueueError::SinkFailure;
			}
			video_packets.front().duration = static_cast<std::int64_t>(
			    slice.end_ticks() - slice.begin_ticks());

			const std::vector<std::int16_t> &audio = slice.audio_samples();
			if (audio.empty() || (audio.size() % 2u) != 0u) {
				last_error_ = "audio_interval_empty";
				return QueueError::SinkFailure;
			}
			std::vector<std::int32_t> audio_s32(audio.size());
			for (std::size_t index = 0u; index < audio.size(); ++index) {
				audio_s32[index] = static_cast<std::int32_t>(audio[index]);
			}
			const PcmSampleBlock audio_block{
			    audio_s32.data(), static_cast<std::uint64_t>(audio.size() / 2u),
			    static_cast<std::uint64_t>(audio_s32.size() * sizeof(audio_s32[0])),
			    slice.audio_sample_begin()};
			if (audio_encoder_->encode(audio_block, &audio_packets) !=
			    FfmpegPcmEncoderError::None) {
				last_error_ = "audio_encode_failure";
				return QueueError::SinkFailure;
			}
			FfmpegMatroskaWriterError mux_error = FfmpegMatroskaWriterError::InvalidInput;
			if (video_packets.empty()) {
				last_error_ = "video_encoder_no_packet";
				return QueueError::SinkFailure;
			}
			if (audio_packets.empty()) {
				last_error_ = "audio_encoder_no_packet";
				return QueueError::SinkFailure;
			}
			mux_error = writer_->write_interval(video_packets, audio_packets);
			if (mux_error != FfmpegMatroskaWriterError::None) {
				last_error_ = "mux_write_" +
				               std::string(ffmpeg_matroska_writer_error_name(
				                   mux_error == FfmpegMatroskaWriterError::None
				                       ? FfmpegMatroskaWriterError::InvalidInput
				                       : mux_error));
				return QueueError::SinkFailure;
			}
			return QueueError::None;
		} catch (const std::bad_alloc &) {
			last_error_ = "allocation_failure";
			return QueueError::AllocationFailure;
		}
	}

	QueueError finish() noexcept override {
		try {
			std::vector<EncodedVideoPacket> video_packets;
			std::vector<EncodedPcmPacket> audio_packets;
			if (video_encoder_->flush(&video_packets) != FfmpegVideoEncoderError::None ||
			    audio_encoder_->flush(&audio_packets) != FfmpegPcmEncoderError::None) {
				last_error_ = "encoder_flush_failure";
				return QueueError::SinkFailure;
			}
			if (!video_packets.empty() || !audio_packets.empty()) {
				FfmpegMatroskaWriterError mux_error = FfmpegMatroskaWriterError::InvalidInput;
				if (!video_packets.empty() && !audio_packets.empty()) {
					mux_error = writer_->write_interval(video_packets, audio_packets);
				}
				if (mux_error != FfmpegMatroskaWriterError::None) {
					last_error_ = "mux_flush_" +
					               std::string(ffmpeg_matroska_writer_error_name(
					                   mux_error == FfmpegMatroskaWriterError::None
					                       ? FfmpegMatroskaWriterError::InvalidInput
					                       : mux_error));
					return QueueError::SinkFailure;
				}
			}
			if (writer_->finish() != FfmpegMatroskaWriterError::None) {
				last_error_ = "mux_finalize_failure";
				return QueueError::SinkFailure;
			}
			finalized_ = true;
			return QueueError::None;
		} catch (const std::bad_alloc &) {
			last_error_ = "allocation_failure";
			return QueueError::AllocationFailure;
		}
	}

	void cancel() noexcept override {
		if (!finalized_ && writer_ != nullptr) {
			(void)writer_->cancel();
			finalized_ = true;
		}
	}

	const std::string &last_error() const noexcept {
		return last_error_;
	}

	const char *error_name() const noexcept override {
		return last_error_.empty() ? nullptr : last_error_.c_str();
	}

  private:
	NativeRecordingSink(std::unique_ptr<FfmpegVideoEncoder> video_encoder,
	                   std::unique_ptr<FfmpegPcmEncoder> audio_encoder,
	                   std::unique_ptr<FfmpegMatroskaWriter> writer) noexcept
	    : video_encoder_(std::move(video_encoder)), audio_encoder_(std::move(audio_encoder)),
	      writer_(std::move(writer)), finalized_(false), last_error_() {
	}

	std::unique_ptr<FfmpegVideoEncoder> video_encoder_;
	std::unique_ptr<FfmpegPcmEncoder> audio_encoder_;
	std::unique_ptr<FfmpegMatroskaWriter> writer_;
	bool finalized_;
	std::string last_error_;
};

struct StagedCapture {
	std::vector<std::uint8_t> pixels;
	std::uint32_t width = 0u;
	std::uint32_t height = 0u;
	std::uint32_t pitch = 0u;
	std::uint64_t sequence = 0u;
	std::uint64_t begin_ticks = 0u;
	std::uint64_t end_ticks = 0u;
	std::uint32_t descriptor_generation = 0u;
	VAEG_RECORDING_OSD_SNAPSHOT osd{};
	bool ready = false;

	void swap(StagedCapture &other) noexcept {
		pixels.swap(other.pixels);
		std::swap(width, other.width);
		std::swap(height, other.height);
		std::swap(pitch, other.pitch);
		std::swap(sequence, other.sequence);
		std::swap(begin_ticks, other.begin_ticks);
		std::swap(end_ticks, other.end_ticks);
		std::swap(descriptor_generation, other.descriptor_generation);
		std::swap(osd, other.osd);
		std::swap(ready, other.ready);
	}
};

class NativeRecordingSession;

class NativeRecordingSession {
  public:
	NativeRecordingSession(const std::string &path, std::uint64_t frame_limit,
	                       bool displayed) noexcept
	    : path_(path), displayed_(displayed), frame_limit_(frame_limit), descriptor_generation_(0u),
	      audio_epoch_(0u), audio_rate_(0u), sound_started_(false), writer_started_(false),
	      failed_(false), stop_requested_(false), manual_stop_requested_(false),
	      audio_primed_(false), accepted_intervals_(0u), movie_start_ticks_(0u),
	      last_end_ticks_(0u), next_audio_sample_(0u), last_error_() {
		std::memset(&video_descriptor_, 0, sizeof(video_descriptor_));
		std::memset(&audio_descriptor_, 0, sizeof(audio_descriptor_));
		std::memset(&grid_, 0, sizeof(grid_));
	}

	~NativeRecordingSession() {
		abort("session_destroyed");
	}

	bool start() noexcept {
		SOUNDSTREAMSTATE stream{};
		SOUNDSTREAMSTATE armed_stream{};
		SOUNDRECORDINGSTATE recording{};
		std::uint64_t capacity;
		std::uint64_t start_ticks;
		const UINT32 clock_rate = pccore.realclock;
		if (!validate_output_path()) {
			return false;
		}
		if (displayed_) {
			if (scrnmng_get_displayed_recording_video_descriptor(
			        &video_descriptor_, &descriptor_generation_) != SUCCESS) {
				last_error_ = "displayed_source_unavailable";
				return false;
			}
		} else {
			if (scrnmng_get_native_recording_video_descriptor(&video_descriptor_) != SUCCESS) {
				last_error_ = "native_descriptor_unavailable";
				return false;
			}
			descriptor_generation_ = scrnmng_native_recording_descriptor_generation();
		}
		if (descriptor_generation_ == 0u || sound_get_stream_state(&stream) != SUCCESS ||
		    stream.rate == 0u || clock_rate == 0u || stream.accounting_overflow ||
		    stream.generated_sample_count > static_cast<std::uint64_t>(INT64_MAX)) {
			last_error_ = "recording_clock_unavailable";
			return false;
		}
		audio_rate_ = stream.rate;
		audio_descriptor_ = {audio_rate_, 2u, 16u, VAEG_RECORDING_AUDIO_PCM_S16LE};
		capacity = static_cast<std::uint64_t>(stream.pending_samples) +
		           static_cast<std::uint64_t>(audio_rate_) * 2u;
		if (capacity == 0u || capacity > static_cast<std::uint64_t>(UINT_MAX)) {
			last_error_ = "audio_capacity_overflow";
			return false;
		}
		if (sound_recording_start(static_cast<UINT>(capacity)) != SOUND_RECORDING_ERROR_OK) {
			last_error_ = "audio_capture_start_failed";
			return false;
		}
		sound_started_ = true;
		if (sound_get_stream_state(&armed_stream) != SUCCESS ||
		    sound_recording_get_state(&recording) != SUCCESS) {
			last_error_ = "audio_clock_snapshot_unavailable";
			return false;
		}
		if (armed_stream.rate != audio_rate_ ||
		    armed_stream.generation_epoch != stream.generation_epoch ||
		    armed_stream.accounting_overflow || armed_stream.pcm_locked ||
		    armed_stream.generated_sample_count > static_cast<std::uint64_t>(INT64_MAX) ||
		    recording.rate != armed_stream.rate ||
		    recording.epoch != armed_stream.generation_epoch ||
		    !recording.active ||
		    recording.first_sample > recording.next_sample ||
		    recording.next_sample != armed_stream.generated_sample_count) {
			last_error_ = "audio_clock_snapshot_invalid";
			return false;
		}
		start_ticks = pccore_guest_clock();
		if (pccore.realclock != clock_rate) {
			last_error_ = "recording_clock_changed_during_start";
			return false;
		}
		audio_epoch_ = recording.epoch;
		const std::uint32_t delta =
		    static_cast<std::uint32_t>(static_cast<std::uint32_t>(start_ticks) -
		                               armed_stream.lastclock);
		if (static_cast<std::uint64_t>(delta) > start_ticks) {
			last_error_ = "audio_clock_origin_invalid";
			return false;
		}
		grid_.sample_rate_hz = audio_rate_;
		grid_.clock_rate_hz = clock_rate;
		grid_.origin_ticks = start_ticks - delta;
		grid_.origin_sample_numerator =
		    static_cast<std::int64_t>(armed_stream.generated_sample_count) - 1;
		grid_.origin_sample_denominator = 1u;
		try {
			const std::size_t video_bytes = static_cast<std::size_t>(video_descriptor_.stride_bytes) *
			                                video_descriptor_.height;
			staged_.pixels.resize(video_bytes);
			incoming_.pixels.resize(video_bytes);
		} catch (const std::bad_alloc &) {
			last_error_ = "native_stage_allocation_failed";
			return false;
		}
		capture_start_ticks_ = start_ticks;
		if (displayed_) {
			if (scrnmng_start_displayed_recording_capture(
			        &NativeRecordingSession::displayed_callback, this) != SUCCESS) {
				last_error_ = vaeg_recording_displayed_capture_error_name(
				    scrnmng_displayed_recording_capture_error());
				return false;
			}
			displayed_capture_owned_ = true;
		} else if (vaeg_recording_native_tap_start(&NativeRecordingSession::tap_callback, this,
		                                           start_ticks, descriptor_generation_) !=
		           VAEG_RECORDING_NATIVE_TAP_OK) {
			last_error_ = "native_tap_start_failed";
			return false;
		}
		return true;
	}

	bool after_guest_exec() noexcept {
		if (awaiting_epoch_)
			return true;
		if (stop_requested_ && !writer_started_ && !failed_)
			return true;
		if (failed_) {
			return false;
		}
		if (displayed_ ? !scrnmng_displayed_recording_capture_active()
		               : !vaeg_recording_native_tap_is_active()) {
			fail_and_cancel(displayed_ ? vaeg_recording_displayed_capture_error_name(
			                                scrnmng_displayed_recording_capture_error())
			                          : "native_tap_stopped");
			return false;
		}
		if (!incoming_.ready) {
			return true;
		}
		if (!staged_.ready) {
			staged_.swap(incoming_);
			return prime_audio();
		}
		if (!commit_interval(staged_, incoming_.end_ticks)) {
			fail_and_cancel(last_error_.empty() ? "native_interval_failed" : last_error_.c_str());
			return false;
		}
		staged_.swap(incoming_);
		incoming_.ready = false;
		if ((frame_limit_ != 0u) && (accepted_intervals_ >= frame_limit_)) {
			stop_requested_ = true;
		}
		return true;
	}

	bool is_displayed() const noexcept { return displayed_; }
	bool displayed_capture_changed() const noexcept {
		if (!displayed_ || scrnmng_displayed_recording_capture_active()) return false;
		const auto error = scrnmng_displayed_recording_capture_error();
		return error == VAEG_RECORDING_DISPLAYED_CAPTURE_GEOMETRY_CHANGED ||
		       error == VAEG_RECORDING_DISPLAYED_CAPTURE_PRESENTER_CHANGED ||
		       error == VAEG_RECORDING_DISPLAYED_CAPTURE_UNAVAILABLE;
	}
	void enable_auto_split() noexcept {
		/* Native snapshots can rearm at a guest boundary. Displayed descriptors
		 * stop and save on presentation changes instead of fabricating pixels. */
		auto_split_ = !displayed_;
	}
	bool auto_split_enabled() const noexcept {
		return auto_split_;
	}
	void request_split() noexcept {
		split_pending_ = true;
	}
	const std::string &output_path() const noexcept {
		return segment_path_.empty() ? path_ : segment_path_;
	}
	const std::string &published_path() const noexcept {
		return published_path_;
	}

	VAEG_RECORDING_NATIVE_TAP_ERROR submit_frame(std::uint64_t sequence, std::uint64_t ticks,
	                                             std::uint32_t generation,
	                                             const VAEG_RECORDING_NATIVE_FRAME *frame,
	                                             const VAEG_RECORDING_OSD_SNAPSHOT *osd) {
		if (displayed_ || (stop_requested_ && !writer_started_))
			return VAEG_RECORDING_NATIVE_TAP_OK;
		if (awaiting_epoch_) {
			if (manual_stop_requested_) {
				stop_requested_ = true;
				return VAEG_RECORDING_NATIVE_TAP_OK;
			}
			if (ticks != pccore_guest_clock() || !start()) {
				if (last_error_.empty())
					last_error_ = "segment_epoch_rearm_failed";
				return VAEG_RECORDING_NATIVE_TAP_ERROR_CALLBACK_REJECTED;
			}
			awaiting_epoch_ = false;
			split_pending_ = false;
			return vaeg_recording_native_tap_submit_initial_snapshot(sequence, ticks, generation,
			                                                         frame, osd);
		}
		if (!auto_split_ || (!split_pending_ && generation == descriptor_generation_)) {
			return vaeg_recording_native_tap_submit_frame(sequence, ticks, generation, frame, osd);
		}
		/* Keep the same capture FIFO and rational sample grid across segments.
		 * Rearming audio here can lose samples already delivered to the device. */
		if (!complete_boundary(ticks))
			return VAEG_RECORDING_NATIVE_TAP_ERROR_CALLBACK_REJECTED;
		vaeg_recording_native_tap_stop();
		if (stop_requested_ || manual_stop_requested_) {
			stop_requested_ = true;
			return VAEG_RECORDING_NATIVE_TAP_OK;
		}
		if (!advance_segment())
			return VAEG_RECORDING_NATIVE_TAP_ERROR_CALLBACK_REJECTED;
		if (scrnmng_get_native_recording_video_descriptor(&video_descriptor_) != SUCCESS) {
			last_error_ = "segment_descriptor_or_path_invalid";
			return VAEG_RECORDING_NATIVE_TAP_ERROR_CALLBACK_REJECTED;
		}
		descriptor_generation_ = scrnmng_native_recording_descriptor_generation();
		if (descriptor_generation_ != generation) {
			last_error_ = "segment_descriptor_changed_during_split";
			return VAEG_RECORDING_NATIVE_TAP_ERROR_CALLBACK_REJECTED;
		}
		segment_first_interval_ = accepted_intervals_;
		staged_.ready = incoming_.ready = false;
		const std::size_t bytes =
		    static_cast<std::size_t>(video_descriptor_.stride_bytes) * video_descriptor_.height;
		staged_.pixels.resize(bytes);
		incoming_.pixels.resize(bytes);
		split_pending_ = false;
		if (vaeg_recording_native_tap_start(&NativeRecordingSession::tap_callback, this, ticks,
		                                    descriptor_generation_) !=
		    VAEG_RECORDING_NATIVE_TAP_OK) {
			last_error_ = "segment_tap_start_failed";
			return VAEG_RECORDING_NATIVE_TAP_ERROR_CALLBACK_REJECTED;
		}
		return vaeg_recording_native_tap_submit_initial_snapshot(sequence, ticks, generation, frame,
		                                                         osd);
	}

	/* Reset-driven monitor changes begin a new clock/audio epoch. Seal the old
	 * epoch before reset, then rearm on its first actual guest frame. */
	bool change_epoch() {
		if (awaiting_epoch_)
			return true;
		if (!complete_boundary(pccore_guest_clock()))
			return false;
		vaeg_recording_native_tap_stop();
		if (sound_started_) {
			sound_recording_stop();
			sound_recording_release();
			sound_started_ = false;
		}
		if (stop_requested_ || manual_stop_requested_)
			return false;
		if (!advance_segment())
			return false;
		completed_epoch_ticks_ = elapsed_ticks();
		audio_primed_ = false;
		staged_.ready = incoming_.ready = false;
		segment_first_interval_ = accepted_intervals_;
		split_pending_ = false;
		awaiting_epoch_ = true;
		return true;
	}

	bool stop() noexcept {
		if (failed_) {
			abort(last_error_.empty() ? "recording_failed" : last_error_.c_str());
			return false;
		}
		/* The last actually displayed image holds through this guest boundary.
		 * Seal an orderly descriptor change, never an arbitrary readback failure. */
		if (displayed_ && (scrnmng_displayed_recording_capture_active() ||
		                   displayed_capture_changed()) &&
		    !complete_boundary(pccore_guest_clock())) {
			abort(last_error_.c_str());
			return false;
		}
		stop_video_capture();
		if (sound_started_) {
			sound_recording_stop();
		}
		const bool result = finish_segment();
		if (sound_started_) {
			sound_recording_release();
			sound_started_ = false;
		}
		if (result && accepted_intervals_ == 0u)
			last_error_ = "recording_no_intervals";
		return result && accepted_intervals_ != 0u;
	}

	void abort(const char *reason) noexcept {
		if (reason != nullptr && last_error_.empty()) {
			last_error_ = reason;
		}
		stop_video_capture();
		if (writer_started_) {
			(void)session_.cancel();
			writer_started_ = false;
		}
		if (sound_started_) {
			sound_recording_stop();
			sound_recording_release();
			sound_started_ = false;
		}
		failed_ = true;
	}

	void fail_and_cancel(const char *reason) noexcept {
		if (reason != nullptr) {
			last_error_ = reason;
		}
		abort(reason);
	}

	bool should_stop() const noexcept {
		return stop_requested_;
	}

	bool request_stop() noexcept {
		if (failed_) {
			return false;
		}
		manual_stop_requested_ = true;
		return true;
	}

	bool manual_stop_requested() const noexcept {
		return manual_stop_requested_;
	}

	std::uint64_t elapsed_ticks() const noexcept {
		if (!audio_primed_ || last_end_ticks_ < recording_start_ticks_) {
			return completed_epoch_ticks_;
		}
		return completed_epoch_ticks_ + last_end_ticks_ - recording_start_ticks_;
	}

	std::uint64_t accepted_intervals() const noexcept {
		return accepted_intervals_;
	}

	const std::string &last_error() const noexcept {
		return last_error_;
	}

  private:
	void stop_video_capture() noexcept {
		if (displayed_) {
			if (displayed_capture_owned_) {
				scrnmng_stop_displayed_recording_capture();
				displayed_capture_owned_ = false;
			}
		} else {
			vaeg_recording_native_tap_stop();
		}
	}
	static int displayed_callback(void *context,
	                              const VAEG_RECORDING_DISPLAYED_FRAME *capture) {
		return static_cast<NativeRecordingSession *>(context)->accept_displayed(capture) ? 1 : 0;
	}
	bool accept_displayed(const VAEG_RECORDING_DISPLAYED_FRAME *capture) noexcept {
		const std::uint64_t ticks = pccore_guest_clock();
		if (failed_ || stop_requested_) return !failed_;
		if (capture == nullptr || incoming_.ready ||
		    capture->descriptor_generation != descriptor_generation_ ||
		    capture->width != video_descriptor_.width ||
		    capture->height != video_descriptor_.height || ticks < capture_start_ticks_ ||
		    pccore.realclock != grid_.clock_rate_hz ||
		    (staged_.ready && (ticks <= staged_.end_ticks ||
		                      (capture->frame_token != staged_.sequence + 1u &&
		                       !(staged_.sequence == UINT32_MAX && capture->frame_token == 0u))))) {
			last_error_ = "displayed_capture_mismatch";
			failed_ = true;
			return false;
		}
		const auto error = vaeg_recording_displayed_capture_to_rgb24(
		    capture, incoming_.pixels.data(), static_cast<std::uint32_t>(video_descriptor_.stride_bytes),
		    incoming_.pixels.size());
		if (error != VAEG_RECORDING_DISPLAYED_CAPTURE_OK) {
			last_error_ = vaeg_recording_displayed_capture_error_name(error);
			failed_ = true;
			return false;
		}
		incoming_.width = capture->width;
		incoming_.height = capture->height;
		incoming_.pitch = static_cast<std::uint32_t>(video_descriptor_.stride_bytes);
		incoming_.sequence = capture->frame_token;
		incoming_.begin_ticks = staged_.ready ? staged_.end_ticks : capture_start_ticks_;
		incoming_.end_ticks = ticks;
		incoming_.descriptor_generation = capture->descriptor_generation;
		incoming_.ready = true;
		/* Process while this exact guest boundary is still current. Workers
		 * receive only owned RGB/PCM slices, never a GPU or borrowed pointer. */
		return after_guest_exec();
	}
	bool advance_segment() {
		if (segment_index_ == UINT64_MAX) {
			last_error_ = "segment_index_overflow";
			return false;
		}
		++segment_index_;
		std::string number = std::to_string(segment_index_);
		if (number.size() < 3)
			number.insert(0, 3 - number.size(), '0');
		segment_path_ = path_.substr(0, path_.size() - 4) + "-part-" + number + ".mkv";
		if (segment_path_.size() >= 1024) {
			last_error_ = "segment_path_too_long";
			return false;
		}
		return true;
	}
	bool complete_boundary(std::uint64_t ticks) noexcept {
		if (ticks != pccore_guest_clock()) {
			last_error_ = "segment_not_at_guest_boundary";
			return false;
		}
		sound_sync();
		if (displayed_ && displayed_capture_changed()) {
			/* The old image was held until this boundary even though the new
			 * drawable is incompatible. No new pixels enter its descriptor. */
			if (failed_ || incoming_.ready) return false;
		} else if (!after_guest_exec()) {
			return false;
		}
		if (staged_.ready && ticks < staged_.end_ticks) {
			last_error_ = "segment_time_regressed";
			return false;
		}
		if (staged_.ready && !stop_requested_ && ticks > staged_.end_ticks &&
		    !commit_interval(staged_, ticks))
			return false;
		return finish_segment();
	}
	bool finish_segment() noexcept {
		if (!writer_started_)
			return true;
		const std::uint64_t count = accepted_intervals_ - segment_first_interval_;
		const QueueError result = count == 0u ? session_.cancel() : session_.stop();
		if (result != QueueError::None || session_.written_slices() != count) {
			const char *error = session_.sink_error_name();
			last_error_ = result == QueueError::None
			                  ? "written_slice_count_mismatch"
			                  : (error != nullptr ? error : queue_failure_name(result));
			failed_ = true;
			return false;
		}
		writer_started_ = false;
		if (count != 0u) {
			try {
				published_path_ = output_path();
			} catch (const std::bad_alloc &) {
				failed_ = true;
				return false;
			}
		}
		/* A canceled empty segment is terminal and needs explicit acknowledgement. */
		if (count == 0u && session_.acknowledge_terminal() != QueueError::None) {
			last_error_ = "segment_empty_acknowledge_failed";
			return false;
		}
		return true;
	}

	bool validate_output_path() noexcept {
		const std::string &output_path_string = output_path();
		std::error_code error;
		fs::path output;

		if (output_path_string.empty()) {
			last_error_ = "recording_path_empty";
			return false;
		}
		if (output_path_string.find("://") != std::string::npos) {
			last_error_ = "recording_path_not_local";
			return false;
		}
		try {
			output = fs::u8path(output_path_string);
		} catch (const fs::filesystem_error &) {
			last_error_ = "recording_path_invalid";
			return false;
		}
		if (output.empty() || output.filename() == fs::path()) {
			last_error_ = "recording_path_invalid";
			return false;
		}
		if (output.extension().u8string() != ".mkv") {
			last_error_ = "recording_extension_invalid";
			return false;
		}
		if (fs::exists(output, error)) {
			last_error_ = "recording_target_exists";
			return false;
		}
		if (error) {
			last_error_ = "recording_path_invalid";
			return false;
		}
		const fs::path parent = output.parent_path().empty() ? fs::path(".")
		                                                   : output.parent_path();
		if (!fs::is_directory(parent, error) || error) {
			last_error_ = "recording_parent_unavailable";
			return false;
		}
		return true;
	}

	static int tap_callback(void *context, const VAEG_RECORDING_NATIVE_CAPTURE *capture) {
		return static_cast<NativeRecordingSession *>(context)->accept_capture(capture) ? 1 : 0;
	}

	bool accept_capture(const VAEG_RECORDING_NATIVE_CAPTURE *capture) noexcept {
		if (capture == nullptr || capture->frame == nullptr || capture->osd == nullptr ||
		    incoming_.ready || capture->descriptor_generation != descriptor_generation_ ||
		    capture->frame->width != video_descriptor_.width ||
		    capture->frame->height != video_descriptor_.height ||
		    capture->frame->pitch_bytes != video_descriptor_.stride_bytes) {
			last_error_ = "native_capture_mismatch";
			return false;
		}
		const std::size_t size = static_cast<std::size_t>(video_descriptor_.stride_bytes) *
		                         video_descriptor_.height;
		std::memcpy(incoming_.pixels.data(), capture->frame->pixels, size);
		incoming_.width = capture->frame->width;
		incoming_.height = capture->frame->height;
		incoming_.pitch = capture->frame->pitch_bytes;
		incoming_.sequence = capture->sequence;
		incoming_.begin_ticks = capture->begin_ticks;
		incoming_.end_ticks = capture->end_ticks;
		incoming_.descriptor_generation = capture->descriptor_generation;
		incoming_.osd = *capture->osd;
		incoming_.ready = true;
		return true;
	}

	bool discard_audio_before(std::uint64_t target) noexcept {
		SOUNDRECORDINGSTATE state{};
		std::vector<std::int16_t> scratch;
		if (sound_recording_get_state(&state) != SUCCESS || state.epoch != audio_epoch_ ||
		    state.first_sample > target) {
			last_error_ = "audio_epoch_or_origin_mismatch";
			return false;
		}
		std::uint64_t remaining = target - state.first_sample;
		try {
			scratch.resize(1024u * 2u);
		} catch (const std::bad_alloc &) {
			last_error_ = "audio_discard_allocation_failed";
			return false;
		}
		std::uint64_t expected = state.first_sample;
		while (remaining != 0u) {
			const UINT request = static_cast<UINT>(std::min<std::uint64_t>(remaining, 1024u));
			UINT64 sample_begin = 0u;
			UINT32 epoch = 0u;
			const UINT count = sound_recording_read(scratch.data(), request, &sample_begin, &epoch);
			if (count == 0u || sample_begin != expected || epoch != audio_epoch_) {
				last_error_ = "audio_discard_underflow";
				return false;
			}
			expected += count;
			remaining -= count;
		}
		return true;
	}

	bool read_audio(std::uint64_t first_sample, std::uint64_t frame_count,
	                std::vector<std::int16_t> *audio) noexcept {
		if (audio == nullptr || frame_count == 0u || frame_count > UINT_MAX) {
			last_error_ = "audio_interval_invalid";
			return false;
		}
		try {
			audio->resize(static_cast<std::size_t>(frame_count) * 2u);
		} catch (const std::bad_alloc &) {
			last_error_ = "audio_interval_allocation_failed";
			return false;
		}
		std::uint64_t expected = first_sample;
		std::uint64_t remaining = frame_count;
		std::size_t offset = 0u;
		while (remaining != 0u) {
			const UINT request = static_cast<UINT>(std::min<std::uint64_t>(remaining, 2048u));
			UINT64 sample_begin = 0u;
			UINT32 epoch = 0u;
			const UINT count = sound_recording_read(audio->data() + offset, request,
			                                       &sample_begin, &epoch);
			if (count == 0u || sample_begin != expected || epoch != audio_epoch_) {
				last_error_ = "audio_interval_underflow";
				return false;
			}
			expected += count;
			remaining -= count;
			offset += static_cast<std::size_t>(count) * 2u;
		}
		return true;
	}

	bool calculate_origin_offset(std::uint64_t movie_start, std::uint64_t sample_index,
	                             std::uint64_t *numerator,
	                             std::uint64_t *denominator) const noexcept {
		if (numerator == nullptr || denominator == nullptr || sample_index > INT64_MAX ||
		    grid_.origin_sample_denominator == 0u || movie_start < grid_.origin_ticks ||
		    grid_.sample_rate_hz == 0u || grid_.clock_rate_hz == 0u) {
			return false;
		}
		Wide sample_index_scaled;
		Wide origin_sample_abs;
		Wide sample_delta_numerator;
		Wide sample_time_numerator;
		Wide tick_numerator;
		Wide wide_denominator;
		if (!wide_mul(static_cast<Wide>(sample_index),
		              static_cast<Wide>(grid_.origin_sample_denominator),
		              &sample_index_scaled)) {
			return false;
		}
		const SignedWide signed_origin_sample =
		    static_cast<SignedWide>(grid_.origin_sample_numerator);
		origin_sample_abs = static_cast<Wide>(signed_origin_sample < 0
		                                          ? -signed_origin_sample
		                                          : signed_origin_sample);
		if (signed_origin_sample < 0) {
			if (!wide_add(sample_index_scaled, origin_sample_abs,
			              &sample_delta_numerator)) {
				return false;
			}
		} else {
			if (sample_index_scaled < origin_sample_abs) {
				return false;
			}
			sample_delta_numerator = sample_index_scaled - origin_sample_abs;
		}
		if (!wide_mul(sample_delta_numerator,
		              static_cast<Wide>(grid_.clock_rate_hz),
		              &sample_time_numerator) ||
		    !wide_mul(static_cast<Wide>(movie_start - grid_.origin_ticks),
		              static_cast<Wide>(grid_.sample_rate_hz), &tick_numerator) ||
		    !wide_mul(tick_numerator,
		              static_cast<Wide>(grid_.origin_sample_denominator),
		              &tick_numerator) ||
		    sample_time_numerator < tick_numerator ||
		    !wide_mul(static_cast<Wide>(grid_.sample_rate_hz),
		              static_cast<Wide>(grid_.clock_rate_hz), &wide_denominator) ||
		    !wide_mul(wide_denominator,
		              static_cast<Wide>(grid_.origin_sample_denominator),
		              &wide_denominator)) {
			return false;
		}
		const Wide value = sample_time_numerator - tick_numerator;
		if (wide_denominator == 0u || value > UINT64_MAX || wide_denominator > UINT64_MAX) {
			return false;
		}
		return reduce_fraction(static_cast<std::uint64_t>(value),
		                      static_cast<std::uint64_t>(wide_denominator), numerator,
		                      denominator) &&
		       *denominator <= static_cast<std::uint64_t>(INT32_MAX) && *numerator < *denominator;
	}

	bool prime_audio() noexcept {
		std::uint64_t first_sample;
		if (sample_boundary_ceil(grid_, staged_.end_ticks, &first_sample) != ClockError::None) {
			last_error_ = "audio_prime_clock_invalid";
			return false;
		}
		/* FIFO first_sample is its immutable origin, NOT its current read cursor.
		 * A same-epoch split already consumed exactly to this boundary. */
		if (audio_primed_ ? (first_sample != next_audio_sample_)
		                  : !discard_audio_before(first_sample)) {
			if (last_error_.empty()) last_error_ = "segment_audio_boundary_mismatch";
			return false;
		}
		next_audio_sample_ = first_sample;
		if (!calculate_origin_offset(staged_.end_ticks, next_audio_sample_,
	                             &origin_offset_numerator_, &origin_offset_denominator_) ||
		    !initialize_writer(staged_.end_ticks, next_audio_sample_)) {
			if (last_error_.empty()) {
				last_error_ = "audio_prime_failed";
			}
			return false;
		}
		if (!audio_primed_)
			recording_start_ticks_ = staged_.end_ticks;
		audio_primed_ = true;
		return true;
	}

	bool initialize_writer(std::uint64_t movie_start, std::uint64_t first_audio_sample) noexcept {
		if (pccore.realclock == 0u || pccore.realclock > static_cast<UINT32>(INT32_MAX)) {
			last_error_ = "video_clock_invalid";
			return false;
		}
		const std::uint64_t frame_bytes = video_descriptor_.stride_bytes * video_descriptor_.height;
		const std::uint64_t audio_reserve_bytes = static_cast<std::uint64_t>(audio_rate_) * 4u;
		if (frame_bytes > UINT64_MAX / (displayed_ ? 20u : 12u) ||
		    audio_reserve_bytes > UINT64_MAX / 12u) {
			last_error_ = "recording_budget_overflow";
			return false;
		}
		QueueConfig queue_config{};
		queue_config.budget.max_slices = 8u;
		queue_config.budget.active_interval_bytes = frame_bytes + audio_reserve_bytes;
		/* Displayed owns the reusable RGBA readback and two RGB snapshots in
		 * addition to queued/worker slices. Account for those explicitly. */
		queue_config.budget.readback_staging_bytes = displayed_
		    ? static_cast<std::uint64_t>(video_descriptor_.width) * video_descriptor_.height * 4u +
		          frame_bytes * 2u
		    : frame_bytes;
		queue_config.budget.max_bytes = frame_bytes * 12u + audio_reserve_bytes * 12u +
		    (queue_config.budget.readback_staging_bytes - frame_bytes);
		queue_config.video = video_descriptor_;
		queue_config.audio = audio_descriptor_;
		queue_config.descriptor_generation = descriptor_generation_;
		queue_config.first_begin_ticks = movie_start;
		queue_config.first_audio_sample = first_audio_sample;
		std::string sink_error;
		std::unique_ptr<NativeRecordingSink> sink = NativeRecordingSink::create(
		    output_path(), video_descriptor_, audio_descriptor_, movie_start, first_audio_sample,
		    origin_offset_numerator_, origin_offset_denominator_, pccore.realclock, displayed_, &sink_error);
		if (sink == nullptr) {
			last_error_ = sink_error.empty() ? "recording_sink_create_failed" : sink_error;
			return false;
		}
		if (session_.prepare(queue_config) != QueueError::None ||
		    session_.arm(std::unique_ptr<SliceSink>(std::move(sink))) != QueueError::None ||
		    session_.begin() != QueueError::None) {
			last_error_ = "recording_session_arm_failed";
			return false;
		}
		movie_start_ticks_ = movie_start;
		writer_started_ = true;
		return true;
	}

	bool commit_interval(const StagedCapture &frame, std::uint64_t end_ticks) noexcept {
		SampleRange range{};
		std::vector<std::int16_t> audio;
		if (!audio_primed_ || end_ticks <= frame.end_ticks ||
		    sample_range_for_interval(grid_, frame.end_ticks, end_ticks, &range) !=
		        ClockError::None ||
		    range.begin != next_audio_sample_ || range.count() == 0u ||
		    !read_audio(range.begin, range.count(), &audio)) {
			if (last_error_.empty()) {
				last_error_ = "recording_interval_clock_mismatch";
			}
			return false;
		}
		CaptureSliceInput input{};
		input.sequence = accepted_intervals_ - segment_first_interval_;
		input.begin_ticks = frame.end_ticks;
		input.end_ticks = end_ticks;
		input.audio_sample_begin = range.begin;
		input.audio_frame_count = range.count();
		input.descriptor_generation = descriptor_generation_;
		input.video = video_descriptor_;
		input.video_pixels = frame.pixels.data();
		input.video_bytes = frame.pixels.size();
		input.audio = audio_descriptor_;
		input.audio_s16 = audio.data();
		CaptureSliceResult slice = CaptureSlice::create(input);
		if (!slice) {
			last_error_ = queue_error_name(slice.error);
			return false;
		}
		const QueueError submit = session_.wait_submit(slice.slice);
		if (submit != QueueError::None) {
			const char *sink_error = session_.sink_error_name();
			last_error_ = sink_error != nullptr ? sink_error : queue_error_name(submit);
			return false;
		}
		next_audio_sample_ = range.end;
		++accepted_intervals_;
		if (frame_limit_ != 0u && accepted_intervals_ >= frame_limit_)
			stop_requested_ = true;
		last_end_ticks_ = end_ticks;
		return true;
	}

	std::string path_;
	bool displayed_;
	bool displayed_capture_owned_ = false;
	std::uint64_t capture_start_ticks_ = 0u;
	std::string segment_path_;
	std::string published_path_;
	bool auto_split_ = false;
	bool split_pending_ = false;
	bool awaiting_epoch_ = false;
	std::uint64_t completed_epoch_ticks_ = 0u;
	std::uint64_t segment_index_ = 1u;
	std::uint64_t segment_first_interval_ = 0u;
	std::uint64_t recording_start_ticks_ = 0u;
	std::uint64_t frame_limit_;
	std::uint32_t descriptor_generation_;
	std::uint32_t audio_epoch_;
	std::uint32_t audio_rate_;
	bool sound_started_;
	bool writer_started_;
	bool failed_;
	bool stop_requested_;
	bool manual_stop_requested_;
	bool audio_primed_;
	std::uint64_t accepted_intervals_;
	std::uint64_t movie_start_ticks_;
	std::uint64_t last_end_ticks_;
	std::uint64_t next_audio_sample_;
	std::uint64_t origin_offset_numerator_ = 0u;
	std::uint64_t origin_offset_denominator_ = 1u;
	std::string last_error_;
	VAEG_RECORDING_VIDEO_DESCRIPTOR video_descriptor_;
	VAEG_RECORDING_AUDIO_DESCRIPTOR audio_descriptor_;
	SampleGrid grid_;
	StagedCapture staged_;
	StagedCapture incoming_;
	RecordingSession session_;
};

std::unique_ptr<NativeRecordingSession> active_session;
std::string last_session_error;
std::string last_published_path;

bool parse_frame_limit(const char *value, std::uint64_t *limit) noexcept {
	char *end = nullptr;
	unsigned long long parsed;
	if (limit == nullptr) {
		return false;
	}
	*limit = 0u;
	if (value == nullptr || value[0] == '\0') {
		return true;
	}
	errno = 0;
	parsed = std::strtoull(value, &end, 10);
	if (errno != 0 || end == value || *end != '\0' || parsed == 0u) {
		return false;
	}
	*limit = static_cast<std::uint64_t>(parsed);
	return true;
}

} // namespace
} // namespace vaeg::recording

extern "C" int vaeg_recording_session_start(const char *path, uint32_t source,
                                             uint64_t frame_limit) {
	if (source != VAEG_RECORDING_SOURCE_NATIVE && source != VAEG_RECORDING_SOURCE_DISPLAYED) {
		vaeg::recording::last_session_error = "invalid_recording_source";
		return 0;
	}
	using vaeg::recording::active_session;
	using vaeg::recording::last_published_path;
	using vaeg::recording::last_session_error;
	using vaeg::recording::NativeRecordingSession;
	if (path == nullptr || path[0] == '\0' || active_session != nullptr) {
		last_session_error = (active_session != nullptr) ? "recording_busy" : "recording_path_empty";
		return 0;
	}
	try {
		std::unique_ptr<NativeRecordingSession> session(
		    new NativeRecordingSession(path, frame_limit, source == VAEG_RECORDING_SOURCE_DISPLAYED));
		if (!session->start()) {
			last_session_error = session->last_error();
			return 0;
		}
		active_session = std::move(session);
		last_published_path.clear();
		last_session_error.clear();
		return 1;
	} catch (const std::bad_alloc &) {
		last_session_error = "session_allocation_failed";
		return 0;
	}
}

extern "C" int vaeg_recording_native_session_start(const char *path, uint64_t frame_limit) {
	return vaeg_recording_session_start(path, VAEG_RECORDING_SOURCE_NATIVE, frame_limit);
}
extern "C" int vaeg_recording_session_is_displayed(void) {
	return vaeg::recording::active_session && vaeg::recording::active_session->is_displayed();
}
extern "C" int vaeg_recording_session_displayed_capture_changed(void) {
	return vaeg::recording::active_session &&
	       vaeg::recording::active_session->displayed_capture_changed();
}
extern "C" void vaeg_recording_native_session_enable_auto_split(void) {
	if (vaeg::recording::active_session)
		vaeg::recording::active_session->enable_auto_split();
}
extern "C" int vaeg_recording_native_session_auto_split_enabled(void) {
	return vaeg::recording::active_session && vaeg::recording::active_session->auto_split_enabled();
}
extern "C" void vaeg_recording_native_session_request_split(void) {
	if (vaeg::recording::active_session)
		vaeg::recording::active_session->request_split();
}
extern "C" const char *vaeg_recording_native_session_output_path(void) {
	return vaeg::recording::active_session ? vaeg::recording::active_session->output_path().c_str()
	                                       : vaeg::recording::last_published_path.c_str();
}
extern "C" VAEG_RECORDING_NATIVE_TAP_ERROR
vaeg_recording_native_session_submit_frame(uint64_t sequence, uint64_t ticks, uint32_t generation,
                                           const VAEG_RECORDING_NATIVE_FRAME *frame,
                                           const VAEG_RECORDING_OSD_SNAPSHOT *osd) {
	using namespace vaeg::recording;
	if (!active_session)
		return vaeg_recording_native_tap_submit_frame(sequence, ticks, generation, frame, osd);
	VAEG_RECORDING_NATIVE_TAP_ERROR error;
	try {
		error = active_session->submit_frame(sequence, ticks, generation, frame, osd);
		if (error == VAEG_RECORDING_NATIVE_TAP_OK || !active_session->auto_split_enabled()) {
			return error;
		}
		last_session_error = active_session->last_error();
		if (last_session_error.empty())
			last_session_error = vaeg_recording_native_tap_error_name(error);
	} catch (const std::bad_alloc &) {
		last_session_error = "segment_allocation_failed";
		error = VAEG_RECORDING_NATIVE_TAP_ERROR_CALLBACK_REJECTED;
	}
	last_published_path = active_session->published_path();
	active_session->abort("segment_failed");
	active_session.reset();
	return error;
}

extern "C" int vaeg_recording_native_session_start_from_environment(void) {
	const char *path = std::getenv("VAEG_RECORDING_NATIVE_PATH");
	std::uint64_t frame_limit = 0u;
	if (path == nullptr || path[0] == '\0') {
		return 1;
	}
	if (!vaeg::recording::parse_frame_limit(std::getenv("VAEG_RECORDING_NATIVE_FRAMES"),
	                                        &frame_limit)) {
		vaeg::recording::last_session_error = "invalid_recording_frame_limit";
		return 0;
	}
	return vaeg_recording_native_session_start(path, frame_limit);
}

extern "C" int vaeg_recording_native_session_after_guest_exec(void) {
	if (vaeg::recording::active_session == nullptr) {
		return 1;
	}
	if (!vaeg::recording::active_session->after_guest_exec()) {
		vaeg::recording::last_session_error =
		    vaeg::recording::active_session->last_error();
		vaeg::recording::last_published_path = vaeg::recording::active_session->published_path();
		vaeg::recording::active_session->abort("recording_interval_failed");
		vaeg::recording::active_session.reset();
		return 0;
	}
	return 1;
}

extern "C" int vaeg_recording_native_session_is_active(void) {
	return vaeg::recording::active_session != nullptr ? 1 : 0;
}

extern "C" int vaeg_recording_native_session_should_stop(void) {
	return (vaeg::recording::active_session != nullptr &&
	        vaeg::recording::active_session->should_stop())
	           ? 1
	           : 0;
}

extern "C" int vaeg_recording_native_session_request_stop(void) {
	if (vaeg::recording::active_session == nullptr) {
		vaeg::recording::last_session_error = "recording_not_active";
		return 0;
	}
	if (!vaeg::recording::active_session->request_stop()) {
		vaeg::recording::last_session_error =
		    vaeg::recording::active_session->last_error();
		return 0;
	}
	return 1;
}

extern "C" int vaeg_recording_native_session_poll_stop(void) {
	if (vaeg::recording::active_session == nullptr ||
	    !vaeg::recording::active_session->manual_stop_requested()) {
		return 1;
	}
	const bool result = vaeg::recording::active_session->stop();
	vaeg::recording::last_session_error = vaeg::recording::active_session->last_error();
	vaeg::recording::last_published_path = vaeg::recording::active_session->published_path();
	vaeg::recording::active_session.reset();
	return result ? 1 : 0;
}

extern "C" int vaeg_recording_native_session_is_finalizing(void) {
	return (vaeg::recording::active_session != nullptr &&
	        vaeg::recording::active_session->manual_stop_requested())
	           ? 1
	           : 0;
}

extern "C" uint64_t vaeg_recording_native_session_elapsed_ticks(void) {
	return vaeg::recording::active_session != nullptr
	           ? vaeg::recording::active_session->elapsed_ticks()
	           : 0u;
}

extern "C" uint64_t vaeg_recording_native_session_accepted_frames(void) {
	return vaeg::recording::active_session != nullptr
	           ? vaeg::recording::active_session->accepted_intervals()
	           : 0u;
}

extern "C" int vaeg_recording_native_session_stop(void) {
	if (vaeg::recording::active_session == nullptr) {
		return 1;
	}
	const bool result = vaeg::recording::active_session->stop();
	vaeg::recording::last_session_error = vaeg::recording::active_session->last_error();
	vaeg::recording::last_published_path = vaeg::recording::active_session->published_path();
	vaeg::recording::active_session.reset();
	return result ? 1 : 0;
}

extern "C" void vaeg_recording_native_session_cancel(void) {
	if (vaeg::recording::active_session != nullptr) {
		vaeg::recording::active_session->abort("recording_canceled");
		vaeg::recording::last_session_error =
		    vaeg::recording::active_session->last_error();
		vaeg::recording::active_session.reset();
	}
}

extern "C" void vaeg_recording_native_session_before_reset(void) {
	using namespace vaeg::recording;
	if (active_session && active_session->is_displayed()) {
		/* Reset changes the audio/guest epoch. Seal before either is replaced. */
		(void)vaeg_recording_native_session_stop();
		return;
	}
	if (active_session && active_session->auto_split_enabled()) {
		try {
			if (active_session->change_epoch())
				return;
			last_session_error = active_session->last_error();
		} catch (const std::bad_alloc &) {
			last_session_error = "segment_epoch_allocation_failed";
		}
		last_published_path = active_session->published_path();
		active_session->abort("segment_epoch_finished");
		active_session.reset();
		return;
	}
	vaeg_recording_native_session_epoch_change();
}

extern "C" void vaeg_recording_native_session_epoch_change(void) {
	if (vaeg::recording::active_session != nullptr) {
		vaeg::recording::active_session->abort("recording_epoch_changed");
		vaeg::recording::last_session_error =
		    vaeg::recording::active_session->last_error();
		vaeg::recording::active_session.reset();
	}
}

extern "C" const char *vaeg_recording_native_session_last_error(void) {
	return vaeg::recording::last_session_error.empty()
	           ? "none"
	           : vaeg::recording::last_session_error.c_str();
}

#endif
