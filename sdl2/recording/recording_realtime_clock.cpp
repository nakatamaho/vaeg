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
#include "recording/recording_realtime_clock.hpp"
#include <cstring>
#include <limits>
#include <stdexcept>

namespace vaeg::recording {
namespace {
std::size_t checked_image_size(std::size_t bytes) {
	if (bytes == 0 || bytes > 256u * 1024u * 1024u) {
		throw std::invalid_argument("invalid_latest_image_size");
	}
	return bytes;
}
bool scaled_position(std::uint64_t elapsed_ns, std::uint32_t rate,
                     std::uint64_t *position) noexcept {
	if (position == nullptr || rate == 0) {
		return false;
	}
	/* Split before multiplying: portable to MinGW without __int128. */
	const auto seconds = elapsed_ns / 1000000000u;
	const auto fraction = (elapsed_ns % 1000000000u) * rate / 1000000000u;
	if (seconds > (std::numeric_limits<std::uint64_t>::max() - fraction) / rate) {
		return false;
	}
	*position = seconds * rate + fraction;
	return true;
}
}

HostScheduleError HostVideoSchedule::sample_position(std::uint64_t now_ns,
                                                     std::uint32_t rate,
                                                     std::uint64_t *position) const noexcept {
	if (now_ns < epoch_ns_) {
		return HostScheduleError::BeforeEpoch;
	}
	return scaled_position(now_ns - epoch_ns_, rate, position)
	           ? HostScheduleError::None : HostScheduleError::Overflow;
}

HostScheduleError HostVideoSchedule::poll(std::uint64_t now_ns, HostSample *sample) noexcept {
	if (sample == nullptr) {
		return HostScheduleError::Overflow;
	}
	*sample = {};
	if (now_ns < epoch_ns_) {
		return HostScheduleError::BeforeEpoch;
	}
	if (now_ns < last_ns_) {
		return HostScheduleError::ClockReversed;
	}
	std::uint64_t slot;
	if (!scaled_position(now_ns - epoch_ns_, 60, &slot) ||
	    slot == std::numeric_limits<std::uint64_t>::max()) {
		return HostScheduleError::Overflow;
	}
	last_ns_ = now_ns;
	if (slot >= next_slot_) {
		*sample = {slot, slot - next_slot_, true};
		next_slot_ = slot + 1;
	}
	return HostScheduleError::None;
}

LatestImageMailbox::LatestImageMailbox(std::size_t bytes) : pixels_(checked_image_size(bytes)) {
}
LatestImageResult LatestImageMailbox::publish(const std::uint8_t *pixels, std::size_t bytes,
                                             std::uint64_t slot) noexcept {
	if (pixels == nullptr || bytes != pixels_.size()) {
		return LatestImageResult::Invalid;
	}
	std::unique_lock<std::mutex> lock(mutex_, std::try_to_lock);
	if (!lock.owns_lock()) {
		return LatestImageResult::Busy;
	}
	if (has_published_ && slot <= slot_) {
		return LatestImageResult::Invalid;
	}
	const auto result = pending_ ? LatestImageResult::Replaced : LatestImageResult::Accepted;
	std::memcpy(pixels_.data(), pixels, bytes);
	slot_ = slot;
	pending_ = true;
	has_published_ = true;
	return result;
}
bool LatestImageMailbox::take(std::vector<std::uint8_t> *destination,
                              std::uint64_t *slot) noexcept {
	if (destination == nullptr || slot == nullptr || destination->size() != pixels_.size()) {
		return false;
	}
	std::lock_guard<std::mutex> lock(mutex_);
	if (!pending_) {
		return false;
	}
	std::memcpy(destination->data(), pixels_.data(), pixels_.size());
	*slot = slot_;
	pending_ = false;
	return true;
}
} // namespace vaeg::recording
