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
#ifndef VAEG_SDL2_RECORDING_QUEUE_HPP
#define VAEG_SDL2_RECORDING_QUEUE_HPP

#include <condition_variable>
#include <cstdint>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "recording/recording.h"

namespace vaeg::recording {

enum class QueueError {
	None = 0,
	InvalidConfiguration,
	InvalidSlice,
	AllocationFailure,
	WouldBlock,
	Closed,
	Canceled,
	Failed,
	SequenceGap,
	DescriptorMismatch,
	IntervalGap,
	AudioGap,
	SinkFailure,
	InvalidState,
	NullSink,
};

const char *queue_error_name(QueueError error) noexcept;

struct QueueBudget {
	std::uint64_t max_slices;
	std::uint64_t max_bytes;
	std::uint64_t active_interval_bytes;
	std::uint64_t readback_staging_bytes;
};

struct QueueConfig {
	QueueBudget budget;
	VAEG_RECORDING_VIDEO_DESCRIPTOR video;
	VAEG_RECORDING_AUDIO_DESCRIPTOR audio;
	std::uint64_t descriptor_generation;
	std::uint64_t first_begin_ticks;
	std::uint64_t first_audio_sample;
};

struct CaptureSliceInput {
	std::uint64_t sequence;
	std::uint64_t begin_ticks;
	std::uint64_t end_ticks;
	std::uint64_t audio_sample_begin;
	std::uint64_t audio_frame_count;
	std::uint64_t descriptor_generation;
	VAEG_RECORDING_VIDEO_DESCRIPTOR video;
	const void *video_pixels;
	std::uint64_t video_bytes;
	VAEG_RECORDING_AUDIO_DESCRIPTOR audio;
	const std::int16_t *audio_s16;
};

class CaptureSlice;

struct CaptureSliceResult {
	QueueError error;
	std::unique_ptr<CaptureSlice> slice;

	explicit operator bool() const noexcept {
		return error == QueueError::None && slice != nullptr;
	}
};

class CaptureSlice {
  public:
	static CaptureSliceResult create(const CaptureSliceInput &input) noexcept;

	std::uint64_t sequence() const noexcept;
	std::uint64_t begin_ticks() const noexcept;
	std::uint64_t end_ticks() const noexcept;
	std::uint64_t audio_sample_begin() const noexcept;
	std::uint64_t audio_frame_count() const noexcept;
	std::uint64_t descriptor_generation() const noexcept;
	const VAEG_RECORDING_VIDEO_DESCRIPTOR &video_descriptor() const noexcept;
	const VAEG_RECORDING_AUDIO_DESCRIPTOR &audio_descriptor() const noexcept;
	const std::vector<std::uint8_t> &video_bytes() const noexcept;
	const std::vector<std::int16_t> &audio_samples() const noexcept;
	std::uint64_t resident_bytes() const noexcept;

  private:
	CaptureSlice(const CaptureSliceInput &input, std::uint64_t resident_bytes,
	             std::vector<std::uint8_t> video_bytes,
	             std::vector<std::int16_t> audio_samples) noexcept;

	std::uint64_t sequence_;
	std::uint64_t begin_ticks_;
	std::uint64_t end_ticks_;
	std::uint64_t audio_sample_begin_;
	std::uint64_t descriptor_generation_;
	VAEG_RECORDING_VIDEO_DESCRIPTOR video_descriptor_;
	VAEG_RECORDING_AUDIO_DESCRIPTOR audio_descriptor_;
	std::vector<std::uint8_t> video_bytes_;
	std::vector<std::int16_t> audio_samples_;
	std::uint64_t resident_bytes_;
};

struct QueueCreateResult;

class BoundedSliceQueue {
  public:
	static QueueCreateResult create(const QueueConfig &config) noexcept;
	~BoundedSliceQueue();

	BoundedSliceQueue(const BoundedSliceQueue &) = delete;
	BoundedSliceQueue &operator=(const BoundedSliceQueue &) = delete;

	QueueError try_push(std::unique_ptr<CaptureSlice> &slice) noexcept;
	QueueError wait_pop(std::unique_ptr<CaptureSlice> *slice) noexcept;
	void close_producer() noexcept;
	void cancel() noexcept;
	void fail(QueueError error) noexcept;

	QueueBudget budget() const noexcept;
	std::uint64_t reserved_bytes() const noexcept;
	std::uint64_t queued_bytes() const noexcept;
	std::uint64_t queued_slices() const noexcept;
	std::uint64_t high_water_bytes() const noexcept;
	std::uint64_t admitted_slices() const noexcept;
	std::uint64_t popped_slices() const noexcept;

  private:
	explicit BoundedSliceQueue(const QueueConfig &config) noexcept;

	bool descriptor_matches(const CaptureSlice &slice) const noexcept;
	QueueError validate_admission(const CaptureSlice &slice) const noexcept;

	const QueueConfig config_;
	const std::uint64_t reserved_bytes_;
	std::deque<std::unique_ptr<CaptureSlice>> queue_;
	mutable std::mutex mutex_;
	std::condition_variable data_available_;
	std::condition_variable space_available_;
	std::uint64_t queued_bytes_;
	std::uint64_t high_water_bytes_;
	std::uint64_t admitted_slices_;
	std::uint64_t popped_slices_;
	std::uint64_t next_sequence_;
	std::uint64_t next_begin_ticks_;
	std::uint64_t next_audio_sample_;
	bool producer_closed_;
	bool canceled_;
	QueueError failure_;
};

struct QueueCreateResult {
	QueueError error;
	std::unique_ptr<BoundedSliceQueue> queue;

	explicit operator bool() const noexcept {
		return error == QueueError::None && queue != nullptr;
	}
};

class SliceSink {
  public:
	virtual ~SliceSink() = default;
	virtual QueueError write(const CaptureSlice &slice) noexcept = 0;
};

class SliceWorker {
  public:
	explicit SliceWorker(std::unique_ptr<SliceSink> sink) noexcept;
	~SliceWorker();

	SliceWorker(const SliceWorker &) = delete;
	SliceWorker &operator=(const SliceWorker &) = delete;

	QueueError start(BoundedSliceQueue *queue) noexcept;
	void join() noexcept;
	bool joinable() const noexcept;
	QueueError result() const noexcept;
	std::uint64_t written_slices() const noexcept;

  private:
	void run() noexcept;

	std::unique_ptr<SliceSink> sink_;
	BoundedSliceQueue *queue_;
	std::thread thread_;
	mutable std::mutex mutex_;
	QueueError result_;
	std::uint64_t written_slices_;
};

enum class SessionState {
	Idle = 0,
	Preparing,
	Armed,
	Recording,
	Stopping,
	Finalizing,
	Canceled,
	Failed,
};

class RecordingSession {
  public:
	RecordingSession() noexcept;
	~RecordingSession();

	RecordingSession(const RecordingSession &) = delete;
	RecordingSession &operator=(const RecordingSession &) = delete;

	QueueError prepare(const QueueConfig &config) noexcept;
	QueueError arm(std::unique_ptr<SliceSink> sink) noexcept;
	QueueError begin() noexcept;
	QueueError try_submit(std::unique_ptr<CaptureSlice> &slice) noexcept;
	QueueError stop() noexcept;
	QueueError cancel() noexcept;
	QueueError acknowledge_terminal() noexcept;

	SessionState state() const noexcept;
	QueueError last_error() const noexcept;
	std::uint64_t accepted_slices() const noexcept;
	std::uint64_t written_slices() const noexcept;

  private:
	void finish_worker(QueueError terminal_error) noexcept;
	void refresh_worker_failure() noexcept;

	mutable std::mutex mutex_;
	SessionState state_;
	QueueError last_error_;
	QueueConfig config_;
	std::unique_ptr<BoundedSliceQueue> queue_;
	std::unique_ptr<SliceWorker> worker_;
	std::uint64_t accepted_slices_;
	std::uint64_t written_slices_;
};

} // namespace vaeg::recording

#endif
