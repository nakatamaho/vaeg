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
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <stdexcept>
using namespace vaeg::recording;
static void check(bool condition, const char *code) {
	if (!condition) {
		std::fprintf(stderr, "%s\n", code);
		std::exit(1);
	}
}
int main() {
	const std::uint64_t epoch = 1234567890123;
	HostVideoSchedule clock(epoch);
	HostSample sample;
	check(clock.poll(epoch, &sample) == HostScheduleError::None && sample.due &&
	      sample.slot == 0 && sample.skipped_slots == 0, "OSR_EPOCH");
	check(clock.poll(epoch + 16666666, &sample) == HostScheduleError::None && !sample.due,
	      "OSR_EARLY");
	check(clock.poll(epoch + 16666667, &sample) == HostScheduleError::None &&
	      sample.due && sample.slot == 1, "OSR_BOUNDARY");
	/* Independent integer expected clock: exact whole seconds, including five
	 * seconds of guest pause. No call into guest, audio or renderer clocks. */
	check(clock.poll(epoch + 5000000000, &sample) == HostScheduleError::None &&
	      sample.slot == 300 && sample.skipped_slots == 298, "OSR_PAUSE_TIME");
	std::uint64_t audio;
	check(clock.sample_position(epoch + 5000000000, 48000, &audio) == HostScheduleError::None &&
	      audio == 240000, "OSR_SHARED_EPOCH");
	check(clock.poll(epoch + 4999999999, &sample) == HostScheduleError::ClockReversed,
	      "OSR_CLOCK_REVERSED");
	check(clock.poll(epoch - 1, &sample) == HostScheduleError::BeforeEpoch, "OSR_BEFORE_EPOCH");
	for (std::uint64_t second = 6; second <= 36000; ++second) {
		check(clock.poll(epoch + second * 1000000000, &sample) == HostScheduleError::None &&
		      sample.slot == second * 60 && sample.skipped_slots == 59, "OSR_LONG_CLOCK");
		check(clock.sample_position(epoch + second * 1000000000, 44100, &audio) ==
		      HostScheduleError::None && audio == second * 44100, "OSR_LONG_AUDIO_GRID");
	}
	HostVideoSchedule overflow(0);
	check(overflow.sample_position(std::numeric_limits<std::uint64_t>::max(),
	      std::numeric_limits<std::uint32_t>::max(), &audio) == HostScheduleError::Overflow,
	      "OSR_SAMPLE_OVERFLOW");
	LatestImageMailbox mailbox(12);
	std::vector<std::uint8_t> input(12, 10), output(12, 0);
	std::uint64_t slot;
	check(mailbox.publish(input.data(), input.size(), 1) == LatestImageResult::Accepted,
	      "OSR_ADMIT");
	input.assign(12, 20);
	check(mailbox.publish(input.data(), input.size(), 2) == LatestImageResult::Replaced,
	      "OSR_LATEST_REPLACEMENT");
	input.assign(12, 30);
	check(mailbox.take(&output, &slot) && slot == 2 && output[0] == 20 && output[11] == 20,
	      "OSR_OWNED_IMAGE");
	check(!mailbox.take(&output, &slot), "OSR_EMPTY_MAILBOX");
	for (std::uint64_t i = 3; i < 100003; ++i) {
		check(mailbox.publish(input.data(), input.size(), i) ==
		      (i == 3 ? LatestImageResult::Accepted : LatestImageResult::Replaced),
		      "OSR_BOUNDED_OVERLOAD");
	}
	check(mailbox.take(&output, &slot) && slot == 100002, "OSR_NEWEST_ONLY");
	check(mailbox.publish(input.data(), 11, 100003) == LatestImageResult::Invalid,
	      "OSR_SIZE_CHANGE");
	bool rejected = false;
	try { LatestImageMailbox invalid(std::numeric_limits<std::size_t>::max()); }
	catch (const std::invalid_argument &) { rejected = true; }
	check(rejected, "OSR_SIZE_LIMIT_BEFORE_ALLOCATION");
	std::puts("OSR_HOST_SCHEDULE_AND_LATEST_IMAGE_PASS");
}
