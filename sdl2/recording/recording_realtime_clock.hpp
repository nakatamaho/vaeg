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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef VAEG_RECORDING_REALTIME_CLOCK_HPP
#define VAEG_RECORDING_REALTIME_CLOCK_HPP

#include <cstdint>
#include <mutex>
#include <vector>

namespace vaeg::recording {

enum class HostScheduleError { None, BeforeEpoch, ClockReversed, Overflow };
struct HostSample {
	std::uint64_t slot = 0;
	std::uint64_t skipped_slots = 0;
	bool due = false;
};

/* Absolute monotonic nanoseconds, not civil time or guest ticks. Video PTS uses
 * slot units (1/60 second). Audio must be mapped to the SAME epoch elsewhere.
 * After a delayed poll only the newest due slot is admitted: earlier slots are
 * counted, never executed as extra guest/shader frames. A worker may repeat its
 * last owned image to construct CFR output; gaps must not shorten duration. */
class HostVideoSchedule {
  public:
	explicit HostVideoSchedule(std::uint64_t epoch_ns) : epoch_ns_(epoch_ns), last_ns_(epoch_ns) {}
	HostScheduleError poll(std::uint64_t now_ns, HostSample *sample) noexcept;
	HostScheduleError sample_position(std::uint64_t now_ns, std::uint32_t rate,
	                                  std::uint64_t *position) const noexcept;
  private:
	std::uint64_t epoch_ns_;
	std::uint64_t last_ns_;
	std::uint64_t next_slot_ = 0;
};

enum class LatestImageResult { Accepted, Replaced, Busy, Invalid };
/* Single pending owned image. All storage allocated at preparation; producer
 * try-locks, copies, and returns without waiting. Never queues encoded packets.
 * Caller retains the last complete image for idle/paused sampling. Not an audio
 * callback adapter; playback clock and audio-loss handling are separate. */
class LatestImageMailbox {
  public:
	explicit LatestImageMailbox(std::size_t bytes);
	LatestImageResult publish(const std::uint8_t *pixels, std::size_t bytes,
	                          std::uint64_t slot) noexcept;
	bool take(std::vector<std::uint8_t> *destination, std::uint64_t *slot) noexcept;
  private:
	std::mutex mutex_;
	std::vector<std::uint8_t> pixels_;
	std::uint64_t slot_ = 0;
	bool pending_ = false;
};

} // namespace vaeg::recording
#endif
