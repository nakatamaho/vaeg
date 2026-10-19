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
#ifndef VAEG_SDL2_RECORDING_CLOCK_HPP
#define VAEG_SDL2_RECORDING_CLOCK_HPP

#include <cstdint>

namespace vaeg::recording {

enum class ClockError {
	None = 0,
	ZeroDenominator,
	Overflow,
	InvalidRange,
	BackwardTime,
	InvalidWrap,
};

struct Rational {
	std::uint64_t numerator;
	std::uint64_t denominator;
};

struct GuestClock {
	std::uint64_t ticks_per_second;
	std::uint64_t wrap_modulus;
};

struct SampleGrid {
	std::uint64_t sample_rate_hz;
	std::uint64_t clock_rate_hz;
	std::uint64_t origin_ticks;
	std::int64_t origin_sample_numerator;
	std::uint64_t origin_sample_denominator;
};

struct TimeBase {
	std::uint64_t numerator;
	std::uint64_t denominator;
};

struct SampleRange {
	std::uint64_t begin;
	std::uint64_t end;

	std::uint64_t count() const noexcept {
		return end - begin;
	}
};

bool make_reduced_rational(std::uint64_t numerator, std::uint64_t denominator,
	                         Rational *result) noexcept;
bool checked_add(std::uint64_t left, std::uint64_t right, std::uint64_t *result) noexcept;
bool checked_sub(std::uint64_t left, std::uint64_t right, std::uint64_t *result) noexcept;

class WrapExtender {
  public:
	 explicit WrapExtender(std::uint64_t modulus) noexcept;

	 ClockError extend(std::uint64_t raw, std::uint64_t *extended) noexcept;
	 void reset() noexcept;
	 bool initialized() const noexcept;

  private:
	std::uint64_t modulus_;
	std::uint64_t last_;
	std::uint64_t epoch_;
	bool initialized_;
};

ClockError sample_boundary_ceil(const SampleGrid &grid, std::uint64_t ticks,
	                              std::uint64_t *boundary) noexcept;
ClockError sample_range_for_interval(const SampleGrid &grid, std::uint64_t begin_ticks,
	                                    std::uint64_t end_ticks, SampleRange *range) noexcept;

ClockError rescale_timestamp_floor(std::uint64_t timestamp, const TimeBase &source,
	                                  const TimeBase &destination,
	                                  std::uint64_t *rescaled) noexcept;
ClockError rescale_timestamp_ceil(std::uint64_t timestamp, const TimeBase &source,
	                                 const TimeBase &destination,
	                                 std::uint64_t *rescaled) noexcept;

} // namespace vaeg::recording

#endif
