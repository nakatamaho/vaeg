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
#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "recording/recording_queue.hpp"

using vaeg::recording::BoundedSliceQueue;
using vaeg::recording::CaptureSlice;
using vaeg::recording::CaptureSliceInput;
using vaeg::recording::CaptureSliceResult;
using vaeg::recording::QueueConfig;
using vaeg::recording::QueueError;
using vaeg::recording::RecordingSession;
using vaeg::recording::SessionState;
using vaeg::recording::SliceSink;

static QueueConfig make_config(std::uint64_t max_slices = 4u, std::uint64_t max_bytes = 4096u) {
	QueueConfig config{};
	config.budget = {max_slices, max_bytes, 32u, 32u};
	config.video = {2u, 2u, 4u, VAEG_RECORDING_PIXEL_RGB565};
	config.audio = {48000u, 2u, 16u, VAEG_RECORDING_AUDIO_PCM_S16LE};
	config.descriptor_generation = 11u;
	config.first_begin_ticks = 0u;
	config.first_audio_sample = 0u;
	return config;
}

static CaptureSliceResult make_slice(const QueueConfig &config, std::uint64_t sequence,
                                     std::uint64_t begin_ticks, std::uint64_t end_ticks,
                                     std::uint64_t audio_sample_begin,
                                     std::uint64_t audio_frame_count = 1u) {
	std::vector<std::uint8_t> pixels(8u, static_cast<std::uint8_t>(sequence + 0x20u));
	std::vector<std::int16_t> audio(audio_frame_count * 2u,
	                                static_cast<std::int16_t>(sequence + 1u));
	CaptureSliceInput input{};
	input.sequence = sequence;
	input.begin_ticks = begin_ticks;
	input.end_ticks = end_ticks;
	input.audio_sample_begin = audio_sample_begin;
	input.audio_frame_count = audio_frame_count;
	input.descriptor_generation = config.descriptor_generation;
	input.video = config.video;
	input.video_pixels = pixels.data();
	input.video_bytes = pixels.size();
	input.audio = config.audio;
	input.audio_s16 = audio.data();
	return CaptureSlice::create(input);
}

static void test_owned_slice() {
	const QueueConfig config = make_config();
	std::vector<std::uint8_t> pixels(8u, 0x31u);
	std::vector<std::int16_t> audio{100, -100};
	CaptureSliceInput input{};
	input.sequence = 0u;
	input.begin_ticks = 0u;
	input.end_ticks = 10u;
	input.audio_sample_begin = 0u;
	input.audio_frame_count = 1u;
	input.descriptor_generation = config.descriptor_generation;
	input.video = config.video;
	input.video_pixels = pixels.data();
	input.video_bytes = pixels.size();
	input.audio = config.audio;
	input.audio_s16 = audio.data();
	CaptureSliceResult result = CaptureSlice::create(input);
	assert(result);
	pixels[0] = 0xe1u;
	audio[0] = 999;
	assert(result.slice->video_bytes()[0] == 0x31u);
	assert(result.slice->audio_samples()[0] == 100);
	assert(result.slice->resident_bytes() == 12u);
}

static void test_queue_validation_and_bound() {
	const QueueConfig config = make_config(1u, 128u);
	const auto created = BoundedSliceQueue::create(config);
	assert(created);
	std::unique_ptr<CaptureSlice> first = make_slice(config, 0u, 0u, 10u, 0u).slice;
	std::unique_ptr<CaptureSlice> second = make_slice(config, 1u, 10u, 20u, 1u).slice;
	assert(created.queue->try_push(first) == QueueError::None);
	assert(first == nullptr);
	assert(created.queue->try_push(second) == QueueError::WouldBlock);
	assert(second != nullptr);
	assert(created.queue->queued_slices() == 1u);
	std::unique_ptr<CaptureSlice> popped;
	assert(created.queue->wait_pop(&popped) == QueueError::None);
	assert(popped != nullptr && popped->sequence() == 0u);
	assert(created.queue->try_push(second) == QueueError::None);
	created.queue->close_producer();
	assert(created.queue->wait_pop(&popped) == QueueError::None);
	assert(popped != nullptr && popped->sequence() == 1u);
	assert(created.queue->wait_pop(&popped) == QueueError::Closed);
	assert(created.queue->reserved_bytes() == 64u);
	assert(created.queue->high_water_bytes() == 12u);

	const auto invalid_reserved = BoundedSliceQueue::create(make_config(2u, 128u));
	assert(invalid_reserved);
	QueueConfig no_slice = make_config(1u, 64u);
	no_slice.budget.active_interval_bytes = 32u;
	no_slice.budget.readback_staging_bytes = 32u;
	assert(!BoundedSliceQueue::create(no_slice));

	QueueConfig sequence_config = make_config();
	const auto sequence_queue = BoundedSliceQueue::create(sequence_config);
	std::unique_ptr<CaptureSlice> invalid = make_slice(sequence_config, 1u, 0u, 10u, 0u).slice;
	assert(sequence_queue.queue->try_push(invalid) == QueueError::SequenceGap);
	QueueConfig descriptor_config = sequence_config;
	descriptor_config.video.width = 4u;
	descriptor_config.video.stride_bytes = 8u;
	const auto descriptor_queue = BoundedSliceQueue::create(sequence_config);
	std::vector<std::uint8_t> descriptor_pixels(16u, 0x44u);
	std::vector<std::int16_t> descriptor_audio{1, 2};
	CaptureSliceInput descriptor_input{};
	descriptor_input.sequence = 0u;
	descriptor_input.begin_ticks = 0u;
	descriptor_input.end_ticks = 10u;
	descriptor_input.audio_sample_begin = 0u;
	descriptor_input.audio_frame_count = 1u;
	descriptor_input.descriptor_generation = sequence_config.descriptor_generation;
	descriptor_input.video = descriptor_config.video;
	descriptor_input.video_pixels = descriptor_pixels.data();
	descriptor_input.video_bytes = descriptor_pixels.size();
	descriptor_input.audio = sequence_config.audio;
	descriptor_input.audio_s16 = descriptor_audio.data();
	CaptureSliceResult descriptor_slice = CaptureSlice::create(descriptor_input);
	assert(descriptor_slice);
	assert(descriptor_queue.queue->try_push(descriptor_slice.slice) ==
	       QueueError::DescriptorMismatch);

	const auto gap_queue = BoundedSliceQueue::create(sequence_config);
	std::unique_ptr<CaptureSlice> interval_gap = make_slice(sequence_config, 0u, 1u, 10u, 0u).slice;
	assert(gap_queue.queue->try_push(interval_gap) == QueueError::IntervalGap);
	std::unique_ptr<CaptureSlice> audio_gap = make_slice(sequence_config, 0u, 0u, 10u, 1u).slice;
	assert(gap_queue.queue->try_push(audio_gap) == QueueError::AudioGap);
}

static void test_cancel_wakes_waiter() {
	const auto created = BoundedSliceQueue::create(make_config());
	std::atomic<int> result{static_cast<int>(QueueError::None)};
	std::thread waiter([&] {
		std::unique_ptr<CaptureSlice> slice;
		result.store(static_cast<int>(created.queue->wait_pop(&slice)));
	});
	created.queue->cancel();
	waiter.join();
	assert(static_cast<QueueError>(result.load()) == QueueError::Canceled);
}

static void test_wait_push_blocks_until_space() {
	const QueueConfig config = make_config(1u, 128u);
	const auto created = BoundedSliceQueue::create(config);
	std::unique_ptr<CaptureSlice> first = make_slice(config, 0u, 0u, 10u, 0u).slice;
	std::unique_ptr<CaptureSlice> second = make_slice(config, 1u, 10u, 20u, 1u).slice;
	assert(created.queue->try_push(first) == QueueError::None);
	std::atomic<int> result{static_cast<int>(QueueError::WouldBlock)};
	std::thread producer([&] {
		result.store(static_cast<int>(created.queue->wait_push(second)));
	});
	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	assert(second != nullptr);
	std::unique_ptr<CaptureSlice> popped;
	assert(created.queue->wait_pop(&popped) == QueueError::None);
	producer.join();
	assert(static_cast<QueueError>(result.load()) == QueueError::None);
	assert(second == nullptr);
	assert(created.queue->wait_pop(&popped) == QueueError::None);
	assert(popped != nullptr && popped->sequence() == 1u);
	created.queue->close_producer();
	assert(created.queue->wait_pop(&popped) == QueueError::Closed);
}

struct SinkState {
	std::mutex mutex;
	std::condition_variable condition;
	bool entered = false;
	bool release = true;
	bool fail = false;
	std::uint64_t writes = 0u;
};

class TestSink final : public SliceSink {
  public:
	TestSink(std::shared_ptr<SinkState> state, bool block, bool fail) noexcept
	    : state_(std::move(state)), block_(block), fail_(fail) {
	}

	QueueError write(const CaptureSlice &) noexcept override {
		std::unique_lock<std::mutex> lock(state_->mutex);
		state_->entered = true;
		++state_->writes;
		state_->condition.notify_all();
		if (block_) {
			state_->condition.wait(lock, [this] {
				return state_->release;
			});
		}
		return fail_ ? QueueError::SinkFailure : QueueError::None;
	}

	const char *error_name() const noexcept override {
		return fail_ ? "test_sink_failure" : nullptr;
	}

  private:
	std::shared_ptr<SinkState> state_;
	bool block_;
	bool fail_;
};

static void test_session_lifecycle_and_worker() {
	const QueueConfig config = make_config();
	RecordingSession preparing;
	assert(preparing.prepare(config) == QueueError::None);
	assert(preparing.state() == SessionState::Preparing);
	assert(preparing.stop() == QueueError::None);
	assert(preparing.state() == SessionState::Idle);

	RecordingSession armed;
	assert(armed.prepare(config) == QueueError::None);
	assert(armed.arm(nullptr) == QueueError::NullSink);
	assert(armed.state() == SessionState::Preparing);
	assert(armed.arm(std::unique_ptr<SliceSink>(
	           new TestSink(std::make_shared<SinkState>(), false, false))) == QueueError::None);
	assert(armed.state() == SessionState::Armed);
	assert(armed.stop() == QueueError::None);
	assert(armed.state() == SessionState::Idle);
	assert(armed.stop() == QueueError::None);

	RecordingSession normal;
	assert(normal.prepare(config) == QueueError::None);
	assert(normal.arm(std::unique_ptr<SliceSink>(
	           new TestSink(std::make_shared<SinkState>(), false, false))) == QueueError::None);
	assert(normal.begin() == QueueError::None);
	std::unique_ptr<CaptureSlice> first = make_slice(config, 0u, 0u, 10u, 0u).slice;
	std::unique_ptr<CaptureSlice> second = make_slice(config, 1u, 10u, 20u, 1u).slice;
	assert(normal.try_submit(first) == QueueError::None);
	assert(normal.try_submit(second) == QueueError::None);
	assert(normal.stop() == QueueError::None);
	assert(normal.state() == SessionState::Idle);
	assert(normal.accepted_slices() == 2u);
	assert(normal.written_slices() == 2u);
	assert(normal.stop() == QueueError::None);

	RecordingSession failing;
	assert(failing.prepare(config) == QueueError::None);
	assert(failing.arm(std::unique_ptr<SliceSink>(
	           new TestSink(std::make_shared<SinkState>(), false, true))) == QueueError::None);
	assert(failing.begin() == QueueError::None);
	std::unique_ptr<CaptureSlice> failed_slice = make_slice(config, 0u, 0u, 10u, 0u).slice;
	assert(failing.try_submit(failed_slice) == QueueError::None);
	assert(failing.stop() == QueueError::Failed);
	assert(failing.state() == SessionState::Failed);
	assert(failing.last_error() == QueueError::SinkFailure);
	assert(failing.sink_error_name() != nullptr);
	assert(std::strcmp(failing.sink_error_name(), "test_sink_failure") == 0);
	assert(failing.acknowledge_terminal() == QueueError::None);
	assert(failing.state() == SessionState::Idle);
}

static void test_finalizing_state_and_cancel() {
	const QueueConfig config = make_config();
	const auto state = std::make_shared<SinkState>();
	state->release = false;
	RecordingSession session;
	assert(session.prepare(config) == QueueError::None);
	assert(session.arm(std::unique_ptr<SliceSink>(new TestSink(state, true, false))) ==
	       QueueError::None);
	assert(session.begin() == QueueError::None);
	std::unique_ptr<CaptureSlice> slice = make_slice(config, 0u, 0u, 10u, 0u).slice;
	assert(session.try_submit(slice) == QueueError::None);
	{
		std::unique_lock<std::mutex> lock(state->mutex);
		assert(state->condition.wait_for(lock, std::chrono::seconds(1), [&] {
			return state->entered;
		}));
	}
	std::atomic<int> stop_result{static_cast<int>(QueueError::Failed)};
	std::thread stopper([&] {
		stop_result.store(static_cast<int>(session.stop()));
	});
	for (unsigned int attempt = 0u;
	     attempt < 100000u && session.state() != SessionState::Finalizing; ++attempt) {
		std::this_thread::yield();
	}
	assert(session.state() == SessionState::Finalizing);
	assert(session.stop() == QueueError::None);
	{
		std::lock_guard<std::mutex> lock(state->mutex);
		state->release = true;
	}
	state->condition.notify_all();
	stopper.join();
	assert(static_cast<QueueError>(stop_result.load()) == QueueError::None);
	assert(session.state() == SessionState::Idle);

	RecordingSession canceled;
	assert(canceled.prepare(config) == QueueError::None);
	assert(canceled.cancel() == QueueError::None);
	assert(canceled.state() == SessionState::Canceled);
	assert(canceled.acknowledge_terminal() == QueueError::None);
	assert(canceled.state() == SessionState::Idle);
}

int main() {
	test_owned_slice();
	test_queue_validation_and_bound();
	test_cancel_wakes_waiter();
	test_wait_push_blocks_until_space();
	test_session_lifecycle_and_worker();
	test_finalizing_state_and_cancel();
	return 0;
}
