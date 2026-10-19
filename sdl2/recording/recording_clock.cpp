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
#include "recording/recording_clock.hpp"

#include <limits>

namespace vaeg::recording {
namespace {

using Wide = unsigned __int128;
using SignedWide = __int128_t;

std::uint64_t gcd_u64(std::uint64_t left, std::uint64_t right) noexcept {
	while (right != 0u) {
		const std::uint64_t remainder = left % right;
		left = right;
		right = remainder;
	}
	return left;
}

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

ClockError validate_time_base(const TimeBase &base) noexcept {
	if (base.numerator == 0u || base.denominator == 0u) {
		return ClockError::ZeroDenominator;
	}
	return ClockError::None;
}

ClockError rescale_impl(std::uint64_t timestamp, const TimeBase &source,
	                       const TimeBase &destination, bool round_up,
	                       std::uint64_t *rescaled) noexcept {
	Wide numerator;
	Wide denominator;
	Wide first;
	Wide quotient;
	Wide remainder;

	if (rescaled == nullptr) {
		return ClockError::InvalidRange;
	}
	if (validate_time_base(source) != ClockError::None ||
	    validate_time_base(destination) != ClockError::None) {
		return ClockError::ZeroDenominator;
	}
	if (!wide_mul(static_cast<Wide>(timestamp), source.numerator, &first) ||
	    !wide_mul(first, destination.denominator, &numerator) ||
	    !wide_mul(static_cast<Wide>(source.denominator), destination.numerator, &denominator) ||
	    denominator == 0u) {
		return ClockError::Overflow;
	}
	quotient = numerator / denominator;
	remainder = numerator % denominator;
	if (round_up && remainder != 0u) {
		if (quotient == ~static_cast<Wide>(0u)) {
			return ClockError::Overflow;
		}
		++quotient;
	}
	if (quotient > static_cast<Wide>(std::numeric_limits<std::uint64_t>::max())) {
		return ClockError::Overflow;
	}
	*rescaled = static_cast<std::uint64_t>(quotient);
	return ClockError::None;
}

} // namespace

bool make_reduced_rational(std::uint64_t numerator, std::uint64_t denominator,
	                         Rational *result) noexcept {
	if (result == nullptr || denominator == 0u) {
		return false;
	}
	if (numerator == 0u) {
		result->numerator = 0u;
		result->denominator = 1u;
		return true;
	}
	const std::uint64_t divisor = gcd_u64(numerator, denominator);
	result->numerator = numerator / divisor;
	result->denominator = denominator / divisor;
	return true;
}

bool checked_add(std::uint64_t left, std::uint64_t right, std::uint64_t *result) noexcept {
	if (result == nullptr || left > std::numeric_limits<std::uint64_t>::max() - right) {
		return false;
	}
	*result = left + right;
	return true;
}

bool checked_sub(std::uint64_t left, std::uint64_t right, std::uint64_t *result) noexcept {
	if (result == nullptr || left < right) {
		return false;
	}
	*result = left - right;
	return true;
}

WrapExtender::WrapExtender(std::uint64_t modulus) noexcept
	: modulus_(modulus), last_(0u), epoch_(0u), initialized_(false) {
}

ClockError WrapExtender::extend(std::uint64_t raw, std::uint64_t *extended) noexcept {
	if (extended == nullptr || modulus_ < 2u || raw >= modulus_) {
		return ClockError::InvalidWrap;
	}
	if (!initialized_) {
		last_ = raw;
		initialized_ = true;
		*extended = raw;
		return ClockError::None;
	}
	if (raw < last_) {
		const std::uint64_t backwards = last_ - raw;
		if (backwards <= modulus_ / 2u) {
			return ClockError::BackwardTime;
		}
		if (epoch_ > std::numeric_limits<std::uint64_t>::max() - modulus_) {
			return ClockError::Overflow;
		}
		epoch_ += modulus_;
	}
	last_ = raw;
	if (epoch_ > std::numeric_limits<std::uint64_t>::max() - raw) {
		return ClockError::Overflow;
	}
	*extended = epoch_ + raw;
	return ClockError::None;
}

void WrapExtender::reset() noexcept {
	last_ = 0u;
	epoch_ = 0u;
	initialized_ = false;
}

bool WrapExtender::initialized() const noexcept {
	return initialized_;
}

ClockError sample_boundary_ceil(const SampleGrid &grid, std::uint64_t ticks,
	                              std::uint64_t *boundary) noexcept {
	Wide delta;
	Wide scaled_delta;
	Wide denominator;
	Wide offset_abs;
	Wide scaled_offset;
	Wide numerator;
	Wide quotient;
	Wide remainder;
	const bool negative_offset = grid.origin_sample_numerator < 0;

	if (boundary == nullptr || grid.sample_rate_hz == 0u || grid.clock_rate_hz == 0u ||
	    grid.origin_sample_denominator == 0u || ticks < grid.origin_ticks) {
		return (ticks < grid.origin_ticks) ? ClockError::InvalidRange : ClockError::ZeroDenominator;
	}
	delta = static_cast<Wide>(ticks - grid.origin_ticks);
	if (!wide_mul(delta, static_cast<Wide>(grid.sample_rate_hz), &scaled_delta) ||
	    !wide_mul(scaled_delta, static_cast<Wide>(grid.origin_sample_denominator), &scaled_delta) ||
	    !wide_mul(static_cast<Wide>(grid.clock_rate_hz),
	              static_cast<Wide>(grid.origin_sample_denominator), &denominator) ||
	    denominator == 0u) {
		return ClockError::Overflow;
	}
	const SignedWide signed_offset = static_cast<SignedWide>(grid.origin_sample_numerator);
	offset_abs = static_cast<Wide>(negative_offset ? -signed_offset : signed_offset);
	if (!wide_mul(offset_abs, static_cast<Wide>(grid.clock_rate_hz), &scaled_offset)) {
		return ClockError::Overflow;
	}
	if (negative_offset) {
		if (scaled_delta < scaled_offset) {
			return ClockError::InvalidRange;
		}
		numerator = scaled_delta - scaled_offset;
	} else if (!wide_add(scaled_delta, scaled_offset, &numerator)) {
		return ClockError::Overflow;
	}
	quotient = numerator / denominator;
	remainder = numerator % denominator;
	if (remainder != 0u) {
		++quotient;
	}
	if (quotient > static_cast<Wide>(std::numeric_limits<std::uint64_t>::max())) {
		return ClockError::Overflow;
	}
	*boundary = static_cast<std::uint64_t>(quotient);
	return ClockError::None;
}

ClockError sample_range_for_interval(const SampleGrid &grid, std::uint64_t begin_ticks,
	                                    std::uint64_t end_ticks, SampleRange *range) noexcept {
	std::uint64_t begin;
	std::uint64_t end;
	ClockError error;

	if (range == nullptr || end_ticks < begin_ticks) {
		return ClockError::InvalidRange;
	}
	error = sample_boundary_ceil(grid, begin_ticks, &begin);
	if (error != ClockError::None) {
		return error;
	}
	error = sample_boundary_ceil(grid, end_ticks, &end);
	if (error != ClockError::None || end < begin) {
		return (error == ClockError::None) ? ClockError::InvalidRange : error;
	}
	range->begin = begin;
	range->end = end;
	return ClockError::None;
}

ClockError rescale_timestamp_floor(std::uint64_t timestamp, const TimeBase &source,
	                                  const TimeBase &destination,
	                                  std::uint64_t *rescaled) noexcept {
	return rescale_impl(timestamp, source, destination, false, rescaled);
}

ClockError rescale_timestamp_ceil(std::uint64_t timestamp, const TimeBase &source,
	                                 const TimeBase &destination,
	                                 std::uint64_t *rescaled) noexcept {
	return rescale_impl(timestamp, source, destination, true, rescaled);
}

} // namespace vaeg::recording
