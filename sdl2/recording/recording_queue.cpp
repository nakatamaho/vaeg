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
#include "recording/recording_queue.hpp"

#include <algorithm>
#include <cstring>
#include <limits>
#include <new>

namespace vaeg::recording {
namespace {

bool checked_add(std::uint64_t left, std::uint64_t right, std::uint64_t *result) noexcept {
	if (result == nullptr || left > std::numeric_limits<std::uint64_t>::max() - right) {
		return false;
	}
	*result = left + right;
	return true;
}

bool checked_mul(std::uint64_t left, std::uint64_t right, std::uint64_t *result) noexcept {
	if (result == nullptr ||
	    (right != 0u && left > std::numeric_limits<std::uint64_t>::max() / right)) {
		return false;
	}
	*result = left * right;
	return true;
}

bool video_descriptor_equal(const VAEG_RECORDING_VIDEO_DESCRIPTOR &left,
                            const VAEG_RECORDING_VIDEO_DESCRIPTOR &right) noexcept {
	return left.width == right.width && left.height == right.height &&
	       left.stride_bytes == right.stride_bytes && left.pixel_format == right.pixel_format;
}

bool audio_descriptor_equal(const VAEG_RECORDING_AUDIO_DESCRIPTOR &left,
                            const VAEG_RECORDING_AUDIO_DESCRIPTOR &right) noexcept {
	return left.sample_rate_hz == right.sample_rate_hz && left.channels == right.channels &&
	       left.sample_bits == right.sample_bits && left.format == right.format;
}

} // namespace

const char *queue_error_name(QueueError error) noexcept {
	switch (error) {
	case QueueError::None:
		return "none";
	case QueueError::InvalidConfiguration:
		return "invalid_configuration";
	case QueueError::InvalidSlice:
		return "invalid_slice";
	case QueueError::AllocationFailure:
		return "allocation_failure";
	case QueueError::WouldBlock:
		return "would_block";
	case QueueError::Closed:
		return "closed";
	case QueueError::Canceled:
		return "canceled";
	case QueueError::Failed:
		return "failed";
	case QueueError::SequenceGap:
		return "sequence_gap";
	case QueueError::DescriptorMismatch:
		return "descriptor_mismatch";
	case QueueError::IntervalGap:
		return "interval_gap";
	case QueueError::AudioGap:
		return "audio_gap";
	case QueueError::SinkFailure:
		return "sink_failure";
	case QueueError::InvalidState:
		return "invalid_state";
	case QueueError::NullSink:
		return "null_sink";
	default:
		return "unknown";
	}
}

CaptureSlice::CaptureSlice(const CaptureSliceInput &input, std::uint64_t resident_bytes,
                           std::vector<std::uint8_t> video_bytes,
                           std::vector<std::int16_t> audio_samples) noexcept
    : sequence_(input.sequence), begin_ticks_(input.begin_ticks), end_ticks_(input.end_ticks),
      audio_sample_begin_(input.audio_sample_begin),
      descriptor_generation_(input.descriptor_generation), video_descriptor_(input.video),
      audio_descriptor_(input.audio), video_bytes_(std::move(video_bytes)),
      audio_samples_(std::move(audio_samples)), resident_bytes_(resident_bytes) {
}

CaptureSliceResult CaptureSlice::create(const CaptureSliceInput &input) noexcept {
	std::uint64_t video_storage_bytes;
	std::uint64_t audio_scalar_count;
	std::uint64_t audio_bytes;
	std::uint64_t resident_bytes;
	std::vector<std::uint8_t> video_bytes;
	std::vector<std::int16_t> audio_samples;

	if (vaeg_recording_validate_video_descriptor(&input.video) != VAEG_RECORDING_ERROR_OK ||
	    vaeg_recording_validate_audio_descriptor(&input.audio) != VAEG_RECORDING_ERROR_OK ||
	    input.end_ticks <= input.begin_ticks) {
		return {QueueError::InvalidSlice, nullptr};
	}
	if (!checked_mul(input.video.stride_bytes, input.video.height, &video_storage_bytes) ||
	    video_storage_bytes > input.video_bytes ||
	    !checked_mul(input.audio_frame_count, input.audio.channels, &audio_scalar_count) ||
	    !checked_mul(audio_scalar_count, input.audio.sample_bits / 8u, &audio_bytes) ||
	    audio_bytes > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()) ||
	    video_storage_bytes > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max()) ||
	    !checked_add(video_storage_bytes, audio_bytes, &resident_bytes)) {
		return {QueueError::InvalidSlice, nullptr};
	}
	if ((input.video_pixels == nullptr && video_storage_bytes != 0u) ||
	    (input.audio_s16 == nullptr && audio_scalar_count != 0u)) {
		return {QueueError::InvalidSlice, nullptr};
	}
	try {
		video_bytes.resize(static_cast<std::size_t>(video_storage_bytes));
		if (video_storage_bytes != 0u) {
			std::memcpy(video_bytes.data(), input.video_pixels,
			            static_cast<std::size_t>(video_storage_bytes));
		}
		audio_samples.resize(static_cast<std::size_t>(audio_scalar_count));
		if (audio_scalar_count != 0u) {
			std::memcpy(audio_samples.data(), input.audio_s16,
			            static_cast<std::size_t>(audio_bytes));
		}
		return {QueueError::None,
		        std::unique_ptr<CaptureSlice>(new CaptureSlice(
		            input, resident_bytes, std::move(video_bytes), std::move(audio_samples)))};
	} catch (const std::bad_alloc &) {
		return {QueueError::AllocationFailure, nullptr};
	}
}

std::uint64_t CaptureSlice::sequence() const noexcept {
	return sequence_;
}

std::uint64_t CaptureSlice::begin_ticks() const noexcept {
	return begin_ticks_;
}

std::uint64_t CaptureSlice::end_ticks() const noexcept {
	return end_ticks_;
}

std::uint64_t CaptureSlice::audio_sample_begin() const noexcept {
	return audio_sample_begin_;
}

std::uint64_t CaptureSlice::audio_frame_count() const noexcept {
	return static_cast<std::uint64_t>(audio_samples_.size() / 2u);
}

std::uint64_t CaptureSlice::descriptor_generation() const noexcept {
	return descriptor_generation_;
}

const VAEG_RECORDING_VIDEO_DESCRIPTOR &CaptureSlice::video_descriptor() const noexcept {
	return video_descriptor_;
}

const VAEG_RECORDING_AUDIO_DESCRIPTOR &CaptureSlice::audio_descriptor() const noexcept {
	return audio_descriptor_;
}

const std::vector<std::uint8_t> &CaptureSlice::video_bytes() const noexcept {
	return video_bytes_;
}

const std::vector<std::int16_t> &CaptureSlice::audio_samples() const noexcept {
	return audio_samples_;
}

std::uint64_t CaptureSlice::resident_bytes() const noexcept {
	return resident_bytes_;
}

QueueCreateResult BoundedSliceQueue::create(const QueueConfig &config) noexcept {
	std::uint64_t reserved_bytes;
	if (config.budget.max_slices == 0u || config.budget.max_bytes == 0u ||
	    vaeg_recording_validate_video_descriptor(&config.video) != VAEG_RECORDING_ERROR_OK ||
	    vaeg_recording_validate_audio_descriptor(&config.audio) != VAEG_RECORDING_ERROR_OK ||
	    !checked_add(config.budget.active_interval_bytes, config.budget.readback_staging_bytes,
	                 &reserved_bytes) ||
	    reserved_bytes >= config.budget.max_bytes) {
		return {QueueError::InvalidConfiguration, nullptr};
	}
	try {
		return {QueueError::None,
		        std::unique_ptr<BoundedSliceQueue>(new BoundedSliceQueue(config))};
	} catch (const std::bad_alloc &) {
		return {QueueError::AllocationFailure, nullptr};
	}
}

BoundedSliceQueue::BoundedSliceQueue(const QueueConfig &config) noexcept
    : config_(config),
      reserved_bytes_(config.budget.active_interval_bytes + config.budget.readback_staging_bytes),
      queued_bytes_(0u), high_water_bytes_(0u), admitted_slices_(0u), popped_slices_(0u),
      next_sequence_(0u), next_begin_ticks_(config.first_begin_ticks),
      next_audio_sample_(config.first_audio_sample), producer_closed_(false), canceled_(false),
      failure_(QueueError::None) {
}

BoundedSliceQueue::~BoundedSliceQueue() {
	cancel();
}

bool BoundedSliceQueue::descriptor_matches(const CaptureSlice &slice) const noexcept {
	return slice.descriptor_generation() == config_.descriptor_generation &&
	       video_descriptor_equal(slice.video_descriptor(), config_.video) &&
	       audio_descriptor_equal(slice.audio_descriptor(), config_.audio);
}

QueueError BoundedSliceQueue::validate_admission(const CaptureSlice &slice) const noexcept {
	if (slice.sequence() != next_sequence_) {
		return QueueError::SequenceGap;
	}
	if (!descriptor_matches(slice)) {
		return QueueError::DescriptorMismatch;
	}
	if (slice.begin_ticks() != next_begin_ticks_ || slice.end_ticks() <= slice.begin_ticks()) {
		return QueueError::IntervalGap;
	}
	if (slice.audio_sample_begin() != next_audio_sample_) {
		return QueueError::AudioGap;
	}
	return QueueError::None;
}

QueueError BoundedSliceQueue::try_push(std::unique_ptr<CaptureSlice> &slice) noexcept {
	std::uint64_t new_bytes;
	QueueError validation;
	if (slice == nullptr) {
		return QueueError::InvalidSlice;
	}
	std::unique_lock<std::mutex> lock(mutex_);
	if (failure_ != QueueError::None) {
		return QueueError::Failed;
	}
	if (canceled_) {
		return QueueError::Canceled;
	}
	if (producer_closed_) {
		return QueueError::Closed;
	}
	validation = validate_admission(*slice);
	if (validation != QueueError::None) {
		return validation;
	}
	if (queue_.size() >= config_.budget.max_slices ||
	    !checked_add(queued_bytes_, slice->resident_bytes(), &new_bytes) ||
	    new_bytes > config_.budget.max_bytes - reserved_bytes_) {
		return QueueError::WouldBlock;
	}
	std::uint64_t next_audio_sample;
	if (!checked_add(slice->audio_sample_begin(), slice->audio_frame_count(), &next_audio_sample)) {
		return QueueError::InvalidSlice;
	}
	queued_bytes_ = new_bytes;
	high_water_bytes_ = std::max(high_water_bytes_, queued_bytes_);
	++admitted_slices_;
	++next_sequence_;
	next_begin_ticks_ = slice->end_ticks();
	next_audio_sample_ = next_audio_sample;
	queue_.push_back(std::move(slice));
	lock.unlock();
	data_available_.notify_one();
	return QueueError::None;
}

QueueError BoundedSliceQueue::wait_pop(std::unique_ptr<CaptureSlice> *slice) noexcept {
	if (slice == nullptr) {
		return QueueError::InvalidSlice;
	}
	*slice = nullptr;
	std::unique_lock<std::mutex> lock(mutex_);
	data_available_.wait(lock, [this] {
		return !queue_.empty() || producer_closed_ || canceled_ || failure_ != QueueError::None;
	});
	if (failure_ != QueueError::None) {
		return QueueError::Failed;
	}
	if (canceled_) {
		return QueueError::Canceled;
	}
	if (!queue_.empty()) {
		*slice = std::move(queue_.front());
		queue_.pop_front();
		queued_bytes_ -= (*slice)->resident_bytes();
		++popped_slices_;
		lock.unlock();
		space_available_.notify_all();
		return QueueError::None;
	}
	return QueueError::Closed;
}

void BoundedSliceQueue::close_producer() noexcept {
	{
		std::lock_guard<std::mutex> lock(mutex_);
		producer_closed_ = true;
	}
	data_available_.notify_all();
	space_available_.notify_all();
}

void BoundedSliceQueue::cancel() noexcept {
	{
		std::lock_guard<std::mutex> lock(mutex_);
		canceled_ = true;
		queue_.clear();
		queued_bytes_ = 0u;
	}
	data_available_.notify_all();
	space_available_.notify_all();
}

void BoundedSliceQueue::fail(QueueError error) noexcept {
	if (error == QueueError::None || error == QueueError::Failed) {
		error = QueueError::SinkFailure;
	}
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (failure_ == QueueError::None) {
			failure_ = error;
		}
	}
	data_available_.notify_all();
	space_available_.notify_all();
}

QueueBudget BoundedSliceQueue::budget() const noexcept {
	return config_.budget;
}

std::uint64_t BoundedSliceQueue::reserved_bytes() const noexcept {
	return reserved_bytes_;
}

std::uint64_t BoundedSliceQueue::queued_bytes() const noexcept {
	std::lock_guard<std::mutex> lock(mutex_);
	return queued_bytes_;
}

std::uint64_t BoundedSliceQueue::queued_slices() const noexcept {
	std::lock_guard<std::mutex> lock(mutex_);
	return static_cast<std::uint64_t>(queue_.size());
}

std::uint64_t BoundedSliceQueue::high_water_bytes() const noexcept {
	std::lock_guard<std::mutex> lock(mutex_);
	return high_water_bytes_;
}

std::uint64_t BoundedSliceQueue::admitted_slices() const noexcept {
	std::lock_guard<std::mutex> lock(mutex_);
	return admitted_slices_;
}

std::uint64_t BoundedSliceQueue::popped_slices() const noexcept {
	std::lock_guard<std::mutex> lock(mutex_);
	return popped_slices_;
}

SliceWorker::SliceWorker(std::unique_ptr<SliceSink> sink) noexcept
    : sink_(std::move(sink)), queue_(nullptr), result_(QueueError::None), written_slices_(0u) {
}

SliceWorker::~SliceWorker() {
	if (thread_.joinable()) {
		if (queue_ != nullptr) {
			queue_->cancel();
		}
		thread_.join();
	}
}

QueueError SliceWorker::start(BoundedSliceQueue *queue) noexcept {
	if (queue == nullptr || sink_ == nullptr || thread_.joinable()) {
		return (sink_ == nullptr) ? QueueError::NullSink : QueueError::InvalidState;
	}
	queue_ = queue;
	try {
		thread_ = std::thread(&SliceWorker::run, this);
		return QueueError::None;
	} catch (...) {
		queue_ = nullptr;
		return QueueError::AllocationFailure;
	}
}

void SliceWorker::join() noexcept {
	if (thread_.joinable()) {
		thread_.join();
	}
}

bool SliceWorker::joinable() const noexcept {
	return thread_.joinable();
}

QueueError SliceWorker::result() const noexcept {
	std::lock_guard<std::mutex> lock(mutex_);
	return result_;
}

std::uint64_t SliceWorker::written_slices() const noexcept {
	std::lock_guard<std::mutex> lock(mutex_);
	return written_slices_;
}

void SliceWorker::run() noexcept {
	for (;;) {
		std::unique_ptr<CaptureSlice> slice;
		const QueueError pop_result = queue_->wait_pop(&slice);
		if (pop_result == QueueError::Closed || pop_result == QueueError::Canceled ||
		    pop_result == QueueError::Failed) {
			std::lock_guard<std::mutex> lock(mutex_);
			result_ = (pop_result == QueueError::Closed) ? QueueError::None : pop_result;
			return;
		}
		if (pop_result != QueueError::None || slice == nullptr) {
			std::lock_guard<std::mutex> lock(mutex_);
			result_ = QueueError::Failed;
			queue_->fail(QueueError::InvalidSlice);
			return;
		}
		const QueueError sink_result = sink_->write(*slice);
		if (sink_result != QueueError::None) {
			std::lock_guard<std::mutex> lock(mutex_);
			result_ = QueueError::SinkFailure;
			queue_->fail(QueueError::SinkFailure);
			return;
		}
		{
			std::lock_guard<std::mutex> lock(mutex_);
			++written_slices_;
		}
	}
}

RecordingSession::RecordingSession() noexcept
    : state_(SessionState::Idle), last_error_(QueueError::None), config_{}, queue_(nullptr),
      worker_(nullptr), accepted_slices_(0u), written_slices_(0u) {
}

RecordingSession::~RecordingSession() {
	cancel();
}

QueueError RecordingSession::prepare(const QueueConfig &config) noexcept {
	std::lock_guard<std::mutex> lock(mutex_);
	if (state_ != SessionState::Idle) {
		return QueueError::InvalidState;
	}
	const QueueCreateResult validation = BoundedSliceQueue::create(config);
	if (!validation) {
		return validation.error;
	}
	config_ = config;
	accepted_slices_ = 0u;
	written_slices_ = 0u;
	last_error_ = QueueError::None;
	state_ = SessionState::Preparing;
	return QueueError::None;
}

QueueError RecordingSession::arm(std::unique_ptr<SliceSink> sink) noexcept {
	std::unique_lock<std::mutex> lock(mutex_);
	if (state_ != SessionState::Preparing) {
		return QueueError::InvalidState;
	}
	if (sink == nullptr) {
		return QueueError::NullSink;
	}
	QueueCreateResult created = BoundedSliceQueue::create(config_);
	if (!created) {
		last_error_ = created.error;
		state_ = SessionState::Failed;
		return created.error;
	}
	std::unique_ptr<SliceWorker> worker;
	try {
		worker.reset(new SliceWorker(std::move(sink)));
	} catch (const std::bad_alloc &) {
		last_error_ = QueueError::AllocationFailure;
		state_ = SessionState::Failed;
		return QueueError::AllocationFailure;
	}
	queue_ = std::move(created.queue);
	const QueueError start_result = worker->start(queue_.get());
	if (start_result != QueueError::None) {
		last_error_ = start_result;
		queue_.reset();
		state_ = SessionState::Failed;
		return start_result;
	}
	worker_ = std::move(worker);
	state_ = SessionState::Armed;
	return QueueError::None;
}

QueueError RecordingSession::begin() noexcept {
	std::lock_guard<std::mutex> lock(mutex_);
	if (state_ != SessionState::Armed) {
		return QueueError::InvalidState;
	}
	state_ = SessionState::Recording;
	return QueueError::None;
}

void RecordingSession::refresh_worker_failure() noexcept {
	if (worker_ != nullptr && worker_->result() != QueueError::None &&
	    state_ == SessionState::Recording) {
		last_error_ = worker_->result();
		state_ = SessionState::Failed;
	}
}

QueueError RecordingSession::try_submit(std::unique_ptr<CaptureSlice> &slice) noexcept {
	std::unique_lock<std::mutex> lock(mutex_);
	refresh_worker_failure();
	if (state_ != SessionState::Recording || queue_ == nullptr) {
		return (state_ == SessionState::Failed) ? QueueError::Failed : QueueError::InvalidState;
	}
	const QueueError result = queue_->try_push(slice);
	if (result == QueueError::None) {
		++accepted_slices_;
	} else if (result == QueueError::Failed) {
		last_error_ = worker_ != nullptr ? worker_->result() : QueueError::Failed;
		state_ = SessionState::Failed;
	}
	return result;
}

void RecordingSession::finish_worker(QueueError terminal_error) noexcept {
	if (queue_ != nullptr) {
		if (terminal_error == QueueError::Canceled) {
			queue_->cancel();
		} else {
			queue_->close_producer();
		}
	}
	if (worker_ != nullptr) {
		worker_->join();
	}
	const QueueError worker_error = worker_ != nullptr ? worker_->result() : QueueError::None;
	const std::uint64_t written_slices = worker_ != nullptr ? worker_->written_slices() : 0u;
	std::lock_guard<std::mutex> lock(mutex_);
	written_slices_ = written_slices;
	if (terminal_error == QueueError::Canceled || worker_error == QueueError::Canceled) {
		state_ = SessionState::Canceled;
		last_error_ = QueueError::Canceled;
	} else if (worker_error != QueueError::None) {
		state_ = SessionState::Failed;
		last_error_ = worker_error;
	} else {
		state_ = SessionState::Idle;
		last_error_ = QueueError::None;
		queue_.reset();
		worker_.reset();
	}
}

QueueError RecordingSession::stop() noexcept {
	{
		std::unique_lock<std::mutex> lock(mutex_);
		if (state_ == SessionState::Idle || state_ == SessionState::Canceled ||
		    state_ == SessionState::Failed) {
			return QueueError::None;
		}
		if (state_ == SessionState::Preparing) {
			state_ = SessionState::Idle;
			return QueueError::None;
		}
		if (state_ == SessionState::Stopping || state_ == SessionState::Finalizing) {
			return QueueError::None;
		}
		state_ = SessionState::Finalizing;
	}
	finish_worker(QueueError::None);
	return (state() == SessionState::Failed) ? QueueError::Failed : QueueError::None;
}

QueueError RecordingSession::cancel() noexcept {
	{
		std::unique_lock<std::mutex> lock(mutex_);
		if (state_ == SessionState::Idle || state_ == SessionState::Canceled ||
		    state_ == SessionState::Failed) {
			return QueueError::None;
		}
		if (state_ == SessionState::Preparing) {
			state_ = SessionState::Canceled;
			last_error_ = QueueError::Canceled;
			return QueueError::None;
		}
		if (state_ == SessionState::Stopping || state_ == SessionState::Finalizing) {
			return QueueError::None;
		}
		state_ = SessionState::Stopping;
	}
	finish_worker(QueueError::Canceled);
	return QueueError::None;
}

QueueError RecordingSession::acknowledge_terminal() noexcept {
	std::lock_guard<std::mutex> lock(mutex_);
	if (state_ != SessionState::Canceled && state_ != SessionState::Failed) {
		return QueueError::InvalidState;
	}
	if (queue_ != nullptr) {
		queue_->cancel();
	}
	if (worker_ != nullptr) {
		worker_->join();
	}
	queue_.reset();
	worker_.reset();
	last_error_ = QueueError::None;
	state_ = SessionState::Idle;
	return QueueError::None;
}

SessionState RecordingSession::state() const noexcept {
	std::lock_guard<std::mutex> lock(mutex_);
	return state_;
}

QueueError RecordingSession::last_error() const noexcept {
	std::lock_guard<std::mutex> lock(mutex_);
	return last_error_;
}

std::uint64_t RecordingSession::accepted_slices() const noexcept {
	std::lock_guard<std::mutex> lock(mutex_);
	return accepted_slices_;
}

std::uint64_t RecordingSession::written_slices() const noexcept {
	std::lock_guard<std::mutex> lock(mutex_);
	return worker_ != nullptr ? worker_->written_slices() : written_slices_;
}

} // namespace vaeg::recording
