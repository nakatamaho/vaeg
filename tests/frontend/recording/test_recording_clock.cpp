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
#include <cassert>
#include <cstdint>
#include <limits>
#include <vector>

#include "recording/recording_clock.hpp"

using vaeg::recording::ClockError;
using vaeg::recording::SampleGrid;
using vaeg::recording::SampleRange;
using vaeg::recording::TimeBase;

static void test_sample_partition_invariance() {
	const SampleGrid grid{48000u, 1001u, 0u, 0, 1u};
	const std::uint64_t end_ticks = 1001u * 60u * 60u * 3u;
	SampleRange whole;
	std::uint64_t partitioned = 0u;
	std::uint64_t previous = 0u;
	assert(vaeg::recording::sample_range_for_interval(grid, 0u, end_ticks, &whole) ==
	       ClockError::None);
	for (std::uint64_t tick = 0u; tick < end_ticks; tick += 17u) {
		const std::uint64_t next = (tick + 17u < end_ticks) ? tick + 17u : end_ticks;
		SampleRange piece;
		assert(vaeg::recording::sample_range_for_interval(grid, previous, next, &piece) ==
		       ClockError::None);
		partitioned += piece.count();
		previous = next;
		if (next == end_ticks) break;
	}
	assert(partitioned == whole.count());
	assert(whole.count() == 518400000u);
}

static void test_origin_and_invalid_inputs() {
	const SampleGrid grid{3u, 10u, 100u, 1, 2u};
	std::uint64_t boundary = 0u;
	assert(vaeg::recording::sample_boundary_ceil(grid, 100u, &boundary) == ClockError::None);
	assert(boundary == 1u);
	assert(vaeg::recording::sample_boundary_ceil(grid, 101u, &boundary) == ClockError::None);
	assert(boundary == 1u);
	assert(vaeg::recording::sample_boundary_ceil(grid, 102u, &boundary) == ClockError::None);
	assert(boundary == 2u);
	assert(vaeg::recording::sample_boundary_ceil(grid, 99u, &boundary) ==
	       ClockError::InvalidRange);
	const SampleGrid invalid{0u, 10u, 0u, 0, 1u};
	assert(vaeg::recording::sample_boundary_ceil(invalid, 0u, &boundary) ==
	       ClockError::ZeroDenominator);
}

static void test_wrap_extension() {
	vaeg::recording::WrapExtender extender(256u);
	std::uint64_t value = 0u;
	assert(extender.extend(250u, &value) == ClockError::None && value == 250u);
	assert(extender.extend(2u, &value) == ClockError::None && value == 258u);
	assert(extender.extend(3u, &value) == ClockError::None && value == 259u);
	assert(extender.extend(1u, &value) == ClockError::BackwardTime);
	extender.reset();
	assert(!extender.initialized());
}

static void test_rescale() {
	const TimeBase milliseconds{1u, 1000u};
	const TimeBase samples{1u, 48000u};
	std::uint64_t result = 0u;
	assert(vaeg::recording::rescale_timestamp_floor(1u, milliseconds, samples, &result) ==
	       ClockError::None);
	assert(result == 48u);
	assert(vaeg::recording::rescale_timestamp_ceil(1u, samples, milliseconds, &result) ==
	       ClockError::None);
	assert(result == 1u);
	const TimeBase invalid{0u, 1u};
	assert(vaeg::recording::rescale_timestamp_floor(1u, invalid, milliseconds, &result) ==
	       ClockError::ZeroDenominator);
	assert(vaeg::recording::rescale_timestamp_floor(std::numeric_limits<std::uint64_t>::max(),
	                                                milliseconds, samples, &result) ==
	       ClockError::Overflow);
}

int main() {
	test_sample_partition_invariance();
	test_origin_and_invalid_inputs();
	test_wrap_extension();
	test_rescale();
	return 0;
}
